/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
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
        static constexpr const int LoadingThumbnailSize = 128;

        //////////////////////////////////////////////////////////////////////////
        // LoadingThumbnail
        //////////////////////////////////////////////////////////////////////////
        static constexpr const char* LoadingIconPath = "Assets/Editor/Icons/AssetBrowser/in_progress.gif";

        LoadingThumbnail::LoadingThumbnail()
            : Thumbnail(MAKE_TKEY(ThumbnailKey))
            , m_angle(0)
        {
            auto absoluteIconPath = AZ::IO::FixedMaxPath(AZ::Utils::GetEnginePath()) / LoadingIconPath;
            m_loadingMovie.setFileName(absoluteIconPath.c_str());
            m_loadingMovie.setCacheMode(QMovie::CacheMode::CacheAll);
            m_loadingMovie.setScaledSize(QSize(LoadingThumbnailSize, LoadingThumbnailSize));
            m_loadingMovie.start();
            m_pixmap = m_loadingMovie.currentPixmap();
            m_state = State::Ready;

            BusConnect();
        }

        LoadingThumbnail::~LoadingThumbnail()
        {
            BusDisconnect();
        }

        void LoadingThumbnail::UpdateTime(float)
        {
            m_pixmap = m_loadingMovie.currentPixmap();
            Q_EMIT Updated();
        }

        void LoadingThumbnail::OnTick(float deltaTime, AZ::ScriptTimePoint /*time*/)
        {
            UpdateTime(deltaTime);
        }

    } // namespace Thumbnailer
} // namespace AzToolsFramework

#include "Thumbnails/moc_LoadingThumbnail.cpp"
