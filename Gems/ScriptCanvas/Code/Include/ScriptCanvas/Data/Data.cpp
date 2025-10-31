/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
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
                else if (typeID == AZ::GetGenericClassInfoArrayTypeId() || typeID == AZ::GetGenericClassInfoFixedVectorTypeId())
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
                    if (AZ::Attribute* prettyNameAttribute =
                            AZ::FindAttribute(AZ::ScriptCanvasAttributes::PrettyName, behaviorClass->m_attributes))
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

            case eType::MatrixMxN:
                return eTraits<eType::MatrixMxN>::GetName();

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

            case eType::VectorN:
                return eTraits<eType::VectorN>::GetName();

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
    }
}
