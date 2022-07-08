/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Math/Vector2.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/Quaternion.h>
#include <AzNetworking/Serialization/ISerializer.h>
#include <AzNetworking/Utilities/NetworkCommon.h>

namespace AzNetworking
{
    template <uint32_t NUM_ELEMENTS>
    struct QuantizedValuesHelper;

    template <>
    struct QuantizedValuesHelper<1>
    {
        using ValueType = float;
        using SimdType = AZ::Simd::Vec1;
        static ValueType FloatsToValue(SimdType::FloatType vector, const float* quantizedValues);
        static SimdType::FloatType ValueToSimd(const ValueType& value);
        static float SelectElement(const ValueType& value, int32_t index);
    };

    template <>
    struct QuantizedValuesHelper<2>
    {
        using ValueType = AZ::Vector2;
        using SimdType = AZ::Simd::Vec2;
        static ValueType FloatsToValue(SimdType::FloatType vector, const float* quantizedValues);
        static SimdType::FloatType ValueToSimd(const ValueType& value);
        static float SelectElement(const ValueType& value, int32_t index);
    };

    template <>
    struct QuantizedValuesHelper<3>
    {
        using ValueType = AZ::Vector3;
        using SimdType = AZ::Simd::Vec3;
        static ValueType FloatsToValue(SimdType::FloatType vector, const float* quantizedValues);
        static SimdType::FloatType ValueToSimd(const ValueType& value);
        static float SelectElement(const ValueType& value, int32_t index);
    };

    template <>
    struct QuantizedValuesHelper<4>
    {
        using ValueType = AZ::Quaternion;
        using SimdType = AZ::Simd::Vec4;
        static ValueType FloatsToValue(SimdType::FloatType vector, const float* quantizedValues);
        static SimdType::FloatType ValueToSimd(const ValueType& value);
        static float SelectElement(const ValueType& value, int32_t index);
    };

    template <AZStd::size_t BYTE_COUNT> struct MaxSerializeValue { };
    template <> struct MaxSerializeValue<4> { static constexpr AZStd::size_t Value = 0xFFFFFFFF - 1; };
    template <> struct MaxSerializeValue<3> { static constexpr AZStd::size_t Value = 0x00FFFFFF - 1; };
    template <> struct MaxSerializeValue<2> { static constexpr AZStd::size_t Value = 0x0000FFFF - 1; };
    template <> struct MaxSerializeValue<1> { static constexpr AZStd::size_t Value = 0x000000FF - 1; };

    template <AZStd::size_t NUM_ELEMENTS, AZStd::size_t NUM_BYTES, int32_t MIN_VALUE, int32_t MAX_VALUE>
    class QuantizedValues
    {
    public:

        using SelfType = QuantizedValues<NUM_ELEMENTS, NUM_BYTES, MIN_VALUE, MAX_VALUE>;
        using SimdTypef = typename QuantizedValuesHelper<NUM_ELEMENTS>::SimdType::FloatType;
        using SimdTypei = typename QuantizedValuesHelper<NUM_ELEMENTS>::SimdType::Int32Type;
        using ValueType = typename QuantizedValuesHelper<NUM_ELEMENTS>::ValueType;

        //! Default constructor.
        QuantizedValues();

        //! Copy construct from same type.
        //! @param value instance to construct from
        QuantizedValues(const SelfType& value);

        //! Copy construct from float.
        //! @param value vector value to construct from
        explicit QuantizedValues(const ValueType& value);

        //! Assignment from same type.
        //! @param rhs instance to assign from
        SelfType& operator =(const SelfType& rhs);

        //! Assignment from value value.
        //! @param rhs value to assign from
        SelfType& operator =(const ValueType& rhs);

        //! Const underlying type operator.
        //! @return underlying value
        operator ValueType() const;

        //! Sum operator.
        //! @param value second operand
        //! @return result of operation
        ValueType operator +(const ValueType& value) const;

        //! Difference operator.
        //! @param value second operand
        //! @return result of operation
        ValueType operator -(const ValueType& value) const;

        //! Product operator.
        //! @param value second operand
        //! @return result of operation
        ValueType operator *(const ValueType& value) const;

        //! Division operator.
        //! @param value second operand
        //! @return result of operation
        ValueType operator /(const ValueType& value) const;

        //! Sum operator.
        //! @param value second operand
        //! @return result of operation
        SelfType& operator +=(const ValueType& value);

        //! Difference operator.
        //! @param value second operand
        //! @return result of operation
        SelfType& operator -=(const ValueType& value);

        //! Product operator.
        //! @param value second operand
        //! @return result of operation
        SelfType& operator *=(const ValueType& value);

        //! Division operator.
        //! @param value second operand
        //! @return result of operation
        SelfType& operator /=(const ValueType& value);

        //! Equality operator.
        //! @param rhs base type value to compare against
        //! @return boolean true if this == rhs
        bool operator ==(const SelfType& rhs) const;

        //! Equality operator.
        //! @param rhs base type value to compare against
        //! @return boolean true if this == rhs
        bool operator ==(const ValueType& rhs) const;

        //! Inequality operator.
        //! @param rhs base type value to compare against
        //! @return boolean true if this != rhs
        bool operator !=(const SelfType& rhs) const;

        //! Inequality operator.
        //! @param rhs base type value to compare against
        //! @return boolean true if this != rhs
        bool operator !=(const ValueType& rhs) const;

        //! Retrieves the quantized integral value used during serialization of this QuantizedValues instance.
        //! @return the quantized integral value used during serialization of this QuantizedValues instance
        const uint32_t* GetQuantizedIntegralValues() const;

        //! Base serialize method for all serializable structures or classes to implement.
        //! @param serializer ISerializer instance to use for serialization
        //! @return boolean true for success, false for serialization failure
        bool Serialize(ISerializer& serializer);

    private:

        //! Helper method to convert and store an un-quantized value.
        //! @param value the input value to convert and store
        void Set(const ValueType& value);

        //! Takes a quantized integral value and stores the floating point representation.
        void DecodeQuantizedValues();

        AZ_PUSH_DISABLE_WARNING(4324, "-Wunknown-warning-option") // structure was padded due to alignment
        union
        {
            float m_quantizedValues[NUM_ELEMENTS];
            SimdTypef m_quantizedVector;
        };

        union
        {
            uint32_t m_serializeValues[NUM_ELEMENTS];
            SimdTypei m_serializeVector;
        };
        AZ_POP_DISABLE_WARNING

        template <AZStd::size_t NUM_ELEMENTS2, AZStd::size_t NUM_BYTES2, int32_t MIN_VALUE2, int32_t MAX_VALUE2>
        friend struct QuantizedValuesConversionHelper;
    };
}

#include <AzNetworking/Utilities/QuantizedValues.inl>
