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

#include <Include/ScriptCanvas/Libraries/Time/DurationNodeable.generated.h>

namespace ScriptCanvas
{
    namespace Nodeables
    {
        namespace Time
        {
            class DurationNodeable
                : public ScriptCanvas::Nodeable
                , public AZ::TickBus::Handler
            {
                SCRIPTCANVAS_NODE(DurationNodeable);

            public:
                AZ_CLASS_ALLOCATOR(DurationNodeable, AZ::SystemAllocator)

                ~DurationNodeable() override;

            protected:
                void OnDeactivate() override;

                // TickBus
                void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;

            private:
                float m_elapsedTime = 0.0f;
                float m_duration = 0.0f;
            };
        }
    }
}
