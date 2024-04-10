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
    //! GALIB: Add comment
    //! Remark: This is not a Resource!
    class SubpassDependencies
    {
        //friend class FrameGraph;
        //friend class FrameGraphCompiler;
    public:
        AZ_RTTI(SubpassDependencies, "{C1902708-76B9-44E8-A1F5-7B5C876A6C51}");

        SubpassDependencies() = default;
        virtual ~SubpassDependencies() = default;


    protected:


    private:
        //////////////////////////////////////////////////////////////////////////
        // Platform API


        //////////////////////////////////////////////////////////////////////////

    };
}
