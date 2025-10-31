/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Script/ScriptContextAttributes.h>
#include <ScriptCanvas/Data/DataMacros.h>
#include <ScriptCanvas/Data/DataTraitBase.h>

namespace ContainerTypeReflection
{
    template<typename t_Type>
    struct BehaviorClassReflection
    {
    };

#define SCRIPT_CANVAS_REFLECT_SERIALIZATION_TYPE_AS_KEY_VALUE_TYPE(VALUE_TYPE, CONTAINER_TYPE)\
        serializeContext->RegisterGenericType<CONTAINER_TYPE <t_Type, VALUE_TYPE##Type>>();

#define SCRIPT_CANVAS_REFLECT_BEHAVIOR_TYPE_AS_KEY_METHOD(VALUE_TYPE, CONTAINER_TYPE)\
        ->Method("Reflect" #VALUE_TYPE #CONTAINER_TYPE "Func", [](const CONTAINER_TYPE < t_Type, VALUE_TYPE##Type >&){})

    template<typename KeyType, typename ValueType, bool isHashable = ScriptCanvas::Data::Traits<KeyType>::s_isKey>
    struct CreateTypeAsMapValueHelper
    {
        static void ReflectClassInfo([[maybe_unused]] AZ::SerializeContext* serializeContext)
        {
            // By default, do nothing. If the key is hashable, the template specialization will handle it.
        }

        static void AddMethod([[maybe_unused]] AZ::BehaviorContext::ClassBuilder<BehaviorClassReflection<ValueType>>* classBuilder)
        {
            // By default, do nothing. If the key is hashable, the template specialization will handle it.
        }
    };

    template<typename KeyType, typename ValueType>
    struct CreateTypeAsMapValueHelper<KeyType, ValueType, true>
    {
        static void ReflectClassInfo(AZ::SerializeContext* serializeContext)
        {
            if (auto genericClassInfo = AZ::SerializeGenericTypeInfo< AZStd::unordered_map<KeyType, ValueType>>::GetGenericInfo())
            {
                genericClassInfo->Reflect(serializeContext);
            }
        }

        static void AddMethod(AZ::BehaviorContext::ClassBuilder<BehaviorClassReflection<ValueType>>* classBuilder)
        {
            AZ::Uuid keyUuid = azrtti_typeid<KeyType>();
            AZ::Uuid valueUuid = azrtti_typeid<ValueType>();

            AZStd::string finalName = AZStd::string::format("Map_%s_to_%s_Func", keyUuid.ToString<AZStd::string>().c_str(), valueUuid.ToString<AZStd::string>().c_str());
            
            classBuilder->Method(finalName.c_str(), [](const AZStd::unordered_map<KeyType, ValueType>&) {});
        }
    };

#define SCRIPT_CANVAS_REFLECT_SERIALIZATION_TYPE_AS_VALUE_MAP_TYPE(VALUE_TYPE)\
        CreateTypeAsMapValueHelper<VALUE_TYPE##Type, t_Type>::ReflectClassInfo(serializeContext);

#define SCRIPT_CANVAS_REFLECT_BEHAVIOR_TYPE_AS_VALUE_MAP_METHOD(VALUE_TYPE)\
        CreateTypeAsMapValueHelper<VALUE_TYPE##Type, t_Type>::AddMethod(&classBuilder);

    template<typename t_Type>
    struct HashContainerReflector
    {
        static void Reflect(AZ::ReflectContext* reflectContext)
        {
            using namespace ScriptCanvas::Data;
            if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(reflectContext))
            {
                if constexpr (ScriptCanvas::Data::Traits<t_Type>::s_isKey)
                {
                    serializeContext->RegisterGenericType<AZStd::unordered_set<t_Type>>();
                    
                    // First Macro creates a list of all of the types, that is invoked using the second macro.
                    SCRIPT_CANVAS_PER_DATA_TYPE_1(SCRIPT_CANVAS_REFLECT_SERIALIZATION_TYPE_AS_KEY_VALUE_TYPE, AZStd::unordered_map);

                    if constexpr (ScriptCanvas::Data::Traits<t_Type>::s_type == eType::BehaviorContextObject)
                    {
                        // First Macro creates a list of all of the types, that is invoked using the second macro.
                        SCRIPT_CANVAS_PER_DATA_TYPE(SCRIPT_CANVAS_REFLECT_SERIALIZATION_TYPE_AS_VALUE_MAP_TYPE);
                    }
                }
            }

            if (auto* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(reflectContext))
            {
                AZ::BehaviorClass* behaviorClass = AZ::BehaviorContextHelper::GetClass(behaviorContext, azrtti_typeid<BehaviorClassReflection<t_Type>>());
                AZ::BehaviorContext::ClassBuilder<BehaviorClassReflection<t_Type>> classBuilder(behaviorContext, behaviorClass);

                if constexpr (ScriptCanvas::Data::Traits<t_Type>::s_isKey)
                {
                    classBuilder->Method("ReflectSet", [](const AZStd::unordered_set<t_Type>&) {})

                        // First Macro creates a list of all of the types, that is invoked using the second macro.
                        SCRIPT_CANVAS_PER_DATA_TYPE_1(SCRIPT_CANVAS_REFLECT_BEHAVIOR_TYPE_AS_KEY_METHOD, AZStd::unordered_map)
                        ;
                }

                if constexpr (ScriptCanvas::Data::Traits<t_Type>::s_type == eType::BehaviorContextObject)
                {
                    SCRIPT_CANVAS_PER_DATA_TYPE(SCRIPT_CANVAS_REFLECT_BEHAVIOR_TYPE_AS_VALUE_MAP_METHOD)
                }
            }
        }
    }; 

#undef SCRIPT_CANVAS_REFLECT_SERIALIZATION_TYPE_AS_KEY_VALUE_TYPE
#undef SCRIPT_CANVAS_REFLECT_BEHAVIOR_TYPE_AS_KEY_METHOD
#undef SCRIPT_CANVAS_REFLECT_SERIALIZATION_TYPE_AS_VALUE_MAP_TYPE
#undef SCRIPT_CANVAS_REFLECT_BEHAVIOR_TYPE_AS_VALUE_MAP_METHOD

    template<typename t_Type>
    struct TraitsReflector
    {
        static void Reflect(AZ::ReflectContext* reflectContext)
        {
            if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(reflectContext))
            {
                if (auto genericClassInfo = AZ::SerializeGenericTypeInfo<AZStd::vector<t_Type>>::GetGenericInfo())
                {
                    genericClassInfo->Reflect(serializeContext);
                }
            }

            if (auto* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(reflectContext))
            {
                behaviorContext->Class<BehaviorClassReflection<t_Type>>(AZStd::string::format("ReflectOnDemandTargets_%s", ScriptCanvas::Data::Traits<t_Type>::GetName().data()).data())
                    ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)
                    ->Attribute(AZ::Script::Attributes::Ignore, true)
                    ->Method("ReflectVector", [](const AZStd::vector<t_Type>&) {})
                    ;
            }

            HashContainerReflector<t_Type>::Reflect(reflectContext);
        }
    };
}

namespace AZ
{
    AZ_TYPE_INFO_TEMPLATE_WITH_NAME(ContainerTypeReflection::BehaviorClassReflection, "(BehaviorClassReflection<t_Type>)", "{0EADF8F5-8AB8-42E9-9C50-F5C78255C817}", AZ_TYPE_INFO_TYPENAME);
}
