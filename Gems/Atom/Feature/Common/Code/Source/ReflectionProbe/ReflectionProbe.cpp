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

#include <ReflectionProbe/ReflectionProbe.h>
#include <AzCore/Debug/EventTrace.h>
#include <Atom/Feature/ReflectionProbe/ReflectionProbeFeatureProcessor.h>
#include <Atom/RHI/RHISystemInterface.h>
#include <Atom/RPI.Public/Pass/Pass.h>
#include <Atom/RPI.Public/Pass/PassSystemInterface.h>
#include <Atom/RPI.Public/RenderPipeline.h>
#include <Atom/RPI.Public/View.h>
#include <Atom/RPI.Reflect/Asset/AssetUtils.h>
#include <Atom/RPI.Reflect/Material/MaterialAsset.h>
#include <Atom/RPI.Reflect/Pass/EnvironmentCubeMapPassData.h>

namespace AZ
{
    namespace Render
    {
        static const char* ReflectionProbeDrawListTag("reflectionprobevisualization");

        ReflectionProbe::~ReflectionProbe()
        {
            m_meshFeatureProcessor->ReleaseMesh(m_visualizationMeshHandle);
        }

        void ReflectionProbe::Init(RPI::Scene* scene, ReflectionRenderData* reflectionRenderData)
        {
            AZ_Assert(scene, "ReflectionProbe::Init called with a null Scene pointer");

            m_reflectionRenderData = reflectionRenderData;

            // load visualization sphere model and material
            m_meshFeatureProcessor = scene->GetFeatureProcessor<Render::MeshFeatureProcessorInterface>();

            m_visualizationModelAsset = AZ::RPI::AssetUtils::GetAssetByProductPath<AZ::RPI::ModelAsset>(
                "Models/ReflectionProbeSphere.azmodel",
                AZ::RPI::AssetUtils::TraceLevel::Assert);

            m_visualizationMaterialAsset = AZ::RPI::AssetUtils::GetAssetByProductPath<AZ::RPI::MaterialAsset>(
                "Materials/ReflectionProbe/ReflectionProbeVisualization.azmaterial",
                AZ::RPI::AssetUtils::TraceLevel::Assert);

            m_visualizationMeshHandle = m_meshFeatureProcessor->AcquireMesh(m_visualizationModelAsset, AZ::RPI::Material::FindOrCreate(m_visualizationMaterialAsset));
            m_meshFeatureProcessor->SetExcludeFromReflectionCubeMaps(m_visualizationMeshHandle, true);
            m_meshFeatureProcessor->SetRayTracingEnabled(m_visualizationMeshHandle, false);
            m_meshFeatureProcessor->SetTransform(m_visualizationMeshHandle, AZ::Transform::CreateIdentity());

            // reflection render Srgs
            m_stencilSrg = RPI::ShaderResourceGroup::Create(m_reflectionRenderData->m_stencilSrgAsset);
            AZ_Error("ReflectionProbeFeatureProcessor", m_stencilSrg.get(), "Failed to create stencil shader resource group");

            m_blendWeightSrg = RPI::ShaderResourceGroup::Create(m_reflectionRenderData->m_blendWeightSrgAsset);
            AZ_Error("ReflectionProbeFeatureProcessor", m_blendWeightSrg.get(), "Failed to create blend weight shader resource group");

            m_renderOuterSrg = RPI::ShaderResourceGroup::Create(m_reflectionRenderData->m_renderOuterSrgAsset);
            AZ_Error("ReflectionProbeFeatureProcessor", m_renderOuterSrg.get(), "Failed to create render outer reflection shader resource group");

            m_renderInnerSrg = RPI::ShaderResourceGroup::Create(m_reflectionRenderData->m_renderInnerSrgAsset);
            AZ_Error("ReflectionProbeFeatureProcessor", m_renderInnerSrg.get(), "Failed to create render inner reflection shader resource group");
        }

