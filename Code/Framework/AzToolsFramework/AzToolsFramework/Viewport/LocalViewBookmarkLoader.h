/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once
#include <AzCore/Settings/SettingsRegistryImpl.h>
#include <Viewport/LocalViewBookmarkComponent.h>
#include <Viewport/ViewBookmarkLoaderInterface.h>

namespace AzToolsFramework
{
    //! @class LocalViewBookmarkLoader.
    //! @brief class used to load/store local ViewBookmarks from project/user/Registry/ViewBookmarks.
    class LocalViewBookmarkLoader final : public ViewBookmarkLoaderInterface
    {
    public:
        AZ_CLASS_ALLOCATOR(LocalViewBookmarkLoader, AZ::SystemAllocator, 0);
        AZ_RTTI(LocalViewBookmarkLoader, "{A64F2300-0958-4430-9EEA-1D457997E618}", ViewBookmarkLoaderInterface);

        enum class ViewBookmarkType : int
        {
            Standard,
            LastKnownLocation
        };

        void RegisterViewBookmarkLoaderInterface();
        void UnregisterViewBookmarkLoaderInterface();

        // ViewBookmarkLoaderInterface overrides ...
        bool SaveBookmark(const ViewBookmark& bookmark) override;
        bool ModifyBookmarkAtIndex(const ViewBookmark& bookmark, int index) override;
        bool SaveLastKnownLocation(const ViewBookmark& bookmark) override;
        bool RemoveBookmarkAtIndex(int index) override;
        AZStd::optional<ViewBookmark> LoadBookmarkAtIndex(int index) override;
        AZStd::optional<ViewBookmark> LoadLastKnownLocation() const override;

    private:
        bool SaveLocalBookmark(const ViewBookmark& bookmark, ViewBookmarkType bookmarkType);

        void SetupLocalViewBookmarkComponent();
        void WriteBookmarksSettingsRegistryToFile();
        bool ReadViewBookmarksFromSettingsRegistry();
        AZStd::string GenerateBookmarkFileName() const;
        LocalViewBookmarkComponent* RetrieveLocalViewBookmarkComponent();

        AZStd::vector<ViewBookmark> m_localBookmarks;
        AZStd::optional<ViewBookmark> m_lastKnownLocation;
        AZStd::string m_bookmarkFileName;
    };
} // namespace AzToolsFramework
