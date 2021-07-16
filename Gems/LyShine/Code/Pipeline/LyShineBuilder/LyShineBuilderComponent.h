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
#include <Pipeline/LyShineBuilder/UiCanvasBuilderWorker.h>

namespace LyShine
{
    namespace LyShineBuilder
    {
        class LyShineBuilderComponent
            : public AZ::Component
        {
        public:
            AZ_COMPONENT(LyShineBuilderComponent, "{EBDFDA04-0D23-4E54-BD4C-2EF8EEF5A606}");
            static void Reflect(AZ::ReflectContext* context);

            LyShineBuilderComponent() = default;

            //////////////////////////////////////////////////////////////////////////
            // AZ::Component
            void Activate() override;
            void Deactivate() override;
            //////////////////////////////////////////////////////////////////////////

        private:

            //class cannot be copied
            LyShineBuilderComponent(const LyShineBuilderComponent&) = delete;
            LyShineBuilderComponent& operator=(const LyShineBuilderComponent&) = delete;

            UiCanvasBuilderWorker m_uiCanvasBuilder;
        };
    } // namespace LyShineBuilder
} // namespace LyShine
