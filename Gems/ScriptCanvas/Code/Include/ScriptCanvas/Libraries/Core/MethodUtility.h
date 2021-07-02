/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <ScriptCanvas/Core/Graph.h>
#include <ScriptCanvas/Core/Node.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/std/containers/map.h>

namespace ScriptCanvas::Grammar
{
    struct FunctionPrototype;
}

namespace ScriptCanvas
{
    int CountMatchingInputTypes(const Grammar::FunctionPrototype& a, const Grammar::FunctionPrototype& b, const AZStd::vector<size_t>& checkedIndices);

    struct BehaviorContextMethodHelper
    {
        enum BehaviorContextInputOutput : size_t
        {
            MaxCount = 40,
        };

        enum class MethodCallStatus
        {
            NotAttempted = 0,
            Attempted,
            Failed,
            Succeeded,
        };
        
        struct CallResult
        {
            const MethodCallStatus m_status = MethodCallStatus::NotAttempted;
            const AZStd::string m_executionOutOverride = "Out";

            AZ_INLINE CallResult(const MethodCallStatus status, AZStd::string executionOutOverride = "Out")
                : m_status(status)
                , m_executionOutOverride(executionOutOverride)
            {}
        };

        template<typename t_Call, typename t_Slots>
        static CallResult CallGeneric(Node& node, const AZ::BehaviorMethod* method, AZ::BehaviorValueParameter* paramBegin, AZ::BehaviorValueParameter* paramEnd, t_Call attempt, t_Slots& slots);
        
        static CallResult Call(Node& node, bool isExpectingMultipleResults, const AZ::BehaviorMethod* method, AZ::BehaviorValueParameter* paramBegin, AZ::BehaviorValueParameter* paramEnd, AZStd::vector<SlotId>& resultSlotIds);
        static CallResult Call(Node& node, const AZ::BehaviorMethod* method, AZ::BehaviorValueParameter* paramBegin, AZ::BehaviorValueParameter* paramEnd, SlotId resultSlotId);
        static CallResult Call(Node& node, const AZ::BehaviorMethod* method, AZ::BehaviorValueParameter* paramBegin, AZ::BehaviorValueParameter* paramEnd, AZStd::vector<SlotId>& resultSlotIds);
        static AZ::Outcome<CallResult, AZStd::string> AttemptCallWithResults(Node& node, const AZ::BehaviorMethod* method, AZ::BehaviorValueParameter* params, unsigned int numExpectedArgs, SlotId resultSlotId);
        static AZ::Outcome<CallResult, AZStd::string> AttemptCallWithTupleResults(Node& node, const AZ::BehaviorMethod* method, AZ::BehaviorValueParameter* params, unsigned int numExpectedArgs, AZStd::vector<SlotId> resultSlotIds);
        
        template<typename ... Args>
        static AZ::Outcome<Datum, AZStd::string> CallMethodOnDatum(const Datum& input, AZStd::string_view methodName, Args&& ... args);

        template<typename ... Args>
        static AZ::Outcome<Datum, AZStd::string> CallMethodOnDatumUnpackOutcomeSuccess(const Datum& input, AZStd::string_view methodName, Args&& ... args);
                
        template<typename ... Args>
        static AZStd::vector<AZ::BehaviorValueParameter> CreateParameterList(const AZ::BehaviorMethod* method, Args&&... args);

        static AZ::Outcome<Datum, AZStd::string> CallTupleGetMethod(const AZ::BehaviorMethod* method, Datum& thisPointer);

    private:
        static AZ::Outcome<CallResult, AZStd::string> CallOutcomeTupleMethod(Node& node, const SlotId& resultSlotId, Datum& outcomeDatum, size_t index, AZStd::string outSlotName);
        static AZ::BehaviorValueParameter ToBehaviorValueParameter(const AZ::BehaviorMethod* method, size_t index, const Datum& datum);

        template <typename T>
        static AZ::BehaviorValueParameter ToBehaviorValueParameter(const AZ::BehaviorMethod*, size_t, const T& arg);

        template<typename ... Args, size_t ... Indices>
        static AZStd::vector<AZ::BehaviorValueParameter> CreateParameterListInternal(const AZ::BehaviorMethod* method, AZStd::index_sequence<Indices...>, Args&&... args);
    };

    AZStd::unordered_map<size_t, const AZ::BehaviorMethod*> GetTupleGetMethodsFromResult(const AZ::BehaviorMethod& method);

    AZStd::unordered_map<size_t, const AZ::BehaviorMethod*> GetTupleGetMethods(const AZ::TypeId& typeId);

    AZ::Outcome<const AZ::BehaviorMethod*, void> GetTupleGetMethod(const AZ::TypeId& typeID, size_t index);

    template<typename t_Call, typename t_Slots>
    BehaviorContextMethodHelper::CallResult BehaviorContextMethodHelper::CallGeneric(Node& node, const AZ::BehaviorMethod* method, AZ::BehaviorValueParameter* paramBegin, AZ::BehaviorValueParameter* paramEnd, t_Call attempt, t_Slots& slots)
    {
        const auto numExpectedArgs(static_cast<unsigned int>(method->GetNumArguments()));
        if ((paramEnd - paramBegin) == numExpectedArgs)
        {
            AZ::Outcome<CallResult, AZStd::string> withResultsOutcome = attempt(node, method, paramBegin, numExpectedArgs, slots);

            if (!withResultsOutcome.IsSuccess())
            {
                SCRIPTCANVAS_REPORT_ERROR((node), "Script Canvas attempt to call %s with a result failed: %s", method->m_name.data(), withResultsOutcome.GetError().data());
                return CallResult(MethodCallStatus::Failed);
            }
            else if (withResultsOutcome.GetValue().m_status == MethodCallStatus::Attempted)
            {
                return CallResult(MethodCallStatus::Succeeded, withResultsOutcome.GetValue().m_executionOutOverride);
            }
            else if (method->Call(paramBegin, numExpectedArgs))
            {
                return CallResult(MethodCallStatus::Succeeded);
            }
            else
            {
                SCRIPTCANVAS_REPORT_ERROR((node), "Script Canvas attempt to call %s failed", method->m_name.data());
                return CallResult(MethodCallStatus::Failed);
            }
        }
        else
        {
            SCRIPTCANVAS_REPORT_ERROR((node), "Script Canvas attempt to call %s failed, it expects %d args but called with %d", method->m_name.c_str(), numExpectedArgs, paramEnd - paramBegin);
            return CallResult(MethodCallStatus::NotAttempted);
        }
    }

