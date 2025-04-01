/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RPI.Public/Pass/ParentPass.h>
#include <Atom/RPI.Public/Pass/RasterPass.h>
#include <AzCore/std/containers/span.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/containers/vector.h>
#include "LyShinePassDataBus.h"

namespace LyShine
{
    class LyShineChildPass;

    //! Manages child passes at runtime that render to render targets
    class LyShinePass final
        : public AZ::RPI::ParentPass
        , protected LyShinePassRequestBus::Handler
    {
        AZ_RPI_PASS(LyShinePass);
        using Base = AZ::RPI::ParentPass;

    public:
        AZ_CLASS_ALLOCATOR(LyShinePass, AZ::SystemAllocator);
        AZ_RTTI(LyShinePass, "C3B812ED-3771-42F4-A96F-EBD94B4D54CA", Base);

        virtual ~LyShinePass();
        static AZ::RPI::Ptr<LyShinePass> Create(const AZ::RPI::PassDescriptor& descriptor);

    protected:
        // Pass behavior overrides
        void ResetInternal() override;
        void BuildInternal() override;
        void CreateChildPassesInternal() override;
        void SetRenderPipeline(AZ::RPI::RenderPipeline* pipeline) override;

        // LyShinePassRequestBus overrides
        void RebuildRttChildren() override;
        AZ::RPI::RasterPass* GetRttPass(const AZStd::string& name) override;
        AZ::RPI::RasterPass* GetUiCanvasPass() override;

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
        AZ::RPI::Ptr<LyShineChildPass> m_uiCanvasChildPass;
    };

    // Child pass with potential attachment dependencies
    class LyShineChildPass
        : public AZ::RPI::RasterPass
    {
        AZ_RPI_PASS(LyShineChildPass);

        friend class LyShinePass;
    public:
        AZ_RTTI(LyShineChildPass, "{41D525F9-09EB-4004-97DC-082078FF8DD2}", RasterPass);
        AZ_CLASS_ALLOCATOR(LyShineChildPass, AZ::SystemAllocator);
        virtual ~LyShineChildPass();

        //! Creates a LyShineChildPass
        static AZ::RPI::Ptr<LyShineChildPass> Create(const AZ::RPI::PassDescriptor& descriptor);

    protected:
        LyShineChildPass(const AZ::RPI::PassDescriptor& descriptor);

        // Scope producer Overrides...
        void SetupFrameGraphDependencies(AZ::RHI::FrameGraphInterface frameGraph) override;

        AttachmentImages m_attachmentImageDependencies;
    };

    // Child pass that renders UI elements to a render target
    class RttChildPass
        : public LyShineChildPass
    {
        AZ_RPI_PASS(RttChildPass);

        friend class LyShinePass;

    public:
        AZ_RTTI(RttChildPass, "{54B0574D-2EB3-4054-9E1D-0E0D9C8CB09A}", LyShineChildPass);
        AZ_CLASS_ALLOCATOR(RttChildPass, AZ::SystemAllocator);
        virtual ~RttChildPass();

        //! Creates a RttChildPass
        static AZ::RPI::Ptr<RttChildPass> Create(const AZ::RPI::PassDescriptor& descriptor);

    protected:
        RttChildPass(const AZ::RPI::PassDescriptor& descriptor);

        // Pass behavior overrides
        void BuildInternal() override;

        AZ::Data::Instance<AZ::RPI::AttachmentImage> m_attachmentImage;
    };
} // namespace LyShine
