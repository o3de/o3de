/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>
#include <AzCore/Memory/SystemAllocator.h>

#include <AzToolsFramework/ContainerEntity/ContainerEntityInterface.h>
#include <AzToolsFramework/Entity/EditorEntityContextBus.h>

namespace AzToolsFramework
{
    //! System Component to track Container Entity registration and open state.
    //! An entity registered as Container is just like a regular entity when open. If its state is changed 
    //! to closed, all descendants of the entity will be treated as part of the entity itself. Selecting any
    //! descendant will result in the container being selected, and descendants will be hidden until the
    //! container is opened.
    class ContainerEntitySystemComponent final
        : public AZ::Component
        , private ContainerEntityInterface
        , private EditorEntityContextNotificationBus::Handler
    {
    public:
        AZ_COMPONENT(ContainerEntitySystemComponent, "{74349759-B36B-44A6-B89F-F45D7111DD11}");

        ContainerEntitySystemComponent() = default;
        virtual ~ContainerEntitySystemComponent() = default;

        // AZ::Component overrides ...
        void Activate() override;
        void Deactivate() override;

        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);

        // ContainerEntityInterface overrides ...
        ContainerEntityOperationResult RegisterEntityAsContainer(AZ::EntityId entityId) override;
        ContainerEntityOperationResult UnregisterEntityAsContainer(AZ::EntityId entityId) override;
        bool IsContainer(AZ::EntityId entityId) const override;
        ContainerEntityOperationResult SetContainerOpen(AZ::EntityId entityId, bool open) override;
        bool IsContainerOpen(AZ::EntityId entityId) const override;
        AZ::EntityId FindHighestSelectableEntity(AZ::EntityId entityId) const override;
        ContainerEntityOperationResult Clear(AzFramework::EntityContextId entityContextId) override;
        bool IsUnderClosedContainerEntity(AZ::EntityId entityId) const override;

        // EditorEntityContextNotificationBus overrides ...
        void OnEntityStreamLoadSuccess() override;

    private:
        AZStd::unordered_set<AZ::EntityId> m_containers;      //!< All entities in this set are containers.
        AZStd::unordered_set<AZ::EntityId> m_openContainers;  //!< All entities in this set are open containers.
    };

} // namespace AzToolsFramework
