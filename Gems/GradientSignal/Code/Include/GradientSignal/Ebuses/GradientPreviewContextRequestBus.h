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
#include <AzCore/Component/EntityId.h>
#include <AzCore/Math/Aabb.h>
#include <AzCore/Math/Vector3.h>

namespace GradientSignal
{
    /**
    * determine the order of execution of requests
    */
    enum class GradientPreviewContextPriority : AZ::u8
    {
        Standard = 0,
        Superior = 1,
    };

    /**
    * Bus providing context and settings to control the gradient previewer
    */
    class GradientPreviewContextRequests
        : public AZ::EBusTraits
    {
    public:
        ////////////////////////////////////////////////////////////////////////
        // EBusTraits
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::MultipleAndOrdered;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = AZ::EntityId;
        using MutexType = AZStd::recursive_mutex;
        ////////////////////////////////////////////////////////////////////////

        virtual ~GradientPreviewContextRequests() = default;

        virtual AZ::EntityId GetPreviewEntity() const { return AZ::EntityId(); }
        virtual AZ::Aabb GetPreviewBounds() const { return AZ::Aabb::CreateNull(); }
        virtual bool GetConstrainToShape() const { return true; }

        /**
        * Determines the order in which handlers receive events.
        */
        struct BusHandlerOrderCompare
        {
            bool operator()(GradientPreviewContextRequests* left, GradientPreviewContextRequests* right) const { return left->GetPreviewContextPriority() < right->GetPreviewContextPriority(); }
        };

        virtual GradientPreviewContextPriority GetPreviewContextPriority() const { return GradientPreviewContextPriority::Standard; };
    };

    using GradientPreviewContextRequestBus = AZ::EBus<GradientPreviewContextRequests>;

} // namespace GradientSignal