/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
            inline constexpr AZ::TypeId SsaoComponentTypeId{ "{F1203F4B-89B6-409E-AB99-B9CC87AABC2E}" };
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
