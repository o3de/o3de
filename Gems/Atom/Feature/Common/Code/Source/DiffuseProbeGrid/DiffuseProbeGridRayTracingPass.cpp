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

#include <DiffuseProbeGrid/DiffuseProbeGridRayTracingPass.h>
#include <Atom/RHI/CommandList.h>
#include <Atom/RHI/DispatchRaysItem.h>
#include <Atom/RHI/Factory.h>
#include <Atom/RHI/FrameScheduler.h>
#include <Atom/RHI/RHISystemInterface.h>
#include <Atom/RHI/ScopeProducerFunction.h>
#include <Atom/RPI.Public/Buffer/BufferSystemInterface.h>
#include <Atom/RPI.Public/Buffer/Buffer.h>
#include <Atom/RPI.Public/RenderPipeline.h>
#include <Atom/RPI.Public/Scene.h>
#include <Atom/RPI.Public/Pass/PassUtils.h>
#include <Atom/RPI.Public/RPIUtils.h>
#include <Atom/RPI.Public/View.h>
#include <Atom/Feature/TransformService/TransformServiceFeatureProcessor.h>
#include <DiffuseProbeGrid/DiffuseProbeGridFeatureProcessor.h>
#include <RayTracing/RayTracingFeatureProcessor.h>

namespace AZ
{
    namespace Render
    {
        RPI::Ptr<DiffuseProbeGridRayTracingPass> DiffuseProbeGridRayTracingPass::Create(const RPI::PassDescriptor& descriptor)
        {
            RPI::Ptr<DiffuseProbeGridRayTracingPass> pass = aznew DiffuseProbeGridRayTracingPass(descriptor);
            return AZStd::move(pass);
        }

        DiffuseProbeGridRayTracingPass::DiffuseProbeGridRayTracingPass(const RPI::PassDescriptor& descriptor)
            : RPI::RenderPass(descriptor)
        {
            RHI::Ptr<RHI::Device> device = RHI::RHISystemInterface::Get()->GetDevice();
            if (device->GetFeatures().m_rayTracing == false)
            {
                // raytracing is not supported on this platform
                SetEnabled(false);
            }
        }

        DiffuseProbeGridRayTracingPass::~DiffuseProbeGridRayTracingPass()
        {
            delete m_rayTracingScopeProducerShaderTable;
        }

