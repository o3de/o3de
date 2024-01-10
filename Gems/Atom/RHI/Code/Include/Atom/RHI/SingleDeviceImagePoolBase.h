/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/SingleDeviceResourcePool.h>
#include <Atom/RHI/SingleDeviceImage.h>

namespace AZ::RHI
{
    //! A simple base class for image pools. This mainly exists so that various
    //! image pool implementations can have some type safety separate from other
    //! resource pool types.
    class SingleDeviceImagePoolBase
        : public SingleDeviceResourcePool
    {
    public:
        AZ_RTTI(SingleDeviceImagePoolBase, "{6353E390-C5D2-42FF-8AA9-9AFCD1F2F1B5}", SingleDeviceResourcePool);
        virtual ~SingleDeviceImagePoolBase() override = default;

    protected:
        SingleDeviceImagePoolBase() = default;

        ResultCode InitImage(
            SingleDeviceImage* image,
            const ImageDescriptor& descriptor,
            PlatformMethod platformInitResourceMethod);

    private:
        using SingleDeviceResourcePool::InitResource;
    };
}
