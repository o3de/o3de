/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Component/EntityId.h>
#include <AzCore/Math/Aabb.h>
#include <AzCore/Math/Color.h>
#include <AzCore/Math/Crc.h>
#include <AzCore/Math/Matrix3x3.h>
#include <AzCore/Math/Matrix4x4.h>
#include <AzCore/Math/Obb.h>
#include <AzCore/Math/Plane.h>
#include <AzCore/Math/Quaternion.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/Math/Uuid.h>
#include <AzCore/Math/Vector2.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/Vector4.h>
#include <AzCore/RTTI/TypeInfo.h>
#include <AzCore/std/string/string.h>
#include <ScriptCanvas/Core/Core.h>

namespace AZ
{
    class ReflectContext;
}

namespace ScriptCanvas
{
    namespace Data
    {
        //////////////////////////////////////////////////////////////////////////
        // type interface
        //////////////////////////////////////////////////////////////////////////

        /// \note CHANGING THE ORDER OR NUMBER OF VALUES IN THIS LIST ALMOST CERTAINLY INVALIDATES PREVIOUS DATA
        enum class eType : AZ::u32
        {
            Boolean,
            EntityID,
            Invalid,
            Number,
            BehaviorContextObject,
            String,
            Quaternion,
            Transform,
            Vector3,
            Vector2,
            Vector4,
            AABB,
            Color,
            CRC,
            Matrix3x3,
            Matrix4x4,
            OBB,
            Plane,
            NamedEntityID,
            // Function, 
            // List,
            AssetId,
        };

        using AABBType = AZ::Aabb;
        using AssetIdType = AZ::Data::AssetId;
        using BooleanType = bool;
        using CRCType = AZ::Crc32;
        using ColorType = AZ::Color;
        using EntityIDType = AZ::EntityId;
        using NamedEntityIDType = AZ::NamedEntityId;
        using Matrix3x3Type = AZ::Matrix3x3;
        using Matrix4x4Type = AZ::Matrix4x4;
        using NumberType = double;
        using OBBType = AZ::Obb;
        using PlaneType = AZ::Plane;
        using QuaternionType = AZ::Quaternion;
        using StringType = AZStd::string;
        using TransformType = AZ::Transform;
        using Vector2Type = AZ::Vector2;
        using Vector3Type = AZ::Vector3;
        using Vector4Type = AZ::Vector4;

        class Type final
        {
        public:
            AZ_TYPE_INFO(Type, "{0EADF8F5-8AB8-42E9-9C50-F5C78255C817}");

            static void Reflect(AZ::ReflectContext* reflection);

            static Type AABB();
            static Type AssetId();
            static Type BehaviorContextObject(const AZ::Uuid& aztype);
            static Type Boolean();
            static Type Color();
            static Type CRC();
            static Type EntityID();
            static Type NamedEntityID();
            static Type Invalid();
            static Type Matrix3x3();
            static Type Matrix4x4();
            static Type Number();
            static Type OBB();
            static Type Plane();
            static Type Quaternion();
            static Type String();
            static Type Transform();
            static Type Vector2();
            static Type Vector3();
            static Type Vector4();

            // the default ctr produces the invalid type, and is only here to help the compiler
            Type();

            AZ::Uuid GetAZType() const;
            eType GetType() const;
            bool IsValid() const;
            explicit operator bool() const;
            bool operator!() const;

            bool operator==(const Type& other) const;
            bool operator!=(const Type& other) const;

            // returns true if this type is, or is derived from the other type
            AZ_FORCE_INLINE bool IS_A(const Type& other) const;
            AZ_FORCE_INLINE bool IS_EXACTLY_A(const Type& other) const;

            AZ_FORCE_INLINE bool IsConvertibleFrom(const AZ::Uuid& target) const;
            AZ_FORCE_INLINE bool IsConvertibleFrom(const Type& target) const;

            AZ_FORCE_INLINE bool IsConvertibleTo(const AZ::Uuid& target) const;
            AZ_FORCE_INLINE bool IsConvertibleTo(const Type& target) const;

