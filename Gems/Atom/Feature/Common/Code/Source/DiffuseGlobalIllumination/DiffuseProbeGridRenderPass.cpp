/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI/RHISystemInterface.h>
#include <Atom/RPI.Public/Image/AttachmentImagePool.h>
#include <Atom/RPI.Public/Image/ImageSystemInterface.h>
#include <Atom/RPI.Public/RenderPipeline.h>
#include <Atom/RPI.Public/RPIUtils.h>
#include <DiffuseGlobalIllumination/DiffuseProbeGridRenderPass.h>
#include <DiffuseGlobalIllumination/DiffuseProbeGridFeatureProcessor.h>
#include <RayTracing/RayTracingFeatureProcessor.h>

namespace AZ
{
    namespace Render
    {
        RPI::Ptr<Render::DiffuseProbeGridRenderPass> DiffuseProbeGridRenderPass::Create(const RPI::PassDescriptor& descriptor)
        {
            return aznew DiffuseProbeGridRenderPass(descriptor);
        }

        DiffuseProbeGridRenderPass::DiffuseProbeGridRenderPass(const RPI::PassDescriptor& descriptor)
            : Base(descriptor)
        {
            // create the shader resource group
            // Note: the shader may not be available on all platforms
            AZStd::string shaderFilePath = "Shaders/DiffuseGlobalIllumination/DiffuseProbeGridRender.azshader";
            m_shader = RPI::LoadCriticalShader(shaderFilePath);
            if (m_shader == nullptr)
            {
                return;
            }

            m_srgLayout = m_shader->FindShaderResourceGroupLayout(RPI::SrgBindingSlot::Pass);
            AZ_Assert(m_srgLayout != nullptr, "[DiffuseProbeGridRenderPass '%s']: Failed to find SRG layout", GetPathName().GetCStr());

            m_shaderResourceGroup = RPI::ShaderResourceGroup::Create(m_shader->GetAsset(), m_shader->GetSupervariantIndex(), m_srgLayout->GetName());
            AZ_Assert(m_shaderResourceGroup, "[DiffuseProbeGridRenderPass '%s']: Failed to create SRG", GetPathName().GetCStr());
        }

        void DiffuseProbeGridRenderPass::FrameBeginInternal(FramePrepareParams params)
        {
            RHI::Ptr<RHI::Device> device = RHI::RHISystemInterface::Get()->GetDevice();
            RPI::Scene* scene = m_pipeline->GetScene();
            DiffuseProbeGridFeatureProcessor* diffuseProbeGridFeatureProcessor = scene->GetFeatureProcessor<DiffuseProbeGridFeatureProcessor>();

            if (!diffuseProbeGridFeatureProcessor || diffuseProbeGridFeatureProcessor->GetProbeGrids().empty())
            {
                // no diffuse probe grids
                return;
            }

            // get output attachment size
            AZ_Assert(GetInputOutputCount() > 0, "DiffuseProbeGridRenderPass: Could not find output bindings");
            RPI::PassAttachment* m_outputAttachment = GetInputOutputBinding(0).m_attachment.get();
            AZ_Assert(m_outputAttachment, "DiffuseProbeGridRenderPass: Output binding has no attachment!");
            
            RHI::Size size = m_outputAttachment->m_descriptor.m_image.m_size;
            RHI::Viewport viewport(0.f, aznumeric_cast<float>(size.m_width), 0.f, aznumeric_cast<float>(size.m_height));
            RHI::Scissor scissor(0, 0, size.m_width, size.m_height);
            
            params.m_viewportState = viewport;
            params.m_scissorState = scissor;

            Base::FrameBeginInternal(params);

            // process attachment readback for RealTime grids, if raytracing is supported on this device            
            if (device->GetFeatures().m_rayTracing)
            {
                for (auto& diffuseProbeGrid : diffuseProbeGridFeatureProcessor->GetRealTimeProbeGrids())
                {
                    diffuseProbeGrid->GetTextureReadback().FrameBegin(params);
                }
            }
        }

