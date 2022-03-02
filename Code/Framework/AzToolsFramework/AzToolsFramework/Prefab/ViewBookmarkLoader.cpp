/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "API/ToolsApplicationAPI.h"
#include <AzCore/Settings/SettingsRegistry.h>
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>
#include <AzCore/StringFunc/StringFunc.h>
#include <AzCore/Utils/Utils.h>
#include <Prefab/ViewBookmarkLoader.h>

/*!
 * ViewBookmarkIntereface
 * Interface for saving/loading View Bookmarks.
 */
namespace AzToolsFramework
{
    namespace Prefab
    {
        static constexpr const char* s_viewBookmarksRegistryPath = "/O3DE/ViewBookmarks/";

        void ViewBookmarkLoader::RegisterViewBookmarkLoaderInterface()
        {
            AZ::Interface<ViewBookmarkLoaderInterface>::Register(this);
        }

        void ViewBookmarkLoader::UnregisterViewBookmarkLoaderInterface()
        {
            AZ::Interface<ViewBookmarkLoaderInterface>::Unregister(this);
        }

        void ViewBookmarkLoader::SaveBookmarkSettingsFile()
        {
            auto registry = AZ::SettingsRegistry::Get();
            if (registry == nullptr)
            {
                AZ_Warning("SEditorSettings", false, "Unable to access global settings registry. Editor Preferences cannot be saved");
                return;
            }

            // Resolve path to user folder
            AZ::IO::FixedMaxPath editorBookmarkFilePath =
                AZ::IO::FixedMaxPath(AZ::Utils::GetProjectPath()) / "user/Registry/ViewBookmarks/";

            editorBookmarkFilePath /= m_bookmarkfileName;

            AZ::SettingsRegistryMergeUtils::DumperSettings dumperSettings;
            dumperSettings.m_prettifyOutput = true;

            AZStd::string bookmarkKey = "/" + m_bookmarkfileName;
            dumperSettings.m_includeFilter = [&bookmarkKey](AZStd::string_view path)
            {
                AZStd::string_view o3dePrefixPath(bookmarkKey);
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

            // Once written to the desired file remove the key from the settings registry
            registry->Remove(bookmarkKey + "/");
            AZ_Warning(
                "SEditorSettings", saved, R"(Unable to save Editor Preferences registry file to path "%s"\n)",
                editorBookmarkFilePath.c_str());
        }

        bool ViewBookmarkLoader::SaveBookmark(ViewBookmark bookmark, StorageMode mode)
        {
            switch (mode)
            {
            case AzToolsFramework::ViewBookmarkLoaderInterface::Shared:
                return SaveSharedBookmark(bookmark);
            case AzToolsFramework::ViewBookmarkLoaderInterface::Local:
                return SaveLocalBookmark(bookmark);
            case AzToolsFramework::ViewBookmarkLoaderInterface::Invalid:
                return false;
            }
            return false;
        }

        bool ViewBookmarkLoader::SaveLastKnownLocationInLevel(ViewBookmark bookmark)
        {
            return SaveLocalBookmark(bookmark, true);
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
                        AZStd::optional<AZStd::string_view> localBookmarksID = AZ::StringFunc::TokenizeLast(path, "/");
                        if (path == m_viewBookmarksKey && localBookmarksID && !localBookmarksID->empty())
                        {
                            AZStd::string_view bookmarkKey = localBookmarksID.value();

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
                    AZStd::string_view path, AZStd::string_view valueIndex, AZ::SettingsRegistryInterface::Type, double value) override
                {
                    AZ::StringFunc::TokenizeLast(path, "/");
                    AZStd::optional<AZStd::string_view> dataType = AZ::StringFunc::TokenizeLast(path, "/");
                    AZStd::optional<AZStd::string_view> bookmarkIndexStr = AZ::StringFunc::TokenizeLast(path, "/");
                    AZStd::optional<AZStd::string_view> bookmarkType;
                    if (bookmarkIndexStr == "LastKnownLocation")
                    {
                        bookmarkType = bookmarkIndexStr;
                    }
                    else
                    {
                        // differentiate between local Bookmarks and LastKnownLocation
                        bookmarkType = AZ::StringFunc::TokenizeLast(path, "/");
                    }
                    AZStd::optional<AZStd::string_view> localBookmarksID = AZ::StringFunc::TokenizeLast(path, "/");

                    if (path == m_viewBookmarksKey && localBookmarksID && !localBookmarksID->empty())
                    {
                        auto setVec3 = [value](AZ::Vector3& inout, int currentIndex)
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

                        if (bookmarkType == "LastKnownLocation")
                        {
                            int currentIndex = stoi(AZStd::string(valueIndex));
                            if (dataType == "Position")
                            {
                                setVec3(m_lastKnownLocation.m_position, currentIndex);
                            }
                            else if (dataType == "Rotation")
                            {
                                setVec3(m_lastKnownLocation.m_rotation, currentIndex);
                            }
                        }
                        else if(bookmarkType == "LocalBookmarks")
                        {
                            auto existingBookmarkEntry = m_bookmarkMap.find(localBookmarksID.value());
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

                                    if (dataType == "Position")
                                    {
                                        setVec3(bookmark.m_position, currentIndex);
                                    }
                                    else if (dataType == "Rotation")
                                    {
                                        setVec3(bookmark.m_rotation, currentIndex);
                                    }
                                }
                            }
                        }
                    }
                };

