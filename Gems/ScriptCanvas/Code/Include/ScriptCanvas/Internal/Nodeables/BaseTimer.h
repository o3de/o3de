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
                SCRIPTCANVAS_NODE(BaseTimer)

            public:

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
