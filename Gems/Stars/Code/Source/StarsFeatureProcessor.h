/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Stars/StarsFeatureProcessorInterface.h>

#include <Atom/RPI.Public/Buffer/Buffer.h>
#include <Atom/RPI.Public/PipelineState.h>
#include <Atom/RPI.Public/Shader/ShaderResourceGroup.h>
#include <Atom/RPI.Public/ViewportContextBus.h>

namespace AZ::Render
{
    class Scene;
    class Shader;

    class StarsFeatureProcessor final
        : public StarsFeatureProcessorInterface
        , protected RPI::ViewportContextIdNotificationBus::Handler
        , protected Data::AssetBus::Handler
    {
    public:
        AZ_CLASS_ALLOCATOR(StarsFeatureProcessor, AZ::SystemAllocator)
        AZ_RTTI(AZ::Render::StarsFeatureProcessor, "{34B9EE52-2893-4D02-AC19-8C5DCAFFE608}", AZ::Render::StarsFeatureProcessorInterface);

        static void Reflect(AZ::ReflectContext* context);

        StarsFeatureProcessor() = default;
        virtual ~StarsFeatureProcessor() = default;

        //! FeatureProcessor
        void Activate() override;
        void Deactivate() override;
        void Simulate(const FeatureProcessor::SimulatePacket& packet) override;
        void Render(const FeatureProcessor::RenderPacket& packet) override;

        //! StarsFeatureProcessorInterface
        void SetStars(const AZStd::vector<StarVertex>& starVertexData) override;
        void SetExposure(float exposure) override;
        void SetRadiusFactor(float radius) override;
        void SetOrientation(AZ::Quaternion orientation) override;
        void SetTwinkleRate(float twinkleRate) override;

    protected:
        //! RPI::SceneNotificationBus
        void OnRenderPipelineChanged(RPI::RenderPipeline* pipeline, RPI::SceneNotification::RenderPipelineChangeType changeType) override;

        //! RPI::ViewportContextIdNotificationBus
        void OnViewportSizeChanged(AzFramework::WindowSize size) override;

        //! Data::AssetBus
        void OnAssetReloaded(Data::Asset<Data::AssetData> asset) override;

    private:
        static constexpr const char* FeatureProcessorName = "StarsFeatureProcessor";

        StarsFeatureProcessor(const StarsFeatureProcessor&) = delete;

        void UpdateShaderConstants();
        void UpdateDrawPacket();
        void UpdateBackgroundClearColor();

        //! build a draw packet to draw the star mesh
        RHI::ConstPtr<RHI::DrawPacket> BuildDrawPacket();

        RPI::Ptr<RPI::PipelineStateForDraw> m_meshPipelineState;
        RHI::GeometryView m_geometryView;

        Data::Instance<RPI::ShaderResourceGroup> m_drawSrg = nullptr;
        Data::Instance<RPI::Shader> m_shader = nullptr;

        RHI::ShaderInputNameIndex m_starParamsIndex = "m_starParams";
        RHI::ShaderInputNameIndex m_rotationIndex = "m_rotation";

        Data::Instance<RPI::Buffer> m_starsVertexBuffer = nullptr;
        RHI::DrawListTag m_drawListTag;
        RHI::ConstPtr<RHI::DrawPacket> m_drawPacket;

        bool m_updateShaderConstants = false;
        AZ::Matrix3x3 m_orientation = AZ::Matrix3x3::CreateIdentity();
        float m_exposure = StarsDefaultExposure;
        float m_radiusFactor = StarsDefaultRadiusFactor;

        struct StarShaderConstants
        {
            float m_scaleX = 1.f;
            float m_scaleY = 1.f;
            float m_scaledExposure = StarsDefaultExposure;
            float m_twinkleRate = StarsDefaultTwinkleRate;
        };
        StarShaderConstants m_shaderConstants;
        AZStd::vector<StarVertex> m_starsMeshData;
        uint32_t m_numStarsVertices = 0;
        AzFramework::WindowSize m_viewportSize{0,0};
    };
} // namespace AZ::Render
