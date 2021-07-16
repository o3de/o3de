/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI.Reflect/TransientImageDescriptor.h>
#include <Atom/RHI/AliasedAttachmentAllocator.h>
#include <RHI/Device.h>
#include <RHI/Image.h>
#include <RHI/Scope.h>
#include <RHI/TransientAttachmentPool.h>

namespace AZ
{
    namespace Null
    {
        RHI::Ptr<TransientAttachmentPool> TransientAttachmentPool::Create()
        {
            return aznew TransientAttachmentPool();
        }
    }
}

