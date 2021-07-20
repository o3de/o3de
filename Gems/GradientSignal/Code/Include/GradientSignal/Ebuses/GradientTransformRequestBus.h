/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Component/EntityId.h>
#include <AzCore/EBus/EBus.h>
#include <AzCore/Math/Aabb.h>
#include <AzCore/Math/Vector3.h>

namespace GradientSignal
{
    class GradientTransformRequests
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = AZ::EntityId;

        //! allows multiple threads to call shape requests
        using MutexType = AZStd::recursive_mutex;

        virtual ~GradientTransformRequests() = default;

        virtual void TransformPositionToUVW(const AZ::Vector3& inPosition, AZ::Vector3& outUVW, const bool shouldNormalizeOutput, bool& wasPointRejected) const = 0;
        virtual void GetGradientLocalBounds(AZ::Aabb& bounds) const = 0;
        virtual void GetGradientEncompassingBounds(AZ::Aabb& bounds) const = 0;
    };

    using GradientTransformRequestBus = AZ::EBus<GradientTransformRequests>;
} //namespace GradientSignal
