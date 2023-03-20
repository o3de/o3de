/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

namespace AzNetworking
{
    inline QuantizedValuesHelper<1>::ValueType QuantizedValuesHelper<1>::FloatsToValue(SimdType::FloatType, const float* quantizedValues)
    {
        return quantizedValues[0];
    }

    inline typename QuantizedValuesHelper<1>::SimdType::FloatType QuantizedValuesHelper<1>::ValueToSimd(const ValueType& value)
    {
        return SimdType::Splat(value);
    }

    inline float QuantizedValuesHelper<1>::SelectElement(const ValueType& value, [[maybe_unused]] int32_t index)
    {
        return value;
    }

    inline typename QuantizedValuesHelper<2>::ValueType QuantizedValuesHelper<2>::FloatsToValue(SimdType::FloatType vector, [[maybe_unused]] const float* quantizedValues)
    {
        return ValueType(vector);
    }

    inline typename QuantizedValuesHelper<2>::SimdType::FloatType QuantizedValuesHelper<2>::ValueToSimd(const ValueType& value)
    {
        return value.GetSimdValue();
    }

    inline float QuantizedValuesHelper<2>::SelectElement(const ValueType& value, int32_t index)
    {
        return value.GetElement(index);
    }

    inline typename QuantizedValuesHelper<3>::ValueType QuantizedValuesHelper<3>::FloatsToValue(SimdType::FloatType vector, [[maybe_unused]] const float* quantizedValues)
    {
        return ValueType(vector);
    }

    inline typename QuantizedValuesHelper<3>::SimdType::FloatType QuantizedValuesHelper<3>::ValueToSimd(const ValueType& value)
    {
        return value.GetSimdValue();
    }

    inline float QuantizedValuesHelper<3>::SelectElement(const ValueType& value, int32_t index)
    {
        return value.GetElement(index);
    }

    inline typename QuantizedValuesHelper<4>::ValueType QuantizedValuesHelper<4>::FloatsToValue(SimdType::FloatType vector, [[maybe_unused]] const float* quantizedValues)
    {
        return ValueType(vector);
    }

    inline typename QuantizedValuesHelper<4>::SimdType::FloatType QuantizedValuesHelper<4>::ValueToSimd(const ValueType& value)
    {
        return value.GetSimdValue();
    }

    inline float QuantizedValuesHelper<4>::SelectElement(const ValueType& value, int32_t index)
    {
        return value.GetElement(index);
    }

    template <AZStd::size_t NUM_ELEMENTS, AZStd::size_t NUM_BYTES, int32_t MIN_VALUE, int32_t MAX_VALUE>
    inline QuantizedValues<NUM_ELEMENTS, NUM_BYTES, MIN_VALUE, MAX_VALUE>::QuantizedValues()
    {
        m_quantizedVector = QuantizedValuesHelper<NUM_ELEMENTS>::SimdType::ZeroFloat();
        m_serializeVector = QuantizedValuesHelper<NUM_ELEMENTS>::SimdType::ZeroInt();
    }

    template <AZStd::size_t NUM_ELEMENTS, AZStd::size_t NUM_BYTES, int32_t MIN_VALUE, int32_t MAX_VALUE>
    inline QuantizedValues<NUM_ELEMENTS, NUM_BYTES, MIN_VALUE, MAX_VALUE>::QuantizedValues(const SelfType& value)
    {
        m_quantizedVector = value.m_quantizedVector;
        m_serializeVector = value.m_serializeVector;
    }

    template <AZStd::size_t NUM_ELEMENTS, AZStd::size_t NUM_BYTES, int32_t MIN_VALUE, int32_t MAX_VALUE>
    inline QuantizedValues<NUM_ELEMENTS, NUM_BYTES, MIN_VALUE, MAX_VALUE>::QuantizedValues(const ValueType& value)
    {
        Set(value);
    }

    template <AZStd::size_t NUM_ELEMENTS, AZStd::size_t NUM_BYTES, int32_t MIN_VALUE, int32_t MAX_VALUE>
    inline QuantizedValues<NUM_ELEMENTS, NUM_BYTES, MIN_VALUE, MAX_VALUE>& QuantizedValues<NUM_ELEMENTS, NUM_BYTES, MIN_VALUE, MAX_VALUE>::operator =(const SelfType& rhs)
    {
        m_quantizedVector = rhs.m_quantizedVector;
        m_serializeVector = rhs.m_serializeVector;
        return *this;
    }