        void ReflectionProbe::Simulate(RPI::Scene* scene, [[maybe_unused]] ReflectionProbeFeatureProcessor* featureProcessor, uint32_t probeIndex)
        {
            AZ_Assert(scene, "ReflectionProbe::Simulate called with a null Scene pointer");

            if (m_buildingCubeMap && m_environmentCubeMapPass->IsFinished())
            {
                // all faces of the cubemap have been rendered, invoke the callback
                m_callback(m_environmentCubeMapPass->GetTextureData(), m_environmentCubeMapPass->GetTextureFormat());

                // remove the pipeline
                scene->RemoveRenderPipeline(m_environmentCubeMapPipelineId);
                m_environmentCubeMapPass = nullptr;

                m_buildingCubeMap = false;
            }

            if (m_updateSrg)
            {
                // stencil Srg
                // Note: the stencil pass uses a slightly reduced inner AABB to avoid seams
                Vector3 innerExtentsReduced = m_innerExtents - Vector3(0.1f, 0.1f, 0.1f);
                Matrix3x4 modelToWorldStencil = Matrix3x4::CreateFromMatrix3x3AndTranslation(Matrix3x3::CreateIdentity(), m_position) * Matrix3x4::CreateScale(innerExtentsReduced);
                m_stencilSrg->SetConstant(m_reflectionRenderData->m_modelToWorldStencilConstantIndex, modelToWorldStencil);
                m_stencilSrg->Compile();

                // blend weight Srg
                Matrix3x4 modelToWorldOuter = Matrix3x4::CreateFromMatrix3x3AndTranslation(Matrix3x3::CreateIdentity(), m_position) * Matrix3x4::CreateScale(m_outerExtents);
                m_blendWeightSrg->SetConstant(m_reflectionRenderData->m_modelToWorldRenderConstantIndex, modelToWorldOuter);
                m_blendWeightSrg->SetConstant(m_reflectionRenderData->m_aabbPosRenderConstantIndex, m_outerAabbWs.GetCenter());
                m_blendWeightSrg->SetConstant(m_reflectionRenderData->m_outerAabbMinRenderConstantIndex, m_outerAabbWs.GetMin());
                m_blendWeightSrg->SetConstant(m_reflectionRenderData->m_outerAabbMaxRenderConstantIndex, m_outerAabbWs.GetMax());
                m_blendWeightSrg->SetConstant(m_reflectionRenderData->m_innerAabbMinRenderConstantIndex, m_innerAabbWs.GetMin());
                m_blendWeightSrg->SetConstant(m_reflectionRenderData->m_innerAabbMaxRenderConstantIndex, m_innerAabbWs.GetMax());
                m_blendWeightSrg->SetConstant(m_reflectionRenderData->m_useParallaxCorrectionRenderConstantIndex, m_useParallaxCorrection);
                m_blendWeightSrg->SetImage(m_reflectionRenderData->m_reflectionCubeMapRenderImageIndex, m_cubeMapImage);
                m_blendWeightSrg->Compile();

                // render outer Srg
                m_renderOuterSrg->SetConstant(m_reflectionRenderData->m_modelToWorldRenderConstantIndex, modelToWorldOuter);
                m_renderOuterSrg->SetConstant(m_reflectionRenderData->m_aabbPosRenderConstantIndex, m_outerAabbWs.GetCenter());
                m_renderOuterSrg->SetConstant(m_reflectionRenderData->m_outerAabbMinRenderConstantIndex, m_outerAabbWs.GetMin());
                m_renderOuterSrg->SetConstant(m_reflectionRenderData->m_outerAabbMaxRenderConstantIndex, m_outerAabbWs.GetMax());
                m_renderOuterSrg->SetConstant(m_reflectionRenderData->m_innerAabbMinRenderConstantIndex, m_innerAabbWs.GetMin());
                m_renderOuterSrg->SetConstant(m_reflectionRenderData->m_innerAabbMaxRenderConstantIndex, m_innerAabbWs.GetMax());
                m_renderOuterSrg->SetConstant(m_reflectionRenderData->m_useParallaxCorrectionRenderConstantIndex, m_useParallaxCorrection);
                m_renderOuterSrg->SetImage(m_reflectionRenderData->m_reflectionCubeMapRenderImageIndex, m_cubeMapImage);
                m_renderOuterSrg->Compile();

                // render inner Srg
                Matrix3x4 modelToWorldInner = Matrix3x4::CreateFromMatrix3x3AndTranslation(Matrix3x3::CreateIdentity(), m_position) * Matrix3x4::CreateScale(m_innerExtents);
                m_renderInnerSrg->SetConstant(m_reflectionRenderData->m_modelToWorldRenderConstantIndex, modelToWorldInner);
                m_renderInnerSrg->SetConstant(m_reflectionRenderData->m_aabbPosRenderConstantIndex, m_outerAabbWs.GetCenter());
                m_renderInnerSrg->SetConstant(m_reflectionRenderData->m_outerAabbMinRenderConstantIndex, m_outerAabbWs.GetMin());
                m_renderInnerSrg->SetConstant(m_reflectionRenderData->m_outerAabbMaxRenderConstantIndex, m_outerAabbWs.GetMax());
                m_renderInnerSrg->SetConstant(m_reflectionRenderData->m_innerAabbMinRenderConstantIndex, m_innerAabbWs.GetMin());
                m_renderInnerSrg->SetConstant(m_reflectionRenderData->m_innerAabbMaxRenderConstantIndex, m_innerAabbWs.GetMax());
                m_renderInnerSrg->SetConstant(m_reflectionRenderData->m_useParallaxCorrectionRenderConstantIndex, m_useParallaxCorrection);
                m_renderInnerSrg->SetImage(m_reflectionRenderData->m_reflectionCubeMapRenderImageIndex, m_cubeMapImage);
                m_renderInnerSrg->Compile();

                m_updateSrg = false;
            }

            // the list index passed in from the feature processor is the index of this probe in the sorted probe list.
            // this is needed to render the probe volumes in order from largest to smallest
            RHI::DrawItemSortKey sortKey = static_cast<RHI::DrawItemSortKey>(probeIndex);
            if (sortKey != m_sortKey)
            {
                // the sort key changed, rebuild draw packets
                m_sortKey = sortKey;

                m_stencilDrawPacket = BuildDrawPacket(
                    m_stencilSrg,
                    m_reflectionRenderData->m_stencilPipelineState,
                    m_reflectionRenderData->m_stencilDrawListTag,
                    Render::StencilRefs::None);

                m_blendWeightDrawPacket = BuildDrawPacket(
                    m_blendWeightSrg,
                    m_reflectionRenderData->m_blendWeightPipelineState,
                    m_reflectionRenderData->m_blendWeightDrawListTag,
                    Render::StencilRefs::UseSpecularIBLPass);

                m_renderOuterDrawPacket = BuildDrawPacket(
                    m_renderOuterSrg,
                    m_reflectionRenderData->m_renderOuterPipelineState,
                    m_reflectionRenderData->m_renderOuterDrawListTag,
                    Render::StencilRefs::UseSpecularIBLPass);

                m_renderInnerDrawPacket = BuildDrawPacket(
                    m_renderInnerSrg,
                    m_reflectionRenderData->m_renderInnerPipelineState,
                    m_reflectionRenderData->m_renderInnerDrawListTag,
                    Render::StencilRefs::UseSpecularIBLPass);
            }
        }

