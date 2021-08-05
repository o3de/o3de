/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
