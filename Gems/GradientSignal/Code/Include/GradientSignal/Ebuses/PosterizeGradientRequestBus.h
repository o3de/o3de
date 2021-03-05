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