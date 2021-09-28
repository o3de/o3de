/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/DrawPacketBuilder.h>
#include <Atom/RPI.Public/Culling.h>
#include <Atom/RPI.Public/PipelineState.h>
#include <Atom/RPI.Public/Scene.h>
#include <AzCore/Math/Random.h>
#include <AzCore/Math/Aabb.h>
#include <DiffuseGlobalIllumination/DiffuseProbeGridTextureReadback.h>

namespace AZ
{
    namespace Render
    {
        class DiffuseProbeGridFeatureProcessor;

        struct DiffuseProbeGridRenderData
        {
            // [GFX TODO][ATOM-15650] Change DiffuseProbeGrid Classification texture to R8_UINT
            static const RHI::Format RayTraceImageFormat = RHI::Format::R32G32B32A32_FLOAT;
            static const RHI::Format IrradianceImageFormat = RHI::Format::R16G16B16A16_UNORM;
            static const RHI::Format DistanceImageFormat = RHI::Format::R32G32_FLOAT;
            static const RHI::Format RelocationImageFormat = RHI::Format::R16G16B16A16_FLOAT;
            static const RHI::Format ClassificationImageFormat = RHI::Format::R32_FLOAT;

            // image pool
            RHI::Ptr<RHI::ImagePool> m_imagePool;

            AZStd::array<RHI::StreamBufferView, 1> m_boxPositionBufferView;
            RHI::IndexBufferView m_boxIndexBufferView;
            uint32_t m_boxIndexCount = 0;

            // image views
            RHI::ImageViewDescriptor m_probeRayTraceImageViewDescriptor;
            RHI::ImageViewDescriptor m_probeIrradianceImageViewDescriptor;
            RHI::ImageViewDescriptor m_probeDistanceImageViewDescriptor;
            RHI::ImageViewDescriptor m_probeRelocationImageViewDescriptor;
            RHI::ImageViewDescriptor m_probeClassificationImageViewDescriptor;

            // render pipeline state
            RPI::Ptr<RPI::PipelineStateForDraw> m_pipelineState;

            // For the render Srg
            Data::Instance<RPI::Shader> m_shader;
            RHI::Ptr<RHI::ShaderResourceGroupLayout> m_srgLayout;

            // render drawlist tag
            RHI::DrawListTag m_drawListTag;
        };

        //! This class manages contains the functionality necessary to update diffuse probes and
        //! generate diffuse global illumination.
        class DiffuseProbeGrid final
        {
        public:
            DiffuseProbeGrid();
            ~DiffuseProbeGrid();

            void Init(RPI::Scene* scene, DiffuseProbeGridRenderData* diffuseProbeGridRenderData);
            void Simulate(uint32_t probeIndex);

            void SetTransform(const AZ::Transform& transform);

            bool ValidateExtents(const AZ::Vector3& newExtents);
            const AZ::Vector3& GetExtents() const { return m_extents; }
            void SetExtents(const AZ::Vector3& extents);

            const AZ::Obb& GetObbWs() const { return m_obbWs; }

            bool ValidateProbeSpacing(const AZ::Vector3& newSpacing);
            const AZ::Vector3& GetProbeSpacing() const { return m_probeSpacing; }
            void SetProbeSpacing(const AZ::Vector3& probeSpacing);

            float GetNormalBias() const { return m_normalBias; }
            void SetNormalBias(float normalBias);

            float GetViewBias() const { return m_viewBias; }
            void SetViewBias(float viewBias);

            float GetAmbientMultiplier() const { return m_ambientMultiplier; }
            void SetAmbientMultiplier(float ambientMultiplier);

            void Enable(bool enabled);

            bool GetGIShadows() const { return m_giShadows; }
            void SetGIShadows(bool giShadows) { m_giShadows = giShadows; }

            bool GetUseDiffuseIbl() const { return m_useDiffuseIbl; }
            void SetUseDiffuseIbl(bool useDiffuseIbl) { m_useDiffuseIbl = useDiffuseIbl; }

            DiffuseProbeGridMode GetMode() const { return m_mode; }
            void SetMode(DiffuseProbeGridMode mode);

            uint32_t GetNumRaysPerProbe() const { return m_numRaysPerProbe; }

