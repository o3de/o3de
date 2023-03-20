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

#include <AzToolsFramework/Entity/EditorEntityContextBus.h>
#include <AzToolsFramework/Entity/ReadOnly/ReadOnlyEntityInterface.h>

namespace AzToolsFramework
{
    //! System Component to track read-only entity registration.
    //! An entity registered as ReadOnly cannot be altered in the Editor.
    class ReadOnlyEntitySystemComponent final
        : public AZ::Component
        , private ReadOnlyEntityPublicInterface
        , private ReadOnlyEntityQueryInterface
        , private EditorEntityContextNotificationBus::Handler
    {
    public:
        AZ_COMPONENT(ReadOnlyEntitySystemComponent, "{B32EB03F-D88F-4B3A-9C16-071AF04DA646}");

        ReadOnlyEntitySystemComponent() = default;
        virtual ~ReadOnlyEntitySystemComponent() = default;

        // AZ::Component overrides ...
        void Activate() override;
        void Deactivate() override;

        static void Reflect(AZ::ReflectContext* context);
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);

        // ReadOnlyEntityPublicInterface overrides ...
        bool IsReadOnly(const AZ::EntityId& entityId) override;

        // ReadOnlyEntityQueryInterface overrides ...
        void RefreshReadOnlyState(const EntityIdList& entityIds) override;
        void RefreshReadOnlyStateForAllEntities() override;

        // EditorEntityContextNotificationBus overrides ...
        void OnContextReset() override;

    private:
        void QueryReadOnlyStateForEntity(const AZ::EntityId& entityId);

        AZStd::unordered_map<AZ::EntityId, bool> m_readOnlystates;
    };

} // namespace AzToolsFramework
