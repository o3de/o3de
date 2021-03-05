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
#include <AtomLyIntegration/CommonFeatures/PostProcess/Ssao/SsaoComponentConfiguration.h>
#include <Atom/Feature/PostProcess/Ssao/SsaoConstants.h>
#include <PostProcess/Ssao/SsaoComponentController.h>

namespace AZ
{
    namespace Render
    {
        namespace Ssao
        {
            static constexpr const char* const SsaoComponentTypeId = "{F1203F4B-89B6-409E-AB99-B9CC87AABC2E}";
        }

        class SsaoComponent final
            : public AzFramework::Components::ComponentAdapter<SsaoComponentController, SsaoComponentConfig>
        {
        public:
            using BaseClass = AzFramework::Components::ComponentAdapter<SsaoComponentController, SsaoComponentConfig>;
            AZ_COMPONENT(AZ::Render::SsaoComponent, Ssao::SsaoComponentTypeId , BaseClass);

            SsaoComponent() = default;
            SsaoComponent(const SsaoComponentConfig& config);

            static void Reflect(AZ::ReflectContext* context);
        };
    } // namespace Render
} // namespace AZ
