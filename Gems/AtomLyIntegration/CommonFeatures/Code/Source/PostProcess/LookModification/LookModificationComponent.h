/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>
#include <AzFramework/Components/ComponentAdapter.h>
#include <AtomLyIntegration/CommonFeatures/PostProcess/LookModification/LookModificationComponentConfig.h>
#include <AtomLyIntegration/CommonFeatures/PostProcess/LookModification/LookModificationComponentConstants.h>
#include <PostProcess/LookModification/LookModificationComponentController.h>

namespace AZ
{
    namespace Render
    {
        class LookModificationComponent final
            : public AzFramework::Components::ComponentAdapter<LookModificationComponentController, LookModificationComponentConfig>
        {
        public:
            using BaseClass = AzFramework::Components::ComponentAdapter<LookModificationComponentController, LookModificationComponentConfig>;
            AZ_COMPONENT(AZ::Render::LookModificationComponent, LookModificationComponentTypeId , BaseClass);

            LookModificationComponent() = default;
            LookModificationComponent(const LookModificationComponentConfig& config);

            static void Reflect(AZ::ReflectContext* context);
        };
    } // namespace Render
} // namespace AZ
