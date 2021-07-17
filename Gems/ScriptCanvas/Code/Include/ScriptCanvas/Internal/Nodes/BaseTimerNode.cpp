/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
                AZStd::string CreateTimeSlotName(const AZStd::string& stringFormat, BaseTimerNode::TimeUnits delayUnits)
                {
                    AZStd::string stringifiedUnits = BaseTimerNode::s_timeUnitNames[delayUnits];
                    return AZStd::string::format(stringFormat.c_str(), stringifiedUnits.c_str());
                }
            }
            
            void BaseTimerNode::OnInit()
            {
                AZStd::string slotName = GetTimeSlotName();

                Slot* slot = GetSlotByName(slotName);

                if (slot)
                {
                    m_timeSlotId = slot->GetId();
                }
                // Versioning to deal with slot name needing to update
                // based on old version of the slot name
                else
                {
                    AZStd::string slotName2 = CreateTimeSlotName(GetTimeSlotFormat(), static_cast<TimeUnits>(m_timeUnits));

                    slot = GetSlotByName(slotName2);

                    if (slot)
                    {
                        slot->SetToolTip(GetBaseTimeSlotToolTip());
                        m_timeSlotId = slot->GetId();
                    }
                }

                UpdateTimeName();
                ////

                m_timeUnitsInterface.SetPropertyReference(&m_timeUnits);

                m_timeUnitsInterface.RegisterValueType(s_timeUnitNames[TimeUnits::Ticks], TimeUnits::Ticks);
                m_timeUnitsInterface.RegisterValueType(s_timeUnitNames[TimeUnits::Milliseconds], TimeUnits::Milliseconds);
                m_timeUnitsInterface.RegisterValueType(s_timeUnitNames[TimeUnits::Seconds], TimeUnits::Seconds);

                m_timeUnitsInterface.RegisterListener(this);
            }
            
            void BaseTimerNode::OnConfigured()
            {
                AddTimeDataSlot();
            }
            
            void BaseTimerNode::ConfigureVisualExtensions()
            {
                {
                    VisualExtensionSlotConfiguration visualExtensions(VisualExtensionSlotConfiguration::VisualExtensionType::PropertySlot);

                    visualExtensions.m_name = "Units";
                    visualExtensions.m_tooltip = "";
                    visualExtensions.m_connectionType = ConnectionType::Input;

                    visualExtensions.m_identifier = GetTimeUnitsPropertyId();

                    RegisterExtension(visualExtensions);
                }
            }

            NodePropertyInterface* BaseTimerNode::GetPropertyInterface(AZ::Crc32 propertyId)
            {
                if (propertyId == GetTimeUnitsPropertyId())
                {
                    return &m_timeUnitsInterface;
                }

                return nullptr;
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
                    slotConfiguration.m_toolTip = GetBaseTimeSlotToolTip();
                    slotConfiguration.SetConnectionType(ConnectionType::Input);
                    slotConfiguration.SetDefaultValue(1.0);

                    m_timeSlotId = AddSlot(slotConfiguration);
                }
            }
                
            AZStd::string BaseTimerNode::GetTimeSlotName() const
            {
                return GetBaseTimeSlotName();
            }
            
            BaseTimerNode::TimeUnits BaseTimerNode::GetTimeUnits() const
            {
                return static_cast<TimeUnits>(m_timeUnits);
            }
            
            AZStd::vector<AZStd::pair<int, AZStd::string>> BaseTimerNode::GetTimeUnitList() const
            {
                AZStd::vector<AZStd::pair<int, AZStd::string>> timeUnits;

                timeUnits.push_back(AZStd::make_pair(TimeUnits::Ticks, s_timeUnitNames[TimeUnits::Ticks]));
                timeUnits.push_back(AZStd::make_pair(TimeUnits::Milliseconds, s_timeUnitNames[TimeUnits::Milliseconds]));
                timeUnits.push_back(AZStd::make_pair(TimeUnits::Seconds, s_timeUnitNames[TimeUnits::Seconds]));

                return timeUnits;
            }
            
            void BaseTimerNode::OnTimeUnitsChanged(const int&)
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
                return m_isActive;
            }

            bool BaseTimerNode::AllowInstantResponse() const
            {
                return false;
            }
                        
            void BaseTimerNode::ConfigureTimeSlot(DataSlotConfiguration& configuration)
            {
                AZ_UNUSED(configuration);
            }

            void BaseTimerNode::OnPropertyChanged()
            {
                OnTimeUnitsChanged(m_timeUnits);
            }
        }
    }
}
