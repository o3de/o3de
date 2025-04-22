/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Slice/SliceMetadataInfoComponent.h>

namespace AZ
{

    SliceMetadataInfoComponent::SliceMetadataInfoComponent(bool persistent)
        : m_persistent(persistent)
    {
    }

    SliceMetadataInfoComponent::~SliceMetadataInfoComponent()
    {
    }

    //////////////////////////////////////////////////////////////////////////
    // AZ::Component
    void SliceMetadataInfoComponent::Activate()
    {
        AZ::SliceMetadataInfoNotificationBus::Bind(m_notificationBus, m_entity->GetId());

        // This bus provides an interface for keeping the component synchronized
        SliceMetadataInfoManipulationBus::Handler::BusConnect(GetEntityId());
        // General information requests for the component
        SliceMetadataInfoRequestBus::Handler::BusConnect(GetEntityId());

        // Application Events used to maintain synchronization
        for (EntityId entityId : m_children) 
        {
            EntityBus::MultiHandler::BusConnect(entityId);
        }

        for (EntityId entityId : m_associatedEntities)
        {
            EntityBus::MultiHandler::BusConnect(entityId);
        }
    }

    void SliceMetadataInfoComponent::Deactivate()
    {
        SliceMetadataInfoManipulationBus::Handler::BusDisconnect();
        SliceMetadataInfoRequestBus::Handler::BusDisconnect();

        for (EntityId entityId : m_children)
        {
            EntityBus::MultiHandler::BusDisconnect(entityId);
        }

        for (EntityId entityId : m_associatedEntities)
        {
            EntityBus::MultiHandler::BusDisconnect(entityId);
        }
        m_notificationBus = nullptr;
    }

    //////////////////////////////////////////////////////////////////////////
    // SliceMetadataInfoManipulationBus
    void SliceMetadataInfoComponent::AddChildMetadataEntity(EntityId childEntityId)
    {
        AZ_Assert(m_children.find(childEntityId) == m_children.end(), "Attempt to establish a child connection that already exists.");
        m_children.insert(childEntityId);
        EntityBus::MultiHandler::BusConnect(childEntityId);
    }

    void SliceMetadataInfoComponent::RemoveChildMetadataEntity(EntityId childEntityId)
    {
        AZ_Assert(m_children.find(childEntityId) != m_children.end(), "Entity Specified is not an existing child metadata entity");
        m_children.erase(childEntityId);
        EntityBus::MultiHandler::BusDisconnect(childEntityId);

        CheckDependencyCount();
    }

    void SliceMetadataInfoComponent::SetParentMetadataEntity(EntityId parentEntityId)
    {
        m_parent = parentEntityId;
    }

    void SliceMetadataInfoComponent::AddAssociatedEntity(EntityId associatedEntityId)
    {
        if (associatedEntityId.IsValid())
        {
            m_associatedEntities.insert(associatedEntityId);
            EntityBus::MultiHandler::BusConnect(associatedEntityId);
        }
    }

    void SliceMetadataInfoComponent::RemoveAssociatedEntity(EntityId associatedEntityId)
    {
        if (!associatedEntityId.IsValid())
        {
            return;
        }

        AZ_Assert(m_associatedEntities.find(associatedEntityId) != m_associatedEntities.end(), "Entity Specified is not an existing associated editor entity");
        m_associatedEntities.erase(associatedEntityId);
        EntityBus::MultiHandler::BusDisconnect(associatedEntityId);

        // During asset processing and level loading, the component may not yet have an
        // associated entity. These cases occur when loading and processing older levels,
        // during cloning of intermediate slice instantiation. In these cases, skipping
        // dependency checking is okay because the components on the final cloned entities
        // are attached to valid entities will be checked properly during instantiation.
        if (GetEntity())
        {
            CheckDependencyCount();
        }
    }

    void SliceMetadataInfoComponent::MarkAsPersistent(bool persistent)
    {
        m_persistent = persistent;
    }

    //////////////////////////////////////////////////////////////////////////
    // EntityBus
    void SliceMetadataInfoComponent::OnEntityDestruction(const AZ::EntityId& entityId)
    {
        if (!entityId.IsValid())
        {
            return;
        }

        // The given ID may be either a dependent editor entity or a child metadata entity. Remove it from
        // either list if it exists.
        m_associatedEntities.erase(entityId);
        m_children.erase(entityId);
        EntityBus::MultiHandler::BusDisconnect(entityId);
        CheckDependencyCount();
    }

    //////////////////////////////////////////////////////////////////////////
    // SliceMetadataInfoRequestBus
    bool SliceMetadataInfoComponent::IsAssociated(EntityId entityId)
    {
        return m_associatedEntities.find(entityId) != m_associatedEntities.end();
    }

