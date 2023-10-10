/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/MultiDeviceBuffer.h>
#include <Atom/RHI/MultiDeviceResourcePool.h>

namespace AZ::RHI
{
    //! A simple base class for buffer pools. This mainly exists so that various
    //! buffer pool implementations can have some type safety separate from other
    //! resource pool types.
    class MultiDeviceBufferPoolBase : public MultiDeviceResourcePool
    {
    public:
        AZ_RTTI(MultiDeviceBufferPoolBase, "{08EC3384-CC9F-4F04-B87E-0BB9D23F7CB0}", MultiDeviceResourcePool);
        virtual ~MultiDeviceBufferPoolBase() override = default;

    protected:
        MultiDeviceBufferPoolBase() = default;

        ResultCode InitBuffer(MultiDeviceBuffer* buffer, const BufferDescriptor& descriptor, PlatformMethod platformInitResourceMethod);

    private:
        using MultiDeviceResourcePool::InitResource;
    };
} // namespace AZ::RHI
