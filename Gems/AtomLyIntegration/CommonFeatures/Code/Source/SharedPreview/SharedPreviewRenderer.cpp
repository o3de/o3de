/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RPI.Reflect/Material/MaterialAsset.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserBus.h>
#include <SharedPreview/SharedPreviewRenderer.h>
#include <SharedPreview/SharedPreviewRendererData.h>
#include <SharedPreview/SharedPreviewRendererCaptureState.h>
#include <SharedPreview/SharedPreviewRendererIdleState.h>
#include <SharedPreview/SharedPreviewRendererInitState.h>
#include <SharedPreview/SharedPreviewRendererReleaseState.h>
#include <SharedPreview/SharedPreviewRendererLoadState.h>

namespace AZ
{
    namespace LyIntegration
    {
        namespace Thumbnails
        {
            SharedPreviewRenderer::SharedPreviewRenderer()
                : m_data(new SharedPreviewRendererData)
            {
                for (const AZ::Uuid& typeId : SharedPreviewUtils::GetSupportedAssetTypes())
                {
                    AzToolsFramework::Thumbnailer::ThumbnailerRendererRequestBus::MultiHandler::BusConnect(typeId);
                }
                SystemTickBus::Handler::BusConnect();
                ThumbnailFeatureProcessorProviderBus::Handler::BusConnect();

                m_states[State::Init] = AZStd::make_shared<SharedPreviewRendererInitState>(this);
                m_states[State::Idle] = AZStd::make_shared<SharedPreviewRendererIdleState>(this);
                m_states[State::Load] = AZStd::make_shared<SharedPreviewRendererLoadState>(this);
                m_states[State::Capture] = AZStd::make_shared<SharedPreviewRendererCaptureState>(this);
                m_states[State::Release] = AZStd::make_shared<SharedPreviewRendererReleaseState>(this);

                m_minimalFeatureProcessors =
                {
                    "AZ::Render::TransformServiceFeatureProcessor",
                    "AZ::Render::MeshFeatureProcessor",
                    "AZ::Render::SimplePointLightFeatureProcessor",
                    "AZ::Render::SimpleSpotLightFeatureProcessor",
                    "AZ::Render::PointLightFeatureProcessor",
                    // There is currently a bug where having multiple DirectionalLightFeatureProcessors active can result in shadow
                    // flickering [ATOM-13568]
                    // as well as continually rebuilding MeshDrawPackets [ATOM-13633]. Lets just disable the directional light FP for now.
                    // Possibly re-enable with [GFX TODO][ATOM-13639]
                    // "AZ::Render::DirectionalLightFeatureProcessor",
                    "AZ::Render::DiskLightFeatureProcessor",
                    "AZ::Render::CapsuleLightFeatureProcessor",
                    "AZ::Render::QuadLightFeatureProcessor",
                    "AZ::Render::DecalTextureArrayFeatureProcessor",
                    "AZ::Render::ImageBasedLightFeatureProcessor",
                    "AZ::Render::PostProcessFeatureProcessor",
                    "AZ::Render::SkyBoxFeatureProcessor"
                };
            }

            SharedPreviewRenderer::~SharedPreviewRenderer()
            {
                if (m_currentState != State::None)
                {
                    SharedPreviewRenderer::SetState(State::Release);
                }
                AzToolsFramework::Thumbnailer::ThumbnailerRendererRequestBus::MultiHandler::BusDisconnect();
                SystemTickBus::Handler::BusDisconnect();
                ThumbnailFeatureProcessorProviderBus::Handler::BusDisconnect();
            }

            void SharedPreviewRenderer::SetState(State state)
            {
                if (m_currentState != State::None)
                {
                    m_states[m_currentState]->Stop();
                }
                m_currentState = state;
                m_states[m_currentState]->Start();
            }

            State SharedPreviewRenderer::GetState() const
            {
                return m_currentState;
            }

            bool SharedPreviewRenderer::Installed() const
            {
                return true;
            }

            void SharedPreviewRenderer::OnSystemTick()
            {
                AzToolsFramework::Thumbnailer::ThumbnailerRendererRequestBus::ExecuteQueuedEvents();
            }

            const AZStd::vector<AZStd::string>& SharedPreviewRenderer::GetCustomFeatureProcessors() const
            {
                return m_minimalFeatureProcessors;
            }
            
            AZStd::shared_ptr<SharedPreviewRendererData> SharedPreviewRenderer::GetData() const
            {
                return m_data;
            }

            void SharedPreviewRenderer::RenderThumbnail(AzToolsFramework::Thumbnailer::SharedThumbnailKey thumbnailKey, int thumbnailSize)
            {
                m_data->m_thumbnailSize = thumbnailSize;
                m_data->m_thumbnailQueue.push(thumbnailKey);
                if (m_currentState == State::None)
                {
                    SetState(State::Init);
                }
            }
        } // namespace Thumbnails
    } // namespace LyIntegration
} // namespace AZ