                const AZ::SettingsRegistryInterface::FixedValueString m_viewBookmarksKey;
                AZStd::unordered_map<AZStd::string, AZStd::vector<ViewBookmark>> m_bookmarkMap;
                ViewBookmark m_lastKnownLocation;
            };

            ViewBookmarkComponent* bookmarkComponent = FindBookmarkComponent();
            if (bookmarkComponent)
            {
                // Get the file we want to merge into the settings registry.
                if (!bookmarkComponent->GetLocalBookmarksFileName().empty())
                {
                    auto registry = AZ::SettingsRegistry::Get();
                    if (registry == nullptr)
                    {
                        AZ_Warning(
                            "SEditorSettings", false, "Unable to access global settings registry. Editor Preferences cannot be saved");
                        return false;
                    }

                    auto projectUserRegistryPath = AZ::IO::FixedMaxPath(AZ::Utils::GetProjectPath()) / "user/Registry/ViewBookmarks";
                    projectUserRegistryPath /= AZ::IO::FixedMaxPathString(bookmarkComponent->GetLocalBookmarksFileName());

                    // Merge the current viewBookmark file into the settings registry.
                    bool isMerged = registry->MergeSettingsFile(
                        projectUserRegistryPath.Native(), AZ::SettingsRegistryInterface::Format::JsonMergePatch, "/O3DE/ViewBookmarks");

                    if (isMerged)
                    {
                        using FixedValueString = AZ::SettingsRegistryInterface::FixedValueString;
                        AZStd::string bookmarkKey = s_viewBookmarksRegistryPath + m_bookmarkfileName;
                        auto viewBookmarkSettingsKey = FixedValueString::format(bookmarkKey.c_str());
                        ViewBookmarkVisitor viewBookmarkVisitor;

                        const bool visitedViewBookmarks = registry->Visit(viewBookmarkVisitor, viewBookmarkSettingsKey);

                        if (visitedViewBookmarks)
                        {
                            m_localBookmarks = viewBookmarkVisitor.m_bookmarkMap.at(m_bookmarkfileName);
                            m_localBookmarkCount = m_localBookmarks.size() - 1;
                            m_lastKnownLocation = viewBookmarkVisitor.m_lastKnownLocation;
                        }

                        // once loaded we can remove the data from the settings registry.
                        registry->Remove(bookmarkKey + "/");
                        return visitedViewBookmarks;
                    }
                }
            }

