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
