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
#include <Viewport/ViewBookmarkComponent.h>

namespace AzToolsFramework
{
    //! @class ViewBookmarkIntereface
    //! @brief Interface for saving/loading View Bookmarks.
    class ViewBookmarkLoaderInterface
    {
    public:
        AZ_RTTI(ViewBookmarkLoaderInterface, "{71E7E178-4107-4975-A6E6-1C4B005C981A}")

        enum class StorageMode : int
        {
            Shared = 0,
            Local = 1,
            Invalid = -1
        };

        virtual bool SaveBookmark(ViewBookmark bookamark, const StorageMode mode) = 0;
        virtual bool SaveLastKnownLocationInLevel(ViewBookmark bookamark) = 0;
        virtual bool LoadViewBookmarks() = 0;
        virtual ViewBookmark GetBookmarkAtIndex(int index) const = 0;
        virtual ViewBookmark GetLastKnownLocationInLevel() const = 0;

    private:
        virtual void SaveBookmarkSettingsFile() = 0;
    };
} // namespace AzToolsFramework
