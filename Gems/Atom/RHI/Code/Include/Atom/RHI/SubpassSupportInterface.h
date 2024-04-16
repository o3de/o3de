/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Interface/Interface.h>

#include <Atom/RHI.Reflect/RenderAttachmentLayout.h>
#include <Atom/RHI.Reflect/SubpassDependencies.h>

namespace AZ::RHI
{
    //! This is an optional RHI interface. Only RHIs, like Vulkan, that support subpasses should implement this inteface.
    //! This API is typically invoked by the RPI when instantiating RasterPasses that
    //! should be merged as Subpasses.
    //! For more details:
    //! https://github.com/o3de/sig-graphics-audio/blob/9e4e4111ad9bc04d73f3149c6e54301781ffd569/rfcs/SubpassesSupportInRPI/RFC_SubpassesSupportInRPI.md
    class ISubpassSupport
    {
    public:
        AZ_RTTI(ISubpassSupport, "{0432D83C-6EE2-4086-BDB6-7C62BF39458A}");
        AZ_DISABLE_COPY_MOVE(ISubpassSupport);

        ISubpassSupport() = default;
        virtual ~ISubpassSupport() = default;

        //! @returns A shared pointer to an opaque blob that encapsulates Subpass Dependency data from a Render Attachment Layout.
        //! The RPI calls this function AFTER RasterPass::BuildInternal() has been called on all RasterPasses that should be merged
        //! as a group of subpasses.
        //! @remarks This function should only be called if there's more than one subpass declared in @layout.
        virtual AZStd::shared_ptr<SubpassDependencies> BuildSubpassDependencies(const RHI::RenderAttachmentLayout& layout) const = 0;
    };

    using SubpassSupportInterface = AZ::Interface<ISubpassSupport>;
} //namespace AZ::RHI
