/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <ScriptEvents/ScriptEventTypes.h>

#include <AzCore/Math/Vector2.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/Vector4.h>
#include <AzCore/Math/Quaternion.h>
#include <AzCore/Math/Matrix3x3.h>
#include <AzCore/Math/Matrix4x4.h>

#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/std/sort.h>

namespace ScriptEvents
{
    namespace Types
    {
        namespace
        {
            void ParseForVersionedTypes(AZ::BehaviorClass* behaviorClass, AZ::Crc32 attributeId, VersionedTypes& typeTarget, AZ::BehaviorContext& behaviorContext)
            {
                if (auto attributeAttribute = behaviorClass->FindAttribute(attributeId))
                {
                    AZ::AttributeReader reader(behaviorClass, attributeAttribute);

                    bool isEnabled = false;

                    if (reader.Read<bool>(isEnabled))
                    {
                        if (isEnabled)
                        {
                            AZStd::string displayName = behaviorClass->m_name;

                            auto prettyNameAttribute = behaviorClass->FindAttribute(AZ::ScriptCanvasAttributes::PrettyName);

                            if (prettyNameAttribute != nullptr)
                            {
                                AZ::AttributeReader prettyNameReader(behaviorClass, prettyNameAttribute);

                                if (!prettyNameReader.Read<AZStd::string>(displayName, behaviorContext))
                                {
                                    displayName = behaviorClass->m_name;
                                }
                            }

                            typeTarget.push_back(AZStd::make_pair(ScriptEventData::VersionedProperty::Make<AZ::Uuid>(behaviorClass->m_typeId, displayName.c_str()), displayName));
                        }
                    }
                }
            }

            void ParseForTypeId(AZ::BehaviorClass* behaviorClass, AZ::Crc32 attributeId, AZStd::vector<AZ::Uuid>& uuidList)
            {
                if (auto paramAttribute = behaviorClass->FindAttribute(attributeId))
                {
                    AZ::AttributeReader reader(behaviorClass, paramAttribute);

                    bool isEnabled = false;

                    if (reader.Read<bool>(isEnabled))
                    {
                        if (isEnabled)
                        {
                            uuidList.push_back(behaviorClass->m_typeId);
                        }
                    }
                }
            }
        }

        VersionedTypes GetValidAddressTypes()
        {
            using namespace ScriptEventData;

            static VersionedTypes validAddressTypes;

            if (validAddressTypes.empty())
            {
                validAddressTypes.push_back(AZStd::make_pair(VersionedProperty::Make<AZ::Uuid>(azrtti_typeid<void>(), "None"), "None"));
                validAddressTypes.push_back(AZStd::make_pair(VersionedProperty::Make<AZ::Uuid>(azrtti_typeid<AZStd::string>(), "String"), "String"));
                validAddressTypes.push_back(AZStd::make_pair(VersionedProperty::Make<AZ::Uuid>(azrtti_typeid<AZ::EntityId>(), "Entity Id"), "Entity Id"));
                validAddressTypes.push_back(AZStd::make_pair(VersionedProperty::Make<AZ::Uuid>(azrtti_typeid<AZ::Crc32>(), "Tag"), "Tag"));
            }

            return validAddressTypes;
        }        

        // Returns the list of the valid Script Event method parameter type Ids, this is used 
        AZStd::vector<AZ::Uuid> GetSupportedParameterTypes()
        {
            static AZStd::vector<AZ::Uuid> supportedTypes;
            if (supportedTypes.empty())
            {
                supportedTypes.push_back(azrtti_typeid<bool>());
                supportedTypes.push_back(azrtti_typeid<double>());
                supportedTypes.push_back(azrtti_typeid<AZ::EntityId>());
                supportedTypes.push_back(azrtti_typeid<AZStd::string>());
                supportedTypes.push_back(azrtti_typeid<AZ::Vector2>());
                supportedTypes.push_back(azrtti_typeid<AZ::Vector3>());
                supportedTypes.push_back(azrtti_typeid<AZ::Vector4>());
                supportedTypes.push_back(azrtti_typeid<AZ::Matrix3x3>());
                supportedTypes.push_back(azrtti_typeid<AZ::Matrix4x4>());
                supportedTypes.push_back(azrtti_typeid<AZ::Transform>());
                supportedTypes.push_back(azrtti_typeid<AZ::Quaternion>());
                supportedTypes.push_back(azrtti_typeid<AZ::Crc32>());

                AZ::BehaviorContext* behaviorContext = nullptr;
                AZ::ComponentApplicationBus::BroadcastResult(behaviorContext, &AZ::ComponentApplicationRequests::GetBehaviorContext);

                if (behaviorContext)
                {
                    for (auto behaviorClassPair : behaviorContext->m_classes)
                    {
                        ParseForTypeId(behaviorClassPair.second, AZ::Script::Attributes::EnableAsScriptEventParamType, supportedTypes);
                    }
                }
            }

            return supportedTypes;
        }

