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
#ifndef GM_DELTACOMPRESSED_DATASET_H
#define GM_DELTACOMPRESSED_DATASET_H

#pragma once

#include <AzCore/Math/Vector3.h>
#include <GridMate/Serialize/DataMarshal.h>
#include <GridMate/Replica/DataSet.h>

namespace GridMate
{
    namespace Helper
    {
        template<AZ::u32 DeltaRange>
        AZ::u8 GetQuantized(float value)
        {
            /*
             * Quantizing into a single byte, thus 255 values.
             * [-DeltaRange           V    +DeltaRange]
             * [0                     Q            255]
             * Given V, solve for Q.
             */
            const float quantized = (value + DeltaRange) * 255.f / (2.f * DeltaRange);
            const int clamped = AZ::GetClamp(static_cast<int>(quantized), 0, 255);
            return static_cast<AZ::u8>(clamped);
        }

        template<AZ::u32 DeltaRange>
        float GetUnquantized(AZ::u8 quantized)
        {
            /*
             * Unquantizing from a single byte, out of 255 values.
             * [0                     Q            255]
             * [-DeltaRange           V    +DeltaRange]
             * Given Q, solve for V.
             */
            return 2 * DeltaRange * quantized / 255.f - DeltaRange;
        }

        template<typename FieldType>
        struct DeltaHelper;

        /**
         * \brief Works for integer and floating points numbers
         */
        template<typename FieldType>
        struct DeltaHelper
        {
            static bool IsWithinDelta(const FieldType& base, const FieldType& another, AZ::u32 deltaRange)
            {
                return abs(base - another) < deltaRange;
            }
        };

        /**
         * \brief Specialization for AZ::Vector3
         */
        template<>
        struct DeltaHelper<AZ::Vector3>
        {
            static bool IsWithinDelta(const AZ::Vector3& base, const AZ::Vector3& another, AZ::u32 deltaRange)
            {
                const AZ::Vector3 absDiff = (base - another).GetAbs();
                return absDiff.GetX() < deltaRange && absDiff.GetY() < deltaRange && absDiff.GetZ() < deltaRange;
            }
        };
    }

    /**
     * \brief Packing a value into a single byte within +/- @DeltaRange
     */
    template<AZ::u32 DeltaRange, typename FieldType>
    class DeltaMarshaller;

    // float specialization
    template<AZ::u32 DeltaRange>
    class DeltaMarshaller<DeltaRange, float>
    {
    public:
        void Marshal(WriteBuffer& wb, const float &value)
        {
            wb.Write(Helper::GetQuantized<DeltaRange>(value));
        }

        void Unmarshal(float& value, ReadBuffer &rb)
        {
            AZ::u8 delta;
            rb.Read(delta);
            value = Helper::GetUnquantized<DeltaRange>(delta);
        }
    };

    // AZ::Vector3 specialization
    template<AZ::u32 DeltaRange>
    class DeltaMarshaller<DeltaRange, AZ::Vector3>
    {
    public:
        void Marshal(WriteBuffer& wb, const AZ::Vector3& value)
        {
            wb.Write(Helper::GetQuantized<DeltaRange>(value.GetX()));
            wb.Write(Helper::GetQuantized<DeltaRange>(value.GetY()));
            wb.Write(Helper::GetQuantized<DeltaRange>(value.GetZ()));
        }

        void Unmarshal(AZ::Vector3& value, ReadBuffer& rb)
        {
            AZ::u8 delta[3];
            rb.Read(delta[0]);
            rb.Read(delta[1]);
            rb.Read(delta[2]);

            value = AZ::Vector3(Helper::GetUnquantized<DeltaRange>(delta[0]), Helper::GetUnquantized<DeltaRange>(delta[1]), Helper::GetUnquantized<DeltaRange>(delta[2]));
        }
    };

    /**
     * \brief Delta compressed DataSet, stateless and cacheless. Stateless - because it does not keep per-player state of any kind.
     * Cacheless - because it does not keep a history of its values.
     * This approach requires only one extra copy of a field, because the field is split into two portions: absolute and relative portions.
     * The value is always the sum of two portions. We leverage existing DataSets to omit sending the larger absolute value, thus achieving compression.
     *
     * \tparam FieldType
     * \tparam DeltaRange
     * \tparam MarshalerType
     * \tparam DeltaMarshalerType
     */
    template<typename FieldType, AZ::u32 DeltaRange, typename MarshalerType = Marshaler<FieldType>, typename DeltaMarshalerType = DeltaMarshaller<DeltaRange, FieldType>>
    class DeltaCompressedDataSet
    {
    public:
        virtual ~DeltaCompressedDataSet() = default;

