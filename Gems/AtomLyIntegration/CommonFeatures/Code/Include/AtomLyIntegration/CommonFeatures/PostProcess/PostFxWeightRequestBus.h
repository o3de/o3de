/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>

namespace AZ
{
    namespace Render
    {
        // PostFxWeightRequestBus provides an interface to call methods for modulating a PostFx's weight.
        class PostFxWeightRequests
            : public ComponentBus
        {
        public:
            static const EBusHandlerPolicy HandlerPolicy = EBusHandlerPolicy::Single;

            virtual ~PostFxWeightRequests() = default;

            // Given a position, return a weight value. 
            virtual float GetWeightAtPosition(const AZ::Vector3& influencerPosition) const = 0;
        };

        using PostFxWeightRequestBus = AZ::EBus<PostFxWeightRequests>;
    }
}
