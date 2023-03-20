/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <ScriptCanvas/Grammar/AbstractCodeModel.h>
#include <ScriptCanvas/Translation/TranslationResult.h>

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
    } 
} 
