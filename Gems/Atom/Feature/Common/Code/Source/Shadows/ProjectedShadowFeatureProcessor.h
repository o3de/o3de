/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/Feature/Shadows/ProjectedShadowFeatureProcessorInterface.h>
#include <Atom/Feature/Utils/GpuBufferHandler.h>
#include <Atom/Feature/Utils/IndexedDataVector.h>
#include <Atom/Feature/Utils/MultiSparseVector.h>
#include <CoreLights/EsmShadowmapsPass.h>
#include <CoreLights/ProjectedShadowmapsPass.h>

namespace AZ::Render
{
    //! This feature processor handles creation of shadow passes and manages shadow related data. Use AcquireShadow()
    //! to create a new shadow. The ID that is returned from AcquireShadow() corresponds to an index in the
    //! m_projectedShadows and m_projectedFilterParams buffers in the View SRG.
    class ProjectedShadowFeatureProcessor final
        : public ProjectedShadowFeatureProcessorInterface
    {
    public:

        AZ_RTTI(AZ::Render::ProjectedShadowFeatureProcessor, "{02AFA06D-8B37-4D47-91BD-849CAC7FB330}", AZ::Render::ProjectedShadowFeatureProcessorInterface);

        static void Reflect(AZ::ReflectContext* context);

        ProjectedShadowFeatureProcessor() = default;
        virtual ~ProjectedShadowFeatureProcessor() = default;

        // FeatureProcessor overrides ...
        void Activate() override;
        void Deactivate() override;
        void Simulate(const SimulatePacket& packet) override;
        void PrepareViews(const PrepareViewsPacket&, AZStd::vector<AZStd::pair<RPI::PipelineViewTag, RPI::ViewPtr>>&) override;
        void Render(const RenderPacket& packet) override;

        // ProjectedShadowFeatureProcessorInterface overrides ...
        ShadowId AcquireShadow() override;
        void ReleaseShadow(ShadowId id) override;
        void SetShadowTransform(ShadowId id, Transform transform) override;
        void SetNearFarPlanes(ShadowId id, float nearPlaneDistance, float farPlaneDistance) override;
        void SetAspectRatio(ShadowId id, float aspectRatio) override;
        void SetFieldOfViewY(ShadowId id, float fieldOfViewYRadians) override;
        void SetShadowmapMaxResolution(ShadowId id, ShadowmapSize size) override;
        void SetShadowBias(ShadowId id, float bias) override;
        void SetNormalShadowBias(ShadowId id, float normalShadowBias) override;
        void SetShadowFilterMethod(ShadowId id, ShadowFilterMethod method) override;
        void SetFilteringSampleCount(ShadowId id, uint16_t count) override;
        void SetShadowProperties(ShadowId id, const ProjectedShadowDescriptor& descriptor) override;
        const ProjectedShadowDescriptor& GetShadowProperties(ShadowId id) override;

        void SetEsmExponent(ShadowId id, float exponent);

    private:

        // GPU data stored in m_projectedShadows.
        struct ShadowData
        {
            Matrix4x4 m_depthBiasMatrix = Matrix4x4::CreateIdentity();
            uint32_t m_shadowmapArraySlice = 0; // array slice who has shadowmap in the atlas.
            uint32_t m_shadowFilterMethod = 0; // filtering method of shadows.
            float m_boundaryScale = 0.f; // the half of boundary of lit/shadowed areas. (in degrees)
            uint32_t m_filteringSampleCount = 0;
            AZStd::array<float, 2> m_unprojectConstants = { {0, 0} };
            float m_bias;
            float m_normalShadowBias;
            float m_esmExponent = 87.0f;
            float m_padding[3];
        };

        // CPU data used for constructing & updating ShadowData
        struct ShadowProperty
        {
            ProjectedShadowDescriptor m_desc;
            RPI::ViewPtr m_shadowmapView;
            float m_bias = 0.1f;
            float m_normalShadowBias = 0.0f;
            ShadowId m_shadowId;
        };

        using FilterParameter = EsmShadowmapsPass::FilterParameter;
        static constexpr float MinimumFieldOfView = 0.001f;

        // RPI::SceneNotificationBus::Handler overrides...
        void OnRenderPipelinePassesChanged(RPI::RenderPipeline* renderPipeline) override;
        void OnRenderPipelineAdded(RPI::RenderPipelinePtr pipeline) override;
        void OnRenderPipelineRemoved(RPI::RenderPipeline* pipeline) override;
        
        // Shadow specific functions
        void UpdateShadowView(ShadowProperty& shadowProperty);
        void InitializeShadow(ShadowId shadowId);
            
        // Functions for caching the ProjectedShadowmapsPass and EsmShadowmapsPass.
        void CachePasses();
        void CacheProjectedShadowmapsPass();
        void CacheEsmShadowmapsPass();
            
        //! Functions to update the parameter of Gaussian filter used in ESM.
        void UpdateFilterParameters();
        void UpdateEsmPassEnabled();
        void SetFilterParameterToPass();
        bool FilterMethodIsEsm(const ShadowData& shadowData) const;

        ShadowProperty& GetShadowPropertyFromShadowId(ShadowId id);

        GpuBufferHandler m_shadowBufferHandler; // For ViewSRG m_projectedShadows
        GpuBufferHandler m_filterParamBufferHandler; // For ViewSRG m_projectedFilterParams

        // Stores CPU side shadow information in a packed vector so it's easy to iterate through.
        IndexedDataVector<ShadowProperty> m_shadowProperties;

        // Used for easier indexing of m_shadowData
        enum
        {
            ShadowDataIndex,
            FilterParamIndex,
            ShadowPropertyIdIndex,
        };

        // Stores GPU data that is pushed to buffers in the View SRG. ShadowData corresponds to m_projectedShadows and
        // FilterParameter corresponds to m_projectedFilterParams. The uint16_t is used to reference data in
        // m_shadowProperties.
        MultiSparseVector<ShadowData, FilterParameter, uint16_t> m_shadowData;

        AZStd::vector<ProjectedShadowmapsPass*> m_projectedShadowmapsPasses;
        AZStd::vector<EsmShadowmapsPass*> m_esmShadowmapsPasses;

        RHI::ShaderInputConstantIndex m_shadowmapAtlasSizeIndex;
        RHI::ShaderInputConstantIndex m_invShadowmapAtlasSizeIndex;

        bool m_deviceBufferNeedsUpdate = false;
        bool m_shadowmapPassNeedsUpdate = true;
        bool m_filterParameterNeedsUpdate = false;
    };
}
