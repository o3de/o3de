/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <PostProcessing/FilmGrainPass.h>
#include <PostProcess/PostProcessFeatureProcessor.h>
#include <Atom/RPI.Public/RenderPipeline.h>
#include <Atom/RPI.Public/Scene.h>

namespace AZ
{
    namespace Render
    {
        RPI::Ptr<FilmGrainPass> FilmGrainPass::Create(const RPI::PassDescriptor& descriptor)
        {
            RPI::Ptr<FilmGrainPass> pass = aznew FilmGrainPass(descriptor);
            return AZStd::move(pass);
        }

        FilmGrainPass::FilmGrainPass(const RPI::PassDescriptor& descriptor)
            : RPI::ComputePass(descriptor)
        {
        }

        bool FilmGrainPass::IsEnabled() const
        {
            if (!ComputePass::IsEnabled())
            {
                return false;
            }
            const RPI::Scene* scene = GetScene();
            if (!scene)
            {
                return false;
            }
            PostProcessFeatureProcessor* fp = scene->GetFeatureProcessor<PostProcessFeatureProcessor>();
            const RPI::ViewPtr view = GetRenderPipeline()->GetDefaultView();
            if (!fp)
            {
                return false;
            }
            PostProcessSettings* postProcessSettings = fp->GetLevelSettingsFromView(view);
            if (!postProcessSettings)
            {
                return false;
            }
            const FilmGrainSettings* filmGrainSettings = postProcessSettings->GetFilmGrainSettings();
            if (!filmGrainSettings)
            {
                return false;
            }
            return filmGrainSettings->GetEnabled();
        }

        void FilmGrainPass::FrameBeginInternal(FramePrepareParams params)
        {
            // Must match the struct in FilmGrain.azsl
            struct Constants
            {
                AZStd::array<u32, 2> m_outputSize;
                AZStd::array<u32, 2> m_grainTextureSize;
                float m_intensity = FilmGrain::DefaultIntensity;
                float m_luminanceDampening = FilmGrain::DefaultLuminanceDampening;
                float m_tilingScale = FilmGrain::DefaultTilingScale;
                float m_pad;
            } constants{};

            RPI::Scene* scene = GetScene();
            PostProcessFeatureProcessor* fp = scene->GetFeatureProcessor<PostProcessFeatureProcessor>();
            if (fp)
            {
                RPI::ViewPtr view = scene->GetDefaultRenderPipeline()->GetDefaultView();
                PostProcessSettings* postProcessSettings = fp->GetLevelSettingsFromView(view);
                if (postProcessSettings)
                {
                    FilmGrainSettings* filmGrainSettings = postProcessSettings->GetFilmGrainSettings();
                    if (filmGrainSettings)
                    {
                        constants.m_intensity = filmGrainSettings->GetIntensity();
                        constants.m_luminanceDampening = filmGrainSettings->GetLuminanceDampening();
                        constants.m_tilingScale = filmGrainSettings->GetTilingScale();

                        AZStd::string settingsGrainPath = filmGrainSettings->GetGrainPath();
                        if (m_currentGrainPath != settingsGrainPath)
                        {
                            m_currentGrainPath = settingsGrainPath;
                            m_grainImage = filmGrainSettings->LoadStreamingImage(settingsGrainPath.c_str(), "FilmGrain");
                        }
                    }
                }
            }

            m_shaderResourceGroup->SetImage(m_grainIndex, m_grainImage);

            RHI::Size grainTextureSize = m_grainImage->GetDescriptor().m_size;

            constants.m_grainTextureSize[0] = grainTextureSize.m_width;
            constants.m_grainTextureSize[1] = grainTextureSize.m_height;
            
            AZ_Assert(GetOutputCount() > 0, "FilmGrainPass: No output bindings!");
            RPI::PassAttachment* outputAttachment = GetOutputBinding(0).GetAttachment().get();

            AZ_Assert(outputAttachment != nullptr, "FilmGrainPass: Output binding has no attachment!");
            RHI::Size size = outputAttachment->m_descriptor.m_image.m_size;

            constants.m_outputSize[0] = size.m_width;
            constants.m_outputSize[1] = size.m_height;

            m_shaderResourceGroup->SetConstant(m_constantsIndex, constants);

            RPI::ComputePass::FrameBeginInternal(params);
        }
    } // namespace Render
} // namespace AZ
