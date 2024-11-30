/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <ReflectionProbe/ReflectionProbe.h>
#include <ReflectionProbe/ReflectionProbeFeatureProcessor.h>
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
        ReflectionProbe::~ReflectionProbe()
        {
            Data::AssetBus::Handler::BusDisconnect();
            m_scene->GetCullingScene()->UnregisterCullable(m_cullable);
            m_meshFeatureProcessor->ReleaseMesh(m_visualizationMeshHandle);
        }

        void ReflectionProbe::OnAssetReady(Data::Asset<Data::AssetData> asset)
        {
            if (m_visualizationMaterialAsset.GetId() == asset.GetId())
            {
                m_visualizationMaterialAsset = asset;
                Data::AssetBus::Handler::BusDisconnect();

                m_meshFeatureProcessor->SetCustomMaterials(m_visualizationMeshHandle, AZ::RPI::Material::FindOrCreate(m_visualizationMaterialAsset));
            }
        }

        void ReflectionProbe::OnAssetError(Data::Asset<Data::AssetData> asset)
        {
            AZ_Error("ReflectionProbe", false, "Failed to load ReflectionProbe dependency asset %s", asset.ToString<AZStd::string>().c_str());
            Data::AssetBus::Handler::BusDisconnect();
        }

        void ReflectionProbe::Init(RPI::Scene* scene, ReflectionRenderData* reflectionRenderData)
        {
            AZ_Assert(scene, "ReflectionProbe::Init called with a null Scene pointer");

            m_scene = scene;
            m_reflectionRenderData = reflectionRenderData;

            CubeMapRenderer::SetScene(m_scene);

            // load visualization sphere model and material
            m_meshFeatureProcessor = m_scene->GetFeatureProcessor<Render::MeshFeatureProcessorInterface>();

            // We don't have to pre-load this asset before passing it to MeshFeatureProcessor, because the MeshFeatureProcessor will handle the async-load for us.
            m_visualizationModelAsset = AZ::RPI::AssetUtils::GetAssetByProductPath<AZ::RPI::ModelAsset>(
                "Models/ReflectionProbeSphere.fbx.azmodel",
                AZ::RPI::AssetUtils::TraceLevel::Assert);

            MeshHandleDescriptor visualizationMeshDescriptor;
            visualizationMeshDescriptor.m_modelAsset = m_visualizationModelAsset;
            visualizationMeshDescriptor.m_isRayTracingEnabled = false;
            m_visualizationMeshHandle = m_meshFeatureProcessor->AcquireMesh(visualizationMeshDescriptor);

            m_meshFeatureProcessor->SetExcludeFromReflectionCubeMaps(m_visualizationMeshHandle, true);
            m_meshFeatureProcessor->SetTransform(m_visualizationMeshHandle, AZ::Transform::CreateIdentity());

            // We have to pre-load this asset before creating a material instance because the InstanceDatabase will attempt a blocking load which could deadlock,
            // particularly when slices are involved.
            // Note that m_visualizationMeshHandle had to be set up first, because AssetBus BusConnect() might call ReflectionProbe::OnAssetReady() immediately on this callstack.
            m_visualizationMaterialAsset = AZ::RPI::AssetUtils::GetAssetByProductPath<AZ::RPI::MaterialAsset>(
                "Materials/ReflectionProbe/ReflectionProbeVisualization.azmaterial",
                AZ::RPI::AssetUtils::TraceLevel::Assert);
            m_visualizationMaterialAsset.QueueLoad();
            Data::AssetBus::Handler::BusConnect(m_visualizationMaterialAsset.GetId());

            // reflection render Srgs
            m_stencilSrg = RPI::ShaderResourceGroup::Create(
                m_reflectionRenderData->m_stencilShader->GetAsset(),
                m_reflectionRenderData->m_stencilShader->GetSupervariantIndex(),
                m_reflectionRenderData->m_stencilSrgLayout->GetName());
            AZ_Error("ReflectionProbeFeatureProcessor", m_stencilSrg.get(), "Failed to create stencil shader resource group");

            m_blendWeightSrg = RPI::ShaderResourceGroup::Create(
                m_reflectionRenderData->m_blendWeightShader->GetAsset(),
                m_reflectionRenderData->m_blendWeightShader->GetSupervariantIndex(),
                m_reflectionRenderData->m_blendWeightSrgLayout->GetName());
            AZ_Error("ReflectionProbeFeatureProcessor", m_blendWeightSrg.get(), "Failed to create blend weight shader resource group");

            m_renderOuterSrg = RPI::ShaderResourceGroup::Create(
                m_reflectionRenderData->m_renderOuterShader->GetAsset(),
                m_reflectionRenderData->m_renderOuterShader->GetSupervariantIndex(),
                m_reflectionRenderData->m_renderOuterSrgLayout->GetName());
            AZ_Error("ReflectionProbeFeatureProcessor", m_renderOuterSrg.get(), "Failed to create render outer reflection shader resource group");

            m_renderInnerSrg = RPI::ShaderResourceGroup::Create(
                m_reflectionRenderData->m_renderInnerShader->GetAsset(),
                m_reflectionRenderData->m_renderInnerShader->GetSupervariantIndex(),
                m_reflectionRenderData->m_renderInnerSrgLayout->GetName());
            AZ_Error("ReflectionProbeFeatureProcessor", m_renderInnerSrg.get(), "Failed to create render inner reflection shader resource group");

            // setup culling
            m_cullable.SetDebugName(AZ::Name("ReflectionProbe Volume"));
        }

        void ReflectionProbe::Simulate(uint32_t probeIndex)
        {
            CubeMapRenderer::Update();

            // track if we need to update culling based on changes to the draw packets or Srg
            bool updateCulling = false;

            if (m_updateSrg)
            {
                // stencil Srg
                // Note: the stencil pass uses a slightly reduced inner OBB to avoid seams
                Vector3 innerExtentsReduced = m_innerExtents - Vector3(0.1f, 0.1f, 0.1f);
                Matrix3x4 modelToWorldStencil = Matrix3x4::CreateFromQuaternionAndTranslation(m_transform.GetRotation(), m_transform.GetTranslation()) * Matrix3x4::CreateScale(innerExtentsReduced);
                m_stencilSrg->SetConstant(m_reflectionRenderData->m_modelToWorldStencilConstantIndex, modelToWorldStencil);
                m_stencilSrg->Compile();

                Matrix3x4 modelToWorldInverse = Matrix3x4::CreateFromTransform(m_transform).GetInverseFull();

                // blend weight Srg
                Matrix3x4 modelToWorldOuter = Matrix3x4::CreateFromQuaternionAndTranslation(m_transform.GetRotation(), m_transform.GetTranslation()) * Matrix3x4::CreateScale(m_outerExtents);
                m_blendWeightSrg->SetConstant(m_reflectionRenderData->m_modelToWorldRenderConstantIndex, modelToWorldOuter);
                m_blendWeightSrg->SetConstant(m_reflectionRenderData->m_modelToWorldInverseRenderConstantIndex, modelToWorldInverse);
                m_blendWeightSrg->SetConstant(m_reflectionRenderData->m_outerObbHalfLengthsRenderConstantIndex, m_outerObbWs.GetHalfLengths());
                m_blendWeightSrg->SetConstant(m_reflectionRenderData->m_innerObbHalfLengthsRenderConstantIndex, m_innerObbWs.GetHalfLengths());
                m_blendWeightSrg->SetConstant(m_reflectionRenderData->m_useParallaxCorrectionRenderConstantIndex, m_useParallaxCorrection);
                m_blendWeightSrg->SetImage(m_reflectionRenderData->m_reflectionCubeMapRenderImageIndex, m_cubeMapImage);
                m_blendWeightSrg->Compile();

                // render outer Srg
                m_renderOuterSrg->SetConstant(m_reflectionRenderData->m_modelToWorldRenderConstantIndex, modelToWorldOuter);
                m_renderOuterSrg->SetConstant(m_reflectionRenderData->m_modelToWorldInverseRenderConstantIndex, modelToWorldInverse);
                m_renderOuterSrg->SetConstant(m_reflectionRenderData->m_outerObbHalfLengthsRenderConstantIndex, m_outerObbWs.GetHalfLengths());
                m_renderOuterSrg->SetConstant(m_reflectionRenderData->m_innerObbHalfLengthsRenderConstantIndex, m_innerObbWs.GetHalfLengths());
                m_renderOuterSrg->SetConstant(m_reflectionRenderData->m_useParallaxCorrectionRenderConstantIndex, m_useParallaxCorrection);
                m_renderOuterSrg->SetConstant(m_reflectionRenderData->m_exposureConstantIndex, m_renderExposure);
                m_renderOuterSrg->SetImage(m_reflectionRenderData->m_reflectionCubeMapRenderImageIndex, m_cubeMapImage);
                m_renderOuterSrg->Compile();

                // render inner Srg
                Matrix3x4 modelToWorldInner = Matrix3x4::CreateFromQuaternionAndTranslation(m_transform.GetRotation(), m_transform.GetTranslation()) * Matrix3x4::CreateScale(m_innerExtents);
                m_renderInnerSrg->SetConstant(m_reflectionRenderData->m_modelToWorldRenderConstantIndex, modelToWorldInner);
                m_renderInnerSrg->SetConstant(m_reflectionRenderData->m_modelToWorldInverseRenderConstantIndex, modelToWorldInverse);
                m_renderInnerSrg->SetConstant(m_reflectionRenderData->m_outerObbHalfLengthsRenderConstantIndex, m_outerObbWs.GetHalfLengths());
                m_renderInnerSrg->SetConstant(m_reflectionRenderData->m_innerObbHalfLengthsRenderConstantIndex, m_innerObbWs.GetHalfLengths());
                m_renderInnerSrg->SetConstant(m_reflectionRenderData->m_useParallaxCorrectionRenderConstantIndex, m_useParallaxCorrection);
                m_renderInnerSrg->SetConstant(m_reflectionRenderData->m_exposureConstantIndex, m_renderExposure);
                m_renderInnerSrg->SetImage(m_reflectionRenderData->m_reflectionCubeMapRenderImageIndex, m_cubeMapImage);
                m_renderInnerSrg->Compile();

                m_updateSrg = false;
                updateCulling = true;
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
                    Render::StencilRefs::UseIBLSpecularPass);

                m_renderOuterDrawPacket = BuildDrawPacket(
                    m_renderOuterSrg,
                    m_reflectionRenderData->m_renderOuterPipelineState,
                    m_reflectionRenderData->m_renderOuterDrawListTag,
                    Render::StencilRefs::UseIBLSpecularPass);

                m_renderInnerDrawPacket = BuildDrawPacket(
                    m_renderInnerSrg,
                    m_reflectionRenderData->m_renderInnerPipelineState,
                    m_reflectionRenderData->m_renderInnerDrawListTag,
                    Render::StencilRefs::UseIBLSpecularPass);

                updateCulling = true;
            }

            if (updateCulling)
            {
                UpdateCulling();
            }
        }

        void ReflectionProbe::OnRenderEnd()
        {
            CubeMapRenderer::CheckAndRemovePipeline();
        }

        void ReflectionProbe::SetTransform(const AZ::Transform& transform)
        {
            // retrieve previous scale and revert the scale on the inner/outer extents
            float previousScale = m_transform.GetUniformScale();
            m_outerExtents /= previousScale;
            m_innerExtents /= previousScale;

            // store new transform
            m_transform = transform;

            // avoid scaling the visualization sphere
            AZ::Transform visualizationTransform = m_transform;
            visualizationTransform.ExtractUniformScale();
            m_meshFeatureProcessor->SetTransform(m_visualizationMeshHandle, visualizationTransform);

            // update the inner/outer extents with the new scale
            m_outerExtents *= m_transform.GetUniformScale();
            m_innerExtents *= m_transform.GetUniformScale();

            m_outerObbWs = Obb::CreateFromPositionRotationAndHalfLengths(m_transform.GetTranslation(), m_transform.GetRotation(), m_outerExtents / 2.0f);
            m_innerObbWs = Obb::CreateFromPositionRotationAndHalfLengths(m_transform.GetTranslation(), m_transform.GetRotation(), m_innerExtents / 2.0f);
            m_updateSrg = true;
        }

        void ReflectionProbe::SetOuterExtents(const AZ::Vector3& outerExtents)
        {
            m_outerExtents = outerExtents * m_transform.GetUniformScale();
            m_outerObbWs = Obb::CreateFromPositionRotationAndHalfLengths(m_transform.GetTranslation(), m_transform.GetRotation(), m_outerExtents / 2.0f);
            m_updateSrg = true;
        }

        void ReflectionProbe::SetInnerExtents(const AZ::Vector3& innerExtents)
        {
            m_innerExtents = innerExtents * m_transform.GetUniformScale();
            m_innerObbWs = Obb::CreateFromPositionRotationAndHalfLengths(m_transform.GetTranslation(), m_transform.GetRotation(), m_innerExtents / 2.0f);
            m_updateSrg = true;
        }

        void ReflectionProbe::SetCubeMapImage(const Data::Instance<RPI::Image>& cubeMapImage, const AZStd::string& relativePath)
        {
            m_cubeMapImage = cubeMapImage;
            m_cubeMapRelativePath = relativePath;
            m_updateSrg = true;
        }

        void ReflectionProbe::Bake(RenderCubeMapCallback callback)
        {
            CubeMapRenderer::StartRender(callback, m_transform, m_bakeExposure);
        }

        void ReflectionProbe::OnRenderPipelinePassesChanged(RPI::RenderPipeline* renderPipeline)
        {
            CubeMapRenderer::SetDefaultView(renderPipeline);
        }

        void ReflectionProbe::ShowVisualization(bool showVisualization)
        {
            m_meshFeatureProcessor->SetVisible(m_visualizationMeshHandle, showVisualization);
        }

        void ReflectionProbe::SetRenderExposure(float renderExposure)
        {
            m_renderExposure = renderExposure;
            m_updateSrg = true;
        }

        void ReflectionProbe::SetBakeExposure(float bakeExposure)
        {
            m_bakeExposure = bakeExposure;
        }

        RHI::ConstPtr<RHI::DrawPacket> ReflectionProbe::BuildDrawPacket(
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

            RHI::DrawPacketBuilder drawPacketBuilder{RHI::MultiDevice::AllDevices};
            drawPacketBuilder.Begin(nullptr);
            drawPacketBuilder.SetGeometryView(&m_reflectionRenderData->m_geometryView);
            drawPacketBuilder.AddShaderResourceGroup(srg->GetRHIShaderResourceGroup());

            RHI::DrawPacketBuilder::DrawRequest drawRequest;
            drawRequest.m_listTag = drawListTag;
            drawRequest.m_streamIndices = m_reflectionRenderData->m_geometryView.GetFullStreamBufferIndices();
            drawRequest.m_pipelineState = pipelineState->GetRHIPipelineState();
            drawRequest.m_stencilRef = static_cast<uint8_t>(stencilRef);
            drawRequest.m_sortKey = m_sortKey;
            drawPacketBuilder.AddDrawItem(drawRequest);

            return drawPacketBuilder.End();
        }

        void ReflectionProbe::UpdateCulling()
        {
            // set draw list mask
            m_cullable.m_cullData.m_drawListMask.reset();

            // check for draw packets due certain render pipelines such as lowend render pipeline that might not have this feature enabled 
            if (m_stencilDrawPacket)
            {
                m_cullable.m_cullData.m_drawListMask |= m_stencilDrawPacket->GetDrawListMask();
            } 
            
            if (m_blendWeightDrawPacket)
            {
                m_cullable.m_cullData.m_drawListMask |= m_blendWeightDrawPacket->GetDrawListMask();
            } 
            
            if (m_renderOuterDrawPacket)
            {
                m_cullable.m_cullData.m_drawListMask |= m_renderOuterDrawPacket->GetDrawListMask();
            } 
            
            if (m_renderInnerDrawPacket)
            {
                m_cullable.m_cullData.m_drawListMask |= m_renderInnerDrawPacket->GetDrawListMask();
            }

            // setup the Lod entry, using one entry for all four draw packets
            m_cullable.m_lodData.m_lods.clear();
            m_cullable.m_lodData.m_lods.resize(1);
            RPI::Cullable::LodData::Lod& lod = m_cullable.m_lodData.m_lods.back();

            // add draw packets
            lod.m_drawPackets.push_back(m_stencilDrawPacket.get());
            lod.m_drawPackets.push_back(m_blendWeightDrawPacket.get());
            lod.m_drawPackets.push_back(m_renderOuterDrawPacket.get());
            lod.m_drawPackets.push_back(m_renderInnerDrawPacket.get());

            // set screen coverage
            // probe volume should cover at least a screen pixel at 1080p to be drawn
            static const float MinimumScreenCoverage = 1.0f / 1080.0f;
            lod.m_screenCoverageMin = MinimumScreenCoverage;
            lod.m_screenCoverageMax = 1.0f;

            // update cullable bounds
            Aabb outerAabb = Aabb::CreateFromObb(m_outerObbWs);
            Vector3 center;
            float radius;
            outerAabb.GetAsSphere(center, radius);

            m_cullable.m_cullData.m_boundingSphere = Sphere(center, radius);
            m_cullable.m_cullData.m_boundingObb = m_outerObbWs;
            m_cullable.m_cullData.m_visibilityEntry.m_boundingVolume = outerAabb;
            m_cullable.m_cullData.m_visibilityEntry.m_userData = &m_cullable;
            m_cullable.m_cullData.m_visibilityEntry.m_typeFlags = AzFramework::VisibilityEntry::TYPE_RPI_Cullable;
            m_cullable.m_cullData.m_componentUuid = m_uuid;
            m_cullable.m_cullData.m_componentType = Culling::ComponentType::ReflectionProbe;

            // register with culling system
            m_scene->GetCullingScene()->RegisterOrUpdateCullable(m_cullable);
        }
    } // namespace Render
} // namespace AZ
