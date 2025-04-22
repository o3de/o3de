/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#if !defined(Q_MOC_RUN)
#include <AzCore/Asset/AssetCommon.h>

#include <QObject>
#endif

namespace AzToolsFramework
{
    namespace AssetBrowser
    {
        class AssetBrowserEntry;

        class AssetBrowserFavoriteItem
            : public QObject
        {
            Q_OBJECT
        public:
            enum class FavoriteType
            {
                AssetBrowserEntry,
                Search
            };

            AZ_RTTI(AssetBrowserFavoriteItem, "{DA9CFE0D-41B5-43C3-8909-9924D28E51C0}");

            virtual FavoriteType GetFavoriteType() const = 0;
        };
    } // namespace AssetBrowser
} // namespace AzToolsFramework

Q_DECLARE_METATYPE(const AzToolsFramework::AssetBrowser::AssetBrowserFavoriteItem*)
