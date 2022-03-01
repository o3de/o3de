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

            // Resolve path user/.o3de
            AZ::IO::FixedMaxPath editorBookmarkFilePath = AZ::Utils::GetO3deManifestDirectory();

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
                else // if the field is empty we don't have a file linked to the prefab, so we create one and we save it in the component
                {
                    m_bookmarkfileName = GenerateBookmarkFileName();
                    bookmarkComponent->SetLocalBookmarksFileName(m_bookmarkfileName);
                }
            }
            else
            {
                AZ_Error("ViewBookmarkLoader", false, "Couldn't retreive a ViewBookmarkComponent from the level entity.");
                return;
            }

            editorBookmarkFilePath /= "Registry/ViewBookmarks/";
            editorBookmarkFilePath /= m_bookmarkfileName;

            AZ::SettingsRegistryMergeUtils::DumperSettings dumperSettings;
            dumperSettings.m_prettifyOutput = true;

            AZStd::string bookmarkKey = m_bookmarkfileName;
            dumperSettings.m_includeFilter = [&bookmarkKey](AZStd::string_view path)
            {
                AZStd::string_view o3dePrefixPath(s_viewBookmarksRegistryPath + bookmarkKey + "/");
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
                "SEditorSettings", saved, R"(Unable to save Editor Preferences registry file to path "%s"\n)",
                editorBookmarkFilePath.c_str());

            /*registry->Remove("/O3DE/ViewBookmarks");*/
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
            return false;
        }

        ViewBookmark ViewBookmarkLoader::GetBookmarkAtIndex(int index) const
        {
            ViewBookmarkComponent* bookmarkComponent = FindBookmarkComponent();
            if (bookmarkComponent)
            {
                return bookmarkComponent->GetBookmarkAtIndex(index);
            }

            // TODO: return invalid bookmark;
            return ViewBookmark();
        }

        ViewBookmark ViewBookmarkLoader::GetLastKnownLocationInLevel() const
        {
            ViewBookmarkComponent* bookmarkComponent = FindBookmarkComponent();
            if (bookmarkComponent)
            {
                return bookmarkComponent->GetLastKnownLocation();
            }

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

            return AZStd::string("");
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
                AZStd::string finalPath;
                if (!isLastKnownLocation)
                {
                    m_localBookmarks.push_back(bookmark);
                    finalPath = s_viewBookmarksRegistryPath + m_bookmarkfileName + "/" + AZStd::to_string(m_localBookmarks.size() - 1);
                }
                else
                {
                    m_lastKnownLocation = bookmark;
                    finalPath = s_viewBookmarksRegistryPath + m_bookmarkfileName + "/" + "LastKnownLocation";
                }

                bool success = registry->SetObject(finalPath, bookmark);

                // If we managed to add the bookmark
                if (success)
                {
                    SaveBookmarkSettingsFile();
                }

                return success;
            }
            return false;
        }

    } // namespace Prefab
} // namespace AzToolsFramework
