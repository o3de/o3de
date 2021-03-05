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

#include <AzCore/base.h>
#include <AzCore/EBus/EBus.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Component/ComponentBus.h>
#include <AzCore/Component/Entity.h>

namespace AzToolsFramework
{
    using EntityIdList = AZStd::vector<AZ::EntityId>;

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