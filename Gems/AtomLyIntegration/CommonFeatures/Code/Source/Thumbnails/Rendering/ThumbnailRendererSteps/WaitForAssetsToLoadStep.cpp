/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "Thumbnails/ThumbnailerBus.h"

#include <Thumbnails/Rendering/ThumbnailRendererData.h>
#include <Thumbnails/Rendering/ThumbnailRendererContext.h>
#include <Thumbnails/Rendering/ThumbnailRendererSteps/CaptureStep.h>
#include <Thumbnails/Rendering/ThumbnailRendererSteps/WaitForAssetsToLoadStep.h>

namespace AZ
{
    namespace LyIntegration
    {
        namespace Thumbnails
        {
            WaitForAssetsToLoadStep::WaitForAssetsToLoadStep(ThumbnailRendererContext* context)
                : ThumbnailRendererStep(context)
            {
            }

            void WaitForAssetsToLoadStep::Start()
            {
                LoadNextAsset();
            }

            void WaitForAssetsToLoadStep::Stop()
            {
                Data::AssetBus::Handler::BusDisconnect();
                TickBus::Handler::BusDisconnect();
                m_context->GetData()->m_assetsToLoad.clear();
            }

            void WaitForAssetsToLoadStep::LoadNextAsset()
            {
                if (m_context->GetData()->m_assetsToLoad.empty())
                {
                    // When all assets are loaded, render the thumbnail itself
                    m_context->SetStep(Step::Capture);
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

            void WaitForAssetsToLoadStep::OnAssetReady([[maybe_unused]] Data::Asset<Data::AssetData> asset)
            {
                Data::AssetBus::Handler::BusDisconnect();
                LoadNextAsset();
            }

            void WaitForAssetsToLoadStep::OnAssetError([[maybe_unused]] Data::Asset<Data::AssetData> asset)
            {
                Data::AssetBus::Handler::BusDisconnect();
                AzToolsFramework::Thumbnailer::ThumbnailerRendererNotificationBus::Event(
                    m_context->GetData()->m_thumbnailKeyRendered,
                    &AzToolsFramework::Thumbnailer::ThumbnailerRendererNotifications::ThumbnailFailedToRender);
                m_context->SetStep(Step::FindThumbnailToRender);
            }

            void WaitForAssetsToLoadStep::OnAssetCanceled([[maybe_unused]] Data::AssetId assetId)
            {
                Data::AssetBus::Handler::BusDisconnect();
                AzToolsFramework::Thumbnailer::ThumbnailerRendererNotificationBus::Event(
                    m_context->GetData()->m_thumbnailKeyRendered,
                    &AzToolsFramework::Thumbnailer::ThumbnailerRendererNotifications::ThumbnailFailedToRender);
                m_context->SetStep(Step::FindThumbnailToRender);
            }

            void WaitForAssetsToLoadStep::OnTick(float deltaTime, [[maybe_unused]] AZ::ScriptTimePoint time)
            {
                m_timeRemainingS -= deltaTime;
                if (m_timeRemainingS < 0)
                {
                    auto assetIdStr = m_assetId.ToString<AZStd::string>();
                    AZ_Warning("CommonThumbnailRenderer", false, "Timed out waiting for asset %s to load.", assetIdStr.c_str());
                    AzToolsFramework::Thumbnailer::ThumbnailerRendererNotificationBus::Event(
                        m_context->GetData()->m_thumbnailKeyRendered,
                        &AzToolsFramework::Thumbnailer::ThumbnailerRendererNotifications::ThumbnailFailedToRender);
                    m_context->SetStep(Step::FindThumbnailToRender);
                }
            }
        } // namespace Thumbnails
    } // namespace LyIntegration
} // namespace AZ
