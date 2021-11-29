/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "EditorLayerComponent.h"
#include <AzCore/IO/FileIO.h>
#include <AzCore/Asset/AssetSerializer.h>
#include <AzCore/RTTI/ReflectContext.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/Utils.h>
#include <AzCore/Slice/SliceAsset.h>
#include <AzCore/std/containers/stack.h>
#include <AzCore/std/string/regex.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/API/ComponentEntityObjectBus.h>
#include <AzToolsFramework/API/EntityCompositionRequestBus.h>
#include <AzToolsFramework/Entity/EditorEntityContextBus.h>
#include <AzToolsFramework/Entity/EditorEntityInfoBus.h>
#include <AzToolsFramework/Entity/SliceEditorEntityOwnershipServiceBus.h>
#include <AzToolsFramework/ToolsComponents/EditorLockComponentBus.h>
#include <AzToolsFramework/ToolsComponents/EditorVisibilityBus.h>
#include <AzToolsFramework/ToolsComponents/TransformComponent.h>
#include <AzToolsFramework/Commands/EntityStateCommand.h>
#include <AzCore/Script/ScriptContextAttributes.h>

#include <QCryptographicHash>
AZ_PUSH_DISABLE_WARNING(4251, "-Wunknown-warning-option") // 'QFileInfo::d_ptr': class 'QSharedDataPointer<QFileInfoPrivate>' needs to have dll-interface to be used by clients of class 'QFileInfo'
#include <QDir>
#include <QDirIterator>
AZ_POP_DISABLE_WARNING
#include <QFileInfo>
#include <QMenu>
#include <QMessageBox>

namespace AzToolsFramework
{
    namespace Layers
    {
        void LayerProperties::Reflect(AZ::ReflectContext* context)
        {
            if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<LayerProperties>()
                    ->Version(2)
                    ->Field("m_color", &LayerProperties::m_color)
                    ->Field("m_saveAsBinary", &LayerProperties::m_saveAsBinary)
                    ->Field("m_isLayerVisible", &LayerProperties::m_isLayerVisible)
                    ;

                AZ::EditContext* editContext = serializeContext->GetEditContext();
                if (editContext)
                {
                    // m_isLayerVisible is purposely omitted from the editor here, it is toggled through the outliner.
                    editContext->Class<LayerProperties>("Layer Properties", "Layer properties that are saved to the layer file, and not the level.")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                        ->DataElement(AZ::Edit::UIHandlers::Color, &LayerProperties::m_color, "Color", "Color for the layer.")
                        ->DataElement(AZ::Edit::UIHandlers::CheckBox, &LayerProperties::m_saveAsBinary, "Save as binary", "Save the layer as an XML or binary file.");
                }
            }
        }

