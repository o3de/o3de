/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include <AzCore/Component/Entity.h>
#include <AzCore/Slice/SliceMetadataInfoBus.h>

#include <AzToolsFramework/Entity/EditorEntitySortComponent.h>
#include <AzToolsFramework/ToolsComponents/EditorDisabledCompositionComponent.h>
#include <AzToolsFramework/ToolsComponents/EditorInspectorComponent.h>
#include <AzToolsFramework/ToolsComponents/EditorPendingCompositionComponent.h>
#include <AzToolsFramework/Undo/UndoCacheInterface.h>

#include "SliceMetadataEntityContextComponent.h"


namespace AzToolsFramework
{
    /*!  Default Constructor
    */
    SliceMetadataEntityContextComponent::SliceMetadataEntityContextComponent()
        : AzFramework::EntityContext(AzFramework::EntityContextId::CreateRandom())
        , m_requiredSliceMetadataComponentTypes
        // These are the components that will be force added to every slice metadata entity
        ({
            azrtti_typeid<Components::EditorEntitySortComponent>(),
            azrtti_typeid<AzToolsFramework::Components::EditorPendingCompositionComponent>(),
            azrtti_typeid<AzToolsFramework::Components::EditorDisabledCompositionComponent>(),
            azrtti_typeid<AzToolsFramework::Components::EditorInspectorComponent>()
        })
    {
    }

