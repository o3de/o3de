/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/Feature/PostProcess/ChromaticAberration/ChromaticAberrationConstants.h>
#include <PostProcess/ChromaticAberration/ChromaticAberrationComponentController.h>
#include <AtomLyIntegration/CommonFeatures/PostProcess/ChromaticAberration/ChromaticAberrationComponentConfig.h>
#include <AzCore/Component/Component.h>
#include <AzFramework/Components/ComponentAdapter.h>

namespace AZ
{
    namespace Render
    {
        namespace ChromaticAberration
        {
            inline constexpr AZ::TypeId ChromaticAberrationComponentTypeId{ "{123FF51D-7234-429D-817B-FA89F436826B}" };
        }

        class ChromaticAberrationComponent final
            : public AzFramework::Components::ComponentAdapter<ChromaticAberrationComponentController, ChromaticAberrationComponentConfig>
        {
        public:
            using BaseClass = AzFramework::Components::ComponentAdapter<ChromaticAberrationComponentController, ChromaticAberrationComponentConfig>;
            AZ_COMPONENT(AZ::Render::ChromaticAberrationComponent, ChromaticAberration::ChromaticAberrationComponentTypeId, BaseClass);

            ChromaticAberrationComponent() = default;
            ChromaticAberrationComponent(const ChromaticAberrationComponentConfig& config);

            static void Reflect(AZ::ReflectContext* context);
        };
    } // namespace Render
} // namespace AZ
