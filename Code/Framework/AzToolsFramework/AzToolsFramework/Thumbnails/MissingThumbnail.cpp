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

#include <AzToolsFramework/Thumbnails/MissingThumbnail.h>

namespace AzToolsFramework
{
    namespace Thumbnailer
    {
        static const char* MISSING_ICON_PATH = "Icons/AssetBrowser/Default_16.svg";

        MissingThumbnail::MissingThumbnail(int thumbnailSize)
            : Thumbnail(MAKE_TKEY(ThumbnailKey), thumbnailSize)
        {
            m_icon = QIcon(MISSING_ICON_PATH);
            m_state = State::Ready;
        }

    } // namespace Thumbnailer
} // namespace AzToolsFramework

#include "Thumbnails/moc_MissingThumbnail.cpp"
