/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

// AzCore
#include <AzCore/Console/IConsole.h>
#include <AzCore/IO/ByteContainerStream.h>
#include <AzCore/IO/Path/Path.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Settings/SettingsRegistry.h>
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>
#include <AzCore/StringFunc/StringFunc.h>
#include <AzCore/Utils/Utils.h>

#include <API/ComponentEntitySelectionBus.h>
#include <Prefab/ViewBookmarkLoader.h>

#pragma optimize("", off)

static constexpr const char* s_viewBookmarksRegistryPath = "/O3DE/ViewBookmarks/";

void ViewBookmarkLoader::Reflect(AZ::ReflectContext* context)
{
    ViewBookmark::Reflect(context);
    if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
    {
        serializeContext->RegisterGenericType<ViewBookmark>();

        serializeContext->Class<ViewBookmarkLoader>()->Version(0);
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

    if (isLastKnownLocationInLevel)
    {
        // Create the entry, if it already exists then  add the bookmark to it.
        auto existingBookmarkEntry = m_viewBookmarkMap.find(levelEntityId);
        if (existingBookmarkEntry != m_viewBookmarkMap.end())
        {
            AZStd::vector<ViewBookmark>& bookmarks = existingBookmarkEntry->second;
            bookmarks.at(0) = bookmark;
        }
        else
        {
            m_viewBookmarkMap.insert(AZStd::make_pair(levelEntityId, AZStd::vector<ViewBookmark>{ bookmark }));
        }
    }
    else
    {
        auto existingBookmarkEntry = m_viewBookmarkMap.find(levelEntityId);
        if (existingBookmarkEntry != m_viewBookmarkMap.end())
        {
            existingBookmarkEntry->second.push_back(bookmark);
        }
        else
        {
            m_viewBookmarkMap.insert(AZStd::make_pair(levelEntityId, AZStd::vector<ViewBookmark>{ ViewBookmark(), bookmark }));
        }
    }

    // Write to the settings registry
    if (auto registry = AZ::SettingsRegistry::Get())
    {
        size_t currentBookmarkIndex = isLastKnownLocationInLevel ? 0 : m_viewBookmarkMap.at(levelEntityId).size() - 1;
        AZStd::string finalPath = s_viewBookmarksRegistryPath + levelEntityId.ToString() + "/" + AZStd::to_string(currentBookmarkIndex);

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

bool ViewBookmarkLoader::LoadViewBookmarks()
{

    struct ViewBookmarkVisitor : AZ::SettingsRegistryInterface::Visitor
    {
        ViewBookmarkVisitor()
            : m_viewBookmarksKey{ "/O3DE/ViewBookmarks" } {};

        AZ::SettingsRegistryInterface::VisitResponse Traverse(
            AZStd::string_view path,
            AZStd::string_view,
            AZ::SettingsRegistryInterface::VisitAction action,
            AZ::SettingsRegistryInterface::Type) override
        {
            if (action == AZ::SettingsRegistryInterface::VisitAction::Begin)
            {
                // Strip off the last JSON pointer key from the path and if it matches the viewmark key then add an entry
                // to the ViewBookmark Map
                AZStd::optional<AZStd::string_view> levelId = AZ::StringFunc::TokenizeLast(path, "/");
                if (path == m_viewBookmarksKey && levelId && !levelId->empty())
                {
                    AZStd::string_view bookmarkKey = levelId.value();

                    auto existingBookmarkEntry = m_bookmarkMap.find(bookmarkKey);
                    if (existingBookmarkEntry == m_bookmarkMap.end())
                    {
                        m_bookmarkMap.insert(AZStd::make_pair(bookmarkKey, AZStd::vector<ViewBookmark>{}));
                    }
                }
            }

            return AZ::SettingsRegistryInterface::VisitResponse::Continue;
        }

        using AZ::SettingsRegistryInterface::Visitor::Visit;
        void Visit(
            AZStd::string_view path,
            AZStd::string_view valueIndex,
            AZ::SettingsRegistryInterface::Type,
            double value) override
        {
            AZ::StringFunc::TokenizeLast(path, "/");
            AZStd::optional<AZStd::string_view> dataType = AZ::StringFunc::TokenizeLast(path, "/");
            AZStd::optional<AZStd::string_view> bookmarkIndexStr = AZ::StringFunc::TokenizeLast(path, "/");
            AZStd::optional<AZStd::string_view> levelId = AZ::StringFunc::TokenizeLast(path, "/");

            if (path == m_viewBookmarksKey && levelId && !levelId->empty())
            {
                auto existingBookmarkEntry = m_bookmarkMap.find(levelId.value());
                if (existingBookmarkEntry != m_bookmarkMap.end())
                {
                    AZStd::vector<ViewBookmark>& bookmarks = existingBookmarkEntry->second;
                    // if it is the first bookmark and it is the Position data it means it is the first one
                    // and we have to create the Bookmark.
                    if (valueIndex == "0" && dataType == "Position")
                    {
                        ViewBookmark bookmark;
                        bookmark.m_position.SetX(value);
                        bookmarks.push_back(bookmark);
                    }
                    else
                    {
                        int bookmarkIndex = stoi(AZStd::string(bookmarkIndexStr->data()));
                        ViewBookmark& bookmark = bookmarks.at(bookmarkIndex);
                        int currentIndex = stoi(AZStd::string(valueIndex));

                        auto setVec3 = [value, currentIndex](AZ::Vector3& inout)
                        {
                            switch (currentIndex)
                            {
                            case 0:
                                inout.SetX(value);
                                break;
                            case 1:
                                inout.SetY(value);
                                break;
                            case 2:
                                inout.SetZ(value);
                                break;
                            }
                        };

                        if (dataType == "Position")
                        {
                            setVec3(bookmark.m_position);
                        }
                        else if (dataType == "Rotation")
                        {
                            setVec3(bookmark.m_rotation);
                        }
                    }
                }
            }
        };

        [[maybe_unused]] const AZ::SettingsRegistryInterface::FixedValueString m_viewBookmarksKey;
        AZStd::unordered_map<AZStd::string, AZStd::vector<ViewBookmark>> m_bookmarkMap;
    };

    using FixedValueString = AZ::SettingsRegistryInterface::FixedValueString;
    auto viewBookmarkSettingsKey = FixedValueString::format("/O3DE/ViewBookmarks");
    ViewBookmarkVisitor viewBookmarkVisitor;

    auto registry = AZ::SettingsRegistry::Get();
    if (registry == nullptr)
    {
        AZ_Warning("SEditorSettings", false, "Unable to access global settings registry. Editor Preferences cannot be saved");
        return false;
    }
    [[maybe_unused]] const bool visitedViewBookmarks = registry->Visit(viewBookmarkVisitor, viewBookmarkSettingsKey);

    return visitedViewBookmarks;
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
        "SEditorSettings", saved, R"(Unable to save Editor Preferences registry file to path "%s"\n)", editorBookmarkFilePath.c_str());
}

#pragma optimize("", on)
