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

namespace AZ
{
    class Aabb;
}

namespace SurfaceData
{
    /**
    * the EBus is used to send notification information about surfaces
    */
    class SurfaceDataSystemNotifications
        : public AZ::EBusTraits
    {
    public:
        ////////////////////////////////////////////////////////////////////////
        // EBusTraits
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        ////////////////////////////////////////////////////////////////////////

        //! allows multiple threads to call
        using MutexType = AZStd::recursive_mutex;

        virtual void OnSurfaceChanged(const AZ::EntityId& entityId, const AZ::Aabb& oldBounds, const AZ::Aabb& newBounds) = 0;
    };

    typedef AZ::EBus<SurfaceDataSystemNotifications> SurfaceDataSystemNotificationBus;
}
