/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Math/MathUtils.h>

#include <PostProcess/PostProcessFeatureProcessor.h>
#include <PostProcess/DepthOfField/DepthOfFieldSettings.h>
#include <PostProcessing/NewDepthOfFieldPasses.h>

#include <Atom/RPI.Public/RenderPipeline.h>
#include <Atom/RPI.Public/Scene.h>
#include <Atom/RPI.Public/View.h>

namespace AZ
{
    namespace Render
    {

        // Must match the struct in NewDepthOfFieldCommon.azsli
        struct NewDepthOfFieldConstants
        {
            static constexpr uint32_t numberOfLoops = 3;
            static constexpr float loopCounts[] = { 8.0f, 16.0f, 24.0f };

            AZStd::array<float[4], 60> m_samplePositions;   // XY are sample positions (normalized so max lenght is 1)
                                                            // Z is the length of XY (0 - 1)
                                                            // W is unused
        };


        // --- Depth of Field Parent Pass ---

        RPI::Ptr<NewDepthOfFieldParentPass> NewDepthOfFieldParentPass::Create(const RPI::PassDescriptor& descriptor)
        {
            RPI::Ptr<NewDepthOfFieldParentPass> pass = aznew NewDepthOfFieldParentPass(descriptor);
            return AZStd::move(pass);
        }

        NewDepthOfFieldParentPass::NewDepthOfFieldParentPass(const RPI::PassDescriptor& descriptor)
            : RPI::ParentPass(descriptor)
        { }

        bool NewDepthOfFieldParentPass::IsEnabled() const
        {
            if (!ParentPass::IsEnabled())
            {
                return false;
            }
            RPI::Scene* scene = GetScene();
            if (!scene)
            {
                return false;
            }
            PostProcessFeatureProcessor* fp = scene->GetFeatureProcessor<PostProcessFeatureProcessor>();
            AZ::RPI::ViewPtr view = GetRenderPipeline()->GetDefaultView();
            if (!fp)
            {
                return false;
            }
            PostProcessSettings* postProcessSettings = fp->GetLevelSettingsFromView(view);
            if (!postProcessSettings)
            {
                return false;
            }
            DepthOfFieldSettings* dofSettings = postProcessSettings->GetDepthOfFieldSettings();
            return (dofSettings != nullptr) && dofSettings->GetEnabled();
        }

        void NewDepthOfFieldParentPass::FrameBeginInternal(FramePrepareParams params)
        {
            RPI::Scene* scene = GetScene();
            PostProcessFeatureProcessor* fp = scene->GetFeatureProcessor<PostProcessFeatureProcessor>();
            AZ::RPI::ViewPtr view = GetRenderPipeline()->GetDefaultView();
            if (fp)
            {
                PostProcessSettings* postProcessSettings = fp->GetLevelSettingsFromView(view);
                if (postProcessSettings)
                {
                    DepthOfFieldSettings* dofSettings = postProcessSettings->GetDepthOfFieldSettings();
                    if (dofSettings)
                    {
                        dofSettings->SetValuesToViewSrg(view->GetShaderResourceGroup());
                    }
                }
            }

            ParentPass::FrameBeginInternal(params);
        }

        // --- Tile Reduce Pass ---

        RPI::Ptr<NewDepthOfFieldTileReducePass> NewDepthOfFieldTileReducePass::Create(const RPI::PassDescriptor& descriptor)
        {
            RPI::Ptr<NewDepthOfFieldTileReducePass> pass = aznew NewDepthOfFieldTileReducePass(descriptor);
            return AZStd::move(pass);
        }

        NewDepthOfFieldTileReducePass::NewDepthOfFieldTileReducePass(const RPI::PassDescriptor& descriptor)
            : RPI::ComputePass(descriptor)
        {
            // Though this is a fullscreen pass, the algorithm used makes each thread output 3 blurred pixels, so
            // it's not a 1-to-1 ratio and requires custom calculation of target thread counts
            m_isFullscreenPass = false;
        }

