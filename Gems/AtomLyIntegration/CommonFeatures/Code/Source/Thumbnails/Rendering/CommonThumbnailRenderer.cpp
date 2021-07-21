/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RPI.Reflect/Material/MaterialAsset.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserBus.h>
#include <Thumbnails/Rendering/CommonThumbnailRenderer.h>
#include <Thumbnails/Rendering/ThumbnailRendererData.h>
#include <Thumbnails/Rendering/ThumbnailRendererSteps/CaptureStep.h>
#include <Thumbnails/Rendering/ThumbnailRendererSteps/FindThumbnailToRenderStep.h>
#include <Thumbnails/Rendering/ThumbnailRendererSteps/InitializeStep.h>
#include <Thumbnails/Rendering/ThumbnailRendererSteps/ReleaseResourcesStep.h>
#include <Thumbnails/Rendering/ThumbnailRendererSteps/WaitForAssetsToLoadStep.h>

namespace AZ
{
    namespace LyIntegration
    {
        namespace Thumbnails
        {
            CommonThumbnailRenderer::CommonThumbnailRenderer()
                : m_data(new ThumbnailRendererData)
            {
                // CommonThumbnailRenderer supports both models and materials, but we connect on materialAssetType
                // since MaterialOrModelThumbnail dispatches event on materialAssetType address too
                AzToolsFramework::Thumbnailer::ThumbnailerRendererRequestBus::MultiHandler::BusConnect(RPI::MaterialAsset::RTTI_Type());
                AzToolsFramework::Thumbnailer::ThumbnailerRendererRequestBus::MultiHandler::BusConnect(RPI::ModelAsset::RTTI_Type());
                SystemTickBus::Handler::BusConnect();
                ThumbnailFeatureProcessorProviderBus::Handler::BusConnect();

                m_steps[Step::Initialize] = AZStd::make_shared<InitializeStep>(this);
                m_steps[Step::FindThumbnailToRender] = AZStd::make_shared<FindThumbnailToRenderStep>(this);
                m_steps[Step::WaitForAssetsToLoad] = AZStd::make_shared<WaitForAssetsToLoadStep>(this);
                m_steps[Step::Capture] = AZStd::make_shared<CaptureStep>(this);
                m_steps[Step::ReleaseResources] = AZStd::make_shared<ReleaseResourcesStep>(this);

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

            CommonThumbnailRenderer::~CommonThumbnailRenderer()
            {
                if (m_currentStep != Step::None)
                {
                    CommonThumbnailRenderer::SetStep(Step::ReleaseResources);
                }
                AzToolsFramework::Thumbnailer::ThumbnailerRendererRequestBus::MultiHandler::BusDisconnect();
                SystemTickBus::Handler::BusDisconnect();
                ThumbnailFeatureProcessorProviderBus::Handler::BusDisconnect();
            }

            void CommonThumbnailRenderer::SetStep(Step step)
            {
                if (m_currentStep != Step::None)
                {
                    m_steps[m_currentStep]->Stop();
                }
                m_currentStep = step;
                m_steps[m_currentStep]->Start();
            }

            Step CommonThumbnailRenderer::GetStep() const
            {
                return m_currentStep;
            }

            bool CommonThumbnailRenderer::Installed() const
            {
                return true;
            }

            void CommonThumbnailRenderer::OnSystemTick()
            {
                AzToolsFramework::Thumbnailer::ThumbnailerRendererRequestBus::ExecuteQueuedEvents();
            }

            const AZStd::vector<AZStd::string>& CommonThumbnailRenderer::GetCustomFeatureProcessors() const
            {
                return m_minimalFeatureProcessors;
            }
            
            AZStd::shared_ptr<ThumbnailRendererData> CommonThumbnailRenderer::GetData() const
            {
                return m_data;
            }

            void CommonThumbnailRenderer::RenderThumbnail(AzToolsFramework::Thumbnailer::SharedThumbnailKey thumbnailKey, int thumbnailSize)
            {
                m_data->m_thumbnailSize = thumbnailSize;
                m_data->m_thumbnailQueue.push(thumbnailKey);
                if (m_currentStep == Step::None)
                {
                    SetStep(Step::Initialize);
                }
            }
        } // namespace Thumbnails
    } // namespace LyIntegration
} // namespace AZ
