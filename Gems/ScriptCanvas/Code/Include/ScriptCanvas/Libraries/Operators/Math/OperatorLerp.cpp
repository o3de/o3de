/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "OperatorLerp.h"

#include <ScriptCanvas/Core/Slot.h>
#include <ScriptCanvas/Core/Core.h>
#include <ScriptCanvas/Utils/VersionConverters.h>

#include <Include/ScriptCanvas/Data/NumericData.h>

namespace ScriptCanvas
{
    namespace Nodes
    {
        void LerpBetween::OnInit()
        {
            SetupInternalSlotReferences();

            ///////// Version Conversion to Dynamic Grouped based operators
            SlotId stepId = LerpBetweenProperty::GetStepSlotId(this);

            if (stepId.IsValid())
            {                
                for (const SlotId& slotId : { m_startSlotId, m_stopSlotId, m_speedSlotId, stepId })
                {
                    Slot* slot = GetSlot(slotId);

                    if (slot->IsDynamicSlot() && slot->GetDynamicGroup() == AZ::Crc32())
                    {
                        SetDynamicGroup(slotId, AZ::Crc32("LerpGroup"));
                    }
                }
            }
            ////////

            //////// Version Conversion will remove the Display type in a few revisions
            if (m_displayType.IsValid())
            {
                Data::Type displayType = GetDisplayType(AZ::Crc32("LerpGroup"));

                if (!displayType.IsValid())
                {
                    SetDisplayType(AZ::Crc32("LerpGroup"), displayType);
                }
            }
            ////////
        }

        void LerpBetween::OnDeactivate()
        {
            AZ::SystemTickBus::Handler::BusDisconnect();
            AZ::TickBus::Handler::BusDisconnect();
        }

        void LerpBetween::OnConfigured()
        {
            SetupInternalSlotReferences();
        }
        
        void LerpBetween::OnSystemTick()
        {
            // Ping pong between the system and the normal tick bus for a consistent starting point for the lerp
            AZ::SystemTickBus::Handler::BusDisconnect();
            AZ::TickBus::Handler::BusConnect();
        }
        
        void LerpBetween::OnTick(float deltaTime, [[maybe_unused]] AZ::ScriptTimePoint timePoint)
        {
            bool lerpComplete = false;
            
            m_counter += deltaTime;
            
            if (m_counter >= m_duration)
            {
                lerpComplete = true;
                m_counter = m_duration;
            }
            
            float percent = m_counter/m_duration;
            
            SignalLerpStep(percent);
        }
        
        void LerpBetween::OnInputSignal(const SlotId& slotId)
        {
            if (slotId == LerpBetweenProperty::GetCancelSlotId(this))
            {
                CancelLerp();
            }
            else if (slotId == LerpBetweenProperty::GetInSlotId(this))
            {
                CancelLerp();
                
                AZ::SystemTickBus::Handler::BusConnect();
                
                float speedOnlyTime = 0.0;
                float maxDuration = 0.0;
                
                const Datum* durationDatum = FindDatum(m_maximumTimeSlotId);
                const Datum* speedDatum = FindDatum(m_speedSlotId);
                
                if (durationDatum)
                {
                    maxDuration = static_cast<float>((*durationDatum->GetAs<Data::NumberType>()));
                }

                Data::Type displayType = GetDisplayType(AZ::Crc32("LerpGroup"));

                if (displayType == Data::Type::Number())
                {
                    SetupStartStop<Data::NumberType>(displayType);

                    if (!m_differenceDatum.GetType().IsValid())
                    {
                        return;
                    }
                    
                    Data::NumberType difference = (*m_differenceDatum.GetAs<Data::NumberType>());                    
                    Data::NumberType speed = (*speedDatum->GetAs<Data::NumberType>());
                    
                    if (AZ::IsClose(speed, 0.0, Data::ToleranceEpsilon()))
                    {
                        speedOnlyTime = -1.0f;
                    }
                    else
                    {
                        speedOnlyTime = fabsf(static_cast<float>(difference)/static_cast<float>(speed));
                    }                        
                }
                else if (displayType == Data::Type::Vector2())
                {
                    SetupStartStop<Data::Vector2Type>(displayType);
                    speedOnlyTime = CalculateVectorSpeedTime<Data::Vector2Type>(speedDatum);
                }
                else if (displayType == Data::Type::Vector3())
                {
                    SetupStartStop<Data::Vector3Type>(displayType);
                    speedOnlyTime = CalculateVectorSpeedTime<Data::Vector3Type>(speedDatum);
                }
                else if (displayType == Data::Type::Vector4())
                {
                    SetupStartStop<Data::Vector4Type>(displayType);
                    speedOnlyTime = CalculateVectorSpeedTime<Data::Vector4Type>(speedDatum);
                }
                
                m_counter = 0;
                
                if (speedOnlyTime >= 0.0f && maxDuration >= 0.0f)
                {
                    m_duration = AZStd::min(speedOnlyTime, maxDuration);
                }
                else if (speedOnlyTime >= 0.0f)
                {
                    m_duration = speedOnlyTime;
                }
                else if (maxDuration >= 0.0f)
                {
                    m_duration = maxDuration;
                }
                else
                {
                    AZ_Error("ScriptCanvas", false, "Lerp Between not given a valid speed or duration to perofrm the lerp at. Defaulting to 1 second duration");
                    m_duration = 1.0;
                }
                
                if (AZ::IsClose(m_duration, 0.0f, AZ::Constants::FloatEpsilon))
                {
                    CancelLerp();
                    
                    SignalLerpStep(1.0f);
                }
                
                SignalOutput(LerpBetweenProperty::GetOutSlotId(this));
            }
        }        

