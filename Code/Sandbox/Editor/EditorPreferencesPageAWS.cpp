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
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>
#include <AzCore/Jobs/JobFunction.h>

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
    m_settingsRegistry = AZStd::make_unique<AZ::SettingsRegistryImpl>();
    InitializeSettings();
    m_icon = QIcon(":/res/AWS_preferences_icon.svg");
}

CEditorPreferencesPage_AWS::~CEditorPreferencesPage_AWS()
{
    m_settingsRegistry.reset();
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
    m_settingsRegistry->Set(AWSAttributionEnabledKey, m_usageOptions.m_awsAttributionEnabled);
    SaveSettingsRegistryFile();
}

const CEditorPreferencesPage_AWS::UsageOptions& CEditorPreferencesPage_AWS::GetUsageOptions()
{
    return m_usageOptions;
}

void CEditorPreferencesPage_AWS::SaveSettingsRegistryFile()
{
    AZ::Job* job = AZ::CreateJobFunction(
        [this]()
        {
            AZ::IO::FileIOBase* fileIO = AZ::IO::FileIOBase::GetInstance();
            AZ_Assert(fileIO, "File IO is not initialized.");

            // Resolve path to editor_aws_preferences.setreg
            AZStd::string editorPreferencesFilePath =
                AZStd::string::format("@user@/%s/%s", AZ::SettingsRegistryInterface::RegistryFolder, EditorAWSPreferencesFileName);
            AZStd::array<char, AZ::IO::MaxPathLength> resolvedPath{};
            fileIO->ResolvePath(editorPreferencesFilePath.c_str(), resolvedPath.data(), resolvedPath.size());

            AZ::SettingsRegistryMergeUtils::DumperSettings dumperSettings;
            dumperSettings.m_prettifyOutput = true;
            dumperSettings.m_jsonPointerPrefix = AWSAttributionSettingsPrefixKey;

            AZStd::string stringBuffer;
            AZ::IO::ByteContainerStream stringStream(&stringBuffer);
            if (!AZ::SettingsRegistryMergeUtils::DumpSettingsRegistryToStream(
                    *m_settingsRegistry, AWSAttributionSettingsPrefixKey, stringStream, dumperSettings))
            {
                AZ_Warning(
                    "AWSAttributionManager", false, R"(Unable to save changes to the Editor AWS Preferences registry file at "%s"\n)",
                    resolvedPath.data());
                return;
            }

            bool saved{};
            constexpr auto configurationMode =
                AZ::IO::SystemFile::SF_OPEN_CREATE | AZ::IO::SystemFile::SF_OPEN_CREATE_PATH | AZ::IO::SystemFile::SF_OPEN_WRITE_ONLY;
            if (AZ::IO::SystemFile outputFile; outputFile.Open(resolvedPath.data(), configurationMode))
            {
                saved = outputFile.Write(stringBuffer.data(), stringBuffer.size()) == stringBuffer.size();
            }

            AZ_Warning(
                "AWSAttributionManager", saved, R"(Unable to save Editor AWS Preferences registry file to path "%s"\n)",
                editorPreferencesFilePath.c_str());
        },
        true);
    job->Start();
}

void CEditorPreferencesPage_AWS::InitializeSettings()
{
    AZ::IO::FileIOBase* fileIO = AZ::IO::FileIOBase::GetInstance();
    AZ_Assert(fileIO, "File IO is not initialized.");

    // Resolve path to editor_aws_preferences.setreg
    AZStd::string editorAWSPreferencesFilePath =
        AZStd::string::format("@user@/%s/%s", AZ::SettingsRegistryInterface::RegistryFolder, EditorAWSPreferencesFileName);
    AZStd::array<char, AZ::IO::MaxPathLength> resolvedPathAWSPreference{};
    if (!fileIO->ResolvePath(editorAWSPreferencesFilePath.c_str(), resolvedPathAWSPreference.data(), resolvedPathAWSPreference.size()))
    {
        AZ_Warning("AWSAttributionManager", false, "Error resolving path %s", resolvedPathAWSPreference.data());
        return;
    }

    if (fileIO->Exists(resolvedPathAWSPreference.data()))
    {
        m_settingsRegistry->MergeSettingsFile(resolvedPathAWSPreference.data(), AZ::SettingsRegistryInterface::Format::JsonMergePatch, "");
    }

    if (!m_settingsRegistry->Get(m_usageOptions.m_awsAttributionEnabled, AWSAttributionEnabledKey))
    {
        // If key is missing default to on.
        m_usageOptions.m_awsAttributionEnabled = true;
    }
}
