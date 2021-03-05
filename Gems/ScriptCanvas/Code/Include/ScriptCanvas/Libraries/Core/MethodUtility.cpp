/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#include <AzCore/std/containers/map.h>
#include <ScriptCanvas/Libraries/Core/MethodUtility.h>
#include <ScriptCanvas/Libraries/Core/Method.h>

namespace ScriptCanvas
{

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

    AZ::Outcome<BehaviorContextMethodHelper::CallResult, AZStd::string> BehaviorContextMethodHelper::AttemptCallWithResults(Node& node, const AZ::BehaviorMethod* method, AZ::BehaviorValueParameter* params, unsigned int numExpectedArgs, SlotId resultSlotId)
    {
        auto resultSlot = resultSlotId.IsValid() ? node.GetSlot(resultSlotId) : nullptr;
        const AZ::BehaviorParameter* resultType = method->GetResult();
        if (method->HasResult() && resultType && resultSlot)
        {
            const AZ::Outcome<Datum, AZStd::string> result = Datum::CallBehaviorContextMethodResult(method, resultType, params, numExpectedArgs);

            if (result.IsSuccess())
            {
                node.PushOutput(result.GetValue(), (*resultSlot));
                return AZ::Success(CallResult(MethodCallStatus::Attempted));
            }
            else
            {
                // it is not fine to fail the call attempt
                return AZ::Failure(result.GetError());
            }
        }
        else if (resultSlotId.IsValid())
        {
            return AZ::Failure(AZStd::string::format("Script Canvas attempt to call %s failed, valid slot ID passed in, but no slot found for it", method->m_name.c_str()));
        }
        // it is fine for the method to have no result, under any other condition
        return AZ::Success(CallResult(MethodCallStatus::NotAttempted));
    }

    AZ::Outcome<BehaviorContextMethodHelper::CallResult, AZStd::string> BehaviorContextMethodHelper::AttemptCallWithTupleResults(Node& node, const AZ::BehaviorMethod* method, AZ::BehaviorValueParameter* params, unsigned int numExpectedArgs, AZStd::vector<SlotId> resultSlotIds)
    {
        AZ_Assert(resultSlotIds.size() < BehaviorContextInputOutput::MaxCount, "Result slot id size is too large");

        const AZ::BehaviorParameter* resultType = method->GetResult();

        if (method->HasResult() && resultType)
        {
            bool validSlotIdFound = false;

            for (const auto& slotId : resultSlotIds)
            {
                validSlotIdFound = validSlotIdFound || slotId.IsValid();
            }

            AZ::Outcome<Datum, AZStd::string> methodCallResult = Datum::CallBehaviorContextMethodResult(method, resultType, params, numExpectedArgs);
            if (methodCallResult.IsSuccess())
            {
                if (AZ::FindAttribute(AZ::ScriptCanvasAttributes::AutoUnpackOutputOutcomeSlots, method->m_attributes))
                {
                    AZ::Outcome<Datum, AZStd::string> isSuccessCallResult = CallMethodOnDatum(methodCallResult.GetValue(), "IsSuccess");
                    if (isSuccessCallResult.IsSuccess())
                    {
                        if (*isSuccessCallResult.GetValue().GetAs<bool>())
                        {
                            // the AZ::Outcome result of the method call is a success
                            return CallOutcomeTupleMethod(node, resultSlotIds.front(), methodCallResult.GetValue(), 0, "Success");
                        }
                        else
                        {
                            AZStd::string failureName{ "Failure" };
                            if (auto failureOverridePtr = AZ::FindAttribute(AZ::ScriptCanvasAttributes::AutoUnpackOutputOutcomeFailureSlotName, method->m_attributes))
                            {
                                AZ::AttributeReader(nullptr, failureOverridePtr).Read<AZStd::string>(failureName);
                            }

                            // the AZ::Outcome result of the method call is a failure
                            return CallOutcomeTupleMethod(node, resultSlotIds.back(), methodCallResult.GetValue(), 1, failureName);
                        }
                    }
                    else
                    {
                        return AZ::Failure(AZStd::string::format("Script Canvas attempt to call %s failed, Failed to query result Outcome success", method->m_name.c_str()));
                    }
                }
                else
                {
                    return AZ::Failure(AZStd::string::format("Script Canvas attempt to call %s failed, multiple results requested, but no unpack attribute has been used", method->m_name.c_str()));
                }
            }
            else
            {
                return AZ::Failure(methodCallResult.GetError());
            }

            if (validSlotIdFound)
            {
                return AZ::Failure(AZStd::string::format("Script Canvas attempt to call %s failed, valid slot ID passed in, but no slot found for it", method->m_name.c_str()));
            }
        }

        // it is fine for the method to have no result, under any other condition
        return AZ::Success(CallResult(MethodCallStatus::NotAttempted));
    }

