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

                m_steps[Step::Initialize] = AZStd::make_shared<InitializeStep>(this);
                m_steps[Step::FindThumbnailToRender] = AZStd::make_shared<FindThumbnailToRenderStep>(this);
                m_steps[Step::WaitForAssetsToLoad] = AZStd::make_shared<WaitForAssetsToLoadStep>(this);
                m_steps[Step::Capture] = AZStd::make_shared<CaptureStep>(this);
                m_steps[Step::ReleaseResources] = AZStd::make_shared<ReleaseResourcesStep>(this);
            }

            CommonThumbnailRenderer::~CommonThumbnailRenderer()
            {
                if (m_currentStep != Step::None)
                {
                    CommonThumbnailRenderer::SetStep(Step::ReleaseResources);
                }
                AzToolsFramework::Thumbnailer::ThumbnailerRendererRequestBus::MultiHandler::BusDisconnect();
                SystemTickBus::Handler::BusDisconnect();
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

            AZStd::shared_ptr<ThumbnailRendererData> CommonThumbnailRenderer::GetData() const
            {
                return m_data;
            }

            void CommonThumbnailRenderer::RenderThumbnail(AzToolsFramework::Thumbnailer::SharedThumbnailKey thumbnailKey, [[maybe_unused]] int thumbnailSize)
            {
                m_data->m_thumbnailQueue.push(thumbnailKey);
                if (m_currentStep == Step::None)
                {
                    SetStep(Step::Initialize);
                }
            }
        } // namespace Thumbnails
    } // namespace LyIntegration
} // namespace AZ