        void LerpBetween::SetupInternalSlotReferences()
        {
            m_startSlotId = LerpBetweenProperty::GetStartSlotId(this);
            m_stopSlotId = LerpBetweenProperty::GetStopSlotId(this);
            m_maximumTimeSlotId = LerpBetweenProperty::GetMaximumDurationSlotId(this);
            m_speedSlotId = LerpBetweenProperty::GetSpeedSlotId(this);

            m_stepSlotId = LerpBetweenProperty::GetStepSlotId(this);
            m_percentSlotId = LerpBetweenProperty::GetPercentSlotId(this);
        }
        
        void LerpBetween::CancelLerp()
        {
            AZ::SystemTickBus::Handler::BusDisconnect();
            AZ::TickBus::Handler::BusDisconnect();            
        }
        
        void LerpBetween::SignalLerpStep(float percent)
        {
            Data::Type displayType = GetDisplayType(AZ::Crc32("LerpGroup"));

            Datum stepDatum(displayType, Datum::eOriginality::Original);
            stepDatum.SetToDefaultValueOfType();
            
            if (displayType == Data::Type::Number())
            {
                CalculateLerpStep<Data::NumberType>(percent, stepDatum);
            }
            else if (displayType == Data::Type::Vector2())
            {
                CalculateLerpStep<Data::Vector2Type>(percent, stepDatum);
            }
            else if (displayType == Data::Type::Vector3())
            {
                CalculateLerpStep<Data::Vector3Type>(percent, stepDatum);
            }
            else if (displayType == Data::Type::Vector4())
            {
                CalculateLerpStep<Data::Vector4Type>(percent, stepDatum);
            }

            if (AZ::IsClose(percent, 1.0f, AZ::Constants::FloatEpsilon))
            {
                SignalOutput(LerpBetweenProperty::GetLerpCompleteSlotId(this));
                AZ::TickBus::Handler::BusDisconnect();
            }

            Datum percentDatum(ScriptCanvas::Data::Type::Number(), Datum::eOriginality::Original);
            percentDatum.Set<Data::NumberType>(percent);
            
            PushOutput(percentDatum, *GetSlot(m_percentSlotId));
            PushOutput(stepDatum, *GetSlot(m_stepSlotId));
            
            SignalOutput(LerpBetweenProperty::GetTickSlotId(this));            
        }
        
        bool LerpBetween::IsGroupConnected() const
        {
            bool hasConnections = false;
            for (auto slotId : m_groupedSlotIds)
            {
                if (IsConnected(slotId))
                {
                    hasConnections = true;
                    break;
                }
            }
            
            return hasConnections;
        }
        
        bool LerpBetween::IsInGroup(const SlotId& slotId) const
        {
            return m_groupedSlotIds.count(slotId) > 0;
        }
    }
}
