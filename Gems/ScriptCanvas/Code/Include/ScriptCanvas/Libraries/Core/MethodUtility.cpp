/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/std/containers/map.h>
#include <ScriptCanvas/Libraries/Core/MethodUtility.h>
#include <ScriptCanvas/Libraries/Core/Method.h>

namespace ScriptCanvas
{
    AZStd::unordered_map<size_t, const AZ::BehaviorMethod*> GetTupleGetMethodsFromResult(const AZ::BehaviorMethod& method)
    {
        if (method.HasResult())
        {
            if (const AZ::BehaviorParameter* result = method.GetResult())
            {
                return GetTupleGetMethods(result->m_typeId);
            }
        }

        return {};
    }

    AZStd::unordered_map<size_t, const AZ::BehaviorMethod*> GetTupleGetMethods(const AZ::TypeId& typeId)
    {
        AZStd::unordered_map<size_t, const AZ::BehaviorMethod*> tupleGetMethodMap;

        AZ::BehaviorContext* behaviorContext{};
        AZ::ComponentApplicationBus::BroadcastResult(behaviorContext, &AZ::ComponentApplicationRequests::GetBehaviorContext);
        auto bcClassIterator = behaviorContext->m_typeToClassMap.find(typeId);
        const AZ::BehaviorClass* behaviorClass = bcClassIterator != behaviorContext->m_typeToClassMap.end() ? bcClassIterator->second : nullptr;

        if (behaviorClass)
        {
            for (auto& methodPair : behaviorClass->m_methods)
            {
                const AZ::BehaviorMethod* behaviorMethod = methodPair.second;
                if (AZ::Attribute* attribute = FindAttribute(AZ::ScriptCanvasAttributes::TupleGetFunctionIndex, behaviorMethod->m_attributes))
                {
                    AZ::AttributeReader tupleGetFuncAttrReader(nullptr, attribute);
                    int tupleGetFuncIndex = -1;
                    if (tupleGetFuncAttrReader.Read<int>(tupleGetFuncIndex))
                    {
                        auto insertIter = tupleGetMethodMap.emplace(tupleGetFuncIndex, behaviorMethod);
                        AZ_Error("Script Canvas", insertIter.second, "Multiple methods with the same TupleGetFunctionIndex attribute"
                            "has been registered for the class name: %s with typeid: %s",
                            behaviorClass->m_name.data(), behaviorClass->m_typeId.ToString<AZStd::string>().data())
                    }
                }
            }
        }

        return tupleGetMethodMap;
    }

    AZ::Outcome<const AZ::BehaviorMethod*, void> GetTupleGetMethod(const AZ::TypeId& typeId, size_t index)
    {
        const AZ::BehaviorContext* behaviorContext{};
        AZ::ComponentApplicationBus::BroadcastResult(behaviorContext, &AZ::ComponentApplicationRequests::GetBehaviorContext);
        auto bcClassIterator = behaviorContext->m_typeToClassMap.find(typeId);
        const AZ::BehaviorClass* behaviorClass = bcClassIterator != behaviorContext->m_typeToClassMap.end() ? bcClassIterator->second : nullptr;

        if (behaviorClass)
        {
            for (auto methodPair : behaviorClass->m_methods)
            {
                const AZ::BehaviorMethod* behaviorMethod = methodPair.second;
                if (AZ::Attribute* attribute = FindAttribute(AZ::ScriptCanvasAttributes::TupleGetFunctionIndex, behaviorMethod->m_attributes))
                {
                    AZ::AttributeReader tupleGetFuncAttrReader(nullptr, attribute);
                    int tupleGetFuncIndex = -1;
                    if (tupleGetFuncAttrReader.Read<int>(tupleGetFuncIndex) && index == tupleGetFuncIndex)
                    {
                        return AZ::Success(behaviorMethod);
                    }
                }
            }
        }

        return AZ::Failure();
    }

}
