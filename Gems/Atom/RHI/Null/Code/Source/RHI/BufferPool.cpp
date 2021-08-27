/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <RHI/BufferPool.h>

namespace AZ
{
    namespace Null
    {
        RHI::Ptr<BufferPool> BufferPool::Create()
        {
            return aznew BufferPool();
        }

        void BufferPool::OnFrameEnd()
        {
            Base::OnFrameEnd();
        }
    }
}
