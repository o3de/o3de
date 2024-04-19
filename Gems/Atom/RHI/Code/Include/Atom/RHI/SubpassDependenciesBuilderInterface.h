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
    //! This is an optional interface that any RHI can support to build
    //! SubpassDependencies data when the RHI supports grouping Raster passes Subpasses.
    //! This API is typically invoked by the RPI when instantiating RasterPasses that
    //! should be merged as Subpasses.
    class ISubpassDependenciesBuilder
    {
    public:
        AZ_RTTI(ISubpassDependenciesBuilder, "{0432D83C-6EE2-4086-BDB6-7C62BF39458A}");
        AZ_DISABLE_COPY_MOVE(ISubpassDependenciesBuilder);

        ISubpassDependenciesBuilder() = default;
        virtual ~ISubpassDependenciesBuilder() = default;

        //! @returns A shared pointer to an opaque blob that encapsulates Subpass Dependency data from a Render Attachment Layout.
        //! The RPI calls this function AFTER RasterPass::BuildInternal() has been called on all RasterPasses that should be merged
        //! as a group of subpasses.
        //! @remarks This function should only be called of there's more than one subpass declared in @layout.
        virtual AZStd::shared_ptr<SubpassDependencies> BuildSubpassDependencies(const RHI::RenderAttachmentLayout& layout) const = 0;
    };

    using SubpassDependenciesBuilderInterface = AZ::Interface<ISubpassDependenciesBuilder>;
} //namespace AZ::RHI
