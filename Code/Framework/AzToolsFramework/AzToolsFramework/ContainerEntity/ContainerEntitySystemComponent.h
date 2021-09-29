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

namespace AzToolsFramework
{
    //! System Component to handle Container Entity
    class ContainerEntitySystemComponent final
        : public AZ::Component
        , private ContainerEntityInterface
    {
    public:
        AZ_COMPONENT(ContainerEntitySystemComponent, "{74349759-B36B-44A6-B89F-F45D7111DD11}");

        ContainerEntitySystemComponent() = default;
        virtual ~ContainerEntitySystemComponent() = default;

        // AZ::Component overrides ...
        void Init() override;
        void Activate() override;
        void Deactivate() override;

        static void Reflect(AZ::ReflectContext* context);
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);

        // ContainerEntityInterface overrides ...
        ContainerEntityOperationResult RegisterEntityAsContainer(AZ::EntityId entityId) override;
        ContainerEntityOperationResult UnregisterEntityAsContainer(AZ::EntityId entityId) override;
        bool IsContainer(AZ::EntityId entityId) const override;
        ContainerEntityOperationResult SetContainerClosedState(AZ::EntityId entityId, bool closed) override;
        bool IsContainerClosed(AZ::EntityId entityId) const override;

    private:
        AZStd::unordered_set<AZ::EntityId> m_containerSet;          //!< All entities in this set are containers.
        AZStd::unordered_set<AZ::EntityId> m_closedContainerSet;    //!< All entities in this set are closed containers.
    };

} // namespace AzToolsFramework
