/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/ScopeProducer.h>
#include <Atom/RPI.Public/Pass/Pass.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <ffx_fsr2.h>

namespace AZ::Render
{
    struct Fsr2TaaUpscalePassData : public RPI::PassData
    {
        AZ_RTTI(Fsr2TaaUpscalePassData, "{68B8A0CD-DB11-47F2-BC78-4E730C65EA73}", RPI::PassData);
        AZ_CLASS_ALLOCATOR(Fsr2TaaUpscalePassData, SystemAllocator, 0);

        static void Reflect(ReflectContext* context);

        ~Fsr2TaaUpscalePassData() override = default;

        bool m_depthInverted = true;
        bool m_depthInfinite = true;
        bool m_autoExposure = true;
        bool m_dynamicResolution = true;
    };

    class Fsr2TaaUpscalePass
        : public RPI::Pass
        , public RHI::ScopeProducer
    {
        AZ_RPI_PASS(Fsr2TaaUpscalePass);

    public:
        AZ_RTTI(Fsr2TaaUpscalePass, "{43E97DDD-BE91-4DEA-B942-D7780D75894B}", RPI::Pass);
        AZ_CLASS_ALLOCATOR(Fsr2TaaUpscalePass, SystemAllocator, 0);

        ~Fsr2TaaUpscalePass() override;

        static RPI::Ptr<Fsr2TaaUpscalePass> Create(const RPI::PassDescriptor& descriptor);

    private:
        Fsr2TaaUpscalePass(const RPI::PassDescriptor& descriptor);

        // RPI::Pass overrides
        void ResetInternal() override;
        void BuildInternal() override;
        void FrameBeginInternal(FramePrepareParams params) override;

        void MaybeCreateFsr2Context();

        // RHI::ScopeProducer overrides
        void SetupFrameGraphDependencies(RHI::FrameGraphInterface frameGraph) override;
        void CompileResources(const RHI::FrameGraphCompileContext& context) override;
        void BuildCommandList(const RHI::FrameGraphExecuteContext& context) override;

        bool IsFsr2ContextValid() const;

        // Pass attachments
        RPI::PassAttachmentBinding* m_inputColor = nullptr;
        RPI::PassAttachmentBinding* m_inputDepth = nullptr;
        RPI::PassAttachmentBinding* m_inputMotionVectors = nullptr;
        RPI::PassAttachmentBinding* m_outputColor = nullptr;

        // State needed to submit an FSR2 dispatch
        FfxFsr2ContextDescription m_fsr2ContextDesc{};
        FfxFsr2Context m_fsr2Context{};
        FfxFsr2DispatchDescription m_fsr2DispatchDesc{};
        uint32_t m_frameCount = 0u;
        double m_lastFrameTimeMS = 0.0;
    };
} // namespace AZ::Render

