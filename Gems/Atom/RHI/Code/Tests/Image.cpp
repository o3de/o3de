/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
#include <Tests/Image.h>

namespace UnitTest
{
    using namespace AZ;

    RHI::ResultCode ImageView::InitInternal(RHI::Device&, const RHI::Resource&)
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

    RHI::ResultCode ImagePool::InitImageInternal(const RHI::ImageInitRequest&)
    {
        return RHI::ResultCode::Success;
    }

    void ImagePool::ShutdownResourceInternal(RHI::Resource&)
    {
    }
}
