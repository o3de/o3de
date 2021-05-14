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

#include <AzCore/Asset/AssetManager.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/Component/EntityUtils.h>
#include <AzCore/Debug/Trace.h>
#include <AzCore/JSON/prettywriter.h>
#include <AzCore/Module/Module.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/Utils.h>
#include <AzCore/Serialization/Json/JsonSerialization.h>
#include <AzCore/Settings/SettingsRegistryImpl.h>
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>
#include <AzCore/std/containers/unordered_set.h>
#include <AzCore/Utils/Utils.h>
#include <AzFramework/Archive/IArchive.h>
#include <AzToolsFramework/Prefab/PrefabDomUtils.h>
#include <AzToolsFramework/Prefab/EditorPrefabComponent.h>
#include <AzToolsFramework/Entity/EditorEntityContextBus.h>
#include <AzToolsFramework/Entity/PrefabEditorEntityOwnershipInterface.h>
#include <AzToolsFramework/ToolsComponents/TransformComponent.h>
#include <Application.h>
#include <SliceConverter.h>
#include <Utilities.h>

// SliceConverter reads in a slice file (saved in an ObjectStream format), instantiates it, creates a prefab out of the data,
// and saves the prefab in a JSON format.  This can be used for one-time migrations of slices or slice-based levels to prefabs.
// This converter is still in an early state.  It can convert trivial slices, but it cannot handle nested slices yet.
// 
// If the slice contains legacy data, it will print out warnings / errors about the data that couldn't be serialized.
// The prefab will be generated without that data.

namespace AZ
{
    namespace SerializeContextTools
    {
        bool SliceConverter::ConvertSliceFiles(Application& application)
        {
            using namespace AZ::JsonSerializationResult;

            const AZ::CommandLine* commandLine = application.GetAzCommandLine();
            if (!commandLine)
            {
                AZ_Error("SerializeContextTools", false, "Command line not available.");
                return false;
            }
            
            JsonSerializerSettings convertSettings;
            convertSettings.m_keepDefaults = commandLine->HasSwitch("keepdefaults");
            convertSettings.m_registrationContext = application.GetJsonRegistrationContext();
            convertSettings.m_serializeContext = application.GetSerializeContext();
            if (!convertSettings.m_serializeContext)
            {
                AZ_Error("Convert-Slice", false, "No serialize context found.");
                return false;
            }
            if (!convertSettings.m_registrationContext)
            {
                AZ_Error("Convert-Slice", false, "No json registration context found.");
                return false;
            }
            AZStd::string logggingScratchBuffer;
            SetupLogging(logggingScratchBuffer, convertSettings.m_reporting, *commandLine);

            bool isDryRun = commandLine->HasSwitch("dryrun");

            JsonDeserializerSettings verifySettings;
            verifySettings.m_registrationContext = application.GetJsonRegistrationContext();
            verifySettings.m_serializeContext = application.GetSerializeContext();
            SetupLogging(logggingScratchBuffer, verifySettings.m_reporting, *commandLine);

            auto archiveInterface = AZ::Interface<AZ::IO::IArchive>::Get();

            // Find the Prefab System Component for use in creating and saving the prefab
            AZ::Entity* systemEntity = application.FindEntity(AZ::SystemEntityId);
            AZ_Assert(systemEntity != nullptr, "System entity doesn't exist.");
            auto prefabSystemComponent = systemEntity->FindComponent<AzToolsFramework::Prefab::PrefabSystemComponent>();
            AZ_Assert(prefabSystemComponent != nullptr, "Prefab System component doesn't exist");

            bool result = true;
            rapidjson::StringBuffer scratchBuffer;

            AZStd::vector<AZStd::string> fileList = Utilities::ReadFileListFromCommandLine(application, "files");
            for (AZStd::string& filePath : fileList)
            {
                bool packOpened = false;

                AZ::IO::Path outputPath = filePath;
                outputPath.ReplaceExtension("prefab");

                AZ_Printf("Convert-Slice", "------------------------------------------------------------------------------------------\n");
                AZ_Printf("Convert-Slice", "Converting '%s' to '%s'\n", filePath.c_str(), outputPath.c_str());

                AZ::IO::Path inputPath = filePath;
                auto fileExtension = inputPath.Extension();
                if (fileExtension == ".ly")
                {
                    // Special case:  for level files, we need to open the .ly zip file and convert the levelentities.editor_xml file
                    // inside of it.  All the other files can be ignored as they are deprecated legacy system files that are no longer
                    // loaded with prefab-based levels.
                    packOpened = archiveInterface->OpenPack(filePath);
                    inputPath.ReplaceFilename("levelentities.editor_xml");
                    AZ_Warning("Convert-Slice", packOpened, "  '%s' could not be opened as a pack file.\n", filePath.c_str());
                }
                else
                {
                    AZ_Warning(
                        "Convert-Slice", (fileExtension == ".slice"),
                        "  Warning: Only .ly and .slice files are supported, conversion of '%.*s' may not work.\n",
                        AZ_STRING_ARG(fileExtension.Native()));
                }

                auto callback = [prefabSystemComponent, &outputPath, isDryRun]
                    (void* classPtr, const Uuid& classId, [[maybe_unused]] SerializeContext* context)
                {
                    if (classId != azrtti_typeid<AZ::Entity>())
                    {
                        AZ_Printf("Convert-Slice", "  File not converted: Slice root is not an entity.\n");
                        return false;
                    }

                    AZ::Entity* rootEntity = reinterpret_cast<AZ::Entity*>(classPtr);
                    return ConvertSliceFile(prefabSystemComponent, outputPath, isDryRun, rootEntity);
                };

                if (!Utilities::InspectSerializedFile(inputPath.c_str(), convertSettings.m_serializeContext, callback))
                {
                    AZ_Warning("Convert-Slice", false, "Failed to load '%s'. File may not contain an object stream.", inputPath.c_str());
                    result = false;
                }

                if (packOpened)
                {
                    [[maybe_unused]] bool closeResult = archiveInterface->ClosePack(filePath);
                    AZ_Warning("Convert-Slice", !closeResult, "Failed to close '%s'.", filePath.c_str());
                }

                AZ_Printf("Convert-Slice", "Finished converting '%s' to '%s'\n", filePath.c_str(), outputPath.c_str());
                AZ_Printf("Convert-Slice", "------------------------------------------------------------------------------------------\n");
            }

            return result;
        }

