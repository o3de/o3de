/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Prefab/ViewBookmarkComponent.h>
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
        }

        bool ViewBookmarkLoader::SaveBookmark([[maybe_unused]] ViewBookmark bookamark)
        {
            return false;
        }

        bool ViewBookmarkLoader::SaveLastKnownLocationInLevel([[maybe_unused]] ViewBookmark bookamark)
        {
            return false;
        }

        bool ViewBookmarkLoader::LoadViewBookmarks()
        {
            return false;
        }

    } // namespace Prefab
} // namespace AzToolsFramework
