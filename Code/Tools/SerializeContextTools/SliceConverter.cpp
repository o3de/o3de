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
#include <AzFramework/Asset/AssetSystemBus.h>
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <AzToolsFramework/Prefab/PrefabDomUtils.h>
#include <AzToolsFramework/Prefab/EditorPrefabComponent.h>
#include <AzToolsFramework/Prefab/PrefabPublicInterface.h>
#include <AzToolsFramework/Entity/EditorEntityContextBus.h>
#include <AzToolsFramework/Entity/PrefabEditorEntityOwnershipInterface.h>
#include <AzToolsFramework/ToolsComponents/TransformComponent.h>
#include <Application.h>
#include <SliceConverter.h>
#include <Utilities.h>

// SliceConverter reads in a slice file (saved in an ObjectStream format), instantiates it, creates a prefab out of the data,
// and saves the prefab in a JSON format.  This can be used for one-time migrations of slices or slice-based levels to prefabs.
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

            // Connect to the Asset Processor so that we can get the correct source path to any nested slice references.
            if (!ConnectToAssetProcessor())
            {
                AZ_Error("Convert-Slice", false, "  Failed to connect to the Asset Processor.\n");
                return false;
            }

            // Load the asset catalog so that we can find any nested assets successfully.  We also need to tick the tick bus
            // so that the OnCatalogLoaded event gets processed now, instead of during application shutdown.
            AZ::Data::AssetCatalogRequestBus::Broadcast(
                &AZ::Data::AssetCatalogRequestBus::Events::LoadCatalog, "@assets@/assetcatalog.xml");
            application.Tick();

            AZStd::string logggingScratchBuffer;
            SetupLogging(logggingScratchBuffer, convertSettings.m_reporting, *commandLine);

            bool isDryRun = commandLine->HasSwitch("dryrun");

            JsonDeserializerSettings verifySettings;
            verifySettings.m_registrationContext = application.GetJsonRegistrationContext();
            verifySettings.m_serializeContext = application.GetSerializeContext();
            SetupLogging(logggingScratchBuffer, verifySettings.m_reporting, *commandLine);

            bool result = true;
            rapidjson::StringBuffer scratchBuffer;

            // Loop through the list of requested files and convert them.
            AZStd::vector<AZStd::string> fileList = Utilities::ReadFileListFromCommandLine(application, "files");
            for (AZStd::string& filePath : fileList)
            {
                bool convertResult = ConvertSliceFile(convertSettings.m_serializeContext, filePath, isDryRun);
                result = result && convertResult;
            }

            DisconnectFromAssetProcessor();
            return result;
        }

        bool SliceConverter::ConvertSliceFile(
            AZ::SerializeContext* serializeContext, const AZStd::string& slicePath, bool isDryRun)
        {
            bool result = true;
            bool packOpened = false;

            auto archiveInterface = AZ::Interface<AZ::IO::IArchive>::Get();

            AZ::IO::Path outputPath = slicePath;
            outputPath.ReplaceExtension("prefab");

            AZ_Printf("Convert-Slice", "------------------------------------------------------------------------------------------\n");
            AZ_Printf("Convert-Slice", "Converting '%s' to '%s'\n", slicePath.c_str(), outputPath.c_str());

            AZ::IO::Path inputPath = slicePath;
            auto fileExtension = inputPath.Extension();
            if (fileExtension == ".ly")
            {
                // Special case:  for level files, we need to open the .ly zip file and convert the levelentities.editor_xml file
                // inside of it.  All the other files can be ignored as they are deprecated legacy system files that are no longer
                // loaded with prefab-based levels.
                packOpened = archiveInterface->OpenPack(slicePath);
                inputPath.ReplaceFilename("levelentities.editor_xml");
                AZ_Warning("Convert-Slice", packOpened, "  '%s' could not be opened as a pack file.\n", slicePath.c_str());
            }
            else
            {
                AZ_Warning(
                    "Convert-Slice", (fileExtension == ".slice"),
                    "  Warning: Only .ly and .slice files are supported, conversion of '%.*s' may not work.\n",
                    AZ_STRING_ARG(fileExtension.Native()));
            }

            auto callback = [&outputPath, isDryRun](void* classPtr, const Uuid& classId, SerializeContext* context)
            {
                if (classId != azrtti_typeid<AZ::Entity>())
                {
                    AZ_Printf("Convert-Slice", "  File not converted: Slice root is not an entity.\n");
                    return false;
                }

                AZ::Entity* rootEntity = reinterpret_cast<AZ::Entity*>(classPtr);
                return ConvertSliceToPrefab(context, outputPath, isDryRun, rootEntity);
            };

            // Read in the slice file and call the callback on completion to convert the read-in slice to a prefab.
            if (!Utilities::InspectSerializedFile(inputPath.c_str(), serializeContext, callback))
            {
                AZ_Warning("Convert-Slice", false, "Failed to load '%s'. File may not contain an object stream.", inputPath.c_str());
                result = false;
            }

            if (packOpened)
            {
                [[maybe_unused]] bool closeResult = archiveInterface->ClosePack(slicePath);
                AZ_Warning("Convert-Slice", closeResult, "Failed to close '%s'.", slicePath.c_str());
            }

            AZ_Printf("Convert-Slice", "Finished converting '%s' to '%s'\n", slicePath.c_str(), outputPath.c_str());
            AZ_Printf("Convert-Slice", "------------------------------------------------------------------------------------------\n");

            return result;
        }

        bool SliceConverter::ConvertSliceToPrefab(
            AZ::SerializeContext* serializeContext, AZ::IO::PathView outputPath, bool isDryRun, AZ::Entity* rootEntity)
        {
            auto prefabSystemComponentInterface = AZ::Interface<AzToolsFramework::Prefab::PrefabSystemComponentInterface>::Get();

            // Find the slice from the root entity.
            SliceComponent* sliceComponent = AZ::EntityUtils::FindFirstDerivedComponent<SliceComponent>(rootEntity);
            if (sliceComponent == nullptr)
            {
                AZ_Printf("Convert-Slice", "  File not converted: Root entity did not contain a slice component.\n");
                return false;
            }

            // Get all of the entities from the slice.
            SliceComponent::EntityList sliceEntities = sliceComponent->GetNewEntities();
            AZ_Printf("Convert-Slice", "  Slice contains %zu entities.\n", sliceEntities.size());

            // Create the Prefab with the entities from the slice
            AZStd::unique_ptr<AzToolsFramework::Prefab::Instance> sourceInstance(
                prefabSystemComponentInterface->CreatePrefab(sliceEntities, {}, outputPath));

            // Dispatch events here, because prefab creation might trigger asset loads in rare circumstances.
            AZ::Data::AssetManager::Instance().DispatchEvents();

            // Fix up the container entity to have the proper components and fix up the slice entities to have the proper hierarchy
            // with the container as the top-most parent.
            AzToolsFramework::Prefab::EntityOptionalReference container = sourceInstance->GetContainerEntity();
            FixPrefabEntities(container->get(), sliceEntities);

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
            prefabSystemComponentInterface->UpdatePrefabTemplate(templateId, prefabDom);

            // Dispatch events here, because prefab serialization might trigger asset loads in rare circumstances.
            AZ::Data::AssetManager::Instance().DispatchEvents();

            // If this slice has nested slices, we need to loop through those, convert them to prefabs as well, and
            // set up the new nesting relationships correctly.
            const SliceComponent::SliceList& sliceList = sliceComponent->GetSlices();
            AZ_Printf("Convert-Slice", "  Slice contains %zu nested slices.\n", sliceList.size());
            if (!sliceList.empty())
            {
                bool nestedSliceResult = ConvertNestedSlices(sliceComponent, sourceInstance.get(), serializeContext, isDryRun);
                if (!nestedSliceResult)
                {
                    return false;
                }
            }

            if (isDryRun)
            {
                PrintPrefab(templateId);
                return true;
            }
            else
            {
                return SavePrefab(templateId);
            }
        }

        void SliceConverter::FixPrefabEntities(AZ::Entity& containerEntity, SliceComponent::EntityList& sliceEntities)
        {
            // Set up the Prefab container entity to be a proper Editor entity.  (This logic is normally triggered
            // via an EditorRequests EBus in CreatePrefab, but the subsystem that listens for it isn't present in this tool.)
            AzToolsFramework::EditorEntityContextRequestBus::Broadcast(
                &AzToolsFramework::EditorEntityContextRequestBus::Events::AddRequiredComponents, containerEntity);
            containerEntity.AddComponent(aznew AzToolsFramework::Prefab::EditorPrefabComponent());

            // Reparent any root-level slice entities to the container entity.
            for (auto entity : sliceEntities)
            {
                AzToolsFramework::Components::TransformComponent* transformComponent =
                    entity->FindComponent<AzToolsFramework::Components::TransformComponent>();
                if (transformComponent)
                {
                    if (!transformComponent->GetParentId().IsValid())
                    {
                        transformComponent->SetParent(containerEntity.GetId());
                        transformComponent->UpdateCachedWorldTransform();
                    }
                }
            }
        }

        bool SliceConverter::ConvertNestedSlices(
            SliceComponent* sliceComponent, AzToolsFramework::Prefab::Instance* sourceInstance,
            AZ::SerializeContext* serializeContext, bool isDryRun)
        {
            const SliceComponent::SliceList& sliceList = sliceComponent->GetSlices();
            auto prefabSystemComponentInterface = AZ::Interface<AzToolsFramework::Prefab::PrefabSystemComponentInterface>::Get();

            for (auto& slice : sliceList)
            {
                // Get the nested slice asset
                auto sliceAsset = slice.GetSliceAsset();
                sliceAsset.QueueLoad();
                sliceAsset.BlockUntilLoadComplete();

                // The slice list gives us asset IDs, and we need to get to the source path.  So first we get the asset path from the ID,
                // then we get the source path from the asset path.

                AZStd::string processedAssetPath;
                AZ::Data::AssetCatalogRequestBus::BroadcastResult(
                    processedAssetPath, &AZ::Data::AssetCatalogRequests::GetAssetPathById, sliceAsset.GetId());

                AZStd::string assetPath;
                AzToolsFramework::AssetSystemRequestBus::Broadcast(
                    &AzToolsFramework::AssetSystemRequestBus::Events::GetFullSourcePathFromRelativeProductPath,
                    processedAssetPath, assetPath);
                if (assetPath.empty())
                {
                    AZ_Warning("Convert-Slice", false,
                        "  Source path for nested slice '%s' could not be found, slice not converted.", processedAssetPath.c_str());
                    return false;
                }

                // Now, convert the nested slice to a prefab.
                bool nestedSliceResult = ConvertSliceFile(serializeContext, assetPath, isDryRun);
                if (!nestedSliceResult)
                {
                    AZ_Warning("Convert-Slice", nestedSliceResult, "  Nested slice '%s' could not be converted.", assetPath.c_str());
                    return false;
                }

                // Load the prefab template for the newly-created nested prefab.
                // To get the template, we need to take our absolute slice path and turn it into a project-relative prefab path.
                AZ::IO::Path nestedPrefabPath = assetPath;
                nestedPrefabPath.ReplaceExtension("prefab");

                auto prefabLoaderInterface = AZ::Interface<AzToolsFramework::Prefab::PrefabLoaderInterface>::Get();
                nestedPrefabPath = prefabLoaderInterface->GetRelativePathToProject(nestedPrefabPath);

                AzToolsFramework::Prefab::TemplateId nestedTemplateId =
                    prefabSystemComponentInterface->GetTemplateIdFromFilePath(nestedPrefabPath);
                AzToolsFramework::Prefab::TemplateReference nestedTemplate =
                    prefabSystemComponentInterface->FindTemplate(nestedTemplateId);

                // For each slice instance of the nested slice, convert it to a nested prefab instance instead.

                auto instances = slice.GetInstances();
                AZ_Printf(
                    "Convert-Slice", "  Attaching %zu instances of nested slice '%s'.\n", instances.size(),
                    nestedPrefabPath.Native().c_str());

                for (auto& instance : instances)
                {
                    bool instanceConvertResult = ConvertSliceInstance(instance, sliceAsset, nestedTemplate, sourceInstance);
                    if (!instanceConvertResult)
                    {
                        return false;
                    }
                }
            }

            return true;
        }

        bool SliceConverter::ConvertSliceInstance(
            [[maybe_unused]] AZ::SliceComponent::SliceInstance& instance,
            [[maybe_unused]] AZ::Data::Asset<AZ::SliceAsset>& sliceAsset,
            AzToolsFramework::Prefab::TemplateReference nestedTemplate,
            AzToolsFramework::Prefab::Instance* topLevelInstance)
        {
            auto instanceToTemplateInterface = AZ::Interface<AzToolsFramework::Prefab::InstanceToTemplateInterface>::Get();
            auto prefabSystemComponentInterface = AZ::Interface<AzToolsFramework::Prefab::PrefabSystemComponentInterface>::Get();

            // Create a new unmodified prefab Instance for the nested slice instance.
            auto nestedInstance = AZStd::make_unique<AzToolsFramework::Prefab::Instance>();
            AzToolsFramework::Prefab::Instance::EntityList newEntities;
            if (!AzToolsFramework::Prefab::PrefabDomUtils::LoadInstanceFromPrefabDom(
                    *nestedInstance, newEntities, nestedTemplate->get().GetPrefabDom()))
            {
                AZ_Error(
                    "Convert-Slice", false, "  Failed to load and instantiate nested Prefab Template '%s'.",
                    nestedTemplate->get().GetFilePath().c_str());
                return false;
            }

            // Get the DOM for the unmodified nested instance.  This will be used later below for generating the correct patch
            // to the top-level template DOM.
            AzToolsFramework::Prefab::PrefabDom unmodifiedNestedInstanceDom;
            instanceToTemplateInterface->GenerateDomForInstance(unmodifiedNestedInstanceDom, *(nestedInstance.get()));

            // Currently, DataPatch conversions for nested slices aren't implemented, so all nested slice overrides will
            // be lost.
            AZ_Warning(
                "Convert-Slice", false, "  Nested slice instances will lose all of their override data during conversion.",
                nestedTemplate->get().GetFilePath().c_str());

            // Set the container entity of the nested prefab to have the top-level prefab as the parent.
            // Once DataPatch conversions are supported, this will need to change to nest the prefab under the appropriate entity
            // within the level.
            auto containerEntity = nestedInstance->GetContainerEntity();
            AzToolsFramework::Components::TransformComponent* transformComponent =
                containerEntity->get().FindComponent<AzToolsFramework::Components::TransformComponent>();
            if (transformComponent)
            {
                transformComponent->SetParent(topLevelInstance->GetContainerEntityId());
                transformComponent->UpdateCachedWorldTransform();
            }

            // Add the nested instance itself to the top-level prefab.  To do this, we need to add it to our top-level instance,
            // create a patch out of it, and patch the top-level prefab template.

            AzToolsFramework::Prefab::PrefabDom topLevelInstanceDomBefore;
            instanceToTemplateInterface->GenerateDomForInstance(topLevelInstanceDomBefore, *topLevelInstance);

            AzToolsFramework::Prefab::Instance& addedInstance = topLevelInstance->AddInstance(AZStd::move(nestedInstance));

            AzToolsFramework::Prefab::PrefabDom topLevelInstanceDomAfter;
            instanceToTemplateInterface->GenerateDomForInstance(topLevelInstanceDomAfter, *topLevelInstance);

            AzToolsFramework::Prefab::PrefabDom addedInstancePatch;
            instanceToTemplateInterface->GeneratePatch(addedInstancePatch, topLevelInstanceDomBefore, topLevelInstanceDomAfter);
            instanceToTemplateInterface->PatchTemplate(addedInstancePatch, topLevelInstance->GetTemplateId());

            // Get the DOM for the modified nested instance.  Now that the data has been fixed up, and the instance has been added
            // to the top-level instance, we've got all the changes we need to generate the correct patch.

            AzToolsFramework::Prefab::PrefabDom modifiedNestedInstanceDom;
            instanceToTemplateInterface->GenerateDomForInstance(modifiedNestedInstanceDom, addedInstance);

            AzToolsFramework::Prefab::PrefabDom linkPatch;
            instanceToTemplateInterface->GeneratePatch(linkPatch, unmodifiedNestedInstanceDom, modifiedNestedInstanceDom);

            prefabSystemComponentInterface->CreateLink(
                topLevelInstance->GetTemplateId(), addedInstance.GetTemplateId(), addedInstance.GetInstanceAlias(), linkPatch,
                AzToolsFramework::Prefab::InvalidLinkId);
            prefabSystemComponentInterface->PropagateTemplateChanges(topLevelInstance->GetTemplateId());

            return true;
        }

        void SliceConverter::PrintPrefab(AzToolsFramework::Prefab::TemplateId templateId)
        {
            auto prefabSystemComponentInterface = AZ::Interface<AzToolsFramework::Prefab::PrefabSystemComponentInterface>::Get();

            auto prefabTemplate = prefabSystemComponentInterface->FindTemplate(templateId);
            auto& prefabDom = prefabTemplate->get().GetPrefabDom();
            const AZ::IO::Path& templatePath = prefabTemplate->get().GetFilePath();

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

        bool SliceConverter::ConnectToAssetProcessor()
        {
            AzFramework::AssetSystem::ConnectionSettings connectionSettings;
            AzFramework::AssetSystem::ReadConnectionSettingsFromSettingsRegistry(connectionSettings);

            connectionSettings.m_launchAssetProcessorOnFailedConnection = true;
            connectionSettings.m_connectionDirection =
                AzFramework::AssetSystem::ConnectionSettings::ConnectionDirection::ConnectToAssetProcessor;
            connectionSettings.m_connectionIdentifier = AzFramework::AssetSystem::ConnectionIdentifiers::Editor;
            connectionSettings.m_loggingCallback = [](AZStd::string_view logData)
            {
                AZ_Printf("Convert-Slice", "%.*s\n", AZ_STRING_ARG(logData));
            };

            bool connectedToAssetProcessor = false;

            AzFramework::AssetSystemRequestBus::BroadcastResult(
                connectedToAssetProcessor, &AzFramework::AssetSystemRequestBus::Events::EstablishAssetProcessorConnection,
                connectionSettings);

            return connectedToAssetProcessor;
        }

        void SliceConverter::DisconnectFromAssetProcessor()
        {
            AzFramework::AssetSystemRequestBus::Broadcast(
                &AzFramework::AssetSystem::AssetSystemRequests::StartDisconnectingAssetProcessor);

            // Wait for the disconnect to finish.
            bool disconnected = false;
            AzFramework::AssetSystemRequestBus::BroadcastResult(disconnected, 
                &AzFramework::AssetSystem::AssetSystemRequests::WaitUntilAssetProcessorDisconnected, AZStd::chrono::seconds(30));

            AZ_Error("Convert-Slice", disconnected, "Asset Processor failed to disconnect successfully.");
        }

    } // namespace SerializeContextTools
} // namespace AZ
