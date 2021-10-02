/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/containers/vector.h>

#include <ScriptEvents/ScriptEventParameter.h>
#include <ScriptEvents/ScriptEventsMethod.h>
#include <ScriptEvents/Internal/VersionedProperty.h>

namespace ScriptEvents
{
    //! Defines a Script Event.
    //! This is the user-facing Script Event definition.
    //! When users create Script Events from Lua or in the editor this is the data definition that 
    //! a Script Event Asset will serialize.
    class ScriptEvent
    {
    public:

        AZ_TYPE_INFO(ScriptEvent, "{10A08CD3-32C9-4E18-8039-4B8A8157918E}");

        bool IsAddressRequired() const
        {
            AZ::Uuid id = GetAddressType();
            return id != azrtti_typeid<void>() && id != AZ::Uuid::CreateNull();
        }

        ScriptEvent()
            : m_version(1)
            , m_name("Name")
            , m_category("Category")
            , m_tooltip("Tooltip")
            , m_addressType("Address Type")
        {
            m_name.Set("EventName");
            m_category.Set("Script Events");
            m_tooltip.Set("");
            m_addressType.Set(azrtti_typeid<void>());
        }

        ScriptEvent(AZ::ScriptDataContext& dc)
            : ScriptEvent()
        {
            if (dc.GetNumArguments() > 0)
            {
                AZStd::string name;
                if (dc.ReadArg(0, name))
                {
                    m_name.Set(name);
                }

                // \todo align with ScriptEvents error reporting policy, if the there is an argument but it is not an aztypeid
                if (dc.GetNumArguments() > 1 && dc.IsClass<AZ::Uuid>(1))
                {
                    AZ::Uuid addressType;
                    if (dc.ReadArg(1, addressType))
                    {
                        m_addressType.Set(addressType);
                    }
                    else
                    {
                        m_addressType.Set(azrtti_typeid<void>());
                    }
                }
            }
        }

        void MakeBackup()
        {
            IncreaseVersion();
        }

        void AddMethod(AZ::ScriptDataContext& dc)
        {
            if (dc.GetNumArguments() > 0)
            {
                Method& method = NewMethod();
                method.FromScript(dc);
                dc.PushResult(method);
            }
        }

        void RegisterInternal();
        void Register(AZ::ScriptDataContext& dc);
        void Release(AZ::ScriptDataContext& dc);

        Method& NewMethod()
        {
            m_methods.emplace_back();
            return m_methods.back();
        }

        bool FindMethod(const AZ::Crc32& eventId, Method& outMethod) const
        {
            for (auto method : m_methods)
            {
                if (method.GetEventId() == eventId)
                {
                    outMethod = method;
                    return true;
                }
            }

            return false;
        }

        bool FindMethod(const AZStd::string_view& name, Method& outMethod) const
        {
            outMethod = {};

            for (auto& method : m_methods)
            {
                if (method.GetName().compare(name) == 0)
                {
                    outMethod = method;
                    return true;
                }
            }
            return false;
        }

        bool HasMethod(const AZ::Crc32& eventId) const
        {
            for (auto& method : m_methods)
            {
                if (method.GetEventId() == eventId)
                {
                    return true;
                }
            }
            
            return false;
        }

        static void Reflect(AZ::ReflectContext* context);

        AZ::u32 GetVersion() const { return m_version; }
        AZStd::string GetName() const { return m_name.Get<AZStd::string>() ? *m_name.Get<AZStd::string>() : ""; }
        AZStd::string GetCategory() const { return m_category.Get<AZStd::string>() ? *m_category.Get<AZStd::string>() : ""; }
        AZStd::string GetTooltip() const { return m_tooltip.Get<AZStd::string>() ? *m_tooltip.Get<AZStd::string>() : ""; }
        AZ::Uuid GetAddressType() const { return m_addressType.Get<AZ::Uuid>() ? *m_addressType.Get<AZ::Uuid>() : AZ::Uuid::CreateNull(); }
        AZStd::string GetBehaviorContextName() const { return CreateBehaviorContextName(GetVersion()); }
        AZStd::string CreateBehaviorContextName([[maybe_unused]] AZ::u32 versionNumber) const { return AZStd::string::format("%s_%i", GetName().c_str(), GetVersion()); }
        
        const AZStd::vector<Method>& GetMethods() const { return m_methods; }

        AZStd::string_view GetLabel() const { return { m_name.Get<AZStd::string>()->data(), m_name.Get<AZStd::string>()->size() }; }

        void SetVersion(AZ::u32 version) { m_version = version; }
        ScriptEventData::VersionedProperty& GetNameProperty() { return m_name; }
        ScriptEventData::VersionedProperty& GetCategoryProperty() { return m_category; }
        ScriptEventData::VersionedProperty& GetTooltipProperty() { return m_tooltip; }
        ScriptEventData::VersionedProperty& GetAddressTypeProperty() { return m_addressType; }

        const ScriptEventData::VersionedProperty& GetNameProperty() const { return m_name; }
        const ScriptEventData::VersionedProperty& GetCategoryProperty() const { return m_category; }
        const ScriptEventData::VersionedProperty& GetTooltipProperty() const { return m_tooltip; }
        const ScriptEventData::VersionedProperty& GetAddressTypeProperty() const { return m_addressType; }

        //! Validates that the asset data being stored is valid and supported.
        AZ::Outcome<bool, AZStd::string> Validate() const;

        void IncreaseVersion() 
        { 
            m_name.PreSave();
            m_category.PreSave();
            m_tooltip.PreSave();
            m_addressType.PreSave();

            for (Method& method : m_methods)
            {
                method.PreSave();
            }

            ++m_version; 
        }

        void Flatten()
        {
            m_name.Flatten();
            m_category.Flatten();
            m_tooltip.Flatten();
            m_addressType.Flatten();

            for (Method& method : m_methods)
            {
                method.Flatten();
            }
        }

    private:

        AZ::u32 m_version;
        ScriptEventData::VersionedProperty m_name;
        ScriptEventData::VersionedProperty m_category;
        ScriptEventData::VersionedProperty m_tooltip;
        ScriptEventData::VersionedProperty m_addressType;
        AZStd::vector<Method> m_methods;

    };
}