    template<typename ... Args>
    AZ::Outcome<Datum, AZStd::string> BehaviorContextMethodHelper::CallMethodOnDatumUnpackOutcomeSuccess(const Datum& input, AZStd::string_view methodName, Args&& ... args)
    {
        AZ::Outcome<Datum, AZStd::string> methodCallResult = CallMethodOnDatum(input, methodName, args...);
        if (methodCallResult.IsSuccess())
        {
            // Even when successfully called, the method will return an outcome as the result, we need to test that
            // outcome in case any errors were returned from the invoked function call.
            const Datum& methodCallResultDatum = methodCallResult.GetValue();

            const auto* actualOutcome = methodCallResultDatum.GetAs<AZ::Outcome<AZ::BehaviorValueParameter, AZStd::string>>();
            if (actualOutcome && !actualOutcome->IsSuccess())
            {
                return AZ::Failure(AZStd::string::format("%s returned an error: %s", methodName.data(), actualOutcome->GetError().c_str()));
            }

            AZ::Outcome<Datum, AZStd::string> isSuccessCallResult = CallMethodOnDatum(methodCallResult.GetValue(), "IsSuccess");
            if (isSuccessCallResult.IsSuccess())
            {
                if (*isSuccessCallResult.GetValue().GetAs<bool>())
                {
                    return CallMethodOnDatum(methodCallResult.GetValue(), "GetValue");
                }
                else
                {
                    return AZ::Failure(AZStd::string::format("%s returned an error", methodName.data()));
                }
            }
            else
            {
                return AZ::Failure(AZStd::string::format("Script Canvas attempt to call %s failed, Failed to query result Outcome success", methodName.data()));
            }
        }
        else
        {
            return AZ::Failure(methodCallResult.GetError());
        }
        
        return methodCallResult;
    }

    template<typename ... Args>
    AZ::Outcome<Datum, AZStd::string> BehaviorContextMethodHelper::CallMethodOnDatum(const Datum& input, AZStd::string_view methodName, Args&& ... args)
    {
        const AZ::BehaviorClass* behaviorClass = AZ::BehaviorContextHelper::GetClass(input.GetType().GetAZType());
        if (behaviorClass)
        {
            auto methodIt = behaviorClass->m_methods.find(methodName);

            if (methodIt != behaviorClass->m_methods.end())
            {
                AZ::BehaviorMethod* method = methodIt->second;

                const AZ::BehaviorParameter* resultType = method->HasResult() ? method->GetResult() : nullptr;

                if (resultType)
                {
                    // Populate parameters
                    AZStd::vector<AZ::BehaviorValueParameter> parameters = CreateParameterList(method, input, AZStd::forward<Args>(args)...);
                    return Datum::CallBehaviorContextMethodResult(method, resultType, parameters.begin(), (unsigned int)AZStd::GetMin(parameters.size(), method->GetNumArguments()), behaviorClass->m_name);
                }
                else
                {
                    AZStd::vector<AZ::BehaviorValueParameter> parameters = CreateParameterList(method, input, AZStd::forward<Args>(args)...);
                    if (Datum::CallBehaviorContextMethod(method, parameters.begin(), (unsigned int)AZStd::GetMin(parameters.size(), method->GetNumArguments())).IsSuccess())
                    {
                        return AZ::Success(Datum());
                    }
                }
            }
            else
            {
                return AZ::Failure(AZStd::string::format("ScriptCanvas Behavior Context method call failed; method named \"%s\" not found.", methodName.data()));
            }
        }

        return AZ::Failure(AZStd::string::format("ScriptCanvas Behavior Context method call failed; unable to retrieve Behavior Class."));
    }

    template<typename ... Args>
    AZStd::vector<AZ::BehaviorValueParameter> BehaviorContextMethodHelper::CreateParameterList(const AZ::BehaviorMethod* method, Args&&... args)
    {
        // Create an index sequence for the parameters and forward the arguments
        return CreateParameterListInternal(method, AZStd::make_index_sequence<sizeof...(Args)>{}, AZStd::forward<Args>(args)...);
    }

    template <typename T>
    AZ::BehaviorValueParameter BehaviorContextMethodHelper::ToBehaviorValueParameter(const AZ::BehaviorMethod*, size_t, const T& arg)
    {
        return AZ::BehaviorValueParameter(&arg);
    }

    template<typename ... Args, size_t ... Indices>
    AZStd::vector<AZ::BehaviorValueParameter> BehaviorContextMethodHelper::CreateParameterListInternal(const AZ::BehaviorMethod* method, AZStd::index_sequence<Indices...>, Args&&... args)
    {
        // Unpack the arguments and the index sequence to populate the parameter list
        return { ToBehaviorValueParameter(method, Indices, args)... };
    }

} 
