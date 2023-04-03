/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "DataType.h"

#include <AzCore/Serialization/SerializeContext.h>
#include <ScriptCanvas/Data/DataTypeUtils.h>

namespace ScriptCanvas
{
    namespace Data
    {
        void Type::Reflect(AZ::ReflectContext* reflection)
        {
            if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflection))
            {
                serializeContext->Class<Type>()
                    ->Version(2)
                    ->Field("m_type", &Type::m_type)
                    ->Field("m_azType", &Type::m_azType);
            }
        }

        Type::Type()
            : m_type(eType::Invalid)
            , m_azType(AZ::Uuid::CreateNull())
        {
        }

        Type::Type(eType type)
            : m_type(type)
            , m_azType(AZ::Uuid::CreateNull())
        {
        }

        Type::Type(const AZ::Uuid& aztype)
            : m_type(eType::BehaviorContextObject)
            , m_azType(aztype)
        {
            AZ_Error("ScriptCanvas", !aztype.IsNull(), "no invalid aztypes allowed");
        }

        Type Type::AABB()
        {
            return Type(eType::AABB);
        }

        Type Type::AssetId()
        {
            return Type(eType::AssetId);
        }

        Type Type::BehaviorContextObject(const AZ::Uuid& aztype)
        {
            return Type(aztype);
        }

        Type Type::Boolean()
        {
            return Type(eType::Boolean);
        }

        Type Type::Color()
        {
            return Type(eType::Color);
        }

        Type Type::CRC()
        {
            return Type(eType::CRC);
        }

        Type Type::EntityID()
        {
            return Type(eType::EntityID);
        }

        Type Type::NamedEntityID()
        {
            return Type(eType::NamedEntityID);
        }

        Type Type::Invalid()
        {
            return Type();
        }

        Type Type::Matrix3x3()
        {
            return Type(eType::Matrix3x3);
        }

        Type Type::Matrix4x4()
        {
            return Type(eType::Matrix4x4);
        }

        Type Type::Number()
        {
            return Type(eType::Number);
        }

        Type Type::OBB()
        {
            return Type(eType::OBB);
        }

        Type Type::Plane()
        {
            return Type(eType::Plane);
        }

        Type Type::Quaternion()
        {
            return Type(eType::Quaternion);
        }

        Type Type::String()
        {
            return Type(eType::String);
        }

        Type Type::Transform()
        {
            return Type(eType::Transform);
        }

        Type Type::Vector3()
        {
            return Type(eType::Vector3);
        }

        Type Type::Vector2()
        {
            return Type(eType::Vector2);
        }

        Type Type::Vector4()
        {
            return Type(eType::Vector4);
        }

        AZ::Uuid Type::GetAZType() const
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

        eType Type::GetType() const
        {
            return m_type;
        }

        bool Type::IsValid() const
        {
            return m_type != eType::Invalid;
        }

        bool Type::IsConvertibleFrom(const AZ::Uuid& target) const
        {
            return FromAZType(target).IsConvertibleTo(*this);
        }

        bool Type::IsConvertibleFrom(const Type& target) const
        {
            return target.IsConvertibleTo(*this);
        }

        bool Type::IsConvertibleTo(const AZ::Uuid& target) const
        {
            return IsConvertibleTo(FromAZType(target));
        }

        bool Type::IsConvertibleTo(const Type& target) const
        {
            auto isVector2Vector = [&]() -> bool
            {
                return IsVectorType(target) || (target.GetType() == eType::BehaviorContextObject && IsVectorType(target.GetAZType()));
            };

            AZ_Assert(!IS_A(target), "Don't mix concepts, it is too dangerous. Check IS-A separately from conversion at all times. "
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
                return targetEType == eType::Vector3 || targetEType == eType::Vector4;

            case eType::Matrix3x3:
                return targetEType == eType::Quaternion;

            case eType::Matrix4x4:
                return targetEType == eType::Transform || targetEType == eType::Quaternion;

            case eType::Number:
                return targetEType == eType::Boolean;

            case eType::Transform:
                return targetEType == eType::Matrix4x4;

            case eType::Quaternion:
                return targetEType == eType::Matrix3x3 || targetEType == eType::Matrix4x4 || targetEType == eType::Transform;

            case eType::Vector2:
                return isVector2Vector();
            case eType::Vector3:
            case eType::Vector4:
                return isVector2Vector() || targetEType == eType::Color;
            default:
                return false;
            }
        }

        bool Type::IS_A(const Type& other) const
        {
            return m_type == other.m_type &&
                (m_type != eType::BehaviorContextObject || m_azType == other.m_azType || IsAZRttiTypeOf(m_azType, other.m_azType));
        }

        bool Type::IS_EXACTLY_A(const Type& other) const
        {
            return m_type == other.m_type && m_azType == other.m_azType;
        }

        Type::operator bool() const
        {
            return m_type != eType::Invalid;
        }

        bool Type::operator!() const
        {
            return m_type == eType::Invalid;
        }

        bool Type::operator==(const Type& other) const
        {
            return IS_EXACTLY_A(other);
        }

        bool Type::operator!=(const Type& other) const
        {
            return !((*this) == other);
        }
    } // namespace Data
} // namespace ScriptCanvas
