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
#include <API/ComponentEntitySelectionBus.h>


#pragma optimize("", off)

static constexpr const char* s_viewBookmarksRegistryPath = "/O3DE/ViewBookmarks/";

void ViewBookmarkLoader::Reflect(AZ::ReflectContext* context)
{
    ViewBookmark::Reflect(context);
    if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
    {
        serializeContext->RegisterGenericType<ViewBookmark>();

        serializeContext->Class<ViewBookmarkLoader>()
            ->Version(0);
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

bool ViewBookmarkLoader::SaveBookmark(ViewBookmark bookmark)
{
    return SaveBookmark_Internal(bookmark);
}

bool ViewBookmarkLoader::SaveLastKnownLocationInLevel(ViewBookmark bookmark)
{
    return SaveBookmark_Internal(bookmark, true);
}

bool ViewBookmarkLoader::SaveBookmark_Internal(ViewBookmark& bookmark, bool isLastKnownLocationInLevel)
{
    AZ::EntityId levelEntityId;
    AzToolsFramework::ToolsApplicationRequestBus::BroadcastResult(
        levelEntityId, &AzToolsFramework::ToolsApplicationRequests::GetCurrentLevelEntityId);

    AZ_Error("View Bookmark Loader ", levelEntityId.IsValid(), "Level Entity ID is invalid.");


    // Create entry or if it already exists, add the bookmark
    //TODO: Refactor this
    auto existingBookmarkEntry = m_viewBookmarkMap.find(levelEntityId);
    if (existingBookmarkEntry != m_viewBookmarkMap.end())
    {
        if (isLastKnownLocationInLevel)
        {
            AZStd::vector<ViewBookmark>& bookmarks = existingBookmarkEntry->second;
            bookmarks.at(0) = bookmark;
        }
        else
        {
            existingBookmarkEntry->second.push_back(bookmark);
        }
    }
    else
    {
        // Create first an empty bookmark that will hold the last Known Location of a level
        if (isLastKnownLocationInLevel)
        {
            m_viewBookmarkMap.insert(AZStd::make_pair(levelEntityId, AZStd::vector<ViewBookmark>{ bookmark }));
        }
        else
        {
            m_viewBookmarkMap.insert(AZStd::make_pair(levelEntityId, AZStd::vector<ViewBookmark>{ ViewBookmark(), bookmark }));
        }
    }

    // Write to the settings registry (We might want to do this step somewhere else)
    if (auto registry = AZ::SettingsRegistry::Get())
    {
        size_t currentBookmarkIndex = isLastKnownLocationInLevel ? 0 : m_viewBookmarkMap.at(levelEntityId).size() - 1;
        AZStd::string finalPath = s_viewBookmarksRegistryPath + levelEntityId.ToString() + "/" + AZStd::to_string(currentBookmarkIndex);

        //Don't store in memory if it is the last known location in the level.
        if (isLastKnownLocationInLevel)
        {
            AZStd::vector<ViewBookmark>& bookmarks = m_viewBookmarkMap.at(levelEntityId);
            return registry->SetObject(finalPath, bookmarks.at(0));
        }
        else
        {
            return registry->SetObject(finalPath, m_viewBookmarkMap.at(levelEntityId).back());
        }
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
        AZStd::string_view o3dePrefixPath(s_viewBookmarksRegistryPath);
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
