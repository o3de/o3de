/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

/**
* @file
* The metadata entity context.
* An entity context that manages metadata entities belonging to active slice instances.
*/

#pragma once

#include <AzCore/Slice/SliceComponent.h>
#include <AzCore/Slice/SliceBus.h>
#include <AzCore/Slice/SliceMetadataInfoBus.h>

#include <AzFramework/Entity/EntityContext.h>
#include <AzFramework/Entity/SliceEntityOwnershipService.h>

#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/Slice/SliceMetadataEntityContextBus.h>

namespace AzToolsFramework
{
    /**
     * System component responsible for owning the edit-time slice metadata entity context
     *
     * The slice metadata entity context that creates and manages entities associated with slice instances. The
     * components on these entities collect, pre-compute, and store useful metadata associated with slices. Unlike
     * existing entity contexts, it doesn't not provide an interface for managing its contents and does not make
     * use of the root slice provided by the base entity context.
     *
     * @note This class may be moved to the Framework for use in the game runtime.
     */
    class SliceMetadataEntityContextComponent
        : public AZ::Component
        , public AzFramework::EntityContext
        , protected AZ::SliceInstanceNotificationBus::Handler
        , protected SliceMetadataEntityContextRequestBus::Handler
        , protected ToolsApplicationEvents::Bus::Handler
        , protected AZ::SliceMetadataInfoNotificationBus::MultiHandler
    {
    public:

        AZ_COMPONENT(SliceMetadataEntityContextComponent, "{F53BF27D-9A95-41CC-BA2F-6496F9BC0C6B}");

        /**
        * Default Constructor - Establishes required components for entities belonging to the context
        */
        SliceMetadataEntityContextComponent();

        /**
        * Default Destructor
        */
        ~SliceMetadataEntityContextComponent() override = default;

        /**
        * Component Descriptor - Component Reflection
        * @param context A reflection context
        */
        static void Reflect(AZ::ReflectContext* context);

        //////////////////////////////////////////////////////////////////////////
        // Component overrides
        void Activate() override;
        void Deactivate() override;
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // SliceMetadataEntityContextRequestBus
        AzFramework::EntityContextId GetSliceMetadataEntityContextId() override;
        void ResetContext() override;
        void GetRequiredComponentTypes(AZ::ComponentTypeList& required) override;
        bool IsSliceMetadataEntity(const AZ::EntityId entityId) override;
        AZ::Entity* GetMetadataEntity(const AZ::EntityId entityId) override;
        AZ::EntityId GetMetadataEntityIdFromEditorEntity(const AZ::EntityId editorEntityId) override;
        AZ::EntityId GetMetadataEntityIdFromSliceAddress(const AZ::SliceComponent::SliceInstanceAddress& address) override;
        void AddMetadataEntityToContext(const AZ::SliceComponent::SliceInstanceAddress& /*sliceAddress*/, AZ::Entity& entity) override;
        //////////////////////////////////////////////////////////////////////////


        /**
        * Component Descriptor - Provided Services
        * @param provided An array to fill out with all the services this component provides
        */
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);

        /**
        * Component Descriptor - Incompatible Services
        * @param incompatible An array to fill out with all the services this component is incompatible with
        */
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);

        /**
        * Component Descriptor - Dependent Services
        * @param dependent An array to fill out with all the services this component depends on
        */
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

    protected:
        //////////////////////////////////////////////////////////////////////////
        // SliceInstanceNotificationBus
        void OnMetadataEntityCreated(const AZ::SliceComponent::SliceInstanceAddress& sliceAdress, AZ::Entity& entity) override;
        void OnMetadataEntityDestroyed(AZ::EntityId entityId) override;
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // SliceMetadataInfoNotificationBus
        void OnMetadataDependenciesRemoved() override;
        //////////////////////////////////////////////////////////////////////////

        /**
        * This context requires that entities added to it have certain components.
        * This function will add those component to any entity passed to it, if needed.
        * @param entity Any component entity that's going to belong to this context.
        */
        void AddRequiredComponents(AZ::Entity& entity);

        /**
        * Removes the given entity from the context and cleans up the quick-reference maps
        * If the entity is not part of this context, the function does nothing.
        * @param entityId The ID of the entity to remove from the context.
        * @note This does not destroy the entity or remove it from the component application.
        */
        void RemoveMetadataEntityFromContext(AZ::EntityId entityId);

        // Not Copyable
        SliceMetadataEntityContextComponent(const SliceMetadataEntityContextComponent&) = delete;
        SliceMetadataEntityContextComponent& operator=(const SliceMetadataEntityContextComponent&) = delete;

        AZ::ComponentTypeList m_requiredSliceMetadataComponentTypes; /**< The list of components that entities in this context are required to have. */

        AZStd::unordered_map<AZ::EntityId, AZ::EntityId> m_editorEntityToMetadataEntityMap; /**< A quick lookup map for finding the metadata entity associated with an editor entity. */

        AZStd::unordered_map<AZ::SliceComponent::SliceInstanceAddress, AZ::EntityId> m_sliceAddressToRootMetadataMap; /**< A quick lookup map for finding the metadata entity associated with the given slice instance. */

        AZStd::unordered_map<AZ::EntityId, AZ::Entity*> m_metadataEntityByIdMap; /**< All of the entities owned by this context, keyed by their IDs. */
    };
} // namespace AzToolsFramework
