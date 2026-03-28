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

        //! Returns the persistent output cubemap (valid after BuildInternal succeeds).
        Data::Instance<RPI::AttachmentImage> GetCubemapImage() const;

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
        // Output Image
        // =====================================================================

        Data::Instance<RPI::AttachmentImage> m_cubemapImage;

        // =====================================================================
        // Gradient Parameters
        // =====================================================================

        Color    m_lowColor  = Color(0.05f, 0.06f, 0.08f, 1.0f);
        Color    m_midColor  = Color(0.20f, 0.30f, 0.55f, 1.0f);
        Color    m_highColor = Color(0.85f, 0.95f, 1.0f,  1.0f);
        float    m_exposure  = 0.0f;
        uint32_t m_faceSize  = 64;

        // Dirty flag: true means the SRG needs recompile before next dispatch
        bool m_dirty = true;

        // =====================================================================
        // Diagnostic Flags (log-once guards to avoid per-frame spam)
        // =====================================================================

        bool m_diagnosticLogFrameGraph = true;
        bool m_diagnosticLogDispatch   = true;
    };

} // namespace AZ::Render
