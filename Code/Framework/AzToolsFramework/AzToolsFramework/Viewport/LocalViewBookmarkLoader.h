/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Settings/SettingsRegistryImpl.h>
#include <AzToolsFramework/Entity/EditorEntityContextBus.h>
#include <Viewport/ViewBookmarkLoaderInterface.h>

namespace AzToolsFramework
{
    class LocalViewBookmarkComponent;

    //! @class LocalViewBookmarkLoader.
    //! @brief class used to load/store local ViewBookmarks from project/user/Registry/ViewBookmarks.
    class LocalViewBookmarkLoader final
        : public ViewBookmarkInterface
        , public ViewBookmarkPersistInterface
    {
    public:
        AZ_CLASS_ALLOCATOR(LocalViewBookmarkLoader, AZ::SystemAllocator, 0);
        AZ_RTTI(LocalViewBookmarkLoader, "{A64F2300-0958-4430-9EEA-1D457997E618}", ViewBookmarkInterface);

        void RegisterViewBookmarkInterface();
        void UnregisterViewBookmarkInterface();

        // ViewBookmarkLoaderInterface overrides ...
        bool SaveBookmarkAtIndex(const ViewBookmark& bookmark, int index) override;
        bool SaveLastKnownLocation(const ViewBookmark& bookmark) override;
        bool RemoveBookmarkAtIndex(int index) override;
        AZStd::optional<ViewBookmark> LoadBookmarkAtIndex(int index) override;
        AZStd::optional<ViewBookmark> LoadLastKnownLocation() override;

        // ViewBookmarkPersistInterface overrides ...
        void SetStreamWriteFn(StreamWriteFn streamWriteFn) override;
        void SetStreamReadFn(StreamReadFn streamReadFn) override;
        void SetFileExistsFn(FileExistsFn fileExistsFn) override;

    private:
        LocalViewBookmarkComponent* FindOrCreateLocalViewBookmarkComponent();

        void WriteViewBookmarksSettingsRegistryToFile(const AZStd::string& localBookmarksFileName);
        bool ReadViewBookmarksSettingsRegistryFromFile(const AZStd::string& localBookmarksFileName);

        //! Writes the content of the string buffer to the stream.
        bool Write(AZ::IO::GenericStream& stream, const AZStd::string& stringBuffer);

        AZStd::vector<ViewBookmark> m_localBookmarks;
        AZStd::optional<ViewBookmark> m_lastKnownLocation;

        StreamWriteFn m_streamWriteFn;
        StreamReadFn m_streamReadFn;
        FileExistsFn m_fileExistsFn;
    };

    //! Helper to store the last known location using the current active camera position
    void StoreViewBookmarkLastKnownLocationFromActiveCamera();
} // namespace AzToolsFramework
