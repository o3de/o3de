/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Prefab/ViewBookmarkLoader.h>
#include "API/ToolsApplicationAPI.h"

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

            //TODO: return invalid bookmark;
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
