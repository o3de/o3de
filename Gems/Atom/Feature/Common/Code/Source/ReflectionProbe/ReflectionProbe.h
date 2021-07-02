/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Math/Transform.h>
#include <RenderCommon.h>
#include <Atom/Feature/Mesh/MeshFeatureProcessorInterface.h>
#include <Atom/RPI.Public/Base.h>
#include <Atom/RPI.Public/Model/Model.h>
#include <Atom/RPI.Public/MeshDrawPacket.h>
#include <Atom/RPI.Public/Pass/Specific/EnvironmentCubeMapPass.h>
#include <Atom/RPI.Public/PipelineState.h>
#include <Atom/RPI.Public/Shader/ShaderResourceGroup.h>
#include <Atom/RPI.Reflect/Shader/ShaderResourceGroupAsset.h>
#include <Atom/RPI.Public/Scene.h>

namespace AZ
{
    namespace Render
    {
        class ReflectionProbeFeatureProcessor;
        using BuildCubeMapCallback = AZStd::function<void(uint8_t* const* cubeMapTextureData, const RHI::Format cubeMapTextureFormat)>;

        // shared data for rendering reflections, loaded and stored by the ReflectionProbeFeatureProcessor and passed to all probes
        struct ReflectionRenderData
        {
            AZStd::array<RHI::StreamBufferView, 1> m_boxPositionBufferView;
            RHI::IndexBufferView m_boxIndexBufferView;
            uint32_t m_boxIndexCount = 0;

            RPI::Ptr<RPI::PipelineStateForDraw> m_stencilPipelineState;
            RPI::Ptr<RPI::PipelineStateForDraw> m_blendWeightPipelineState;
            RPI::Ptr<RPI::PipelineStateForDraw> m_renderOuterPipelineState;
            RPI::Ptr<RPI::PipelineStateForDraw> m_renderInnerPipelineState;

            Data::Asset<RPI::ShaderResourceGroupAsset> m_stencilSrgAsset;
            Data::Asset<RPI::ShaderResourceGroupAsset> m_blendWeightSrgAsset;
            Data::Asset<RPI::ShaderResourceGroupAsset> m_renderOuterSrgAsset;
            Data::Asset<RPI::ShaderResourceGroupAsset> m_renderInnerSrgAsset;

            RHI::DrawListTag m_stencilDrawListTag;
            RHI::DrawListTag m_blendWeightDrawListTag;
            RHI::DrawListTag m_renderOuterDrawListTag;
            RHI::DrawListTag m_renderInnerDrawListTag;

            RHI::ShaderInputConstantIndex m_modelToWorldStencilConstantIndex;
            RHI::ShaderInputConstantIndex m_modelToWorldRenderConstantIndex;
            RHI::ShaderInputConstantIndex m_aabbPosRenderConstantIndex;
            RHI::ShaderInputConstantIndex m_outerAabbMinRenderConstantIndex;
            RHI::ShaderInputConstantIndex m_outerAabbMaxRenderConstantIndex;
            RHI::ShaderInputConstantIndex m_innerAabbMinRenderConstantIndex;
            RHI::ShaderInputConstantIndex m_innerAabbMaxRenderConstantIndex;
            RHI::ShaderInputConstantIndex m_useParallaxCorrectionRenderConstantIndex;
            RHI::ShaderInputImageIndex m_reflectionCubeMapRenderImageIndex;
        };

