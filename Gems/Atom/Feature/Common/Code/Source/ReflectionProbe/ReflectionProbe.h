/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Math/Transform.h>
#include <Atom/Feature/RenderCommon.h>
#include <Atom/Feature/Mesh/MeshFeatureProcessorInterface.h>
#include <Atom/RHI/GeometryView.h>
#include <Atom/RPI.Public/Base.h>
#include <Atom/RPI.Public/Model/Model.h>
#include <Atom/RPI.Public/MeshDrawPacket.h>
#include <Atom/RPI.Public/Pass/Specific/EnvironmentCubeMapPass.h>
#include <Atom/RPI.Public/PipelineState.h>
#include <Atom/RPI.Public/Shader/ShaderResourceGroup.h>
#include <Atom/RPI.Public/Scene.h>
#include <CubeMapCapture/CubeMapRenderer.h>

namespace AZ
{
    namespace Render
    {
        // shared data for rendering reflections, loaded and stored by the ReflectionProbeFeatureProcessor and passed to all probes
        struct ReflectionRenderData
        {
            RHI::GeometryView m_geometryView;

            RPI::Ptr<RPI::PipelineStateForDraw> m_stencilPipelineState;
            RPI::Ptr<RPI::PipelineStateForDraw> m_blendWeightPipelineState;
            RPI::Ptr<RPI::PipelineStateForDraw> m_renderOuterPipelineState;
            RPI::Ptr<RPI::PipelineStateForDraw> m_renderInnerPipelineState;

            Data::Instance<RPI::Shader> m_stencilShader;
            RHI::Ptr<RHI::ShaderResourceGroupLayout> m_stencilSrgLayout;

            Data::Instance<RPI::Shader> m_blendWeightShader;
            RHI::Ptr<RHI::ShaderResourceGroupLayout> m_blendWeightSrgLayout;

            Data::Instance<RPI::Shader> m_renderOuterShader;
            RHI::Ptr<RHI::ShaderResourceGroupLayout> m_renderOuterSrgLayout;

            Data::Instance<RPI::Shader> m_renderInnerShader;
            RHI::Ptr<RHI::ShaderResourceGroupLayout> m_renderInnerSrgLayout;

            RHI::DrawListTag m_stencilDrawListTag;
            RHI::DrawListTag m_blendWeightDrawListTag;
            RHI::DrawListTag m_renderOuterDrawListTag;
            RHI::DrawListTag m_renderInnerDrawListTag;

            RHI::ShaderInputNameIndex m_modelToWorldStencilConstantIndex = "m_modelToWorld";
            RHI::ShaderInputNameIndex m_modelToWorldRenderConstantIndex = "m_modelToWorld";
            RHI::ShaderInputNameIndex m_modelToWorldInverseRenderConstantIndex = "m_modelToWorldInverse";
            RHI::ShaderInputNameIndex m_outerObbHalfLengthsRenderConstantIndex = "m_outerObbHalfLengths";
            RHI::ShaderInputNameIndex m_innerObbHalfLengthsRenderConstantIndex = "m_innerObbHalfLengths";
            RHI::ShaderInputNameIndex m_useParallaxCorrectionRenderConstantIndex = "m_useParallaxCorrection";
            RHI::ShaderInputNameIndex m_exposureConstantIndex = "m_exposure";
            RHI::ShaderInputNameIndex m_reflectionCubeMapRenderImageIndex = "m_reflectionCubeMap";
        };

        // ReflectionProbe manages all aspects of a single probe, including rendering, visualization, and cubemap generation
        class ReflectionProbe final
            : public AZ::Data::AssetBus::Handler
            , private CubeMapRenderer
        {
        public:
            ReflectionProbe() = default;
            ~ReflectionProbe();

            void Init(RPI::Scene* scene, ReflectionRenderData* reflectionRenderData);
            void Simulate(uint32_t probeIndex);
            void OnRenderEnd();

            const Vector3& GetPosition() const { return m_transform.GetTranslation(); }
            const AZ::Transform& GetTransform() const { return m_transform; }
            void SetTransform(const AZ::Transform& transform);

            const AZ::Vector3& GetOuterExtents() const { return m_outerExtents; }
            void SetOuterExtents(const AZ::Vector3& outerExtents);

            const AZ::Vector3& GetInnerExtents() const { return m_innerExtents; }
            void SetInnerExtents(const AZ::Vector3& innerExtents);

            const Obb& GetOuterObbWs() const { return m_outerObbWs; }
            const Obb& GetInnerObbWs() const { return m_innerObbWs; }

            const Data::Instance<RPI::Image>& GetCubeMapImage() const { return m_cubeMapImage; }
            void SetCubeMapImage(const Data::Instance<RPI::Image>& cubeMapImage, const AZStd::string& relativePath);

            const AZStd::string& GetCubeMapRelativePath() const { return m_cubeMapRelativePath; }

            bool GetUseParallaxCorrection() const { return m_useParallaxCorrection; }
            void SetUseParallaxCorrection(bool useParallaxCorrection) { m_useParallaxCorrection = useParallaxCorrection; }

            const AZ::Uuid& GetUuid() const { return m_uuid; }

            // initiates the reflection probe bake and invokes the callback when the cubemap is finished rendering
            void Bake(RenderCubeMapCallback callback);

            // called by the feature processor so the probe can set the default view for the pipeline
            void OnRenderPipelinePassesChanged(RPI::RenderPipeline* renderPipeline);

            // enables or disables rendering of the visualization sphere
            void ShowVisualization(bool showVisualization);

            // the exposure to use when rendering meshes with this probe's cubemap
            void SetRenderExposure(float renderExposure);
            float GetRenderExposure() const { return m_renderExposure; }

            // the exposure to use when baking the probe cubemap
            void SetBakeExposure(float bakeExposure);
            float GetBakeExposure() const { return m_bakeExposure; }

        private:

            AZ_DISABLE_COPY_MOVE(ReflectionProbe);

            RHI::ConstPtr<RHI::DrawPacket> BuildDrawPacket(
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

            // probe volume OBBs (world space), built from position and extents
            Obb m_outerObbWs;
            Obb m_innerObbWs;

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
            float m_renderExposure = 0.0f;
            float m_bakeExposure = 0.0f;
            bool m_updateSrg = false;

            const RHI::DrawItemSortKey InvalidSortKey = static_cast<RHI::DrawItemSortKey>(-1);
            RHI::DrawItemSortKey m_sortKey = InvalidSortKey;

            // culling
            RPI::Cullable m_cullable;
            AZ::Uuid m_uuid = AZ::Uuid::Create();
        };

    } // namespace Render
} // namespace AZ