        void DiffuseProbeGridRayTracingPass::CreateRayTracingPipelineState()
        {
            RHI::Ptr<RHI::Device> device = RHI::RHISystemInterface::Get()->GetDevice();

            // load the ray tracing shader
            // Note: the shader may not be available on all platforms
            AZStd::string shaderFilePath = "Shaders/DiffuseGlobalIllumination/DiffuseProbeGridRayTracing.azshader";
            m_rayTracingShader = RPI::LoadShader(shaderFilePath);
            if (m_rayTracingShader == nullptr)
            {
                return;
            }

            auto shaderVariant = m_rayTracingShader->GetVariant(RPI::ShaderAsset::RootShaderVariantStableId);
            RHI::PipelineStateDescriptorForRayTracing rayGenerationShaderDescriptor;
            shaderVariant.ConfigurePipelineState(rayGenerationShaderDescriptor);

            // closest hit shader
            AZStd::string closestHitShaderFilePath = "Shaders/DiffuseGlobalIllumination/DiffuseProbeGridRayTracingClosestHit.azshader";
            m_closestHitShader = RPI::LoadShader(closestHitShaderFilePath);

            auto closestHitShaderVariant = m_closestHitShader->GetVariant(RPI::ShaderAsset::RootShaderVariantStableId);
            RHI::PipelineStateDescriptorForRayTracing closestHitShaderDescriptor;
            closestHitShaderVariant.ConfigurePipelineState(closestHitShaderDescriptor);

            // miss shader
            AZStd::string missShaderFilePath = "Shaders/DiffuseGlobalIllumination/DiffuseProbeGridRayTracingMiss.azshader";
            m_missShader = RPI::LoadShader(missShaderFilePath);

            auto missShaderVariant = m_missShader->GetVariant(RPI::ShaderAsset::RootShaderVariantStableId);
            RHI::PipelineStateDescriptorForRayTracing missShaderDescriptor;
            missShaderVariant.ConfigurePipelineState(missShaderDescriptor);

            // global pipeline state and Srg
            m_globalPipelineState = m_rayTracingShader->AcquirePipelineState(rayGenerationShaderDescriptor);
            AZ_Assert(m_globalPipelineState, "Failed to acquire ray tracing global pipeline state");

            m_globalSrgAsset = m_rayTracingShader->FindShaderResourceGroupAsset(Name{ "RayTracingGlobalSrg" });
            AZ_Error("ReflectionProbeFeatureProcessor", m_globalSrgAsset.GetId().IsValid(), "Failed to find RayTracingGlobalSrg asset for shader [%s]", shaderFilePath.c_str());
            AZ_Error("ReflectionProbeFeatureProcessor", m_globalSrgAsset.IsReady(), "RayTracingGlobalSrg asset is not loaded for shader [%s]", shaderFilePath.c_str());

            // build the ray tracing pipeline state descriptor
            RHI::RayTracingPipelineStateDescriptor descriptor;
            descriptor.Build()
                ->PipelineState(m_globalPipelineState.get())
                ->MaxPayloadSize(64)
                ->MaxAttributeSize(32)
                ->MaxRecursionDepth(2)
                ->ShaderLibrary(rayGenerationShaderDescriptor)
                    ->RayGenerationShaderName(AZ::Name("RayGen"))
                ->ShaderLibrary(missShaderDescriptor)
                    ->MissShaderName(AZ::Name("Miss"))
                ->ShaderLibrary(closestHitShaderDescriptor)
                    ->ClosestHitShaderName(AZ::Name("ClosestHit"))
                ->HitGroup(AZ::Name("HitGroup"))
                    ->ClosestHitShaderName(AZ::Name("ClosestHit"));

            // create the ray tracing pipeline state object
            m_rayTracingPipelineState = RHI::Factory::Get().CreateRayTracingPipelineState();
            m_rayTracingPipelineState->Init(*device.get(), &descriptor);
        }

        void DiffuseProbeGridRayTracingPass::FrameBeginInternal(FramePrepareParams params)
        {
            if (!m_initialized)
            {
                CreateRayTracingPipelineState();
                CreateShaderTableScope();
                m_initialized = true;
            }

            if (!m_rayTracingShaderTable)
            {
                m_rayTracingShaderTable = RHI::Factory::Get().CreateRayTracingShaderTable();
            }

            RPI::Scene* scene = m_pipeline->GetScene();
            DiffuseProbeGridFeatureProcessor* diffuseProbeGridFeatureProcessor = scene->GetFeatureProcessor<DiffuseProbeGridFeatureProcessor>();
            if (!diffuseProbeGridFeatureProcessor || diffuseProbeGridFeatureProcessor->GetProbeGrids().empty())
            {
                // no diffuse probe grids
                return;
            }

            RayTracingFeatureProcessor* rayTracingFeatureProcessor = scene->GetFeatureProcessor<RayTracingFeatureProcessor>();
            uint32_t rayTracingRevision = rayTracingFeatureProcessor->GetRevision();
            if (m_rayTracingRevision != rayTracingRevision)
            {
                // scene changed, need to rebuild the shader table
                m_rayTracingRevision = rayTracingRevision;

                // [GFX TODO][ATOM-13575] Move the RHI::RayTracingShaderTable build into the RHI frame and remove this scope
                params.m_frameGraphBuilder->ImportScopeProducer(*m_rayTracingScopeProducerShaderTable);
            }

            RenderPass::FrameBeginInternal(params);
        }

