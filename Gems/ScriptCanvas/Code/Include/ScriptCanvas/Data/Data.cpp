/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "Data.h"
#include <ScriptCanvas/Data/DataTrait.h>

#include <AzCore/Component/EntityBus.h>
#include <AzCore/RTTI/AttributeReader.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/RTTI/ReflectContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/Utils.h>

namespace DataCpp
{
    using namespace ScriptCanvas;
    using namespace ScriptCanvas::Data;

    AZ_INLINE AZStd::pair<bool, Type> FromAZTypeHelper(const AZ::Uuid& type)
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
            return{ true, Type::NamedEntityID() };
        }
        else if (IsMatrix3x3(type))
        {
            return { true, Type::Matrix3x3() };
        }
        else if (IsMatrix4x4(type))
        {
            return { true, Type::Matrix4x4() };
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
        else
        {
            return { false, Type::Invalid() };
        }
    }

    AZ_INLINE AZStd::pair<bool, Type> FromBehaviorContextTypeHelper(const AZ::Uuid& type)
    {
        if (type.IsNull())
        {
            return { true, Type::Invalid() };
        }
        else if (IsBoolean(type))
        {
            return { true, Type::Boolean() };
        }
        else if (IsEntityID(type))
        {
            return { true, Type::EntityID() };
        }
        else if (IsNamedEntityID(type))
        {
            return{ true, Type::NamedEntityID() };
        }
        else if (IsNumber(type))
        {
            return { true, Type::Number() };
        }
        else if (IsString(type))
        {
            return { true, Type::String() };
        }
        else
        {
            return { false, Type::Invalid() };
        }
    }
    
    AZ_INLINE bool IsSupportedBehaviorContextObject(const AZ::Uuid& typeID)
    {
        return AZ::BehaviorContextHelper::GetClass(typeID) != nullptr;
    }

    AZ_INLINE const char* GetRawBehaviorContextName(const AZ::Uuid& typeID)
    {
        AZ::BehaviorContext* behaviorContext(nullptr);
        AZ::ComponentApplicationBus::BroadcastResult(behaviorContext, &AZ::ComponentApplicationRequests::GetBehaviorContext);
        if (!behaviorContext)
        {
            AZ_Assert(behaviorContext, "A behavior context is required!");
            return "Invalid BehaviorContext::Class name";
        }

        if (const AZ::BehaviorClass* behaviorClass = AZ::BehaviorContextHelper::GetClass(typeID))
        {
            return behaviorClass->m_name.data();
        }

        return "Invalid BehaviorContext::Class name";
    }
}

namespace ScriptCanvas
{
    namespace Data
    {
        Type FromAZType(const AZ::Uuid& type)
        {
            AZStd::pair<bool, Type> help = DataCpp::FromAZTypeHelper(type);
            return help.first ? help.second : Type::BehaviorContextObject(type);
        }
        
        Type FromAZTypeChecked(const AZ::Uuid& type)
        {
            AZStd::pair<bool, Type> help = DataCpp::FromAZTypeHelper(type);
            return help.first 
                ? help.second 
                : DataCpp::IsSupportedBehaviorContextObject(type)
                    ? Type::BehaviorContextObject(type)
                    : Type::Invalid();
        }
        
        AZStd::string GetBehaviorClassName(const AZ::Uuid& typeID)
        {
            AZ::BehaviorContext* behaviorContext(nullptr);
            AZ::ComponentApplicationBus::BroadcastResult(behaviorContext, &AZ::ComponentApplicationRequests::GetBehaviorContext);
            if (!behaviorContext)
            {
                AZ_Error("Behavior Context", false, "A behavior context is required!");
                return "Invalid BehaviorContext::Class name";
            }

            if (AZ::Utils::IsGenericContainerType(typeID))
            {
                if (AZ::Utils::IsSetContainerType(typeID))
                {
                    return "Set";
                }
                else if (AZ::Utils::IsMapContainerType(typeID))
                {
                    return "Map";
                }
                // Special casing out the fixed size vectors/arrays.
                // Will need a more in depth way of generating these names long term I think.
                else if (typeID == AZ::GetGenericClassInfoArrayTypeId()
                    || typeID == AZ::GetGenericClassInfoFixedVectorTypeId())
                {
                    return "Fixed Size Array";
                }
                else if (AZ::Utils::IsVectorContainerType(typeID))
                {
                    return "Array";
                }                
                else
                {
                    return "Unknown Container";
                }
            }
            else
            {
                if (const AZ::BehaviorClass* behaviorClass = AZ::BehaviorContextHelper::GetClass(typeID))
                {
                    if (AZ::Attribute* prettyNameAttribute = AZ::FindAttribute(AZ::ScriptCanvasAttributes::PrettyName, behaviorClass->m_attributes))
                    {
                        AZ::AttributeReader operatorAttrReader(nullptr, prettyNameAttribute);
                        AZStd::string prettyName;

                        if (operatorAttrReader.Read<AZStd::string>(prettyName, *behaviorContext))
                        {
                            return prettyName;
                        }
                    }

                    return behaviorClass->m_name;
                }
            }

            return "Invalid BehaviorContext::Class name";
        }
        