            uint32_t GetRemainingRelocationIterations() const { return aznumeric_cast<uint32_t>(m_remainingRelocationIterations); }
            void DecrementRemainingRelocationIterations() { m_remainingRelocationIterations = AZStd::max(0, m_remainingRelocationIterations - 1); }
            void ResetRemainingRelocationIterations() { m_remainingRelocationIterations = DefaultNumRelocationIterations; }

            void ResetCullingVisibility();
            bool GetIsVisible() const;

            // compute total number of probes in the grid
            uint32_t GetTotalProbeCount() const;

            // compute probe counts for a 2D texture layout
            void GetTexture2DProbeCount(uint32_t& probeCountX, uint32_t& probeCountY) const;

            // apply probe grid settings to a Srg
            void SetGridConstants(Data::Instance<RPI::ShaderResourceGroup>& srg);            

            // Srgs
            const Data::Instance<RPI::ShaderResourceGroup>& GetRayTraceSrg() const { return m_rayTraceSrg; }
            const Data::Instance<RPI::ShaderResourceGroup>& GetBlendIrradianceSrg() const { return m_blendIrradianceSrg; }
            const Data::Instance<RPI::ShaderResourceGroup>& GetBlendDistanceSrg() const { return m_blendDistanceSrg; }
            const Data::Instance<RPI::ShaderResourceGroup>& GetBorderUpdateRowIrradianceSrg() const { return m_borderUpdateRowIrradianceSrg; }
            const Data::Instance<RPI::ShaderResourceGroup>& GetBorderUpdateColumnIrradianceSrg() const { return m_borderUpdateColumnIrradianceSrg; }
            const Data::Instance<RPI::ShaderResourceGroup>& GetBorderUpdateRowDistanceSrg() const { return m_borderUpdateRowDistanceSrg; }
            const Data::Instance<RPI::ShaderResourceGroup>& GetBorderUpdateColumnDistanceSrg() const { return m_borderUpdateColumnDistanceSrg; }
            const Data::Instance<RPI::ShaderResourceGroup>& GetRelocationSrg() const { return m_relocationSrg; }
            const Data::Instance<RPI::ShaderResourceGroup>& GetClassificationSrg() const { return m_classificationSrg; }
            const Data::Instance<RPI::ShaderResourceGroup>& GetRenderObjectSrg() const { return m_renderObjectSrg; }

            // Srg updates
            void UpdateRayTraceSrg(const Data::Instance<RPI::Shader>& shader, const RHI::Ptr<RHI::ShaderResourceGroupLayout>& srgLayout);
            void UpdateBlendIrradianceSrg(const Data::Instance<RPI::Shader>& shader, const RHI::Ptr<RHI::ShaderResourceGroupLayout>& srgLayout);
            void UpdateBlendDistanceSrg(const Data::Instance<RPI::Shader>& shader, const RHI::Ptr<RHI::ShaderResourceGroupLayout>& srgLayout);
            void UpdateBorderUpdateSrgs(const Data::Instance<RPI::Shader>& rowShader, const RHI::Ptr<RHI::ShaderResourceGroupLayout>& rowSrgLayout,
                                        const Data::Instance<RPI::Shader>& columnShader, const RHI::Ptr<RHI::ShaderResourceGroupLayout>& columnSrgLayout);
            void UpdateRelocationSrg(const Data::Instance<RPI::Shader>& shader, const RHI::Ptr<RHI::ShaderResourceGroupLayout>& srgLayout);
            void UpdateClassificationSrg(const Data::Instance<RPI::Shader>& shader, const RHI::Ptr<RHI::ShaderResourceGroupLayout>& srgLayout);
            void UpdateRenderObjectSrg();

