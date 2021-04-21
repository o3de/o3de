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

#include <AzCore/StringFunc/StringFunc.h>
#include <AzToolsFramework/Thumbnails/LoadingThumbnail.h>
#include <AzFramework/Application/Application.h>

namespace AzToolsFramework
{
    namespace Thumbnailer
    {
        //////////////////////////////////////////////////////////////////////////
        // LoadingThumbnail
        //////////////////////////////////////////////////////////////////////////
        static const char* LoadingIconPath = "Icons/AssetBrowser/in_progress.gif";

        LoadingThumbnail::LoadingThumbnail(int thumbnailSize)
            : Thumbnail(MAKE_TKEY(ThumbnailKey), thumbnailSize)
            , m_angle(0)
        {
            const char* engineRoot = nullptr;
            AzFramework::ApplicationRequests::Bus::BroadcastResult(engineRoot, &AzFramework::ApplicationRequests::GetEngineRoot);
            AZ_Assert(engineRoot, "Engine Root not initialized");
            AZStd::string iconPath;
            AZ::StringFunc::Path::Join(engineRoot, LoadingIconPath, iconPath);
            m_loadingMovie.setFileName(iconPath.c_str());
            m_loadingMovie.setCacheMode(QMovie::CacheMode::CacheAll);
            m_loadingMovie.setScaledSize(QSize(m_thumbnailSize, m_thumbnailSize));
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
