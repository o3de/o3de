/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#include <AzCore/IO/Path/Path.h>
#include <AzToolsFramework/Prefab/PrefabLoader.h>

#include <AzFramework/FileFunc/FileFunc.h>
#include <AzToolsFramework/Prefab/PrefabDomUtils.h>
#include <AzToolsFramework/Prefab/PrefabSystemComponentInterface.h>

namespace AzToolsFramework
{
    namespace Prefab
    {
        void PrefabLoader::RegisterPrefabLoaderInterface()
        {
            m_prefabSystemComponentInterface = AZ::Interface<PrefabSystemComponentInterface>::Get();
            AZ_Assert(
                m_prefabSystemComponentInterface != nullptr,
                "Prefab System Component Interface could not be found. "
                "It is a requirement for the PrefabLoader class. "
                "Check that it is being correctly initialized.");

            AZ::Interface<PrefabLoaderInterface>::Register(this);
        }

        void PrefabLoader::UnregisterPrefabLoaderInterface()
        {
            AZ::Interface<PrefabLoaderInterface>::Unregister(this);
        }

        TemplateId PrefabLoader::LoadTemplate(const AZStd::string& filePath)
        {
            AZStd::unordered_set<AZStd::string> progressedFilePathsSet;
            TemplateId newTemplateId = LoadTemplate(filePath, progressedFilePathsSet);
            return newTemplateId;
        }

        TemplateId PrefabLoader::LoadTemplate(
            const AZStd::string& filePath,
            AZStd::unordered_set<AZStd::string>& progressedFilePathsSet)
        {
            // Cyclical dependency detected if the prefab file is already part of the progressed
            // file path set.
            if (progressedFilePathsSet.contains(filePath))
            {
                AZ_Error("Prefab", false,
                    "PrefabLoader::LoadTemplate - "
                    "Prefab file %s has been detected to directly or indirectly depend on itself."
                    "Terminating any further loading of this branch of its prefab hierarchy.",
                    filePath.c_str());
                return InvalidTemplateId;
            }

            // Directly return loaded Template id.
            TemplateId loadedTemplateId = m_prefabSystemComponentInterface->GetTemplateIdFromFilePath(filePath);
            if (loadedTemplateId != InvalidTemplateId)
            {
                return loadedTemplateId;
            }

            // Read Template's prefab file from disk and parse Prefab DOM from file.
            AZ::Outcome<PrefabDom, AZStd::string> readPrefabFileResult = AzFramework::FileFunc::ReadJsonFile(AZ::IO::Path(filePath));
            if (!readPrefabFileResult.IsSuccess())
            {
                AZ_Error("Prefab", false,
                    "PrefabLoader::LoadPrefabFile - Failed to load Prefab file from '%s'."
                    "Error message: '%s'",
                    filePath.c_str(), readPrefabFileResult.GetError().c_str());

                return InvalidTemplateId;
            }

            // Create new Template with the Prefab DOM.
            TemplateId newTemplateId = m_prefabSystemComponentInterface->AddTemplate(filePath, readPrefabFileResult.TakeValue());
            if (newTemplateId == InvalidTemplateId)
            {
                AZ_Error("Prefab", false,
                    "PrefabLoader::LoadTemplate - "
                    "Failed to create a template from instance with source file path '%s': "
                    "invalid template id returned.",
                    filePath.c_str());

                return InvalidTemplateId;
            }

            TemplateReference newTemplateReference = m_prefabSystemComponentInterface->FindTemplate(newTemplateId);
            Template& newTemplate = newTemplateReference->get();

            // Mark the file as being in progress.
            progressedFilePathsSet.emplace(filePath);

            // Get 'Instances' value from Template.
            bool isLoadedWithErrors = false;
            PrefabDomValueReference instancesReference = newTemplate.GetInstancesValue();
            if (instancesReference.has_value())
            {
                PrefabDomValue& instances = instancesReference->get();

                // For each instance value in 'instances', try to create source Templates for target Template's nested instance data.
                // Also create Links between source/target Templates if source Template loaded successfully.
                for (PrefabDomValue::MemberIterator instanceIterator = instances.MemberBegin(); instanceIterator != instances.MemberEnd(); ++instanceIterator)
                {
                    const PrefabDomValue& instance = instanceIterator->value;
                    if (!LoadNestedInstance(instanceIterator, newTemplateId, progressedFilePathsSet))
                    {
                        isLoadedWithErrors = true;
                        AZ_Error("Prefab", false,
                            "PrefabLoader::LoadTemplate - "
                            "Loading nested instance '%s' in target Template '%u' from Prefab file '%s' failed.",
                            instanceIterator->name.GetString(),
                            newTemplateId,
                            filePath.c_str());
                    }
                }
            }
            newTemplate.MarkAsLoadedWithErrors(isLoadedWithErrors);

            // Un-mark the file as being in progress.
            progressedFilePathsSet.erase(filePath);

            // Return target Template id.
            return newTemplateId;
        }

