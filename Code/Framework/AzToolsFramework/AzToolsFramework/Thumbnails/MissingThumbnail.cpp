/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Utils/Utils.h>
#include <AzFramework/API/ApplicationAPI.h>
#include <AzToolsFramework/Thumbnails/MissingThumbnail.h>

namespace AzToolsFramework
{
    namespace Thumbnailer
    {
        static constexpr const char* MissingIconPath = "Assets/Editor/Icons/AssetBrowser/Default_16.svg";

        MissingThumbnail::MissingThumbnail()
            : Thumbnail(MAKE_TKEY(ThumbnailKey))
        {
            auto absoluteIconPath = AZ::IO::FixedMaxPath(AZ::Utils::GetEnginePath()) / MissingIconPath;
            m_pixmap.load(absoluteIconPath.c_str());
            m_state = m_pixmap.isNull() ? State::Failed : State::Ready;
        }

    } // namespace Thumbnailer
} // namespace AzToolsFramework

#include "Thumbnails/moc_MissingThumbnail.cpp"
