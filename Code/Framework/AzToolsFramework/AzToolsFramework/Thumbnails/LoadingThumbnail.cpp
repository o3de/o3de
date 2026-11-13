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

        // =======================================================================
        // Paint Colours
        // =======================================================================

        //! The colour the placeholder of a loading thumbnail is filled with, named, so the legacy and the flattened GUI read one value. A
        //! named namespace, not an anonymous one: a unity build joins files into one translation unit, where anonymous namespaces merge.
        namespace LoadingThumbnailFlatInternal
        {
            const QColor Fill(Qt::black);   // LoadingThumbnail.cpp:26. TOKENIZE: LoadingThumbnailFill
        }

        //////////////////////////////////////////////////////////////////////////
        // LoadingThumbnail
        //////////////////////////////////////////////////////////////////////////
        LoadingThumbnail::LoadingThumbnail()
            : Thumbnail(MAKE_TKEY(ThumbnailKey))
        {
            m_pixmap = QPixmap(LoadingThumbnailSize, LoadingThumbnailSize);
            m_pixmap.fill(LoadingThumbnailFlatInternal::Fill);
            m_state = State::Ready;
        }
   } // namespace Thumbnailer
} // namespace AzToolsFramework