        bool PrefabLoader::LoadNestedInstance(
            PrefabDomValue::MemberIterator& instanceIterator,
            TemplateId targetTemplateId,
            AZStd::unordered_set<AZStd::string>& progressedFilePathsSet)
        {
            const PrefabDomValue& instance = instanceIterator->value;

            if (instanceIterator->name.GetStringLength() == 0)
            {
                AZ_Error("Prefab", false,
                    "PrefabLoader::LoadNestedInstance - "
                    "There's an Instance without a name in the target Template on file path '%s'.",
                    instanceIterator->name.GetString(),
                    m_prefabSystemComponentInterface->FindTemplate(targetTemplateId)->get().GetFilePath().c_str());
                return false;
            }

            // Get source Template's path for getting nested instance data.
            PrefabDomValueConstReference sourceReference =
                PrefabDomUtils::FindPrefabDomValue(instance, PrefabDomUtils::SourceName);
            if (!sourceReference.has_value() ||
                !sourceReference->get().IsString() ||
                sourceReference->get().GetStringLength() == 0)
            {
                AZ_Error("Prefab", false,
                    "PrefabLoader::LoadNestedInstance - "
                    "Can't get '%s' string value in Instance value '%s'of Template's Prefab DOM from file '%s'.",
                    PrefabDomUtils::SourceName, instanceIterator->name.GetString(),
                    m_prefabSystemComponentInterface->FindTemplate(targetTemplateId)->get().GetFilePath().c_str());
                return false;
            }
            const PrefabDomValue& source = sourceReference->get();
            AZStd::string_view nestedTemplatePath(source.GetString(), source.GetStringLength());

            // Get Template id of nested instance from its path.
            // If source Template is already loaded, get the id from Template File Path To Id Map, 
            // else load the source Template by calling 'LoadTemplate' recursively.
            TemplateId nestedTemplateId = LoadTemplate(nestedTemplatePath, progressedFilePathsSet);
            TemplateReference nestedTemplateReference = m_prefabSystemComponentInterface->FindTemplate(nestedTemplateId);
            if (!nestedTemplateReference.has_value() ||
                !nestedTemplateReference->get().IsValid())
            {
                AZ_Error("Prefab", false,
                    "PrefabLoader::LoadNestedInstance - "
                    "Error occurred while loading nested Prefab file '%.*s' from Prefab file '%s'.",
                    aznumeric_cast<int>(nestedTemplatePath.size()), nestedTemplatePath.data(),
                    m_prefabSystemComponentInterface->FindTemplate(targetTemplateId)->get().GetFilePath().c_str());
                return false;
            }

            // After source template has been loaded, create Link between source/target Template.
            LinkId newLinkId = m_prefabSystemComponentInterface->AddLink(nestedTemplateId, targetTemplateId, instanceIterator, AZStd::nullopt);
            if (newLinkId == InvalidLinkId)
            {
                AZ_Error("Prefab", false,
                    "PrefabLoader::LoadNestedInstance - "
                    "Failed to add a new Link to Nested Template Instance '%s' which connects source Template '%.*s' and target Template '%s'.",
                    instanceIterator->name.GetString(),
                    aznumeric_cast<int>(nestedTemplatePath.size()), nestedTemplatePath.data(),
                    m_prefabSystemComponentInterface->FindTemplate(targetTemplateId)->get().GetFilePath().c_str());

                return false;
            }

            // Let the new Template carry up the error flag of its nested Prefab.
            return !nestedTemplateReference->get().IsLoadedWithErrors();
        }

        bool PrefabLoader::SaveTemplate(const TemplateId& templateId)
        {
            // Acquire the template we are saving
            TemplateReference templateToSaveReference = m_prefabSystemComponentInterface->FindTemplate(templateId);
            if (!templateToSaveReference.has_value())
            {
                AZ_Warning("Prefab", false,
                    "PrefabLoader::SaveTemplate - Unable to save prefab template with id: '%llu'. "
                    "Template with that id could not be found",
                    templateId);

                return false;
            }

            Template& templateToSave = templateToSaveReference->get();
            if (!templateToSave.IsValid())
            {
                AZ_Warning("Prefab", false,
                    "PrefabLoader::SaveTemplate - Unable to save Prefab Template with id: %llu. "
                    "Template with that id is invalid",
                    templateId);

                return false;
            }

            // Make a copy of a our prefab DOM where nested instances become file references with patch data
            PrefabDom templateDomToSave;
            if (!templateToSave.CopyTemplateIntoPrefabFileFormat(templateDomToSave))
            {
                AZ_Error("Prefab", false,
                    "PrefabLoader::SaveTemplate - Unable to store a collapsed version of prefab Template while attempting to save to %s"
                    "Save cannot continue",
                    templateToSave.GetFilePath().c_str());

                return false;
            }

            // Save the file
            return AzFramework::FileFunc::WriteJsonFile(templateDomToSave, templateToSave.GetFilePath()).IsSuccess();
        }

    } // namespace Prefab
} // namespace AzToolsFramework