            // textures
            const RHI::Ptr<RHI::Image> GetRayTraceImage() { return m_rayTraceImage[m_currentImageIndex]; }
            const RHI::Ptr<RHI::Image> GetIrradianceImage() { return m_mode == DiffuseProbeGridMode::RealTime ? m_irradianceImage[m_currentImageIndex] : m_bakedIrradianceImage->GetRHIImage(); }
            const RHI::Ptr<RHI::Image> GetDistanceImage() { return m_mode == DiffuseProbeGridMode::RealTime ? m_distanceImage[m_currentImageIndex] : m_bakedDistanceImage->GetRHIImage(); }
            const RHI::Ptr<RHI::Image> GetRelocationImage() { return m_mode == DiffuseProbeGridMode::RealTime ? m_relocationImage[m_currentImageIndex] : m_bakedRelocationImage; }
            const RHI::Ptr<RHI::Image> GetClassificationImage() { return m_mode == DiffuseProbeGridMode::RealTime ? m_classificationImage[m_currentImageIndex] : m_bakedClassificationImage; }

            const AZStd::string& GetBakedIrradianceRelativePath() const { return m_bakedIrradianceRelativePath; }
            const AZStd::string& GetBakedDistanceRelativePath() const { return m_bakedDistanceRelativePath; }
            const AZStd::string& GetBakedRelocationRelativePath() const { return m_bakedRelocationRelativePath; }
            const AZStd::string& GetBakedClassificationRelativePath() const { return m_bakedClassificationRelativePath; }

            // attachment Ids
            const RHI::AttachmentId GetRayTraceImageAttachmentId() const { return m_rayTraceImageAttachmentId; }
            const RHI::AttachmentId GetIrradianceImageAttachmentId() const { return m_irradianceImageAttachmentId; }
            const RHI::AttachmentId GetDistanceImageAttachmentId() const { return m_distanceImageAttachmentId; }
            const RHI::AttachmentId GetRelocationImageAttachmentId() const { return m_relocationImageAttachmentId; }
            const RHI::AttachmentId GetClassificationImageAttachmentId() const { return m_classificationImageAttachmentId; }

            const DiffuseProbeGridRenderData* GetRenderData() const { return m_renderData; }

            // the irradiance image needs to be manually cleared after it is resized in the editor
            bool GetIrradianceClearRequired() const { return m_irradianceClearRequired; }
            void ResetIrradianceClearRequired() { m_irradianceClearRequired = false; }

            // texture readback
            DiffuseProbeGridTextureReadback& GetTextureReadback() { return m_textureReadback; }

            void SetBakedTextures(const DiffuseProbeGridBakedTextures& bakedTextures);
            bool HasValidBakedTextures() const;

            static constexpr uint32_t DefaultNumIrradianceTexels = 6;
            static constexpr uint32_t DefaultNumDistanceTexels = 14;
            static constexpr int32_t DefaultNumRelocationIterations = 100;

        private:
            void UpdateTextures();
            void ComputeProbeCount(const AZ::Vector3& extents, const AZ::Vector3& probeSpacing, uint32_t& probeCountX, uint32_t& probeCountY, uint32_t& probeCountZ);
            bool ValidateProbeCount(const AZ::Vector3& extents, const AZ::Vector3& probeSpacing);
            void UpdateProbeCount();
            void UpdateCulling();

            // scene
            RPI::Scene* m_scene = nullptr;

            // probe grid transform
            AZ::Transform m_transform = AZ::Transform::CreateIdentity();

            // extents of the probe grid
            AZ::Vector3 m_extents = AZ::Vector3(0.0f, 0.0f, 0.0f);

            // probe grid OBB (world space), built from transform and extents
            AZ::Obb m_obbWs;

            // per-axis spacing of probes in the grid
            AZ::Vector3 m_probeSpacing;

            // per-axis number of probes in the grid
            uint32_t m_probeCountX = 0;
            uint32_t m_probeCountY = 0;
            uint32_t m_probeCountZ = 0;

            // grid settings
            bool     m_enabled = true;
            float    m_normalBias = 0.6f;
            float    m_viewBias = 0.01f;
            uint32_t m_numRaysPerProbe = 288;
            float    m_probeMaxRayDistance = 30.0f;
            float    m_probeDistanceExponent = 50.0f;
            float    m_probeHysteresis = 0.95f;
            float    m_probeChangeThreshold = 0.2f;
            float    m_probeBrightnessThreshold = 1.0f;
            float    m_probeIrradianceEncodingGamma = 5.0f;
            float    m_probeInverseIrradianceEncodingGamma = 1.0f / m_probeIrradianceEncodingGamma;
            float    m_probeMinFrontfaceDistance = 1.0f;
            float    m_probeBackfaceThreshold = 0.25f;
            float    m_ambientMultiplier = 1.0f;
            bool     m_giShadows = true;
            bool     m_useDiffuseIbl = true;

