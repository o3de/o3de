/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/TickBus.h>
#include <AzCore/RTTI/TypeInfo.h>
#include <AzCore/std/containers/unordered_set.h>

#include <ScriptCanvas/Core/Datum.h>
#include <ScriptCanvas/Core/Node.h>
#include <ScriptCanvas/Core/GraphBus.h>


#include <Include/ScriptCanvas/Libraries/Operators/Math/OperatorLerp.generated.h>

namespace ScriptCanvas
{
    namespace Nodes
    {
#define LERPABLE_TYPES { Data::Type::Number(), Data::Type::Vector2(), Data::Type::Vector3(), Data::Type::Vector4() }

        //! Deprecated: see NodeableNodeOverloadedLerp
        class LerpBetween 
            : public Node
            , public AZ::SystemTickBus::Handler
            , public AZ::TickBus::Handler
        {
        public:

            SCRIPTCANVAS_NODE(LerpBetween);

            Data::Type m_displayType;

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
            
            // SystemTickBus...
            void OnSystemTick() override;
            ////
            
            // TickBus...
            void OnTick(float deltaTime, AZ::ScriptTimePoint timePoint) override;                
            ////
            
            // Node...
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