        void DiffuseProbeGridRayTracingPass::CreateShaderTableScope()
        {
            struct ScopeData { };
            const auto prepareFunction = [this]([[maybe_unused]] RHI::FrameGraphInterface& scopeBuilder, [[maybe_unused]] ScopeData& scopeData) {};

            const auto compileFunction = [this]([[maybe_unused]] const RHI::FrameGraphCompileContext& context, [[maybe_unused]] const ScopeData& scopeData)
            {
                // create a SRG array entry for every ray tracing mesh in the scene
                RPI::Scene* scene = m_pipeline->GetScene();
                TransformServiceFeatureProcessor* transformFeatureProcessor = scene->GetFeatureProcessor<TransformServiceFeatureProcessor>();
                RayTracingFeatureProcessor* rayTracingFeatureProcessor = scene->GetFeatureProcessor<RayTracingFeatureProcessor>();

                m_meshVertexPositionBuffer.clear();
                m_meshVertexNormalBuffer.clear();
                m_meshIndexBuffer.clear();

                const RayTracingFeatureProcessor::MeshMap& rayTracingMeshes = rayTracingFeatureProcessor->GetMeshes();
                m_meshCount = 0;
                for (const auto& mesh : rayTracingMeshes)
                {
                    const RayTracingFeatureProcessor::SubMeshVector& subMeshes = mesh.second.m_subMeshes;
                    for (const auto& subMesh : subMeshes)
                    {
                        // [GFX TODO][ATOM-14780] SRG support for unbounded arrays
                        // we are limited to 512 meshes until unbounded array support is implemented
                        if (m_meshCount == 512)
                        {
                            AZ_Warning("DiffuseProbeGridRayTracingPass", false, "Maximum number of meshes reached");
                            break;
                        }

                        // set irradiance color and worldInverseTranspose constants
                        Vector4 color(subMesh.m_irradianceColor.GetR(), subMesh.m_irradianceColor.GetG(), subMesh.m_irradianceColor.GetB(), 1.0f);

                        AZ::Transform meshTransform = transformFeatureProcessor->GetTransformForId(TransformServiceFeatureProcessorInterface::ObjectId(mesh.first));
                        AZ::Transform noScaleTransform = meshTransform;
                        noScaleTransform.ExtractScale();
                        AZ::Matrix3x3 rotationMatrix = Matrix3x3::CreateFromTransform(noScaleTransform);
                        rotationMatrix = rotationMatrix.GetInverseFull().GetTranspose();

                        m_closestHitData[m_meshCount].m_materialColor = color;
                        m_closestHitData[m_meshCount].m_worldInvTranspose = rotationMatrix;
                        m_closestHitData[m_meshCount].m_positionOffset = subMesh.m_positionVertexBufferView.GetByteOffset();
                        m_closestHitData[m_meshCount].m_normalOffset = subMesh.m_normalVertexBufferView.GetByteOffset();
                        m_closestHitData[m_meshCount].m_indexOffset = subMesh.m_indexBufferView.GetByteOffset();

                        // set vertex and index streams
                        m_meshVertexPositionBuffer.push_back(subMesh.m_positionShaderBufferView.get());
                        m_meshVertexNormalBuffer.push_back(subMesh.m_normalShaderBufferView.get());
                        m_meshIndexBuffer.push_back(subMesh.m_indexShaderBufferView.get());

                        m_meshCount++;
                    }
                }
            };

            const auto executeFunction = [this]([[maybe_unused]] const RHI::FrameGraphExecuteContext& context, [[maybe_unused]] const ScopeData& scopeData)
            {
                RHI::Ptr<RHI::Device> device = RHI::RHISystemInterface::Get()->GetDevice();
                RayTracingFeatureProcessor* rayTracingFeatureProcessor = m_pipeline->GetScene()->GetFeatureProcessor<RayTracingFeatureProcessor>();
                RHI::RayTracingBufferPools& rayTracingBufferPools = rayTracingFeatureProcessor->GetBufferPools();

                if (m_meshCount == 0)
                {
                    m_rayTracingShaderTable = nullptr;
                    return;
                }

                // build the ray tracing shader table descriptor
                RHI::RayTracingShaderTableDescriptor descriptor;
                RHI::RayTracingShaderTableDescriptor* descriptorBuild = descriptor.Build(AZ::Name("RayTracingShaderTable"), m_rayTracingPipelineState)
                    ->RayGenerationRecord(AZ::Name("RayGen"))
                    ->MissRecord(AZ::Name("Miss"));

                // add a hit group for each mesh to the shader table
                for (uint32_t i = 0; i < m_meshCount; ++i)
                {
                    descriptorBuild->HitGroupRecord(AZ::Name("HitGroup"));
                }

                m_rayTracingShaderTable->Init(*device.get(), &descriptor, rayTracingBufferPools);
            };

            AZStd::string uuidString = AZ::Uuid::CreateRandom().ToString<AZStd::string>();
            AZStd::string scopeName = AZStd::string::format("DiffuseProbeRayTracingBuildShaderTable_%s", uuidString.c_str());

            m_rayTracingScopeProducerShaderTable =
                aznew RHI::ScopeProducerFunction<
                ScopeData,
                decltype(prepareFunction),
                decltype(compileFunction),
                decltype(executeFunction)>(
                    RHI::ScopeId{ scopeName },
                    ScopeData{ },
                    prepareFunction,
                    compileFunction,
                    executeFunction);
        }

