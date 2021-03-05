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
        class ShapeWeightModifierComponentConfig final
            : public ComponentConfig
        {
        public:
            AZ_RTTI(AZ::Render::ShapeWeightModifierComponentConfig, "{2CB13B7C-C532-4DFB-8FF4-79EF9F860868}", AZ::ComponentConfig);

            static void Reflect(ReflectContext* context);

            float m_falloffDistance = 1.0f;
        };
    }
}