    template <AZStd::size_t NUM_ELEMENTS, AZStd::size_t NUM_BYTES, int32_t MIN_VALUE, int32_t MAX_VALUE>
    inline QuantizedValues<NUM_ELEMENTS, NUM_BYTES, MIN_VALUE, MAX_VALUE>& QuantizedValues<NUM_ELEMENTS, NUM_BYTES, MIN_VALUE, MAX_VALUE>::operator = (const ValueType& rhs)
    {
        Set(rhs);
        return *this;
    }

    template <AZStd::size_t NUM_ELEMENTS, AZStd::size_t NUM_BYTES, int32_t MIN_VALUE, int32_t MAX_VALUE>
    inline QuantizedValues<NUM_ELEMENTS, NUM_BYTES, MIN_VALUE, MAX_VALUE>::operator ValueType() const
    {
        return QuantizedValuesHelper<NUM_ELEMENTS>::FloatsToValue(m_quantizedVector, m_quantizedValues);
    }

    template <AZStd::size_t NUM_ELEMENTS, AZStd::size_t NUM_BYTES, int32_t MIN_VALUE, int32_t MAX_VALUE>
    inline typename QuantizedValues<NUM_ELEMENTS, NUM_BYTES, MIN_VALUE, MAX_VALUE>::ValueType QuantizedValues<NUM_ELEMENTS, NUM_BYTES, MIN_VALUE, MAX_VALUE>::operator +(const ValueType& value) const
    {
        return QuantizedValuesHelper<NUM_ELEMENTS>::FloatsToValue(m_quantizedVector, m_quantizedValues) + value;
    }

    template <AZStd::size_t NUM_ELEMENTS, AZStd::size_t NUM_BYTES, int32_t MIN_VALUE, int32_t MAX_VALUE>
    inline typename QuantizedValues<NUM_ELEMENTS, NUM_BYTES, MIN_VALUE, MAX_VALUE>::ValueType QuantizedValues<NUM_ELEMENTS, NUM_BYTES, MIN_VALUE, MAX_VALUE>::operator -(const ValueType& value) const
    {
        return QuantizedValuesHelper<NUM_ELEMENTS>::FloatsToValue(m_quantizedVector, m_quantizedValues) - value;
    }

    template <AZStd::size_t NUM_ELEMENTS, AZStd::size_t NUM_BYTES, int32_t MIN_VALUE, int32_t MAX_VALUE>
    inline typename QuantizedValues<NUM_ELEMENTS, NUM_BYTES, MIN_VALUE, MAX_VALUE>::ValueType QuantizedValues<NUM_ELEMENTS, NUM_BYTES, MIN_VALUE, MAX_VALUE>::operator *(const ValueType& value) const
    {
        return QuantizedValuesHelper<NUM_ELEMENTS>::FloatsToValue(m_quantizedVector, m_quantizedValues) * value;
    }

    template <AZStd::size_t NUM_ELEMENTS, AZStd::size_t NUM_BYTES, int32_t MIN_VALUE, int32_t MAX_VALUE>
    inline typename QuantizedValues<NUM_ELEMENTS, NUM_BYTES, MIN_VALUE, MAX_VALUE>::ValueType QuantizedValues<NUM_ELEMENTS, NUM_BYTES, MIN_VALUE, MAX_VALUE>::operator /(const ValueType& value) const
    {
        return QuantizedValuesHelper<NUM_ELEMENTS>::FloatsToValue(m_quantizedVector, m_quantizedValues) / value;
    }

    template <AZStd::size_t NUM_ELEMENTS, AZStd::size_t NUM_BYTES, int32_t MIN_VALUE, int32_t MAX_VALUE>
    inline QuantizedValues<NUM_ELEMENTS, NUM_BYTES, MIN_VALUE, MAX_VALUE>& QuantizedValues<NUM_ELEMENTS, NUM_BYTES, MIN_VALUE, MAX_VALUE>::operator +=(const ValueType& value)
    {
        Set(QuantizedValuesHelper<NUM_ELEMENTS>::FloatsToValue(m_quantizedVector, m_quantizedValues) + value);
        return *this;
    }

