/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI.Reflect/ScopeId.h>
#include <Atom/RHI.Reflect/AttachmentEnums.h>
#include <Atom/RHI.Reflect/Handle.h>
#include <Atom/RHI/ResourcePool.h>
#include <Atom/RHI/QueryPool.h>
#include <Atom/RHI/Fence.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/containers/array.h>

namespace AZ::RHI
{
    //! This class serves as an opaque container for RHI specific data related with Subpass Dependencies.
    //! Being an opaque handle, it can be freely used and passed around within the RPI, typically wrapped
    //! around an AZstd::shared_ptr.
    //! For more details:
    //! https://github.com/o3de/sig-graphics-audio/blob/9e4e4111ad9bc04d73f3149c6e54301781ffd569/rfcs/SubpassesSupportInRPI/RFC_SubpassesSupportInRPI.md
    class SubpassDependencies
    {
    public:
        AZ_RTTI(SubpassDependencies, "{C1902708-76B9-44E8-A1F5-7B5C876A6C51}");

        SubpassDependencies() = default;
        virtual ~SubpassDependencies() = default;

        //! @returns true if this opaque blob contains valid data.
        //!     Each RHI would have its own custom logic.
        virtual bool IsValid() const = 0;
    };
}
