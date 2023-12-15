/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Utils/Utils.h>
#include <AzToolsFramework/Thumbnails/LoadingThumbnail.h>
#include <AzFramework/Application/Application.h>

namespace AzToolsFramework
{
    namespace Thumbnailer
    {
        static constexpr const int LoadingThumbnailSize = 1;

        //////////////////////////////////////////////////////////////////////////
        // LoadingThumbnail
        //////////////////////////////////////////////////////////////////////////
        LoadingThumbnail::LoadingThumbnail()
            : Thumbnail(MAKE_TKEY(ThumbnailKey))
        {
            m_pixmap = QPixmap(LoadingThumbnailSize, LoadingThumbnailSize);
            m_pixmap.fill(Qt::black);
            m_state = State::Ready;
        }
   } // namespace Thumbnailer
} // namespace AzToolsFramework

#include "Thumbnails/moc_LoadingThumbnail.cpp"