    template <AZStd::size_t NUM_ELEMENTS, AZStd::size_t NUM_BYTES, int32_t MIN_VALUE, int32_t MAX_VALUE>
    inline QuantizedValues<NUM_ELEMENTS, NUM_BYTES, MIN_VALUE, MAX_VALUE>& QuantizedValues<NUM_ELEMENTS, NUM_BYTES, MIN_VALUE, MAX_VALUE>::operator -=(const ValueType& value)
    {
        Set(QuantizedValuesHelper<NUM_ELEMENTS>::FloatsToValue(m_quantizedVector, m_quantizedValues) - value);
        return *this;
    }

    template <AZStd::size_t NUM_ELEMENTS, AZStd::size_t NUM_BYTES, int32_t MIN_VALUE, int32_t MAX_VALUE>
    inline QuantizedValues<NUM_ELEMENTS, NUM_BYTES, MIN_VALUE, MAX_VALUE>& QuantizedValues<NUM_ELEMENTS, NUM_BYTES, MIN_VALUE, MAX_VALUE>::operator *=(const ValueType& value)
    {
        Set(QuantizedValuesHelper<NUM_ELEMENTS>::FloatsToValue(m_quantizedVector, m_quantizedValues) * value);
        return *this;
    }

    template <AZStd::size_t NUM_ELEMENTS, AZStd::size_t NUM_BYTES, int32_t MIN_VALUE, int32_t MAX_VALUE>
    inline QuantizedValues<NUM_ELEMENTS, NUM_BYTES, MIN_VALUE, MAX_VALUE>& QuantizedValues<NUM_ELEMENTS, NUM_BYTES, MIN_VALUE, MAX_VALUE>::operator /=(const ValueType& value)
    {
        Set(QuantizedValuesHelper<NUM_ELEMENTS>::FloatsToValue(m_quantizedVector, m_quantizedValues) / value);
        return *this;
    }

    template <AZStd::size_t NUM_ELEMENTS, AZStd::size_t NUM_BYTES, int32_t MIN_VALUE, int32_t MAX_VALUE>
    inline bool QuantizedValues<NUM_ELEMENTS, NUM_BYTES, MIN_VALUE, MAX_VALUE>::operator ==(const SelfType& rhs) const
    {
        for (AZStd::size_t i = 0; i < NUM_ELEMENTS; ++i)
        {
            if (m_serializeValues[i] != rhs.m_serializeValues[i])
            {
                return false;
            }
        }
        return true;
    }

    template <AZStd::size_t NUM_ELEMENTS, AZStd::size_t NUM_BYTES, int32_t MIN_VALUE, int32_t MAX_VALUE>
    inline bool QuantizedValues<NUM_ELEMENTS, NUM_BYTES, MIN_VALUE, MAX_VALUE>::operator ==(const ValueType& rhs) const
    {
        SelfType selfType(rhs);
        return (*this == selfType);
    }

    template <AZStd::size_t NUM_ELEMENTS, AZStd::size_t NUM_BYTES, int32_t MIN_VALUE, int32_t MAX_VALUE>
    inline bool QuantizedValues<NUM_ELEMENTS, NUM_BYTES, MIN_VALUE, MAX_VALUE>::operator !=(const SelfType& rhs) const
    {
        for (AZStd::size_t i = 0; i < NUM_ELEMENTS; ++i)
        {
            if (m_serializeValues[i] != rhs.m_serializeValues[i])
            {
                return true;
            }
        }
        return false;
    }

    template <AZStd::size_t NUM_ELEMENTS, AZStd::size_t NUM_BYTES, int32_t MIN_VALUE, int32_t MAX_VALUE>
    inline bool QuantizedValues<NUM_ELEMENTS, NUM_BYTES, MIN_VALUE, MAX_VALUE>::operator !=(const ValueType& rhs) const
    {
        SelfType selfType(rhs);
        return (*this != selfType);
    }

