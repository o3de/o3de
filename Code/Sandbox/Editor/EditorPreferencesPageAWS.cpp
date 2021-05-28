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
#include "EditorDefs.h"

#include "EditorPreferencesPageAWS.h"

// AzCore
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Settings/SettingsRegistry.h>

constexpr char AWSAttributionEnabledKey[] = "/Amazon/Preferences/AWS/AWSAttributionEnabled";

void CEditorPreferencesPage_AWS::Reflect(AZ::SerializeContext& serialize)
{
    serialize.Class<UsageOptions>()
        ->Version(1)
        ->Field("AWSAttributionEnabled", &UsageOptions::m_awsAttributionEnabled);

    serialize.Class<CEditorPreferencesPage_AWS>()
        ->Version(1)
        ->Field("UsageOptions", &CEditorPreferencesPage_AWS::m_usageOptions);

    AZ::EditContext* editContext = serialize.GetEditContext();
    if (editContext)
    {
        editContext->Class<UsageOptions>("Options", "")
            ->DataElement(AZ::Edit::UIHandlers::CheckBox, &UsageOptions::m_awsAttributionEnabled, "Send Metrics usage to AWS",
            "Reports Gem usage to AWS on Editor launch");

        editContext->Class<CEditorPreferencesPage_AWS>("AWS Preferences", "AWS Preferences")
            ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
            ->Attribute(AZ::Edit::Attributes::Visibility, AZ_CRC("PropertyVisibility_ShowChildrenOnly", 0xef428f20))
            ->DataElement(AZ::Edit::UIHandlers::Default, &CEditorPreferencesPage_AWS::m_usageOptions, "AWS Usage Data", "AWS Usage Options");
    }
}


CEditorPreferencesPage_AWS::CEditorPreferencesPage_AWS()
{
    InitializeSettings();

    // TODO Update with AWS svg.
    m_icon = QIcon(":/res/Experimental.svg");
}

const char* CEditorPreferencesPage_AWS::GetTitle()
{
    return "AWS";
}

QIcon& CEditorPreferencesPage_AWS::GetIcon()
{
    return m_icon;
}

void CEditorPreferencesPage_AWS::OnApply()
{
    auto registry = AZ::SettingsRegistry::Get();
    if (registry == nullptr)
    {
        AZ_Warning("CEditorPreferencesPage_AWS", false, "Unable to access global settings registry. Editor Preferences cannot be saved");
        return;
    }

    registry->Set(AWSAttributionEnabledKey, m_usageOptions.m_awsAttributionEnabled);
}

const CEditorPreferencesPage_AWS::UsageOptions& CEditorPreferencesPage_AWS::GetUsageOptions()
{
    return m_usageOptions;
}

void CEditorPreferencesPage_AWS::InitializeSettings()
{
    auto registry = AZ::SettingsRegistry::Get();
    if (registry == nullptr)
    {
        AZ_Warning("CEditorPreferencesPage_AWS", false, "Unable to access global settings registry. Editor Preferences cannot be saved");
        return;
    }

    if (!registry->Get(m_usageOptions.m_awsAttributionEnabled, AWSAttributionEnabledKey))
    {
        // If key is missing default to on.
        m_usageOptions.m_awsAttributionEnabled = true;
    }
}