        // Returns the list of the valid Script Event method parameters, this is used to populate the ReflectedPropertyEditor's combobox
        VersionedTypes GetValidParameterTypes()
        {
            using namespace ScriptEventData;

            static VersionedTypes validParamTypes;

            if (validParamTypes.empty())
            {
                validParamTypes.push_back(AZStd::make_pair(VersionedProperty::Make<AZ::Uuid>(azrtti_typeid<bool>(), "Boolean"), "Boolean"));
                validParamTypes.push_back(AZStd::make_pair(VersionedProperty::Make<AZ::Uuid>(azrtti_typeid<double>(), "Number"), "Number"));
                validParamTypes.push_back(AZStd::make_pair(VersionedProperty::Make<AZ::Uuid>(azrtti_typeid<AZStd::string>(), "String"), "String"));
                validParamTypes.push_back(AZStd::make_pair(VersionedProperty::Make<AZ::Uuid>(azrtti_typeid<AZ::EntityId>(), "Entity Id"), "Entity Id"));
                validParamTypes.push_back(AZStd::make_pair(VersionedProperty::Make<AZ::Uuid>(azrtti_typeid<AZ::Vector2>(), "Vector2"), "Vector2")); 
                validParamTypes.push_back(AZStd::make_pair(VersionedProperty::Make<AZ::Uuid>(azrtti_typeid<AZ::Vector3>(), "Vector3"), "Vector3"));
                validParamTypes.push_back(AZStd::make_pair(VersionedProperty::Make<AZ::Uuid>(azrtti_typeid<AZ::Vector4>(), "Vector4"), "Vector4"));
                validParamTypes.push_back(AZStd::make_pair(VersionedProperty::Make<AZ::Uuid>(azrtti_typeid<AZ::Matrix3x3>(), "Matrix3x3"), "Matrix3x3"));
                validParamTypes.push_back(AZStd::make_pair(VersionedProperty::Make<AZ::Uuid>(azrtti_typeid<AZ::Matrix4x4>(), "Matrix4x4"), "Matrix4x4"));
                validParamTypes.push_back(AZStd::make_pair(VersionedProperty::Make<AZ::Uuid>(azrtti_typeid<AZ::Transform>(), "Transform"), "Transform"));
                validParamTypes.push_back(AZStd::make_pair(VersionedProperty::Make<AZ::Uuid>(azrtti_typeid<AZ::Quaternion>(), "Quaternion"), "Quaternion"));
                validParamTypes.push_back(AZStd::make_pair(VersionedProperty::Make<AZ::Uuid>(azrtti_typeid<AZ::Crc32>(), "Tag"), "Tag"));

                AZ::BehaviorContext* behaviorContext = nullptr;
                AZ::ComponentApplicationBus::BroadcastResult(behaviorContext, &AZ::ComponentApplicationRequests::GetBehaviorContext);

                if (behaviorContext)
                {
                    for (auto behaviorClassPair : behaviorContext->m_classes)
                    {
                        ParseForVersionedTypes(behaviorClassPair.second, AZ::Script::Attributes::EnableAsScriptEventParamType, validParamTypes, (*behaviorContext));
                    }
                }
            }

            return validParamTypes;
        }

        // Determines whether the specified type is a valid parameter on a Script Event method argument list
        bool IsValidParameterType(const AZ::Uuid& typeId)
        {
            for (const auto& supportedParam : GetSupportedParameterTypes())
            {
                if (supportedParam == typeId)
                {
                    return true;
                }
            }

            return false;
        }