            return false;
        }

        ViewBookmark ViewBookmarkLoader::GetBookmarkAtIndex(int index) const
        {
            ViewBookmarkComponent* bookmarkComponent = FindBookmarkComponent();
            if (bookmarkComponent)
            {
                return bookmarkComponent->GetBookmarkAtIndex(index);
            }

            return ViewBookmark();
        }

        ViewBookmark ViewBookmarkLoader::GetLastKnownLocationInLevel() const
        {
            return ViewBookmark();
        }

        ViewBookmarkComponent* ViewBookmarkLoader::FindBookmarkComponent() const
        {
            AZ::EntityId levelEntityId;
            AzToolsFramework::ToolsApplicationRequestBus::BroadcastResult(
                levelEntityId, &AzToolsFramework::ToolsApplicationRequests::GetCurrentLevelEntityId);

            if (levelEntityId.IsValid())
            {
                AZ::Entity* levelEntity = nullptr;
                AZ::ComponentApplicationBus::BroadcastResult(levelEntity, &AZ::ComponentApplicationBus::Events::FindEntity, levelEntityId);
                if (levelEntity)
                {
                    ViewBookmarkComponent* bookmarkComponent = levelEntity->FindComponent<ViewBookmarkComponent>();
                    if (bookmarkComponent)
                    {
                        return bookmarkComponent;
                    }
                }
            }
            return nullptr;
        }

        AZStd::string ViewBookmarkLoader::GenerateBookmarkFileName() const
        {
            AZ::EntityId levelEntityId;
            AzToolsFramework::ToolsApplicationRequestBus::BroadcastResult(
                levelEntityId, &AzToolsFramework::ToolsApplicationRequests::GetCurrentLevelEntityId);

            if (levelEntityId.IsValid())
            {
                AZ::Entity* levelEntity = nullptr;
                AZ::ComponentApplicationBus::BroadcastResult(levelEntity, &AZ::ComponentApplicationBus::Events::FindEntity, levelEntityId);
                if (levelEntity)
                {
                    AZStd::chrono::system_clock::time_point now = AZStd::chrono::system_clock::now();
                    AZStd::string sTime = AZStd::string::format("%llu", now.time_since_epoch().count());

                    return levelEntity->GetName() + sTime + ".setreg";
                }
            }

            return AZStd::string();
        }
        bool ViewBookmarkLoader::SaveSharedBookmark(ViewBookmark& bookmark)
        {
            ViewBookmarkComponent* bookmarkComponent = FindBookmarkComponent();
            if (bookmarkComponent)
            {
                bookmarkComponent->AddBookmark(bookmark);
                return true;
            }
            return false;
        }

        bool ViewBookmarkLoader::SaveLocalBookmark(ViewBookmark& bookmark, bool isLastKnownLocation)
        {
            // Write to the settings registry
            if (auto registry = AZ::SettingsRegistry::Get())
            {
                // This adds a dependency on the ViewBookmarkComponent. If necessary we could move the "localBookmarksFile" field
                // to another part of the prefab.
                ViewBookmarkComponent* bookmarkComponent = FindBookmarkComponent();

                if (bookmarkComponent)
                {
                    // if the field is not empty then we have a file linked to the prefab.
                    if (bookmarkComponent && !bookmarkComponent->GetLocalBookmarksFileName().empty())
                    {
                        // TODO: check that the file actually exists
                        m_bookmarkfileName = bookmarkComponent->GetLocalBookmarksFileName();
                    }
                    else // if the field is empty we don't have a file linked to the prefab, so we create one and we save it in the
                         // component
                    {
                        m_bookmarkfileName = GenerateBookmarkFileName();
                        bookmarkComponent->SetLocalBookmarksFileName(m_bookmarkfileName);
                    }
                }
                else
                {
                    AZ_Warning("ViewBookmarkLoader", false, "Couldn't retreive a ViewBookmarkComponent from the level entity.");
                    return false;
                }

                AZStd::string finalPath;
                if (!isLastKnownLocation)
                {
                    finalPath = "/" + m_bookmarkfileName + "/LocalBookmarks/" + AZStd::to_string(m_localBookmarkCount++);
                }
                else
                {
                    finalPath = "/" + m_bookmarkfileName + "/" + "LastKnownLocation";
                }

                bool success = registry->SetObject(finalPath, bookmark);

                // If we managed to add the bookmark
                if (success)
                {
                    SaveBookmarkSettingsFile();
                    LoadViewBookmarks();
                }

                return success;
            }
            return false;
        }

    } // namespace Prefab
} // namespace AzToolsFramework
