/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once


#include <Atom/Feature/Stars/StarsFeatureProcessorInterface.h>

#include <Atom/RPI.Public/Buffer/Buffer.h>
#include <Atom/RPI.Public/PipelineState.h>
#include <Atom/RPI.Public/Shader/ShaderResourceGroup.h>

namespace AZ::Render
{
    class Scene;
    class Shader;

    class StarsFeatureProcessor final
        : public StarsFeatureProcessorInterface
    {
    public:
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
        void Enable(bool enable) override;
        bool IsEnabled() override;
        void SetStars(const AZStd::vector<StarVertex>& starVertexData) override;
        void SetIntensityFactor(float intensityFactor) override;
        void SetRadiusFactor(float radius) override;
        void SetOrientation(AZ::Quaternion orientation) override;

    private:
        static constexpr const char* FeatureProcessorName = "StarsFeatureProcessor";

        StarsFeatureProcessor(const StarsFeatureProcessor&) = delete;

        RPI::Ptr<RPI::PipelineStateForDraw> m_meshPipelineState;
        AZStd::array<AZ::RHI::StreamBufferView, 1> m_meshStreamBufferViews;

        Data::Instance<RPI::ShaderResourceGroup> m_sceneSrg = nullptr;
        Data::Instance<RPI::Shader> m_shader = nullptr;

        RHI::ShaderInputNameIndex m_starsDataBufferIndex = "m_starParams";
        RHI::ShaderInputNameIndex m_starsRotationMatrixIndex = "m_starRotationMatrix";
        RHI::InputStreamLayout m_starsVertexStreamLayout;
        Data::Instance<RPI::Buffer> m_starsVertexBuffer = nullptr;
        RHI::DrawListTag m_drawListTag;

        bool m_enabled = false;
        AZ::Matrix3x3 m_orientation = AZ::Matrix3x3::CreateIdentity();
        float m_intensityFactor = StarsDefaultIntensityFactor;
        float m_radiusFactor = StarsDefaultRadiusFactor;
        AZStd::array<float, 4> m_starsData;
        AZStd::vector<StarVertex> m_starsMeshData;
        uint32_t m_numStarsVertices = 0;
    };
} // namespace AZ::Render
