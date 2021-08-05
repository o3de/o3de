/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <ScriptCanvas/Core/Node.h>
#include <Include/ScriptCanvas/Internal/Nodes/BaseTimerNode.generated.h>

namespace ScriptCanvas
{
    namespace Nodes    
    {
        namespace Internal
        {
            //! Deprecated: see TimerNodeable
            class BaseTimerNode
                : public Node
                , public NodePropertyInterfaceListener
            {
            public:

                SCRIPTCANVAS_NODE(BaseTimerNode);

                enum TimeUnits
                {
                    Ticks,
                    Milliseconds,
                    Seconds
                };

                static constexpr const char* s_timeUnitNames[] =
                {
                    "Ticks",
                    "Milliseconds",
                    "Seconds"
                };
                
                // Node...
                void OnInit() override;
                void OnConfigured() override;

                void ConfigureVisualExtensions() override;
                NodePropertyInterface* GetPropertyInterface(AZ::Crc32 propertyId) override;
                                                
                // Method that will handle displaying and viewing of the time slot
                void AddTimeDataSlot();
                
                AZStd::string GetTimeSlotName() const;

            protected:
            
                TimeUnits GetTimeUnits() const;
                AZStd::vector<AZStd::pair<int, AZStd::string>> GetTimeUnitList() const;
                
                void OnTimeUnitsChanged(const int& delayUnits);
                void UpdateTimeName();

                bool IsActive() const;
                
                virtual bool AllowInstantResponse() const;

                // Store until versioning is complete
                virtual const char* GetTimeSlotFormat() const { return "Time (%s)";  }
                ////

                virtual const char* GetBaseTimeSlotName() const { return "Delay"; }
                virtual const char* GetBaseTimeSlotToolTip() const { return "The amount of time for the specific action to trigger."; }
                
                virtual void ConfigureTimeSlot(DataSlotConfiguration& configuration);

                SlotId              m_timeSlotId;
                
            private:

                AZ::Crc32 GetTimeUnitsPropertyId() const { return AZ::Crc32("TimeUnitProperty"); }

                // NodePropertyInterface
                void OnPropertyChanged() override;
                ////

                int m_timeUnits = 0;
                int m_tickOrder = 1000; // from TICK_DEFAULT
                    
                bool m_isActive = false;
                    
                Data::NumberType m_timerCounter   = 0.0;
                Data::NumberType m_timerDuration   = 0.0;

                ScriptCanvas::EnumComboBoxNodePropertyInterface m_timeUnitsInterface;
            };
        }
    }
}
