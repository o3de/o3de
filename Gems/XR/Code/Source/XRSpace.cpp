/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <XR/XRSpace.h>

namespace XR
{
    AZ::RHI::ResultCode Space::Init(Descriptor descriptor)
    {
        m_descriptor = descriptor;
        return InitInternal();
    }

    const Space::Descriptor& Space::GetDescriptor() const
    {
        return m_descriptor;
    }

    void Space::Shutdown()
    {
        ShutdownInternal();
    }
} // namespace XR
