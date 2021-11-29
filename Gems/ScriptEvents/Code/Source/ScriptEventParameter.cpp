/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "ScriptEvents/ScriptEventParameter.h"

#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/string/regex.h>

namespace ScriptEvents
{
    void Parameter::FromScript(AZ::ScriptDataContext& dc)
    {
        if (dc.GetNumArguments() > 0)
        {
            AZStd::string name;
            if (dc.ReadArg(0, name))
            {
                m_name.Set(name.c_str());
            }

            if (dc.GetNumArguments() > 1)
            {
                AZ::Uuid parameterType;
                if (dc.ReadArg(1, parameterType))
                {
                    m_type.Set(parameterType);
                }
            }
        }

        // AZ_TracePrintf("Script Events", "Added Parameter: %s (type: %s)\n", GetName().c_str(), GetType().ToString<AZStd::string>()
        // .c_str());
    }

    void Parameter::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<Parameter>()
                ->Field("m_name", &Parameter::m_name)
                ->Field("m_tooltip", &Parameter::m_tooltip)
                ->Field("m_type", &Parameter::m_type);

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<Parameter>("A Script Event's method parameter", "A parameter to a Script Event's event definition")
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &Parameter::m_name, "Name",
                        "Name of the parameter, ex. void foo(int thisIsTheParameterName)")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &Parameter::m_tooltip, "Tooltip", "A description of this parameter")
                    ->DataElement(
                        AZ::Edit::UIHandlers::ComboBox, &Parameter::m_type, "Type",
                        "The typeid of the parameter, ex. void foo(AZ::type_info<int>::Uuid())")
                    ->Attribute(AZ::Edit::Attributes::GenericValueList, &Types::GetValidParameterTypes);
            }
        }

        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Class<Parameter>("Parameter")
                ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)
                ->Property("Name", BehaviorValueProperty(&Parameter::m_name))
                ->Property("Type", BehaviorValueProperty(&Parameter::m_type));
        }
    }

    AZ::Outcome<bool, AZStd::string> Parameter::Validate() const
    {
        const AZStd::string& name = GetName();
        const AZ::Uuid* parameterType = m_type.Get<const AZ::Uuid>();

        AZ_Assert(parameterType && !parameterType->IsNull(), "The Parameter type should not be null");

        // Validate address type
        if (!Types::IsValidParameterType(*parameterType))
        {
            return AZ::Failure(AZStd::string::format(
                "The specified type %s is not valid as parameter type for Script Event: %s",
                (*parameterType).ToString<AZStd::string>().c_str(), name.c_str()));
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
        if (match.size() > 0)
        {
            return AZ::Failure(AZStd::string::format("%s, invalid name, names may only contain ASCII characters", name.c_str()));
        }

        // Function name syntax
        AZStd::regex validate_regex("[_[:alpha:]][_[:alnum:]]*");
        AZStd::regex_match(name, match, validate_regex);
        if (match.size() == 0)
        {
            return AZ::Failure(AZStd::string::format("%s, invalid name specified", name.c_str()));
        }

        return AZ::Success(true);
    }
} // namespace ScriptEvents