        void DiffuseProbeGridRenderPass::SetupFrameGraphDependencies(RHI::FrameGraphInterface frameGraph)
        {
            RPI::Scene* scene = m_pipeline->GetScene();
            DiffuseProbeGridFeatureProcessor* diffuseProbeGridFeatureProcessor = scene->GetFeatureProcessor<DiffuseProbeGridFeatureProcessor>();

            for (auto& diffuseProbeGrid : diffuseProbeGridFeatureProcessor->GetProbeGrids())
            {
                if (!ShouldRender(diffuseProbeGrid))
                {
                    continue;
                }

                // probe irradiance image
                {
                    if (diffuseProbeGrid->GetMode() == DiffuseProbeGridMode::Baked)
                    {
                        // import the irradiance image now, since it is baked and therefore was not imported during the raytracing pass
                        [[maybe_unused]] RHI::ResultCode result = frameGraph.GetAttachmentDatabase().ImportImage(diffuseProbeGrid->GetIrradianceImageAttachmentId(), diffuseProbeGrid->GetIrradianceImage());
                        AZ_Assert(result == RHI::ResultCode::Success, "Failed to import probeIrradianceImage");
                    }

                    RHI::ImageScopeAttachmentDescriptor desc;
                    desc.m_attachmentId = diffuseProbeGrid->GetIrradianceImageAttachmentId();
                    desc.m_imageViewDescriptor = diffuseProbeGrid->GetRenderData()->m_probeIrradianceImageViewDescriptor;
                    desc.m_loadStoreAction.m_loadAction = AZ::RHI::AttachmentLoadAction::Load;

                    frameGraph.UseShaderAttachment(desc, RHI::ScopeAttachmentAccess::Read);
                }

                // probe distance image
                {
                    if (diffuseProbeGrid->GetMode() == DiffuseProbeGridMode::Baked)
                    {
                        // import the distance image now, since it is baked and therefore was not imported during the raytracing pass
                        [[maybe_unused]] RHI::ResultCode result = frameGraph.GetAttachmentDatabase().ImportImage(diffuseProbeGrid->GetDistanceImageAttachmentId(), diffuseProbeGrid->GetDistanceImage());
                        AZ_Assert(result == RHI::ResultCode::Success, "Failed to import probeDistanceImage");
                    }

                    RHI::ImageScopeAttachmentDescriptor desc;
                    desc.m_attachmentId = diffuseProbeGrid->GetDistanceImageAttachmentId();
                    desc.m_imageViewDescriptor = diffuseProbeGrid->GetRenderData()->m_probeDistanceImageViewDescriptor;
                    desc.m_loadStoreAction.m_loadAction = AZ::RHI::AttachmentLoadAction::Load;

                    frameGraph.UseShaderAttachment(desc, RHI::ScopeAttachmentAccess::Read);
                }

                // probe relocation image
                {
                    if (diffuseProbeGrid->GetMode() == DiffuseProbeGridMode::Baked)
                    {
                        // import the relocation image now, since it is baked and therefore was not imported during the raytracing pass
                        [[maybe_unused]] RHI::ResultCode result = frameGraph.GetAttachmentDatabase().ImportImage(diffuseProbeGrid->GetRelocationImageAttachmentId(), diffuseProbeGrid->GetRelocationImage());
                        AZ_Assert(result == RHI::ResultCode::Success, "Failed to import probeRelocationImage");
                    }

                    RHI::ImageScopeAttachmentDescriptor desc;
                    desc.m_attachmentId = diffuseProbeGrid->GetRelocationImageAttachmentId();
                    desc.m_imageViewDescriptor = diffuseProbeGrid->GetRenderData()->m_probeRelocationImageViewDescriptor;
                    desc.m_loadStoreAction.m_loadAction = AZ::RHI::AttachmentLoadAction::Load;

                    frameGraph.UseShaderAttachment(desc, RHI::ScopeAttachmentAccess::ReadWrite);
                }

                // probe classification image
                {
                    if (diffuseProbeGrid->GetMode() == DiffuseProbeGridMode::Baked)
                    {
                        // import the classification image now, since it is baked and therefore was not imported during the raytracing pass
                        [[maybe_unused]] RHI::ResultCode result = frameGraph.GetAttachmentDatabase().ImportImage(diffuseProbeGrid->GetClassificationImageAttachmentId(), diffuseProbeGrid->GetClassificationImage());
                        AZ_Assert(result == RHI::ResultCode::Success, "Failed to import probeClassificationImage");
                    }

                    RHI::ImageScopeAttachmentDescriptor desc;
                    desc.m_attachmentId = diffuseProbeGrid->GetClassificationImageAttachmentId();
                    desc.m_imageViewDescriptor = diffuseProbeGrid->GetRenderData()->m_probeClassificationImageViewDescriptor;
                    desc.m_loadStoreAction.m_loadAction = AZ::RHI::AttachmentLoadAction::Load;

                    frameGraph.UseShaderAttachment(desc, RHI::ScopeAttachmentAccess::ReadWrite);
                }

                diffuseProbeGrid->GetTextureReadback().Update(GetName());
            }

            Base::SetupFrameGraphDependencies(frameGraph);
        }

        void DiffuseProbeGridRenderPass::CompileResources(const RHI::FrameGraphCompileContext& context)
        {
            RPI::Scene* scene = m_pipeline->GetScene();
            DiffuseProbeGridFeatureProcessor* diffuseProbeGridFeatureProcessor = scene->GetFeatureProcessor<DiffuseProbeGridFeatureProcessor>();

            for (auto& diffuseProbeGrid : diffuseProbeGridFeatureProcessor->GetProbeGrids())
            {
                if (!ShouldRender(diffuseProbeGrid))
                {
                    continue;
                }

                // the diffuse probe grid Srg must be updated in the Compile phase in order to successfully bind the ReadWrite shader inputs
                // (see ValidateSetImageView() of ShaderResourceGroupData.cpp)
                diffuseProbeGrid->UpdateRenderObjectSrg();

                diffuseProbeGrid->GetRenderObjectSrg()->Compile();
            }

            Base::CompileResources(context);
        }

        bool DiffuseProbeGridRenderPass::ShouldRender(const AZStd::shared_ptr<DiffuseProbeGrid>& diffuseProbeGrid)
        {
            RHI::Ptr<RHI::Device> device = RHI::RHISystemInterface::Get()->GetDevice();

            // check for baked mode with no valid textures
            if (diffuseProbeGrid->GetMode() == DiffuseProbeGridMode::Baked &&
                !diffuseProbeGrid->HasValidBakedTextures())
            {
                return false;
            }

            // check for RealTime mode without ray tracing
            if (diffuseProbeGrid->GetMode() == DiffuseProbeGridMode::RealTime &&
                !device->GetFeatures().m_rayTracing)
            {
                return false;
            }

            // check if culled out
            if (!diffuseProbeGrid->GetIsVisible())
            {
                return false;
            }

            // DiffuseProbeGrid should be rendered
            return true;
        }
    } // namespace Render
} // namespace AZ
