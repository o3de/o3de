/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/ComponentBus.h>
#include <GradientSignal/GradientSampler.h>

namespace GradientSignal
{
    class PosterizeGradientRequests
        : public AZ::ComponentBus
    {
    public:
        /**
         * Overrides the default AZ::EBusTraits handler policy to allow one
         * listener only.
         */
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        //////////////////////////////////////////////////////////////////////////

        virtual AZ::s32 GetBands() const = 0;
        virtual void SetBands(AZ::s32 bands) = 0;

        virtual AZ::u8 GetModeType() const = 0;
        virtual void SetModeType(AZ::u8 modeType) = 0;

        virtual GradientSampler& GetGradientSampler() = 0;
    };

    using PosterizeGradientRequestBus = AZ::EBus<PosterizeGradientRequests>;
}
