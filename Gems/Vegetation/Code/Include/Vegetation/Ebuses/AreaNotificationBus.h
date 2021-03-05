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