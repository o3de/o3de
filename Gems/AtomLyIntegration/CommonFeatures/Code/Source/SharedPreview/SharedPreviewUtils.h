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
#include <AzCore/Asset/AssetManagerBus.h>
#include <AzToolsFramework/Thumbnails/Thumbnail.h>
#endif

namespace AZ
{
    namespace LyIntegration
    {
        namespace SharedPreviewUtils
        {
            //! Get the set of all asset types supported by the shared preview
            AZStd::vector<AZ::Uuid> GetSupportedAssetTypes();

            //! Determine if a thumbnail key has an asset supported by the shared preview
            bool IsSupportedAssetType(AzToolsFramework::Thumbnailer::SharedThumbnailKey key);

            //! Get assetInfo of source or product thumbnail key if asset type is supported by the shared preview
            AZ::Data::AssetInfo GetSupportedAssetInfo(AzToolsFramework::Thumbnailer::SharedThumbnailKey key);

            //! Get assetId of source or product thumbnail key if asset type is supported by the shared preview
            AZ::Data::AssetId GetSupportedAssetId(
                AzToolsFramework::Thumbnailer::SharedThumbnailKey key, const AZ::Data::AssetId& defaultAssetId = {});

            //! Wraps AZ::RPI::AssetUtils::GetAssetIdForProductPath to handle empty productPath
            AZ::Data::AssetId GetAssetIdForProductPath(const AZStd::string_view productPath);

            //! Inserts new line characters into a string whenever the maximum number of characters per line is exceeded
            QString WordWrap(const QString& string, int maxLength);

        } // namespace SharedPreviewUtils
    } // namespace LyIntegration
} // namespace AZ
