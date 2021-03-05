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