    void SliceMetadataInfoComponent::GetAssociatedEntities(AZStd::set<EntityId>& associatedEntityIds)
    {
        associatedEntityIds.insert(m_associatedEntities.begin(), m_associatedEntities.end());
    }

    EntityId SliceMetadataInfoComponent::GetParentId()
    {
        return m_parent;
    }

    void SliceMetadataInfoComponent::GetChildIDs(AZStd::unordered_set<EntityId>& childEntityIds)
    {
        childEntityIds.insert(m_children.begin(), m_children.end());
    }

    size_t SliceMetadataInfoComponent::GetAssociationCount()
    {
        return m_children.size() + m_associatedEntities.size();
    }

    //////////////////////////////////////////////////////////////////////////

    void SliceMetadataInfoComponent::Reflect(AZ::ReflectContext* context)
    {
        SerializeContext* serializeContext = azrtti_cast<SerializeContext*>(context);

        if (serializeContext)
        {
            // m_associatedEntities is an unordered_set in v1. Therefore, we need to explicitly reflect unordered_set for it to be
            // registered with the serializeContext rather than assume that the serializeContext will have it reflected by some
            // other class
            serializeContext->RegisterGenericType<AZStd::unordered_set<AZ::EntityId>>();
            serializeContext->Class<SliceMetadataInfoComponent, Component>()->
                Version(2, &VersionConverter)->
                Field("AssociatedIds", &SliceMetadataInfoComponent::m_associatedEntities)->
                Field("ParentId", &SliceMetadataInfoComponent::m_parent)->
                Field("ChildrenIds", &SliceMetadataInfoComponent::m_children)->
                Field("PersistenceFlag", &SliceMetadataInfoComponent::m_persistent);

            AZ::EditContext* editContext = serializeContext->GetEditContext();
            if (editContext)
            {
                editContext->Class<SliceMetadataInfoComponent>(
                    "Slice Metadata Info", "The Slice Metadata Info Component maintains a list of all of the entities that are associated with a slice metadata entity.")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::Hide)
                    ->Attribute(AZ::Edit::Attributes::RuntimeExportCallback, &SliceMetadataInfoComponent::ExportComponent)
                    ;
            }
        }
    }

    bool SliceMetadataInfoComponent::VersionConverter(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement)
    {
        // Conversion from version 1:
        // - Change AssociatedIds(m_associatedEntities) to be a set of entityIds instead of an unordered_set
        if (classElement.GetVersion() <= 1)
        {
            int associatedIdsIndex = classElement.FindElement(AZ_CRC_CE("AssociatedIds"));

            if (associatedIdsIndex < 0)
            {
                return false;
            }

            AZ::SerializeContext::DataElementNode& associatedIds = classElement.GetSubElement(associatedIdsIndex);
            AZStd::unordered_set<AZ::EntityId> associatedIdsUnorderedSet;
            AZStd::set<AZ::EntityId> associatedIdsOrderedSet;
            if (!associatedIds.GetData<AZStd::unordered_set<AZ::EntityId>>(associatedIdsUnorderedSet))
            {
                return false;
            }
            associatedIdsOrderedSet.insert(associatedIdsUnorderedSet.begin(), associatedIdsUnorderedSet.end());
            associatedIds.Convert<AZStd::set<AZ::EntityId>>(context);
            if (!associatedIds.SetData<AZStd::set<AZ::EntityId>>(context, associatedIdsOrderedSet))
            {
                return false;
            }
        }

        return true;
    }

    AZ::ExportedComponent SliceMetadataInfoComponent::ExportComponent(AZ::Component* /*thisComponent*/, const AZ::PlatformTagSet& /*platformTags*/)
    {
        // SliceMetadataInfoComponent should only exist in the Editor, so we return a null component on exports.
        return AZ::ExportedComponent();
    }


    void SliceMetadataInfoComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("SliceMetadataInfoService"));
    }

    void SliceMetadataInfoComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("SliceMetadataInfoService"));
    }

    void SliceMetadataInfoComponent::CheckDependencyCount()
    {
        // When the dependency count reaches zero, non-persistent metadata entities are no longer useful and need to be removed from the metadata entity context.
        // While removing all entities belonging to any non-nested slice will trigger the destruction of that slice, the deletion of its metadata entity,
        // and the removal of the metadata entity from the metadata entity context, nested slices are only properly destroyed when their non-nested ancestor
        // is destroyed. To ensure that the context knows the metadata entity is no longer relevant, we dispatch an OnMetadataDependenciesRemoved event.
        bool isActive = GetEntity()->GetState() == AZ::Entity::State::Active;
        bool hasNoDependencies = m_associatedEntities.empty() && m_children.empty();
        if (isActive && !m_persistent && hasNoDependencies)
        {
            SliceMetadataInfoNotificationBus::Event(m_notificationBus, &SliceMetadataInfoNotificationBus::Events::OnMetadataDependenciesRemoved);
        }
    }

} // Namespace AZ
