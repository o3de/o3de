/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Component/EntityId.h>
#include <AzCore/EBus/EBus.h>
#include <AzCore/Math/Aabb.h>
#include <AzCore/Math/Vector3.h>
#include <GradientSignal/GradientTransform.h>

namespace GradientSignal
{
    class GradientTransformRequests
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = AZ::EntityId;

        //! allows multiple threads to call gradient transform requests
        using MutexType = AZStd::recursive_mutex;

        virtual ~GradientTransformRequests() = default;

        //! Get the GradientTransform that's been configured by the bus listener.
        //! /return the GradientTransform instance that can be used to transform world points into gradient lookup space.
        virtual const GradientTransform& GetGradientTransform() const = 0;
    };

    using GradientTransformRequestBus = AZ::EBus<GradientTransformRequests>;

   /**
    * Notifies about changes to the GradientTransform configuration
    */
    class GradientTransformNotifications
        : public AZ::EBusTraits
    {
    public:
        ////////////////////////////////////////////////////////////////////////
        // EBusTraits
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = AZ::EntityId;
        using MutexType = AZStd::recursive_mutex;
        ////////////////////////////////////////////////////////////////////////

        //! Notify listeners that the GradientTransform configuration has changed.
        //! /return the GradientTransform instance that can be used to transform world points into gradient lookup space.
        virtual void OnGradientTransformChanged(const GradientTransform& newTransform) = 0;

        //! Connection policy that auto-calls OnGradientTransformChanged on connection with the current GradientTransform data.
        template<class Bus>
        struct ConnectionPolicy : public AZ::EBusConnectionPolicy<Bus>
        {
            static void Connect(
                typename Bus::BusPtr& busPtr,
                typename Bus::Context& context,
                typename Bus::HandlerNode& handler,
                typename Bus::Context::ConnectLockGuard& connectLock,
                const typename Bus::BusIdType& id = 0)
            {
                AZ::EBusConnectionPolicy<Bus>::Connect(busPtr, context, handler, connectLock, id);

                GradientTransform transform;
                GradientTransformRequestBus::EventResult(transform, id, &GradientTransformRequests::GetGradientTransform);
                handler->OnGradientTransformChanged(transform);
            }
        };
    };

    using GradientTransformNotificationBus = AZ::EBus<GradientTransformNotifications>;

} //namespace GradientSignal
