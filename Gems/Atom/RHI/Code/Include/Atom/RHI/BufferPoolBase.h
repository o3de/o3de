/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/Buffer.h>
#include <Atom/RHI/ResourcePool.h>

namespace AZ::RHI
{
    //! A simple base class for buffer pools. This mainly exists so that various
    //! buffer pool implementations can have some type safety separate from other
    //! resource pool types.
    class BufferPoolBase : public ResourcePool
    {
    public:
        AZ_RTTI(BufferPoolBase, "{08EC3384-CC9F-4F04-B87E-0BB9D23F7CB0}", ResourcePool);
        virtual ~BufferPoolBase() override = default;

    protected:
        BufferPoolBase() = default;

        ResultCode InitBuffer(Buffer* buffer, const BufferDescriptor& descriptor, PlatformMethod platformInitResourceMethod);

    private:
        using ResourcePool::InitResource;
    };
} // namespace AZ::RHI
