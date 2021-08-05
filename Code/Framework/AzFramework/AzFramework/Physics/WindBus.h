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

namespace Physics
{
    //! An interface to get wind values.
    class WindRequests
    {
    public:
        AZ_TYPE_INFO(WindRequests, "{87CB77B6-BE47-45FD-AA98-C42C19600EE6}");

        WindRequests() = default;        

        //! AZ::Interface requires these to be deleted.
        WindRequests(WindRequests&&) = delete;
        WindRequests& operator=(WindRequests&&) = delete;

        virtual AZ::Vector3 GetGlobalWind() const = 0;

        //! Get accumulated wind value at given world position.
        virtual AZ::Vector3 GetWind(const AZ::Vector3& worldPosition) const = 0;

        //! Get accumulated wind value inside given AABB volume.
        virtual AZ::Vector3 GetWind(const AZ::Aabb& aabb) const = 0;

    protected:
        ~WindRequests() = default;
    };

    //! Wind requests bus traits. Singleton pattern.
    class WindRequestsTraits
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
    };

    using WindRequestsBus = AZ::EBus<WindRequests, WindRequestsTraits>;

    //! Broadcasts notifications when wind state changes - wind providers implement WindRequests bus.
    class WindNotifications
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;

        //! Dispatched when global wind value is changed.
        virtual void OnGlobalWindChanged() {}

        //! Dispatched when local wind volume is moved or changes value.
        virtual void OnWindChanged([[maybe_unused]] const AZ::Aabb& aabb) {}

    protected:
        ~WindNotifications() = default;
    };

    using WindNotificationsBus = AZ::EBus<WindNotifications>;
} // namespace Physics
