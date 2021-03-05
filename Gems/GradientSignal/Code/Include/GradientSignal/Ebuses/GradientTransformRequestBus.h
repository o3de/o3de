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