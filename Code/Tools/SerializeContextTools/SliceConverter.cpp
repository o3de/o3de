/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
#include <AzToolsFramework/Prefab/Instance/InstanceUpdateExecutorInterface.h>
#include <AzToolsFramework/Entity/EditorEntityContextBus.h>
#include <AzToolsFramework/Entity/PrefabEditorEntityOwnershipInterface.h>
#include <AzToolsFramework/ToolsComponents/TransformComponent.h>
#include <Application.h>
#include <SliceConverter.h>
#include <SliceConverterEditorEntityContextComponent.h>
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

            AZStd::string logggingScratchBuffer;
            SetupLogging(logggingScratchBuffer, convertSettings.m_reporting, *commandLine);

            bool isDryRun = commandLine->HasSwitch("dryrun");

            JsonDeserializerSettings verifySettings;
            verifySettings.m_registrationContext = application.GetJsonRegistrationContext();
            verifySettings.m_serializeContext = application.GetSerializeContext();
            SetupLogging(logggingScratchBuffer, verifySettings.m_reporting, *commandLine);

            bool result = true;
            rapidjson::StringBuffer scratchBuffer;

            // For slice conversion, disable the EditorEntityContextComponent logic that activates entities on creation.
            // This prevents a lot of error messages and crashes during conversion due to lack of full environment and subsystem setup.
            AzToolsFramework::SliceConverterEditorEntityContextComponent::DisableOnContextEntityLogic();

            // Loop through the list of requested files and convert them.
            AZStd::vector<AZStd::string> fileList = Utilities::ReadFileListFromCommandLine(application, "files");
            for (AZStd::string& filePath : fileList)
            {
                bool convertResult = ConvertSliceFile(convertSettings.m_serializeContext, filePath, isDryRun);
                result = result && convertResult;

                // Clear out all registered prefab templates between each top-level file that gets processed.
                auto prefabSystemComponentInterface = AZ::Interface<AzToolsFramework::Prefab::PrefabSystemComponentInterface>::Get();
                for (auto templateId : m_createdTemplateIds)
                {
                    // We don't just want to call RemoveAllTemplates() because the root template should remain between file conversions.
                    prefabSystemComponentInterface->RemoveTemplate(templateId);
                }
                m_aliasIdMapper.clear();
                m_createdTemplateIds.clear();
            }

            DisconnectFromAssetProcessor();
            return result;
        }

        bool SliceConverter::ConvertSliceFile(AZ::SerializeContext* serializeContext, const AZStd::string& slicePath, bool isDryRun)
        {
            /* To convert a slice file, we read the input file in via ObjectStream, then use the "class ready" callback to convert
            * the data in memory to a Prefab.
            * If the input file is a level file (.ly), we actually need to load the level slice file ("levelentities.editor_xml") from
            * within the level file, which effectively is a zip file of the level slice file and a bunch of legacy level files that won't
            * be converted, since the systems that would use them no longer exist.
            */

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

            auto callback = [this, &outputPath, isDryRun](void* classPtr, const Uuid& classId, SerializeContext* context)
            {
                if (classId != azrtti_typeid<AZ::Entity>())
                {
                    AZ_Printf("Convert-Slice", "  File not converted: Slice root is not an entity.\n");
                    return false;
                }

                AZ::Entity* rootEntity = reinterpret_cast<AZ::Entity*>(classPtr);
                bool convertResult = ConvertSliceToPrefab(context, outputPath, isDryRun, rootEntity);

                // Delete the root entity pointer.  Otherwise, it will leak itself along with all of the slice asset references held
                // within it.
                delete rootEntity;
                return convertResult;
            };

            // Read in the slice file and call the callback on completion to convert the read-in slice to a prefab.
            // This will also load dependent slice assets, but no other dependent asset types.
            // Since we're not actually initializing any of the entities, we don't need any of the non-slice assets to be loaded.
            if (!Utilities::InspectSerializedFile(
                    inputPath.c_str(), serializeContext, callback,
                    [](const AZ::Data::AssetFilterInfo& filterInfo)
                    {
                        return (filterInfo.m_assetType == azrtti_typeid<AZ::SliceAsset>());
                    }))
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

            // Get all of the entities from the slice.  We're taking ownership of them, so we also remove them from the slice component
            // without deleting them.
            constexpr bool deleteEntities = false;
            constexpr bool removeEmptyInstances = true;
            SliceComponent::EntityList sliceEntities = sliceComponent->GetNewEntities();
            sliceComponent->RemoveAllEntities(deleteEntities, removeEmptyInstances);
            AZ_Printf("Convert-Slice", "  Slice contains %zu entities.\n", sliceEntities.size());

            // Create an empty Prefab as the start of our conversion.
            // The entities are added in a separate step so that we can give them deterministic entity aliases that match their entity Ids
            AZStd::unique_ptr<AzToolsFramework::Prefab::Instance> sourceInstance(
                prefabSystemComponentInterface->CreatePrefab({}, {}, outputPath));

            // Add entities into our prefab.
            // In slice->prefab conversions, there's a chicken-and-egg problem that occurs with entity references, so we're initially
            // going to add empty dummy entities with the right IDs and aliases.
            // The problem is that we can have entities in this root list that have references to nested slice instance entities that we
            // haven't created yet, and we will have nested slice entities that need to reference these entities as parents.
            // If we create these entities as fully-formed first, they will fail to serialize correctly when adding each nested instance,
            // due to the references not pointing to valid entities yet.  And if we *wait* to create these and build the nested instances
            // first, they'll fail to serialize correctly due to referencing these as parents.
            // So our solution is that we'll initially create these entities as empty placeholders with no references, *then* we'll build
            // up the nested instances, *then* we'll finish building these entities out.

            // prefabPlaceholderEntities will hold onto pointers to the entities we're building up in the prefab.  The prefab will own
            // the lifetime of them, but we'll use the references here for convenient access.
            AZStd::vector<AZ::Entity*> prefabPlaceholderEntities;
            // entityAliases will hold onto the alias we want to use for each of those entities.  We'll need to use the same alias when
            // we replace the entities at the end.
            AZStd::vector<AZStd::string> entityAliases;
            for (auto& entity : sliceEntities)
            {
                auto id = entity->GetId();
                prefabPlaceholderEntities.emplace_back(aznew AZ::Entity(id));
                entityAliases.emplace_back(AZStd::string::format("Entity_%s", id.ToString().c_str()));
                sourceInstance->AddEntity(*(prefabPlaceholderEntities.back()), entityAliases.back());

                // Save off a mapping of the original slice entity IDs to the new prefab template entity aliases.
                // We'll need this mapping for fixing up all the entity references in this slice as well as any nested instances.
                auto result = m_aliasIdMapper.emplace(id, SliceEntityMappingInfo(sourceInstance->GetTemplateId(), entityAliases.back()));
                if (!result.second)
                {
                    AZ_Printf("Convert-Slice", "  Duplicate entity alias -> entity id entries found, conversion may not be successful.\n");
                }
            }

            // Dispatch events here, because prefab creation might trigger asset loads in rare circumstances.
            AZ::Data::AssetManager::Instance().DispatchEvents();

            // Keep track of the template Id we created, we're going to remove it at the end of slice file conversion to make sure
            // the data doesn't stick around between file conversions.
            auto templateId = sourceInstance->GetTemplateId();
            if (templateId == AzToolsFramework::Prefab::InvalidTemplateId)
            {
                AZ_Printf("Convert-Slice", "  Path error. Path could be invalid, or the prefab may not be loaded in this level.\n");
                return false;
            }
            m_createdTemplateIds.emplace(templateId);

            // Save off the the first version of this prefab template with our empty placeholder entities.
            // As it saves off, the entities will all change IDs during serialization / propagation, but the aliases will remain the same.
            AzToolsFramework::Prefab::PrefabDom prefabDom;
            bool storeResult = AzToolsFramework::Prefab::PrefabDomUtils::StoreInstanceInPrefabDom(*sourceInstance, prefabDom);
            if (storeResult == false)
            {
                AZ_Printf("Convert-Slice", "  Failed to convert prefab instance data to a PrefabDom.\n");
                return false;
            }
            prefabSystemComponentInterface->UpdatePrefabTemplate(templateId, prefabDom);
            AZ::Interface<AzToolsFramework::Prefab::InstanceUpdateExecutorInterface>::Get()->UpdateTemplateInstancesInQueue();

            // Dispatch events here, because prefab serialization might trigger asset loads in rare circumstances.
            AZ::Data::AssetManager::Instance().DispatchEvents();

            // Save off a mapping of the slice's metadata entity ID as well, even though we never converted the entity itself.
            // This will help us better detect entity ID mapping errors for nested slice instances.
            AZ::Entity* metadataEntity = sliceComponent->GetMetadataEntity();
            constexpr bool isMetadataEntity = true;
            m_aliasIdMapper.emplace(metadataEntity->GetId(), SliceEntityMappingInfo(templateId, "MetadataEntity", isMetadataEntity));

            // Also save off a mapping of the prefab's container entity ID.
            m_aliasIdMapper.emplace(sourceInstance->GetContainerEntityId(), SliceEntityMappingInfo(templateId, "ContainerEntity"));

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

            // *After* converting the nested slices, remove our placeholder entities and replace them with the correct ones.
            // The placeholder entity IDs will have changed from what we originally created, so we need to make sure our replacement
            // entities have the same IDs and aliases as the placeholders so that any instance references that have already been fixed
            // up continue to reference the correct entities here.
            for (size_t curEntityIdx = 0; curEntityIdx < sliceEntities.size(); curEntityIdx++)
            {
                auto& sliceEntity = sliceEntities[curEntityIdx];
                auto& prefabEntity = prefabPlaceholderEntities[curEntityIdx];
                sliceEntity->SetId(prefabEntity->GetId());
            }
            // Remove and delete our placeholder entities.
            // (By using an empty callback on DetachEntities, the unique_ptr will auto-delete the placeholder entities)
            sourceInstance->DetachEntities([](AZStd::unique_ptr<AZ::Entity>){});
            prefabPlaceholderEntities.clear();
            for (size_t curEntityIdx = 0; curEntityIdx < sliceEntities.size(); curEntityIdx++)
            {
                UpdateCachedTransform(*(sliceEntities[curEntityIdx]));
                sourceInstance->AddEntity(*(sliceEntities[curEntityIdx]), entityAliases[curEntityIdx]);
            }

            // Fix up the container entity to have the proper components and fix up the slice entities to have the proper hierarchy
            // with the container as the top-most parent.
            AzToolsFramework::Prefab::EntityOptionalReference container = sourceInstance->GetContainerEntity();
            FixPrefabEntities(container->get(), sliceEntities);

            // Also save off a mapping of the prefab's container entity ID.
            m_aliasIdMapper.emplace(sourceInstance->GetContainerEntityId(), SliceEntityMappingInfo(templateId, "ContainerEntity"));

            // Remap all of the entity references that exist in these top-level slice entities.
            SliceComponent::InstantiatedContainer instantiatedEntities(false);
            instantiatedEntities.m_entities = sliceEntities;
            RemapIdReferences(m_aliasIdMapper, sourceInstance.get(), sourceInstance.get(), &instantiatedEntities, serializeContext);

            // Finally, store the completed slice->prefab conversion back into the template.
            storeResult = AzToolsFramework::Prefab::PrefabDomUtils::StoreInstanceInPrefabDom(*sourceInstance, prefabDom);
            if (storeResult == false)
            {
                AZ_Printf("Convert-Slice", "  Failed to convert prefab instance data to a PrefabDom.\n");
                return false;
            }
            prefabSystemComponentInterface->UpdatePrefabTemplate(templateId, prefabDom);
            AZ::Interface<AzToolsFramework::Prefab::InstanceUpdateExecutorInterface>::Get()->UpdateTemplateInstancesInQueue();

            // Dispatch events here, because prefab serialization might trigger asset loads in rare circumstances.
            AZ::Data::AssetManager::Instance().DispatchEvents();

            if (isDryRun)
            {
                PrintPrefab(templateId);
                return true;
            }
            else
            {
                return SavePrefab(outputPath, templateId);
            }
        }

        void SliceConverter::FixPrefabEntities(AZ::Entity& containerEntity, SliceComponent::EntityList& sliceEntities)
        {
            // Set up the Prefab container entity to be a proper Editor entity.  (This logic is normally triggered
            // via an EditorRequests EBus in CreatePrefab, but the subsystem that listens for it isn't present in this tool.)
            AzToolsFramework::EditorEntityContextRequestBus::Broadcast(
                &AzToolsFramework::EditorEntityContextRequestBus::Events::AddRequiredComponents, containerEntity);
            if (containerEntity.FindComponent<AzToolsFramework::Prefab::EditorPrefabComponent>() == nullptr)
            {
                containerEntity.AddComponent(aznew AzToolsFramework::Prefab::EditorPrefabComponent());
            }

            // Make all the components on the container entity have deterministic component IDs, so that multiple runs of the tool
            // on the same slice will produce the same prefab output.  We're going to cheat a bit and just use the component type hash
            // as the component ID.  This would break if we had multiple components of the same type, but that currently doesn't
            // happen for the container entity.
            auto containerComponents = containerEntity.GetComponents();
            for (auto& component : containerComponents)
            {
                component->SetId(component->GetUnderlyingComponentType().GetHash());
            }

            // Reparent any root-level slice entities to the container entity.
            for (auto entity : sliceEntities)
            {
                constexpr bool onlySetIfInvalid = true;
                SetParentEntity(*entity, containerEntity.GetId(), onlySetIfInvalid);
            }
        }

        bool SliceConverter::ConvertNestedSlices(
            SliceComponent* sliceComponent, AzToolsFramework::Prefab::Instance* sourceInstance,
            AZ::SerializeContext* serializeContext, bool isDryRun)
        {
            /* Given a root slice, find all the nested slices and convert them. */

            // Get the list of nested slices that this slice uses.
            const SliceComponent::SliceList& sliceList = sliceComponent->GetSlices();
            auto prefabSystemComponentInterface = AZ::Interface<AzToolsFramework::Prefab::PrefabSystemComponentInterface>::Get();

            // For each nested slice, convert it.
            for (auto& slice : sliceList)
            {
                // Get the nested slice asset.  These should already be preloaded due to loading the root asset.
                auto sliceAsset = slice.GetSliceAsset();
                AZ_Assert(sliceAsset.IsReady(), "slice asset hasn't been loaded yet!");

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

                // Check to see if we've already converted this slice at a higher level of slice nesting, or if this is our first
                // occurrence and we need to convert it now.

                // First, take our absolute slice path and turn it into a project-relative prefab path.
                AZ::IO::Path nestedPrefabPath = assetPath;
                nestedPrefabPath.ReplaceExtension("prefab");

                auto prefabLoaderInterface = AZ::Interface<AzToolsFramework::Prefab::PrefabLoaderInterface>::Get();
                nestedPrefabPath = prefabLoaderInterface->GenerateRelativePath(nestedPrefabPath);

                // Now, see if we already have a template ID in memory for it.
                AzToolsFramework::Prefab::TemplateId nestedTemplateId =
                    prefabSystemComponentInterface->GetTemplateIdFromFilePath(nestedPrefabPath);

                // If we don't have a template ID yet, convert the nested slice to a prefab and get the template ID.
                if (nestedTemplateId == AzToolsFramework::Prefab::InvalidTemplateId)
                {
                    bool nestedSliceResult = ConvertSliceFile(serializeContext, assetPath, isDryRun);
                    if (!nestedSliceResult)
                    {
                        AZ_Warning("Convert-Slice", nestedSliceResult, "  Nested slice '%s' could not be converted.", assetPath.c_str());
                        return false;
                    }

                    nestedTemplateId = prefabSystemComponentInterface->GetTemplateIdFromFilePath(nestedPrefabPath);
                    AZ_Assert(nestedTemplateId != AzToolsFramework::Prefab::InvalidTemplateId,
                        "Template ID for %s is invalid", nestedPrefabPath.c_str());
                }

                // Get the nested prefab template.
                AzToolsFramework::Prefab::TemplateReference nestedTemplate =
                    prefabSystemComponentInterface->FindTemplate(nestedTemplateId);

                // For each slice instance of the nested slice, convert it to a nested prefab instance instead.

                auto instances = slice.GetInstances();
                AZ_Printf(
                    "Convert-Slice", "Attaching %zu instances of nested slice '%s'.\n", instances.size(),
                    nestedPrefabPath.Native().c_str());

                // Before processing any further, save off all the known entity IDs from all the instances and how they map back to
                // the base nested prefab that they've come from (i.e. this one).  As we proceed up the chain of nesting, this will
                // build out a hierarchical list of owning instances for each entity that we can trace upwards to know where to add
                // the entity into our nested prefab instance.
                // This step needs to occur *before* converting the instances themselves, because while converting instances, they
                // might have entity ID references that point to other instances.  By having the full instance entity ID map in place
                // before conversion, we'll be able to fix them up appropriately.

                for (auto& instance : instances)
                {
                    AZStd::string instanceAlias = GetInstanceAlias(instance);
                    UpdateSliceEntityInstanceMappings(instance.GetEntityIdToBaseMap(), instanceAlias);
                }

                // Now that we have all the entity ID mappings, convert all the instances.
                size_t curInstance = 0;
                for (auto& instance : instances)
                {
                    AZ_Printf("Convert-Slice", "  Converting instance %zu.\n", curInstance++);
                    bool instanceConvertResult = ConvertSliceInstance(instance, sliceAsset, nestedTemplate, sourceInstance);
                    if (!instanceConvertResult)
                    {
                        return false;
                    }
                }

                AZ_Printf(
                    "Convert-Slice", "Finished attaching %zu instances of nested slice '%s'.\n", instances.size(),
                    nestedPrefabPath.Native().c_str());
            }

            return true;
        }

        AZStd::string SliceConverter::GetInstanceAlias(const AZ::SliceComponent::SliceInstance& instance)
        {
            // When creating the new instance, we would like to have deterministic instance aliases.  Prefabs that depend on this one
            // will have patches that reference the alias, so if we reconvert this slice a second time, we would like it to produce
            // the same results.  To get a deterministic and unique alias, we rely on the slice instance.  The slice instance contains
            // a map of slice entity IDs to unique instance entity IDs.  We'll just consistently use the first entry in the map as the
            // unique instance ID.
            AZStd::string instanceAlias;
            auto entityIdMap = instance.GetEntityIdMap();
            if (!entityIdMap.empty())
            {
                instanceAlias = AZStd::string::format("Instance_%s", entityIdMap.begin()->second.ToString().c_str());
            }
            else
            {
                AZ_Error("Convert-Slice", false, "  Couldn't create deterministic instance alias.");
                instanceAlias = AZStd::string::format("Instance_%s", AZ::Entity::MakeId().ToString().c_str());
            }
            return instanceAlias;
        }


        bool SliceConverter::ConvertSliceInstance(
            AZ::SliceComponent::SliceInstance& instance,
            AZ::Data::Asset<AZ::SliceAsset>& sliceAsset,
            AzToolsFramework::Prefab::TemplateReference nestedTemplate,
            AzToolsFramework::Prefab::Instance* topLevelInstance)
        {
            /* To convert a slice instance, it's important to understand the similarities and differences between slices and prefabs.
            * Both slices and prefabs have the concept of instances of a nested slice/prefab, where each instance can have its own
            * set of changed data (transforms, component values, etc).
            * For slices, the changed data comes from applying a DataPatch to an instantiated set of entities from the nested slice.
            * From prefabs, the changed data comes from Json patches that are applied to the instantiated set of entities from the
            * nested prefab.  The prefab instance entities also have different IDs than the slice instance entities, so we'll need
            * to remap some of them along the way.
            * To get from one to the other, we'll need to do the following:
            * - Instantiate the nested slice and nested prefab
            * - Patch the nested slice instance and fix up the entity ID references
            * - Replace the nested prefab instance entities with the fixed-up slice ones
            * - Add the nested instance (and the link patch) to the top-level prefab
            */

            auto instanceToTemplateInterface = AZ::Interface<AzToolsFramework::Prefab::InstanceToTemplateInterface>::Get();
            auto prefabSystemComponentInterface = AZ::Interface<AzToolsFramework::Prefab::PrefabSystemComponentInterface>::Get();

            AZStd::string instanceAlias = GetInstanceAlias(instance);

            // Create a new unmodified prefab Instance for the nested slice instance.
            auto nestedInstance = AZStd::make_unique<AzToolsFramework::Prefab::Instance>(AZStd::move(instanceAlias));
            AzToolsFramework::EntityList newEntities;
            if (!AzToolsFramework::Prefab::PrefabDomUtils::LoadInstanceFromPrefabDom(
                    *nestedInstance, newEntities, nestedTemplate->get().GetPrefabDom()))
            {
                AZ_Error(
                    "Convert-Slice", false, "  Failed to load and instantiate nested Prefab Template '%s'.",
                    nestedTemplate->get().GetFilePath().c_str());
                return false;
            }

            // Save off a mapping of the new nested Instance's container ID
            m_aliasIdMapper.emplace(nestedInstance->GetContainerEntityId(),
                SliceEntityMappingInfo(nestedInstance->GetTemplateId(), "ContainerEntity"));

            // Get the DOM for the unmodified nested instance.  This will be used later below for generating the correct patch
            // to the top-level template DOM.
            AzToolsFramework::Prefab::PrefabDom unmodifiedNestedInstanceDom;
            instanceToTemplateInterface->GenerateDomForInstance(unmodifiedNestedInstanceDom, *(nestedInstance.get()));

            // Instantiate a new instance of the nested slice
            SliceComponent* dependentSlice = sliceAsset.Get()->GetComponent();
            [[maybe_unused]] AZ::SliceComponent::InstantiateResult instantiationResult = dependentSlice->Instantiate();
            AZ_Assert(instantiationResult == AZ::SliceComponent::InstantiateResult::Success, "Failed to instantiate instance");

            // Apply the data patch for this instance of the nested slice.  This will provide us with a version of the slice's entities
            // with all data overrides applied to them.
            DataPatch::FlagsMap sourceDataFlags = dependentSlice->GetDataFlagsForInstances().GetDataFlagsForPatching();
            DataPatch::FlagsMap targetDataFlags = instance.GetDataFlags().GetDataFlagsForPatching(&instance.GetEntityIdToBaseMap());
            AZ::ObjectStream::FilterDescriptor filterDesc(AZ::Data::AssetFilterNoAssetLoading);

            AZ::SliceComponent::InstantiatedContainer sourceObjects(false);
            dependentSlice->GetEntities(sourceObjects.m_entities);
            dependentSlice->GetAllMetadataEntities(sourceObjects.m_metadataEntities);

            const DataPatch& dataPatch = instance.GetDataPatch();
            auto instantiated =
                dataPatch.Apply(&sourceObjects, dependentSlice->GetSerializeContext(), filterDesc, sourceDataFlags, targetDataFlags);

            // Replace all the entities in the instance with the new patched ones.  To do this, we'll remove all existing entities
            // throughout the entire nested hierarchy, then add the new patched entities back in at the appropriate place in the hierarchy.
            // (This is easier than trying to figure out what the patched data changes are - we can let the JSON patch handle it for us)

            nestedInstance->RemoveEntitiesInHierarchy(
                [](const AZStd::unique_ptr<AZ::Entity>&)
                {
                    return true;
                });

            AZStd::vector<AZStd::pair<AZ::Entity*, AzToolsFramework::Prefab::Instance*>> addedEntityList;

            for (auto& entity : instantiated->m_entities)
            {
                auto entityEntry = m_aliasIdMapper.find(entity->GetId());
                if (entityEntry != m_aliasIdMapper.end())
                {
                    auto& mappingStruct = entityEntry->second;

                    // Starting with the current nested instance, walk downwards through the nesting hierarchy until we're at the
                    // correct level for this instanced entity ID, then add it.  Because we're adding it with the non-instanced alias,
                    // it doesn't matter what the slice's instanced entity ID is, and the JSON patch will correctly pick up the changes
                    // we've made for this instance.
                    AzToolsFramework::Prefab::Instance* addingInstance = nestedInstance.get();
                    for (auto it = mappingStruct.m_nestedInstanceAliases.rbegin(); it != mappingStruct.m_nestedInstanceAliases.rend(); it++)
                    {
                        auto foundInstance = addingInstance->FindNestedInstance(*it);
                        if (foundInstance.has_value())
                        {
                            addingInstance = &(foundInstance->get());
                        }
                        else
                        {
                            AZ_Assert(false, "Couldn't find nested instance %s", it->c_str());
                        }
                    }
                    UpdateCachedTransform(*entity);
                    addingInstance->AddEntity(*entity, mappingStruct.m_entityAlias);
                    addedEntityList.emplace_back(entity, addingInstance);
                }
                else
                {
                    AZ_Assert(false, "Failed to find entity alias.");
                    UpdateCachedTransform(*entity);
                    nestedInstance->AddEntity(*entity);
                    addedEntityList.emplace_back(entity, nestedInstance.get());
                }
            }

            for (auto& [entity, addingInstance] : addedEntityList)
            {
                // Fix up the parent hierarchy:
                // - Invalid parents need to get set to the container.
                // - Valid parents into the top-level instance mean that the nested slice instance is also child-nested under an entity.
                //   Prefabs handle this type of nesting differently - we need to set the parent to the container, and the container's
                //   parent to that other instance.
                auto containerEntity = addingInstance->GetContainerEntity();
                auto containerEntityId = containerEntity->get().GetId();
                AzToolsFramework::Components::TransformComponent* transformComponent =
                    entity->FindComponent<AzToolsFramework::Components::TransformComponent>();
                if (transformComponent)
                {
                    bool onlySetIfInvalid = true;
                    auto parentId = transformComponent->GetParentId();
                    if (parentId.IsValid())
                    {
                        // Look to see if the parent ID exists in a different instance (i.e. an entity in the nested slice is a
                        // child of an entity in the containing slice).  If this case exists, we need to adjust the parents so that
                        // the child entity connects to the prefab container, and the *container* is the child of the entity in the
                        // containing slice.  (i.e. go from A->B to A->container->B)
                        auto parentEntry = m_aliasIdMapper.find(parentId);
                        if (parentEntry != m_aliasIdMapper.end())
                        {
                            auto& parentMappingInfo = parentEntry->second;
                            if (parentMappingInfo.m_templateId != addingInstance->GetTemplateId())
                            {
                                if (topLevelInstance->GetTemplateId() == parentMappingInfo.m_templateId)
                                {
                                    // This entity has a parent from the topLevelInstance, so get its parent ID.
                                    parentId = topLevelInstance->GetEntityId(parentMappingInfo.m_entityAlias);
                                }
                                else
                                {
                                    AzToolsFramework::Prefab::Instance* parentInstance = addingInstance;

                                    while ((parentInstance->GetParentInstance().has_value()) &&
                                           (parentInstance->GetTemplateId() != parentMappingInfo.m_templateId))
                                    {
                                        parentInstance = &(parentInstance->GetParentInstance()->get());
                                    }

                                    if (parentInstance->GetTemplateId() == parentMappingInfo.m_templateId)
                                    {
                                        parentId = parentInstance->GetEntityId(parentMappingInfo.m_entityAlias);
                                    }
                                    else
                                    {
                                        AZ_Assert(false, "Could not find parent instance");
                                    }
                                }

                                // Set the container's parent to this entity's parent, and set this entity's parent to the container
                                SetParentEntity(containerEntity->get(), parentId, false);
                                onlySetIfInvalid = false;
                            }
                            else
                            {
                                // If the parent ID is valid and exists inside the same slice instance (i.e. template IDs are equal)
                                // then it's just a nested entity hierarchy inside the slice and we don't need to adjust anything.
                                // "onlySetIfInvalid" will still be true, which means we won't change the parent ID below.
                            }
                        }
                        else
                        {
                            // If the parent ID is set to something valid, but we can't find it in our ID mapper, something went wrong.
                            // We'll assert, but don't change the container entity's parent below.
                            AZ_Assert(false, "Could not find parent entity id: %s", parentId.ToString().c_str());
                        }

                    }

                    SetParentEntity(*entity, containerEntityId, onlySetIfInvalid);
                }
            }


            // Set the container entity of the nested prefab to have the top-level prefab as the parent if it hasn't already gotten
            // another entity as its parent.
            {
                auto containerEntity = nestedInstance->GetContainerEntity();
                constexpr bool onlySetIfInvalid = true;
                SetParentEntity(containerEntity->get(), topLevelInstance->GetContainerEntityId(), onlySetIfInvalid);
            }

            // After doing all of the above, run through entity references in any of the patched entities, and fix up the entity IDs to
            // match the new ones in our prefabs.
            RemapIdReferences(m_aliasIdMapper, topLevelInstance, nestedInstance.get(), instantiated, dependentSlice->GetSerializeContext());

            // Add the nested instance itself to the top-level prefab.  To do this, we need to add it to our top-level instance,
            // create a patch out of it, and patch the top-level prefab template.

            AzToolsFramework::Prefab::PrefabDom topLevelInstanceDomBefore;
            instanceToTemplateInterface->GenerateDomForInstance(topLevelInstanceDomBefore, *topLevelInstance);

            // Use the deterministic instance alias for this new instance
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

            AZ::Interface<AzToolsFramework::Prefab::InstanceUpdateExecutorInterface>::Get()->UpdateTemplateInstancesInQueue();

            return true;
        }

        void SliceConverter::SetParentEntity(const AZ::Entity& entity, const AZ::EntityId& parentId, bool onlySetIfInvalid)
        {
            AzToolsFramework::Components::TransformComponent* transformComponent =
                entity.FindComponent<AzToolsFramework::Components::TransformComponent>();
            if (transformComponent)
            {
                // Only set the parent if we didn't set the onlySetIfInvalid flag, or if we did and the parent is currently invalid
                if (!onlySetIfInvalid || !transformComponent->GetParentId().IsValid())
                {
                    transformComponent->SetParent(parentId);
                    transformComponent->UpdateCachedWorldTransform();
                }
            }
        }

        void SliceConverter::UpdateCachedTransform(const AZ::Entity& entity)
        {
            AzToolsFramework::Components::TransformComponent* transformComponent =
                entity.FindComponent<AzToolsFramework::Components::TransformComponent>();
            if (transformComponent)
            {
                transformComponent->UpdateCachedWorldTransform();
            }
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

        bool SliceConverter::SavePrefab(AZ::IO::PathView outputPath, AzToolsFramework::Prefab::TemplateId templateId)
        {
            auto prefabLoaderInterface = AZ::Interface<AzToolsFramework::Prefab::PrefabLoaderInterface>::Get();

            AZStd::string out;
            if (prefabLoaderInterface->SaveTemplateToString(templateId, out))
            {
                IO::SystemFile outputFile;
                if (!outputFile.Open(
                    AZStd::string(outputPath.Native()).c_str(),
                    IO::SystemFile::OpenMode::SF_OPEN_CREATE |
                    IO::SystemFile::OpenMode::SF_OPEN_CREATE_PATH |
                    IO::SystemFile::OpenMode::SF_OPEN_WRITE_ONLY))
                {
                    AZ_Error("Convert-Slice", false, "  Unable to create output file '%.*s'.", AZ_STRING_ARG(outputPath.Native()));
                    return false;
                }

                outputFile.Write(out.data(), out.size());
                outputFile.Close();
                return true;
            }

            AZ_Printf("Convert-Slice", "  Could not save prefab - internal error (Json write operation failure).\n");
            return false;
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

        void SliceConverter::UpdateSliceEntityInstanceMappings(
            const AZ::SliceComponent::EntityIdToEntityIdMap& sliceEntityIdMap, const AZStd::string& currentInstanceAlias)
        {
            // For each instanced entity, map its ID all the way back to the original prefab template and entity ID that it came from.
            // This counts on being run recursively from the leaf nodes upwards, so we first get B->A,
            // then C->B which becomes a C->A entry, then D->C which becomes D->A, etc.
            for (auto& [newId, oldId] : sliceEntityIdMap)
            {
                // Try to find the conversion chain from the old ID.  if it's there, copy it and use it for the new ID, plus add this
                // instance's name to the end of the chain.  If it's not there, skip it, since it's probably the slice metadata entity,
                // which we didn't convert.
                auto parentEntry = m_aliasIdMapper.find(oldId);
                if (parentEntry != m_aliasIdMapper.end())
                {
                    // Only add this instance's name if we don't already have an entry for the new ID.
                    if (m_aliasIdMapper.find(newId) == m_aliasIdMapper.end())
                    {
                        auto newMappingEntry = m_aliasIdMapper.emplace(newId, parentEntry->second).first;
                        newMappingEntry->second.m_nestedInstanceAliases.emplace_back(currentInstanceAlias);
                    }
                    else
                    {
                        // If we already had an entry for the new ID, it might be because the old and new ID are the same.  This happens
                        // when nesting multiple prefabs directly underneath each other without a nesting entity in-between.
                        // If the IDs are different, it's an unexpected error condition.
                        AZ_Assert(oldId == newId, "The same entity instance ID has unexpectedly appeared twice in the same nested prefab.");
                    }
                }
                else
                {
                    AZ_Warning("Convert-Slice", false, "  Couldn't find an entity ID conversion for %s.", oldId.ToString().c_str());
                }
            }
        }

        void SliceConverter::RemapIdReferences(
            const AZStd::unordered_map<AZ::EntityId, SliceEntityMappingInfo>& idMapper,
            AzToolsFramework::Prefab::Instance* topLevelInstance,
            AzToolsFramework::Prefab::Instance* nestedInstance,
            SliceComponent::InstantiatedContainer* instantiatedEntities,
            SerializeContext* context)
        {
            // Given a set of instantiated entities, run through all of them, look for entity references, and replace the entity IDs with
            // new ones that match up with our prefabs.

            IdUtils::Remapper<EntityId>::ReplaceIdsAndIdRefs(
                instantiatedEntities,
                [idMapper, &topLevelInstance, &nestedInstance](
                    const EntityId& sourceId, bool isEntityId, [[maybe_unused]] const AZStd::function<EntityId()>& idGenerator) -> EntityId
                {
                    EntityId newId = sourceId;

                    // Only convert valid entity references.  Actual entity IDs have already been taken care of elsewhere, so ignore them.
                    if (!isEntityId && sourceId.IsValid())
                    {
                        auto entityEntry = idMapper.find(sourceId);

                        // The id mapping table should include all of our known slice entities, slice metadata entities, and prefab
                        // container entities.  If we can't find the entity reference, it should either be because it's actually invalid
                        // in the source data or because it's a transform parent id that we've already remapped prior to this point.
                        // Either way, just keep it as-is and return it.
                        if (entityEntry == idMapper.end())
                        {
                            return sourceId;
                        }

                        // We've got a slice->prefab mapping entry, so now we need to use it.
                        auto& mappingStruct = entityEntry->second;

                        if (mappingStruct.m_nestedInstanceAliases.empty())
                        {
                            // If we don't have a chain of nested instance aliases, then this entity reference is either within the
                            // current nested instance or it's pointing to an entity in the top-level instance.  We'll try them both
                            // to look for a match.

                            EntityId prefabId = nestedInstance->GetEntityId(mappingStruct.m_entityAlias);
                            if (!prefabId.IsValid())
                            {
                                prefabId = topLevelInstance->GetEntityId(mappingStruct.m_entityAlias);
                            }

                            if (prefabId.IsValid())
                            {
                                newId = prefabId;
                            }
                            else
                            {
                                AZ_Error("Convert-Slice", false, "  Couldn't find source ID %s", sourceId.ToString().c_str());
                                newId = sourceId;
                            }
                        }
                        else
                        {
                            // We *do* have a chain of nested instance aliases.  This chain could either be relative to the nested instance
                            // or the top-level instance.  We can tell which one it is by which one can find the first nested instance
                            // alias.

                            AzToolsFramework::Prefab::Instance* entityInstance = nestedInstance;
                            auto it = mappingStruct.m_nestedInstanceAliases.rbegin();
                            if (!entityInstance->FindNestedInstance(*it).has_value())
                            {
                                entityInstance = topLevelInstance;
                            }

                            // Now that we've got a starting point, iterate through the chain of nested instance aliases to find the
                            // correct instance to get the entity ID for.  We have to go from slice IDs -> entity aliases -> entity IDs
                            // because prefab instance creation can change some of our entity IDs along the way.
                            for (; it != mappingStruct.m_nestedInstanceAliases.rend(); it++)
                            {
                                auto foundInstance = entityInstance->FindNestedInstance(*it);
                                if (foundInstance.has_value())
                                {
                                    entityInstance = &(foundInstance->get());
                                }
                                else
                                {
                                    AZ_Assert(false, "Couldn't find nested instance %s", it->c_str());
                                }
                            }

                            EntityId prefabId = entityInstance->GetEntityId(mappingStruct.m_entityAlias);
                            if (prefabId.IsValid())
                            {
                                newId = prefabId;
                            }
                        }
                    }

                    return newId;
                },
                context);
        }

    } // namespace SerializeContextTools
} // namespace AZ
