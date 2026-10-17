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
#include <Atom/RHI.Reflect/ImageDescriptor.h>
#include <Atom/RHI.Reflect/ImageViewDescriptor.h>
#include <AzCore/Math/Color.h>

namespace AZ::Render
{
    // =========================================================================
    // GradientGICubemapPass
    //
    // GPU compute pass that writes a procedural gradient into a persistent
    // cubemap AttachmentImage. The pass runs every frame but only recompiles
    // its SRG when gradient parameters change (m_dirty).
    //
    // Dispatch: (ceil(faceSize/8), ceil(faceSize/8), 6 faces)
    // Shader: Shaders/GradientGI/GradientGICubemap.shader
    // =========================================================================
    class GradientGICubemapPass final : public RPI::RenderPass
    {
    public:
        AZ_RTTI(GradientGICubemapPass, "{A2B3C4D5-E6F7-8901-BCDE-F12345678902}", RPI::RenderPass);
        AZ_CLASS_ALLOCATOR(GradientGICubemapPass, AZ::SystemAllocator);

        // =====================================================================
        // Platform Support
        // =====================================================================

        //! Returns true if the current GPU supports UAV writes to cubemaps.
        //! (DX12/Vulkan desktop: yes; Metal/mobile: no)
        static bool IsGpuComputeSupported();

        // =====================================================================
        // Lifecycle
        // =====================================================================

        explicit GradientGICubemapPass(const RPI::PassDescriptor& descriptor);
        ~GradientGICubemapPass() override = default;

        // =====================================================================
        // Public API
        // =====================================================================

        //! Update gradient parameters. Marks the SRG dirty so the next frame
        //! re-dispatches with the new colors.
        void SetGradientColors(
            const Color& low, const Color& mid, const Color& high,
            float exposure, uint32_t faceSize);

        //! Update the detail texture layer. A null texture disables the layer (the gradient
        //! shows unmodified). mapping/blend codes match the component enums; strength is 0..1.
        //! Only updates SRG state -- it never reallocates, so color edits stay cheap.
        void SetDetailLayer(
            const Data::Instance<RPI::Image>& texture, uint8_t mapping, uint8_t blend, float strength);

        //! Update the specular texture layer. Composites into the specular output only; a null
        //! texture means specular == diffuse base. Codes match the component enums; strength 0..1.
        void SetSpecularLayer(
            const Data::Instance<RPI::Image>& texture, uint8_t mapping, uint8_t blend, float strength);

        //! Returns the persistent output cubemaps (valid after BuildInternal succeeds).
        //! Diffuse and specular feed the scene SRG's separate IBL slots.
        Data::Instance<RPI::AttachmentImage> GetDiffuseImage() const;
        Data::Instance<RPI::AttachmentImage> GetSpecularImage() const;

        //! Take over output cubemaps produced by a previous instance of this pass.
        //!
        //! A pipeline pass-tree rebuild discards dynamically injected passes, so the feature
        //! processor hosts a replacement. Carrying the images across avoids reallocating them, and
        //! -- more importantly -- lets the replacement start with the previous contents instead of
        //! an unwritten (black) cubemap that the scene would sample until the first dispatch lands.
        //! Images that do not match the current face size are ignored and rebuilt.
        void AdoptOutputImages(
            const Data::Instance<RPI::AttachmentImage>& diffuse,
            const Data::Instance<RPI::AttachmentImage>& specular);

    protected:
        // =====================================================================
        // RPI::Pass Overrides
        // =====================================================================

        void BuildInternal() override;
        void SetupFrameGraphDependencies(RHI::FrameGraphInterface frameGraph) override;
        void CompileResources(const RHI::FrameGraphCompileContext& context) override;
        void BuildCommandListInternal(const RHI::FrameGraphExecuteContext& context) override;

    private:

        // =====================================================================
        // Shader and Pipeline State
        // =====================================================================

        Data::Instance<RPI::Shader>              m_shader;
        const RHI::PipelineState*                m_pipelineState = nullptr;
        Data::Instance<RPI::ShaderResourceGroup> m_passSrg;

        // =====================================================================
        // Output Images (separate diffuse + specular cubemaps)
        // =====================================================================

        Data::Instance<RPI::AttachmentImage> m_diffuseImage;
        Data::Instance<RPI::AttachmentImage> m_specularImage;

        //! Creates one output cubemap AttachmentImage at m_faceSize. Returns nullptr on failure.
        Data::Instance<RPI::AttachmentImage> CreateOutputCubemap(const char* debugName) const;

        //! True when both output cubemaps exist and were allocated at the current face size.
        //! A resolution change invalidates them: the dispatch grid and the image dimensions must
        //! agree or faces are under-written (stale texels) or overrun (aliasing).
        bool OutputImagesMatchFaceSize() const;

        // =====================================================================
        // Gradient Parameters
        // =====================================================================

        Color    m_lowColor  = Color(0.05f, 0.06f, 0.08f, 1.0f);
        Color    m_midColor  = Color(0.20f, 0.30f, 0.55f, 1.0f);
        Color    m_highColor = Color(0.85f, 0.95f, 1.0f,  1.0f);
        float    m_exposure  = 0.0f;
        uint32_t m_faceSize  = 64;

        // =====================================================================
        // Detail Texture Layer (resident input; recombined per dispatch)
        // =====================================================================

        Data::Instance<RPI::Image> m_detailTexture;       // null = layer disabled
        uint8_t                    m_detailMapping  = 1;  // GradientGITextureMapping (Stretched)
        uint8_t                    m_detailBlend    = 3;  // GradientGIBlendMode (Overlay)
        float                      m_detailStrength = 1.0f;

        Data::Instance<RPI::Image> m_specularTexture;     // null = specular == diffuse base
        uint8_t                    m_specularMapping  = 1; // GradientGITextureMapping (Stretched)
        uint8_t                    m_specularBlend    = 3; // GradientGIBlendMode (Overlay)
        float                      m_specularStrength = 1.0f;

        // 1x1 white cubemap bound to the cube detail slot whenever it is unused (there is no
        // system default cubemap). Created lazily and cached for the pass lifetime.
        Data::Instance<RPI::Image> m_whiteFallbackCube;
        Data::Instance<RPI::Image> GetOrCreateWhiteFallbackCube();

        //! Bind one texture layer's SRG slots (2D + cube + params) during CompileResources.
        //! Picks the 2D or cube slot per the mapping mode, falls back to white for the unused
        //! slot, and disables the layer if Cube is requested with a non-cube texture.
        void BindTextureLayer(
            const AZ::Name& tex2DName, const AZ::Name& texCubeName,
            const AZ::Name& mappingName, const AZ::Name& blendName,
            const AZ::Name& strengthName, const AZ::Name& enabledName,
            const Data::Instance<RPI::Image>& texture, uint8_t mapping, uint8_t blend, float strength);

        // Dirty flag: true means the SRG needs recompile before next dispatch
        bool m_dirty = true;
    };

} // namespace AZ::Render
