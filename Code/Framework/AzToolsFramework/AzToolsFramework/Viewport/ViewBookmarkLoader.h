/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once
#include <AzCore/Settings/SettingsRegistryImpl.h>
#include <Viewport/ViewBookmarkLoaderInterface.h>

namespace AzToolsFramework
{
    class ViewBookmarkLoader final : public ViewBookmarkLoaderInterface
    {
    public:
        AZ_CLASS_ALLOCATOR(ViewBookmarkLoader, AZ::SystemAllocator, 0);
        AZ_RTTI(ViewBookmarkLoader, "{A64F2300-0958-4430-9EEA-1D457997E618}", ViewBookmarkLoaderInterface);

        void RegisterViewBookmarkLoaderInterface();
        void UnregisterViewBookmarkLoaderInterface();

        bool SaveBookmark(ViewBookmark bookmark, const StorageMode mode) override;

        bool SaveLastKnownLocationInLevel(ViewBookmark bookmark) override;
        bool LoadViewBookmarks() override;
        ViewBookmark GetBookmarkAtIndex(int index) const override;
        ViewBookmark GetLastKnownLocationInLevel() const override;

    private:
        void SaveBookmarkSettingsFile() override;
        bool SaveLocalBookmark(ViewBookmark& bookmark, bool isLastKnownLocation = false);
        bool SaveSharedBookmark(ViewBookmark& bookmark);

        template<typename BookmarkComponentType>
        BookmarkComponentType* FindBookmarkComponent() const;

        AZStd::string GenerateBookmarkFileName() const;

    private:
        AZStd::vector<ViewBookmark> m_localBookmarks;
        ViewBookmark m_lastKnownLocation;
        size_t m_localBookmarkCount = 0;
        AZStd::string m_bookmarkfileName;
    };

} // namespace AzToolsFramework
