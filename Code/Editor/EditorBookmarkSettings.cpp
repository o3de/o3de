/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

// AzCore
#include <AzCore/IO/Path/Path.h>
#include <AzCore/Settings/SettingsRegistry.h>
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>
#include <AzCore/StringFunc/StringFunc.h>
#include <AzCore/Utils/Utils.h>
#include <AzCore/Console/IConsole.h>
#include <AzCore/IO/ByteContainerStream.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>

#include <EditorBookmarkSettings.h>


#pragma optimize("", off)

void BookmarkConfig::Reflect(AZ::ReflectContext* context)
{
    
    if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
    {
        serializeContext->Class<BookmarkConfig>()
            ->Version(1)
            ->Field("x", &BookmarkConfig::m_xPos)
            ->Field("y", &BookmarkConfig::m_yPos)
            ->Field("z", &BookmarkConfig::m_zPos);
    }
}

EditorBookmarkSettings::EditorBookmarkSettings()
    : m_bookmarSettings(AZStd::make_unique<AZ::SettingsRegistryImpl>())
{
    m_serializeContext = AZStd::make_unique<AZ::SerializeContext>();
    //m_registrationContext = AZStd::make_unique<AZ::JsonRegistrationContext>();
    Setup();
}

 EditorBookmarkSettings::~EditorBookmarkSettings()
{
     m_registrationContext.reset();
     m_serializeContext.reset();
     m_bookmarSettings.reset();
 }

void EditorBookmarkSettings::Reflect(AZ::ReflectContext* context)
{
    if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
    {
        serializeContext->Class<EditorBookmarkSettings>()
            ->Field("BookmarkConfig", &EditorBookmarkSettings::m_bookmarkConfig)
            ->Version(0);
}   }

 void EditorBookmarkSettings::Setup()
{

    //m_serializeContext->Class<BookmarkConfig>()
    //    ->Version(0)
    //    ->Field("x", &BookmarkConfig::m_xPos)
    //    ->Field("y", &BookmarkConfig::m_yPos)
    //    ->Field("z", &BookmarkConfig::m_zPos);
    AZ::JsonSystemComponent::Reflect(m_registrationContext.get());
    m_bookmarSettings->SetContext(m_serializeContext.get());
    m_bookmarSettings->SetContext(m_registrationContext.get());
}



void EditorBookmarkSettings::SaveBookmarkSettingsFile()
{
    auto registry = AZ::SettingsRegistry::Get();
    if (m_bookmarSettings == nullptr)
    {
        AZ_Warning("SEditorSettings", false, "Unable to access global settings registry. Editor Preferences cannot be saved");
        return;
    }



    // Resolve path to editorpreferences.setreg
    AZ::IO::FixedMaxPath editorBookmarkFilePath = AZ::Utils::GetProjectPath();
    editorBookmarkFilePath /= "user/Registry/editorbookmarks.setreg";

    m_bookmarkConfig.m_xPos = 1.0f;
    m_bookmarkConfig.m_yPos = 2.0f;
    m_bookmarkConfig.m_zPos = 4.0f;

    bool success = false;
    //AZStd::string_view path = AZStd::string_view("/O3DE/CameraBookmark");
    success = registry->SetObject("/O3DE/CameraBookmark", &m_bookmarkConfig);

    AZ::SettingsRegistryMergeUtils::DumperSettings dumperSettings;
    dumperSettings.m_prettifyOutput = true;
    //dumperSettings.m_includeFilter = [](AZStd::string_view path)
    //{
    //    AZStd::string_view o3dePrefixPath("/O3DE/CameraBookmark");
    //    return o3dePrefixPath.starts_with(path.substr(0, o3dePrefixPath.size()));
    //};

    AZStd::string stringBuffer;
    AZ::IO::ByteContainerStream stringStream(&stringBuffer);
    if (!AZ::SettingsRegistryMergeUtils::DumpSettingsRegistryToStream(*m_bookmarSettings, "", stringStream, dumperSettings))
    {
        AZ_Warning(
            "SEditorSettings", false, R"(Unable to save changes to the Editor Preferences registry file at "%s"\n)",
            editorBookmarkFilePath.c_str());
        return;
    }

    bool saved{};
    constexpr auto configurationMode =
        AZ::IO::SystemFile::SF_OPEN_CREATE | AZ::IO::SystemFile::SF_OPEN_CREATE_PATH | AZ::IO::SystemFile::SF_OPEN_WRITE_ONLY;
    if (AZ::IO::SystemFile outputFile; outputFile.Open(editorBookmarkFilePath.c_str(), configurationMode))
    {
        saved = outputFile.Write(stringBuffer.data(), stringBuffer.size()) == stringBuffer.size();
    }

    AZ_Warning(
        "SEditorSettings",
        saved, R"(Unable to save Editor Preferences registry file to path "%s"\n)", editorBookmarkFilePath.c_str());
}


#pragma optimize("", on)
