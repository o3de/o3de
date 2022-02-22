/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Interface/Interface.h>
#include <AzCore/Serialization/SerializeContext.h>

 /*!
 * ViewBookmarkIntereface
 * Interface for saving/loading View Bookmarks.
 */
namespace AzToolsFramework
{
    namespace Prefab
    {
        struct ViewBookmark;
        class ViewBookmarkLoaderInterface
        {
        public:
            AZ_RTTI(ViewBookmarkLoaderInterface, "{71E7E178-4107-4975-A6E6-1C4B005C981A}");

            virtual void SaveBookmarkSettingsFile() = 0;
            virtual bool SaveBookmark(ViewBookmark bookamark) = 0;
            virtual bool SaveLastKnownLocationInLevel(ViewBookmark bookamark) = 0;
            virtual bool LoadViewBookmarks() = 0;
        };
    } // namespace Prefab
} // namespace AzToolsFramework