        void ReflectionProbe::RenderReflections(RPI::ViewPtr view)
        {
            // [GFX TODO][ATOM-4364] Add culling for reflection probe volumes

            if (view->HasDrawListTag(m_reflectionRenderData->m_stencilDrawListTag))
            {
                view->AddDrawPacket(m_stencilDrawPacket.get());
            }
            
            if (view->HasDrawListTag(m_reflectionRenderData->m_blendWeightDrawListTag))
            {
                view->AddDrawPacket(m_blendWeightDrawPacket.get());
            }
            
            if (view->HasDrawListTag(m_reflectionRenderData->m_renderOuterDrawListTag))
            {
                view->AddDrawPacket(m_renderOuterDrawPacket.get());
            }
            
            if (view->HasDrawListTag(m_reflectionRenderData->m_renderInnerDrawListTag))
            {
                view->AddDrawPacket(m_renderInnerDrawPacket.get());
            }
        }

        void ReflectionProbe::SetTransform(const AZ::Transform& transform)
        {
            m_position = transform.GetTranslation();
            m_meshFeatureProcessor->SetTransform(m_visualizationMeshHandle, transform);
            m_outerAabbWs = Aabb::CreateCenterHalfExtents(m_position, m_outerExtents / 2.0f);
            m_innerAabbWs = Aabb::CreateCenterHalfExtents(m_position, m_innerExtents / 2.0f);
            m_updateSrg = true;
        }

        void ReflectionProbe::SetOuterExtents(const AZ::Vector3& outerExtents)
        {
            m_outerExtents = outerExtents;
            m_outerAabbWs = Aabb::CreateCenterHalfExtents(m_position, m_outerExtents / 2.0f);
            m_updateSrg = true;
        }

        void ReflectionProbe::SetInnerExtents(const AZ::Vector3& innerExtents)
        {
            m_innerExtents = innerExtents;
            m_innerAabbWs = Aabb::CreateCenterHalfExtents(m_position, m_innerExtents / 2.0f);
            m_updateSrg = true;
        }

        void ReflectionProbe::SetCubeMapImage(const Data::Instance<RPI::Image>& cubeMapImage)
        {
            m_cubeMapImage = cubeMapImage;
            m_updateSrg = true;
        }

