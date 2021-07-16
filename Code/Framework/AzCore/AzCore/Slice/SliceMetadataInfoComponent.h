/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

/**
* @file
* Declaration of the SliceMetadataInfoComponent class and supporting types.
*/

#pragma once

#include <AzCore/Component/Component.h>
#include <AzCore/Component/EntityBus.h>
#include <AzCore/Slice/SliceMetadataInfoBus.h>
#include <AzCore/Component/ComponentExport.h>

namespace AZ
{
    /**
    * Slice Metadata Entity Information Component
    * This component maintains a list of all of the entities that are associated with a slice metadata
    * entity. An associated entity is defined as one that is new to the slice at the nested level
    * that the metadata entity is associated with, that is, it wasn't inherited from a base slice. All
    * entities in every slice are associated with exactly one metadata entity. In addition to associated
    * editor entities, this component also tracks any metadata entities belonging to any root slices
    * referenced by the associated slice as well as, if applicable, the metadata entity belonging to the
    * slice holding the reference to the associated slice.
    */
    class SliceMetadataInfoComponent
        : public AZ::Component
        , public SliceMetadataInfoRequestBus::Handler
        , public AZ::EntityBus::MultiHandler
        , public SliceMetadataInfoManipulationBus::Handler
    {
    public:
        using EntityIdSet = AZStd::unordered_set<AZ::EntityId>;

        AZ_COMPONENT(SliceMetadataInfoComponent, "{25EE4D75-8A17-4449-81F4-E561005BAABD}");

        /**
        * Metadata Info Component Constructor
        * @param persistent True if this info component should be persistent. A persistent component
        *                   will not emit a notification when it has no dependencies. Only the actual
        *                   destruction of the metadata entity will remove it from the context.
        */
        SliceMetadataInfoComponent(bool persistent = false);

        /**
        * Destructor (Default)
        */
        ~SliceMetadataInfoComponent() override; // = default;

        /**
        * Component Descriptor - Component Reflection
        * @param context A reflection context
        */
        static void Reflect(AZ::ReflectContext* context);

        //////////////////////////////////////////////////////////////////////////
        // SliceMetadataInfoManipulationBus
        void AddChildMetadataEntity(EntityId childEntityId) override;
        void RemoveChildMetadataEntity(EntityId childEntityId) override;
        void SetParentMetadataEntity(EntityId parentEntityId) override;
        void AddAssociatedEntity(EntityId associatedEntityId) override;
        void RemoveAssociatedEntity(EntityId associatedEntityId) override;
        void MarkAsPersistent(bool persistent) override;
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // AZ::Component
        void Activate() override;
        void Deactivate() override;
        //////////////////////////////////////////////////////////////////////////
    protected:
        // The slice component needs to modify new and existing metadata info components
        // before they can register their handlers.
        friend SliceComponent;

        //////////////////////////////////////////////////////////////////////////
        // EntityBus
        void OnEntityDestruction(const AZ::EntityId& entityId) override;
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // SliceMetadataInfoRequestBus
        bool IsAssociated(EntityId entityId) override;
        void GetAssociatedEntities(AZStd::set<EntityId>& associatedEntityIds) override;
        AZ::EntityId GetParentId() override;
        void GetChildIDs(AZStd::unordered_set<EntityId>& childEntityIds) override;
        size_t GetAssociationCount() override;
        //////////////////////////////////////////////////////////////////////////

        /**
        * Component Descriptor - Provided Services
        * @param provided An array to fill out with all the services this component is dependent on
        */
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);

        /**
        * Component Descriptor - Incompatible Services
        * @param incompatible An array to fill out with all the services this component is incompatible with
        */
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);

        /**
        * Gets the number of objects that are attached to this metadata entity. This includes both associated
        * editor entities and child metadata entities belonging to referenced slices.
        * If the dependency count is zero, this will send an OnMetadataDependenciesRemoved notification.
        * Because this could result in the destruction of the metadata entity, be careful when calling this.
        * This function does nothing if the component is marked as persistent.
        * Note: This function can trigger the removal of the owning entity from the metadata context but can
        *       not trigger the deletion of the entity itself.
        */
        void CheckDependencyCount();

        /**
        * Handles the runtime export of the SliceMetadataInfoComponent.  
        * The SliceMetadataInfo component shouldn't exist at runtime, so the export function returns a null component.
        * @param thisComponent The source component that is being exported.
        * @param platformTags The set of platforms to export for.
        * @return The exported component, along with a flag on whether or not to delete it at the end of the export process.
        */
        AZ::ExportedComponent ExportComponent(AZ::Component* thisComponent, const AZ::PlatformTagSet& platformTags);


        SliceMetadataInfoNotificationBus::BusPtr m_notificationBus; /**< A bus pointer for emitting addressed notifications */

        //! Entity Association Data
        AZStd::set<AZ::EntityId> m_associatedEntities; /**< A list of editor entities associated with this slice metadata
                                            *  An associated entity is one that is new to this slice as opposed to an entity cloned
                                            *  from a referenced slice. Every editor entity should always be associated with exactly
                                            *  one metadata entity. Entities that are not part of slices are associated with the
                                            *  metadata component belonging to the root slice. */

        // Slice Hierarchy Data
        AZ::EntityId m_parent; /**< If the associated slice instance belongs to another slice, this is the ID of the metadata entity
                                 *  associated with that parent slice instance. Note: The root entity will never show up as a parent.
                                 *  Slices that are not instances belonging to a non-root slice have no parent. */

        EntityIdSet m_children; /**< The IDs of each metadata entity associated with an slice instance belonging to the associated slice. */

        bool m_persistent; /**< The persistence flag indicates that the object is valid even without dependencies and will not emit the
                             *  OnMetadataDependenciesRemoved notification when it's dependency count goes to 0.
                             *  For example: The Root Slice. */
    private:
        static bool VersionConverter(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement);
    };
} // namespace AZ