        void DiffuseProbeGridRayTracingPass::SetupFrameGraphDependencies(RHI::FrameGraphInterface frameGraph)
        {
            RenderPass::SetupFrameGraphDependencies(frameGraph);

            RPI::Scene* scene = m_pipeline->GetScene();
            DiffuseProbeGridFeatureProcessor* diffuseProbeGridFeatureProcessor = scene->GetFeatureProcessor<DiffuseProbeGridFeatureProcessor>();
            RayTracingFeatureProcessor* rayTracingFeatureProcessor = scene->GetFeatureProcessor<RayTracingFeatureProcessor>();

            frameGraph.SetEstimatedItemCount(aznumeric_cast<uint32_t>(diffuseProbeGridFeatureProcessor->GetProbeGrids().size()));
            frameGraph.ExecuteAfter(m_rayTracingScopeProducerShaderTable->GetScopeId());

            for (const auto& diffuseProbeGrid : diffuseProbeGridFeatureProcessor->GetProbeGrids())
            {
                // TLAS
                {
                    AZ::RHI::AttachmentId tlasAttachmentId = rayTracingFeatureProcessor->GetTlasAttachmentId();
                    if (frameGraph.GetAttachmentDatabase().IsAttachmentValid(tlasAttachmentId))
                    {
                        uint32_t tlasBufferByteCount = aznumeric_cast<uint32_t>(rayTracingFeatureProcessor->GetTlas()->GetTlasBuffer()->GetDescriptor().m_byteCount);
                        RHI::BufferViewDescriptor tlasBufferViewDescriptor = RHI::BufferViewDescriptor::CreateRaw(0, tlasBufferByteCount);

                        RHI::BufferScopeAttachmentDescriptor desc;
                        desc.m_attachmentId = tlasAttachmentId;
                        desc.m_bufferViewDescriptor = tlasBufferViewDescriptor;
                        desc.m_loadStoreAction.m_loadAction = AZ::RHI::AttachmentLoadAction::Load;

                        frameGraph.UseShaderAttachment(desc, RHI::ScopeAttachmentAccess::ReadWrite);
                    }
                }

                // probe raytrace
                {
                    RHI::ResultCode result = frameGraph.GetAttachmentDatabase().ImportImage(diffuseProbeGrid->GetRayTraceImageAttachmentId(), diffuseProbeGrid->GetRayTraceImage());
                    AZ_Assert(result == RHI::ResultCode::Success, "Failed to import probeRayTraceImage");

                    RHI::ImageScopeAttachmentDescriptor desc;
                    desc.m_attachmentId = diffuseProbeGrid->GetRayTraceImageAttachmentId();
                    desc.m_imageViewDescriptor = diffuseProbeGrid->GetRenderData()->m_probeRayTraceImageViewDescriptor;
                    desc.m_loadStoreAction.m_loadAction = AZ::RHI::AttachmentLoadAction::DontCare;

                    frameGraph.UseShaderAttachment(desc, RHI::ScopeAttachmentAccess::ReadWrite);
                }

                // probe irradiance
                {
                    RHI::ResultCode result = frameGraph.GetAttachmentDatabase().ImportImage(diffuseProbeGrid->GetIrradianceImageAttachmentId(), diffuseProbeGrid->GetIrradianceImage());
                    AZ_Assert(result == RHI::ResultCode::Success, "Failed to import probeIrradianceImage");

                    RHI::ImageScopeAttachmentDescriptor desc;
                    desc.m_attachmentId = diffuseProbeGrid->GetIrradianceImageAttachmentId();
                    desc.m_imageViewDescriptor = diffuseProbeGrid->GetRenderData()->m_probeIrradianceImageViewDescriptor;
                    if (diffuseProbeGrid->GetIrradianceClearRequired())
                    {
                        desc.m_loadStoreAction.m_loadAction = AZ::RHI::AttachmentLoadAction::Clear;
                        diffuseProbeGrid->ResetIrradianceClearRequired();
                    }
                    else
                    {
                        desc.m_loadStoreAction.m_loadAction = AZ::RHI::AttachmentLoadAction::Load;
                    }

                    frameGraph.UseShaderAttachment(desc, RHI::ScopeAttachmentAccess::ReadWrite);
                }

                // probe distance
                {
                    RHI::ResultCode result = frameGraph.GetAttachmentDatabase().ImportImage(diffuseProbeGrid->GetDistanceImageAttachmentId(), diffuseProbeGrid->GetDistanceImage());
                    AZ_Assert(result == RHI::ResultCode::Success, "Failed to import probeDistanceImage");

                    RHI::ImageScopeAttachmentDescriptor desc;
                    desc.m_attachmentId = diffuseProbeGrid->GetDistanceImageAttachmentId();
                    desc.m_imageViewDescriptor = diffuseProbeGrid->GetRenderData()->m_probeDistanceImageViewDescriptor;
                    desc.m_loadStoreAction.m_loadAction = AZ::RHI::AttachmentLoadAction::DontCare;

                    frameGraph.UseShaderAttachment(desc, RHI::ScopeAttachmentAccess::ReadWrite);
                }

                // probe relocation
                {
                    RHI::ResultCode result = frameGraph.GetAttachmentDatabase().ImportImage(diffuseProbeGrid->GetRelocationImageAttachmentId(), diffuseProbeGrid->GetRelocationImage());
                    AZ_Assert(result == RHI::ResultCode::Success, "Failed to import probeRelocationImage");

                    RHI::ImageScopeAttachmentDescriptor desc;
                    desc.m_attachmentId = diffuseProbeGrid->GetRelocationImageAttachmentId();
                    desc.m_imageViewDescriptor = diffuseProbeGrid->GetRenderData()->m_probeRelocationImageViewDescriptor;
                    desc.m_loadStoreAction.m_loadAction = AZ::RHI::AttachmentLoadAction::Load;

                    frameGraph.UseShaderAttachment(desc, RHI::ScopeAttachmentAccess::ReadWrite);
                }
            }
        }