        void ReflectionProbe::BuildCubeMap(RPI::Scene* scene, BuildCubeMapCallback callback)
        {
            AZ_Assert(m_buildingCubeMap == false, "ReflectionProbe::BuildCubeMap called while a cubemap build was already in progress");
            if (m_buildingCubeMap)
            {
                return;
            }

            m_buildingCubeMap = true;
            m_callback = callback;

            AZ::RPI::RenderPipelineDescriptor environmentCubeMapPipelineDesc;
            environmentCubeMapPipelineDesc.m_mainViewTagName = "MainCamera";

            // create a unique name for the pipeline
            AZ::Uuid uuid = AZ::Uuid::CreateRandom();
            AZStd::string uuidString;
            uuid.ToString(uuidString);
            environmentCubeMapPipelineDesc.m_name = AZStd::string::format("EnvironmentCubeMapPipeline_%s", uuidString.c_str());
            RPI::RenderPipelinePtr environmentCubeMapPipeline = AZ::RPI::RenderPipeline::CreateRenderPipeline(environmentCubeMapPipelineDesc);
            m_environmentCubeMapPipelineId = environmentCubeMapPipeline->GetId();

            AZStd::shared_ptr<RPI::EnvironmentCubeMapPassData> passData = AZStd::make_shared<RPI::EnvironmentCubeMapPassData>();
            passData->m_position = m_position;

            RPI::PassDescriptor environmentCubeMapPassDescriptor(Name("EnvironmentCubeMapPass"));
            environmentCubeMapPassDescriptor.m_passData = passData;

            m_environmentCubeMapPass = RPI::EnvironmentCubeMapPass::Create(environmentCubeMapPassDescriptor);
            m_environmentCubeMapPass->SetRenderPipeline(environmentCubeMapPipeline.get());

            const RPI::Ptr<RPI::ParentPass>& rootPass = environmentCubeMapPipeline->GetRootPass();
            rootPass->AddChild(m_environmentCubeMapPass);

            scene->AddRenderPipeline(environmentCubeMapPipeline);
        }

        void ReflectionProbe::OnRenderPipelinePassesChanged(RPI::RenderPipeline* renderPipeline)
        {
            // check for an active cubemap build, and that the renderPipeline was created by this probe
            if (m_environmentCubeMapPass
                && m_buildingCubeMap
                && m_environmentCubeMapPipelineId == renderPipeline->GetId())
            {
                m_environmentCubeMapPass->SetDefaultView();
            }
        }

        void ReflectionProbe::ShowVisualization(bool showVisualization)
        {
            m_meshFeatureProcessor->SetVisible(m_visualizationMeshHandle, showVisualization);
        }

        const RHI::DrawPacket* ReflectionProbe::BuildDrawPacket(
            const Data::Instance<RPI::ShaderResourceGroup>& srg,
            const RPI::Ptr<RPI::PipelineStateForDraw>& pipelineState,
            const RHI::DrawListTag& drawListTag,
            uint32_t stencilRef)
        {
            AZ_Assert(m_sortKey != InvalidSortKey, "Invalid probe sort key");

            if (pipelineState->GetRHIPipelineState() == nullptr)
            {
                return nullptr;
            }

            RHI::DrawPacketBuilder drawPacketBuilder;

            RHI::DrawIndexed drawIndexed;
            drawIndexed.m_indexCount = (uint32_t)m_reflectionRenderData->m_boxIndexCount;
            drawIndexed.m_indexOffset = 0;
            drawIndexed.m_vertexOffset = 0;

            drawPacketBuilder.Begin(nullptr);
            drawPacketBuilder.SetDrawArguments(drawIndexed);
            drawPacketBuilder.SetIndexBufferView(m_reflectionRenderData->m_boxIndexBufferView);
            drawPacketBuilder.AddShaderResourceGroup(srg->GetRHIShaderResourceGroup());

            RHI::DrawPacketBuilder::DrawRequest drawRequest;
            drawRequest.m_listTag = drawListTag;
            drawRequest.m_pipelineState = pipelineState->GetRHIPipelineState();
            drawRequest.m_streamBufferViews = m_reflectionRenderData->m_boxPositionBufferView;
            drawRequest.m_stencilRef = stencilRef;
            drawRequest.m_sortKey = m_sortKey;
            drawPacketBuilder.AddDrawItem(drawRequest);

            return drawPacketBuilder.End();
        }
    } // namespace Render
} // namespace AZ
