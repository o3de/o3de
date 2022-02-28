/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "API/ToolsApplicationAPI.h"
#include <AzCore/Utils/Utils.h>
#include <AzCore/Settings/SettingsRegistry.h>
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>
#include <AzCore/StringFunc/StringFunc.h>
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

            // Resolve path to editorpreferences.setreg
            AZ::IO::FixedMaxPath editorBookmarkFilePath = AZ::Utils::GetO3deManifestDirectory();
            editorBookmarkFilePath /= "Registry/ViewBookmarks/editorbookmarks.setreg";

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
                "SEditorSettings", saved, R"(Unable to save Editor Preferences registry file to path "%s"\n)",
                editorBookmarkFilePath.c_str());

            registry->Remove("/O3DE/ViewBookmarks");
        }

        bool ViewBookmarkLoader::SaveBookmark(ViewBookmark bookamark)
        {
            ViewBookmarkComponent* bookmarkComponent = FindBookmarkComponent();
            if (bookmarkComponent)
            {
                bookmarkComponent->AddBookmark(bookamark);
                return true;
            }
            return false;
        }

        bool ViewBookmarkLoader::SaveLastKnownLocationInLevel(ViewBookmark bookamark)
        {
            ViewBookmarkComponent* bookmarkComponent = FindBookmarkComponent();
            if (bookmarkComponent)
            {
                bookmarkComponent->SaveLastKnownLocation(bookamark);
                return true;
            }
            return false;
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
                    AZStd::vector<ViewBookmarkComponent*> bookmarkComponents = levelEntity->FindComponents<ViewBookmarkComponent>();
                    if (!bookmarkComponents.empty())
                    {
                        return bookmarkComponents.front();
                    }
                }
            }
            return nullptr;
        }

    } // namespace Prefab
} // namespace AzToolsFramework
