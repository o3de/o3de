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
    //! GALIB Add Comment
    class ISubpassDependenciesBuilder
    {
    public:
        AZ_RTTI(ISubpassDependenciesBuilder, "{0432D83C-6EE2-4086-BDB6-7C62BF39458A}");
        AZ_DISABLE_COPY_MOVE(ISubpassDependenciesBuilder);

        ISubpassDependenciesBuilder() = default;
        virtual ~ISubpassDependenciesBuilder() = default;

        //! GALIB Add Comment
        virtual AZStd::shared_ptr<SubpassDependencies> BuildSubpassDependencies(const RHI::RenderAttachmentLayout& layout) const = 0;
    };

    using SubpassDependenciesBuilderInterface = AZ::Interface<ISubpassDependenciesBuilder>;
} //namespace AZ::RHI