            Type& operator=(const Type& source) = default;

        private:
            eType m_type;
            AZ::Uuid m_azType;

            explicit Type(eType type);
            // for BehaviorContextObjects specifically
            explicit Type(const AZ::Uuid& aztype);
        };

        /**
         * assumes that azType is a valid script canvas type of some kind, asserts if not
         * favors native ScriptCanvas types over BehaviorContext class types with the corresponding AZ type id
         */
        Type FromAZType(const AZ::Uuid& aztype);

        // if azType is not a valid script canvas type of some kind, returns invalid, NOT for use at run-time
        Type FromAZTypeChecked(const AZ::Uuid& aztype);

        template<typename T>
        Type FromAZType();

        // returns the most raw name for the type
        const char* GetBehaviorContextName(const AZ::Uuid& azType);
        const char* GetBehaviorContextName(const Type& type);
        // returns a possibly prettier name for the type
        AZStd::string GetBehaviorClassName(const AZ::Uuid& typeID);
        // returns a possibly prettier name for the type
        AZStd::string GetName(const Type& type);
        const Type GetBehaviorParameterDataType(const AZ::BehaviorParameter& parameter);

        bool IsAZRttiTypeOf(const AZ::Uuid& candidate, const AZ::Uuid& reference);

        // returns true if candidate is, or is derived from reference
        bool IS_A(const Type& candidate, const Type& reference);
        bool IS_EXACTLY_A(const Type& candidate, const Type& reference);

        bool IsConvertible(const Type& source, const AZ::Uuid& target);
        bool IsConvertible(const Type& source, const Type& target);

        bool IsAutoBoxedType(const Type& type);
        bool IsValueType(const Type& type);
        bool IsVectorType(const AZ::Uuid& type);
        bool IsVectorType(const Type& type);
        AZ::Uuid ToAZType(const Type& type);

        bool IsContainerType(const AZ::Uuid& type);
        bool IsContainerType(const Type& type);
        bool IsMapContainerType(const AZ::Uuid& type);
        bool IsMapContainerType(const Type& type);
        bool IsOutcomeType(const AZ::Uuid& type);
        bool IsOutcomeType(const Type& type);
        bool IsSetContainerType(const AZ::Uuid& type);
        bool IsSetContainerType(const Type& type);
        bool IsVectorContainerType(const AZ::Uuid& type);
        bool IsVectorContainerType(const Type& type);

        AZStd::vector<AZ::Uuid> GetContainedTypes(const AZ::Uuid& type);
        AZStd::vector<Type> GetContainedTypes(const Type& type);
        AZStd::pair<AZ::Uuid, AZ::Uuid> GetOutcomeTypes(const AZ::Uuid& type);
        AZStd::pair<Type, Type> GetOutcomeTypes(const Type& type);

        bool IsAABB(const AZ::Uuid& type);
        bool IsAABB(const Type& type);
        bool IsAssetId(const AZ::Uuid& type);
        bool IsAssetId(const Type& type);
        bool IsBoolean(const AZ::Uuid& type);
        bool IsBoolean(const Type& type);
        bool IsColor(const AZ::Uuid& type);
        bool IsColor(const Type& type);
        bool IsCRC(const AZ::Uuid& type);
        bool IsCRC(const Type& type);
        bool IsEntityID(const AZ::Uuid& type);
        bool IsEntityID(const Type& type);
        bool IsNamedEntityID(const AZ::Uuid& type);
        bool IsNamedEntityID(const Type& type);
        bool IsNumber(const AZ::Uuid& type);
        bool IsNumber(const Type& type);
        bool IsMatrix3x3(const AZ::Uuid& type);
        bool IsMatrix3x3(const Type& type);
        bool IsMatrix4x4(const AZ::Uuid& type);
        bool IsMatrix4x4(const Type& type);
        bool IsOBB(const AZ::Uuid& type);
        bool IsOBB(const Type& type);
        bool IsPlane(const AZ::Uuid& type);
        bool IsPlane(const Type& type);
        bool IsQuaternion(const AZ::Uuid& type);
        bool IsQuaternion(const Type& type);
        bool IsString(const AZ::Uuid& type);
        bool IsString(const Type& type);
        bool IsTransform(const AZ::Uuid& type);
        bool IsTransform(const Type& type);
        bool IsVector2(const AZ::Uuid& type);
        bool IsVector2(const Type& type);
        bool IsVector3(const AZ::Uuid& type);
        bool IsVector3(const Type& type);
        bool IsVector4(const AZ::Uuid& type);
        bool IsVector4(const Type& type);

