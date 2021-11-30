/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <ScriptEvents/Internal/VersionedProperty.h>
#include <ScriptEvents/ScriptEventTypes.h>

namespace ScriptEvents
{
    //! An event's parameter definition, see (ScriptEventMethod)
    class Parameter
    {
    public:

        AZ_TYPE_INFO(Parameter, "{0DA4809B-08A6-49DC-9024-F81645D97FAC}");

        Parameter()
            : m_name("Name")
            , m_tooltip("Tooltip")
            , m_type("Type")
        {
            m_tooltip.Set("");
            m_name.Set("ParameterName");
            m_type.Set(azrtti_typeid<bool>());
        }

        Parameter(const Parameter& rhs)
        {
            m_name = rhs.m_name;
            m_tooltip = rhs.m_tooltip;
            m_type = rhs.m_type;
        }

        Parameter(AZ::ScriptDataContext& dc)
        {
            FromScript(dc);
        }

        void FromScript(AZ::ScriptDataContext& dc);

        static void Reflect(AZ::ReflectContext* context);

        AZ::Outcome<bool, AZStd::string> Validate() const;
        AZStd::string GetName() const { return m_name.Get<AZStd::string>() ? *m_name.Get<AZStd::string>() : ""; }
        AZStd::string GetTooltip() const { return m_tooltip.Get<AZStd::string>() ? *m_tooltip.Get<AZStd::string>() : ""; }
        AZ::Uuid GetType() const { return m_type.Get<AZ::Uuid>() ? *m_type.Get<AZ::Uuid>() : AZ::Uuid::CreateNull(); }

        ScriptEventData::VersionedProperty& GetNameProperty() { return m_name; }
        ScriptEventData::VersionedProperty& GetTooltipProperty() { return m_tooltip; }
        ScriptEventData::VersionedProperty& GetTypeProperty() { return m_type; }

        const ScriptEventData::VersionedProperty& GetNameProperty() const { return m_name; }
        const ScriptEventData::VersionedProperty& GetTooltipProperty() const { return m_tooltip; }
        const ScriptEventData::VersionedProperty& GetTypeProperty() const { return m_type; }

        void PreSave()
        {
            m_name.PreSave();
            m_tooltip.PreSave();
            m_type.PreSave();
        }

        void Flatten()
        {
            m_name.Flatten();
            m_tooltip.Flatten();
            m_type.Flatten();
        }

    private:

        ScriptEventData::VersionedProperty m_name;
        ScriptEventData::VersionedProperty m_tooltip;
        ScriptEventData::VersionedProperty m_type;

    };

}