    /*!  Provides basic reflection properties for the serialization and edit reflection contexts
        \param  context     A context provided by the reflection system
    */
    void SliceMetadataEntityContextComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<SliceMetadataEntityContextComponent, AZ::Component>();

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<SliceMetadataEntityContextComponent>(
                    "Slice Metadata Entity Context", "System component responsible for owning the slice metadata entity context")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Category, "Editor")
                    ;
            }
        }
    }

    /*!  Called when the context is activated.
         Creates the root slice and establishes connections to necessary eBuses.
    */
    void SliceMetadataEntityContextComponent::Activate()
    {
        m_entityOwnershipService = AZStd::make_unique<AzFramework::SliceEntityOwnershipService>(GetContextId(), GetSerializeContext());
        InitContext();

        SliceMetadataEntityContextRequestBus::Handler::BusConnect();
        AZ::SliceInstanceNotificationBus::Handler::BusConnect();
    }

    /*!  Called when the context is deactivated.
         Removes all entities, resets the root slice, and disconnects from the root asset.
    */
    void SliceMetadataEntityContextComponent::Deactivate()
    {
        SliceMetadataEntityContextRequestBus::Handler::BusDisconnect();
        AZ::SliceInstanceNotificationBus::Handler::BusDisconnect();

        DestroyContext();
    }

    /*!  Retrieves the ID of this Slice Metadata Entity Context.
        /return The ID of this context.
    */
    AzFramework::EntityContextId SliceMetadataEntityContextComponent::GetSliceMetadataEntityContextId()
    {
        return GetContextId();
    }

    /*!  Called to reset the current context.
         Destroys all entities owned by the context and replaces the root slice with a new, empty one.
    */
    void SliceMetadataEntityContextComponent::ResetContext()
    {
        // Because metadata entities are owned by the slices they're associated with, we can just
        // clear our association maps.
        m_editorEntityToMetadataEntityMap.clear();
        m_metadataEntityByIdMap.clear();
        m_sliceAddressToRootMetadataMap.clear();

        SliceMetadataEntityContextNotificationBus::Broadcast(&SliceMetadataEntityContextNotificationBus::Events::OnContextReset);
    }

    /*!  Called to reset the current context.
         Destroys all entities owned by the context and replaces the root slice with a new, empty one.
        \return True for success
    */
    bool SliceMetadataEntityContextComponent::IsSliceMetadataEntity(AZ::EntityId entityId)
    {
        return nullptr != GetMetadataEntity(entityId);
    }

    AZ::Entity* SliceMetadataEntityContextComponent::GetMetadataEntity(const AZ::EntityId entityId)
    {
        auto metadataEntityByIdIter = m_metadataEntityByIdMap.find(entityId);
        if (metadataEntityByIdIter != m_metadataEntityByIdMap.end())
        {
            return metadataEntityByIdIter->second;
        }

        return nullptr;
    }

    AZ::EntityId SliceMetadataEntityContextComponent::GetMetadataEntityIdFromEditorEntity(const AZ::EntityId editorEntityId)
    {
        auto associationMapIter = m_editorEntityToMetadataEntityMap.find(editorEntityId);
        if (associationMapIter != m_editorEntityToMetadataEntityMap.end())
        {
            return associationMapIter->second;
        }

        return AZ::EntityId();
    }

    /*!  Get the ID of the slice metadata entity associated with the given slice instance address.
        /param address      A Slice Instance Address 
        /return A Component Type List of all the components that need to be added to entities in the Slice Metadata Entity Context
    */
    AZ::EntityId SliceMetadataEntityContextComponent::GetMetadataEntityIdFromSliceAddress(const AZ::SliceComponent::SliceInstanceAddress& sliceAddress)
    {
        auto sliceAddressToRootMetadataIter = m_sliceAddressToRootMetadataMap.find(sliceAddress);
        AZ_Assert(sliceAddressToRootMetadataIter != m_sliceAddressToRootMetadataMap.end(), "Metadata Entity For Slice Address Not Found");
        if (sliceAddressToRootMetadataIter != m_sliceAddressToRootMetadataMap.end())
        {
            return sliceAddressToRootMetadataIter->second;
        }

        return AZ::EntityId();
    }

    /// Adds a slice metadata entity to the context
    void SliceMetadataEntityContextComponent::AddMetadataEntityToContext(const AZ::SliceComponent::SliceInstanceAddress& sliceAddress, AZ::Entity& metadataEntity)
    {
        // If this fires for legit reasons, we may need to deactivate any active entities first. Right now it is assumed they are not already active.
        AZ_Assert(metadataEntity.GetState() < AZ::Entity::State::Active, "Unable to add required components to already active entities");
        AddRequiredComponents(metadataEntity);

        if (metadataEntity.GetState() == AZ::Entity::State::Constructed)
        {
            metadataEntity.Init();
        }

        if (metadataEntity.GetState() == AZ::Entity::State::Init)
        {
            metadataEntity.Activate();
        }

        AZ_Assert(metadataEntity.GetState() == AZ::Entity::State::Active, "Metadata Entity Failed To Activate");

        // All metadata entities created should have a metadata association component
        AZStd::set<AZ::EntityId> associatedEntities;
        AZ::SliceMetadataInfoRequestBus::Event(metadataEntity.GetId(), &AZ::SliceMetadataInfoRequestBus::Events::GetAssociatedEntities, associatedEntities);
        for (const auto& editorEntityId : associatedEntities)
        {
            m_editorEntityToMetadataEntityMap[editorEntityId] = metadataEntity.GetId();
        }

        // If the metadata entity has no parent, we can assume it's the root entity in the slice hierarchy belonging to the slice address provided
        AZ::EntityId parentMetadataId;
        AZ::SliceMetadataInfoRequestBus::EventResult(parentMetadataId, metadataEntity.GetId(), &AZ::SliceMetadataInfoRequestBus::Events::GetParentId);
        if (!parentMetadataId.IsValid())
        {
            m_sliceAddressToRootMetadataMap[sliceAddress] = metadataEntity.GetId();
        }
        m_metadataEntityByIdMap[metadataEntity.GetId()] = &metadataEntity;

        AzFramework::EntityIdContextQueryBus::MultiHandler::BusConnect(metadataEntity.GetId());
        AZ::SliceMetadataInfoNotificationBus::MultiHandler::BusConnect(metadataEntity.GetId());

        SliceMetadataEntityContextNotificationBus::Broadcast(&SliceMetadataEntityContextNotifications::OnMetadataEntityAdded, metadataEntity.GetId());

        // Register the metadata entity with the pre-emptive undo cache (if exists) so it has an initial state
        auto undoCacheInterface = AZ::Interface<UndoSystem::UndoCacheInterface>::Get();
        if (undoCacheInterface)
        {
            undoCacheInterface->UpdateCache(metadataEntity.GetId());
        }
    }


    void SliceMetadataEntityContextComponent::OnMetadataEntityCreated(const AZ::SliceComponent::SliceInstanceAddress& sliceAddress, AZ::Entity& entity)
    {
        AddMetadataEntityToContext(sliceAddress, entity);
    }

    void SliceMetadataEntityContextComponent::OnMetadataEntityDestroyed(const AZ::EntityId entityId)
    {
        RemoveMetadataEntityFromContext(entityId);
    }

    void SliceMetadataEntityContextComponent::OnMetadataDependenciesRemoved()
    {
        RemoveMetadataEntityFromContext(*AZ::SliceMetadataInfoNotificationBus::GetCurrentBusId());
    }

    /*!  Get a list of required component types.
         Entities added to this context require a number of components by default. This returns a list of those required components.
        /return A Component Type List of all the components that need to be added to entities in the Slice Metadata Entity Context
    */
    void SliceMetadataEntityContextComponent::GetRequiredComponentTypes(AZ::ComponentTypeList& required)
    {
        required.insert(required.end(), m_requiredSliceMetadataComponentTypes.begin(), m_requiredSliceMetadataComponentTypes.end());
    }

    /*!  Fills out a list of the component services provided by this context
    */
    void SliceMetadataEntityContextComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("SliceMetadataEntityContextService"));
    }

    /*!  Fills out a list of the component services incompatible with this context
    */
    void SliceMetadataEntityContextComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("SliceMetadataEntityContextService"));
    }

    /*!  Fills out a list of the component services required for this context to function
    */
    void SliceMetadataEntityContextComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        dependent.push_back(AZ_CRC_CE("AssetDatabaseService"));
    }

    /*!  Adds all the required components for this context to the given entity if they don't already exist.
        \param  entity    The entity to add components to
    */
    void SliceMetadataEntityContextComponent::AddRequiredComponents(AZ::Entity& metadataEntity)
    {
        for (const auto& componentType : m_requiredSliceMetadataComponentTypes)
        {
            if (!metadataEntity.FindComponent(componentType))
            {
                metadataEntity.CreateComponent(componentType);
            }
        }
    }

    void SliceMetadataEntityContextComponent::RemoveMetadataEntityFromContext(const AZ::EntityId entityId)
    {
        auto metadataEntityByIdIter = m_metadataEntityByIdMap.find(entityId);
        if (metadataEntityByIdIter == m_metadataEntityByIdMap.end())
        {
            return;
        }

        AzFramework::EntityIdContextQueryBus::MultiHandler::BusDisconnect(entityId);

        // Clean up our quick-lookup maps
        // Find our entry in the slice address map and remove it.
        for (auto sliceAddressToRootMetadataIter = m_sliceAddressToRootMetadataMap.begin(); sliceAddressToRootMetadataIter != m_sliceAddressToRootMetadataMap.end();)
        {
            if (sliceAddressToRootMetadataIter->second == entityId)
            {
                m_sliceAddressToRootMetadataMap.erase(sliceAddressToRootMetadataIter);
                break;
            }
            ++sliceAddressToRootMetadataIter;
        }

        // Remove all association entries for the given metadata entity ID.
        for (auto editorEntityToMetadataEntityIter = m_editorEntityToMetadataEntityMap.begin(); editorEntityToMetadataEntityIter != m_editorEntityToMetadataEntityMap.end();)
        {
            if (editorEntityToMetadataEntityIter->second == entityId)
            {
                editorEntityToMetadataEntityIter = m_editorEntityToMetadataEntityMap.erase(editorEntityToMetadataEntityIter);
                continue;
            }
            ++editorEntityToMetadataEntityIter;
        }

        SliceMetadataEntityContextNotificationBus::Broadcast(&SliceMetadataEntityContextNotifications::OnMetadataEntityRemoved, entityId);
        m_metadataEntityByIdMap.erase(metadataEntityByIdIter);
    }

} // namespace AzToolsFramework
