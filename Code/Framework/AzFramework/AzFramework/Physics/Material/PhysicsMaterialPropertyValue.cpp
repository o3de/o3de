/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/typetraits/is_same.h>

#include <AzFramework/Physics/Material/PhysicsMaterialPropertyValue.h>

namespace Physics
{
    static_assert(
        (AZStd::is_same_v<AZStd::monostate, AZStd::variant_alternative_t<0, MaterialPropertyValue::ValueType>> &&
         AZStd::is_same_v<bool, AZStd::variant_alternative_t<1, MaterialPropertyValue::ValueType>> &&
         AZStd::is_same_v<AZ::s32, AZStd::variant_alternative_t<2, MaterialPropertyValue::ValueType>> &&
         AZStd::is_same_v<AZ::u32, AZStd::variant_alternative_t<3, MaterialPropertyValue::ValueType>> &&
         AZStd::is_same_v<float, AZStd::variant_alternative_t<4, MaterialPropertyValue::ValueType>> &&
         AZStd::is_same_v<AZ::Vector2, AZStd::variant_alternative_t<5, MaterialPropertyValue::ValueType>> &&
         AZStd::is_same_v<AZ::Vector3, AZStd::variant_alternative_t<6, MaterialPropertyValue::ValueType>> &&
         AZStd::is_same_v<AZ::Vector4, AZStd::variant_alternative_t<7, MaterialPropertyValue::ValueType>> &&
         AZStd::is_same_v<AZ::Color, AZStd::variant_alternative_t<8, MaterialPropertyValue::ValueType>> &&
         AZStd::is_same_v<AZStd::string, AZStd::variant_alternative_t<9, MaterialPropertyValue::ValueType>>),
        "Types must be in the order of the type ID array.");

    void MaterialPropertyValue::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->RegisterGenericType<Physics::MaterialPropertyValue::ValueType>();