        void EditorLayer::Reflect(AZ::ReflectContext* context)
        {
            if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<EditorLayer>()
                    ->Version(3)
                    ->Field("layerEntities", &EditorLayer::m_layerEntities)
                    ->Field("sliceAssetsToSliceInstances", &EditorLayer::m_sliceAssetsToSliceInstances)
                    ->Field("m_layerProperties", &EditorLayer::m_layerProperties)
                    ->Field("m_layerEntityId", &EditorLayer::m_layerEntityId)
                    ;
            }
        }

        void EditorLayerComponent::Reflect(AZ::ReflectContext* context)
        {
            LayerProperties::Reflect(context);
            EditorLayer::Reflect(context);
            if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<EditorLayerComponent, EditorComponentBase>()
                    ->Version(2)
                    ->Field("m_layerFileName", &EditorLayerComponent::m_layerFileName)
                    ->Field("m_editableLayerProperties", &EditorLayerComponent::m_editableLayerProperties);

                AZ::EditContext* editContext = serializeContext->GetEditContext();
                if (editContext)
                {
                    editContext->Class<EditorLayerComponent>("Layer", "The layer component allows entities to be saved to different files on disk.")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::Icon, "Icons/Components/Layers.svg")
                        ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Icons/Components/Layers.svg")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("Layer", 0xe4db211a))
                        ->Attribute(AZ::Edit::Attributes::AddableByUser, false)
                        ->Attribute(AZ::Edit::Attributes::RemoveableByUser, false)
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->DataElement(AZ::Edit::UIHandlers::Default, &EditorLayerComponent::m_editableLayerProperties, "Layer Properties", "Layer properties that are saved to the layer file, and not the level.")
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly);
                }
            }

            if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
            {
                behaviorContext->EBus<EditorLayerComponentRequestBus>("EditorLayerComponentRequestBus")
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                    ->Attribute(AZ::Script::Attributes::Module, "layers")
                    ->Event("SetVisibility", &EditorLayerComponentRequestBus::Events::SetLayerChildrenVisibility)
                    ->Event("SetLayerColor", &EditorLayerComponentRequestBus::Events::SetLayerColor)
                    ->Event("GetColorPropertyValue", &EditorLayerComponentRequestBus::Events::GetColorPropertyValue)
                    ;
                behaviorContext->Class<EditorLayerComponent>("EditorLayerComponent")
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                    ->Attribute(AZ::Script::Attributes::Module, "layers")
                    ->Method("CreateLayerEntityFromName", &EditorLayerComponent::CreateLayerEntityFromName)
                    ->Attribute(AZ::Script::Attributes::Alias, "create_layer_entity")
                    ->Method("RecoverLayer", &EditorLayerComponent::RecoverLayer)
                    ->Attribute(AZ::Script::Attributes::Alias, "recover_layer")
                    ;
            }
        }

        void EditorLayerComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services)
        {
            services.push_back(AZ_CRC("EditorLayerService", 0x3e120934));
        }

        void EditorLayerComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services)
        {
            services.push_back(AZ_CRC("EditorLayerService", 0x3e120934));
        }

        void EditorLayerComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& services)
        {
            // Even though Layers hide their position and rotation, layers require access to the transform service
            // for hierarchy information.
            services.push_back(AZ_CRC("TransformService", 0x8ee22c50));
        }

        EditorLayerComponent::~EditorLayerComponent()
        {
            // We disconnect from the bus here because we need to be able to respond even if the entity and component are not active
            EditorLayerComponentRequestBus::Handler::BusDisconnect();
            EditorLayerInfoRequestsBus::Handler::BusDisconnect();

            if (m_loadedLayer != nullptr)
            {
                AZ_Warning("Layers", false, "Layer %s did not unload itself correctly.", GetEntity()->GetName().c_str());
                CleanupLoadedLayer();
            }
        }

        void EditorLayerComponent::Init()
        {
            // We connect to the bus here because we need to be able to respond even if the entity and component are not active
            EditorLayerComponentRequestBus::Handler::BusConnect(GetEntityId());
            EditorLayerInfoRequestsBus::Handler::BusConnect();
        }

        void EditorLayerComponent::Activate()
        {
            // We need to set the layer as static to stop the runtime complaining about static transforms
            // with a non-static parent. Done here to catch old layers that aren't set.
            AZ::TransformBus::Event(GetEntityId(), &AZ::TransformBus::Events::SetIsStaticTransform, true);

            AZ::TransformNotificationBus::Handler::BusConnect(GetEntityId());

            // When an entity activates, it needs to know its visibility and lock state.
            // Unfortunately there is no guarantee on entity activation order, so if that entity
            // is in a layer and activates before the layer, then the entity can't retrieve the layer's
            // lock or visibility overrides. To work around this, the layer refreshes the visibility
            // and lock state on its children.

            bool isLocked = false;
            AzToolsFramework::EditorEntityInfoRequestBus::EventResult(
                isLocked,
                GetEntityId(),
                &AzToolsFramework::EditorEntityInfoRequestBus::Events::IsJustThisEntityLocked);

            if (!m_editableLayerProperties.m_isLayerVisible ||
                isLocked)
            {
                AZStd::vector<AZ::EntityId> descendants;
                AZ::TransformBus::EventResult(
                    descendants,
                    GetEntityId(),
                    &AZ::TransformBus::Events::GetAllDescendants);

                for (AZ::EntityId descendant : descendants)
                {
                    // Don't need to worry about double checking the hierarchy for a layer overwriting another layer,
                    // layers can only turn visibility off or lock descendants, so there is no risk of conflict.

                    // Check if the descendant is a layer because layers aren't directly changed by these settings and can be skipped.
                    bool isLayerEntity = false;
                    AzToolsFramework::Layers::EditorLayerComponentRequestBus::EventResult(
                        isLayerEntity,
                        descendant,
                        &AzToolsFramework::Layers::EditorLayerComponentRequestBus::Events::HasLayer);
                    if (isLayerEntity)
                    {
                        continue;
                    }

                    ComponentEntityEditorRequestBus::Event(descendant, &ComponentEntityEditorRequestBus::Events::RefreshVisibilityAndLock);
                }
            }

            AzToolsFramework::Layers::EditorLayerComponentNotificationBus::Broadcast(
                &AzToolsFramework::Layers::EditorLayerComponentNotifications::OnLayerComponentActivated, GetEntityId());
        }

        void EditorLayerComponent::Deactivate()
        {
            AzToolsFramework::Layers::EditorLayerComponentNotificationBus::Broadcast(
                &AzToolsFramework::Layers::EditorLayerComponentNotifications::OnLayerComponentDeactivated, GetEntityId());

            AZ::TransformNotificationBus::Handler::BusDisconnect();
        }

        void EditorLayerComponent::UpdateLayerNameConflictMapping(AZStd::unordered_map<AZStd::string, int>& nameConflictsMapping)
        {
            AZ::Outcome<AZStd::string, AZStd::string> layerNameResult = GetLayerBaseFileName();
            if (layerNameResult.IsSuccess())
            {
                AZStd::string layerName = layerNameResult.GetValue();

                EntityIdSet layerEntities;
                AzToolsFramework::Layers::EditorLayerInfoRequestsBus::Broadcast(
                    &AzToolsFramework::Layers::EditorLayerInfoRequestsBus::Events::GatherLayerEntitiesWithName,
                    layerName,
                    layerEntities);

                if (layerEntities.size() > 1)
                {
                    nameConflictsMapping[layerName] = static_cast<int>(layerEntities.size());
                }
            }
        }

        QColor EditorLayerComponent::GetLayerColor()
        {
            // LY uses AZ classes for serialization, but the color to display needs to be a Qt color.
            // Ignore the alpha channel, it's not represented visually.
            return QColor::fromRgbF(m_editableLayerProperties.m_color.GetR(), m_editableLayerProperties.m_color.GetG(), m_editableLayerProperties.m_color.GetB());
        }

        AZ::Color EditorLayerComponent::GetColorPropertyValue()
        {
            return m_editableLayerProperties.m_color;
        }

        AzToolsFramework::Layers::LayerProperties::SaveFormat EditorLayerComponent::GetSaveFormat()
        {
            if (m_editableLayerProperties.m_saveAsBinary)
            {
                return LayerProperties::SaveFormat::Binary;
            } 
            else 
            {
                return LayerProperties::SaveFormat::Xml;
            }
        }

        bool EditorLayerComponent::IsSaveFormatBinary()
        {
            return m_editableLayerProperties.m_saveAsBinary;
        }

        bool EditorLayerComponent::IsLayerNameValid()
        {
            LayerResult fileNameGenerationResult = GenerateLayerFileName();
            // If the name can't be generated, it's definitely not a valid layer name.
            if (!fileNameGenerationResult.IsSuccess())
            {
                return false;
            }
            EntityIdSet layerEntities;
            EditorLayerInfoRequestsBus::Broadcast(
                &EditorLayerInfoRequestsBus::Events::GatherLayerEntitiesWithName,
                m_layerFileName,
                layerEntities);
            return layerEntities.size() == 1;
        }

        void EditorLayerComponent::GatherLayerEntitiesWithName(const AZStd::string& layerName, EntityIdSet& layerEntities)
        {
            LayerResult fileNameGenerationResult = GenerateLayerFileName();
            // Only gather if this layer's name is available.
            if (fileNameGenerationResult.IsSuccess())
            {
                if (azstricmp(m_layerFileName.c_str(), layerName.c_str()) == 0)
                {
                    layerEntities.insert(GetEntityId());
                }
            }
        }

        void EditorLayerComponent::MarkLayerWithUnsavedChanges()
        {
            if (m_hasUnsavedChanges)
            {
                return;
            }
            SetUnsavedChanges(true);
        }

        void EditorLayerComponent::SetOverwriteFlag(bool set)
        {
            m_overwriteCheck = set;
        }

        void EditorLayerComponent::SetUnsavedChanges(bool unsavedChanges)
        {
            m_hasUnsavedChanges = unsavedChanges;
            EditorEntityInfoNotificationBus::Broadcast(
                &EditorEntityInfoNotificationBus::Events::OnEntityInfoUpdatedUnsavedChanges, GetEntityId());
        }

        void EditorLayerComponent::CanParentChange(bool &parentCanChange, AZ::EntityId /*oldParent*/, AZ::EntityId newParent)
        {
            if (newParent.IsValid())
            {
                bool isLayerEntity = false;
                AzToolsFramework::Layers::EditorLayerComponentRequestBus::EventResult(
                    isLayerEntity,
                    newParent,
                    &AzToolsFramework::Layers::EditorLayerComponentRequestBus::Events::HasLayer);
                if (!isLayerEntity)
                {
                    parentCanChange = false;
                }
            }
        }

        LayerResult EditorLayerComponent::WriteLayerAndGetEntities(
            QString levelAbsoluteFolder,
            EntityList& entityList,
            AZ::SliceComponent::SliceReferenceToInstancePtrs& layerInstances)
        {
            AZ_PROFILE_FUNCTION(AzToolsFramework);
            EditorLayer layer;
            LayerResult layerPrepareResult = PrepareLayerForSaving(layer, entityList, layerInstances);
            if (!layerPrepareResult.IsSuccess())
            {
                return layerPrepareResult;
            }

            AZStd::vector<char> entitySaveBuffer;
            AZ::IO::ByteContainerStream<AZStd::vector<char> > entitySaveStream(&entitySaveBuffer);
            LayerResult streamCreationResult = WriteLayerToStream(layer, entitySaveStream);
            if (!streamCreationResult.IsSuccess())
            {
                return streamCreationResult;
            }
            return WriteLayerStreamToDisk(levelAbsoluteFolder, entitySaveStream);
        }

        void EditorLayerComponent::RestoreEditorData()
        {
            m_editableLayerProperties = m_cachedLayerProperties;
        }

        LayerResult EditorLayerComponent::ReadLayer(
            QString levelPakFile,
            AZ::SliceComponent::SliceAssetToSliceInstancePtrs& sliceInstances,
            AZStd::unordered_map<AZ::EntityId, AZ::Entity*>& uniqueEntities)
        {
            AZ_PROFILE_FUNCTION(AzToolsFramework);
            // If this layer is being loaded, it won't have a level save dependency yet, so clear that flag.
            m_mustSaveLevelWhenLayerSaves = false;
            QString fullPathName = levelPakFile;
            if (fullPathName.contains("@projectroot@"))
            {
                AZ::IO::FileIOBase* fileIO = AZ::IO::FileIOBase::GetInstance();
                // Resolving the path through resolvepath would normalize and lowcase it, and in this case, we don't want that.
                fullPathName.replace("@projectroot@", fileIO->GetAlias("@projectroot@"));
            }

            QFileInfo fileNameInfo(fullPathName);
            QDir layerFolder(fileNameInfo.absoluteDir());
            if (!layerFolder.cd(GetLayerDirectory()))
            {
                return LayerResult(
                    LayerResultStatus::Error,
                    QObject::tr("Unable to access layer folder %1 for layer %2.").arg(layerFolder.absoluteFilePath(GetLayerDirectory())).arg(m_layerFileName.c_str()));
            }

            QString myLayerFileName = QString("%1.%2").arg(m_layerFileName.c_str()).arg(GetLayerExtension());
            if (!layerFolder.exists(myLayerFileName))
            {
                return LayerResult(
                    LayerResultStatus::Error,
                    QObject::tr("Unable to access layer %1.").arg(layerFolder.absoluteFilePath(myLayerFileName)));
            }

            QString fullLayerPath(layerFolder.filePath(myLayerFileName));

            // Open 3D Engine will read in the layer in whatever format it's in, so there's no need to check what the save format is set to.
            // The save format is also set in this object that is being loaded, so it wouldn't even be available.
            m_loadedLayer = AZ::Utils::LoadObjectFromFile<EditorLayer>(fullLayerPath.toUtf8().data());

            if (!m_loadedLayer)
            {
                return LayerResult(
                    LayerResultStatus::Error,
                    QObject::tr("Unable to load from disk layer %1.").arg(layerFolder.absoluteFilePath(myLayerFileName)));
            }

            for (auto&& [sliceAsset, sliceInstancePointer] : m_loadedLayer->m_sliceAssetsToSliceInstances)
            {
                sliceAsset.BlockUntilLoadComplete();
            }

            return PopulateFromLoadedLayerData(*m_loadedLayer, sliceInstances, uniqueEntities);
        }

        LayerResult EditorLayerComponent::PopulateFromLoadedLayerData(
            const EditorLayer& loadedLayer,
            AZ::SliceComponent::SliceAssetToSliceInstancePtrs& sliceInstances,
            AZStd::unordered_map<AZ::EntityId, AZ::Entity*>& uniqueEntities)
        {
            // The properties for the component are saved in the layer data instead of the component data
            // so users can change these without checking out the level file.
            m_cachedLayerProperties = loadedLayer.m_layerProperties;
            m_editableLayerProperties = m_cachedLayerProperties;
            // We need to set the overwrite check to false as we know the layer is the same as the disk file
            m_overwriteCheck = false;

            AddUniqueEntitiesAndInstancesFromEditorLayer(loadedLayer, sliceInstances, uniqueEntities);

            return LayerResult::CreateSuccess();
        }

        void EditorLayerComponent::AddUniqueEntitiesAndInstancesFromEditorLayer(
            const EditorLayer& loadedLayer,
            AZ::SliceComponent::SliceAssetToSliceInstancePtrs& sliceInstances,
            AZStd::unordered_map<AZ::EntityId, AZ::Entity*>& uniqueEntities)
        {
            // This tracks entities that are in this current layer, so duplicate entities in this layer can be cleaned up.
            AZStd::unordered_map<AZ::EntityId, AZStd::vector<AZ::Entity*>::iterator> layerEntityIdsToIterators;

            for (AZStd::vector<AZ::Entity*>::iterator loadedLayerEntity = m_loadedLayer->m_layerEntities.begin();
                loadedLayerEntity != m_loadedLayer->m_layerEntities.end();)
            {
                AZ::EntityId currentEntityId = (*loadedLayerEntity)->GetId();
                AZStd::unordered_map<AZ::EntityId, AZ::Entity*>::iterator existingEntity =
                    uniqueEntities.find(currentEntityId);
                if (existingEntity != uniqueEntities.end())
                {
                    AZ_Warning(
                        "Layers",
                        false,
                        "Multiple entities that exist in your scene with ID %s and name %s were found. Attempting to load the last entity found in layer %s.",
                        currentEntityId.ToString().c_str(),
                        (*loadedLayerEntity)->GetName().c_str(),
                        m_layerFileName.c_str());
                    AZ::Entity* duplicateEntity = existingEntity->second;
                    existingEntity->second = *loadedLayerEntity;
                    delete duplicateEntity;
                }
                else
                {
                    uniqueEntities[currentEntityId] = (*loadedLayerEntity);
                }

                AZStd::unordered_map<AZ::EntityId, AZStd::vector<AZ::Entity*>::iterator>::iterator inLayerDuplicateSearch =
                    layerEntityIdsToIterators.find(currentEntityId);
                if (inLayerDuplicateSearch != layerEntityIdsToIterators.end())
                {
                    // Always keep the last found entity. If an entity cloning situation previously occurred,
                    // this is the entity most likely to have the latest changes from the content creator.
                    AZ_Warning(
                        "Layers",
                        false,
                        "Multiple entities that exist in your scene with ID %s and name %s were found in layer %s. The last entity found has been loaded.",
                        currentEntityId.ToString().c_str(),
                        (*loadedLayerEntity)->GetName().c_str(),
                        m_layerFileName.c_str());
                    loadedLayerEntity = m_loadedLayer->m_layerEntities.erase(loadedLayerEntity);
                }
                else
                {
                    layerEntityIdsToIterators[currentEntityId] = loadedLayerEntity;
                    ++loadedLayerEntity;
                }
            }

            for (const auto& instanceIter :
                loadedLayer.m_sliceAssetsToSliceInstances)
            {
                sliceInstances[instanceIter.first].insert(
                    instanceIter.second.begin(),
                    instanceIter.second.end());
            }
        }

        void EditorLayerComponent::CleanupLoadedLayer()
        {
            delete m_loadedLayer;
            m_loadedLayer = nullptr;
        }

        void EditorLayerComponent::SetSaveFormat(LayerProperties::SaveFormat saveFormat) 
        { 
            m_editableLayerProperties.m_saveAsBinary = saveFormat == LayerProperties::SaveFormat::Binary;
        }

        LayerResult EditorLayerComponent::PrepareLayerForSaving(
            EditorLayer& layer,
            EntityList& entityList,
            AZ::SliceComponent::SliceReferenceToInstancePtrs& layerInstances)
        {
            AZ_PROFILE_FUNCTION(AzToolsFramework);
            // Move the editable data into the data serialized to the layer, and not the layer component.
            layer.m_layerProperties = m_editableLayerProperties;
            layer.m_layerEntityId = GetEntityId();
            m_cachedLayerProperties = m_editableLayerProperties;
            // When serialization begins, clear out the editor data so it doesn't serialize to the level.
            m_editableLayerProperties.Clear();

            AzToolsFramework::Components::TransformComponent* transformComponent =
                GetEntity()->FindComponent<AzToolsFramework::Components::TransformComponent>();
            if (!transformComponent)
            {
                return LayerResult(
                    LayerResultStatus::Error,
                    QObject::tr("Unable to find the Transform component for layer %1 %2. This layer will not be exported.").
                    arg(GetEntity()->GetName().c_str()).
                    arg(GetEntity()->GetId().ToString().c_str()));
            }

            // GenerateLayerFileName populates m_layerFileName.
            LayerResult fileNameGenerationResult = GenerateLayerFileName();
            if (!fileNameGenerationResult.IsSuccess())
            {
                return fileNameGenerationResult;
            }

            // For MVP, don't save the layer if there is a name collision.
            // When this save occurs, the layer entities will be saved in the level so they won't be lost.
            if (!IsLayerNameValid())
            {
                return LayerResult(
                    LayerResultStatus::Error,
                    QObject::tr("Layers with the name %1 can't be saved. Layer names at the same hierarchy level must be unique.").
                    arg(GetEntity()->GetName().c_str()));
            }

            // Entity pointers are necessary for saving entities to disk.
            // Grabbing the entities this way also guarantees that only entities that are safe to save to disk
            // will be included in the layer. Slice entities cannot yet be included in layers, only loose entities.
            EntityList editorEntities;
            AzToolsFramework::EditorEntityContextRequestBus::Broadcast(
                &AzToolsFramework::EditorEntityContextRequestBus::Events::GetLooseEditorEntities,
                editorEntities);
            AZStd::unordered_map<AZ::EntityId, AZ::Entity*> entityIdsToEntityPtrs;
            for (AZ::Entity* entity : editorEntities)
            {
                entityIdsToEntityPtrs[entity->GetId()] = entity;
            }

            // All descendants of the layer entity are included in the layer.
            // Note that the layer entity itself is not included in the layer.
            // This is intentional, it allows the level to track what layers exist and only
            // load the layers it expects to see.
            GatherAllNonLayerNonSliceDescendants(entityList, layer, entityIdsToEntityPtrs, *transformComponent);


            AZ::SliceComponent* rootSlice = nullptr;
            AzToolsFramework::SliceEditorEntityOwnershipServiceRequestBus::BroadcastResult(rootSlice,
                &AzToolsFramework::SliceEditorEntityOwnershipServiceRequestBus::Events::GetEditorRootSlice);
            if (rootSlice)
            {
                AZ::SliceComponent::SliceList& slices = rootSlice->GetSlices();
                for (AZ::SliceComponent::SliceReference& sliceReference : slices)
                {
                    // Don't save the root slice instance in the layer.
                    if (sliceReference.GetSliceAsset().Get()->GetComponent() == rootSlice)
                    {
                        continue;
                    }
                    AZ::SliceComponent::SliceReference::SliceInstances& sliceInstances = sliceReference.GetInstances();
                    for (AZ::SliceComponent::SliceInstance& sliceInstance : sliceInstances)
                    {
                        const AZ::SliceComponent::InstantiatedContainer* instantiated = sliceInstance.GetInstantiated();
                        if (!instantiated || instantiated->m_entities.empty())
                        {
                            continue;
                        }
                        AZ::Entity* rootEntity = instantiated->m_entities[0];

                        AzToolsFramework::Components::TransformComponent* sliceTransform =
                            rootEntity->FindComponent<AzToolsFramework::Components::TransformComponent>();

                        if (!sliceTransform)
                        {
                            continue;
                        }

                        for (AZ::TransformInterface* transformLayerSearch = sliceTransform;
                            transformLayerSearch != nullptr && transformLayerSearch->GetParent() != nullptr;
                            transformLayerSearch = transformLayerSearch->GetParent())
                        {
                            bool sliceAncestorParentIsLayer = false;
                            AzToolsFramework::Layers::EditorLayerComponentRequestBus::EventResult(
                                sliceAncestorParentIsLayer,
                                transformLayerSearch->GetParentId(),
                                &AzToolsFramework::Layers::EditorLayerComponentRequestBus::Events::HasLayer);

                            bool amISliceAncestorParent = transformLayerSearch->GetParentId() == GetEntityId();

                            if (sliceAncestorParentIsLayer && !amISliceAncestorParent)
                            {
                                // This slice instance will be handled by a different layer.
                                break;
                            }
                            else if (amISliceAncestorParent)
                            {
                                sliceReference.ComputeDataPatch(&sliceInstance);
                                layer.m_sliceAssetsToSliceInstances[sliceReference.GetSliceAsset()].insert(&sliceInstance);
                                layerInstances[&sliceReference].insert(&sliceInstance);
                                break;
                            }
                        }
                    }
                }
            }
            return LayerResult::CreateSuccess();
        }

        LayerResult EditorLayerComponent::WriteLayerToStream(
            const EditorLayer& layer,
            AZ::IO::ByteContainerStream<AZStd::vector<char> >& entitySaveStream)
        {
            AZ_PROFILE_FUNCTION(AzToolsFramework);
            m_otherLayersToSave.clear();
            m_mustSaveLevelWhenLayerSaves = false;

            bool savedEntitiesResult = false;
            AZ::DataStream::StreamType saveStreamType = layer.m_layerProperties.m_saveAsBinary ? AZ::DataStream::ST_BINARY : AZ::DataStream::ST_XML;
            savedEntitiesResult = AZ::Utils::SaveObjectToStream<EditorLayer>(entitySaveStream, saveStreamType, &layer);

            AZStd::string layerBaseFileName(m_layerFileName);

            if (!savedEntitiesResult)
            {
                return LayerResult(LayerResultStatus::Error, QObject::tr("Unable to serialize layer %1.").arg(layerBaseFileName.c_str()));
            }
            entitySaveStream.Seek(0, AZ::IO::GenericStream::ST_SEEK_BEGIN);
            return LayerResult::CreateSuccess();
        }

        LayerResult EditorLayerComponent::WriteLayerStreamToDisk(
            QString levelAbsoluteFolder,
            const AZ::IO::ByteContainerStream<AZStd::vector<char> >& entitySaveStream)
        {
            AZ_PROFILE_FUNCTION(AzToolsFramework);
            AZStd::string layerBaseFileName(m_layerFileName);

            // Write to a temp file first.
            QDir layerTempFolder(levelAbsoluteFolder);
            QString tempLayerDirectory = GetLayerTempDirectory();

            LayerResult layerErrorResult = CreateDirectoryAtPath(layerTempFolder.absoluteFilePath(tempLayerDirectory).toUtf8().data());
            if (!layerErrorResult.IsSuccess())
            {
                return layerErrorResult;
            }


            if (!layerTempFolder.cd(tempLayerDirectory))
            {
                return LayerResult(LayerResultStatus::Error, QObject::tr("Unable to access directory %1.").arg(tempLayerDirectory));
            }

            QString tempLayerFullPath;
            for (int tempWriteTries = 0; tempWriteTries < GetMaxTempFileWriteAttempts(); ++tempWriteTries)
            {

                QString tempLayerFileName(QString("%1.%2.%3").
                    arg(layerBaseFileName.c_str()).
                    arg(GetLayerTempExtension()).
                    arg(tempWriteTries));
                tempLayerFullPath = layerTempFolder.absoluteFilePath(tempLayerFileName);

                AZ::IO::FileIOStream fileStream(tempLayerFullPath.toUtf8().data(), AZ::IO::OpenMode::ModeWrite | AZ::IO::OpenMode::ModeBinary);
                if (fileStream.IsOpen())
                {
                    if (!fileStream.Write(entitySaveStream.GetLength(), entitySaveStream.GetData()->data()))
                    {
                        layerErrorResult = LayerResult(
                            LayerResultStatus::Error,
                            QObject::tr("Unable to write layer %1 to the stream.").arg(layerBaseFileName.c_str()));
                    }
                    fileStream.Close();
                }
                if (layerErrorResult.IsSuccess())
                {
                    break;
                }
                else
                {
                    // Just record a warning for temp file writes that fail, don't both trying to add these to the final outcome.
                    AZ_Warning("Layers", false, "Unable to save temp file %1 for layer.", tempLayerFullPath.toUtf8().data());
                }
            }

            if (!layerErrorResult.IsSuccess())
            {
                return layerErrorResult;
            }

            AZ::IO::FileIOBase* fileIO = AZ::IO::FileIOBase::GetInstance();

            // The real file

            QDir layerFolder(levelAbsoluteFolder);
            QString layerDirectory = GetLayerDirectory();
            layerErrorResult = CreateDirectoryAtPath(layerFolder.absoluteFilePath(layerDirectory).toUtf8().data());
            if (!layerErrorResult.IsSuccess())
            {
                return layerErrorResult;
            }

            if (!layerFolder.cd(layerDirectory))
            {
                return LayerResult(LayerResultStatus::Error, QObject::tr("Unable to access directory %1.").arg(layerDirectory));
            }
            QString layerFileName(QString("%1.%2").arg(layerBaseFileName.c_str()).arg(GetLayerExtension()));
            QString layerFullPath(layerFolder.absoluteFilePath(layerFileName));

            if (layerFolder.exists(layerFileName))
            {
                QCryptographicHash layerHash(QCryptographicHash::Sha1);
                layerErrorResult = GetFileHash(layerFullPath, layerHash);
                if (!layerErrorResult.IsSuccess())
                {
                    return CleanupTempFileAndFolder(tempLayerFullPath, layerTempFolder, &layerErrorResult);
                }

                // Generate a hash of what was actually written to disk, don't use the in-memory stream.
                // This makes it easy to account for things like newline differences between operating systems.
                QCryptographicHash tempLayerHash(QCryptographicHash::Sha1);
                layerErrorResult = GetFileHash(tempLayerFullPath, tempLayerHash);
                if (!layerErrorResult.IsSuccess())
                {
                    return CleanupTempFileAndFolder(tempLayerFullPath, layerTempFolder, &layerErrorResult);
                }

                // This layer file hasn't actually changed, so return a success.
                if (layerHash.result() == tempLayerHash.result())
                {
                    SetUnsavedChanges(false);
                    return CleanupTempFileAndFolder(tempLayerFullPath, layerTempFolder, nullptr);
                }
            }

            // No need to specifically check for read-only state, if Perforce is not connected, it will
            // automatically overwrite the file.
            bool checkedOutSuccessfully = false;
            AzToolsFramework::ToolsApplicationRequestBus::BroadcastResult(
                checkedOutSuccessfully,
                &ToolsApplicationRequestBus::Events::RequestEditForFileBlocking,
                layerFullPath.toUtf8().data(),
                "Checking out for edit...",
                ToolsApplicationRequestBus::Events::RequestEditProgressCallback());

            if (!checkedOutSuccessfully)
            {
                LayerResult failure(
                    LayerResultStatus::Error,
                    QObject::tr("Layer file %1 is unavailable to edit. Check your source control settings or if the file is locked by another tool.").arg(layerFullPath));
                return CleanupTempFileAndFolder(tempLayerFullPath, layerTempFolder, &failure);
            }

            if (fileIO->Exists(layerFullPath.toUtf8().data()))
            {
                if (m_overwriteCheck)
                {
                    QWidget* mainWindow = nullptr;
                    AzToolsFramework::EditorRequestBus::BroadcastResult(
                        mainWindow,
                        &AzToolsFramework::EditorRequestBus::Events::GetMainWindow);
                    AZStd::string levelName;
                    AzToolsFramework::EditorRequestBus::BroadcastResult(levelName, &AzToolsFramework::EditorRequests::GetLevelName);
                    int overwriteMessageResult = QMessageBox::warning(mainWindow,
                        QObject::tr("Overwrite existing layer"),
                        QObject::tr("A layer named %1 already exists in the %2/layers folder. Do you want to overwrite this layer?").arg(layerBaseFileName.c_str()).arg(levelName.c_str()),
                        QMessageBox::Ok | QMessageBox::Cancel,
                        QMessageBox::Ok);
                    if (overwriteMessageResult  == QMessageBox::Cancel)
                    {
                       // The user chose to cancel this operation.
                        return CleanupTempFileAndFolder(tempLayerFullPath, layerTempFolder, nullptr);
                    }
                }
                AZ::IO::Result removeResult = fileIO->Remove(layerFullPath.toUtf8().data());
                if (!removeResult)
                {
                    LayerResult failure(
                        LayerResultStatus::Error,
                        QObject::tr("Unable to write changes for layer %1. Check whether the file is locked by another tool.").arg(layerFullPath));
                    return CleanupTempFileAndFolder(tempLayerFullPath, layerTempFolder, &failure);
                }
            }

            AZ::IO::Result renameResult = fileIO->Rename(tempLayerFullPath.toUtf8().data(), layerFullPath.toUtf8().data());

            if (!renameResult)
            {
                LayerResult failure(LayerResultStatus::Error, QObject::tr("Unable to save layer %1.").arg(layerFullPath));
                return CleanupTempFileAndFolder(tempLayerFullPath, layerTempFolder, &failure);
            }
            SetUnsavedChanges(false);
            m_overwriteCheck = false;
            return CleanupTempFolder(layerTempFolder, nullptr);
        }

        LayerResult EditorLayerComponent::GenerateLayerFileName()
        {
            // The layer name will be populated in the recursive function.
            m_layerFileName.clear();

            AZ::Outcome<AZStd::string, AZStd::string> layerFileNameResult = GetLayerBaseFileName();
            if (!layerFileNameResult.IsSuccess())
            {
                return LayerResult(
                    LayerResultStatus::Error,
                    QObject::tr("Unable to generate a file name for the layer. The content for the layer will save in the level instead of a layer file. %1").arg(layerFileNameResult.GetError().c_str()));
            }
            m_layerFileName = layerFileNameResult.GetValue();
            return LayerResult::CreateSuccess();
        }

        AZ::Outcome<AZStd::string, AZStd::string> EditorLayerComponent::GetLayerBaseFileName()
        {
            AZ::EntityId entityId(GetEntityId());
            AZ::Entity* myEntity(GetEntity());
            if (!entityId.IsValid() || !myEntity)
            {
                return AZ::Failure(AZStd::string("Entity is unavailable for this layer."));
            }
            AZ::EntityId nextAncestor;
            AZ::TransformBus::EventResult(
                /*result*/ nextAncestor,
                /*address*/ entityId,
                &AZ::TransformBus::Events::GetParentId);

            AZStd::string layerName = myEntity->GetName();
            if (nextAncestor.IsValid())
            {
                AZ::Outcome<AZStd::string, AZStd::string> ancestorLayerNameResult(
                    AZ::Failure(AZStd::string("A layer has a non-layer parent, which is unsupported. This layer will save to the level instead of a layer file.")));
                AzToolsFramework::Layers::EditorLayerComponentRequestBus::EventResult(
                    ancestorLayerNameResult,
                    nextAncestor,
                    &AzToolsFramework::Layers::EditorLayerComponentRequestBus::Events::GetLayerBaseFileName);

                if (!ancestorLayerNameResult.IsSuccess())
                {
                    return ancestorLayerNameResult;
                }
                // Layers use their parents as a namespace to minimize collisions,
                // so a child layer's name is "ParentName.LayerName".
                layerName = AZStd::string::format("%s%s%s", ancestorLayerNameResult.GetValue().c_str(), GetLayerSeparator().c_str(), layerName.c_str());
            }
            return AZ::Success(layerName);
        }

        bool EditorLayerComponent::GatherSaveDependencies(
            AZStd::unordered_set<AZ::EntityId>& allLayersToSave,
            bool& mustSaveLevel)
        {
            bool dependenciesChanged = false;
            if (m_mustSaveLevelWhenLayerSaves)
            {
                mustSaveLevel = true;
                dependenciesChanged = true;
            }
            // There should only be a warning popup to save content if there are additional layers to save.
            for (const AZ::EntityId& dependency : m_otherLayersToSave)
            {
                if (allLayersToSave.find(dependency) == allLayersToSave.end())
                {
                    dependenciesChanged = true;
                    allLayersToSave.insert(dependency);
                }
            }
            return dependenciesChanged;
        }

        void EditorLayerComponent::AddLayerSaveDependency(const AZ::EntityId& layerSaveDependency)
        {
            m_otherLayersToSave.insert(layerSaveDependency);
        }

        void EditorLayerComponent::AddLevelSaveDependency()
        {
            m_mustSaveLevelWhenLayerSaves = true;
        }

        AZ::Outcome<AZStd::string, AZStd::string> EditorLayerComponent::GetLayerFullFileName()
        {
            AZ::Outcome<AZStd::string, AZStd::string> layerBaseNameResult = GetLayerBaseFileName();

            if (!layerBaseNameResult.IsSuccess())
            {
                return layerBaseNameResult;
            }

            AZStd::string layerFullFileName(
                AZStd::string::format("%s%s",layerBaseNameResult.GetValue().c_str(),GetLayerExtensionWithDot().c_str()));
            return AZ::Success(layerFullFileName);
        }

        AZ::Outcome<AZStd::string, AZStd::string> EditorLayerComponent::GetLayerFullFilePath(const QString& levelAbsoluteFolder)
        {
            AZ::Outcome<AZStd::string, AZStd::string> layerFullNameResult = GetLayerFullFileName();
            if (!layerFullNameResult.IsSuccess())
            {
                return layerFullNameResult;
            }

            AZStd::string layerFullFileName = layerFullNameResult.GetValue();
            QDir layerFolder(levelAbsoluteFolder);
            QString layerDirectory = GetLayerDirectory();

            if (!layerFolder.cd(layerDirectory) || !layerFolder.exists(layerFullFileName.c_str()))
            {
                return AZ::Failure(AZStd::string());
            }

            AZStd::string fullFilePath;
            AzFramework::StringFunc::Path::Join(levelAbsoluteFolder.toUtf8().data(), layerDirectory.toUtf8().data(), fullFilePath);
            AzFramework::StringFunc::Path::Join(fullFilePath.c_str(), layerFullFileName.c_str(), fullFilePath);
            return AZ::Success(fullFilePath);
        }

        bool EditorLayerComponent::HasUnsavedChanges()
        {
            return m_hasUnsavedChanges;
        }

        bool EditorLayerComponent::DoesLayerFileExistOnDisk(const QString& levelAbsoluteFolder)
        {
            AZ::Outcome<AZStd::string, AZStd::string> layerFullNameResult = GetLayerFullFileName();
            if (!layerFullNameResult.IsSuccess())
            {
                return false;
            }

            AZStd::string layerFullFileName = layerFullNameResult.GetValue();
            QDir layerFolder(levelAbsoluteFolder);
            QString layerDirectory = GetLayerDirectory();

            return !layerFolder.cd(layerDirectory) || !layerFolder.exists(layerFullFileName.c_str());
        }

        LayerResult EditorLayerComponent::GetFileHash(QString filePath, QCryptographicHash& hash)
        {
            QFile fileForHash(filePath);
            if (fileForHash.open(QIODevice::ReadOnly))
            {
                hash.addData(fileForHash.readAll());
                fileForHash.close();
                return LayerResult::CreateSuccess();
            }
            return LayerResult(LayerResultStatus::Error, QObject::tr("Unable to compute hash for layer %1. This layer will not be saved.").arg(filePath));
        }

        LayerResult EditorLayerComponent::CreateDirectoryAtPath(const QString& path)
        {
            const QString cleanPath = QDir::toNativeSeparators(QDir::cleanPath(path));
            QFileInfo fileInfo(cleanPath);
            if (fileInfo.isDir())
            {
                return LayerResult::CreateSuccess();
            }
            else if (!fileInfo.exists())
            {
                if (QDir().mkpath(cleanPath))
                {
                    return LayerResult::CreateSuccess();
                }
            }
            return LayerResult(LayerResultStatus::Error, QObject::tr("Unable to create directory at %1.").arg(path));
        }

        LayerResult EditorLayerComponent::GenerateCleanupFailureWarningResult(QString message, const LayerResult* currentFailure)
        {
            LayerResult result(LayerResultStatus::Warning, message);
            if (currentFailure)
            {
                // CleanupTempFileAndFolder can only fail with a warning, so if a failure was provided, use that failure's level.
                if (currentFailure->m_result != LayerResultStatus::Success)
                {
                    result.m_result = currentFailure->m_result;
                }
                if (!currentFailure->m_message.isEmpty())
                {
                    // If a message was provided, then treat that as the primary warning, and append the cleanup warning.
                    result.m_message = (QObject::tr("%1, with additional warning: %2").
                        arg(currentFailure->m_message).arg(result.m_message));
                }
            }
            return result;
        }

        LayerResult EditorLayerComponent::CleanupTempFileAndFolder(
            QString tempFile,
            const QDir& layerTempFolder,
            const LayerResult* currentFailure)
        {
            LayerResult cleanupFailureResult(GenerateCleanupFailureWarningResult(
                QObject::tr("Temp layer file %1 was not removed.").arg(tempFile),
                currentFailure));

            AZ::IO::FileIOBase* fileIO = AZ::IO::FileIOBase::GetInstance();
            if (!fileIO)
            {
                return cleanupFailureResult;
            }
            AZ::IO::Result removeResult = fileIO->Remove(tempFile.toUtf8().data());
            if (!removeResult)
            {
                return cleanupFailureResult;
            }

            // Any errors would have already been handled so we don't need to pass on an error.
            LayerResult cleanupSuccessResult = LayerResult::CreateSuccess();
            return CleanupTempFolder(layerTempFolder, &cleanupSuccessResult);
        }

        LayerResult EditorLayerComponent::CleanupTempFolder(const QDir& layerTempFolder, const LayerResult* currentFailure)
        {
            LayerResult cleanupFailureResult(GenerateCleanupFailureWarningResult(
                QObject::tr("Temp layer folder %1 was not removed.").arg(layerTempFolder.absolutePath()),
                currentFailure));

            // FileIO doesn't support removing directory, so use Qt.
            if (layerTempFolder.entryInfoList(
                QDir::NoDotAndDotDot | QDir::AllEntries | QDir::System | QDir::Hidden).count() == 0)
            {
                if (layerTempFolder.rmdir("."))
                {
                    if (currentFailure)
                    {
                        return *currentFailure;
                    }
                    else
                    {
                        return LayerResult::CreateSuccess();
                    }
                }
                return cleanupFailureResult;
            }
            // It's not an error if the folder wasn't empty.
            if (currentFailure)
            {
                return cleanupFailureResult;
            }
            else
            {
                return LayerResult::CreateSuccess();
            }
        }

        void EditorLayerComponent::GatherAllNonLayerNonSliceDescendants(
            EntityList& entityList,
            EditorLayer& layer,
            const AZStd::unordered_map<AZ::EntityId, AZ::Entity*>& entityIdsToEntityPtrs,
            AzToolsFramework::Components::TransformComponent& transformComponent) const
        {
            AZStd::vector<AZ::EntityId> children = transformComponent.GetChildren();
            for (AZ::EntityId child : children)
            {
                AZStd::unordered_map<AZ::EntityId, AZ::Entity*>::const_iterator childEntityIdToPtr =
                    entityIdsToEntityPtrs.find(child);
                if (childEntityIdToPtr == entityIdsToEntityPtrs.end())
                {
                    // We only want loose entities, ignore slice entities, they are tracked differently.
                    continue;
                }

                bool isChildLayerEntity = false;
                AzToolsFramework::Layers::EditorLayerComponentRequestBus::EventResult(
                    isChildLayerEntity,
                    child,
                    &AzToolsFramework::Layers::EditorLayerComponentRequestBus::Events::HasLayer);
                if (isChildLayerEntity)
                {
                    // Don't add child layers or their descendants.
                    // All layers are currently tracked by the scene slice,
                    // and the descendants will be tracked by that child layer.
                    continue;
                }

                entityList.push_back(childEntityIdToPtr->second);
                layer.m_layerEntities.push_back(childEntityIdToPtr->second);

                AzToolsFramework::Components::TransformComponent* childTransform =
                childEntityIdToPtr->second->FindComponent<AzToolsFramework::Components::TransformComponent>();
                if (childTransform)
                {
                    GatherAllNonLayerNonSliceDescendants(entityList, layer, entityIdsToEntityPtrs, *childTransform);
                }
            }
        }

        void EditorLayerComponent::CreateLayerAssetContextMenu(QMenu* menu, const AZStd::string& fullFilePath, QString levelPath)
        {
            if (!menu)
            {
                return;
            }

            // Make sure tooltips are enabled, so additional information can be shown.
            menu->setToolTipsVisible(true);

            // Errors are handled and prevented in two ways:
            //  If the error is simple and doesn't need any user feedback, then create the menu option, but greyed out.
            //  If there is need for user feedback, then create the menu option and show an error after it's selected.
            //      If the missing layer requires another missing layer to be recovered first, then an error message
            //          is useful because there's an action that can be taken to resolve, which is to recover the other layer first.

            // Slash directions might not match, so use QDir to fix that.
            QDir filePathFixedSlashes(fullFilePath.c_str());
            QDir levelPathFixedSlashes(levelPath);

            QAction* layerRecoveryAction = menu->addAction(QObject::tr("Recover layer"), [fullFilePath]()
            {
                RecoverLayer(fullFilePath);
            });
            // Start with the basic tooltip. Any reasons this can't be recovered will update the tooltip.
            layerRecoveryAction->setToolTip(QObject::tr("Recover a layer created in this level"));

            bool isLayerRecoveryAvailable = true;
            // To minimize issues with entity ID collision, prevent importing layers from outside this level.
            if (!filePathFixedSlashes.absolutePath().startsWith(levelPathFixedSlashes.absolutePath()))
            {
                isLayerRecoveryAvailable = false;
                layerRecoveryAction->setToolTip(QObject::tr("Layers from different levels cannot be recovered"));
            }

            AZStd::string fileNameNoExtension;
            AzFramework::StringFunc::Path::GetFileName(fullFilePath.c_str(), fileNameNoExtension);

            EntityIdSet layerEntities;
            AzToolsFramework::Layers::EditorLayerInfoRequestsBus::Broadcast(
                &AzToolsFramework::Layers::EditorLayerInfoRequestsBus::Events::GatherLayerEntitiesWithName,
                fileNameNoExtension,
                layerEntities);

            // If this layer is already in the scene and loaded, then it can't be recovered.
            if (!layerEntities.empty())
            {
                isLayerRecoveryAvailable = false;
                layerRecoveryAction->setToolTip(QObject::tr("Layer already exists in this level"));
            }

            layerRecoveryAction->setEnabled(isLayerRecoveryAvailable);
        }

        bool EditorLayerComponent::CanAttemptToRecoverLayerAndGetLayerInfo(
            const AZStd::string& fullFilePath,
            AZStd::string& newLayerName,
            AZ::EntityId& layerParentId)
        {
            AZStd::string fileNameNoExtension;
            AzFramework::StringFunc::Path::GetFileName(fullFilePath.c_str(), fileNameNoExtension);

            // Sanitize the separator, because there's a good chance it is a special character, like '.'.
            AZStd::regex specialCharacters{ R"([-[\]{}()*+?.,\^$|#\s])" };
            AZStd::string layerAncestorSeparatorSanitized(AZStd::regex_replace(GetLayerSeparator(), specialCharacters, AZStd::string(R"(\$&)")));

            AZStd::regex ancestorRegex(AZStd::string::format("(.*)%s(.*)$", layerAncestorSeparatorSanitized.c_str()));
            AZStd::string layerAncestor(AZStd::regex_replace(fileNameNoExtension, ancestorRegex, "$1"));

            // If this layer has ancestry, check if it's available in the scene.
            if (layerAncestor.compare(fileNameNoExtension) != 0)
            {
                // Check if the layer has a parent, and if that parent is available.
                EntityIdSet layerEntities;
                AzToolsFramework::Layers::EditorLayerInfoRequestsBus::Broadcast(
                    &AzToolsFramework::Layers::EditorLayerInfoRequestsBus::Events::GatherLayerEntitiesWithName,
                    layerAncestor,
                    layerEntities);
                if (layerEntities.size() > 1)
                {
                    AZ_Error(
                        "LayerRecovery",
                        false,
                        "Unable to recover the layer. There is more than one layer with the name %s in the scene. Rename these layers and try again.",
                        layerAncestor.c_str());
                    return false;
                }
                newLayerName = AZStd::regex_replace(fileNameNoExtension, ancestorRegex, "$2");
                if (layerEntities.empty())
                {
                    QWidget* mainWindow = nullptr;
                    AzToolsFramework::EditorRequestBus::BroadcastResult(
                        mainWindow,
                        &AzToolsFramework::EditorRequestBus::Events::GetMainWindow);
                    QMessageBox rebuildAncestryMessageBox(mainWindow);
                    rebuildAncestryMessageBox.setWindowTitle(QObject::tr("Missing layer hierarchy"));
                    rebuildAncestryMessageBox.setText(QObject::tr("The recovered layer's hierarchy doesn't exist in the scene."));
                    rebuildAncestryMessageBox.setInformativeText(QObject::tr("A parent layer named %0 will be created if you recover this layer.").arg(layerAncestor.c_str()));
                    rebuildAncestryMessageBox.setIcon(QMessageBox::Warning);
                    rebuildAncestryMessageBox.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
                    rebuildAncestryMessageBox.setDefaultButton(QMessageBox::Cancel);
                    int rebuildAncestryMessageBoxResult = rebuildAncestryMessageBox.exec();
                    switch (rebuildAncestryMessageBoxResult)
                    {
                    case QMessageBox::Ok:
                        layerParentId = CreateMissingLayerAncestors(layerAncestor);
                        break;
                    case QMessageBox::Cancel:
                        // The user chose to cancel this operation.
                        return false;
                    }
                }
                else
                {
                    layerParentId = *layerEntities.begin();
                }
            }
            // There is no ancestry, so the parent ID is invalid and the entity name is just the file name.
            else
            {
                newLayerName = fileNameNoExtension;
                layerParentId = AZ::EntityId();
            }
            return true;
        }

        AZ::EntityId EditorLayerComponent::CreateMissingLayerAncestors(const AZStd::string& nearestLayerAcenstorName)
        {
            // The standard default color for layers is defined in a Qt JSON file that can't be accessed from here.
            AZ::Color missingLayerColor(0.5f, 0.5f, 0.5f, 1.0f);
            // Create a single layer matching the passed in ancestory.
            return CreateLayerEntity(nearestLayerAcenstorName, missingLayerColor, LayerProperties::SaveFormat::Xml);
        }

        LayerResult EditorLayerComponent::UpdateListOfDiscoveredEntityIds(AZStd::unordered_set<AZ::EntityId> &discoveredEntityIds, const AZ::EntityId& newEntity)
        {
            if (discoveredEntityIds.find(newEntity) != discoveredEntityIds.end())
            {
                return LayerResult(
                    LayerResultStatus::Error,
                    QObject::tr("This layer contains entity IDs that already exist. This layer can't be recovered."));
            }
            discoveredEntityIds.insert(newEntity);
            return LayerResult::CreateSuccess();
        }

        LayerResult EditorLayerComponent::IsLayerDataSafeToRecover(const AZStd::shared_ptr<Layers::EditorLayer> loadedLayer, AZ::SliceComponent& rootSlice)
        {
            if (loadedLayer.get() == nullptr)
            {
                return LayerResult(LayerResultStatus::Error, "Cannot recover a null layer.");
            }

            if (!loadedLayer->m_layerEntityId.IsValid())
            {
                // Layers missing the layer entity ID never shipped externally.
                // Internal teams will be given the release note that they have to re-save layers
                // from before this update to be able to recover them.
                return LayerResult(LayerResultStatus::Error, "This layer lacks necessary recovery information and may have been created before the recover layer feature was available. This layer cannot be recovered.");
            }

            // Track all entity IDs that have been found so far.
            AZStd::unordered_set<AZ::EntityId> usedEntityIds;

            // Make sure the incoming layer doesn't have any duplicate entity IDs.
            for (AZ::Entity* loadedEntity : loadedLayer->m_layerEntities)
            {
                LayerResult discoveryUpdateResult = UpdateListOfDiscoveredEntityIds(usedEntityIds, loadedEntity->GetId());
                if(!discoveryUpdateResult.IsSuccess())
                {
                    return discoveryUpdateResult;
                }
            }

            EntityList editorEntities;
            rootSlice.GetEntities(editorEntities);

            // Make sure the layer itself is not in the scene.
            LayerResult layerDiscoveryResult = UpdateListOfDiscoveredEntityIds(usedEntityIds, loadedLayer->m_layerEntityId);
            if (!layerDiscoveryResult.IsSuccess())
            {
                return layerDiscoveryResult;
            }


            // Make sure the layer doesn't have entity IDs that conflict with other entity IDs in the scene.
            for (AZ::Entity* editorEntity : editorEntities)
            {
                LayerResult discoveryUpdateResult = UpdateListOfDiscoveredEntityIds(usedEntityIds, editorEntity->GetId());
                if (!discoveryUpdateResult.IsSuccess())
                {
                    return discoveryUpdateResult;
                }
            }

            for (const auto& assetToSliceInstanceIter :
                loadedLayer->m_sliceAssetsToSliceInstances)
            {
                if (assetToSliceInstanceIter.first.IsError())
                {
                    return LayerResult(LayerResultStatus::Error, "This layer contains a reference to a slice that can't be loaded. This layer can't be recovered.");
                }

                for (const auto& sliceInstance : assetToSliceInstanceIter.second)
                {
                    if (sliceInstance == nullptr)
                    {
                        return LayerResult(LayerResultStatus::Error, "This layer contains a slice that can't be loaded. This layer can't be recovered.");
                    }
                    for (const auto& sliceIdToInstanceId : sliceInstance->GetEntityIdMap())
                    {
                        // Check if the instance's entity ID was already in the scene.
                        LayerResult discoveryUpdateResult = UpdateListOfDiscoveredEntityIds(usedEntityIds, sliceIdToInstanceId.second);
                        if (!discoveryUpdateResult.IsSuccess())
                        {
                            return discoveryUpdateResult;
                        }
                    }
                }
            }

            return LayerResult::CreateSuccess();
        }

        AZ::EntityId EditorLayerComponent::CreateLayerEntityFromName(const AZStd::string& name)
        {
            AZ::Color layerColor(0.5f, 0.5f, 0.5f, 1.0f);
            return CreateLayerEntity(name, layerColor, LayerProperties::SaveFormat::Xml);
        }

        AZ::EntityId EditorLayerComponent::CreateLayerEntity(const AZStd::string& name, const AZ::Color& layerColor, const LayerProperties::SaveFormat& saveFormat, const AZ::EntityId& optionalEntityId)
        {
            AzToolsFramework::ScopedUndoBatch undo("New Layer");
            AZ::EntityId newEntityId;
            if (optionalEntityId.IsValid())
            {
                AzToolsFramework::EditorEntityContextRequestBus::BroadcastResult(
                    newEntityId,
                    &AzToolsFramework::EditorEntityContextRequestBus::Events::CreateNewEditorEntityWithId,
                    name.c_str(),
                    optionalEntityId);
            }
            else
            {
                AzToolsFramework::EditorEntityContextRequestBus::BroadcastResult(
                    newEntityId,
                    &AzToolsFramework::EditorEntityContextRequestBus::Events::CreateNewEditorEntity,
                    name.c_str());
            }

            if (!newEntityId.IsValid())
            {
                AZ_Error("Editor", false, "Unable to create a layer entity.");
                return newEntityId;
            }

            // Layers are not visible in the 3D viewport, turn them off.
            // The outliner won't allow anyone to turn them back on.
            AzToolsFramework::EditorVisibilityRequestBus::Event(
                newEntityId,
                &AzToolsFramework::EditorVisibilityRequests::SetVisibilityFlag,
                false);

            AzToolsFramework::EntityIdList selection = { newEntityId };
            AzToolsFramework::ToolsApplicationRequests::Bus::Broadcast(
                &AzToolsFramework::ToolsApplicationRequests::Bus::Events::SetSelectedEntities,
                selection);

            AzToolsFramework::Layers::EditorLayerComponent* newLayer = aznew AzToolsFramework::Layers::EditorLayerComponent();

            newLayer->SetLayerColor(layerColor);
            newLayer->SetSaveFormat(saveFormat);
            newLayer->SetOverwriteFlag(true);

            AZStd::vector<AZ::Component*> newComponents;
            newComponents.push_back(newLayer);

            AzToolsFramework::Layers::EditorLayerCreationBus::Broadcast(
                &AzToolsFramework::Layers::EditorLayerCreationBus::Events::OnNewLayerEntity,
                newEntityId,
                newComponents);

            AzToolsFramework::EntityCompositionRequestBus::Broadcast(
                &AzToolsFramework::EntityCompositionRequests::AddExistingComponentsToEntityById,
                newEntityId,
                newComponents);

            return newEntityId;
        }

        void EditorLayerComponent::RecoverLayer(const AZStd::string& fullFilePath)
        {
            // Recovering a layer takes these steps:
            // 1: Perform error checking that can occur before loading.
            //      Layers with missing parents need to present the user with a choice to re-generate
            //          the hierarchy, or cancel the operation so they can resolve it a different way.
            // 2: Load the layer off disk into a LayerComponent.
            // 3: Validate that it's safe to add the entities and slice instances to the level.
            //      Unexpected issues can occur if multiple entities exist with the same entity ID.
            //          These issues range from crashing, to memory leaks, to actions performing on the wrong entity.
            // 4: If it's safe, then add that layer to the scene. If it's not safe, erase it all and show an error.

            AZStd::string newLayerName;
            AZ::EntityId layerParentId;

            // Create undo batch now so that any ancestor creation is included in the recovery undo.
            AzToolsFramework::ScopedUndoBatch undo("RecoverLayer");
            if (!CanAttemptToRecoverLayerAndGetLayerInfo(fullFilePath, newLayerName, layerParentId))
            {
                return;
            }

            AZStd::shared_ptr<EditorLayer> recoveredLayer(AZ::Utils::LoadObjectFromFile<EditorLayer>(fullFilePath.c_str()));
            if (!recoveredLayer)
            {
                AZ_Error(
                    "LayerRecovery",
                    false,
                    "The layer file at path %s could not be loaded to be recovered.",
                    fullFilePath.c_str());
                return;
            }

            LayerResult result = RecoverEditorLayer(recoveredLayer, newLayerName, layerParentId);
            result.MessageResult();
        }

        LayerResult EditorLayerComponent::RecoverEditorLayer(
            const AZStd::shared_ptr<Layers::EditorLayer> editorLayer,
            const AZStd::string& newLayerName,
            const AZ::EntityId& layerParentId)
        {
            if (editorLayer.get() == nullptr)
            {
                return LayerResult(LayerResultStatus::Error, "Cannot recover a null layer.");
            }
            AZ::SliceComponent* rootSlice = nullptr;
            AzToolsFramework::SliceEditorEntityOwnershipServiceRequestBus::BroadcastResult(
                rootSlice, &AzToolsFramework::SliceEditorEntityOwnershipServiceRequestBus::Events::GetEditorRootSlice);
            if (!rootSlice)
            {
                return LayerResult(LayerResultStatus::Error, "The root slice is unavailable. This layer can't be recovered. Check your editor status and try again.");
            }

            // IsLayerDataSafeToRecover reports any errors it encounters.
            LayerResult isSafeToRecoverResult = IsLayerDataSafeToRecover(editorLayer, *rootSlice);
            if (!isSafeToRecoverResult.IsSuccess())
            {
                return isSafeToRecoverResult;
            }

            AzToolsFramework::ScopedUndoBatch undo("RecoverEditorLayer");

            LayerProperties::SaveFormat saveFormat = LayerProperties::SaveFormat::Xml;
            
            if (editorLayer->m_layerProperties.m_saveAsBinary)
            {
                saveFormat = LayerProperties::SaveFormat::Binary;
            }

            AZ::EntityId newLayerEntityId = CreateLayerEntity(newLayerName, editorLayer->m_layerProperties.m_color, saveFormat, editorLayer->m_layerEntityId);

            if (!newLayerEntityId.IsValid())
            {
                return LayerResult(LayerResultStatus::Error, "Open 3D Engine was unable to create a layer entity.");
            }

            // If this new layer has a parent, then set its parent.
            if (layerParentId.IsValid())
            {
                AZ::TransformBus::Event(
                    newLayerEntityId,
                    &AZ::TransformBus::Events::SetParent,
                    layerParentId);
            }

            // Gather the slice instances on this layer so they can be added to the root slice.
            AZ::SliceComponent::SliceAssetToSliceInstancePtrs sliceInstancesToAddToScene;
            for (auto& assetToSliceInstanceIter :
                editorLayer->m_sliceAssetsToSliceInstances)
            {
                sliceInstancesToAddToScene[assetToSliceInstanceIter.first].insert(
                    assetToSliceInstanceIter.second.begin(),
                    assetToSliceInstanceIter.second.end());
            }

            AzToolsFramework::EditorEntityContextRequestBus::Broadcast(
                &AzToolsFramework::EditorEntityContextRequestBus::Events::AddEditorEntities,
                editorLayer->m_layerEntities);

            // Make commands for the entities that have been created, so that they get removed/recreated on undo/redo.
            for (AZ::Entity* entity : editorLayer->m_layerEntities)
            {
                EntityCreateCommand* command = aznew EntityCreateCommand(static_cast<AZ::u64>(entity->GetId()));
                command->Capture(entity);
                command->SetParent(undo.GetUndoBatch());
            }


            AZStd::unordered_set<const AZ::SliceComponent::SliceInstance*> instances;
            rootSlice->AddSliceInstances(sliceInstancesToAddToScene, instances);

            for (auto& sliceInstanceIter : instances)
            {
                EditorEntityContextRequestBus::Broadcast(
                    &EditorEntityContextRequests::HandleEntitiesAdded,
                    sliceInstanceIter->GetInstantiated()->m_entities);

                // Add create commands for the slice creation to the undo buffer.
                for (AZ::Entity* entity : sliceInstanceIter->GetInstantiated()->m_entities)
                {
                    EntityCreateCommand* command = aznew EntityCreateCommand(static_cast<AZ::u64>(entity->GetId()));
                    command->Capture(entity);
                    command->SetParent(undo.GetUndoBatch());
                }
            }

            return LayerResult::CreateSuccess();
        }

        void EditorLayerComponent::SetLayerChildrenVisibility(const bool visible)
        {
            if (m_editableLayerProperties.m_isLayerVisible != visible)
            {
                m_editableLayerProperties.m_isLayerVisible = visible;

                EditorEntityInfoNotificationBus::Broadcast(
                    &EditorEntityInfoNotificationBus::Events::OnEntityInfoUpdatedVisibility,
                    GetEntityId(), visible);
            }
        }
    } // namespace Layers
} // namespace AzToolsFramework