        //////////////////////////////////////////////////////////////////////////
        // type implementation
        //////////////////////////////////////////////////////////////////////////
        template<typename T>
        AZ_INLINE Type FromAZType()
        {
            return FromAZType(azrtti_typeid<T>());
        }

        AZ_INLINE bool IsAABB(const AZ::Uuid& type)
        {
            return type == azrtti_typeid<AABBType>();
        }

        AZ_INLINE bool IsAABB(const Type& type)
        {
            return type.GetType() == eType::AABB;
        }

        AZ_INLINE bool IsAssetId(const AZ::Uuid& type)
        {
            return type == azrtti_typeid<AssetIdType>();
        }

        AZ_INLINE bool IsAssetId(const Type& type)
        {
            return type.GetType() == eType::AssetId;
        }

        AZ_INLINE bool IsBoolean(const AZ::Uuid& type)
        {
            return type == azrtti_typeid<bool>();
        }

        AZ_INLINE bool IsBoolean(const Type& type)
        {
            return type.GetType() == eType::Boolean;
        }

        AZ_INLINE bool IsColor(const AZ::Uuid& type)
        {
            return type == azrtti_typeid<ColorType>();
        }

        AZ_INLINE bool IsColor(const Type& type)
        {
            return type.GetType() == eType::Color;
        }

        AZ_INLINE bool IsCRC(const AZ::Uuid& type)
        {
            return type == azrtti_typeid<CRCType>();
        }

        AZ_INLINE bool IsCRC(const Type& type)
        {
            return type.GetType() == eType::CRC;
        }

        AZ_INLINE bool IsEntityID(const AZ::Uuid& type)
        {
            return type == azrtti_typeid<AZ::EntityId>();
        }

        AZ_INLINE bool IsEntityID(const Type& type)
        {
            return type.GetType() == eType::EntityID;
        }

        AZ_INLINE bool IsNamedEntityID(const AZ::Uuid& type)
        {
            return type == azrtti_typeid<AZ::NamedEntityId>();
        }

        AZ_INLINE bool IsNamedEntityID(const Type& type)
        {
            return type.GetType() == eType::NamedEntityID;
        }

        AZ_INLINE bool IsMatrix3x3(const AZ::Uuid& type)
        {
            return type == azrtti_typeid<AZ::Matrix3x3>();
        }

        AZ_INLINE bool IsMatrix3x3(const Type& type)
        {
            return type.GetType() == eType::Matrix3x3;
        }

        AZ_INLINE bool IsMatrix4x4(const AZ::Uuid& type)
        {
            return type == azrtti_typeid<AZ::Matrix4x4>();
        }

        AZ_INLINE bool IsMatrix4x4(const Type& type)
        {
            return type.GetType() == eType::Matrix4x4;
        }

        AZ_INLINE bool IsNumber(const AZ::Uuid& type)
        {
            return type == azrtti_typeid<AZ::s8>()
                || type == azrtti_typeid<AZ::s16>()
                || type == azrtti_typeid<AZ::s32>()
                || type == azrtti_typeid<AZ::s64>()
                || type == azrtti_typeid<AZ::u8>()
                || type == azrtti_typeid<AZ::u16>()
                || type == azrtti_typeid<AZ::u32>()
                || type == azrtti_typeid<AZ::u64>()
                || type == azrtti_typeid<unsigned long>()
                || type == azrtti_typeid<float>()
                || type == azrtti_typeid<double>();
        }

