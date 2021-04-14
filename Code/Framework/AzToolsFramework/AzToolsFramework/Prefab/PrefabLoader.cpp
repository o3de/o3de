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


#include <AzToolsFramework/Prefab/PrefabLoader.h>

#include <AzCore/Component/Entity.h>
#include <AzCore/IO/Path/Path.h>
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>
#include <AzCore/StringFunc/StringFunc.h>

#include <AzFramework/FileFunc/FileFunc.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
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

            auto settingsRegistry = AZ::SettingsRegistry::Get();
            AZ_Assert(settingsRegistry, "Settings registry is not set");
            
            [[maybe_unused]] bool result =
                settingsRegistry->Get(m_projectPathWithOsSeparator.Native(), AZ::SettingsRegistryMergeUtils::FilePathKey_ProjectPath);
            AZ_Assert(result, "Couldn't retrieve project root path");
            m_projectPathWithSlashSeparator = AZ::IO::Path(m_projectPathWithOsSeparator.Native(), '/').MakePreferred();

            AZ::Interface<PrefabLoaderInterface>::Register(this);
        }

        void PrefabLoader::UnregisterPrefabLoaderInterface()
        {
            AZ::Interface<PrefabLoaderInterface>::Unregister(this);
        }

        TemplateId PrefabLoader::LoadTemplateFromFile(AZ::IO::PathView filePath)
        {
            AZStd::unordered_set<AZ::IO::Path> progressedFilePathsSet;
            TemplateId newTemplateId = LoadTemplateFromFile(filePath, progressedFilePathsSet);
            return newTemplateId;
        }

        TemplateId PrefabLoader::LoadTemplateFromFile(AZ::IO::PathView filePath, AZStd::unordered_set<AZ::IO::Path>& progressedFilePathsSet)
        {
            if (!IsValidPrefabPath(filePath))
            {
                AZ_Error(
                    "Prefab", false,
                    "PrefabLoader::LoadTemplateFromFile - "
                    "Invalid file path: '%.*s'.",
                    AZ_STRING_ARG(filePath.Native())
                );
                return InvalidTemplateId;
            }

            auto readResult = AZ::Utils::ReadFile(GetFullPath(filePath).Native(), MaxPrefabFileSize);
            if (!readResult.IsSuccess())
            {
                AZ_Error(
                    "Prefab", false,
                    "PrefabLoader::LoadTemplate - Failed to load Prefab file from '%.*s'."
                    "Error message: '%s'",
                    AZ_STRING_ARG(filePath.Native()),
                    readResult.GetError().c_str()
                );
                return InvalidTemplateId;
            }

            return LoadTemplateFromString(readResult.GetValue(), filePath, progressedFilePathsSet);
        }

        TemplateId PrefabLoader::LoadTemplateFromString(
            AZStd::string_view content, AZ::IO::PathView originPath)
        {
            AZStd::unordered_set<AZ::IO::Path> progressedFilePathsSet;
            TemplateId newTemplateId = LoadTemplateFromString(content, originPath, progressedFilePathsSet);
            return newTemplateId;
        }

        TemplateId PrefabLoader::LoadTemplateFromString(
            AZStd::string_view fileContent,
            AZ::IO::PathView originPath,
            AZStd::unordered_set<AZ::IO::Path>& progressedFilePathsSet)
        {
            if (!IsValidPrefabPath(originPath))
            {
                AZ_Error(
                    "Prefab", false,
                    "PrefabLoader::LoadTemplateFromString - "
                    "Invalid origin path: '%.*s'",
                    AZ_STRING_ARG(originPath.Native())
                );
                return InvalidTemplateId;
            }

            AZ::IO::Path relativePath = GetRelativePathToProject(originPath);

            // Cyclical dependency detected if the prefab file is already part of the progressed
            // file path set.
            if (progressedFilePathsSet.contains(relativePath))
            {
                AZ_Error(
                    "Prefab", false,
                    "PrefabLoader::LoadTemplateFromString - "
                    "Prefab file '%.*s' has been detected to directly or indirectly depend on itself."
                    "Terminating any further loading of this branch of its prefab hierarchy.",
                    AZ_STRING_ARG(originPath.Native())
                );
                return InvalidTemplateId;
            }

            // Directly return loaded Template id.
            TemplateId loadedTemplateId = m_prefabSystemComponentInterface->GetTemplateIdFromFilePath(relativePath);
            if (loadedTemplateId != InvalidTemplateId)
            {
                return loadedTemplateId;
            }

            // Read Template's prefab file from disk and parse Prefab DOM from file.
            AZ::Outcome<PrefabDom, AZStd::string> readPrefabFileResult = AzFramework::FileFunc::ReadJsonFromString(fileContent);
            if (!readPrefabFileResult.IsSuccess())
            {
                AZ_Error(
                    "Prefab", false,
                    "PrefabLoader::LoadTemplate - Failed to load Prefab file from '%.*s'."
                    "Error message: '%s'",
                    AZ_STRING_ARG(originPath.Native()),
                    readPrefabFileResult.GetError().c_str());

                return InvalidTemplateId;
            }

            // Create new Template with the Prefab DOM.
            TemplateId newTemplateId = m_prefabSystemComponentInterface->AddTemplate(relativePath, readPrefabFileResult.TakeValue());
            if (newTemplateId == InvalidTemplateId)
            {
                AZ_Error(
                    "Prefab", false,
                    "PrefabLoader::LoadTemplate - "
                    "Failed to create a template from instance with source file path '%.*s': "
                    "invalid template id returned.",
                    AZ_STRING_ARG(originPath.Native())
                );

                return InvalidTemplateId;
            }

            TemplateReference newTemplateReference = m_prefabSystemComponentInterface->FindTemplate(newTemplateId);
            Template& newTemplate = newTemplateReference->get();

            // Mark the file as being in progress.
            progressedFilePathsSet.emplace(relativePath);

            // Get 'Instances' value from Template.
            bool isLoadedWithErrors = false;
            PrefabDomValueReference instancesReference = newTemplate.GetInstancesValue();
            if (instancesReference.has_value())
            {
                PrefabDomValue& instances = instancesReference->get();

                // For each instance value in 'instances', try to create source Templates for target Template's nested instance data.
                // Also create Links between source/target Templates if source Template loaded successfully.
                for (PrefabDomValue::MemberIterator instanceIterator = instances.MemberBegin(); instanceIterator != instances.MemberEnd();
                     ++instanceIterator)
                {
                    if (!LoadNestedInstance(instanceIterator, newTemplateId, progressedFilePathsSet))
                    {
                        isLoadedWithErrors = true;
                        AZ_Error(
                            "Prefab", false,
                            "PrefabLoader::LoadTemplate - "
                            "Loading nested instance '%s' in target Template '%u' from Prefab file '%.*s' failed.",
                            instanceIterator->name.GetString(), newTemplateId,
                            AZ_STRING_ARG(originPath.Native())
                        );
                    }
                }
            }
            newTemplate.MarkAsLoadedWithErrors(isLoadedWithErrors);

            // Un-mark the file as being in progress.
            progressedFilePathsSet.erase(originPath);

            // Return target Template id.
            return newTemplateId;
        }

        bool PrefabLoader::LoadNestedInstance(
            PrefabDomValue::MemberIterator& instanceIterator, TemplateId targetTemplateId,
            AZStd::unordered_set<AZ::IO::Path>& progressedFilePathsSet)
        {
            const PrefabDomValue& instance = instanceIterator->value;
            AZ::IO::PathView instancePath = AZStd::string_view(instanceIterator->name.GetString(), instanceIterator->name.GetStringLength());

            if (!IsValidPrefabPath(instancePath))
            {
                AZ_Error(
                    "Prefab", false,
                    "PrefabLoader::LoadNestedInstance - "
                    "There's an Instance with an invalid path '%s' in the target Template on file path '%s'.",
                    instanceIterator->name.GetString(),
                    m_prefabSystemComponentInterface->FindTemplate(targetTemplateId)->get().GetFilePath().c_str());
                return false;
            }

            // Get source Template's path for getting nested instance data.
            PrefabDomValueConstReference sourceReference = PrefabDomUtils::FindPrefabDomValue(instance, PrefabDomUtils::SourceName);
            if (!sourceReference.has_value() || !sourceReference->get().IsString() || sourceReference->get().GetStringLength() == 0)
            {
                AZ_Error(
                    "Prefab", false,
                    "PrefabLoader::LoadNestedInstance - "
                    "Can't get '%s' string value in Instance value '%s' of Template's Prefab DOM from file '%s'.",
                    PrefabDomUtils::SourceName, instanceIterator->name.GetString(),
                    m_prefabSystemComponentInterface->FindTemplate(targetTemplateId)->get().GetFilePath().c_str());
                return false;
            }
            const PrefabDomValue& source = sourceReference->get();
            AZStd::string_view nestedTemplatePath(source.GetString(), source.GetStringLength());

            // Get Template id of nested instance from its path.
            // If source Template is already loaded, get the id from Template File Path To Id Map,
            // else load the source Template by calling 'LoadTemplate' recursively.

            TemplateId nestedTemplateId = LoadTemplateFromFile(nestedTemplatePath, progressedFilePathsSet);
            TemplateReference nestedTemplateReference = m_prefabSystemComponentInterface->FindTemplate(nestedTemplateId);
            if (!nestedTemplateReference.has_value() || !nestedTemplateReference->get().IsValid())
            {
                AZ_Error(
                    "Prefab", false,
                    "PrefabLoader::LoadNestedInstance - "
                    "Error occurred while loading nested Prefab file '%.*s' from Prefab file '%s'.",
                    AZ_STRING_ARG(nestedTemplatePath),
                    m_prefabSystemComponentInterface->FindTemplate(targetTemplateId)->get().GetFilePath().c_str()
                );
                return false;
            }

            // After source template has been loaded, create Link between source/target Template.
            LinkId newLinkId =
                m_prefabSystemComponentInterface->AddLink(nestedTemplateId, targetTemplateId, instanceIterator, AZStd::nullopt);
            if (newLinkId == InvalidLinkId)
            {
                AZ_Error(
                    "Prefab", false,
                    "PrefabLoader::LoadNestedInstance - "
                    "Failed to add a new Link to Nested Template Instance '%s' which connects source Template '%.*s' and target Template "
                    "'%s'.",
                    instanceIterator->name.GetString(), AZ_STRING_ARG(nestedTemplatePath),
                    m_prefabSystemComponentInterface->FindTemplate(targetTemplateId)->get().GetFilePath().c_str()
                );

                return false;
            }

            // Let the new Template carry up the error flag of its nested Prefab.
            return !nestedTemplateReference->get().IsLoadedWithErrors();
        }

        bool PrefabLoader::SaveTemplate(TemplateId templateId)
        {
            const auto& domAndFilepath = StoreTemplateIntoFileFormat(templateId);
            if (!domAndFilepath)
            {
                return false;
            }

            auto outcome = AzFramework::FileFunc::WriteJsonFile(domAndFilepath->first, GetFullPath(domAndFilepath->second));
            if (!outcome.IsSuccess())
            {
                AZ_Error(
                    "Prefab", false,
                    "PrefabLoader::SaveTemplate - "
                    "Failed to save template '%s'."
                    "Error: %s",
                    domAndFilepath->second.c_str(),
                    outcome.GetError().c_str()
                );
                return false;
            }
            m_prefabSystemComponentInterface->SetTemplateDirtyFlag(templateId, false);
            return true;
        }

        bool PrefabLoader::SaveTemplateToString(TemplateId templateId, AZStd::string& output)
        {
            const auto& domAndFilepath = StoreTemplateIntoFileFormat(templateId);
            if (!domAndFilepath)
            {
                return false;
            }

            auto outcome = AzFramework::FileFunc::WriteJsonToString(domAndFilepath->first, output);
            if (!outcome.IsSuccess())
            {
                AZ_Error(
                    "Prefab", false,
                    "PrefabLoader::SaveTemplateToString - "
                    "Failed to serialize template '%s' into a string."
                    "Error: %s",
                    domAndFilepath->second.c_str(),
                    outcome.GetError().c_str()
                );
                return false;
            }
            return true;
        }

        AZStd::optional<AZStd::pair<PrefabDom, AZ::IO::Path>> PrefabLoader::StoreTemplateIntoFileFormat(TemplateId templateId)
        {
            // Acquire the template we are saving
            TemplateReference templateToSaveReference = m_prefabSystemComponentInterface->FindTemplate(templateId);
            if (!templateToSaveReference.has_value())
            {
                AZ_Warning(
                    "Prefab", false,
                    "PrefabLoader::SaveTemplate - Unable to save prefab template with id: '%llu'. "
                    "Template with that id could not be found",
                    templateId
                );

                return AZStd::nullopt;
            }

            Template& templateToSave = templateToSaveReference->get();
            if (!templateToSave.IsValid())
            {
                AZ_Warning(
                    "Prefab", false,
                    "PrefabLoader::SaveTemplate - Unable to save Prefab Template with id: %llu. "
                    "Template with that id is invalid",
                    templateId);

                return AZStd::nullopt;
            }

            // Make a copy of a our prefab DOM where nested instances become file references with patch data
            PrefabDom templateDomToSave;
            if (!templateToSave.CopyTemplateIntoPrefabFileFormat(templateDomToSave))
            {
                AZ_Error(
                    "Prefab", false,
                    "PrefabLoader::SaveTemplate - Unable to store a collapsed version of prefab Template while attempting to save to %s."
                    "Save cannot continue",
                    templateToSave.GetFilePath().c_str()
                );

                return AZStd::nullopt;
            }

            return { { AZStd::move(templateDomToSave), templateToSave.GetFilePath() } };
        }

        bool PrefabLoader::IsValidPrefabPath(AZ::IO::PathView path)
        {
            // Check for OS invalid character and paths ending on '/' '\\' separators as final char
            AZStd::string_view pathStr = path.Native();

            return !path.empty() &&
                (pathStr.find_first_of(AZ_FILESYSTEM_INVALID_CHARACTERS) == AZStd::string::npos) &&
                (pathStr.back() != '\\' && pathStr.back() != '/');
        }

        AZ::IO::Path PrefabLoader::GetFullPath(AZ::IO::PathView path)
        {
            AZ::IO::Path pathWithOSSeparator = AZ::IO::Path(path).MakePreferred();
            if (pathWithOSSeparator.IsAbsolute())
            {
                return path;
            }

            return AZ::IO::Path(m_projectPathWithOsSeparator).Append(pathWithOSSeparator);
        }

        AZ::IO::Path PrefabLoader::GetRelativePathToProject(AZ::IO::PathView path)
        {
            AZ::IO::Path pathWithOSSeparator = AZ::IO::Path(path.Native()).MakePreferred();
            if (!pathWithOSSeparator.IsAbsolute())
            {
                return path;
            }

            return AZ::IO::Path(path.Native(), '/').MakePreferred().LexicallyRelative(m_projectPathWithSlashSeparator);
        }

        AZ::IO::Path PrefabLoaderInterface::GeneratePath()
        {
            return AZStd::string::format("Prefab_%s", AZ::Entity::MakeId().ToString().c_str());
        }

    } // namespace Prefab
} // namespace AzToolsFramework
