/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Asset/AssetCommon.h>

#if !defined(Q_MOC_RUN)
#include <AzToolsFramework/Thumbnails/Thumbnail.h>
#endif

namespace AZ
{
    namespace LyIntegration
    {
        namespace SharedPreviewUtils
        {
            //! Get assetId by assetType that belongs to either source or product thumbnail key
            Data::AssetId GetAssetId(
                AzToolsFramework::Thumbnailer::SharedThumbnailKey key,
                const Data::AssetType& assetType,
                const Data::AssetId& defaultAssetId = {});

            //! Word wrap function for previewer QLabel, since by default it does not break long words such as filenames, so manual word
            //! wrap needed
            QString WordWrap(const QString& string, int maxLength);

            //! Get the set of all asset types supported by the shared preview
            AZStd::unordered_set<AZ::Uuid> GetSupportedAssetTypes();

            //! Determine if a thumbnail key has an asset supported by the shared preview
            bool IsSupportedAssetType(AzToolsFramework::Thumbnailer::SharedThumbnailKey key);
        } // namespace SharedPreviewUtils
    } // namespace LyIntegration
} // namespace AZ
