/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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
