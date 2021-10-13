/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "Thumbnails/ThumbnailerBus.h"
#include <SharedPreview/SharedPreviewRendererCaptureState.h>
#include <SharedPreview/SharedPreviewRendererContext.h>
#include <SharedPreview/SharedPreviewRendererData.h>
#include <SharedPreview/SharedPreviewRendererLoadState.h>

namespace AZ
{
    namespace LyIntegration
    {
        SharedPreviewRendererLoadState::SharedPreviewRendererLoadState(SharedPreviewRendererContext* context)
            : SharedPreviewRendererState(context)
        {
        }

        void SharedPreviewRendererLoadState::Start()
        {
            LoadNextAsset();
        }

        void SharedPreviewRendererLoadState::Stop()
        {
            Data::AssetBus::Handler::BusDisconnect();
            TickBus::Handler::BusDisconnect();
            m_context->GetData()->m_assetsToLoad.clear();
        }

        void SharedPreviewRendererLoadState::LoadNextAsset()
        {
            if (m_context->GetData()->m_assetsToLoad.empty())
            {
                // When all assets are loaded, render the thumbnail itself
                m_context->SetState(State::Capture);
            }
            else
            {
                // Pick the the next asset and wait until its ready
                const auto assetIdIt = m_context->GetData()->m_assetsToLoad.begin();
                m_context->GetData()->m_assetsToLoad.erase(assetIdIt);
                m_assetId = *assetIdIt;
                Data::AssetBus::Handler::BusConnect(m_assetId);
                // If asset is already loaded, then AssetEvents will call OnAssetReady instantly and we don't need to wait this time
                if (Data::AssetBus::Handler::BusIsConnected())
                {
                    TickBus::Handler::BusConnect();
                    m_timeRemainingS = TimeOutS;
                }
            }
        }

        void SharedPreviewRendererLoadState::OnAssetReady([[maybe_unused]] Data::Asset<Data::AssetData> asset)
        {
            Data::AssetBus::Handler::BusDisconnect();
            LoadNextAsset();
        }

        void SharedPreviewRendererLoadState::OnAssetError([[maybe_unused]] Data::Asset<Data::AssetData> asset)
        {
            Data::AssetBus::Handler::BusDisconnect();
            AzToolsFramework::Thumbnailer::ThumbnailerRendererNotificationBus::Event(
                m_context->GetData()->m_thumbnailKeyRendered,
                &AzToolsFramework::Thumbnailer::ThumbnailerRendererNotifications::ThumbnailFailedToRender);
            m_context->SetState(State::Idle);
        }

        void SharedPreviewRendererLoadState::OnAssetCanceled([[maybe_unused]] Data::AssetId assetId)
        {
            Data::AssetBus::Handler::BusDisconnect();
            AzToolsFramework::Thumbnailer::ThumbnailerRendererNotificationBus::Event(
                m_context->GetData()->m_thumbnailKeyRendered,
                &AzToolsFramework::Thumbnailer::ThumbnailerRendererNotifications::ThumbnailFailedToRender);
            m_context->SetState(State::Idle);
        }

        void SharedPreviewRendererLoadState::OnTick(float deltaTime, [[maybe_unused]] AZ::ScriptTimePoint time)
        {
            m_timeRemainingS -= deltaTime;
            if (m_timeRemainingS < 0)
            {
                auto assetIdStr = m_assetId.ToString<AZStd::string>();
                AZ_Warning("SharedPreviewRenderer", false, "Timed out waiting for asset %s to load.", assetIdStr.c_str());
                AzToolsFramework::Thumbnailer::ThumbnailerRendererNotificationBus::Event(
                    m_context->GetData()->m_thumbnailKeyRendered,
                    &AzToolsFramework::Thumbnailer::ThumbnailerRendererNotifications::ThumbnailFailedToRender);
                m_context->SetState(State::Idle);
            }
        }
    } // namespace LyIntegration
} // namespace AZ
