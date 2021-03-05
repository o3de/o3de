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

#ifndef ASSETPROCESSOR_APPLICATIONMANAGERAPI_H
#define ASSETPROCESSOR_APPLICATIONMANAGERAPI_H

#include <AzCore/EBus/EBus.h>
#include <AzCore/std/parallel/mutex.h>

#pragma once

namespace AssetProcessor
{
    //! This class contains notifications broadcast by the Application Manager (Which manages the lifecycle of the application).
    //! note, these events will be dispatched sequentially and safely from one specific thread (main UI thread), but may be
    //! talking to an object on a different "unsafe" thread and thus appropriate thread safety should be observed by the listener.
    class ApplicationManagerNotifications
        : public AZ::EBusTraits
    {
    public:
        //////////////////////////////////////////////////////////////////////////
        /// Public API:

        //! Invoked by the application when its time to shut down
        //! Jobs must quit as soon as they can, with 'failed' status.
        virtual void ApplicationShutdownRequested() = 0;
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        /// Bus Configuration
        ApplicationManagerNotifications() = default;
        ~ApplicationManagerNotifications() = default;
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;  // any number of connected listeners
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single; // no addressing used.
        typedef AZStd::recursive_mutex MutexType; // protect bus addition and removal since listeners can disconnect
        using Bus = AZ::EBus<ApplicationManagerNotifications>;
        //////////////////////////////////////////////////////////////////////////
    };
} // namespace AssetProcesor

#endif // ASSETPROCESSOR_APPLICATIONMANAGERAPI_H