    template <AZStd::size_t NUM_ELEMENTS, AZStd::size_t NUM_BYTES, int32_t MIN_VALUE, int32_t MAX_VALUE>
    inline const uint32_t* QuantizedValues<NUM_ELEMENTS, NUM_BYTES, MIN_VALUE, MAX_VALUE>::GetQuantizedIntegralValues() const
    {
        return m_serializeValues;
    }

    template <AZStd::size_t NUM_ELEMENTS, AZStd::size_t NUM_BYTES, int32_t MIN_VALUE, int32_t MAX_VALUE>
    inline bool QuantizedValues<NUM_ELEMENTS, NUM_BYTES, MIN_VALUE, MAX_VALUE>::Serialize(ISerializer& serializer)
    {
        using SerializeType = typename AZ::SizeType<NUM_BYTES, false>::Type;

        for (AZStd::size_t i = 0; i < NUM_ELEMENTS; ++i)
        {
            SerializeType serializedValue = static_cast<SerializeType>(m_serializeValues[i]);

            if constexpr (NUM_BYTES == 3)
            {
                uint8_t lowByte = static_cast<uint8_t>((serializedValue & 0x000000FF)      );
                uint8_t midByte = static_cast<uint8_t>((serializedValue & 0x0000FF00) >>  8);
                uint8_t hiByte  = static_cast<uint8_t>((serializedValue & 0x00FF0000) >> 16);
                serializer.Serialize(lowByte, GenerateIndexLabel<NUM_ELEMENTS>(i * 3 + 0).c_str());
                serializer.Serialize(midByte, GenerateIndexLabel<NUM_ELEMENTS>(i * 3 + 1).c_str());
                serializer.Serialize(hiByte,  GenerateIndexLabel<NUM_ELEMENTS>(i * 3 + 2).c_str());
                serializedValue = lowByte | (midByte << 8) | (hiByte << 16);
            }
            else
            {
                serializer.Serialize(serializedValue, GenerateIndexLabel<NUM_ELEMENTS>(i).c_str());
            }

            AZ_Assert((serializer.GetSerializerMode() == SerializerMode::WriteToObject) || (static_cast<SerializeType>(m_serializeValues[i]) == serializedValue),
                "If we're reading, the temporary serialized value must match the instance value");
            m_serializeValues[i] = serializedValue;
        }

        if ((serializer.GetSerializerMode() == SerializerMode::WriteToObject))
        {
            DecodeQuantizedValues();
        }

        return serializer.IsValid();
    }

    template <AZStd::size_t NUM_ELEMENTS, AZStd::size_t NUM_BYTES, int32_t MIN_VALUE, int32_t MAX_VALUE>
    struct QuantizedValuesConversionHelper
    {
        using SelfType  = QuantizedValues<NUM_ELEMENTS, NUM_BYTES, MIN_VALUE, MAX_VALUE>;
        using SimdTypef = typename QuantizedValuesHelper<NUM_ELEMENTS>::SimdType::FloatType;
        using SimdTypei = typename QuantizedValuesHelper<NUM_ELEMENTS>::SimdType::Int32Type;
        using ValueType = typename QuantizedValuesHelper<NUM_ELEMENTS>::ValueType;
        static constexpr uint32_t MaxSerializedIntValue = MaxSerializeValue<NUM_BYTES>::Value;

        static inline void Set(SelfType& quantizedValues, const ValueType& value)
        {
            using SimdType = typename QuantizedValuesHelper<NUM_ELEMENTS>::SimdType;
            const SimdTypef maximumInt = SimdType::Splat(static_cast<float>(MaxSerializedIntValue));
            const SimdTypef minimumFloat = SimdType::Splat(static_cast<float>(MIN_VALUE));
            const SimdTypef convertToInt = SimdType::Splat(static_cast<float>(MaxSerializedIntValue) / static_cast<float>(MAX_VALUE - MIN_VALUE));
            const SimdTypef floatData = QuantizedValuesHelper<NUM_ELEMENTS>::ValueToSimd(value);
            const SimdTypef readjusted = SimdType::Mul(convertToInt, SimdType::Sub(floatData, minimumFloat));
            quantizedValues.m_serializeVector = SimdType::ConvertToInt(SimdType::Clamp(readjusted, SimdType::ZeroFloat(), maximumInt));
        }

