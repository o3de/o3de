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
#include <GradientSignal/GradientSampler.h>

namespace AZ
{
    namespace Render
    {
        class GradientWeightModifierComponentConfig final
            : public ComponentConfig
        {
        public:
            AZ_RTTI(AZ::Render::GradientWeightModifierComponentConfig, "{03CF0D7B-6763-4196-B329-7E247EBEDEF8}", AZ::ComponentConfig);

            static void Reflect(ReflectContext* context);

            GradientSignal::GradientSampler m_gradientSampler;
        };
    }
}