        template<class C, void (C::* FuncPtr)(const FieldType&, const TimeContext&)>
        class BindInterface;

        /**
            Constructs a DataSet.
        **/
        explicit DeltaCompressedDataSet(const char* debugName, const FieldType& value = FieldType())
            : m_absolutePortion(debugName, value)
            , m_relativePortion(debugName)
        {
            static_assert(DeltaRange > 0, "Delta range cannot be zero!");

            // We need to intercept changes to our two DataSets, in order to calculate the combined value and report back to Replica Chunk on our time.
            m_absolutePortion.SetDispatchOverride([this](const TimeContext& tc) {OnAbsolutePortionChanged(tc); });
            m_relativePortion.SetDispatchOverride([this](const TimeContext& tc) {OnRelativePortionChanged(tc); });
        }

        /**
            Modify the DataSet. Call this on the Master node to change the data,
            which will be propagated to all proxies.
        **/
        void Set(const FieldType& v)
        {
            m_combinedValue = v;

            if (Helper::DeltaHelper<FieldType>::IsWithinDelta(m_absolutePortion.Get(), v, DeltaRange))
            {
                // within bounds, so only the relative portion needs to be updated
                m_relativePortion.Set(v - m_absolutePortion.Get());
            }
            else
            {
                // relative out of range, reset absolute
                m_absolutePortion.Set(v);
                m_relativePortion.Set(static_cast<FieldType>(0));
            }
        }

        /**
            Returns the current value of the DataSet.
        **/
        const FieldType& Get() const
        {
            return m_combinedValue;
        }

    protected:
        virtual void OnAbsolutePortionChanged(const TimeContext& /*tc*/)
        {
            m_combinedValue = m_absolutePortion.Get() + m_relativePortion.Get();
        }

        virtual void OnRelativePortionChanged(const TimeContext& /*tc*/)
        {
            m_combinedValue = m_absolutePortion.Get() + m_relativePortion.Get();
        }

    private:
        DataSet<FieldType, MarshalerType> m_absolutePortion;
        DataSet<FieldType, DeltaMarshalerType> m_relativePortion;
        FieldType m_combinedValue; // the latest value on either master or proxy
    };

    //-----------------------------------------------------------------------------

    /**
        Declares a DeltaCompressedDataSet with an event handler that is called when the value is changed.
        Use BindInterface<Class, FuncPtr> to dispatch to a method on the ReplicaChunk's
        ReplicaChunkInterface event handler instance.
    **/
    template<typename FieldType, AZ::u32 DeltaRange, typename MarshalerType, typename DeltaMarshalerType>
    template<class C, void (C::* FuncPtr)(const FieldType&, const TimeContext&)>
    class DeltaCompressedDataSet<FieldType, DeltaRange, MarshalerType, DeltaMarshalerType>::BindInterface
        : public DeltaCompressedDataSet<FieldType, DeltaRange, MarshalerType, DeltaMarshalerType>
    {
    public:
        explicit BindInterface(const char* debugName) : DeltaCompressedDataSet(debugName) { }

    protected:
        void OnAbsolutePortionChanged(const GridMate::TimeContext& tc) override
        {
            DeltaCompressedDataSet::OnAbsolutePortionChanged(tc);

            m_lastUpdateTime = m_absolutePortion.GetLastUpdateTime();
            if (m_relativePortion.GetLastUpdateTime() < m_lastUpdateTime)
            {
                // relative portion wasn't updated, so its callback won't be invoked this tick, therefore we need to dispatch change event now
                DispatchChangedEvent(tc);
            }
        }

        void OnRelativePortionChanged(const GridMate::TimeContext& tc) override
        {
            DeltaCompressedDataSet::OnRelativePortionChanged(tc);

            m_lastUpdateTime = m_relativePortion.GetLastUpdateTime();
            // Assuming that relative portion DataSet is dispatched after absolute portion by construction in DeltaCompressedDataSet
            DispatchChangedEvent(tc);
        }

        void DispatchChangedEvent(const TimeContext& tc)
        {
            AZ_Assert(m_relativePortion.GetReplicaChunkBase(), "DataSets should be attached to replica chunks!");

            if (C* c = static_cast<C*>(m_relativePortion.GetReplicaChunkBase()->GetHandler()))
            {
                const TimeContext changeTime{ m_lastUpdateTime, m_lastUpdateTime - (tc.m_realTime - tc.m_localTime) };
                (*c.*FuncPtr)(Get(), changeTime);
            }
        }

    private:
        AZ::u32 m_lastUpdateTime = 0; // the latest update time among m_absolutePortion and m_relativePortion
    };
    //-----------------------------------------------------------------------------
} // namespace GridMate

#endif // GM_DELTACOMPRESSED_DATASET_H
