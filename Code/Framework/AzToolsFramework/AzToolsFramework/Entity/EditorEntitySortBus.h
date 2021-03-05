/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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