        // Supported return types for Script Event methods
        AZStd::vector<AZ::Uuid> GetSupportedReturnTypes()
        {
            static AZStd::vector<AZ::Uuid> supportedTypes;
            if (supportedTypes.empty())
            {
                supportedTypes.push_back(azrtti_typeid<void>());
                supportedTypes.push_back(azrtti_typeid<bool>());
                supportedTypes.push_back(azrtti_typeid<double>());
                supportedTypes.push_back(azrtti_typeid<AZ::EntityId>());
                supportedTypes.push_back(azrtti_typeid<AZStd::string>());
                supportedTypes.push_back(azrtti_typeid<AZ::Vector2>());
                supportedTypes.push_back(azrtti_typeid<AZ::Vector3>());
                supportedTypes.push_back(azrtti_typeid<AZ::Vector4>());
                supportedTypes.push_back(azrtti_typeid<AZ::Matrix3x3>());
                supportedTypes.push_back(azrtti_typeid<AZ::Matrix4x4>());
                supportedTypes.push_back(azrtti_typeid<AZ::Transform>());
                supportedTypes.push_back(azrtti_typeid<AZ::Quaternion>());
                supportedTypes.push_back(azrtti_typeid<AZ::Crc32>());

                AZ::BehaviorContext* behaviorContext = nullptr;
                AZ::ComponentApplicationBus::BroadcastResult(behaviorContext, &AZ::ComponentApplicationRequests::GetBehaviorContext);

                if (behaviorContext)
                {
                    for (auto behaviorClassPair : behaviorContext->m_classes)
                    {
                        ParseForTypeId(behaviorClassPair.second, AZ::Script::Attributes::EnableAsScriptEventReturnType, supportedTypes);
                    }
                }
            }

            return supportedTypes;
        }

        VersionedTypes GetValidReturnTypes()
        {
            using namespace ScriptEventData;

            static VersionedTypes validReturnTypes;

            if (validReturnTypes.empty())
            {
                validReturnTypes.push_back(AZStd::make_pair(VersionedProperty::Make<AZ::Uuid>(azrtti_typeid<void>(), "None"), "None"));
                validReturnTypes.push_back(AZStd::make_pair(VersionedProperty::Make<AZ::Uuid>(azrtti_typeid<bool>(), "Boolean"), "Boolean"));
                validReturnTypes.push_back(AZStd::make_pair(VersionedProperty::Make<AZ::Uuid>(azrtti_typeid<double>(), "Number"), "Number"));
                validReturnTypes.push_back(AZStd::make_pair(VersionedProperty::Make<AZ::Uuid>(azrtti_typeid<AZStd::string>(), "String"), "String"));
                validReturnTypes.push_back(AZStd::make_pair(VersionedProperty::Make<AZ::Uuid>(azrtti_typeid<AZ::EntityId>(), "Entity Id"), "Entity Id"));
                validReturnTypes.push_back(AZStd::make_pair(VersionedProperty::Make<AZ::Uuid>(azrtti_typeid<AZ::Vector2>(), "Vector2"), "Vector2"));
                validReturnTypes.push_back(AZStd::make_pair(VersionedProperty::Make<AZ::Uuid>(azrtti_typeid<AZ::Vector3>(), "Vector3"), "Vector3"));
                validReturnTypes.push_back(AZStd::make_pair(VersionedProperty::Make<AZ::Uuid>(azrtti_typeid<AZ::Vector4>(), "Vector4"), "Vector4"));
                validReturnTypes.push_back(AZStd::make_pair(VersionedProperty::Make<AZ::Uuid>(azrtti_typeid<AZ::Matrix3x3>(), "Matrix3x3"), "Matrix3x3"));
                validReturnTypes.push_back(AZStd::make_pair(VersionedProperty::Make<AZ::Uuid>(azrtti_typeid<AZ::Matrix4x4>(), "Matrix4x4"), "Matrix4x4"));
                validReturnTypes.push_back(AZStd::make_pair(VersionedProperty::Make<AZ::Uuid>(azrtti_typeid<AZ::Transform>(), "Transform"), "Transform"));
                validReturnTypes.push_back(AZStd::make_pair(VersionedProperty::Make<AZ::Uuid>(azrtti_typeid<AZ::Quaternion>(), "Quaternion"), "Quaternion"));
                validReturnTypes.push_back(AZStd::make_pair(VersionedProperty::Make<AZ::Uuid>(azrtti_typeid<AZ::Crc32>(), "Tag"), "Tag"));

                AZ::BehaviorContext* behaviorContext = nullptr;
                AZ::ComponentApplicationBus::BroadcastResult(behaviorContext, &AZ::ComponentApplicationRequests::GetBehaviorContext);

                if (behaviorContext)
                {
                    for (auto behaviorClassPair : behaviorContext->m_classes)
                    {
                        ParseForVersionedTypes(behaviorClassPair.second, AZ::Script::Attributes::EnableAsScriptEventReturnType, validReturnTypes, (*behaviorContext));
                    }
                }
            }

            return validReturnTypes;
        }