        AZ_INLINE bool IsNumber(const Type& type)
        {
            return type.GetType() == eType::Number;
        }

        AZ_INLINE bool IsOBB(const AZ::Uuid& type)
        {
            return type == azrtti_typeid<OBBType>();
        }

        AZ_INLINE bool IsOBB(const Type& type)
        {
            return type.GetType() == eType::OBB;
        }

        AZ_INLINE bool IsPlane(const AZ::Uuid& type)
        {
            return type == azrtti_typeid<PlaneType>();
        }

        AZ_INLINE bool IsPlane(const Type& type)
        {
            return type.GetType() == eType::Plane;
        }

        AZ_INLINE bool IsQuaternion(const AZ::Uuid& type)
        {
            return type == azrtti_typeid<QuaternionType>();
        }

        AZ_INLINE bool IsQuaternion(const Type& type)
        {
            return type.GetType() == eType::Quaternion;
        }

        AZ_INLINE bool IsString(const AZ::Uuid& type)
        {
            return type == azrtti_typeid<StringType>();
        }

        AZ_INLINE bool IsString(const Type& type)
        {
            return type.GetType() == eType::String;
        }

        AZ_INLINE bool IsTransform(const AZ::Uuid& type)
        {
            return type == azrtti_typeid<TransformType>();
        }

        AZ_INLINE bool IsTransform(const Type& type)
        {
            return type.GetType() == eType::Transform;
        }

        AZ_INLINE bool IsVector3(const AZ::Uuid& type)
        {
            return type == azrtti_typeid<AZ::Vector3>();
        }

        AZ_INLINE bool IsVector2(const AZ::Uuid& type)
        {
            return type == azrtti_typeid<AZ::Vector2>();
        }

        AZ_INLINE bool IsVector4(const AZ::Uuid& type)
        {
            return type == azrtti_typeid<AZ::Vector4>();
        }

        AZ_INLINE bool IsVector3(const Type& type)
        {
            return type.GetType() == eType::Vector3;
        }

        AZ_INLINE bool IsVector2(const Type& type)
        {
            return type.GetType() == eType::Vector2;
        }

        AZ_INLINE bool IsVector4(const Type& type)
        {
            return type.GetType() == eType::Vector4;
        }

        AZ_INLINE AZ::Uuid ToAZType(eType type)
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