        AZStd::string GetName(const Type& type)
        {
            switch (type.GetType())
            {
            case eType::AABB:
                return eTraits<eType::AABB>::GetName();

            case eType::AssetId:
                return eTraits<eType::AssetId>::GetName();

            case eType::BehaviorContextObject:
                return GetBehaviorClassName(type.GetAZType());

            case eType::Boolean:
                return eTraits<eType::Boolean>::GetName();

            case eType::Color:
                return eTraits<eType::Color>::GetName();

            case eType::CRC:
                return eTraits<eType::CRC>::GetName();

            case eType::EntityID:
                return eTraits<eType::EntityID>::GetName();

            case eType::NamedEntityID:
                return eTraits<eType::NamedEntityID>::GetName();

            case eType::Invalid:
                return "Invalid";

            case eType::Matrix3x3:
                return eTraits<eType::Matrix3x3>::GetName();

            case eType::Matrix4x4:
                return eTraits<eType::Matrix4x4>::GetName();

            case eType::Number:
                return eTraits<eType::Number>::GetName();

            case eType::OBB:
                return eTraits<eType::OBB>::GetName();

            case eType::Plane:
                return eTraits<eType::Plane>::GetName();

            case eType::Quaternion:
                return eTraits<eType::Quaternion>::GetName();

            case eType::String:
                return eTraits<eType::String>::GetName();

            case eType::Transform:
                return eTraits<eType::Transform>::GetName();

            case eType::Vector2:
                return eTraits<eType::Vector2>::GetName();

            case eType::Vector3:
                return eTraits<eType::Vector3>::GetName();

            case eType::Vector4:
                return eTraits<eType::Vector4>::GetName();

            default:
                AZ_Assert(false, "Invalid type!");
                return "Error: invalid type";
            }
        }

        const Type GetBehaviorParameterDataType(const AZ::BehaviorParameter& parameter)
        {
            return AZ::BehaviorContextHelper::IsStringParameter(parameter) ? Data::Type::String() : Data::FromAZType(parameter.m_typeId);
        }

        const char* GetBehaviorContextName(const AZ::Uuid& azType)
        {
            return GetBehaviorContextName(FromAZType(azType));
        }

        const char* GetBehaviorContextName(const Type& type)
        {
            switch (type.GetType())
            {
            case eType::Boolean:
                return "Boolean";

            case eType::EntityID:
                return "EntityId";

            case eType::Invalid:
                return "Invalid";

            case eType::Number:
                return "Number";

            case eType::String:
                return "String";

            case eType::AABB:
            case eType::AssetId:
            case eType::BehaviorContextObject:
            case eType::Color:
            case eType::CRC:
            case eType::Matrix3x3:
            case eType::Matrix4x4:
            case eType::OBB:
            case eType::Plane:
            case eType::Quaternion:
            case eType::Transform:
            case eType::Vector3:
            case eType::Vector2:
            case eType::Vector4:
            default:
                return DataCpp::GetRawBehaviorContextName(ToAZType(type));
            }
        }

        bool IsAZRttiTypeOf(const AZ::Uuid& candidate, const AZ::Uuid& reference)
        {
            auto bcClass = AZ::BehaviorContextHelper::GetClass(candidate);
            return bcClass
                && bcClass->m_azRtti
                && bcClass->m_azRtti->IsTypeOf(reference);
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

        bool IsMapContainerType(const AZ::Uuid& type)
        {
            return AZ::Utils::IsMapContainerType(type);
        }

        bool IsMapContainerType(const Type& type)
        {
            return AZ::Utils::IsMapContainerType(ToAZType(type));
        }

        bool IsContainerType(const AZ::Uuid& type)
        {
            return AZ::Utils::IsContainerType(type);
        }

        bool IsContainerType(const Type& type)
        {
            return AZ::Utils::IsContainerType(ToAZType(type));
        }

        AZStd::vector<AZ::Uuid> GetContainedTypes(const AZ::Uuid& type)
        {
            return AZ::Utils::GetContainedTypes(type);
        }

        AZStd::vector<Type> GetContainedTypes(const Type& type)
        {
            AZStd::vector<Type> types;

            AZ::SerializeContext* serializeContext = nullptr;
            AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationRequests::GetSerializeContext);

            if (serializeContext)
            {
                AZ::GenericClassInfo* classInfo = serializeContext->FindGenericClassInfo(ToAZType(type));

                if (classInfo)
                {
                    for (int i = 0; i < classInfo->GetNumTemplatedArguments(); ++i)
                    {
                        types.push_back(FromAZType(classInfo->GetTemplatedTypeId(i)));
                    }
                }
            }

            return types;
        }

        AZStd::pair<AZ::Uuid, AZ::Uuid> GetOutcomeTypes(const AZ::Uuid& type)
        {
            return AZ::Utils::GetOutcomeTypes(type);
        }

        AZStd::pair<Type, Type> GetOutcomeTypes(const Type& type)
        {
            AZStd::pair<AZ::Uuid, AZ::Uuid> azOutcomeTypes(GetOutcomeTypes(ToAZType(type)));
            return AZStd::make_pair(FromAZType(azOutcomeTypes.first), FromAZType(azOutcomeTypes.second));
        }

        void Type::Reflect(AZ::ReflectContext* reflection)
        {
            if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflection))
            {
                serializeContext->Class<Type>()
                    ->Version(2)
                    ->Field("m_type", &Type::m_type)
                    ->Field("m_azType", &Type::m_azType)
                    ;
            }
        }

        bool Type::operator==(const Type& other) const
        {
            return IS_EXACTLY_A(other);
        }

        bool Type::operator!=(const Type& other) const
        {
            return !((*this) == other);
        }

    }
}