        void DiffuseProbeGridRayTracingPass::CompileResources([[maybe_unused]] const RHI::FrameGraphCompileContext& context)
        {
            RPI::Scene* scene = m_pipeline->GetScene();
            DiffuseProbeGridFeatureProcessor* diffuseProbeGridFeatureProcessor = scene->GetFeatureProcessor<DiffuseProbeGridFeatureProcessor>();
            RayTracingFeatureProcessor* rayTracingFeatureProcessor = scene->GetFeatureProcessor<RayTracingFeatureProcessor>();

            if (rayTracingFeatureProcessor->GetTlas()->GetTlasBuffer() && m_meshCount > 0)
            {
                for (auto& diffuseProbeGrid : diffuseProbeGridFeatureProcessor->GetProbeGrids())
                {
                    // the diffuse probe grid Srg must be updated in the Compile phase in order to successfully bind the ReadWrite shader
                    // inputs (see line ValidateSetImageView() in ShaderResourceGroupData.cpp)
                    diffuseProbeGrid->UpdateRayTraceSrg(m_globalSrgAsset);

                    const Data::Instance<RPI::ShaderResourceGroup>& globalSrg = diffuseProbeGrid->GetRayTraceSrg();

                    RHI::ShaderInputConstantIndex constantIndex = globalSrg->GetLayout()->FindShaderInputConstantIndex(AZ::Name("m_closestHitData"));
                    globalSrg->SetConstantArray(constantIndex, m_closestHitData);

                    RHI::ShaderInputBufferIndex bufferIndex = globalSrg->GetLayout()->FindShaderInputBufferIndex(AZ::Name("m_meshVertexPositions"));
                    globalSrg->SetBufferViewArray(bufferIndex, m_meshVertexPositionBuffer);

                    bufferIndex = globalSrg->GetLayout()->FindShaderInputBufferIndex(AZ::Name("m_meshVertexNormals"));
                    globalSrg->SetBufferViewArray(bufferIndex, m_meshVertexNormalBuffer);

                    bufferIndex = globalSrg->GetLayout()->FindShaderInputBufferIndex(AZ::Name("m_meshIndices"));
                    globalSrg->SetBufferViewArray(bufferIndex, m_meshIndexBuffer);

                    globalSrg->Compile();
                }
            }
        }
    