            default:
                AZ_Assert(false, "Invalid type!");
                // check for behavior context support
                return AZ::Uuid::CreateNull();
            }
        }

        AZ_INLINE AZ::Uuid ToAZType(const Type& type)
        {
            eType typeEnum = type.GetType();
            if (typeEnum == eType::BehaviorContextObject)
            {
                return type.GetAZType();
            }

            return ToAZType(typeEnum);
        }

        AZ_INLINE bool IsVectorType(const AZ::Uuid& type)
        {
            return type == azrtti_typeid<AZ::Vector3>()
                || type == azrtti_typeid<AZ::Vector2>()
                || type == azrtti_typeid<AZ::Vector4>();
        }

        AZ_INLINE bool IsVectorType(const Type& type)
        {
            static const AZ::u32 s_vectorTypes =
            {
                1 << static_cast<AZ::u32>(eType::Vector3)
                | 1 << static_cast<AZ::u32>(eType::Vector2)
                | 1 << static_cast<AZ::u32>(eType::Vector4)
            };

            return ((1 << static_cast<AZ::u32>(type.GetType())) & s_vectorTypes) != 0;
        }

        AZ_INLINE bool IsAutoBoxedType(const Type& type)
        {
            static const AZ::u32 s_autoBoxedTypes =
            {
                1 << static_cast<AZ::u32>(eType::AABB)
                | 1 << static_cast<AZ::u32>(eType::Color)
                | 1 << static_cast<AZ::u32>(eType::CRC)
                | 1 << static_cast<AZ::u32>(eType::Matrix3x3)
                | 1 << static_cast<AZ::u32>(eType::Matrix4x4)
                | 1 << static_cast<AZ::u32>(eType::OBB)
                | 1 << static_cast<AZ::u32>(eType::Quaternion)
                | 1 << static_cast<AZ::u32>(eType::Transform)
                | 1 << static_cast<AZ::u32>(eType::Vector3)
                | 1 << static_cast<AZ::u32>(eType::Vector2)
                | 1 << static_cast<AZ::u32>(eType::Vector4)
            };

            return ((1 << static_cast<AZ::u32>(type.GetType())) & s_autoBoxedTypes) != 0;
        }

        AZ_INLINE bool IsValueType(const Type& type)
        {
            return type.GetType() != eType::BehaviorContextObject;
        }

        AZ_FORCE_INLINE Type::Type()
            : m_type(eType::Invalid)
            , m_azType(AZ::Uuid::CreateNull())
        {}

        AZ_FORCE_INLINE Type::Type(eType type)
            : m_type(type)
            , m_azType(AZ::Uuid::CreateNull())
        {}

        AZ_FORCE_INLINE Type::Type(const AZ::Uuid& aztype)
            : m_type(eType::BehaviorContextObject)
            , m_azType(aztype)
        {
            AZ_Error("ScriptCanvas", !aztype.IsNull(), "no invalid aztypes allowed");
        }

        AZ_FORCE_INLINE Type Type::AABB()
        {
            return Type(eType::AABB);
        }

        AZ_FORCE_INLINE Type Type::AssetId()
        {
            return Type(eType::AssetId);
        }

        AZ_FORCE_INLINE Type Type::BehaviorContextObject(const AZ::Uuid& aztype)
        {
            return Type(aztype);
        }

        AZ_FORCE_INLINE Type Type::Boolean()
        {
            return Type(eType::Boolean);
        }

        AZ_FORCE_INLINE Type Type::Color()
        {
            return Type(eType::Color);
        }

        AZ_FORCE_INLINE Type Type::CRC()
        {
            return Type(eType::CRC);
        }

        AZ_FORCE_INLINE Type Type::EntityID()
        {
            return Type(eType::EntityID);
        }

        AZ_FORCE_INLINE Type Type::NamedEntityID()
        {
            return Type(eType::NamedEntityID);
        }

        AZ_FORCE_INLINE AZ::Uuid Type::GetAZType() const
        {
            if (m_type == eType::BehaviorContextObject)
            {
                return m_azType;
            }
            else
            {
                return ScriptCanvas::Data::ToAZType((*this));
            }
        }

        AZ_FORCE_INLINE eType Type::GetType() const
        {
            return m_type;
        }

        AZ_FORCE_INLINE Type Type::Invalid()
        {
            return Type();
        }

        AZ_FORCE_INLINE bool IS_A(const Type& candidate, const Type& reference)
        {
            return candidate.IS_A(reference);
        }

        AZ_FORCE_INLINE bool IS_EXACTLY_A(const Type& candidate, const Type& reference)
        {
            return candidate.IS_EXACTLY_A(reference);
        }

        AZ_FORCE_INLINE bool IsConvertible(const Type& source, const AZ::Uuid& target)
        {
            return source.IsConvertibleTo(target);
        }

        AZ_FORCE_INLINE bool IsConvertible(const Type& source, const Type& target)
        {
            return source.IsConvertibleTo(target);
        }

        AZ_FORCE_INLINE bool Type::IsConvertibleFrom(const AZ::Uuid& target) const
        {
            return FromAZType(target).IsConvertibleTo(*this);
        }

        AZ_FORCE_INLINE bool Type::IsConvertibleFrom(const Type& target) const
        {
            return target.IsConvertibleTo(*this);
        }

        AZ_FORCE_INLINE bool Type::IsConvertibleTo(const AZ::Uuid& target) const
        {
            return IsConvertibleTo(FromAZType(target));
        }

        AZ_FORCE_INLINE bool Type::IsConvertibleTo(const Type& target) const
        {
            auto isVector2Vector = [&]()->bool 
            {
                return IsVectorType(target) || (target.GetType() == eType::BehaviorContextObject && IsVectorType(target.GetAZType()));
            };

            AZ_Assert(!IS_A(target)
                , "Don't mix concepts, it is too dangerous. "
                  "Check IS-A separately from conversion at all times. "
                  "Use IS_A || IsConvertibleTo in an expression");

            const auto targetEType = target.GetType();
            if (targetEType == eType::String)
            {
                // everything is implicitly convertible to strings, including strings
                return true;
            }

            switch (GetType())
            {
            case eType::Boolean:
                return targetEType == eType::Number;

            case eType::Color:
                return targetEType == eType::Vector3
                    || targetEType == eType::Vector4;

            case eType::Matrix3x3:
                return targetEType == eType::Quaternion;

            case eType::Matrix4x4:
                return targetEType == eType::Transform
                    || targetEType == eType::Quaternion;

            case eType::Number:
                return targetEType == eType::Boolean;

            case eType::Transform:
                return targetEType == eType::Matrix4x4;

            case eType::Quaternion:
                return targetEType == eType::Matrix3x3
                    || targetEType == eType::Matrix4x4
                    || targetEType == eType::Transform;

            case eType::Vector2:
                return isVector2Vector();
            case eType::Vector3:
            case eType::Vector4:
                return isVector2Vector() || targetEType == eType::Color;
            default:
                return false;
            }
        }

        AZ_FORCE_INLINE bool Type::IS_A(const Type& other) const
        {
            return m_type == other.m_type && (m_type != eType::BehaviorContextObject || m_azType == other.m_azType || IsAZRttiTypeOf(m_azType, other.m_azType));
        }

        AZ_FORCE_INLINE bool Type::IS_EXACTLY_A(const Type& other) const
        {
            return m_type == other.m_type && m_azType == other.m_azType;
        }

        AZ_FORCE_INLINE bool Type::IsValid() const
        {
            return m_type != eType::Invalid;
        }

        AZ_FORCE_INLINE Type Type::Matrix3x3()
        {
            return Type(eType::Matrix3x3);
        }

        AZ_FORCE_INLINE Type Type::Matrix4x4()
        {
            return Type(eType::Matrix4x4);
        }

        AZ_FORCE_INLINE Type Type::Number()
        {
            return Type(eType::Number);
        }

        AZ_FORCE_INLINE Type Type::OBB()
        {
            return Type(eType::OBB);
        }

        AZ_FORCE_INLINE Type::operator bool() const
        {
            return m_type != eType::Invalid;
        }

        AZ_FORCE_INLINE bool Type::operator!() const
        {
            return m_type == eType::Invalid;
        }

        AZ_FORCE_INLINE Type Type::Plane()
        {
            return Type(eType::Plane);
        }

        AZ_FORCE_INLINE Type Type::Quaternion()
        {
            return Type(eType::Quaternion);
        }

        AZ_FORCE_INLINE Type Type::String()
        {
            return Type(eType::String);
        }

        AZ_FORCE_INLINE Type Type::Transform()
        {
            return Type(eType::Transform);
        }

        AZ_FORCE_INLINE Type Type::Vector3()
        {
            return Type(eType::Vector3);
        }

        AZ_FORCE_INLINE Type Type::Vector2()
        {
            return Type(eType::Vector2);
        }

        AZ_FORCE_INLINE Type Type::Vector4()
        {
            return Type(eType::Vector4);
        }

    } 

} 

namespace AZStd
{
    template<>
    struct hash<ScriptCanvas::Data::Type>
    {
        size_t operator()(const ScriptCanvas::Data::Type& ref) const
        {
            size_t seed = 0U;
            hash_combine(seed, ref.GetType());
            hash_combine(seed, ScriptCanvas::Data::ToAZType(ref));
            return seed;
        }
    };
}