        bool SliceConverter::ConvertSliceFile(
            AzToolsFramework::Prefab::PrefabSystemComponent* prefabSystemComponent, AZ::IO::PathView outputPath, bool isDryRun,
            AZ::Entity* rootEntity)
        {
            // Find the slice from the root entity.
            SliceComponent* sliceComponent = AZ::EntityUtils::FindFirstDerivedComponent<SliceComponent>(rootEntity);
            if (sliceComponent == nullptr)
            {
                AZ_Printf("Convert-Slice", "  File not converted: Root entity did not contain a slice component.\n");
                return false;
            }

            // Get all of the entities from the slice.
            SliceComponent::EntityList sliceEntities = sliceComponent->GetNewEntities();
            if (sliceEntities.empty())
            {
                AZ_Printf("Convert-Slice", "  File not converted: Slice entities could not be retrieved.\n");
                return false;
            }

            const SliceComponent::SliceList& sliceList = sliceComponent->GetSlices();
            AZ_Warning("Convert-Slice", sliceList.empty(), "  Slice depends on other slices, this conversion will lose data.\n");

            // Create the Prefab with the entities from the slice
            AZStd::unique_ptr<AzToolsFramework::Prefab::Instance> sourceInstance(
                prefabSystemComponent->CreatePrefab(sliceEntities, {}, outputPath));

            // Dispatch events here, because prefab creation might trigger asset loads in rare circumstances.
            AZ::Data::AssetManager::Instance().DispatchEvents();

            // Set up the Prefab container entity to be a proper Editor entity.  (This logic is normally triggered
            // via an EditorRequests EBus in CreatePrefab, but the subsystem that listens for it isn't present in this tool.)
            AzToolsFramework::Prefab::EntityOptionalReference container = sourceInstance->GetContainerEntity();
            AzToolsFramework::EditorEntityContextRequestBus::Broadcast(
                &AzToolsFramework::EditorEntityContextRequestBus::Events::AddRequiredComponents, container->get());
            container->get().AddComponent(aznew AzToolsFramework::Prefab::EditorPrefabComponent());

            // Reparent any root-level slice entities to the container entity.
            for (auto entity : sliceEntities)
            {
                AzToolsFramework::Components::TransformComponent* transformComponent =
                    entity->FindComponent<AzToolsFramework::Components::TransformComponent>();
                if (transformComponent)
                {
                    if (!transformComponent->GetParentId().IsValid())
                    {
                        transformComponent->SetParent(container->get().GetId());
                    }
                }
            }

            auto templateId = sourceInstance->GetTemplateId();

            if (templateId == AzToolsFramework::Prefab::InvalidTemplateId)
            {
                AZ_Printf("Convert-Slice", "  Path error. Path could be invalid, or the prefab may not be loaded in this level.\n");
                return false;
            }

            // Update the prefab template with the fixed-up data in our prefab instance.
            AzToolsFramework::Prefab::PrefabDom prefabDom;
            bool storeResult = AzToolsFramework::Prefab::PrefabDomUtils::StoreInstanceInPrefabDom(*sourceInstance, prefabDom);
            if (storeResult == false)
            {
                AZ_Printf("Convert-Slice", "  Failed to convert prefab instance data to a PrefabDom.\n");
                return false;
            }
            prefabSystemComponent->UpdatePrefabTemplate(templateId, prefabDom);

            // Dispatch events here, because prefab serialization might trigger asset loads in rare circumstances.
            AZ::Data::AssetManager::Instance().DispatchEvents();

            if (isDryRun)
            {
                PrintPrefab(prefabDom, sourceInstance->GetTemplateSourcePath());
                return true;
            }
            else
            {
                return SavePrefab(templateId);
            }
        }

        void SliceConverter::PrintPrefab(const AzToolsFramework::Prefab::PrefabDom& prefabDom, const AZ::IO::Path& templatePath)
        {
            rapidjson::StringBuffer prefabBuffer;
            rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(prefabBuffer);
            prefabDom.Accept(writer);
            AZ_Printf("Convert-Slice", "JSON for %s:\n", templatePath.c_str());

            // We use Output() to print out the JSON because AZ_Printf has a 4096-character limit.
            AZ::Debug::Trace::Instance().Output("", prefabBuffer.GetString());
            AZ::Debug::Trace::Instance().Output("", "\n");
        }

        bool SliceConverter::SavePrefab(AzToolsFramework::Prefab::TemplateId templateId)
        {
            auto prefabLoaderInterface = AZ::Interface<AzToolsFramework::Prefab::PrefabLoaderInterface>::Get();

            if (!prefabLoaderInterface->SaveTemplate(templateId))
            {
                AZ_Printf("Convert-Slice", "  Could not save prefab - internal error (Json write operation failure).\n");
                return false;
            }

            return true;
        }

    } // namespace SerializeContextTools
} // namespace AZ
