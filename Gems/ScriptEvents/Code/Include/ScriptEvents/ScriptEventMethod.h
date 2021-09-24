/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <ScriptEvents/ScriptEventParameter.h>
#include <ScriptEvents/ScriptEventTypes.h>

namespace AZ
{
    class ReflectContext;
}
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

        void FromScript(AZ::ScriptDataContext& dc);

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

        static void Reflect(AZ::ReflectContext* context);

        AZStd::string GetName() const
        {
            return m_name.Get<AZStd::string>() ? *m_name.Get<AZStd::string>() : "";
        }
        AZStd::string GetTooltip() const
        {
            return m_tooltip.Get<AZStd::string>() ? *m_tooltip.Get<AZStd::string>() : "";
        }

        const AZ::Uuid GetReturnType() const
        {
            return m_returnType.Get<AZ::Uuid>() ? *m_returnType.Get<AZ::Uuid>() : AZ::Uuid::CreateNull();
        }

        const AZStd::vector<Parameter>& GetParameters() const
        {
            return m_parameters;
        }

        ScriptEventData::VersionedProperty& GetNameProperty()
        {
            return m_name;
        }

        ScriptEventData::VersionedProperty& GetTooltipProperty()
        {
            return m_tooltip;
        }

        ScriptEventData::VersionedProperty& GetReturnTypeProperty()
        {
            return m_returnType;
        }

        const ScriptEventData::VersionedProperty& GetNameProperty() const
        {
            return m_name;
        }

        const ScriptEventData::VersionedProperty& GetTooltipProperty() const
        {
            return m_tooltip;
        }

        const ScriptEventData::VersionedProperty& GetReturnTypeProperty() const
        {
            return m_returnType;
        }

        AZ::Crc32 GetEventId() const
        {
            return AZ::Crc32(GetNameProperty().GetId().ToString<AZStd::string>().c_str());
        }
        //! Validates that the asset data being stored is valid and supported.
        AZ::Outcome<bool, AZStd::string> Validate() const;
        void PreSave();
        void Flatten();

    private:
        ScriptEventData::VersionedProperty m_name;
        ScriptEventData::VersionedProperty m_tooltip;
        ScriptEventData::VersionedProperty m_returnType;
        AZStd::vector<Parameter> m_parameters;
    };

} // namespace ScriptEvents
