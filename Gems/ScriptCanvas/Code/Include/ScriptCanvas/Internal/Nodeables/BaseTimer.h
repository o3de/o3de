/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Component/TickBus.h>

#include <Include/ScriptCanvas/Internal/Nodeables/BaseTimer.generated.h>

#include <ScriptCanvas/CodeGen/NodeableCodegen.h>
#include <ScriptCanvas/Core/Node.h>
#include <ScriptCanvas/Core/Nodeable.h>

namespace ScriptCanvas
{
    namespace Nodeables
    {
        namespace Time
        {
            class BaseTimer
                : public ScriptCanvas::Nodeable
                , public NodePropertyInterfaceListener
                , public AZ::TickBus::Handler
                , public AZ::SystemTickBus::Handler
            {
                SCRIPTCANVAS_NODE(BaseTimer);

            public:
                AZ_CLASS_ALLOCATOR(BaseTimer, AZ::SystemAllocator)

                enum TimeUnits
                {
                    Ticks,
                    Milliseconds,
                    Seconds,
                };

                static constexpr const char* s_timeUnitNames[] = 
                {
                    "Ticks",
                    "Milliseconds",
                    "Seconds"
                };

                BaseTimer();
                virtual ~BaseTimer();

                // SystemTickBus
                void OnSystemTick() override;

                // TickBus
                void OnTick(float delta, AZ::ScriptTimePoint timePoint) override;
                int GetTickOrder() override;

                void SetTimeUnits(int timeUnits);
                void StartTimer(Data::NumberType time);
                void StopTimer();

            protected:
                AZStd::vector<AZStd::pair<int, AZStd::string>> GetTimeUnitList() const;

                void OnTimeUnitsChanged(const int& delayUnits);
                void UpdateTimeName();

                virtual bool AllowInstantResponse() const { return false; }

                void OnDeactivate() override;

                virtual void OnTimeElapsed() {}

                ScriptCanvas::EnumComboBoxNodePropertyInterface m_timeUnitsInterface;

            private:

                bool m_isActive = false;
                Data::NumberType m_timerCounter = 0.0;
                Data::NumberType m_timerDuration = 0.0;
            };
        }
    }
}
