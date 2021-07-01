/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/EBus/EBus.h>
#include <AzCore/Component/Entity.h>

#include <AzCore/Component/ComponentBus.h>

namespace AzToolsFramework
{
    /**
     * This bus will notify handlers when an entity's "start active" flag has changed
     */
    class EditorEntityRuntimeActivationChangeNotifications 
        : public AZ::EBusTraits
    {
    public:
        virtual ~EditorEntityRuntimeActivationChangeNotifications() = default;
        
        virtual void OnEntityRuntimeActivationChanged(AZ::EntityId entityId, bool activeOnStart) = 0;
    };

    using EditorEntityRuntimeActivationChangeNotificationBus = AZ::EBus<EditorEntityRuntimeActivationChangeNotifications>;
}