        void DiffuseProbeGridRayTracingPass::BuildCommandListInternal([[maybe_unused]] const RHI::FrameGraphExecuteContext& context)
        {
            RPI::Scene* scene = m_pipeline->GetScene();
            DiffuseProbeGridFeatureProcessor* diffuseProbeGridFeatureProcessor = scene->GetFeatureProcessor<DiffuseProbeGridFeatureProcessor>();
            RayTracingFeatureProcessor* rayTracingFeatureProcessor = scene->GetFeatureProcessor<RayTracingFeatureProcessor>();
            AZ_Assert(rayTracingFeatureProcessor, "DiffuseProbeGridRayTracingPass requires the RayTracingFeatureProcessor");

            if (!rayTracingFeatureProcessor->GetSubMeshCount())
            {
                return;
            }

            if (!m_rayTracingShaderTable)
            {
                return;
            }

            if (m_meshCount == 0)
            {
                return;
            }

            // submit the DispatchRaysItem for each DiffuseProbeGrid
            for (auto& diffuseProbeGrid : diffuseProbeGridFeatureProcessor->GetProbeGrids())
            {
                RHI::DispatchRaysItem dispatchRaysItem;
                dispatchRaysItem.m_width = diffuseProbeGrid->GetNumRaysPerProbe();
                dispatchRaysItem.m_height = diffuseProbeGrid->GetTotalProbeCount();
                dispatchRaysItem.m_depth = 1;
                dispatchRaysItem.m_rayTracingPipelineState = m_rayTracingPipelineState.get();
                dispatchRaysItem.m_rayTracingShaderTable = m_rayTracingShaderTable.get();
                dispatchRaysItem.m_globalSrg = diffuseProbeGrid->GetRayTraceSrg()->GetRHIShaderResourceGroup();
                dispatchRaysItem.m_globalPipelineState = m_globalPipelineState.get();

                // submit the DispatchRays item
                context.GetCommandList()->Submit(dispatchRaysItem);
            }
        }
    }   // namespace RPI
}   // namespace AZ
