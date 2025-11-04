/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "DataTypeUtils.h"

#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/Utils.h>
#include <ScriptCanvas/Data/DataType.h>

namespace ScriptCanvas
{
    namespace Data
    {
        Type FromAZType(const AZ::Uuid& type)
        {
            AZStd::pair<bool, Type> help = FromAZTypeHelper(type);
            return help.first ? help.second : Type::BehaviorContextObject(type);
        }

        Type FromAZTypeChecked(const AZ::Uuid& type)
        {
            AZStd::pair<bool, Type> help = FromAZTypeHelper(type);
            return help.first ?
                help.second : IsSupportedBehaviorContextObject(type) ?
                    Type::BehaviorContextObject(type) : Type::Invalid();
        }

        AZStd::pair<bool, Type> FromAZTypeHelper(const AZ::Uuid& type)
        {
            if (type.IsNull())
            {
                return { true, Type::Invalid() };
            }
            else if (IsAABB(type))
            {
                return { true, Type::AABB() };
            }
            else if (IsAssetId(type))
            {
                return { true, Type::AssetId() };
            }
            else if (IsBoolean(type))
            {
                return { true, Type::Boolean() };
            }
            else if (IsColor(type))
            {
                return { true, Type::Color() };
            }
            else if (IsCRC(type))
            {
                return { true, Type::CRC() };
            }
            else if (IsEntityID(type))
            {
                return { true, Type::EntityID() };
            }
            else if (IsNamedEntityID(type))
            {
                return { true, Type::NamedEntityID() };
            }
            else if (IsMatrix3x3(type))
            {
                return { true, Type::Matrix3x3() };
            }
            else if (IsMatrix4x4(type))
            {
                return { true, Type::Matrix4x4() };
            }
            else if (IsMatrixMxN(type))
            {
                return { true, Type::MatrixMxN() };
            }
            else if (IsNumber(type))
            {
                return { true, Type::Number() };
            }
            else if (IsOBB(type))
            {
                return { true, Type::OBB() };
            }
            else if (IsPlane(type))
            {
                return { true, Type::Plane() };
            }
            else if (IsQuaternion(type))
            {
                return { true, Type::Quaternion() };
            }
            else if (IsString(type))
            {
                return { true, Type::String() };
            }
            else if (IsTransform(type))
            {
                return { true, Type::Transform() };
            }
            else if (IsVector2(type))
            {
                return { true, Type::Vector2() };
            }
            else if (IsVector3(type))
            {
                return { true, Type::Vector3() };
            }
            else if (IsVector4(type))
            {
                return { true, Type::Vector4() };
            }
            else if (IsVectorN(type))
            {
                return { true, Type::VectorN() };
            }
            else
            {
                return { false, Type::Invalid() };
            }
        }

        AZ::Uuid ToAZType(eType type)
        {
            switch (type)
            {
            case eType::AABB:
                return azrtti_typeid<AABBType>();

            case eType::AssetId:
                return azrtti_typeid<AssetIdType>();

            case eType::Boolean:
                return azrtti_typeid<bool>();

            case eType::Color:
                return azrtti_typeid<ColorType>();

            case eType::CRC:
                return azrtti_typeid<CRCType>();

            case eType::EntityID:
                return azrtti_typeid<AZ::EntityId>();

            case eType::NamedEntityID:
                return azrtti_typeid<AZ::NamedEntityId>();

            case eType::Invalid:
                return AZ::Uuid::CreateNull();

            case eType::Matrix3x3:
                return azrtti_typeid<AZ::Matrix3x3>();

            case eType::Matrix4x4:
                return azrtti_typeid<AZ::Matrix4x4>();

            case eType::MatrixMxN:
                return azrtti_typeid<AZ::MatrixMxN>();

            case eType::Number:
                return azrtti_typeid<NumberType>();

            case eType::OBB:
                return azrtti_typeid<OBBType>();

            case eType::Plane:
                return azrtti_typeid<PlaneType>();

            case eType::Quaternion:
                return azrtti_typeid<QuaternionType>();

            case eType::String:
                return azrtti_typeid<StringType>();

            case eType::Transform:
                return azrtti_typeid<TransformType>();

            case eType::Vector2:
                return azrtti_typeid<AZ::Vector2>();

            case eType::Vector3:
                return azrtti_typeid<AZ::Vector3>();

            case eType::Vector4:
                return azrtti_typeid<AZ::Vector4>();

            case eType::VectorN:
                return azrtti_typeid<AZ::VectorN>();

            default:
                AZ_Assert(false, "Invalid type!");
                // check for behavior context support
                return AZ::Uuid::CreateNull();
            }
        }

        AZ::Uuid ToAZType(const Type& type)
        {
            eType typeEnum = type.GetType();
            if (typeEnum == eType::BehaviorContextObject)
            {
                return type.GetAZType();
            }

            return ToAZType(typeEnum);
        }

