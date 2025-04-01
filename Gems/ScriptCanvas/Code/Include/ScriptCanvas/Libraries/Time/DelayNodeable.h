/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/TickBus.h>

#include <ScriptCanvas/Core/Nodeable.h>
#include <ScriptCanvas/Core/NodeableNode.h>
#include <ScriptCanvas/CodeGen/NodeableCodegen.h>

#include <Include/ScriptCanvas/Libraries/Time/DelayNodeable.generated.h>

namespace ScriptCanvas
{
    namespace Nodeables
    {
        namespace Time
        {
            class DelayNodeable
                : public ScriptCanvas::Nodeable
                , public AZ::TickBus::Handler
            {
            public:
                AZ_CLASS_ALLOCATOR(DelayNodeable, AZ::SystemAllocator)
                SCRIPTCANVAS_NODE(DelayNodeable);

            protected:
                void OnDeactivate() override;

                // TickBus
                void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;

            private:
                // help to gate the latent out once cancel gets called
                bool m_cancelled = false;
                bool m_holding = false;
                bool m_looping = false;
                float m_countdownSeconds = 0.0f;
                float m_currentTime = 0.0f;
                float m_holdTime = 0.0f;

                void InitiateCountdown(bool reset, float countdownSeconds, bool looping, float holdTime);
            };
        }
    }
}
