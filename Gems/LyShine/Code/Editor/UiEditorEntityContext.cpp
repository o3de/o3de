/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "EditorCommon.h"
#include <AzCore/Component/Entity.h>
#include <AzCore/Component/EntityUtils.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/Component/ComponentExport.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/IO/ByteContainerStream.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/Serialization/Utils.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/Asset/AssetManager.h>

#include <AzFramework/API/ApplicationAPI.h>
#include <AzFramework/Entity/EntityContext.h>
#include <AzFramework/Entity/GameEntityContextBus.h>
#include <AzFramework/Asset/AssetCatalogBus.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzToolsFramework/Slice/SliceCompilation.h>
#include <AzToolsFramework/ToolsComponents/EditorOnlyEntityComponent.h>

#include <AzToolsFramework/API/EntityCompositionRequestBus.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>

#include <LyShine/Bus/UiElementBus.h>
#include <LyShine/Bus/UiTransformBus.h>
#include <LyShine/Bus/Tools/UiSystemToolsBus.h>
#include <LyShine/UiComponentTypes.h>

#include "UiEditorEntityContext.h"

namespace Internal
{
    void RemoveIncompatibleComponents(AZ::Entity* entity)
    {
        const AZ::Entity::ComponentArrayType components = entity->GetComponents();
        AZ::Entity::ComponentArrayType validComponents;
        AZ::Entity::ComponentArrayType incompatibleComponents;
        AZ::ComponentDescriptor::DependencyArrayType incompatibleServices;
        AZ::ComponentDescriptor::DependencyArrayType providedServices;
        AZStd::string incompatibleNames;
        for (auto component : components)
        {
            AZ::ComponentDescriptor* testComponentDesc = nullptr;
            AZ::ComponentDescriptorBus::EventResult(testComponentDesc, azrtti_typeid(component), &AZ::ComponentDescriptorBus::Events::GetDescriptor);
            providedServices.clear();
            testComponentDesc->GetProvidedServices(providedServices, component);

            bool isIncompatible = false;
            for (auto validComponent : validComponents)
            {
                AZ::ComponentDescriptor* validComponentDesc = nullptr;
                AZ::ComponentDescriptorBus::EventResult(validComponentDesc, azrtti_typeid(validComponent), &AZ::ComponentDescriptorBus::Events::GetDescriptor);

                incompatibleServices.clear();
                validComponentDesc->GetIncompatibleServices(incompatibleServices, validComponent);

                auto foundItr = AZStd::find_first_of(incompatibleServices.begin(), incompatibleServices.end(), providedServices.begin(), providedServices.end());
                if (foundItr != incompatibleServices.end())
                {
                    isIncompatible = true;
                    break;
                }
            }

            if (isIncompatible)
            {
                incompatibleComponents.push_back(component);

                incompatibleNames.append(testComponentDesc->GetName());
                incompatibleNames += '\n';
            }
            else
            {
                validComponents.push_back(component);
            }
        }

        // Should be safe to remove components, because the entity hasn't been activated.
        for (auto componentToRemove : incompatibleComponents)
        {
            entity->RemoveComponent(componentToRemove);
        }

        AZ_Error("UiCanvas", incompatibleComponents.empty(), "The following incompatible component(s) are removed from the entity %s:\n%s", entity->GetName().c_str(), incompatibleNames.c_str());
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
UiEditorEntityContext::UiEditorEntityContext(EditorWindow* editorWindow)
    : m_editorWindow(editorWindow)
    , m_requiredEditorComponentTypes
    ({
        azrtti_typeid<AzToolsFramework::Components::EditorOnlyEntityComponent>()
    })
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
UiEditorEntityContext::~UiEditorEntityContext()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiEditorEntityContext::HandleLoadedRootSliceEntity(AZ::Entity* rootEntity, bool remapIds, AZ::SliceComponent::EntityIdToEntityIdMap* idRemapTable)
{
    AZ_Assert(m_entityOwnershipService->IsInitialized(), "The context has not been initialized.");

    bool rootEntityReloadSuccessful = false;
    AzFramework::SliceEntityOwnershipServiceRequestBus::EventResult(rootEntityReloadSuccessful, GetContextId(),
        &AzFramework::SliceEntityOwnershipServiceRequestBus::Events::HandleRootEntityReloadedFromStream, rootEntity, remapIds, idRemapTable);

    if (!rootEntityReloadSuccessful)
    {
        return false;
    }

    AZ::SliceComponent::EntityList entities;
    m_entityOwnershipService->GetAllEntities(entities);

    AzFramework::SliceEntityOwnershipServiceRequestBus::Event(GetContextId(),
        &AzFramework::SliceEntityOwnershipServiceRequests::SetIsDynamic, true);

    InitializeEntities(entities);

    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiEditorEntityContext::InitUiContext()
{
    m_entityOwnershipService = AZStd::make_unique<AzFramework::SliceEntityOwnershipService>(GetContextId(), GetSerializeContext());
    InitContext();

    // Since root asset initialization happens in EntityOwnershipService and since this class is not inheriting from it,
    // we need to now connect to the asset bus using the root asset id here.
    AzFramework::RootSliceAsset rootSliceAsset;
    AzFramework::SliceEntityOwnershipServiceRequestBus::EventResult(rootSliceAsset, GetContextId(),
        &AzFramework::SliceEntityOwnershipServiceRequestBus::Events::GetRootAsset);
    m_rootAssetId = rootSliceAsset->GetId();
    AZ::Data::AssetBus::MultiHandler::BusConnect(m_rootAssetId);

    m_entityOwnershipService->InstantiateAllPrefabs();

    UiEntityContextRequestBus::Handler::BusConnect(GetContextId());

    UiEditorEntityContextRequestBus::Handler::BusConnect(GetContextId());

    AzToolsFramework::EditorEntityContextPickingRequestBus::Handler::BusConnect(GetContextId());
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiEditorEntityContext::DestroyUiContext()
{
    UiEditorEntityContextRequestBus::Handler::BusDisconnect();

    UiEntityContextRequestBus::Handler::BusDisconnect();

    AzToolsFramework::EditorEntityContextPickingRequestBus::Handler::BusDisconnect();

    AZ::Data::AssetBus::MultiHandler::BusDisconnect(m_rootAssetId);

    DestroyContext();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiEditorEntityContext::SaveToStreamForGame(AZ::IO::GenericStream& stream, AZ::DataStream::StreamType streamType)
{
    AZ::SliceComponent::EntityList sourceEntities;
    m_entityOwnershipService->GetAllEntities(sourceEntities);

    // Create a source slice from our editor components.
    AZ::Entity* sourceSliceEntity = aznew AZ::Entity();
    AZ::SliceComponent* sourceSliceData = sourceSliceEntity->CreateComponent<AZ::SliceComponent>();
    AZ::Data::Asset<AZ::SliceAsset> sourceSliceAsset(aznew AZ::SliceAsset(), AZ::Data::AssetLoadBehavior::Default);
    sourceSliceAsset.Get()->SetData(sourceSliceEntity, sourceSliceData);

    for (AZ::Entity* sourceEntity : sourceEntities)
    {
        sourceSliceData->AddEntity(sourceEntity);
    }

    // Emulate client flags.
    AZ::PlatformTagSet platformTags = { AZ_CRC_CE("renderer") };

    // Compile the source slice into the runtime slice (with runtime components).
    AzToolsFramework::UiEditorOnlyEntityHandler uiEditorOnlyEntityHandler;
    AzToolsFramework::EditorOnlyEntityHandlers handlers =
    {
        &uiEditorOnlyEntityHandler,
    };
    AzToolsFramework::SliceCompilationResult sliceCompilationResult = CompileEditorSlice(sourceSliceAsset, platformTags, *m_serializeContext, handlers);

    // Reclaim entities from the temporary source asset.
    for (AZ::Entity* sourceEntity : sourceEntities)
    {
        sourceSliceData->RemoveEntity(sourceEntity, false);
    }

    if (!sliceCompilationResult)
    {
        m_errorMessage = sliceCompilationResult.GetError();
        return false;
    }

    // Export runtime slice representing the level, which is a completely flat list of entities.
    AZ::Data::Asset<AZ::SliceAsset> exportSliceAsset = sliceCompilationResult.GetValue();
    AZ::Entity* exportSliceAssetEntity = exportSliceAsset.Get()->GetEntity();
    const bool saveObjectSuccess = AZ::Utils::SaveObjectToStream<AZ::Entity>(stream, streamType, exportSliceAssetEntity);

    AZ::SliceComponent* sliceComponent = exportSliceAssetEntity->FindComponent<AZ::SliceComponent>();
    AZ::SliceComponent::EntityList sliceEntities;

    const bool getEntitiesSuccess = sliceComponent->GetEntities(sliceEntities);
    const bool sliceEntitiesValid = getEntitiesSuccess && sliceEntities.size() > 0;

    if (!sliceEntitiesValid)
    {
        AZ_Error("Save Runtime Stream", false, "Failed to export entities for runtime:\n%s", sliceCompilationResult.GetError().c_str());
        return false;
    }

    return saveObjectSuccess;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiEditorEntityContext::SaveCanvasEntityToStreamForGame(AZ::Entity* canvasEntity, AZ::IO::GenericStream& stream, AZ::DataStream::StreamType streamType)
{
    AZ::Entity* sourceCanvasEntity = canvasEntity;
    AZ::Entity* exportCanvasEntity = aznew AZ::Entity(sourceCanvasEntity->GetName().c_str());
    exportCanvasEntity->SetId(sourceCanvasEntity->GetId());
    AZ_Assert(exportCanvasEntity, "Failed to create target entity \"%s\" for export.",
        sourceCanvasEntity->GetName().c_str());

    AzToolsFramework::ToolsApplicationRequests::Bus::Broadcast(
        &AzToolsFramework::ToolsApplicationRequests::Bus::Events::PreExportEntity, *sourceCanvasEntity, *exportCanvasEntity);

    // Export entity representing the canvas, which has only runtime components.
    AZ::Utils::SaveObjectToStream<AZ::Entity>(stream, streamType, exportCanvasEntity);

    AzToolsFramework::ToolsApplicationRequests::Bus::Broadcast(
        &AzToolsFramework::ToolsApplicationRequests::Bus::Events::PostExportEntity, *sourceCanvasEntity, *exportCanvasEntity);

    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZ::Entity* UiEditorEntityContext::CreateUiEntity(const char* name)
{
    AZ::Entity* entity = CreateEntity(name);

    if (entity)
    {
        // we don't currently do anything extra here, UI entities are not automatically
        // Init'ed and Activate'd when they are created. We wait until the required components
        // are added before Init and Activate
    }

    return entity;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZ::SliceComponent* UiEditorEntityContext::GetUiRootSlice()
{
    AZ::SliceComponent* rootSlice = nullptr;
    AzFramework::SliceEntityOwnershipServiceRequestBus::EventResult(rootSlice, GetContextId(),
        &AzFramework::SliceEntityOwnershipServiceRequestBus::Events::GetRootSlice);
    return rootSlice;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiEditorEntityContext::AddUiEntity(AZ::Entity* entity)
{
    AZ_Assert(entity, "Supplied entity is invalid.");

    AddEntity(entity);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiEditorEntityContext::AddUiEntities(const AzFramework::EntityList& entities)
{
    for (AZ::Entity* entity : entities)
    {
        AZ_Assert(!AzFramework::EntityIdContextQueryBus::MultiHandler::BusIsConnectedId(entity->GetId()), "Entity already in context.");
        AzFramework::RootSliceAsset rootSliceAsset;
        AzFramework::SliceEntityOwnershipServiceRequestBus::EventResult(rootSliceAsset, GetContextId(),
            &AzFramework::SliceEntityOwnershipServiceRequestBus::Events::GetRootAsset);
        rootSliceAsset->GetComponent()->AddEntity(entity);
    }

    m_entityOwnershipService->HandleEntitiesAdded(entities);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiEditorEntityContext::CloneUiEntities(const AZStd::vector<AZ::EntityId>& sourceEntities, AzFramework::EntityList& resultEntities)
{
    resultEntities.clear();

    AZ::SliceComponent::InstantiatedContainer sourceObjects(false);
    for (const AZ::EntityId& id : sourceEntities)
    {
        AZ::Entity* entity = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(entity, &AZ::ComponentApplicationBus::Events::FindEntity, id);
        if (entity)
        {
            sourceObjects.m_entities.push_back(entity);
        }
    }

    AZ::SliceComponent::EntityIdToEntityIdMap idMap;
    AZ::SliceComponent::InstantiatedContainer* clonedObjects =
        AZ::EntityUtils::CloneObjectAndFixEntities(&sourceObjects, idMap);
    if (!clonedObjects)
    {
        AZ_Error("UiEntityContext", false, "Failed to clone source entities.");
        return false;
    }

    resultEntities = clonedObjects->m_entities;

    AddUiEntities(resultEntities);

    clonedObjects->m_deleteEntitiesOnDestruction = false;
    delete clonedObjects;

    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiEditorEntityContext::DestroyUiEntity(AZ::EntityId entityId)
{
    return DestroyEntityById(entityId);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiEditorEntityContext::SupportsViewportEntityIdPicking()
{
    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZ::SliceComponent::SliceInstanceAddress UiEditorEntityContext::CloneEditorSliceInstance(
    [[maybe_unused]] AZ::SliceComponent::SliceInstanceAddress sourceInstance)
{
    return AZ::SliceComponent::SliceInstanceAddress();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AzFramework::SliceInstantiationTicket UiEditorEntityContext::InstantiateEditorSlice(const AZ::Data::Asset<AZ::Data::AssetData>& sliceAsset, AZ::Vector2 viewportPosition)
{
    return InstantiateEditorSliceAtChildIndex(sliceAsset, viewportPosition, -1);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AzFramework::SliceInstantiationTicket UiEditorEntityContext::InstantiateEditorSliceAtChildIndex(const AZ::Data::Asset<AZ::Data::AssetData>& sliceAsset,
                                                                                                AZ::Vector2 viewportPosition,
                                                                                                int childIndex)
{
    if (sliceAsset.GetId().IsValid())
    {
        InstantiatingEditorSliceParams instantiatingSliceParams(viewportPosition, childIndex);
        m_instantiatingSlices.push_back(AZStd::make_pair(sliceAsset, instantiatingSliceParams));

        AzFramework::SliceInstantiationTicket ticket;
        AzFramework::SliceEntityOwnershipServiceRequestBus::EventResult(ticket, GetContextId(),
            &AzFramework::SliceEntityOwnershipServiceRequestBus::Events::InstantiateSlice, sliceAsset, nullptr, nullptr);
        if (ticket.IsValid())
        {
            AzFramework::SliceInstantiationResultBus::MultiHandler::BusConnect(ticket);
        }

        return ticket;
    }

    return AzFramework::SliceInstantiationTicket();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiEditorEntityContext::RestoreSliceEntity(AZ::Entity* entity, const AZ::SliceComponent::EntityRestoreInfo& info)
{
    AZ_Error("EditorEntityContext", info.m_assetId.IsValid(), "Invalid asset Id for entity restore.");

    // If asset isn't loaded when this request is made, we need to queue the load and process the request
    // when the asset is ready. Otherwise we'll immediately process the request when OnAssetReady is invoked
    // by the AssetBus connection policy.

    AZ::Data::Asset<AZ::Data::AssetData> asset = 
        AZ::Data::AssetManager::Instance().GetAsset<AZ::SliceAsset>(info.m_assetId, AZ::Data::AssetLoadBehavior::Default);

    SliceEntityRestoreRequest request = {entity, info, asset};
    m_queuedSliceEntityRestores.emplace_back(request);

    AZ::Data::AssetBus::MultiHandler::BusConnect(asset.GetId());
}
    
////////////////////////////////////////////////////////////////////////////////////////////////////
void UiEditorEntityContext::QueueSliceReplacement(const char* targetPath,
                                                  const AZStd::unordered_map<AZ::EntityId, AZ::EntityId>& selectedToAssetMap,
                                                  const AZStd::unordered_set<AZ::EntityId>& entitiesInSelection,
                                                  AZ::Entity* commonParent, AZ::Entity* insertBefore)
{
    AZ_Error("EditorEntityContext", m_queuedSliceReplacement.m_path.empty(), "A slice replacement is already on the queue.");

    m_queuedSliceReplacement.Setup(targetPath, selectedToAssetMap, entitiesInSelection, commonParent, insertBefore);

    AzFramework::AssetCatalogEventBus::Handler::BusConnect();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiEditorEntityContext::DeleteElements(AzToolsFramework::EntityIdList elements)
{
    // Deletes the specified elements using an undoable command
    if (elements.size() > 0)
    {
        HierarchyWidget* hierarchy = m_editorWindow->GetHierarchy();

        // Get the list of currently selected entities so that we can attempt to restore that
        // after the delete (the undoable command currently only works on selected entities)
        QTreeWidgetItemRawPtrQList selection = hierarchy->selectedItems();
        EntityHelpers::EntityIdList selectedEntities = SelectionHelpers::GetSelectedElementIds(hierarchy, selection, false);

        // Make sure elements still exist. There is a situation related to "Push to Slice" where an
        // element to be deleted may no longer exist. This occurs if a new child slice instance is
        // pushed to its parent slice, then "undo" is performed which brings back the child instance
        // that was deleted during the "Push to Slice" process, and then the recovered child instance
        // is pushed to its parent slice again
        elements.erase(
            AZStd::remove_if(
                elements.begin(), elements.end(),
                [](AZ::EntityId entityId)
                {
                    AZ::Entity* entity = nullptr;
                    AZ::ComponentApplicationBus::BroadcastResult(entity, &AZ::ComponentApplicationBus::Events::FindEntity, entityId);
                    return !entity;
                }),
            elements.end());

        if (elements.empty())
        {
            return;
        }

        // Use an undoable command to delete the entities
        // The way the command is implemented depends upon selecting the items first
        HierarchyHelpers::SetSelectedItems(hierarchy, &elements);
        CommandHierarchyItemDelete::Push(m_editorWindow->GetActiveStack(),
            hierarchy,
            hierarchy->selectedItems());

        // Attempt to set the selection back to what it was but first remove any items from the selected
        // list that no longer exist
        selectedEntities.erase(
            std::remove_if(
                selectedEntities.begin(), selectedEntities.end(),
                [](AZ::EntityId entityId)
                {
                    AZ::Entity* entity = nullptr;
                    AZ::ComponentApplicationBus::BroadcastResult(entity, &AZ::ComponentApplicationBus::Events::FindEntity, entityId);
                    return !entity;
                }),
            selectedEntities.end());

        HierarchyHelpers::SetSelectedItems(hierarchy, &selectedEntities);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiEditorEntityContext::HasPendingRequests()
{
    if (!m_queuedSliceEntityRestores.empty())
    {
        return true;
    }

    return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiEditorEntityContext::IsInstantiatingSlices()
{    
    if (!m_instantiatingSlices.empty())
    {
        return true;
    }

    return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiEditorEntityContext::DetachSliceEntities(const AzToolsFramework::EntityIdList& entities)
{
    if (entities.empty())
    {
        return;
    }

    for (const AZ::EntityId& entityId : entities)
    {
        AZ::SliceComponent::SliceInstanceAddress sliceAddress;
        AzFramework::SliceEntityRequestBus::EventResult(sliceAddress, entityId,
            &AzFramework::SliceEntityRequestBus::Events::GetOwningSlice);

        if (sliceAddress.IsValid())
        {
            AZ::Entity* entity = nullptr;
            AZ::ComponentApplicationBus::BroadcastResult(entity, &AZ::ComponentApplicationRequests::FindEntity, entityId);
            AZ_Error("EditorEntityContext", entity, "Unable to find entity for EntityID %llu", entityId);

            if (entity)
            {
                if (sliceAddress.GetReference()->GetSliceComponent()->RemoveEntity(entityId, false)) // Remove from current slice instance without deleting
                {
                    AZ::SliceComponent* rootSlice = nullptr;
                    AzFramework::SliceEntityOwnershipServiceRequestBus::EventResult(rootSlice, GetContextId(),
                        &AzFramework::SliceEntityOwnershipServiceRequestBus::Events::GetRootSlice);
                    rootSlice->AddEntity(entity); // Add back as loose entity
                }
            }
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiEditorEntityContext::OnCatalogAssetAdded(const AZ::Data::AssetId& assetId)
{
    if (m_queuedSliceReplacement.IsValid())
    {
        AZStd::string relativePath;
        AZ::Data::AssetCatalogRequestBus::BroadcastResult(
            relativePath, &AZ::Data::AssetCatalogRequestBus::Events::GetAssetPathById, assetId);

        if (AZStd::string::npos != AzFramework::StringFunc::Find(m_queuedSliceReplacement.m_path.c_str(), relativePath.c_str()))
        {
            AzFramework::AssetCatalogEventBus::Handler::BusDisconnect();

            AZStd::unordered_set<AZ::EntityId> topLevelEntities;
            GetTopLevelEntities(m_queuedSliceReplacement.m_entitiesInSelection, topLevelEntities);

            // Request the slice instantiation.
            AZ::Data::Asset<AZ::Data::AssetData> asset =
                AZ::Data::AssetManager::Instance().FindOrCreateAsset<AZ::SliceAsset>(assetId, AZ::Data::AssetLoadBehavior::Default);
            AZ::Vector2 viewportPosition(-1.0f, -1.0f);
            m_queuedSliceReplacement.m_ticket = InstantiateEditorSlice(asset, viewportPosition);
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiEditorEntityContext::ResetContext()
{
    // First deactivate all the entities, before calling the base class ResetContext which will
    // delete them all.
    // This helps us know that we do not need to maintain the cached pointers between the UiElementComponents
    // as individual elements are destroyed.
    AZ::SliceComponent::EntityList entities;
    bool result = m_entityOwnershipService->GetAllEntities(entities);
    if (result)
    {
        for (AZ::Entity* entity : entities)
        {
            if (entity->GetState() == AZ::Entity::State::Active)
            {
                entity->Deactivate();
            }
        }
    }

    // Now reset the context which will destroy all the entities
    EntityContext::ResetContext();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiEditorEntityContext::OnSlicePreInstantiate([[maybe_unused]] const AZ::Data::AssetId& sliceAssetId, [[maybe_unused]] const AZ::SliceComponent::SliceInstanceAddress& sliceAddress)
{
    // For UI slices we don't need to do anything here. The main EditorEntityContextComponent
    // changes the transforms here. But we need the entities to be initialized and activated
    // before recalculating offsets so we do it in OnSliceInstantiated.
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiEditorEntityContext::OnSliceInstantiated(const AZ::Data::AssetId& sliceAssetId, const AZ::SliceComponent::SliceInstanceAddress& sliceAddress)
{
    AZ::Data::AssetBus::MultiHandler::BusConnect(sliceAssetId);
    const AzFramework::SliceInstantiationTicket ticket = *AzFramework::SliceInstantiationResultBus::GetCurrentBusId();

    // If we got here by creating a new slice then we have extra work to do (deleting the old entities etc)
    AZ::Entity* insertBefore = nullptr;
    if (ticket == m_queuedSliceReplacement.m_ticket)
    {
        m_queuedSliceReplacement.Finalize(sliceAddress, m_editorWindow);

        // Select the common parent (the call to Finalize will have deleted the elements that were selected)
        m_editorWindow->GetHierarchy()->SetUniqueSelectionHighlight(m_queuedSliceReplacement.m_commonParent);
        insertBefore = m_queuedSliceReplacement.m_insertBefore;
    }

    AzFramework::SliceInstantiationResultBus::MultiHandler::BusDisconnect(ticket);

    // Close out the next ticket corresponding to this asset.
    for (auto instantiatingIter = m_instantiatingSlices.begin(); instantiatingIter != m_instantiatingSlices.end(); ++instantiatingIter)
    {
        if (instantiatingIter->first.GetId() == sliceAssetId)
        {
            const AZ::SliceComponent::EntityList& entities = sliceAddress.GetInstance()->GetInstantiated()->m_entities;

            if (entities.size() == 0)
            {
                // if there are no entities there was an error with the instantiation
                AZ::Data::AssetBus::MultiHandler::BusDisconnect(sliceAssetId);
                UiEditorEntityContextNotificationBus::Broadcast(&UiEditorEntityContextNotificationBus::Events::OnSliceInstantiationFailed, sliceAssetId, ticket);
                m_instantiatingSlices.erase(instantiatingIter);
                break;
            }

            // Initialize the new entities and create a set of all the top-level entities.
            AZStd::unordered_set<AZ::Entity*> topLevelEntities;
            for (AZ::Entity* entity : entities)
            {
                if (entity->GetState() == AZ::Entity::State::Constructed)
                {
                    entity->Init();
                }
                if (entity->GetState() == AZ::Entity::State::Init)
                {
                    entity->Activate();
                }

                topLevelEntities.insert(entity);
            }

            // remove anything from the topLevelEntities set that is referenced as the child of another element in the list
            for (AZ::Entity* entity : entities)
            {
                LyShine::EntityArray children;
                UiElementBus::EventResult(children, entity->GetId(), &UiElementBus::Events::GetChildElements);
                
                for (auto child : children)
                {
                    topLevelEntities.erase(child);
                }
            }

            // This can be null if nothing is selected. That is OK, the usage of it below treats that as meaning
            // add as a child of the root element.
            AZ::Entity* parent = m_editorWindow->GetHierarchy()->CurrentSelectedElement();

            int childIndex = instantiatingIter->second.m_childIndex;
            if (!insertBefore && childIndex >= 0)
            {
                if (parent)
                {
                    UiElementBus::EventResult(insertBefore, parent->GetId(), &UiElementBus::Events::GetChildElement, childIndex);
                }
                else
                {
                    UiCanvasBus::EventResult(insertBefore, m_editorWindow->GetCanvas(), &UiCanvasBus::Events::GetChildElement, childIndex);
                }
            }

            // Now topLevelElements contains all of the top-level elements in the set of newly instantiated entities
            // Copy the topLevelEntities set into a list
            LyShine::EntityArray entitiesToInit;
            for (auto entity : topLevelEntities)
            {
                entitiesToInit.push_back(entity);
            }

            // There must be at least one element
            AZ_Assert(entitiesToInit.size() >= 1, "There must be at least one top-level entity in a UI slice.");

            // Initialize the internal parent pointers and the canvas pointer in the elements
            // We do this before adding the elements, otherwise the GetUniqueChildName code in FixupCreatedEntities will
            // already see the new elements and think the names are not unique
            UiCanvasBus::Event(m_editorWindow->GetCanvas(), &UiCanvasBus::Events::FixupCreatedEntities, entitiesToInit, true, parent);

            // Add all of the top-level entities as children of the parent
            for (auto entity : topLevelEntities)
            {
                UiCanvasBus::Event(m_editorWindow->GetCanvas(), &UiCanvasBus::Events::AddElement, entity, parent, insertBefore);
            }

            // Here we adjust the position of the instantiated entities so that if the slice was instantiated from the
            // viewport menu we instantiate it at the mouse position
            AZ::Vector2 desiredViewportPosition = instantiatingIter->second.m_viewportPosition;
            if (desiredViewportPosition != AZ::Vector2(-1.0f, -1.0f))
            {
                // This is the same behavior as the old "Add elements from prefab" had.

                AZ::Entity* rootElement = entitiesToInit[0];

                // Transform pivot position to canvas space
                AZ::Vector2 pivotPos;
                UiTransformBus::EventResult(pivotPos, rootElement->GetId(), &UiTransformBus::Events::GetCanvasSpacePivotNoScaleRotate);

                // Transform destination position to canvas space
                AZ::Matrix4x4 transformFromViewport;
                UiTransformBus::Event(rootElement->GetId(), &UiTransformBus::Events::GetTransformFromViewport, transformFromViewport);
                AZ::Vector3 destPos3 = transformFromViewport * AZ::Vector3(desiredViewportPosition.GetX(), desiredViewportPosition.GetY(), 0.0f);
                AZ::Vector2 destPos(destPos3.GetX(), destPos3.GetY());

                AZ::Vector2 offsetDelta = destPos - pivotPos;

                // Adjust offsets on all top level elements
                for (auto entity : entitiesToInit)
                {
                    UiTransform2dInterface::Offsets offsets;
                    UiTransform2dBus::EventResult(offsets, entity->GetId(), &UiTransform2dBus::Events::GetOffsets);
                    UiTransform2dBus::Event(entity->GetId(), &UiTransform2dBus::Events::SetOffsets, offsets + offsetDelta);
                }
            }

            // the entities have already been created but we need to make an undo command that can undo/redo that action
            HierarchyWidget* hierarchyWidget = m_editorWindow->GetHierarchy();

            QTreeWidgetItemRawPtrQList selectedItems = hierarchyWidget->selectedItems();

            // use an undoable command to create the elements from the slice
            CommandHierarchyItemCreateFromData::Push(m_editorWindow->GetActiveStack(),
                hierarchyWidget,
                selectedItems,
                true,
                [ topLevelEntities ]([[maybe_unused]] HierarchyItem* parent, LyShine::EntityArray& listOfNewlyCreatedTopLevelElements)
                {
                    for (AZ::Entity* entity : topLevelEntities)
                    {
                        listOfNewlyCreatedTopLevelElements.push_back(entity);
                    }
                },
                "Instantiate Slice");

            m_instantiatingSlices.erase(instantiatingIter);

            UiEditorEntityContextNotificationBus::Broadcast(
                &UiEditorEntityContextNotificationBus::Events::OnSliceInstantiated, sliceAssetId, sliceAddress, ticket);

            break;
        }
    }
}
    
////////////////////////////////////////////////////////////////////////////////////////////////////
void UiEditorEntityContext::OnSliceInstantiationFailed(const AZ::Data::AssetId& sliceAssetId)
{
    const AzFramework::SliceInstantiationTicket ticket = *AzFramework::SliceInstantiationResultBus::GetCurrentBusId();

    AzFramework::SliceInstantiationResultBus::MultiHandler::BusDisconnect(ticket);

    for (auto instantiatingIter = m_instantiatingSlices.begin(); instantiatingIter != m_instantiatingSlices.end(); ++instantiatingIter)
    {
        if (instantiatingIter->first.GetId() == sliceAssetId)
        {
            AZ::Data::AssetBus::MultiHandler::BusDisconnect(sliceAssetId);
            UiEditorEntityContextNotificationBus::Broadcast(
                &UiEditorEntityContextNotificationBus::Events::OnSliceInstantiationFailed, sliceAssetId, ticket);

            m_instantiatingSlices.erase(instantiatingIter);
            break;
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiEditorEntityContext::OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset)
{
    // We want to stay connected to the asset bus for root uicontext asset to listen to any changes to prefab assets in the ui canvas.
    if (m_rootAssetId.IsValid() && asset.GetId() == m_rootAssetId)
    {
        return;
    }
    AZ::Data::AssetBus::MultiHandler::BusDisconnect(asset.GetId());

    for (auto iter = m_queuedSliceEntityRestores.begin(); iter != m_queuedSliceEntityRestores.end(); )
    {
        SliceEntityRestoreRequest& request = *iter;
        if (asset.GetId() == request.m_asset.GetId())
        {
            AZ::SliceComponent* rootSlice = nullptr;
            AzFramework::SliceEntityOwnershipServiceRequestBus::EventResult(rootSlice, GetContextId(),
                &AzFramework::SliceEntityOwnershipServiceRequestBus::Events::GetRootSlice);
            AZ::SliceComponent::SliceInstanceAddress address = rootSlice->RestoreEntity(request.m_entity, request.m_restoreInfo);

            // Note that we do not add the entity to the context/rootSlice using AddEntity here.
            // This is because it has already been added to the root slice as a prefab instance.
            // Instead we call HandleEntitiesAdded which just adds it to the context
            if (address.IsValid())
            {
                m_entityOwnershipService->HandleEntitiesAdded({request.m_entity});
            }
            else
            {
                AZ_Error("EditorEntityContext", false, "Failed to restore entity \"%s\" [%llu]", 
                    request.m_entity->GetName().c_str(), request.m_entity->GetId());
                delete request.m_entity;
            }

            iter = m_queuedSliceEntityRestores.erase(iter);
        }
        else
        {
            ++iter;
        }
    }

    // Pass on to base Entity Ownership Service.
    m_entityOwnershipService->OnAssetReady(asset);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Root slice (or its dependents) has been reloaded.
void UiEditorEntityContext::OnAssetReloaded(AZ::Data::Asset<AZ::Data::AssetData> asset)
{
    bool isActive = false;
    if (m_editorWindow->GetEntityContext() && m_editorWindow->GetEntityContext()->GetContextId() == GetContextId())
    {
        isActive = true;
    }

    HierarchyWidget* hierarchy = nullptr;
    EntityHelpers::EntityIdList selectedEntities;
    if (isActive)
    {
        hierarchy = m_editorWindow->GetHierarchy();
        const QTreeWidgetItemRawPtrQList& selection = hierarchy->selectedItems();
        selectedEntities = SelectionHelpers::GetSelectedElementIds(hierarchy, selection, false);

        // This ensures there's no "current item".
        hierarchy->SetUniqueSelectionHighlight((QTreeWidgetItem*)nullptr);

        // IMPORTANT: This is necessary to indirectly trigger detach()
        // in the PropertiesWidget.
        hierarchy->SetUserSelection(nullptr);
    }
    
    m_entityOwnershipService->OnAssetReloaded(asset);

    UiCanvasBus::Event(m_editorWindow->GetCanvasForEntityContext(GetContextId()), &UiCanvasBus::Events::ReinitializeElements);

    if (isActive)
    {
        // Ensure selection set is preserved after applying the new level slice.
        // But make sure we don't add any EntityId to selection that no longer exists as that cause a crash later
        selectedEntities.erase(
            std::remove_if(
                selectedEntities.begin(), selectedEntities.end(),
                [](AZ::EntityId entityId)
                {
                    AZ::Entity* entity = nullptr;
                    AZ::ComponentApplicationBus::BroadcastResult(entity, &AZ::ComponentApplicationBus::Events::FindEntity, entityId);
                    return !entity;
                }),
            selectedEntities.end());
    
        // Refresh the Hierarchy pane
        LyShine::EntityArray childElements;
        UiCanvasBus::EventResult(childElements, m_editorWindow->GetCanvas(), &UiCanvasBus::Events::GetChildElements);
        hierarchy->RecreateItems(childElements);

        HierarchyHelpers::SetSelectedItems(hierarchy, &selectedEntities);
    }

    // We want to update the status for any tabs being used to edit slices.
    // If that tab has just done a push, we want to check at this point whether there are any differences between the
    // reloaded asset and the instance.
    m_editorWindow->UpdateChangedStatusOnAssetChange(GetContextId(), asset);
}

//////////////////////////////////////////////////////////////////////////
void UiEditorEntityContext::OnContextEntitiesAdded(const AzFramework::EntityList& entities)
{
    EntityContext::OnContextEntitiesAdded(entities);

    InitializeEntities(entities);
}

//////////////////////////////////////////////////////////////////////////
bool UiEditorEntityContext::ValidateEntitiesAreValidForContext(const AzFramework::EntityList& entities)
{
    // All entities in a slice being instantiated in the UI editor should
    // have the UiElementComponent on them.
    for (AZ::Entity* entity : entities)
    {
        if (!entity->FindComponent(LyShine::UiElementComponentUuid))
        {
            return false;
        }
    }

    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiEditorEntityContext::SetupUiEntity(AZ::Entity* entity)
{
    InitializeEntities({ entity });
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiEditorEntityContext::InitializeEntities(const AzFramework::EntityList& entities)
{
    // UI entities are now automatically activated on creation

    for (AZ::Entity* entity : entities)
    {
        if (entity->GetState() == AZ::Entity::State::Constructed)
        {
            entity->Init();
        }
    }

    // Add required editor components to entities
    for (AZ::Entity* entity : entities)
    {
        for (const auto& componentType : m_requiredEditorComponentTypes)
        {
            if (!entity->FindComponent(componentType))
            {
                entity->CreateComponent(componentType);
            }
        }
    }

    for (AZ::Entity* entity : entities)
    {
        if (entity->GetState() == AZ::Entity::State::Init)
        {
            // Always invalidate the entity dependencies when loading in the editor 
            // (we don't know what code has changed since the last time the editor was run and the services provided/required
            // by entities might have changed)
            entity->InvalidateDependencies();

            // Because we automatically add the EditorOnlyEntityComponent if it doesn't exist, we can encounter a situation
            // where an entity has duplicate EditorOnlyEntityComponents if an old canvas is resaved and an old slice it uses
            // is also resaved. See LY-90580
            // In the main editor this is handled by disabling the duplicate components, but the UI Editor doesn't use that
            // method (the world editor allows the user to manually add incompatible components and then disable and enable
            // them in the entity, the UI Editor still works how the world editor used to - it doesn't allow users to add
            // incompatible components and has no way to disable/enable components in the property pane).
            // So we do automatic recovery in the case where there are duplicate EditorOnlyEntityComponents. We have to do this
            // before activating in order to avoid errors being reported.
            AZ::Entity::ComponentArrayType editorOnlyEntityComponents =
                entity->FindComponents(AzToolsFramework::Components::EditorOnlyEntityComponent::TYPEINFO_Uuid());
            if (editorOnlyEntityComponents.size() > 1)
            {
                // There are duplicate EditorOnlyEntityComponents. If any of them have m_isEditorOnly set to true we will
                // set the one we keep to true. The reasoning is that these duplicates only happen when canvases and slices
                // are being gradually resaved to the new version with EditorOnlyEntityComponents. Since the default is false,
                // if we find one set to true this is more likely to be one that the user specifically set that way.
                bool isEditorOnly = false;
                for (int i = 0; i < editorOnlyEntityComponents.size(); ++i)
                {
                    AzToolsFramework::Components::EditorOnlyEntityComponent* thisComponent =
                        static_cast<AzToolsFramework::Components::EditorOnlyEntityComponent*>(editorOnlyEntityComponents[i]);
                    if (thisComponent->IsEditorOnlyEntity())
                    {
                        isEditorOnly = true;
                        break;
                    }
                }

                // We are going to keep the first one, ensure that its value of m_isEditorOnly is set the right way
                if (isEditorOnly)
                {
                    AzToolsFramework::Components::EditorOnlyEntityComponent* firstComponent =
                        static_cast<AzToolsFramework::Components::EditorOnlyEntityComponent*>(editorOnlyEntityComponents[0]);
                    if (!firstComponent->IsEditorOnlyEntity())
                    {
                        firstComponent->SetIsEditorOnlyEntity(true);
                    }
                }

                // Now remove all the components except the first one. The first one will be the one from the most deeply nested
                // slice. It is best to keep that one, otherwise we end up with local slice overrides deleting the components from
                // the instanced slices which means we could ignore changes from the slice when we should not.
                for (int i = 1; i < editorOnlyEntityComponents.size(); ++i)
                {
                    AZ::Component* duplicateComponent = editorOnlyEntityComponents[i];
                    entity->RemoveComponent(duplicateComponent);
                    delete duplicateComponent;
                }
            }

            // This is a temporary solution to remove incompatible components so that the entity can 
            // activate properly, otherwise all sorts of bad things will happen.
            // 
            // We do have formal way to handle invalid components for Editor entities (see EditorEntityActionComponent::ScrubEntities()).
            // But it requires components being derived from EditorComponentBase. UiCanvas doesn't seem to distinguish between game-time 
            // and editor-time components, so we can't use the existing scrubbing method.
            Internal::RemoveIncompatibleComponents(entity);

            entity->Activate();
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void UiEditorEntityContext::GetTopLevelEntities(const AZStd::unordered_set<AZ::EntityId>& entities, AZStd::unordered_set<AZ::EntityId>& topLevelEntities)
{
    for (auto entityId : entities)
    {
        // if this entities parent is not in the set then it is a top-level
        AZ::Entity* parentElement = nullptr;
        UiElementBus::EventResult(parentElement, entityId, &UiElementBus::Events::GetParent);

        if (!parentElement || entities.count(parentElement->GetId()) == 0)
        {
            topLevelEntities.insert(entityId);
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiEditorEntityContext::QueuedSliceReplacement::IsValid() const
{
    return !m_path.empty();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiEditorEntityContext::QueuedSliceReplacement::Reset()
{
    m_path.clear();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiEditorEntityContext::QueuedSliceReplacement::Finalize(
    const AZ::SliceComponent::SliceInstanceAddress& instanceAddress,
    EditorWindow* editorWindow)
{
    AZ::SliceComponent::EntityAncestorList ancestors;
    AZStd::unordered_map<AZ::EntityId, AZ::EntityId> remapIds;

    const auto& newEntities = instanceAddress.GetInstance()->GetInstantiated()->m_entities;

    // Store mapping between live Ids we're out to remove, and the ones now provided by
    // the slice instance, so we can fix up references on any still-external entities.
    for (const AZ::Entity* newEntity : newEntities)
    {
        ancestors.clear();
        instanceAddress.GetReference()->GetInstanceEntityAncestry(newEntity->GetId(), ancestors, 1);
                
        AZ_Error("EditorEntityContext", !ancestors.empty(), "Failed to locate ancestor for newly created slice entity.");
        if (!ancestors.empty())
        {
            for (const auto& pair : m_selectedToAssetMap)
            {
                const AZ::EntityId& ancestorId = ancestors.front().m_entity->GetId();
                if (pair.second == ancestorId)
                {
                    remapIds[pair.first] = newEntity->GetId();
                    break;
                }
            }
        }
    }

    AZ::SerializeContext* serializeContext = nullptr;
    AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationBus::Events::GetSerializeContext);
        
    // Remap references on any entities left out of the slice, to any entities in the slice instance.
    for (const AZ::EntityId& selectedId : m_entitiesInSelection)
    {
        if (m_selectedToAssetMap.find(selectedId) != m_selectedToAssetMap.end())
        {
            // Entity is included in the slice; no need to patch.
            continue;
        }

        AZ::Entity* entity = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(entity, &AZ::ComponentApplicationBus::Events::FindEntity, selectedId);

        AZ_Error("EditorEntityContext", entity, "Failed to locate live entity during slice replacement.");

        if (entity)
        {
            entity->Deactivate();

            AZ::EntityUtils::ReplaceEntityRefs(entity,
                [&remapIds](const AZ::EntityId& originalId, bool /*isEntityId*/) -> AZ::EntityId
                {
                    auto iter = remapIds.find(originalId);
                    if (iter == remapIds.end())
                    {
                        return originalId;
                    }
                    else
                    {
                        return iter->second;
                    }
                }, serializeContext);

            entity->Activate();
        }
    }

    // Delete the entities from the world that were used to create the slice, since the slice
    // will be instantiated to replace them.

    AZStd::vector<AZ::EntityId> deleteEntityIds;
    deleteEntityIds.reserve(m_selectedToAssetMap.size());
    for (const auto& pair : m_selectedToAssetMap)
    {
        deleteEntityIds.push_back(pair.first);
    }

    // Use an undoable command to delete the entities
    HierarchyWidget* hierarchy = editorWindow->GetHierarchy();

    CommandHierarchyItemDelete::Push(editorWindow->GetActiveStack(),
        hierarchy,
        hierarchy->selectedItems());

    // This ensures there's no "current item".
    hierarchy->SetUniqueSelectionHighlight((QTreeWidgetItem*)nullptr);

    // IMPORTANT: This is necessary to indirectly trigger detach()
    // in the PropertiesWidget.
    hierarchy->SetUserSelection(nullptr);

    Reset();
}
