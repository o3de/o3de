/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/MultiDeviceImage.h>
#include <Atom/RHI/MultiDeviceResourcePool.h>

namespace AZ::RHI
{
    //! A simple base class for multi-device image pools. This mainly exists so that various
    //! image pool implementations can have some type safety separate from other
    //! resource pool types.
    class MultiDeviceImagePoolBase : public MultiDeviceResourcePool
    {
    public:
        AZ_RTTI(MultiDeviceImagePoolBase, "{CAC2167A-D65A-493F-A450-FDE2B1A883B1}", MultiDeviceResourcePool);
        virtual ~MultiDeviceImagePoolBase() override = default;

    protected:
        MultiDeviceImagePoolBase() = default;

        ResultCode InitImage(MultiDeviceImage* image, const ImageDescriptor& descriptor, PlatformMethod platformInitResourceMethod);

    private:
        using MultiDeviceResourcePool::InitResource;
    };
} // namespace AZ::RHI