        // ReflectionProbe manages all aspects of a single probe, including rendering, visualization, and cubemap generation
        class ReflectionProbe final :
            public AZ::Data::AssetBus::MultiHandler
        {
        public:
            ReflectionProbe() = default;
            ~ReflectionProbe();

            void Init(RPI::Scene* scene, ReflectionRenderData* reflectionRenderData);
            void Simulate(uint32_t probeIndex);

            const Vector3& GetPosition() const { return m_transform.GetTranslation(); }
            void SetTransform(const AZ::Transform& transform);

            const AZ::Vector3& GetOuterExtents() const { return m_outerExtents; }
            void SetOuterExtents(const AZ::Vector3& outerExtents);

            const AZ::Vector3& GetInnerExtents() const { return m_innerExtents; }
            void SetInnerExtents(const AZ::Vector3& innerExtents);

            const Aabb& GetOuterAabbWs() const { return m_outerAabbWs; }
            const Aabb& GetInnerAabbWs() const { return m_innerAabbWs; }

            const Data::Instance<RPI::Image>& GetCubeMapImage() const { return m_cubeMapImage; }
            void SetCubeMapImage(const Data::Instance<RPI::Image>& cubeMapImage, const AZStd::string& relativePath);

            const AZStd::string& GetCubeMapRelativePath() const { return m_cubeMapRelativePath; }

            bool GetUseParallaxCorrection() const { return m_useParallaxCorrection; }
            void SetUseParallaxCorrection(bool useParallaxCorrection) { m_useParallaxCorrection = useParallaxCorrection; }

            // initiates the cubemap bake and invokes the callback when all faces of the cubemap are rendered
            void BuildCubeMap(BuildCubeMapCallback callback);
            bool IsBuildingCubeMap() { return m_buildingCubeMap; }

            // called by the feature processor so the probe can set the default view for the pipeline
            void OnRenderPipelinePassesChanged(RPI::RenderPipeline* renderPipeline);

            // enables or disables rendering of the visualization sphere
            void ShowVisualization(bool showVisualization);

        private:

            AZ_DISABLE_COPY_MOVE(ReflectionProbe);

            const RHI::DrawPacket* BuildDrawPacket(
                const Data::Instance<RPI::ShaderResourceGroup>& srg,
                const RPI::Ptr<RPI::PipelineStateForDraw>& pipelineState,
                const RHI::DrawListTag& drawListTag,
                uint32_t stencilRef);

            void UpdateCulling();

            // AZ::Data::AssetBus::Handler overrides...
            void OnAssetReady(Data::Asset<Data::AssetData> asset) override;
            void OnAssetError(Data::Asset<Data::AssetData> asset) override;

            // scene
            RPI::Scene* m_scene = nullptr;

            // probe volume transform
            AZ::Transform m_transform = AZ::Transform::CreateIdentity();

            // extents of the probe volume
            AZ::Vector3 m_outerExtents = AZ::Vector3(0.0f, 0.0f, 0.0f);
            AZ::Vector3 m_innerExtents = AZ::Vector3(0.0f, 0.0f, 0.0f);

            // probe volume AABBs (world space), built from position and extents
            Aabb m_outerAabbWs;
            Aabb m_innerAabbWs;

            // cubemap
            Data::Instance<RPI::Image> m_cubeMapImage;
            AZStd::string m_cubeMapRelativePath;
            bool m_useParallaxCorrection = false;

            // probe visualization
            AZ::Render::MeshFeatureProcessorInterface* m_meshFeatureProcessor = nullptr;
            Data::Asset<RPI::ModelAsset> m_visualizationModelAsset;
            Data::Asset<RPI::MaterialAsset> m_visualizationMaterialAsset;
            AZ::Render::MeshFeatureProcessorInterface::MeshHandle m_visualizationMeshHandle;

            // reflection rendering
            ReflectionRenderData* m_reflectionRenderData = nullptr;
            Data::Instance<RPI::ShaderResourceGroup> m_stencilSrg;
            Data::Instance<RPI::ShaderResourceGroup> m_blendWeightSrg;
            Data::Instance<RPI::ShaderResourceGroup> m_renderOuterSrg;
            Data::Instance<RPI::ShaderResourceGroup> m_renderInnerSrg;
            RHI::ConstPtr<RHI::DrawPacket> m_stencilDrawPacket;
            RHI::ConstPtr<RHI::DrawPacket> m_blendWeightDrawPacket;
            RHI::ConstPtr<RHI::DrawPacket> m_renderOuterDrawPacket;
            RHI::ConstPtr<RHI::DrawPacket> m_renderInnerDrawPacket;
            bool m_updateSrg = false;

            const RHI::DrawItemSortKey InvalidSortKey = static_cast<RHI::DrawItemSortKey>(-1);
            RHI::DrawItemSortKey m_sortKey = InvalidSortKey;

            // culling
            RPI::Cullable m_cullable;

            // probe baking
            RPI::Ptr<RPI::EnvironmentCubeMapPass> m_environmentCubeMapPass = nullptr;
            RPI::RenderPipelineId m_environmentCubeMapPipelineId;
            BuildCubeMapCallback m_callback;
            RHI::ShaderInputNameIndex m_iblExposureConstantIndex = "m_iblExposure";
            float m_previousExposure = 0.0f;
            bool m_buildingCubeMap = false;
        };

    } // namespace Render
} // namespace AZ
