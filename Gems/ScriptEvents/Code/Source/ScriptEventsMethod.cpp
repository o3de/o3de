/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "ScriptEvents/ScriptEventsMethod.h"

#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/string/regex.h>

namespace ScriptEvents
{
    void Method::FromScript(AZ::ScriptDataContext& dc)
    {
        if (dc.GetNumArguments() > 0)
        {
            AZStd::string name;
            if (dc.IsString(0) && dc.ReadArg(0, name))
            {
                m_name.Set(name.c_str());
            }

            if (dc.GetNumArguments() > 1)
            {
                AZ::Uuid returnType;
                if (dc.ReadArg(1, returnType))
                {
                    m_returnType.Set(returnType);
                }
            }
        }

        // AZ_TracePrintf("Script Events", "Added Script Method: %s (return type: %s)\n", GetName().c_str(), m_returnType.IsEmpty() ? "none"
        // : GetReturnType().ToString<AZStd::string>().c_str());
    }
    void Method::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<Method>()
                ->Field("m_name", &Method::m_name)
                ->Field("m_tooltip", &Method::m_tooltip)
                ->Field("m_returnType", &Method::m_returnType)
                ->Field("m_parameters", &Method::m_parameters);

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<Method>("Script Event", "A script event's definition")
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &Method::m_name, "Name",
                        "The specified name for this event, represents a callable function (i.e. MyScriptEvent())")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &Method::m_tooltip, "Tooltip", "A description of this event")
                    ->DataElement(
                        AZ::Edit::UIHandlers::ComboBox, &Method::m_returnType, "Return value type",
                        "the typeid of the return value, ex. AZ::type_info<int>::Uuid foo()")
                    ->Attribute(AZ::Edit::Attributes::GenericValueList, &Types::GetValidReturnTypes)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &Method::m_parameters, "Parameters",
                        "A list of parameters for the EBus event, ex. void foo(Parameter1, Parameter2)");
            }
        }

        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Class<Method>("Method")
                ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)
                ->Method("AddParameter", &Method::AddParameter)
                ->Property("Name", BehaviorValueProperty(&Method::m_name))
                ->Property("ReturnType", BehaviorValueProperty(&Method::m_returnType))
                ->Property("Parameters", BehaviorValueProperty(&Method::m_parameters));
        }
    }

    AZ::Outcome<bool, AZStd::string> Method::Validate() const
    {
        const AZStd::string name = GetName();
        const AZ::Uuid returnType = GetReturnType();

        // Validate address type
        if (!Types::IsValidReturnType(returnType))
        {
            return AZ::Failure(AZStd::string::format(
                "The specified type %s is not valid as return type for Script Event: %s", returnType.ToString<AZStd::string>().c_str(),
                name.c_str()));
        }

        // Definition name cannot be empty
        if (name.empty())
        {
            return AZ::Failure(AZStd::string("Definition name cannot be empty"));
        }

        // Name cannot start with a number
        if (isdigit(name.at(0)))
        {
            return AZ::Failure(AZStd::string::format("%s, names cannot start with a number", name.c_str()));
        }

        // Conform to valid function names
        AZStd::smatch match;

        // Ascii-only
        AZStd::regex asciionly_regex("[^\x0A\x0D\x20-\x7E]");
        AZStd::regex_match(name, match, asciionly_regex);
        if (!match.empty())
        {
            return AZ::Failure(AZStd::string::format("%s, invalid name, names may only contain ASCII characters", name.c_str()));
        }

        AZStd::regex validate_regex("[_[:alpha:]][_[:alnum:]]*");
        AZStd::regex_match(name, match, validate_regex);
        if (match.empty())
        {
            return AZ::Failure(AZStd::string::format("%s, invalid name specified", name.c_str()));
        }

        AZStd::string parameterName;
        int parameterIndex = 0;
        for (const Parameter& parameter : m_parameters)
        {
            auto outcome = parameter.Validate();
            if (!outcome.IsSuccess())
            {
                return outcome;
            }

            int duplicateCount = 0;
            for (auto item = m_parameters.begin(); item != m_parameters.end(); item++)
            {
                if (item->GetName().compare(parameterName) == 0)
                {
                    duplicateCount++;
                }
                if (duplicateCount > 1)
                {
                    return AZ::Failure(AZStd::string::format(
                        "Cannot have duplicate parameter names (%d: %s) make sure each parameter name is unique",
                        parameterIndex,
                        parameterName.c_str()));
                }
            }

            parameterName = parameter.GetName();
            ++parameterIndex;
        }

        return AZ::Success(true);
    }

    void Method::PreSave()
    {
        m_name.PreSave();
        m_tooltip.PreSave();
        m_returnType.PreSave();

        for (Parameter parameter : m_parameters)
        {
            parameter.PreSave();
        }
    }

    void Method::Flatten()
    {
        m_name.Flatten();
        m_tooltip.Flatten();
        m_returnType.Flatten();
        for (Parameter& parameter : m_parameters)
        {
            parameter.Flatten();
        }
    }
} // namespace ScriptEvents
