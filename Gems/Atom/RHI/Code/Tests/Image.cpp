/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <Tests/Image.h>

namespace UnitTest
{
    using namespace AZ;

    RHI::ResultCode ImageView::InitInternal(RHI::Device&, const RHI::DeviceResource&)
    {
        return RHI::ResultCode::Success;
    }

    RHI::ResultCode ImageView::InvalidateInternal()
    {
        return RHI::ResultCode::Success;
    }

    void ImageView::ShutdownInternal()
    {
    }

    RHI::ResultCode ImagePool::InitInternal(AZ::RHI::Device&, const RHI::ImagePoolDescriptor&)
    {
        return RHI::ResultCode::Success;
    }

    void ImagePool::ShutdownInternal()
    {
    }

    RHI::ResultCode ImagePool::InitImageInternal(const RHI::DeviceImageInitRequest&)
    {
        return RHI::ResultCode::Success;
    }

    void ImagePool::ShutdownResourceInternal(RHI::DeviceResource&)
    {
    }
}