        bool IsAZRttiTypeOf(const AZ::Uuid& candidate, const AZ::Uuid& reference)
        {
            auto bcClass = AZ::BehaviorContextHelper::GetClass(candidate);
            return bcClass && bcClass->m_azRtti && bcClass->m_azRtti->IsTypeOf(reference);
        }

        bool IS_A(const Type& candidate, const Type& reference)
        {
            return candidate.IS_A(reference);
        }

        bool IS_EXACTLY_A(const Type& candidate, const Type& reference)
        {
            return candidate.IS_EXACTLY_A(reference);
        }

        bool IsConvertible(const Type& source, const AZ::Uuid& target)
        {
            return source.IsConvertibleTo(target);
        }

        bool IsConvertible(const Type& source, const Type& target)
        {
            return source.IsConvertibleTo(target);
        }

        bool IsAABB(const AZ::Uuid& type)
        {
            return type == azrtti_typeid<AABBType>();
        }

        bool IsAABB(const Type& type)
        {
            return type.GetType() == eType::AABB;
        }

        bool IsAssetId(const AZ::Uuid& type)
        {
            return type == azrtti_typeid<AssetIdType>();
        }

        bool IsAssetId(const Type& type)
        {
            return type.GetType() == eType::AssetId;
        }

        bool IsBoolean(const AZ::Uuid& type)
        {
            return type == azrtti_typeid<bool>();
        }

        bool IsBoolean(const Type& type)
        {
            return type.GetType() == eType::Boolean;
        }

        bool IsColor(const AZ::Uuid& type)
        {
            return type == azrtti_typeid<ColorType>();
        }

        bool IsColor(const Type& type)
        {
            return type.GetType() == eType::Color;
        }

        bool IsCRC(const AZ::Uuid& type)
        {
            return type == azrtti_typeid<CRCType>();
        }

        bool IsCRC(const Type& type)
        {
            return type.GetType() == eType::CRC;
        }

        bool IsEntityID(const AZ::Uuid& type)
        {
            return type == azrtti_typeid<AZ::EntityId>();
        }

        bool IsEntityID(const Type& type)
        {
            return type.GetType() == eType::EntityID;
        }

        bool IsNamedEntityID(const AZ::Uuid& type)
        {
            return type == azrtti_typeid<AZ::NamedEntityId>();
        }

        bool IsNamedEntityID(const Type& type)
        {
            return type.GetType() == eType::NamedEntityID;
        }

        bool IsMatrix3x3(const AZ::Uuid& type)
        {
            return type == azrtti_typeid<AZ::Matrix3x3>();
        }

        bool IsMatrix3x3(const Type& type)
        {
            return type.GetType() == eType::Matrix3x3;
        }

        bool IsMatrix4x4(const AZ::Uuid& type)
        {
            return type == azrtti_typeid<AZ::Matrix4x4>();
        }

        bool IsMatrix4x4(const Type& type)
        {
            return type.GetType() == eType::Matrix4x4;
        }

        bool IsMatrixMxN(const AZ::Uuid& type)
        {
            return type == azrtti_typeid<AZ::MatrixMxN>();
        }

        bool IsMatrixMxN(const Type& type)
        {
            return type.GetType() == eType::MatrixMxN;
        }

        bool IsNumber(const AZ::Uuid& type)
        {
            return
                type == azrtti_typeid<AZ::s8>() || type == azrtti_typeid<AZ::s16>() || type == azrtti_typeid<AZ::s32>() || type == azrtti_typeid<AZ::s64>() ||
                type == azrtti_typeid<AZ::u8>() || type == azrtti_typeid<AZ::u16>() || type == azrtti_typeid<AZ::u32>() || type == azrtti_typeid<AZ::u64>() ||
                type == azrtti_typeid<long>() || type == azrtti_typeid<unsigned long>() || type == azrtti_typeid<float>() || type == azrtti_typeid<double>();
        }

        bool IsNumber(const Type& type)
        {
            return type.GetType() == eType::Number;
        }

        bool IsOBB(const AZ::Uuid& type)
        {
            return type == azrtti_typeid<OBBType>();
        }

        bool IsOBB(const Type& type)
        {
            return type.GetType() == eType::OBB;
        }

        bool IsPlane(const AZ::Uuid& type)
        {
            return type == azrtti_typeid<PlaneType>();
        }

        bool IsPlane(const Type& type)
        {
            return type.GetType() == eType::Plane;
        }

        bool IsQuaternion(const AZ::Uuid& type)
        {
            return type == azrtti_typeid<QuaternionType>();
        }

        bool IsQuaternion(const Type& type)
        {
            return type.GetType() == eType::Quaternion;
        }

