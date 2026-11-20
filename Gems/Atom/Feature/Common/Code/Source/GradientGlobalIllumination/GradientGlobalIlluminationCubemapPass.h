/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/RPI.Public/Pass/RenderPass.h>
#include <Atom/RPI.Public/Image/AttachmentImage.h>
#include <Atom/RPI.Public/Image/ImageSystemInterface.h>
#include <Atom/RPI.Public/Shader/Shader.h>
#include <Atom/RPI.Public/Shader/ShaderResourceGroup.h>
#include <Atom/RHI/PipelineState.h>
#include <Atom/RHI/ScopeProducer.h>
#include <Atom/RHI.Reflect/ImageDescriptor.h>
#include <Atom/RHI.Reflect/ImageViewDescriptor.h>
#include <AzCore/Math/Color.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>

namespace AZ::Render
{
    //! Composites diffuse and specular cubemaps when inputs change.
    //! A following scope declares shader-read access because scene SRG bindings are invisible to the frame graph.
    class GradientGlobalIlluminationCubemapPass final
        : public RPI::RenderPass
    {
    public:
        AZ_RTTI(GradientGlobalIlluminationCubemapPass, "{A2B3C4D5-E6F7-8901-BCDE-F12345678902}", RPI::RenderPass);
        AZ_CLASS_ALLOCATOR(GradientGlobalIlluminationCubemapPass, AZ::SystemAllocator);

        //! Probe writable RGBA16F cubemap allocation in the current attachment pool.
        static bool IsGpuComputeSupported();

        explicit GradientGlobalIlluminationCubemapPass(const RPI::PassDescriptor& descriptor);
        ~GradientGlobalIlluminationCubemapPass() override = default;

        void SetGradientColors(const Color& low, const Color& mid, const Color& high, uint32_t faceSize);

        //! Set the detail layer. Null disables it. Mapping/blend codes match the component enums.
        void SetDetailLayer(const Data::Instance<RPI::Image>& texture, uint8_t mapping, uint8_t blend, float strength);

        //! Set the specular layer. Null uses the diffuse base. Strength is a blend weight in [0, 1].
        void SetSpecularLayer(const Data::Instance<RPI::Image>& texture, uint8_t mapping, uint8_t blend, float strength);

        //! Output images are allocated by BuildInternal. Check HasWrittenOutputs before publishing them.
        Data::Instance<RPI::AttachmentImage> GetDiffuseImage() const;
        Data::Instance<RPI::AttachmentImage> GetSpecularImage() const;

        //! Adopt initialized images from the preceding pass. Size mismatches are rebuilt.
        void AdoptOutputImages(const Data::Instance<RPI::AttachmentImage>& diffuse, const Data::Instance<RPI::AttachmentImage>& specular);

        //! True for adopted output or a scheduled write. This is not a GPU completion fence.
        bool HasWrittenOutputs() const;

        bool HasBuildFailed() const
        {
            return m_buildFailed;
        }

    protected:
        void BuildInternal() override;
        void FrameBeginInternal(FramePrepareParams params) override;
        void SetupFrameGraphDependencies(RHI::FrameGraphInterface frameGraph) override;
        void CompileResources(const RHI::FrameGraphCompileContext& context) override;
        void BuildCommandListInternal(const RHI::FrameGraphExecuteContext& context) override;

    private:
        Data::Instance<RPI::AttachmentImage> CreateOutputCubemap(const AZ::Name& debugName) const;

        //! Check both image dimensions before reusing them for a resized dispatch.
        bool OutputImagesMatchFaceSize() const;

        //! Declare output reads that are not tracked through the scene SRG.
        void DeclareSampleAccess(RHI::FrameGraphInterface frameGraph);

        Data::Instance<RPI::Image> GetOrCreateWhiteFallbackCube();

        //! Bind both SRG image slots and disable a layer whose texture does not match its mapping.
        void BindTextureLayer(
            const AZ::Name& tex2DName,
            const AZ::Name& texCubeName,
            const AZ::Name& mappingName,
            const AZ::Name& blendName,
            const AZ::Name& strengthName,
            const AZ::Name& enabledName,
            const Data::Instance<RPI::Image>& texture,
            uint8_t mapping,
            uint8_t blend,
            float strength);

    private:
        Data::Instance<RPI::Shader> m_shader;
        const RHI::PipelineState* m_pipelineState = nullptr;
        Data::Instance<RPI::ShaderResourceGroup> m_passSrg;

        Data::Instance<RPI::AttachmentImage> m_diffuseImage;
        Data::Instance<RPI::AttachmentImage> m_specularImage;

        //! Orders shader-read access after compute writes on frames that update output.
        AZStd::shared_ptr<RHI::ScopeProducer> m_sampleAccessScope;

        Color m_lowColor = Color(0.05f, 0.06f, 0.08f, 1.0f);
        Color m_midColor = Color(0.20f, 0.30f, 0.55f, 1.0f);
        Color m_highColor = Color(0.85f, 0.95f, 1.0f, 1.0f);
        uint32_t m_faceSize = 64;

        Data::Instance<RPI::Image> m_detailTexture; // null = layer disabled
        uint8_t m_detailMapping = 1; // GradientGlobalIlluminationTextureMapping (Stretched)
        uint8_t m_detailBlend = 3; // GradientGlobalIlluminationBlendMode (Overlay)
        float m_detailStrength = 1.0f;

        Data::Instance<RPI::Image> m_specularTexture; // null = specular == diffuse base
        uint8_t m_specularMapping = 1; // GradientGlobalIlluminationTextureMapping (Stretched)
        uint8_t m_specularBlend = 3; // GradientGlobalIlluminationBlendMode (Overlay)
        float m_specularStrength = 1.0f;

        // Both texture slots need valid SRVs, including when a layer is disabled.
        Data::Instance<RPI::Image> m_whiteFallbackCube;

        bool m_dirty = true;
        bool m_buildFailed = false;

        // Keep frame-graph declarations and dispatch decisions consistent for the entire frame.
        bool m_writeThisFrame = false;

        bool m_outputsWritten = false;
    };
} // namespace AZ::Render