        void NewDepthOfFieldTileReducePass::FrameBeginInternal(FramePrepareParams params)
        {
            AZ_Assert(GetOutputCount() > 0, "NewDepthOfFieldTileReducePass: No output bindings!");
            RPI::PassAttachment* outputAttachment = GetOutputBinding(0).m_attachment.get();

            AZ_Assert(outputAttachment != nullptr, "NewDepthOfFieldTileReducePass: Output binding has no attachment!");
            RHI::Size outputSize = outputAttachment->m_descriptor.m_image.m_size;

            // The algorithm outputs the min/max CoC values from a 16x16 region using 8x8 threads
            u32 targetThreadCountX = outputSize.m_width * 8;
            u32 targetThreadCountY = outputSize.m_height * 8;
            SetTargetThreadCounts(targetThreadCountX, targetThreadCountY, 1);

            RPI::ComputePass::FrameBeginInternal(params);
        }



        // --- Filter Pass ---

        RPI::Ptr<NewDepthOfFieldFilterPass> NewDepthOfFieldFilterPass::Create(const RPI::PassDescriptor& descriptor)
        {
            RPI::Ptr<NewDepthOfFieldFilterPass> pass = aznew NewDepthOfFieldFilterPass(descriptor);
            return AZStd::move(pass);
        }

        NewDepthOfFieldFilterPass::NewDepthOfFieldFilterPass(const RPI::PassDescriptor& descriptor)
            : RPI::FullscreenTrianglePass(descriptor)
        { }

        void NewDepthOfFieldFilterPass::FrameBeginInternal(FramePrepareParams params)
        {
            NewDepthOfFieldConstants dofConstants;

            uint32_t sampleIndex = 0;

            for (uint32_t loop = 0; loop < NewDepthOfFieldConstants::numberOfLoops; ++loop)
            {
                float radius = (loop + 1.0f) / float(NewDepthOfFieldConstants::numberOfLoops);
                float loopCount = NewDepthOfFieldConstants::loopCounts[loop];

                float angleOffset = 0;

                // Every other loop slightly rotate sample ring so they don't line up
                if (loop & 1)
                {
                    angleOffset = Constants::TwoPi * 0.5f / loopCount;
                }

                for (float i = 0.0f; i < loopCount; ++i)
                {
                    float angle = Constants::TwoPi * i / loopCount;
                    Vector2 pos = Vector2::CreateFromAngle(angle);
                    pos = pos * radius;

                    dofConstants.m_samplePositions[sampleIndex][0] = pos.GetX();
                    dofConstants.m_samplePositions[sampleIndex][1] = pos.GetY();
                    dofConstants.m_samplePositions[sampleIndex][2] = radius;
                    dofConstants.m_samplePositions[sampleIndex][3] = 0.0f;
                    ++sampleIndex;
                }

            }

            m_shaderResourceGroup->SetConstant(m_constantsIndex, dofConstants);

            // TODO HERE
            RPI::FullscreenTrianglePass::FrameBeginInternal(params);
        }



        // --- Composite Pass ---

        RPI::Ptr<NewDepthOfFieldCompositePass> NewDepthOfFieldCompositePass::Create(const RPI::PassDescriptor& descriptor)
        {
            RPI::Ptr<NewDepthOfFieldCompositePass> pass = aznew NewDepthOfFieldCompositePass(descriptor);
            return AZStd::move(pass);
        }

        NewDepthOfFieldCompositePass::NewDepthOfFieldCompositePass(const RPI::PassDescriptor& descriptor)
            : RPI::ComputePass(descriptor)
        { }

        void NewDepthOfFieldCompositePass::FrameBeginInternal(FramePrepareParams params)
        {
            // TODO HERE
            RPI::ComputePass::FrameBeginInternal(params);
        }



    }   // namespace Render
}   // namespace AZ
