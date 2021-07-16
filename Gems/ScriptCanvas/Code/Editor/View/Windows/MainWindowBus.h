/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/EBus/EBus.h>

namespace ScriptCanvasEditor
{
    class MainWindowNotifications : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        
        virtual void PreOnActiveSceneChanged() {}
        virtual void OnActiveSceneChanged(const AZ::EntityId& /*sceneId*/) {}
        virtual void PostOnActiveSceneChanged() {}

        virtual void OnSceneLoaded(const AZ::EntityId& /*sceneId*/) {}
        virtual void OnSceneRefreshed(const AZ::EntityId& /*oldSceneId*/, const AZ::EntityId& /*newSceneId*/) {}
        virtual void OnSceneUnloaded(const AZ::EntityId& /*sceneId*/) {}
    };
    
    using MainWindowNotificationBus = AZ::EBus<MainWindowNotifications>;
}
