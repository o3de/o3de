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
        namespace Thumbnails
        {
            //! Get assetId by assetType that belongs to either source or product thumbnail key
            Data::AssetId GetAssetId(AzToolsFramework::Thumbnailer::SharedThumbnailKey key, const Data::AssetType& assetType);

            //! Word wrap function for previewer QLabel, since by default it does not break long words such as filenames, so manual word wrap needed
            QString WordWrap(const QString& string, int maxLength);
        } // namespace Thumbnails
    } // namespace LyIntegration
} // namespace AZ
