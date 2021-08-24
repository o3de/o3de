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

// Hair specific
#include <Components/HairComponentConfig.h>
#include <Components/HairComponentController.h>

namespace AZ
{
    namespace Render
    {
        namespace Hair
        {
            class HairRenderObject;

            static constexpr const char* const HairComponentTypeId = "{9556883B-6F3C-4010-BB3F-EBB480515D68}";

            //! Parallel to the 'EditorHairComponent' this class is used in game mode.
            class HairComponent final
                : public AzFramework::Components::ComponentAdapter<HairComponentController, HairComponentConfig>
            {
            public:
                using BaseClass = AzFramework::Components::ComponentAdapter<HairComponentController, HairComponentConfig>;
                AZ_COMPONENT(AZ::Render::Hair::HairComponent, Hair::HairComponentTypeId, BaseClass);

                HairComponent() = default;
                HairComponent(const HairComponentConfig& config);
                ~HairComponent();

                static void Reflect(AZ::ReflectContext* context);

                void Activate() override;
            };
        } // namespace Hair
    } // namespace Render
} // namespace AZ
