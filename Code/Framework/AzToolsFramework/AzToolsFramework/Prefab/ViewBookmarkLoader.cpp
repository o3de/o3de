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

#include <Prefab/ViewBookmarkLoader.h>


#pragma optimize("", off)

static constexpr const char* s_viewBookmarksRegistryPath = "/O3DE/ViewBookmarks/";

void ViewBookmarkLoader::Reflect(AZ::ReflectContext* context)
{
    if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
    {
        serializeContext->Class<ViewBookmark>()
            ->Version(0)
            ->Field("x", &ViewBookmark::m_xPos)
            ->Field("y", &ViewBookmark::m_yPos)
            ->Field("z", &ViewBookmark::m_zPos);
            //->Field("Bookmark01", &ViewBookmarkLoader::m_config);
    }
}

void ViewBookmarkLoader::RegisterViewBookmarkLoaderInterface()
{
    AZ::Interface<ViewBookmarkLoaderInterface>::Register(this);
}

void ViewBookmarkLoader::UnregisterViewBookmarkLoaderInterface()
{
    AZ::Interface<ViewBookmarkLoaderInterface>::Unregister(this);
}

bool ViewBookmarkLoader::SaveCustomBookmark(ViewBookmark config)
{
    //m_config = config;
    if (auto registry = AZ::SettingsRegistry::Get())
    {
        return registry->SetObject("/O3DE/ViewBookmarks/ViewBookmark_4", config);
    }
    return false;
}

void ViewBookmarkLoader::SaveBookmarkSettingsFile()
{
    auto registry = AZ::SettingsRegistry::Get();
    if (registry == nullptr)
    {
        AZ_Warning("SEditorSettings", false, "Unable to access global settings registry. Editor Preferences cannot be saved");
        return;
    }

    // Resolve path to editorpreferences.setreg
    AZ::IO::FixedMaxPath editorBookmarkFilePath = AZ::Utils::GetProjectPath();
    editorBookmarkFilePath /= "user/Registry/editorbookmarks.setreg";

    AZ::SettingsRegistryMergeUtils::DumperSettings dumperSettings;
    dumperSettings.m_prettifyOutput = true;
    dumperSettings.m_includeFilter = [](AZStd::string_view path)
    {
        AZStd::string_view o3dePrefixPath("/O3DE/ViewBookmarks");
        return o3dePrefixPath.starts_with(path.substr(0, o3dePrefixPath.size()));
    };

    AZStd::string stringBuffer;
    AZ::IO::ByteContainerStream stringStream(&stringBuffer);
    if (!AZ::SettingsRegistryMergeUtils::DumpSettingsRegistryToStream(*registry, "", stringStream, dumperSettings))
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
        saved,
        R"(Unable to save Editor Preferences registry file to path "%s"\n)", editorBookmarkFilePath.c_str());
}


#pragma optimize("", on)
