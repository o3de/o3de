/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/RTTI/RTTI.h>
#include <AzCore/Component/Component.h>
#include <Pipeline/ShineBuilder/UiCanvasBuilderWorker.h>

namespace Shine
{
    namespace ShineBuilder
    {
        class ShineBuilderComponent
            : public AZ::Component
        {
        public:
            AZ_COMPONENT(ShineBuilderComponent, "{EBDFDA04-0D23-4E54-BD4C-2EF8EEF5A606}");
            static void Reflect(AZ::ReflectContext* context);

            ShineBuilderComponent() = default;

            //////////////////////////////////////////////////////////////////////////
            // AZ::Component
            void Activate() override;
            void Deactivate() override;
            //////////////////////////////////////////////////////////////////////////

        private:

            //class cannot be copied
            ShineBuilderComponent(const ShineBuilderComponent&) = delete;
            ShineBuilderComponent& operator=(const ShineBuilderComponent&) = delete;

            UiCanvasBuilderWorker m_uiCanvasBuilder;
        };
    } // namespace ShineBuilder
} // namespace Shine