        static inline void DecodeQuantizedValues(SelfType& quantizedValues)
        {
            using SimdType = typename QuantizedValuesHelper<NUM_ELEMENTS>::SimdType;
            const SimdTypef minimumFloat = SimdType::Splat(static_cast<float>(MIN_VALUE));
            const SimdTypef convertToFloat = SimdType::Splat(static_cast<float>(MAX_VALUE - MIN_VALUE) / static_cast<float>(MaxSerializedIntValue));
            const SimdTypef quantized = SimdType::ConvertToFloat(quantizedValues.m_serializeVector); // Convert the integral value back into a float
            quantizedValues.m_quantizedVector = SimdType::Add(minimumFloat, SimdType::Mul(quantized, convertToFloat));
        }
    };

    template <AZStd::size_t NUM_ELEMENTS, int32_t MIN_VALUE, int32_t MAX_VALUE>
    struct QuantizedValuesConversionHelper<NUM_ELEMENTS, 4, MIN_VALUE, MAX_VALUE>
    {
        using SelfType  = QuantizedValues<NUM_ELEMENTS, 4, MIN_VALUE, MAX_VALUE>;
        using SimdTypef = typename QuantizedValuesHelper<NUM_ELEMENTS>::SimdType::FloatType;
        using SimdTypei = typename QuantizedValuesHelper<NUM_ELEMENTS>::SimdType::Int32Type;
        using ValueType = typename QuantizedValuesHelper<NUM_ELEMENTS>::ValueType;
        static constexpr uint32_t MaxSerializedIntValue = MaxSerializeValue<4>::Value;

        static inline void Set(SelfType& quantizedValues, const ValueType& value)
        {
            constexpr double maximumInt   = static_cast<double>(MaxSerializedIntValue);
            constexpr double minimumFloat = static_cast<double>(MIN_VALUE);
            constexpr double convertToInt = maximumInt / static_cast<double>(MAX_VALUE - MIN_VALUE);
            for (int32_t i = 0; i < static_cast<int32_t>(NUM_ELEMENTS); ++i)
            {
                const double readjusted = convertToInt * (QuantizedValuesHelper<NUM_ELEMENTS>::SelectElement(value, i) - minimumFloat);
                quantizedValues.m_serializeValues[i] = static_cast<uint32_t>(AZStd::clamp(readjusted, 0.0, maximumInt));
            }
        }

        static inline void DecodeQuantizedValues(SelfType& quantizedValues)
        {
            constexpr double maximumInt     = static_cast<double>(MaxSerializedIntValue);
            constexpr double minimumFloat   = static_cast<double>(MIN_VALUE);
            constexpr double convertToFloat = static_cast<double>(MAX_VALUE - MIN_VALUE) / maximumInt;
            for (int32_t i = 0; i < static_cast<int32_t>(NUM_ELEMENTS); ++i)
            {
                const double quantized = static_cast<double>(quantizedValues.m_serializeValues[i]);
                quantizedValues.m_quantizedValues[i] = static_cast<float>(minimumFloat + static_cast<float>(quantized * convertToFloat));
            }
        }
    };

    template <AZStd::size_t NUM_ELEMENTS, AZStd::size_t NUM_BYTES, int32_t MIN_VALUE, int32_t MAX_VALUE>
    inline void QuantizedValues<NUM_ELEMENTS, NUM_BYTES, MIN_VALUE, MAX_VALUE>::Set(const ValueType& value)
    {
        QuantizedValuesConversionHelper<NUM_ELEMENTS, NUM_BYTES, MIN_VALUE, MAX_VALUE>::Set(*this, value);
        DecodeQuantizedValues();
    }

    template <AZStd::size_t NUM_ELEMENTS, AZStd::size_t NUM_BYTES, int32_t MIN_VALUE, int32_t MAX_VALUE>
    inline void QuantizedValues<NUM_ELEMENTS, NUM_BYTES, MIN_VALUE, MAX_VALUE>::DecodeQuantizedValues()
    {
        QuantizedValuesConversionHelper<NUM_ELEMENTS, NUM_BYTES, MIN_VALUE, MAX_VALUE>::DecodeQuantizedValues(*this);
    }
}
