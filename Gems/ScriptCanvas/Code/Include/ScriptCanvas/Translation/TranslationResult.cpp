/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "TranslationResult.h"

#include <ScriptCanvas/Core/Nodeable.h>
#include <ScriptCanvas/Grammar/AbstractCodeModel.h>

namespace TranslationResultCpp
{
    enum RuntimeInputsVersion
    {
        RemoveGraphType = 0,
        AddedStaticVariables,
        SupportMemberVariableInputs,
        ExecutionStateSelectionIncludesOnGraphStart,
        // add your entry above
        Current
    };
}

namespace ScriptCanvas
{
    namespace Translation
    {
        AZStd::sys_time_t SumDurations(const Translations& translation)
        {
            AZStd::sys_time_t result(0);

            for (const auto& t : translation)
            {
                result += t.second.m_duration;
            }

            return result;
        }

        Result::Result(AZStd::string invalidSourceInfo)
            : m_invalidSourceInfo(invalidSourceInfo)
            , m_parseDuration(0)
            , m_translationDuration(0)
        {}

        Result::Result(Grammar::AbstractCodeModelConstPtr model)
            : m_model(model)
            , m_parseDuration(m_model->GetParseDuration())
            , m_translationDuration(0)
        {}
    
        Result::Result(Grammar::AbstractCodeModelConstPtr model, Translations&& translations, Errors&& errors)
            : m_model(model)
            , m_translations(AZStd::move(translations))
            , m_errors(AZStd::move(errors))
            , m_parseDuration(m_model->GetParseDuration())
            , m_translationDuration(SumDurations(m_translations))
        {}

        AZStd::string Result::ErrorsToString() const
        {
            AZStd::string resultString;

            const ValidationResults::ValidationEventList& list = m_model->GetValidationEvents();

            for (auto& entry : list)
            {
                resultString += "* ";
                resultString += entry->GetDescription();
            }

            for (const auto& errors : m_errors)
            {
                for (auto& entry : errors.second)
                {
                    resultString += "* ";
                    resultString += entry->GetDescription();
                }
            }

            return resultString;
        }

        bool Result::IsSourceValid() const
        {
            return m_invalidSourceInfo.empty();
        }

        bool Result::IsModelValid() const 
        {
            return m_model->IsErrorFree();
        }

        AZ::Outcome<void, AZStd::string> Result::IsSuccess(TargetFlags /*flag*/) const
        {
            if (!IsSourceValid())
            {
                return AZ::Failure(AZStd::string("Graph translation source was invalid"));
            }
            else if (!IsModelValid())
            {
                return AZ::Failure(AZStd::string::format("Graph conversion to abstract code model failed: %s", ErrorsToString().c_str()));
            }
            else if (!TranslationSucceed(ScriptCanvas::Translation::Lua))
            {
                return AZ::Failure(AZStd::string::format("Graph translation to Lua failed: %s", ErrorsToString().c_str()));
            }
            else
            {
                return AZ::Success();
            }
        }

        bool Result::TranslationSucceed(TargetFlags flag) const
        {
            return m_translations.find(flag) != m_translations.end();
        }

        RuntimeInputs::RuntimeInputs(RuntimeInputs&& rhs)
        {
            *this = AZStd::move(rhs);
        }

        void RuntimeInputs::CopyFrom(const Grammar::ParsedRuntimeInputs& rhs)
        {
            m_nodeables = rhs.m_nodeables;
            m_variables = rhs.m_variables;
            m_entityIds = rhs.m_entityIds;
            m_staticVariables = rhs.m_staticVariables;
        }

        size_t RuntimeInputs::GetConstructorParameterCount() const
        {
            return m_nodeables.size() + m_variables.size() + m_entityIds.size();
        }

        RuntimeInputs& RuntimeInputs::operator=(RuntimeInputs&& rhs)
        {
            if (this != &rhs)
            {
                m_executionSelection = AZStd::move(rhs.m_executionSelection);
                m_nodeables = AZStd::move(rhs.m_nodeables);
                m_variables = AZStd::move(rhs.m_variables);
                m_entityIds = AZStd::move(rhs.m_entityIds);
                m_staticVariables = AZStd::move(rhs.m_staticVariables);
            }

            return *this;
        }

        void RuntimeInputs::Reflect(AZ::ReflectContext* reflectContext)
        {
            if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(reflectContext))
            {
                serializeContext->Class<RuntimeInputs>()
                    ->Version(TranslationResultCpp::RuntimeInputsVersion::Current)
                    ->Field("executionSelection", &RuntimeInputs::m_executionSelection)
                    ->Field("nodeables", &RuntimeInputs::m_nodeables)
                    ->Field("variables", &RuntimeInputs::m_variables)
                    ->Field("entityIds", &RuntimeInputs::m_entityIds)
                    ->Field("staticVariables", &RuntimeInputs::m_staticVariables)
                    ;
            }
        }

    } 

} 