        bool IsValidReturnType(const AZ::Uuid& typeId)
        {
            for (const auto& supportedType: GetSupportedReturnTypes())
            {
                if (supportedType == typeId)
                {
                    return true;
                }
            }

            return false;
        }


        AZ::BehaviorMethod* FindBehaviorOperatorMethod(const AZ::BehaviorClass* behaviorClass, AZ::Script::Attributes::OperatorType operatorType)
        {
            AZ_Assert(behaviorClass, "Invalid AZ::BehaviorClass submitted to FindBehaviorOperatorMethod");
            for (auto&& equalMethodCandidatePair : behaviorClass->m_methods)
            {
                const AZ::AttributeArray& attributes = equalMethodCandidatePair.second->m_attributes;
                for (auto&& attributePair : attributes)
                {
                    if (attributePair.second->RTTI_IsTypeOf(AZ::AzTypeInfo<AZ::AttributeData<AZ::Script::Attributes::OperatorType>>::Uuid()))
                    {
                        auto&& attributeData = AZ::RttiCast<AZ::AttributeData<AZ::Script::Attributes::OperatorType>*>(attributePair.second);
                        if (attributeData->Get(nullptr) == operatorType)
                        {
                            return equalMethodCandidatePair.second;
                        }
                    }
                }
            }
            return nullptr;
        }

        bool IsAddressableType(const AZ::Uuid& uuid)
        {
            return !uuid.IsNull() && !AZ::BehaviorContext::IsVoidType(uuid);
        }

        bool ValidateAddressType(const AZ::Uuid& addressTypeId)
        {
            if (IsAddressableType(addressTypeId))
            {
                AZ::BehaviorContext* behaviorContext = nullptr;
                AZ::ComponentApplicationBus::BroadcastResult(behaviorContext, &AZ::ComponentApplicationBus::Events::GetBehaviorContext);

                if (auto&& behaviorClass = behaviorContext->m_typeToClassMap[addressTypeId])
                {
                    if (!behaviorClass->m_valueHasher)
                    {
                        AZ_Warning("Script Events", false, AZStd::string::format("Class %s with id %s must have an AZStd::hash<T> specialization to be a bus id", behaviorClass->m_name.c_str(), addressTypeId.ToString<AZStd::string>().c_str()).c_str());
                        return false;
                    }

                    if (!FindBehaviorOperatorMethod(behaviorClass, AZ::Script::Attributes::OperatorType::Equal) && !behaviorClass->m_equalityComparer)
                    {
                        AZ_Warning("Script Events", false, AZStd::string::format("Class %s with id %s does not have an operator equal defined to be a bus id", behaviorClass->m_name.c_str(), addressTypeId.ToString<AZStd::string>().c_str()).c_str());
                        return false;
                    }
                }
                else
                {
                    AZ::SerializeContext* serializeContext = nullptr;
                    AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationBus::Events::GetSerializeContext);
                    if (auto&& classData = serializeContext->FindClassData(addressTypeId))
                    {
                        AZ_Warning("Script Events", false, AZStd::string::format("Type %s with id %s not found in behavior context", classData->m_name, addressTypeId.ToString<AZStd::string>().c_str()).c_str());
                        return false;
                    }
                    else
                    {
                        AZ_Warning("Script Events", false, AZStd::string::format("Type with id %s not found in behavior context", addressTypeId.ToString<AZStd::string>().c_str()).c_str());
                        return false;
                    }
                }
            }
            return true;
        }
    }
}
