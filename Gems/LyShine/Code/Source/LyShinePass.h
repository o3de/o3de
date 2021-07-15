/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RPI.Public/Pass/ParentPass.h>
#include <Atom/RPI.Public/Pass/RasterPass.h>
#include <AtomCore/std/containers/array_view.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/containers/vector.h>
#include "LyShinePassDataBus.h"

namespace LyShine
{
    class UiCanvasChildPass;

    //! Manages child passes at runtime that render to render targets
    class LyShinePass final
        : public AZ::RPI::ParentPass
        , protected LyShinePassRequestBus::Handler
    {
        AZ_RPI_PASS(LyShinePass);
        using Base = AZ::RPI::ParentPass;

    public:
        AZ_CLASS_ALLOCATOR(LyShinePass, AZ::SystemAllocator, 0);
        AZ_RTTI(LyShinePass, "C3B812ED-3771-42F4-A96F-EBD94B4D54CA", Base);

        virtual ~LyShinePass();
        static AZ::RPI::Ptr<LyShinePass> Create(const AZ::RPI::PassDescriptor& descriptor);

    protected:
        // Pass behavior overrides
        void ResetInternal() override;
        void BuildInternal() override;

        // LyShinePassRequestBus overrides
        void RebuildRttChildren() override;
        AZ::RPI::Pass* GetRttPass(const AZStd::string& name) override;

    private:
        LyShinePass() = delete;
        explicit LyShinePass(const AZ::RPI::PassDescriptor& descriptor);

        // Build the render to texture child passes
        void AddRttChildPasses(LyShine::AttachmentImagesAndDependencies AttachmentImagesAndDependencies);

        // Add a render to texture child pass
        void AddRttChildPass(AZ::Data::Instance<AZ::RPI::AttachmentImage> attachmentImage, AttachmentImages dependentAttachmentImages);

        // Append the final pass to render UI Canvas elements to the screen
        void AddUiCanvasChildPass(LyShine::AttachmentImagesAndDependencies AttachmentImagesAndDependencies);

        // Pass that renders the UI Canvas elements to the screen
        AZ::RPI::Ptr<UiCanvasChildPass> m_uiCanvasChildPass;
    };

    // Child base pass with potential attachment dependencies
    class LyShineChildBasePass
        : public AZ::RPI::RasterPass
    {
        AZ_RPI_PASS(LyShineChildBasePass);

        friend class LyShinePass;
    public:
        AZ_RTTI(LyShineChildBasePass, "{41D525F9-09EB-4004-97DC-082078FF8DD2}", RasterPass);
        AZ_CLASS_ALLOCATOR(LyShineChildBasePass, AZ::SystemAllocator, 0);
        virtual ~LyShineChildBasePass();

    protected:
        LyShineChildBasePass(const AZ::RPI::PassDescriptor& descriptor);

        // Scope producer Overrides...
        void SetupFrameGraphDependencies(AZ::RHI::FrameGraphInterface frameGraph) override;

        AZ::RHI::DrawListTag m_drawListTag;
        AttachmentImages m_attachmentImageDependencies;
    };

    // Child pass that renders UI elements to a render target
    class RttChildPass
        : public LyShineChildBasePass
    {
        AZ_RPI_PASS(RttChildPass);

        friend class LyShinePass;

    public:
        AZ_RTTI(RttChildPass, "{54B0574D-2EB3-4054-9E1D-0E0D9C8CB09A}", LyShineChildBasePass);
        AZ_CLASS_ALLOCATOR(RttChildPass, AZ::SystemAllocator, 0);
        virtual ~RttChildPass();

        //! Creates a RttChildPass
        static AZ::RPI::Ptr<RttChildPass> Create(const AZ::RPI::PassDescriptor& descriptor);

    protected:
        RttChildPass(const AZ::RPI::PassDescriptor& descriptor);

        // Pass behavior overrides
        void BuildInternal() override;

        AZ::Data::Instance<AZ::RPI::AttachmentImage> m_attachmentImage;
    };

    // Child pass that renders the UI Canvases to the screen
    class UiCanvasChildPass
        : public LyShineChildBasePass
    {
        AZ_RPI_PASS(UiCanvasChildPass);

    public:
        AZ_RTTI(UiCanvasChildPass, "{4F2FB790-E66E-4BF4-8890-3B58018B906F}", LyShineChildBasePass);
        AZ_CLASS_ALLOCATOR(UiCanvasChildPass, AZ::SystemAllocator, 0);
        virtual ~UiCanvasChildPass();

        //! Creates a UiCanvasChildPass
        static AZ::RPI::Ptr<UiCanvasChildPass> Create(const AZ::RPI::PassDescriptor& descriptor);

    protected:
        UiCanvasChildPass(const AZ::RPI::PassDescriptor& descriptor);
    };
} // namespace LyShine
