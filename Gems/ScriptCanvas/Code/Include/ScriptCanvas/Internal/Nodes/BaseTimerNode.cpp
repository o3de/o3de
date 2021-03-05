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
#include <ScriptCanvas/Internal/Nodes/BaseTimerNode.h>

#include <Libraries/Core/MethodUtility.h>

namespace ScriptCanvas
{
    namespace Nodes    
    {
        namespace Internal
        {
            namespace
            {
                AZStd::string StringifyUnits(BaseTimerNode::TimeUnits timeUnits)
                {
                    switch (timeUnits)
                    {
                    case BaseTimerNode::TimeUnits::Ticks:
                        return "Ticks";
                    case BaseTimerNode::TimeUnits::Milliseconds:
                        return "Milliseconds";
                    case BaseTimerNode::TimeUnits::Seconds:
                        return "Seconds";
                    default:
                        break;
                    }
                    
                    return "???";
                }

                AZStd::string CreateTimeSlotName(const AZStd::string& stringFormat, BaseTimerNode::TimeUnits delayUnits)
                {
                    AZStd::string stringifiedUnits = StringifyUnits(delayUnits).c_str();
                    return AZStd::string::format(stringFormat.c_str(), stringifiedUnits.c_str());
                }
            }
            
            //////////////////
            // BaseTimerNode
            //////////////////
            
            BaseTimerNode::~BaseTimerNode()
            {
                StopTimer();
            }

            void BaseTimerNode::OnInit()
            {
                AZStd::string slotName = GetTimeSlotName();

                Slot* slot = GetSlotByName(slotName);

                if (slot)
                {
                    m_timeSlotId = slot->GetId();
                }
            }
            
            void BaseTimerNode::OnConfigured()
            {
                AddTimeDataSlot();
            }
            
            void BaseTimerNode::OnDeactivate()
            {
                StopTimer();
            }
            
            void BaseTimerNode::OnSystemTick()
            {
                AZ::SystemTickBus::Handler::BusDisconnect();
                
                if (!AZ::TickBus::Handler::BusIsConnected())
                {
                    AZ::TickBus::Handler::BusConnect();
                }
            }
               
            void BaseTimerNode::OnTick(float delta, [[maybe_unused]] AZ::ScriptTimePoint timePoint)
            {
                switch (m_timeUnits)
                {
                case TimeUnits::Ticks:
                    m_timerCounter += 1;
                    break;
                case TimeUnits::Milliseconds:
                case TimeUnits::Seconds:
                    m_timerCounter += delta;
                    break;
                default:
                    break;
                }

                while (m_timerCounter > m_timerDuration)
                {
                    if (!m_isActive)
                    {
                        break;
                    }
                    
                    m_timerCounter -= m_timerDuration;
                    
                    OnTimeElapsed();
                }
            }

            int BaseTimerNode::GetTickOrder()
            {
                return m_tickOrder;
            }
            
            void BaseTimerNode::AddTimeDataSlot()
            {
                if (!m_timeSlotId.IsValid())
                {
                    AZStd::string slotName = GetTimeSlotName();
                    
                    DataSlotConfiguration slotConfiguration;
                    
                    //  Let the user do whatever they want then we'll stomp over what we care about.
                    ConfigureTimeSlot(slotConfiguration);

                    // For now. Time slot must be an input.
                    // Must have the known name.
                    // Must be a number
                    slotConfiguration.m_name = slotName;
                    slotConfiguration.SetConnectionType(ConnectionType::Input);
                    slotConfiguration.SetDefaultValue(1.0);

                    m_timeSlotId = AddSlot(slotConfiguration);
                }
            }
                
            void BaseTimerNode::StartTimer()
            {
                StopTimer();
                
                m_isActive = true;
                
                const Datum* datum = FindDatum(m_timeSlotId);
                
                if (datum)
                {
                    const Data::NumberType* timeAmount = datum->GetAs<Data::NumberType>();

                    if (timeAmount)
                    {
                        m_timerDuration = (*timeAmount);
                        m_timerCounter = 0;

                        // Manage the different unit types
                        if (m_timeUnits == TimeUnits::Ticks)
                        {
                            // Remove all of the floating points
                            m_timerDuration = aznumeric_cast<float>(aznumeric_cast<int>(m_timerDuration));
                        }
                        else if (m_timeUnits == TimeUnits::Milliseconds)
                        {
                            m_timerDuration /= 1000.0;
                        }
                    }
                }

                if (AZ::TickBus::Handler::BusIsConnected())
                {
                    AZ::TickBus::Handler::BusDisconnect();
                }

                if (AZ::SystemTickBus::Handler::BusIsConnected())
                {
                    AZ::SystemTickBus::Handler::BusDisconnect();
                }
                
                if (!AZ::IsClose(m_timerDuration, 0.0, DBL_EPSILON))
                {
                    AZ::SystemTickBus::Handler::BusConnect();
                }
                else if (AllowInstantResponse())
                {
                    while (m_isActive)
                    {
                        OnTimeElapsed();
                    }
                }
            }
            
            void BaseTimerNode::StopTimer()
            {
                m_isActive = false;

                m_timerCounter = 0.0;
                m_timerDuration = 0.0;
                
                AZ::SystemTickBus::Handler::BusDisconnect();
                AZ::TickBus::Handler::BusDisconnect();
            }

            AZStd::string BaseTimerNode::GetTimeSlotName() const
            {
                return CreateTimeSlotName(GetTimeSlotFormat(), static_cast<TimeUnits>(m_timeUnits));
            }
            
            BaseTimerNode::TimeUnits BaseTimerNode::GetTimeUnits() const
            {
                return static_cast<TimeUnits>(m_timeUnits);
            }
            
            AZStd::vector<AZStd::pair<int, AZStd::string>> BaseTimerNode::GetTimeUnitList() const
            {
                AZStd::vector<AZStd::pair<int, AZStd::string>> timeUnits;
                timeUnits.reserve(static_cast<int>(UnitCount));
                
                for (int i = Ticks; i < UnitCount; ++i)
                {
                    timeUnits.emplace_back(i, StringifyUnits(static_cast<TimeUnits>(i)));
                }
                
                return timeUnits;
            }
            
            void BaseTimerNode::OnTimeUnitsChanged([[maybe_unused]] const int& timeUnits)
            {
                UpdateTimeName();
            }
            
            void BaseTimerNode::UpdateTimeName()
            {
                Slot* slot = GetSlot(m_timeSlotId);

                if (slot)
                {
                    slot->Rename(GetTimeSlotName());
                }
            }

            bool BaseTimerNode::IsActive() const
            {
                return false;
            }

            bool BaseTimerNode::AllowInstantResponse() const
            {
                return false;
            }
            
            void BaseTimerNode::OnTimeElapsed()
            {
            }
            
            void BaseTimerNode::ConfigureTimeSlot(DataSlotConfiguration& configuration)
            {
                AZ_UNUSED(configuration);
            }
        }
    }
}
