/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/EBus/EBus.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/std/containers/vector.h>

namespace AzToolsFramework
{
    using EntityOrderArray = AZStd::vector<AZ::EntityId>;

    class EditorEntitySortRequests : public AZ::EBusTraits
    {
    public:
        virtual ~EditorEntitySortRequests() = default;

        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;

        using BusIdType = AZ::EntityId;
        //////////////////////////////////////////////////////////////////////////

        virtual EntityOrderArray GetChildEntityOrderArray() = 0;
        virtual bool SetChildEntityOrderArray(const EntityOrderArray& entityOrderArray) = 0;
        virtual bool AddChildEntity(const AZ::EntityId& entityId, bool addToBack) = 0;
        virtual bool AddChildEntityAtPosition(const AZ::EntityId& entityId, const AZ::EntityId& beforeEntity) = 0;
        virtual bool RemoveChildEntity(const AZ::EntityId& entityId) = 0;
        virtual AZ::u64 GetChildEntityIndex(const AZ::EntityId& entityId) = 0;
        virtual bool CanMoveChildEntityUp(const AZ::EntityId& entityId) = 0;
        virtual void MoveChildEntityUp(const AZ::EntityId& entityId) = 0;
        virtual bool CanMoveChildEntityDown(const AZ::EntityId& entityId) = 0;
        virtual void MoveChildEntityDown(const AZ::EntityId& entityId) = 0;
    };
    using EditorEntitySortRequestBus = AZ::EBus<EditorEntitySortRequests>;

    class EditorEntitySortNotifications : public AZ::EBusTraits
    {
    public:
        virtual ~EditorEntitySortNotifications() = default;

        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = AZ::EntityId;
        //////////////////////////////////////////////////////////////////////////

        virtual void ChildEntityOrderArrayUpdated() = 0;
    };
    using EditorEntitySortNotificationBus = AZ::EBus<EditorEntitySortNotifications>;

} // namespace AzToolsFramework