        bool IsString(const AZ::Uuid& type)
        {
            return type == azrtti_typeid<StringType>();
        }

        bool IsString(const Type& type)
        {
            return type.GetType() == eType::String;
        }

        bool IsTransform(const AZ::Uuid& type)
        {
            return type == azrtti_typeid<TransformType>();
        }

        bool IsTransform(const Type& type)
        {
            return type.GetType() == eType::Transform;
        }

        bool IsVector2(const AZ::Uuid& type)
        {
            return type == azrtti_typeid<AZ::Vector2>();
        }

        bool IsVector2(const Type& type)
        {
            return type.GetType() == eType::Vector2;
        }

        bool IsVector3(const AZ::Uuid& type)
        {
            return type == azrtti_typeid<AZ::Vector3>();
        }

        bool IsVector3(const Type& type)
        {
            return type.GetType() == eType::Vector3;
        }

        bool IsVector4(const AZ::Uuid& type)
        {
            return type == azrtti_typeid<AZ::Vector4>();
        }

        bool IsVector4(const Type& type)
        {
            return type.GetType() == eType::Vector4;
        }

        bool IsVectorN(const AZ::Uuid& type)
        {
            return type == azrtti_typeid<AZ::VectorN>();
        }

        bool IsVectorN(const Type& type)
        {
            return type.GetType() == eType::VectorN;
        }

        bool IsVectorType(const AZ::Uuid& type)
        {
            return type == azrtti_typeid<AZ::Vector3>() || type == azrtti_typeid<AZ::Vector2>() || type == azrtti_typeid<AZ::Vector4>() || type == azrtti_typeid<AZ::VectorN>();
        }

        bool IsVectorType(const Type& type)
        {
            static const AZ::u32 s_vectorTypes = {
                1 << static_cast<AZ::u32>(eType::Vector3) |
                1 << static_cast<AZ::u32>(eType::Vector2) |
                1 << static_cast<AZ::u32>(eType::Vector4) |
                1 << static_cast<AZ::u32>(eType::VectorN)
            };

            return ((1 << static_cast<AZ::u32>(type.GetType())) & s_vectorTypes) != 0;
        }

        bool IsAutoBoxedType(const Type& type)
        {
            static const AZ::u32 s_autoBoxedTypes = {
                1 << static_cast<AZ::u32>(eType::AABB) |
                1 << static_cast<AZ::u32>(eType::Color) |
                1 << static_cast<AZ::u32>(eType::CRC) |
                1 << static_cast<AZ::u32>(eType::Matrix3x3) |
                1 << static_cast<AZ::u32>(eType::Matrix4x4) |
                1 << static_cast<AZ::u32>(eType::OBB) |
                1 << static_cast<AZ::u32>(eType::Quaternion) |
                1 << static_cast<AZ::u32>(eType::Transform) |
                1 << static_cast<AZ::u32>(eType::Vector2) |
                1 << static_cast<AZ::u32>(eType::Vector3) |
                1 << static_cast<AZ::u32>(eType::Vector4)
            };

            return ((1 << static_cast<AZ::u32>(type.GetType())) & s_autoBoxedTypes) != 0;
        }

        bool IsValueType(const Type& type)
        {
            return type.GetType() != eType::BehaviorContextObject;
        }

        bool IsContainerType(const AZ::Uuid& type)
        {
            return AZ::Utils::IsContainerType(type);
        }

        bool IsContainerType(const Type& type)
        {
            return AZ::Utils::IsContainerType(ToAZType(type));
        }

        bool IsMapContainerType(const AZ::Uuid& type)
        {
            return AZ::Utils::IsMapContainerType(type);
        }

        bool IsMapContainerType(const Type& type)
        {
            return AZ::Utils::IsMapContainerType(ToAZType(type));
        }

        bool IsOutcomeType(const AZ::Uuid& type)
        {
            return AZ::Utils::IsOutcomeType(type);
        }

        bool IsOutcomeType(const Type& type)
        {
            return AZ::Utils::IsOutcomeType(ToAZType(type));
        }

        bool IsVectorContainerType(const AZ::Uuid& type)
        {
            return AZ::Utils::IsVectorContainerType(type);
        }

        bool IsVectorContainerType(const Type& type)
        {
            return AZ::Utils::IsVectorContainerType(ToAZType(type));
        }

        bool IsSetContainerType(const AZ::Uuid& type)
        {
            return AZ::Utils::IsSetContainerType(type);
        }

        bool IsSetContainerType(const Type& type)
        {
            return AZ::Utils::IsSetContainerType(ToAZType(type));
        }

        bool IsSupportedBehaviorContextObject(const AZ::Uuid& type)
        {
            return AZ::BehaviorContextHelper::GetClass(type) != nullptr;
        }
    } // namespace Data
} // namespace ScriptCanvas
