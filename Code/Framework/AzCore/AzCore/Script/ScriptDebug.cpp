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

#include <AzCore/Script/ScriptDebug.h>
#include <AzCore/Script/ScriptContext.h>
#include <AzCore/Script/ScriptSystemBus.h>
#include <AzCore/Outcome/Outcome.h>
#include <AzCore/RTTI/BehaviorContext.h>

#include <limits> // numeric_limits<double>::epsilon()

namespace AZ
{
    namespace Internal
    {
        // If no condition arguments failed, the result is successful and typeless.
        // If a condition failed, the result contains the index of the failed argument.
        using EvaluateArgumentsResult = AZ::Outcome<void, int>;

        //=========================================================================
        AZStd::string ExtractUserMessage(const ScriptDataContext& dc)
        {
            AZStd::string userMessage = "Condition failed";
            const int argCount = dc.GetNumArguments();
            if (argCount > 0 && dc.IsString(argCount - 1))
            {
                const char* value = nullptr;
                if (dc.ReadArg(argCount - 1, value))
                {
                    if (value)
                    {
                        userMessage = value;
                    }
                }
            }

            return userMessage;
        }

        //=========================================================================
        EvaluateArgumentsResult EvaluateArguments(const ScriptDataContext& dc)
        {
            const int argCount = dc.GetNumArguments();

            // Evaluate a variable number of values.
            for (int argIndex = 0; argIndex < argCount; ++argIndex)
            {
                if (dc.IsBoolean(argIndex))
                {
                    bool value = true;
                    dc.ReadArg(argIndex, value);
                    if (!value)
                    {
                        return AZ::Failure(argIndex);
                    }
                }
                else if (dc.IsNumber(argIndex))
                {
                    // With numerics it's ambiguous, so we're assuming the user is asserting that the number is >= ~0.0.
                    double value = 1.f;
                    dc.ReadArg(argIndex, value);
                    const double epsilon = std::numeric_limits<double>::epsilon();
                    if (value < epsilon)
                    {
                        return AZ::Failure(argIndex);
                    }
                }
                else if (dc.IsNil(argIndex))
                {
                    return AZ::Failure(argIndex);
                }
            }

            return AZ::Success();
        }

        //=========================================================================
        void ScriptLog(ScriptDataContext& dc)
        {
            dc.GetScriptContext()->Error(ScriptContext::ErrorType::Log, false, "%s", ExtractUserMessage(dc).c_str());
        }

        /**
         * Warning, Error, and Assert messages take variable condition arguments, allowing validation of multiple
         * values from script. A user message is optional, but if provided, should be the last argument.
         * Booleans, numerics, and tables are all supported for evaluation, and should any condition fail, the
         * index of the failing argument will be included in the message.
         *
         * Examples:
         *      Debug.Warning(someNumber > 5.0, "This isn't ideal.");
         *      Debug.Error(any, number, of, things, "Parameters are invalid.");
         *      Debug.Assert(criticalBool, criticalNumeric, "One of the critical values is in bad shape.");
         *
         * A failure will print as such:
         *      "[Warning/Error/Assert] on argument 0: your message"
         */

        //=========================================================================
        void ScriptWarning(ScriptDataContext& dc)
        {
            const EvaluateArgumentsResult result = EvaluateArguments(dc);

            if (!result.IsSuccess())
            {
                dc.GetScriptContext()->Error(ScriptContext::ErrorType::Warning, true, "Warning on argument %d: %s", result.GetError(), ExtractUserMessage(dc).c_str());
            }
        }

        //=========================================================================
        void ScriptError(ScriptDataContext& dc)
        {
            const EvaluateArgumentsResult result = EvaluateArguments(dc);

            if (!result.IsSuccess())
            {
                dc.GetScriptContext()->Error(ScriptContext::ErrorType::Error, true, "Error on argument %d: %s", result.GetError(), ExtractUserMessage(dc).c_str());
            }
        }

        //=========================================================================
        void ScriptAssert(ScriptDataContext& dc)
        {
            const EvaluateArgumentsResult result = EvaluateArguments(dc);

            if (!result.IsSuccess())
            {
                AZ_Assert(result, AZStd::string::format("Assert on argument %d: %s", result.GetError(), ExtractUserMessage(dc).c_str()).c_str());
            }
        }
    } // namespace Internal

    //=========================================================================
    // Reflect
    //=========================================================================
    void ScriptDebug::Reflect(AZ::ReflectContext* context)
    {
        AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context);
        if (behaviorContext)
        {
            behaviorContext->Class<ScriptDebug>("Debug")
                ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)
                ->Method("Log", &Internal::ScriptLog)
                ->Method("Warning", &Internal::ScriptWarning)
                ->Method("Error", &Internal::ScriptError)
                ->Method("Assert", &Internal::ScriptAssert)
            ;
        }
    }
} // namespace AZ

