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
            RHI::PipelineStateDescriptorForRayTracing shaderDescriptor;
            shaderVariant.ConfigurePipelineState(shaderDescriptor);

            // global pipeline state and Srg
            m_globalPipelineState = m_rayTracingShader->AcquirePipelineState(shaderDescriptor);
            AZ_Assert(m_globalPipelineState, "Failed to acquire ray tracing global pipeline state");

            m_globalSrgAsset = m_rayTracingShader->FindShaderResourceGroupAsset(Name{ "RayTracingGlobalSrg" });
            AZ_Error("ReflectionProbeFeatureProcessor", m_globalSrgAsset.GetId().IsValid(), "Failed to find RayTracingGlobalSrg asset for shader [%s]", shaderFilePath.c_str());
            AZ_Error("ReflectionProbeFeatureProcessor", m_globalSrgAsset.IsReady(), "RayTracingGlobalSrg asset is not loaded for shader [%s]", shaderFilePath.c_str());

            // build the ray tracing pipeline state descriptor
            // [GFX TODO][ATOM-5570] Use the shader PipelineLayout to set DXR root signatures
            RHI::RayTracingPipelineStateDescriptor descriptor;
            descriptor.Build()
                ->MaxPayloadSize(48)
                ->MaxAttributeSize(32)
                ->MaxRecursionDepth(1)
                ->ShaderLibrary(shaderDescriptor)
                ->HitGroup(AZ::Name("HitGroup"))
                    ->ClosestHitShaderName(AZ::Name("ClosestHit"))
                ->GlobalRootSignature()
                    ->CbvParameter()
                        ->RootParameterIndex(0)
                        ->ShaderRegister(0)
                        ->RegisterSpace(0)
                    ->DescriptorTableParameter()
                        ->RootParameterIndex(1)
                        ->Range()
                            ->ShaderRegister(0) // m_scene
                            ->RegisterSpace(0)
                            ->RangeType(RHI::RayTracingRootSignatureDescriptorTableParam::Range::Type::Read)
                        ->Range()
                            ->ShaderRegister(3) // m_directionalLights
                            ->RegisterSpace(0)
                            ->RangeType(RHI::RayTracingRootSignatureDescriptorTableParam::Range::Type::Read)
                        ->Range()
                            ->ShaderRegister(4) // m_pointLights
                            ->RegisterSpace(0)
                            ->RangeType(RHI::RayTracingRootSignatureDescriptorTableParam::Range::Type::Read)
                        ->Range()
                            ->ShaderRegister(5) // m_spotLights
                            ->RegisterSpace(0)
                            ->RangeType(RHI::RayTracingRootSignatureDescriptorTableParam::Range::Type::Read)
                        ->Range()
                            ->ShaderRegister(0) // m_probeRayTrace
                            ->RegisterSpace(0)
                            ->RangeType(RHI::RayTracingRootSignatureDescriptorTableParam::Range::Type::ReadWrite)
                        ->Range()
                            ->ShaderRegister(1) // m_probeOffsets
                            ->RegisterSpace(0)
                            ->RangeType(RHI::RayTracingRootSignatureDescriptorTableParam::Range::Type::ReadWrite)
                        ->Range()
                            ->ShaderRegister(1) // m_probeIrradiance
                            ->RegisterSpace(0)
                            ->RangeType(RHI::RayTracingRootSignatureDescriptorTableParam::Range::Type::Read)
                        ->Range()
                            ->ShaderRegister(2) // m_probeDistance
                            ->RegisterSpace(0)
                            ->RangeType(RHI::RayTracingRootSignatureDescriptorTableParam::Range::Type::Read)
                    ->StaticSampler()
                        ->ShaderRegister(0)
                        ->RegisterSpace(0)
                        ->AddressMode(RHI::AddressMode::Clamp)
                        ->FilterMode(RHI::FilterMode::Linear)
                ->LocalRootSignature()
                    ->ShaderAssociation(AZ::Name("HitGroup"))
                    ->CbvParameter()
                        ->ShaderRegister(0)
                        ->RegisterSpace(1)
                    ->DescriptorTableParameter()
                        ->Range()
                            ->ShaderRegister(0) // m_indices
                            ->RegisterSpace(1)
                            ->RangeType(RHI::RayTracingRootSignatureDescriptorTableParam::Range::Type::Read)
                        ->Range()
                            ->ShaderRegister(1) // m_vertexPositions
                            ->RegisterSpace(1)
                            ->RangeType(RHI::RayTracingRootSignatureDescriptorTableParam::Range::Type::Read)
                        ->Range()
                            ->ShaderRegister(2) // m_vertexPositions
                            ->RegisterSpace(1)
                            ->RangeType(RHI::RayTracingRootSignatureDescriptorTableParam::Range::Type::Read)
                ;

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
                m_localSrgs.clear();

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
                // create a SRG for every ray tracing mesh in the scene
                RPI::Scene* scene = m_pipeline->GetScene();
                TransformServiceFeatureProcessor* transformFeatureProcessor = scene->GetFeatureProcessor<TransformServiceFeatureProcessor>();
                RayTracingFeatureProcessor* rayTracingFeatureProcessor = scene->GetFeatureProcessor<RayTracingFeatureProcessor>();

                Data::Asset<RPI::ShaderResourceGroupAsset> localSrgAsset = m_rayTracingShader->FindShaderResourceGroupAsset(Name{ "RayTracingLocalSrg" });
                AZ_Error("DiffuseProbeRayTracingPass", localSrgAsset.GetId().IsValid(), "Failed to find RayTracingLocalSrg_HitSolid asset");
                AZ_Error("DiffuseProbeRayTracingPass", localSrgAsset.IsReady(), "RayTracingLocalSrg_HitSolid asset is not loaded");

                m_localSrgs.resize(rayTracingFeatureProcessor->GetSubMeshCount());

                const RayTracingFeatureProcessor::MeshMap& rayTracingMeshes = rayTracingFeatureProcessor->GetMeshes();
                uint32_t localSrgIndex = 0;
                for (const auto& mesh : rayTracingMeshes)
                {
                    const RayTracingFeatureProcessor::SubMeshVector& subMeshes = mesh.second.m_subMeshes;
                    for (const auto& subMesh : subMeshes)
                    {
                        m_localSrgs[localSrgIndex] = RPI::ShaderResourceGroup::Create(localSrgAsset);
                        AZ::Data::Instance<RPI::ShaderResourceGroup>& localSrg = m_localSrgs[localSrgIndex];
                        AZ_Error("DiffuseProbeRayTracingPass", localSrg.get(), "Failed to create RayTracingLocalSrg shader resource group");

                        const RHI::ShaderResourceGroupLayout* srgLayout = localSrg->GetLayout();

                        // material color
                        const auto materialColorConstantIndex = srgLayout->FindShaderInputConstantIndex(AZ::Name("m_materialColor"));
                        Vector3 color(subMesh.m_irradianceColor.GetR(), subMesh.m_irradianceColor.GetG(), subMesh.m_irradianceColor.GetB());
                        localSrg->SetConstant(materialColorConstantIndex, color);

                        // world inverse transpose
                        AZ::Transform meshTransform = transformFeatureProcessor->GetTransformForId(TransformServiceFeatureProcessorInterface::ObjectId(mesh.first));
                        AZ::Transform noScaleTransform = meshTransform;
                        noScaleTransform.ExtractScale();
                        AZ::Matrix3x3 rotationMatrix = Matrix3x3::CreateFromTransform(noScaleTransform);
                        rotationMatrix = rotationMatrix.GetInverseFull().GetTranspose();

                        const auto worldInvTransposeMatrixConstantIndex = srgLayout->FindShaderInputConstantIndex(AZ::Name("m_worldInvTranspose"));
                        localSrg->SetConstant(worldInvTransposeMatrixConstantIndex, rotationMatrix);

                        // vertex position buffer
                        const auto vbPositionBufferIndex = srgLayout->FindShaderInputBufferIndex(AZ::Name("m_vertexPositions"));
                        localSrg->SetBufferView(vbPositionBufferIndex, subMesh.m_positionShaderBufferView.get());

                        // vertex normal buffer
                        const auto vbPositionNormalIndex = srgLayout->FindShaderInputBufferIndex(AZ::Name("m_vertexNormals"));
                        localSrg->SetBufferView(vbPositionNormalIndex, subMesh.m_normalShaderBufferView.get());

                        // index buffer
                        const auto ibBufferIndex = srgLayout->FindShaderInputBufferIndex(AZ::Name("m_indices"));
                        localSrg->SetBufferView(ibBufferIndex, subMesh.m_indexShaderBufferView.get());

                        localSrg->Compile();
                        localSrgIndex++;
                    }
                }
            };

            const auto executeFunction = [this]([[maybe_unused]] const RHI::FrameGraphExecuteContext& context, [[maybe_unused]] const ScopeData& scopeData)
            {
                RHI::Ptr<RHI::Device> device = RHI::RHISystemInterface::Get()->GetDevice();
                RayTracingFeatureProcessor* rayTracingFeatureProcessor = m_pipeline->GetScene()->GetFeatureProcessor<RayTracingFeatureProcessor>();
                RHI::RayTracingBufferPools& rayTracingBufferPools = rayTracingFeatureProcessor->GetBufferPools();

                if (m_localSrgs.empty())
                {
                    m_rayTracingShaderTable = nullptr;
                    return;
                }

                // build the ray tracing shader table descriptor
                RHI::RayTracingShaderTableDescriptor descriptor;
                RHI::RayTracingShaderTableDescriptor* descriptorBuild = descriptor.Build(AZ::Name("RayTracingShaderTable"), m_rayTracingPipelineState)
                    ->RayGenerationRecord(AZ::Name("RayGen"))
                    ->MissRecord(AZ::Name("Miss"));

                for (auto& localSrg : m_localSrgs)
                {
                    descriptorBuild->HitGroupRecord(AZ::Name("HitGroup"))
                        ->ShaderResourceGroup(localSrg->GetRHIShaderResourceGroup())
                        ;
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

        void DiffuseProbeGridRayTracingPass::SetupFrameGraphDependencies(RHI::FrameGraphInterface frameGraph, const RPI::PassScopeProducer& producer)
        {
            RenderPass::SetupFrameGraphDependencies(frameGraph, producer);

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
                    desc.m_loadStoreAction.m_loadAction = AZ::RHI::AttachmentLoadAction::DontCare;

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

        void DiffuseProbeGridRayTracingPass::CompileResources([[maybe_unused]] const RHI::FrameGraphCompileContext& context, [[maybe_unused]] const RPI::PassScopeProducer& producer)
        {
            RPI::Scene* scene = m_pipeline->GetScene();
            DiffuseProbeGridFeatureProcessor* diffuseProbeGridFeatureProcessor = scene->GetFeatureProcessor<DiffuseProbeGridFeatureProcessor>();
            RayTracingFeatureProcessor* rayTracingFeatureProcessor = scene->GetFeatureProcessor<RayTracingFeatureProcessor>();

            if (rayTracingFeatureProcessor->GetTlas()->GetTlasBuffer())
            {
                for (auto& diffuseProbeGrid : diffuseProbeGridFeatureProcessor->GetProbeGrids())
                {
                    // the diffuse probe grid Srg must be updated in the Compile phase in order to successfully bind the ReadWrite shader inputs
                    // (see line ValidateSetImageView() in ShaderResourceGroupData.cpp)
                    diffuseProbeGrid->UpdateRayTraceSrg(m_globalSrgAsset, scene);
                }
            }
        }
    
        void DiffuseProbeGridRayTracingPass::BuildCommandList([[maybe_unused]] const RHI::FrameGraphExecuteContext& context, [[maybe_unused]] const RPI::PassScopeProducer& producer)
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

            if (m_localSrgs.empty())
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
