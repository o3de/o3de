/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/RTTI/RTTI.h>
#include <AzCore/RTTI/TypeInfo.h>
#include <AzCore/Memory/Memory.h>
#include <AzCore/std/containers/variant.h>
#include <AzCore/std/any.h>

#include <AzCore/std/string/string.h>
#include <AzCore/Math/Vector2.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/Vector4.h>
#include <AzCore/Math/Color.h>

#include <AzCore/Asset/AssetCommon.h>
#include <Atom/RPI.Reflect/Configuration.h>
#include <Atom/RPI.Reflect/Image/ImageAsset.h>
#include <Atom/RPI.Reflect/Image/StreamingImageAsset.h>
#include <Atom/RPI.Reflect/Image/Image.h>
#include <AtomCore/Instance/Instance.h>

namespace AZ
{
    namespace RPI
    {
        //! This is a variant data type that represents the value of a material property.
        //! For convenience, it supports all the types necessary for *both* the runtime data (MaterialAsset) as well as .material file data (MaterialSourceData).
        //! For example, Instance<Image> is exclusive to the runtime data and AZStd::string is primarily for image file paths in .material files. Most other
        //! data types are relevant in both contexts.
        class ATOM_RPI_REFLECT_API MaterialPropertyValue final
        {
        public:
            AZ_TYPE_INFO(AZ::RPI::MaterialPropertyValue, "{59815051-BBA2-4C6A-A414-A82834A84CB2}");
            static void Reflect(ReflectContext* context);

            //! Variant type definition.
            //! AZStd::monostate is used as default and invalid value.
            //! Specially, image type is stored in different type under different contexts:
            //!   Data::Instance<Image>   is used in Material at runtime
            //!   Data::Asset<ImageAsset> is used in MaterialTypeAsset, MaterialAsset
            //!   AZStd::string           is used in MaterialTypeSourceData, MaterialSourceData
            //! These are included in one MaterialPropertyValue type for convenience, rather than having to maintain three separate classes that are very similar
            using ValueType = AZStd::variant<AZStd::monostate, bool, int32_t, uint32_t, float, Vector2, Vector3, Vector4, Color, Data::Asset<ImageAsset>, Data::Instance<Image>, AZStd::string>;

            //! Two-way conversion to AZStd::any
            //! If the type in AZStd::any is not in ValueType, a warning will be reported and AZStd::monostate will be returned.
            static MaterialPropertyValue FromAny(const AZStd::any& value);
            static AZStd::any ToAny(const MaterialPropertyValue& value);

            //! Constructors to allow implicit conversions
            MaterialPropertyValue() = default;
            MaterialPropertyValue(const bool& value) : m_value(value) {}
            MaterialPropertyValue(const int32_t& value) : m_value(value) {}
            MaterialPropertyValue(const uint32_t& value) : m_value(value) {}
            MaterialPropertyValue(const float& value) : m_value(value) {}
            MaterialPropertyValue(const Vector2& value) : m_value(value) {}
            MaterialPropertyValue(const Vector3& value) : m_value(value) {}
            MaterialPropertyValue(const Vector4& value) : m_value(value) {}
            MaterialPropertyValue(const Color& value) : m_value(value) {}
            MaterialPropertyValue(const Data::Asset<ImageAsset>& value) : m_value(value) {}
            MaterialPropertyValue(const Data::Instance<Image>& value) : m_value(value) {}
            MaterialPropertyValue(const AZStd::string& value) : m_value(value) {}
            MaterialPropertyValue(const Name& value) : m_value(AZStd::string{value.GetStringView()}) {}

            //! Copy constructor
            MaterialPropertyValue(const MaterialPropertyValue& value) : m_value(value.m_value) {}

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
            bool Is() const
            {
                return AZStd::holds_alternative<T>(m_value);
            }

            //! Get TypeId of the type holding.
            TypeId GetTypeId() const;

            //! Check if the variant is holding a valid value.
            bool IsValid() const
            {
                return !AZStd::holds_alternative<AZStd::monostate>(m_value);
            }

            //! Default comparison.
            bool operator== (const MaterialPropertyValue& other) const
            {
                return m_value == other.m_value;
            }
            bool operator!= (const MaterialPropertyValue& other) const
            {
                return m_value != other.m_value;
            }

            //! Attempt to cast the value to another type, handling numerical types (e.g. int to
            //! float, bool to int), vector types (e.g. Vector2 to Vector3) and color<->vector types
            //! (e.g. Vector[3-4] to Color). In conversions between vector based types of different
            //! dimension, the result gets truncated or padded with zeroes as needed. Conversions
            //! between color and vector types are only supported for 3 and 4 dimensional vectors.
            //! In case of incompatible types (e.g. string to float, Vector2 to Color), the current
            //! object is returned as-is.
            MaterialPropertyValue CastToType(TypeId requestedType) const;

        private:

            ValueType m_value;
        };
    }
}
