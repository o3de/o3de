/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include "Include/IPreferencesPage.h"

#include <AzCore/RTTI/RTTIMacros.h>
#include <QIcon>

namespace AZ
{
    class SerializeContext;
    class SettingsRegistryImpl;
}

class CEditorPreferencesPage_AWS
    : public IPreferencesPage
{
public:
    AZ_RTTI(CEditorPreferencesPage_AWS, "{51FB9557-ABA3-4FD7-803A-1784F5B06F5F}", IPreferencesPage)

    static void Reflect(AZ::SerializeContext& serialize);

    CEditorPreferencesPage_AWS();
    virtual ~CEditorPreferencesPage_AWS();

    // IPreferencesPage interface methods.
    virtual const char* GetCategory() override { return "AWS"; }
    virtual const char* GetTitle() override;
    virtual QIcon& GetIcon() override;
    virtual void OnApply() override;
    virtual void OnCancel() override {}
    virtual bool OnQueryCancel() override { return true; }

protected:
    struct UsageOptions
    {
        AZ_TYPE_INFO(UsageOptions, "{2B7D9B19-D13B-4E54-B724-B2FD8D0828B3}")

        bool m_awsAttributionEnabled;
    };

    const UsageOptions& GetUsageOptions();

private:
    void InitializeSettings();
    void SaveSettingsRegistryFile();
    UsageOptions m_usageOptions;
    QIcon m_icon;
    AZStd::unique_ptr<AZ::SettingsRegistryImpl> m_settingsRegistry;

    static constexpr char AWSAttributionEnabledKey[] = "/Amazon/AWS/Preferences/AWSAttributionEnabled";
    static constexpr char EditorPreferencesFileName[] = "editorpreferences.setreg";
    static constexpr char EditorAWSPreferencesFileName[] = "editor_aws_preferences.setreg";
    static constexpr char AWSAttributionSettingsPrefixKey[] = "/Amazon/AWS/Preferences";
};
