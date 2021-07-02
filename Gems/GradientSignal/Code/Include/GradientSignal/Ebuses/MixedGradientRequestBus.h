/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/ComponentBus.h>

namespace GradientSignal
{
    class MixedGradientLayer;

    class MixedGradientRequests
        : public AZ::ComponentBus
    {
    public:
        /**
         * Overrides the default AZ::EBusTraits handler policy to allow one
         * listener only.
         */
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;

        virtual size_t GetNumLayers() const = 0;
        virtual void AddLayer() = 0;
        virtual void RemoveLayer(int layerIndex) = 0;
        virtual MixedGradientLayer* GetLayer(int layerIndex) = 0;
    };

    using MixedGradientRequestBus = AZ::EBus<MixedGradientRequests>;
}
