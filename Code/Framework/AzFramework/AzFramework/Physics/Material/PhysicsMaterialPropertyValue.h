/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/RTTI/TypeInfoSimple.h>
#include <AzCore/std/any.h>
#include <AzCore/std/containers/variant.h>
#include <AzCore/Math/Color.h>
#include <AzCore/Math/Vector2.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/Vector4.h>
#include <AzCore/std/string/string.h>

namespace Physics
{
    //! This is a variant data type that represents the value of a physics material property.
    //! Used by Physics material assets.
    class MaterialPropertyValue final
    {
    public:
        AZ_TYPE_INFO(Physics::MaterialPropertyValue, "{2D9822B6-B4AD-4635-87FD-08F857EEB152}");

        static void Reflect(AZ::ReflectContext* context);

        //! Variant type definition.
        //! AZStd::monostate is used as default and invalid value.
        using ValueType = AZStd::variant<
            AZStd::monostate,
            bool,
            AZ::s32,
            AZ::u32,
            float,
            AZ::Vector2,
            AZ::Vector3,
            AZ::Vector4,
            AZ::Color,
            AZStd::string>;

        // Two-way conversion to AZStd::any
        // If the type in AZStd::any is not in ValueType, a warning will be reported and AZStd::monostate will be returned.
        static MaterialPropertyValue FromAny(const AZStd::any& value);
        static AZStd::any ToAny(const MaterialPropertyValue& value);

        // Constructors to allow implicit conversions
        MaterialPropertyValue() = default;
        MaterialPropertyValue(bool value)
            : m_value(value)
        {
        }
        MaterialPropertyValue(AZ::s32 value)
            : m_value(value)
        {
        }
        MaterialPropertyValue(AZ::u32 value)
            : m_value(value)
        {
        }
        MaterialPropertyValue(float value)
            : m_value(value)
        {
        }
        MaterialPropertyValue(const AZ::Vector2& value)
            : m_value(value)
        {
        }
        MaterialPropertyValue(const AZ::Vector3& value)
            : m_value(value)
        {
        }
        MaterialPropertyValue(const AZ::Vector4& value)
            : m_value(value)
        {
        }
        MaterialPropertyValue(const AZ::Color& value)
            : m_value(value)
        {
        }
        MaterialPropertyValue(const AZStd::string& value)
            : m_value(value)
        {
        }

        //! Copy constructor
        MaterialPropertyValue(const MaterialPropertyValue& value)
            : m_value(value.m_value)
        {
        }

        //! Templated assignment.
        //! The type will be restricted to those defined in the variant at compile time.
        //! If out-of-definition type is used, the compiler will report error.
        template<typename T>
        MaterialPropertyValue& operator=(const T& value)
        {
            m_value = value;
            return *this;
        }

        //! Get actual value from the variant.
        //! The type will be restricted as in operator=().
        template<typename T>
        const T& GetValue() const
        {
            return AZStd::get<T>(m_value);
        }

        //! Check if the type holding is T.
        template<typename T>
        constexpr bool Is() const
        {
            return AZStd::holds_alternative<T>(m_value);
        }

        //! Get TypeId of the type holding.
        AZ::TypeId GetTypeId() const;

        //! Check if the variant is holding a valid value.
        constexpr bool IsValid() const
        {
            return !Is<AZStd::monostate>();
        }

        // Default comparison.
        bool operator==(const MaterialPropertyValue& other) const
        {
            return m_value == other.m_value;
        }
        bool operator!=(const MaterialPropertyValue& other) const
        {
            return !(*this == other);
        }

        //! Attempt to cast the value to another type, handling numerical types (e.g. int to
        //! float, bool to int), vector types (e.g. Vector2 to Vector3) and color<->vector types
        //! (e.g. Vector[3-4] to Color). In conversions between vector based types of different
        //! dimension, the result gets truncated or padded with zeroes as needed. Conversions
        //! between color and vector types are only supported for 3 and 4 dimensional vectors.
        //! In case of incompatible types (e.g. string to float, Vector2 to Color), the current
        //! object is returned as-is.
        MaterialPropertyValue CastToType(AZ::TypeId requestedType) const;

    private:
        ValueType m_value;
    };
} // namespace Physics