            serializeContext->Class<Physics::MaterialPropertyValue>()
                ->Version(1)
                ->Field("Value", &MaterialPropertyValue::m_value)
                ;
        }
    }

    MaterialPropertyValue MaterialPropertyValue::FromAny(const AZStd::any& value)
    {
        MaterialPropertyValue result;

        if (value.empty())
        {
            return result;
        }

        if (value.is<bool>())
        {
            result.m_value = AZStd::any_cast<bool>(value);
        }
        else if (value.is<AZ::s32>())
        {
            result.m_value = AZStd::any_cast<AZ::s32>(value);
        }
        else if (value.is<AZ::u32>())
        {
            result.m_value = AZStd::any_cast<AZ::u32>(value);
        }
        else if (value.is<float>())
        {
            result.m_value = AZStd::any_cast<float>(value);
        }
        else if (value.is<double>())
        {
            result.m_value = aznumeric_cast<float>(AZStd::any_cast<double>(value));
        }
        else if (value.is<AZ::Vector2>())
        {
            result.m_value = AZStd::any_cast<AZ::Vector2>(value);
        }
        else if (value.is<AZ::Vector3>())
        {
            result.m_value = AZStd::any_cast<AZ::Vector3>(value);
        }
        else if (value.is<AZ::Vector4>())
        {
            result.m_value = AZStd::any_cast<AZ::Vector4>(value);
        }
        else if (value.is<AZ::Color>())
        {
            result.m_value = AZStd::any_cast<AZ::Color>(value);
        }
        else if (value.is<AZStd::string>())
        {
            result.m_value = AZStd::any_cast<AZStd::string>(value);
        }
        else
        {
            AZ_Warning(
                "MaterialPropertyValue", false, "Cannot convert any to variant. Type in any is: %s.",
                value.get_type_info().m_id.ToString<AZStd::string>().c_str());
        }

        return result;
    }

    AZStd::any MaterialPropertyValue::ToAny(const MaterialPropertyValue& value)
    {
        AZStd::any result;

        if (value.Is<bool>())
        {
            result = value.GetValue<bool>();
        }
        else if (value.Is<AZ::s32>())
        {
            result = value.GetValue<AZ::s32>();
        }
        else if (value.Is<AZ::u32>())
        {
            result = value.GetValue<AZ::u32>();
        }
        else if (value.Is<float>())
        {
            result = value.GetValue<float>();
        }
        else if (value.Is<AZ::Vector2>())
        {
            result = value.GetValue<AZ::Vector2>();
        }
        else if (value.Is<AZ::Vector3>())
        {
            result = value.GetValue<AZ::Vector3>();
        }
        else if (value.Is<AZ::Vector4>())
        {
            result = value.GetValue<AZ::Vector4>();
        }
        else if (value.Is<AZ::Color>())
        {
            result = value.GetValue<AZ::Color>();
        }
        else if (value.Is<AZStd::string>())
        {
            result = value.GetValue<AZStd::string>();
        }
        else
        {
            AZ_Assert(false, "Unexpected type in variant: %s.",
                value.GetTypeId().ToString<AZStd::string>().c_str());
        }

        return result;
    }

    AZ::TypeId MaterialPropertyValue::GetTypeId() const
    {
        // Must sort in the same order defined in the variant.
        static const AZ::TypeId PropertyValueTypeIds[AZStd::variant_size<ValueType>::value] = {
            azrtti_typeid<AZStd::monostate>(),
            azrtti_typeid<bool>(),
            azrtti_typeid<AZ::s32>(),
            azrtti_typeid<AZ::u32>(),
            azrtti_typeid<float>(),
            azrtti_typeid<AZ::Vector2>(),
            azrtti_typeid<AZ::Vector3>(),
            azrtti_typeid<AZ::Vector4>(),
            azrtti_typeid<AZ::Color>(),
            azrtti_typeid<AZStd::string>()
        };

        return PropertyValueTypeIds[m_value.index()];
    }

    //! Attempts to convert a numeric MaterialPropertyValue to another numeric type @T.
    //! If the original MaterialPropertyValue is not a numeric type, the original value is returned.
    template<typename T>
    static MaterialPropertyValue CastNumericMaterialPropertyValue(const MaterialPropertyValue& value)
    {
        const AZ::TypeId typeId = value.GetTypeId();

        if (typeId == azrtti_typeid<bool>())
        {
            return aznumeric_cast<T>(value.GetValue<bool>());
        }
        else if (typeId == azrtti_typeid<AZ::s32>())
        {
            return aznumeric_cast<T>(value.GetValue<AZ::s32>());
        }
        else if (typeId == azrtti_typeid<AZ::u32>())
        {
            return aznumeric_cast<T>(value.GetValue<AZ::u32>());
        }
        else if (typeId == azrtti_typeid<float>())
        {
            return aznumeric_cast<T>(value.GetValue<float>());
        }
        else
        {
            return value;
        }
    }

    //! Attempts to convert an AZ::Vector[2-4] MaterialPropertyValue to another AZ::Vector[2-4] type @T.
    //! Any extra elements will be dropped or set to 0.0 as needed.
    //! If the original MaterialPropertyValue is not a Vector type, the original value is returned.
    template<typename VectorT>
    static MaterialPropertyValue CastVectorMaterialPropertyValue(const MaterialPropertyValue& value)
    {
        float values[4] = {};

        AZ::TypeId typeId = value.GetTypeId();
        if (typeId == azrtti_typeid<AZ::Vector2>())
        {
            value.GetValue<AZ::Vector2>().StoreToFloat2(values);
        }
        else if (typeId == azrtti_typeid<AZ::Vector3>())
        {
            value.GetValue<AZ::Vector3>().StoreToFloat3(values);
        }
        else if (typeId == azrtti_typeid<AZ::Vector4>())
        {
            value.GetValue<AZ::Vector4>().StoreToFloat4(values);
        }
        else
        {
            return value;
        }

        typeId = azrtti_typeid<VectorT>();
        if (typeId == azrtti_typeid<AZ::Vector2>())
        {
            return AZ::Vector2::CreateFromFloat2(values);
        }
        else if (typeId == azrtti_typeid<AZ::Vector3>())
        {
            return AZ::Vector3::CreateFromFloat3(values);
        }
        else if (typeId == azrtti_typeid<AZ::Vector4>())
        {
            return AZ::Vector4::CreateFromFloat4(values);
        }
        else
        {
            return value;
        }
    }

    MaterialPropertyValue MaterialPropertyValue::CastToType(AZ::TypeId requestedType) const
    {
        if (requestedType == azrtti_typeid<bool>())
        {
            return CastNumericMaterialPropertyValue<bool>(*this);
        }
        else if (requestedType == azrtti_typeid<AZ::s32>())
        {
            return CastNumericMaterialPropertyValue<AZ::s32>(*this);
        }
        else if (requestedType == azrtti_typeid<AZ::u32>())
        {
            return CastNumericMaterialPropertyValue<AZ::u32>(*this);
        }
        else if (requestedType == azrtti_typeid<float>())
        {
            return CastNumericMaterialPropertyValue<float>(*this);
        }
        else if (requestedType == azrtti_typeid<AZ::Vector2>())
        {
            return CastVectorMaterialPropertyValue<AZ::Vector2>(*this);
        }
        else if (requestedType == azrtti_typeid<AZ::Vector3>())
        {
            if (GetTypeId() == azrtti_typeid<AZ::Color>())
            {
                return GetValue<AZ::Color>().GetAsVector3();
            }
            else
            {
                return CastVectorMaterialPropertyValue<AZ::Vector3>(*this);
            }
        }
        else if (requestedType == azrtti_typeid<AZ::Vector4>())
        {
            if (GetTypeId() == azrtti_typeid<AZ::Color>())
            {
                return GetValue<AZ::Color>().GetAsVector4();
            }
            else
            {
                return CastVectorMaterialPropertyValue<AZ::Vector4>(*this);
            }
        }
        else if (requestedType == azrtti_typeid<AZ::Color>())
        {
            if (GetTypeId() == azrtti_typeid<AZ::Vector3>())
            {
                return AZ::Color::CreateFromVector3(GetValue<AZ::Vector3>());
            }
            else if (GetTypeId() == azrtti_typeid<AZ::Vector4>())
            {
                const AZ::Vector4& vector4 = GetValue<AZ::Vector4>();
                return AZ::Color::CreateFromVector3AndFloat(vector4.GetAsVector3(), vector4.GetW());
            }
            else
            {
                // Don't attempt conversion from e.g. Vector2 as that makes little sense.
                return *this;
            }
        }
        else
        {
            // remaining types are non-numerical and cannot be cast to other types: return as-is
            return *this;
        }
    }
} // namespace Physics
