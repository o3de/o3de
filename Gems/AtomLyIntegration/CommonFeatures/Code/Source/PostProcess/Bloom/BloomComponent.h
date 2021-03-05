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
#include <AzFramework/Components/ComponentAdapter.h>
#include <AtomLyIntegration/CommonFeatures/PostProcess/Bloom/BloomComponentConfig.h>
#include <Atom/Feature/PostProcess/Bloom/BloomConstants.h>
#include <PostProcess/Bloom/BloomComponentController.h>

namespace AZ
{
    namespace Render
    {
        namespace Bloom
        {
            static constexpr const char* const BloomComponentTypeId = "{0D38705D-525D-4BA7-A805-26E3E9093F54}";
        }

        class BloomComponent final
            : public AzFramework::Components::ComponentAdapter<BloomComponentController, BloomComponentConfig>
        {
        public:
            using BaseClass = AzFramework::Components::ComponentAdapter<BloomComponentController, BloomComponentConfig>;
            AZ_COMPONENT(AZ::Render::BloomComponent, Bloom::BloomComponentTypeId, BaseClass);

            BloomComponent() = default;
            BloomComponent(const BloomComponentConfig& config);

            static void Reflect(AZ::ReflectContext* context);
        };
    } // namespace Render
} // namespace AZ
