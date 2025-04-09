/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/base.h>
#include <AzCore/Component/ComponentBus.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/EBus/EBus.h>

#include <AzToolsFramework/Entity/EntityTypes.h>

namespace AzToolsFramework
{
    class EntityCompositionNotifications
        : public AZ::EBusTraits
    {
    public:
        /*!
        * Notification that the specified entities are about to have their composition changed due to user interaction in the editor
        *
        * \param entityIds Entities about to be changed
        */
        virtual void OnEntityCompositionChanging(const AzToolsFramework::EntityIdList& /*entityIds*/) {};

        /*!
        * Notification that the specified entities had their composition changed due to user interaction in the editor
        *
        * \param entityIds Entities changed
        */
        virtual void OnEntityCompositionChanged(const AzToolsFramework::EntityIdList& /*entityIds*/) {};

        /*!@{
         * Discrete composition events for adding, removing, enabling and disabling components
        */
        virtual void OnEntityComponentAdded(const AZ::EntityId& /*entityId*/, const AZ::ComponentId& /*componentId*/) {};
        virtual void OnEntityComponentRemoved(const AZ::EntityId& /*entityId*/, const AZ::ComponentId& /*componentId*/) {};
        virtual void OnEntityComponentEnabled(const AZ::EntityId& /*entityId*/, const AZ::ComponentId& /*componentId*/) {};
        virtual void OnEntityComponentDisabled(const AZ::EntityId& /*entityId*/, const AZ::ComponentId& /*componentId*/) {};
        //!@}
    };

    using EntityCompositionNotificationBus = AZ::EBus<EntityCompositionNotifications>;

}
