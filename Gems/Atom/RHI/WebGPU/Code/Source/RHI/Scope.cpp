/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <RHI/WebGPU.h>
#include <RHI/Scope.h>

namespace AZ::WebGPU
{
    RHI::Ptr<Scope> Scope::Create()
    {
        return aznew Scope();
    }

    bool Scope::HasSignalFence() const
    {
        return false;
    }

    bool Scope::HasWaitFences() const
    {
        return false;
    }
}
