/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RPI.Public/RenderPipeline.h>
#include <Atom/RPI.Public/Scene.h>
#include <Atom/RPI.Public/View.h>
#include <Atom/RPI.Public/Pass/PassSystemInterface.h>

#include <PostProcess/PostProcessFeatureProcessor.h>
#include <PostProcess/DepthOfField/DepthOfFieldSettings.h>
#include <PostProcessing/DepthOfFieldReadBackFocusDepthPass.h>

namespace AZ
{
    namespace Render
    {
        uint32_t DepthOfFieldReadBackFocusDepthPass::s_bufferInstance = 0;

        RPI::Ptr<DepthOfFieldReadBackFocusDepthPass> DepthOfFieldReadBackFocusDepthPass::Create(const RPI::PassDescriptor& descriptor)
        {
            RPI::Ptr<DepthOfFieldReadBackFocusDepthPass> pass = aznew DepthOfFieldReadBackFocusDepthPass(descriptor);
            return pass;
        }

        DepthOfFieldReadBackFocusDepthPass::DepthOfFieldReadBackFocusDepthPass(const RPI::PassDescriptor& descriptor)
            : ParentPass(descriptor)
        {
            // Create buffer for read back focus depth. We append static counter to avoid name conflicts.
            RPI::CommonBufferDescriptor desc;
            desc.m_bufferName = "DepthOfFieldReadBackAutoFocusDepthBuffer";
            desc.m_poolType = RPI::CommonBufferPoolType::ReadWrite;
            desc.m_byteCount = sizeof(float);
            desc.m_elementSize = aznumeric_cast<uint32_t>(desc.m_byteCount);
            desc.m_bufferData = nullptr;
            desc.m_elementFormat = RHI::Format::R32_FLOAT;
            m_buffer = RPI::BufferSystemInterface::Get()->CreateBufferFromCommonPool(desc);
        }

        DepthOfFieldReadBackFocusDepthPass::~DepthOfFieldReadBackFocusDepthPass()
        {
            m_buffer = nullptr;
            m_getDepthPass = nullptr;
            m_readbackPass = nullptr;
        }

        float DepthOfFieldReadBackFocusDepthPass::GetFocusDepth()
        {
            return m_readbackPass->GetFocusDepth();
        }

        float DepthOfFieldReadBackFocusDepthPass::GetNormalizedFocusDistanceForAutoFocus() const
        {
            return m_normalizedFocusDistanceForAutoFocus;
        }

        void DepthOfFieldReadBackFocusDepthPass::SetScreenPosition(const AZ::Vector2& screenPosition)
        {
            if(m_getDepthPass)
            {
                m_getDepthPass->SetScreenPosition(screenPosition);
            }
        }

        void DepthOfFieldReadBackFocusDepthPass::CreateChildPassesInternal()
        {
            RPI::PassSystemInterface* passSystem = RPI::PassSystemInterface::Get();

            // Create read back pass
            m_readbackPass = passSystem->CreatePass<DepthOfFieldCopyFocusDepthToCpuPass>(AZ::Name("DepthOfFieldReadBackPass"));
            AZ_Assert(m_readbackPass, "DepthOfFieldReadBackFocusDepthPass : read back pass is invalid");

            AddChild(m_readbackPass);

            // Find GetDepth pass on template
            auto pass = FindChildPass(Name("DepthOfFieldWriteFocusDepthFromGpu"));
            m_getDepthPass = static_cast<DepthOfFieldWriteFocusDepthFromGpuPass*>(pass.get());

            m_getDepthPass->SetBufferRef(m_buffer);
            m_readbackPass->SetBufferRef(m_buffer);
        }

        void DepthOfFieldReadBackFocusDepthPass::FrameBeginInternal(FramePrepareParams params)
        {
            RPI::Scene* scene = GetScene();
            PostProcessFeatureProcessor* fp = scene->GetFeatureProcessor<PostProcessFeatureProcessor>();
            AZ::RPI::ViewPtr view = GetRenderPipeline()->GetFirstView(GetPipelineViewTag());
            if (fp)
            {
                PostProcessSettings* postProcessSettings = fp->GetLevelSettingsFromView(view);
                if (postProcessSettings)
                {
                    DepthOfFieldSettings* dofSettings = postProcessSettings->GetDepthOfFieldSettings();
                    if (dofSettings)
                    {
                        SetScreenPosition(dofSettings->m_autoFocusScreenPosition);
                        UpdateAutoFocus(*dofSettings);
                    }
                }
            }
            RPI::ParentPass::FrameBeginInternal(params);
        }

        void DepthOfFieldReadBackFocusDepthPass::UpdateAutoFocus(DepthOfFieldSettings& dofSettings)
        {
            float viewNear = dofSettings.m_viewNear;
            float viewFar = dofSettings.m_viewFar;

            if (dofSettings.m_enableAutoFocus)
            {
                float deltaTime = dofSettings.m_deltaTime;
                float depthForAutoFocus = GetFocusDepth();

                // Convert depth to linear.
                // This value is in the range [0.0, 1.0], where 0.0 is view near and 1.0 is view far.
                float div1 = viewNear * depthForAutoFocus - viewFar * depthForAutoFocus + viewFar;
                float viewZ = div1 > 0.001f ? viewFar * viewNear / div1 : viewFar;
                float div2 = viewFar - viewNear;
                float targetLinearDepth = div2 > 0.001f ? (viewZ - viewNear) / div2 : 0.0f;
                targetLinearDepth = GetClamp(targetLinearDepth, 0.0f, 1.0f);

                // Response distance when m_autoFocusSensitivity is 0.0.
                // As a standard, the distance from near to far is 1.0.
                constexpr float FocusStartingDistanceMax = 0.5f;

                // The greater m_autoFocusSensitivity, the easier the response.
                // If m_autoFocusSensitivity is 1.0, always re-focus.
                float focusStartingDistance = (1.0f - dofSettings.m_autoFocusSensitivity) * FocusStartingDistanceMax;
                float targetDistance = targetLinearDepth - m_normalizedFocusDistanceForAutoFocus;
                if (!m_isMovingFocus && focusStartingDistance < GetAbs(targetDistance))
                {
                    m_isMovingFocus = true;
                    m_delayTimer = 0.0f;
                }

                if (m_isMovingFocus)
                {
                    if (m_delayTimer < dofSettings.m_autoFocusDelay)
                    {
                        m_delayTimer += deltaTime;
                    }
                    else
                    {
                        // Start focus after m_delayTimer[s] exceeds m_autoFocusDelay[s].
                        float speed = dofSettings.m_autoFocusSpeed * deltaTime;
                        if (GetAbs(targetDistance) < speed)
                        {
                            // Arrive at the focus target.
                            m_normalizedFocusDistanceForAutoFocus = targetLinearDepth;
                            m_isMovingFocus = false;
                        }
                        else
                        {
                            m_normalizedFocusDistanceForAutoFocus += speed * GetSign(targetDistance);
                        }
                    }
                }
            }
            else
            {
                m_isMovingFocus = false;

                // Always update distance for auto focus.
                // Process to prevent a sudden change of focus when changing the auto focus from disabled to enabled.
                // convert from to [Near, Far] to [0, 1]
                float focusDistance = GetClamp(dofSettings.m_focusDistance, viewNear, viewFar);
                m_normalizedFocusDistanceForAutoFocus = (focusDistance - viewNear) / (viewFar - viewNear);
            }
        }

    }   // namespace Render
}   // namespace AZ
