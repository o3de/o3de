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
    int CountMatchingInputTypes(const Grammar::FunctionPrototype& a, const Grammar::FunctionPrototype& b, const AZStd::vector<size_t>& checkedIndices)
    {
        AZ_Warning("ScriptCanvas", a.m_inputs.size() == b.m_inputs.size(), "Function inputs are not the same size");

        int matching = 0;

        if (a.m_inputs.size() == b.m_inputs.size())
        {
            if (checkedIndices.empty())
            {
                for (size_t index = 0; index < a.m_inputs.size(); ++index)
                {
                    if (!a.m_inputs[index]->m_datum.GetType().IsValid()
                        || a.m_inputs[index]->m_datum.GetType() == b.m_inputs[index]->m_datum.GetType())
                    {
                        ++matching;
                    }
                }
            }
            else
            {
                for (const auto checkedIndex : checkedIndices)
                {
                    const auto index = checkedIndices[checkedIndex];

                    if (index < a.m_inputs.size())
                    {
                        if (!a.m_inputs[index]->m_datum.GetType().IsValid()
                            || a.m_inputs[index]->m_datum.GetType() == b.m_inputs[index]->m_datum.GetType())
                        {
                            ++matching;
                        }
                    }
                    else
                    {
                        AZ_Error("ScriptCanvas", false, "Overload checked index is no longer valid");
                    }
                }
            }
        }

        return matching;
    }

    BehaviorContextMethodHelper::CallResult BehaviorContextMethodHelper::Call(Node& node, bool isExpectingMultipleResults, const AZ::BehaviorMethod* method, AZ::BehaviorValueParameter* paramBegin, AZ::BehaviorValueParameter* paramEnd, AZStd::vector<SlotId>& resultSlotIds)
    {
        if (isExpectingMultipleResults)
        {
            return BehaviorContextMethodHelper::Call(node, method, paramBegin, paramEnd, resultSlotIds);
        }
        else
        {
            return BehaviorContextMethodHelper::Call(node, method, paramBegin, paramEnd, resultSlotIds[0]);
        }
    }

    BehaviorContextMethodHelper::CallResult BehaviorContextMethodHelper::Call(Node& node, const AZ::BehaviorMethod* method, AZ::BehaviorValueParameter* paramBegin, AZ::BehaviorValueParameter* paramEnd, SlotId resultSlotId)
    {
        AZ_PROFILE_SCOPE_DYNAMIC(AZ::Debug::ProfileCategory::ScriptCanvas, "ScriptCanvas::Method::OnInputSignal::Call %s", method->m_name.c_str());
        return CallGeneric(node, method, paramBegin, paramEnd, &AttemptCallWithResults, resultSlotId);
    }

    BehaviorContextMethodHelper::CallResult BehaviorContextMethodHelper::Call(Node& node, const AZ::BehaviorMethod* method, AZ::BehaviorValueParameter* paramBegin, AZ::BehaviorValueParameter* paramEnd, AZStd::vector<SlotId>& resultSlotIds)
    {
        AZ_PROFILE_SCOPE_DYNAMIC(AZ::Debug::ProfileCategory::ScriptCanvas, "ScriptCanvas::Method::OnInputSignal::Call %s", method->m_name.c_str());
        return CallGeneric(node, method, paramBegin, paramEnd, &AttemptCallWithTupleResults, resultSlotIds);
    }

    AZ::Outcome<BehaviorContextMethodHelper::CallResult, AZStd::string> BehaviorContextMethodHelper::AttemptCallWithResults(Node&, const AZ::BehaviorMethod*, AZ::BehaviorValueParameter*, unsigned int, SlotId)
    {
        return AZ::Failure(AZStd::string("This method is slated for deletion"));
    }

    AZ::Outcome<BehaviorContextMethodHelper::CallResult, AZStd::string> BehaviorContextMethodHelper::AttemptCallWithTupleResults(Node&, const AZ::BehaviorMethod*, AZ::BehaviorValueParameter*, unsigned int, AZStd::vector<SlotId>)
    {
        return AZ::Failure(AZStd::string("This method is slated for deletion"));
    }

    AZ::Outcome<Datum, AZStd::string> BehaviorContextMethodHelper::CallTupleGetMethod(const AZ::BehaviorMethod*, Datum&)
    {
        return AZ::Failure(AZStd::string("This method is slated for deletion"));
    }

    AZ::Outcome<BehaviorContextMethodHelper::CallResult, AZStd::string> BehaviorContextMethodHelper::CallOutcomeTupleMethod(Node&, const SlotId&, Datum&, size_t, AZStd::string)
    {
        return AZ::Failure(AZStd::string("to be deleted"));
    }

    AZ::BehaviorValueParameter BehaviorContextMethodHelper::ToBehaviorValueParameter(const AZ::BehaviorMethod* method, size_t index, const Datum& datum)
    {
        const AZ::BehaviorParameter* datumParam = method->GetArgument(index);
        auto toParamOutcome = datumParam ? datum.ToBehaviorValueParameter(*datumParam) : AZ::Failure(AZStd::string::format("BehaviorMethod contains nullptr BehaviorParameter at index %zu", index));
        if (toParamOutcome)
        {
            return AZ::BehaviorValueParameter(toParamOutcome.TakeValue());
        }
        return AZ::BehaviorValueParameter();
    }

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
