/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once
#include <AzCore/Settings/SettingsRegistryImpl.h>
#include <Prefab/ViewBookmarkLoaderInterface.h>

namespace AzToolsFramework
{
    namespace Prefab
    {
        class ViewBookmarkLoader final : public ViewBookmarkLoaderInterface
        {
        public:
            AZ_CLASS_ALLOCATOR(ViewBookmarkLoader, AZ::SystemAllocator, 0);
            AZ_RTTI(ViewBookmarkLoader, "{A64F2300-0958-4430-9EEA-1D457997E618}", ViewBookmarkLoaderInterface);

            void RegisterViewBookmarkLoaderInterface();
            void UnregisterViewBookmarkLoaderInterface();

            void SaveBookmarkSettingsFile() override;
            bool SaveBookmark(ViewBookmark bookamark) override;
            bool SaveLastKnownLocationInLevel(ViewBookmark bookamark) override;
            bool LoadViewBookmarks() override;
            ViewBookmark GetBookmarkAtIndex(int index) const override;
            ViewBookmark GetLastKnownLocationInLevel() const override;

        private:
            ViewBookmarkComponent* FindBookmarkComponent() const;
            AZStd::string GenerateBookmarkFileName() const;
        };

    } // namespace Prefab
} // namespace AzToolsFramework
