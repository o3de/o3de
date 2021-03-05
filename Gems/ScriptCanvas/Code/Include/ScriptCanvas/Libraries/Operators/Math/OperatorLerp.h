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
#include <AzCore/RTTI/TypeInfo.h>
#include <AzCore/std/containers/unordered_set.h>

#include <ScriptCanvas/CodeGen/CodeGen.h>
#include <ScriptCanvas/Core/Datum.h>
#include <ScriptCanvas/Core/Node.h>
#include <ScriptCanvas/Core/GraphBus.h>


#include <Include/ScriptCanvas/Libraries/Operators/Math/OperatorLerp.generated.h>

namespace ScriptCanvas
{
    namespace Nodes
    {
#define LERPABLE_TYPES { Data::Type::Number(), Data::Type::Vector2(), Data::Type::Vector3(), Data::Type::Vector4() }

        class LerpBetween 
            : public Node
            , public AZ::SystemTickBus::Handler
            , public AZ::TickBus::Handler
        {
        public:

            ScriptCanvas_Node(LerpBetween,
                ScriptCanvas_Node::Name("Lerp Between")
                ScriptCanvas_Node::Uuid("{58AF538D-021A-4D0C-A3F1-866C2FFF382E}")
                ScriptCanvas_Node::Description("Performs a lerp between the two specified sources using the speed specified or in the amount of time specified, or the minimum of the two")
                ScriptCanvas_Node::Version(1)
                ScriptCanvas_Node::Category("Math")
            );
            
            // General purpose nodes that we don't need to do anything fancy for
            ScriptCanvas_In(ScriptCanvas_In::Name("In", ""));
            ScriptCanvas_In(ScriptCanvas_In::Name("Cancel", ""));
            
            ScriptCanvas_Out(ScriptCanvas_Out::Name("Out", ""));
            ScriptCanvas_OutLatent(ScriptCanvas_Out::Name("Tick", "Signalled at each step of the lerp"));
            ScriptCanvas_OutLatent(ScriptCanvas_Out::Name("Lerp Complete", "Output signal"));
            ////

            // DataInput

            // Because all of the data slots are grouped together. We only need one of them to have
            // the restricted type contract and all of them will get it.
            ScriptCanvas_DynamicDataSlot(ScriptCanvas::DynamicDataType::Value,
                                         ScriptCanvas::ConnectionType::Input,
                                         ScriptCanvas_DynamicDataSlot::Name("Start", "The value to start lerping from.")
                                         ScriptCanvas_DynamicDataSlot::DynamicGroup("LerpGroup")
                                         ScriptCanvas_DynamicDataSlot::RestrictedTypeContractTag(LERPABLE_TYPES)
                                        );

            ScriptCanvas_DynamicDataSlot(ScriptCanvas::DynamicDataType::Value,
                                         ScriptCanvas::ConnectionType::Input,
                                         ScriptCanvas_DynamicDataSlot::Name("Stop", "The value to stop lerping at.")
                                         ScriptCanvas_DynamicDataSlot::DynamicGroup("LerpGroup")
                                        );

            ScriptCanvas_DynamicDataSlot(ScriptCanvas::DynamicDataType::Value,
                                         ScriptCanvas::ConnectionType::Input,
                                         ScriptCanvas_DynamicDataSlot::Name("Speed", "The speed at which to lerp between the start and stop.")
                                         ScriptCanvas_DynamicDataSlot::DynamicGroup("LerpGroup")
                                        );

            ScriptCanvas_PropertyWithDefaults(int, -1,
                                                ScriptCanvas_Property::Name("Maximum Duration", "The maximum amount of time it will take to complete the specified lerp. Negative value implies no limit, 0 implies instant.")
                                                ScriptCanvas_Property::Input
                                            );
            ////

            // DataOutput
            ScriptCanvas_DynamicDataSlot(ScriptCanvas::DynamicDataType::Value,
                                         ScriptCanvas::ConnectionType::Output,
                                         ScriptCanvas_DynamicDataSlot::Name("Step", "The value of the current step of the lerp.")
                                         ScriptCanvas_DynamicDataSlot::DynamicGroup("LerpGroup")
                                        );

            ScriptCanvas_Property(float,
                                  ScriptCanvas_Property::Name("Percent", "The percentage of the way through the lerp this tick is on.")
                                  ScriptCanvas_Property::Output
                                 );
            
            // Serialize Properties
            ScriptCanvas_SerializeProperty(Data::Type, m_displayType);

            // Data Input SlotIds
            SlotId m_startSlotId;
            SlotId m_stopSlotId;
            SlotId m_speedSlotId;
            SlotId m_maximumTimeSlotId;
            ////

            // Data Output SlotIds
            SlotId m_stepSlotId;
            SlotId m_percentSlotId;
            ////

            ////

            LerpBetween() = default;
            ~LerpBetween() override = default;
            
            void OnInit() override;
            void OnDeactivate() override;
            void OnConfigured() override;
            
            // SystemTickBus
            void OnSystemTick() override;
            ////
            
            // TickBus
            void OnTick(float deltaTime, AZ::ScriptTimePoint timePoint) override;                
            ////
            
            // Node
            void OnInputSignal(const SlotId& slotId) override;
            ////
            
        private:

            void SetupInternalSlotReferences();
        
            void CancelLerp();

            void SignalLerpStep(float percent);
            
            bool IsGroupConnected() const;
            bool IsInGroup(const SlotId& slotId) const;            
            
            template<typename DataType>
            void CalculateLerpStep(float percent, Datum& stepDatum)
            {
                const DataType* startValue = m_startDatum.GetAs<DataType>();
                const DataType* differenceValue = m_differenceDatum.GetAs<DataType>();
                
                if (startValue == nullptr || differenceValue == nullptr)
                {
                    return;
                }

                DataType stepValue = (*startValue) + (*differenceValue) * percent;
                stepDatum.Set<DataType>(stepValue);
            }
            
            template<typename DataType>
            void SetupStartStop(Data::Type displayType)
            {
                const Datum* startDatum = FindDatum(m_startSlotId);
                const Datum* endDatum = FindDatum(m_stopSlotId);
                
                m_startDatum = (*startDatum);
                m_differenceDatum = Datum(displayType, Datum::eOriginality::Original);
                
                const DataType* startValue = startDatum->GetAs<DataType>();
                const DataType* endValue = endDatum->GetAs<DataType>();

                if (startValue == nullptr || endValue == nullptr)
                {
                    m_differenceDatum.SetToDefaultValueOfType();
                }
                else
                {
                    DataType difference = (*endValue) - (*startValue);
                    m_differenceDatum.Set<DataType>(difference);
                }
            }
            
            template<typename DataType>
            float CalculateVectorSpeedTime(const Datum* speedDatum)
            {
                const DataType* dataType = speedDatum->GetAs<DataType>();
                
                float speedLength = 0.0f;
                
                if (dataType)
                {
                    speedLength = dataType->GetLength();
                }
                
                if (AZ::IsClose(speedLength, 0.0f, AZ::Constants::FloatEpsilon))
                {
                    return -1.0;
                }
                
                float differenceLength = 0.0;
                const DataType* differenceValue = m_differenceDatum.GetAs<DataType>();
                
                if (differenceValue)
                {
                    differenceLength = differenceValue->GetLength();
                }
                
                return fabsf(static_cast<float>(differenceLength/speedLength));
            }            
            
            AZStd::unordered_set< SlotId > m_groupedSlotIds;
        
            float m_duration;
            float m_counter;
            
            Datum m_startDatum;
            Datum m_differenceDatum;
        };

#undef LERPABLE_TYPES
    }
}
