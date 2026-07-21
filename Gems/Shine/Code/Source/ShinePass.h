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
#include "ShinePassDataBus.h"

namespace Shine
{
    class ShineChildPass;

    //! Manages child passes at runtime that render to render targets
    class ShinePass final
        : public AZ::RPI::ParentPass
        , protected ShinePassRequestBus::Handler
    {
        AZ_RPI_PASS(ShinePass);
        using Base = AZ::RPI::ParentPass;

    public:
        AZ_CLASS_ALLOCATOR(ShinePass, AZ::SystemAllocator);
        AZ_RTTI(ShinePass, "C3B812ED-3771-42F4-A96F-EBD94B4D54CA", Base);

        virtual ~ShinePass();
        static AZ::RPI::Ptr<ShinePass> Create(const AZ::RPI::PassDescriptor& descriptor);

    protected:
        // Pass behavior overrides
        void ResetInternal() override;
        void BuildInternal() override;
        void CreateChildPassesInternal() override;
        void SetRenderPipeline(AZ::RPI::RenderPipeline* pipeline) override;

        // ShinePassRequestBus overrides
        void RebuildRttChildren() override;
        AZ::RPI::RasterPass* GetRttPass(const AZStd::string& name) override;
        AZ::RPI::RasterPass* GetUiCanvasPass() override;

    private:
        ShinePass() = delete;
        explicit ShinePass(const AZ::RPI::PassDescriptor& descriptor);

        // Build the render to texture child passes
        void AddRttChildPasses(Shine::AttachmentImagesAndDependencies AttachmentImagesAndDependencies);

        // Add a render to texture child pass
        void AddRttChildPass(AZ::Data::Instance<AZ::RPI::AttachmentImage> attachmentImage, AttachmentImages dependentAttachmentImages);

        // Append the final pass to render UI Canvas elements to the screen
        void AddUiCanvasChildPass(Shine::AttachmentImagesAndDependencies AttachmentImagesAndDependencies);

        // Pass that renders the UI Canvas elements to the screen
        AZ::RPI::Ptr<ShineChildPass> m_uiCanvasChildPass;
    };

    // Child pass with potential attachment dependencies
    class ShineChildPass
        : public AZ::RPI::RasterPass
    {
        AZ_RPI_PASS(ShineChildPass);

        friend class ShinePass;
    public:
        AZ_RTTI(ShineChildPass, "{41D525F9-09EB-4004-97DC-082078FF8DD2}", RasterPass);
        AZ_CLASS_ALLOCATOR(ShineChildPass, AZ::SystemAllocator);
        virtual ~ShineChildPass();

        //! Creates a ShineChildPass
        static AZ::RPI::Ptr<ShineChildPass> Create(const AZ::RPI::PassDescriptor& descriptor);

    protected:
        ShineChildPass(const AZ::RPI::PassDescriptor& descriptor);

        // Scope producer Overrides...
        void SetupFrameGraphDependencies(AZ::RHI::FrameGraphInterface frameGraph) override;

        AttachmentImages m_attachmentImageDependencies;
    };

    // Child pass that renders UI elements to a render target
    class RttChildPass
        : public ShineChildPass
    {
        AZ_RPI_PASS(RttChildPass);

        friend class ShinePass;

    public:
        AZ_RTTI(RttChildPass, "{54B0574D-2EB3-4054-9E1D-0E0D9C8CB09A}", ShineChildPass);
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
} // namespace Shine
