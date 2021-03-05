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

#include <ScriptCanvas/CodeGen/CodeGen.h>
#include <ScriptCanvas/Core/Node.h>

#include <Include/ScriptCanvas/Internal/Nodes/BaseTimerNode.generated.h>

namespace ScriptCanvas
{
    namespace Nodes    
    {
        namespace Internal
        {
            class BaseTimerNode
                : public Node
                , public AZ::TickBus::Handler
                , public AZ::SystemTickBus::Handler
            {
                ScriptCanvas_Node(BaseTimerNode,
                    ScriptCanvas_Node::Name("BaseTimerNode", "Provides a basic interaction layer for all time based nodes for users(handles swapping between ticks and seconds).")
                    ScriptCanvas_Node::Uuid("{BAD6C904-6078-49E8-B461-CA4410B785A4}")
                    ScriptCanvas_Node::Icon("Editor/Icons/ScriptCanvas/Placeholder.png")
                    ScriptCanvas_Node::Category("Utilities")
                    ScriptCanvas_Node::Version(0)
                );

            public:
                enum TimeUnits
                {
                    Unknown = -1,
                    Ticks,
                    Milliseconds,
                    Seconds,

                    UnitCount
                };
                
                virtual ~BaseTimerNode();

                void OnInit() override;
                void OnConfigured() override;
                void OnDeactivate() override;
                
                // SystemTickBus
                void OnSystemTick() override;
                ////
                
                // TickBus
                void OnTick(float delta, AZ::ScriptTimePoint timePoint) override;
                int GetTickOrder() override;
                ////
                
                // Method that will handle displaying and viewing of the time slot
                void AddTimeDataSlot();
                
                void StartTimer();
                void StopTimer();

                AZStd::string GetTimeSlotName() const;

            protected:
            
                TimeUnits GetTimeUnits() const;
                AZStd::vector<AZStd::pair<int, AZStd::string>> GetTimeUnitList() const;
                
                void OnTimeUnitsChanged(const int& delayUnits);
                void UpdateTimeName();

                bool IsActive() const;
                
                virtual bool AllowInstantResponse() const;
                virtual void OnTimeElapsed();
                
                virtual const char* GetTimeSlotFormat() const { return "Time (%s)";  }
                
                virtual void ConfigureTimeSlot(DataSlotConfiguration& configuration);

                SlotId              m_timeSlotId;                
                
            private:
                ScriptCanvas_EditPropertyWithDefaults(int, m_timeUnits, 0, EditProperty::NameLabelOverride("Units"),
                    EditProperty::DescriptionTextOverride("Units to represent the time in."),
                    EditProperty::UIHandler(AZ::Edit::UIHandlers::ComboBox),
                    EditProperty::EditAttributes(AZ::Edit::Attributes::GenericValueList(&BaseTimerNode::GetTimeUnitList), AZ::Edit::Attributes::PostChangeNotify(&BaseTimerNode::OnTimeUnitsChanged)));

                ScriptCanvas_EditPropertyWithDefaults(int, m_tickOrder, static_cast<int>(AZ::TICK_DEFAULT), EditProperty::NameLabelOverride("TickOrder"),
                    EditProperty::DescriptionTextOverride("When the tick for this time update should be handled.")
                );
                    
                bool                m_isActive      = false;
                    
                Data::NumberType    m_timerCounter   = 0.0;
                Data::NumberType    m_timerDuration   = 0.0;
            };
        }
    }
}