            // rotation transform applied to probe rays
            AZ::Matrix4x4 m_probeRayRotationTransform;
            AZ::SimpleLcgRandom m_random;

            // probe relocation settings
            int32_t m_remainingRelocationIterations = DefaultNumRelocationIterations;

            // render data
            DiffuseProbeGridRenderData* m_renderData = nullptr;

            // render draw packet
            RHI::ConstPtr<RHI::DrawPacket> m_drawPacket;

            // sort key for the draw item
            const RHI::DrawItemSortKey InvalidSortKey = static_cast<RHI::DrawItemSortKey>(-1);
            RHI::DrawItemSortKey m_sortKey = InvalidSortKey;

            // culling
            RPI::Cullable m_cullable;

            // grid mode (RealTime or Baked)
            DiffuseProbeGridMode m_mode = DiffuseProbeGridMode::RealTime;

            // real-time textures
            static const uint32_t MaxTextureDimension = 8192;
            static const uint32_t ImageFrameCount = 3;
            RHI::Ptr<RHI::Image> m_rayTraceImage[ImageFrameCount];
            RHI::Ptr<RHI::Image> m_irradianceImage[ImageFrameCount];
            RHI::Ptr<RHI::Image> m_distanceImage[ImageFrameCount];
            RHI::Ptr<RHI::Image> m_relocationImage[ImageFrameCount];
            RHI::Ptr<RHI::Image> m_classificationImage[ImageFrameCount];
            uint32_t m_currentImageIndex = 0;
            bool m_updateTextures = false;
            bool m_irradianceClearRequired = true;

            // baked textures
            Data::Instance<RPI::Image> m_bakedIrradianceImage;
            Data::Instance<RPI::Image> m_bakedDistanceImage;
            RHI::Ptr<RHI::Image> m_bakedRelocationImage;
            RHI::Ptr<RHI::Image> m_bakedClassificationImage;

            // baked texture relative paths
            AZStd::string m_bakedIrradianceRelativePath;
            AZStd::string m_bakedDistanceRelativePath;
            AZStd::string m_bakedRelocationRelativePath;
            AZStd::string m_bakedClassificationRelativePath;

            // baked texture data (only needed for the relocation and classification textures)
            AZStd::vector<uint8_t> m_bakedRelocationImageData;
            AZStd::vector<uint8_t> m_bakedClassificationImageData;

            // texture readback
            DiffuseProbeGridTextureReadback m_textureReadback;

            // Srgs
            Data::Instance<RPI::ShaderResourceGroup> m_rayTraceSrg;
            Data::Instance<RPI::ShaderResourceGroup> m_blendIrradianceSrg;
            Data::Instance<RPI::ShaderResourceGroup> m_blendDistanceSrg;
            Data::Instance<RPI::ShaderResourceGroup> m_borderUpdateRowIrradianceSrg;
            Data::Instance<RPI::ShaderResourceGroup> m_borderUpdateColumnIrradianceSrg;
            Data::Instance<RPI::ShaderResourceGroup> m_borderUpdateRowDistanceSrg;
            Data::Instance<RPI::ShaderResourceGroup> m_borderUpdateColumnDistanceSrg;
            Data::Instance<RPI::ShaderResourceGroup> m_relocationSrg;
            Data::Instance<RPI::ShaderResourceGroup> m_classificationSrg;
            Data::Instance<RPI::ShaderResourceGroup> m_renderObjectSrg;
            bool m_updateRenderObjectSrg = true;

            // attachment Ids
            RHI::AttachmentId m_rayTraceImageAttachmentId;
            RHI::AttachmentId m_irradianceImageAttachmentId;
            RHI::AttachmentId m_distanceImageAttachmentId;
            RHI::AttachmentId m_relocationImageAttachmentId;
            RHI::AttachmentId m_classificationImageAttachmentId;
        };
    }   // namespace Render
}   // namespace AZ
