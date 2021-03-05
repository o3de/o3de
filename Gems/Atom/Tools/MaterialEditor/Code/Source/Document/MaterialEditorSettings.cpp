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

#include <Document/MaterialEditorSettings.h>

namespace MaterialEditor
{
    MaterialEditorSettings::MaterialEditorSettings()
    {
        MaterialEditorSettingsRequestBus::Handler::BusConnect();
    }

    MaterialEditorSettings::~MaterialEditorSettings()
    {
        MaterialEditorSettingsRequestBus::Handler::BusDisconnect();
    }

    AZ::Outcome<AZStd::any> MaterialEditorSettings::GetProperty(AZStd::string_view name) const
    {
        const auto it = m_propertyMap.find(name);
        if (it != m_propertyMap.end())
        {
            return AZ::Success(it->second);
        }
        AZ_Warning("MaterialEditorSettings", false, "Failed to find property [%s].", name.data());
        return AZ::Failure();
    }

    AZ::Outcome<AZStd::string> MaterialEditorSettings::GetStringProperty(AZStd::string_view name) const
    {
        AZ::Outcome<AZStd::any> outcome = GetProperty(name);
        if (!outcome || !outcome.GetValue().is<AZStd::string>())
        {
            return AZ::Failure();
        }
        return AZ::Success(AZStd::any_cast<AZStd::string>(outcome.GetValue()));
    }

    AZ::Outcome<bool> MaterialEditorSettings::GetBoolProperty(AZStd::string_view name) const
    {
        AZ::Outcome<AZStd::any> outcome = GetProperty(name);
        if (!outcome || !outcome.GetValue().is<bool>())
        {
            return AZ::Failure();
        }
        return AZ::Success(AZStd::any_cast<bool>(outcome.GetValue()));
    }

    void MaterialEditorSettings::SetProperty(AZStd::string_view name, const AZStd::any& value)
    {
        m_propertyMap[name] = value;
        MaterialEditorSettingsNotificationBus::Broadcast(&MaterialEditorSettingsNotifications::OnPropertyChanged, name, value);
    }

    void MaterialEditorSettings::SetStringProperty(AZStd::string_view name, AZStd::string_view stringValue)
    {
        SetProperty(name, AZStd::any(AZStd::string(stringValue)));
    }

    void MaterialEditorSettings::SetBoolProperty(AZStd::string_view name, bool boolValue)
    {
        SetProperty(name, AZStd::any(boolValue));
    }
}
