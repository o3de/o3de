/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/ComponentBus.h>

namespace Vegetation
{
    /**
    * the EBus is used to notify about area changes
    */      
    class AreaNotifications : public AZ::ComponentBus
    {
    public: 
        //! allows multiple threads to call
        using MutexType = AZStd::recursive_mutex;

        //All AreaRequestBus and AreaNotificationBus requests normally occur on a dedicated vegetation thread.
        //The one exception for AreaRequestBus was previously bus connection and disconnection on entity activation and deactivation.
        //Entity activation and deactivation must occur on the main thread.
        //Maintaining a persistent connection and interacting with AreaRequestBus across multiple threads while entities are created and destroyed will
        //cause all threads to hitch while waiting for locks.

        //OnAreaConnect/OnAreaDisconnect are meant to support connecting to AreaRequestBus only as needed.
        //Connecting only when needed on the vegetation thread prevents entity activation/deactivation from being blocked on the main thread.

        //Notify an area or observer to connect to required buses before work begins
        virtual void OnAreaConnect() {}

        //Notify an area or observer to disconnect from required buses when work is complete
        virtual void OnAreaDisconnect() {}

        //Notify an area or observer that an area has been registered with the vegetation area system.
        virtual void OnAreaRegistered() {}

        //Notify an area or observer that an area has been unregistered from the vegetation area system.
        virtual void OnAreaUnregistered() {}

        //Notify an area or observer that an area has been refreshed by the vegetation area system.
        virtual void OnAreaRefreshed() {}
    };

    typedef AZ::EBus<AreaNotifications> AreaNotificationBus;
}
