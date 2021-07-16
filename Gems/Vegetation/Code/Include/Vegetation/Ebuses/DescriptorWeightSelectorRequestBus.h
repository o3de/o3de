/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/ComponentBus.h>
#include <AzCore/Component/EntityId.h>
#include <GradientSignal/GradientSampler.h>

namespace Vegetation
{
    enum class SortBehavior : AZ::u8
    {
        Unsorted = 0,
        Ascending,
        Descending,
    };

    class DescriptorWeightSelectorRequests
        : public AZ::ComponentBus
    {
    public:
        /**
         * Overrides the default AZ::EBusTraits handler policy to allow one
         * listener only.
         */
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        //////////////////////////////////////////////////////////////////////////

        virtual SortBehavior GetSortBehavior() const = 0;
        virtual void SetSortBehavior(SortBehavior behavior) = 0;

        virtual GradientSignal::GradientSampler& GetGradientSampler() = 0;
    };

    using DescriptorWeightSelectorRequestBus = AZ::EBus<DescriptorWeightSelectorRequests>;
}