    AZ::Outcome<Datum, AZStd::string> BehaviorContextMethodHelper::CallTupleGetMethod(const AZ::BehaviorMethod* method, Datum& thisPointer)
    {
        auto argument = method->GetArgument(0);
        if (!argument)
        {
            return AZ::Failure(AZStd::string("Invalid tuple get method, it doesn't take an argument"));
        }

        AZ::Outcome<AZ::BehaviorValueParameter, AZStd::string> bvpOutcome = thisPointer.ToBehaviorValueParameter(*argument);

        if (!bvpOutcome.IsSuccess())
        {
            return AZ::Failure(bvpOutcome.GetError());
        }

        return Datum::CallBehaviorContextMethodResult(method, method->GetResult(), &bvpOutcome.GetValue(), 1);
    }

    AZ::Outcome<BehaviorContextMethodHelper::CallResult, AZStd::string> BehaviorContextMethodHelper::CallOutcomeTupleMethod(Node& node, const SlotId& resultSlotId, Datum& outcomeDatum, size_t index, AZStd::string outSlotName)
    {
        auto methodNode = azrtti_cast<Nodes::Core::Method*>(&node);
        if (!methodNode)
        {
            return AZ::Failure(AZStd::string("Only ScriptCanvas Method nodes support returning a tuple from a BehaviorContext function"));
        }

        const auto& outcomeType = outcomeDatum.GetType().GetAZType();
        auto tupleGetMethodIt = methodNode->m_tupleGetMethods.find(index);
        if (tupleGetMethodIt == methodNode->m_tupleGetMethods.end())
        {
            // When a get method does not exist for a tuple like structure, assume that the call is successful.
            return AZ::Success(CallResult(MethodCallStatus::Attempted, outSlotName));
        }

        // a nullptr method just means a void type, which is acceptable, just no call attempt is made
        if (const AZ::BehaviorMethod* method = tupleGetMethodIt->second)
        {
            if (!method->HasResult())
            {
                return AZ::Failure(AZStd::string("Invalid tuple get method for a valid type of AZ::Outcome"));
            }

            const AZ::Outcome<Datum, AZStd::string> tupleGetResult = CallTupleGetMethod(method, outcomeDatum);

            if (tupleGetResult.IsSuccess())
            {
                if (Slot* resultSlot = methodNode->GetSlot(resultSlotId))
                {
                    methodNode->PushOutput(tupleGetResult.GetValue(), *resultSlot);
                }
            }
            else
            {
                return AZ::Failure(tupleGetResult.GetError());
            }
        }

        return AZ::Success(CallResult(MethodCallStatus::Attempted, outSlotName));
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

    AZStd::map<size_t, const AZ::BehaviorMethod*> GetTupleGetMethods(const AZ::TypeId& typeId)
    {
        AZStd::map<size_t, const AZ::BehaviorMethod*> tupleGetMethodMap;
        
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

} // namespace ScriptCanvas