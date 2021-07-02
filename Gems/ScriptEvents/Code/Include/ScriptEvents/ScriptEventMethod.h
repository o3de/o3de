/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <ScriptEvents/ScriptEventParameter.h>
#include <AzCore/RTTI/ReflectContext.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <ScriptEvents/ScriptEventTypes.h>

namespace ScriptEvents
{
    //! Holds the versioned definition for each of a script events.
    //! You can think of this as a function declaration with a name, a return type
    //! and an optional list of parameters (see ScriptEventParameter).
    class Method
    {
    public:

        AZ_TYPE_INFO(Method, "{E034EA83-C798-413D-ACE8-4923C51CF4F7}");

        Method()
            : m_name("Name")
            , m_tooltip("Tooltip")
            , m_returnType("Return Type")
        {
            m_name.Set("MethodName");
            m_tooltip.Set("");
            m_returnType.Set(azrtti_typeid<void>());
        }

        Method(const Method& rhs)
        {
            m_name = rhs.m_name;
            m_tooltip = rhs.m_tooltip;
            m_returnType = rhs.m_returnType;
            m_parameters = rhs.m_parameters;
        }

        Method& operator=(const Method& rhs)
        {
            if (this != &rhs)
            {
                m_name = rhs.m_name;
                m_tooltip = rhs.m_tooltip;
                m_returnType = rhs.m_returnType;
                m_parameters.assign(rhs.m_parameters.begin(), rhs.m_parameters.end());
            }
            return *this;
        }

        Method(Method&& rhs)
        {
            m_name = AZStd::move(rhs.m_name);
            m_tooltip = AZStd::move(rhs.m_tooltip);
            m_returnType = AZStd::move(rhs.m_returnType);
            m_parameters.swap(rhs.m_parameters);
        }

        Method(AZ::ScriptDataContext& dc)
        {
            FromScript(dc);
        }

        void FromScript(AZ::ScriptDataContext& dc)
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

            //AZ_TracePrintf("Script Events", "Added Script Method: %s (return type: %s)\n", GetName().c_str(), m_returnType.IsEmpty() ? "none" : GetReturnType().ToString<AZStd::string>().c_str());

        }

        ~Method()
        {
            m_parameters.clear();
        }

        void AddParameter(AZ::ScriptDataContext& dc)
        {
            Parameter& parameter = NewParameter();
            parameter.FromScript(dc);
            dc.PushResult(parameter);
        }

        bool IsValid() const;

        Parameter& NewParameter()
        {
            m_parameters.emplace_back();
            return m_parameters.back();
        }

        static void Reflect(AZ::ReflectContext* context)
        {
            if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<Method>()
                    ->Field("m_name", &Method::m_name)
                    ->Field("m_tooltip", &Method::m_tooltip)
                    ->Field("m_returnType", &Method::m_returnType)
                    ->Field("m_parameters", &Method::m_parameters)
                    ;

                if (AZ::EditContext* editContext = serializeContext->GetEditContext())
                {
                    editContext->Class<Method>("Script Event", "A script event's definition")
                        ->DataElement(AZ::Edit::UIHandlers::Default, &Method::m_name, "Name", "The specified name for this event, represents a callable function (i.e. MyScriptEvent())")
                        ->DataElement(AZ::Edit::UIHandlers::Default, &Method::m_tooltip, "Tooltip", "A description of this event")
                        ->DataElement(AZ::Edit::UIHandlers::ComboBox, &Method::m_returnType, "Return value type", "the typeid of the return value, ex. AZ::type_info<int>::Uuid foo()")
                        ->Attribute(AZ::Edit::Attributes::GenericValueList, &Types::GetValidReturnTypes)
                        ->DataElement(AZ::Edit::UIHandlers::Default, &Method::m_parameters, "Parameters", "A list of parameters for the EBus event, ex. void foo(Parameter1, Parameter2)")
                        ;
                }
            }

            if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
            {
                behaviorContext->Class<Method>("Method")
                    ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)
                    ->Method("AddParameter", &Method::AddParameter)
                    ->Property("Name", BehaviorValueProperty(&Method::m_name))
                    ->Property("ReturnType", BehaviorValueProperty(&Method::m_returnType))
                    ->Property("Parameters", BehaviorValueProperty(&Method::m_parameters))
                    ;
            }
        }

        AZStd::string GetName() const { return m_name.Get<AZStd::string>() ? *m_name.Get<AZStd::string>() : ""; }
        AZStd::string GetTooltip() const { return m_tooltip.Get<AZStd::string>() ? *m_tooltip.Get<AZStd::string>() : ""; }
        const AZ::Uuid GetReturnType() const { return m_returnType.Get<AZ::Uuid>() ? *m_returnType.Get<AZ::Uuid>() : AZ::Uuid::CreateNull(); }
        const AZStd::vector<Parameter>& GetParameters() const { return m_parameters; }

        ScriptEventData::VersionedProperty& GetNameProperty() { return m_name; }
        ScriptEventData::VersionedProperty& GetTooltipProperty() { return m_tooltip; }
        ScriptEventData::VersionedProperty& GetReturnTypeProperty() { return m_returnType; }

        const ScriptEventData::VersionedProperty& GetNameProperty() const { return m_name; }
        const ScriptEventData::VersionedProperty& GetTooltipProperty() const { return m_tooltip; }
        const ScriptEventData::VersionedProperty& GetReturnTypeProperty() const { return m_returnType; }

        AZ::Crc32 GetEventId() const { return AZ::Crc32(GetNameProperty().GetId().ToString<AZStd::string>().c_str()); }

        //! Validates that the asset data being stored is valid and supported.
        AZ::Outcome<bool, AZStd::string> Validate() const
        {
            const AZStd::string name = GetName();
            const AZ::Uuid returnType = GetReturnType();

            // Validate address type
            if (!Types::IsValidReturnType(returnType))
            {
                return AZ::Failure(AZStd::string::format("The specified type %s is not valid as return type for Script Event: %s", returnType.ToString<AZStd::string>().c_str(), name.c_str()));
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

                if (parameter.GetName().compare(parameterName) == 0)
                {
                    return AZ::Failure(AZStd::string::format("Cannot have duplicate parameter names (%d: %s) make sure each parameter name is unique", parameterIndex, parameterName.c_str()));
                }

                parameterName = parameter.GetName();
                ++parameterIndex;
            }

            return AZ::Success(true);
        }

        void PreSave()
        {
            m_name.PreSave();
            m_tooltip.PreSave();
            m_returnType.PreSave();

            for (Parameter parameter : m_parameters)
            {
                parameter.PreSave();
            }
        }

        void Flatten()
        {
            m_name.Flatten();
            m_tooltip.Flatten();
            m_returnType.Flatten();
            for (Parameter& parameter : m_parameters)
            {
                parameter.Flatten();
            }
        }

    private:

        ScriptEventData::VersionedProperty m_name;
        ScriptEventData::VersionedProperty m_tooltip;
        ScriptEventData::VersionedProperty m_returnType;
        AZStd::vector<Parameter> m_parameters;

        };

}
