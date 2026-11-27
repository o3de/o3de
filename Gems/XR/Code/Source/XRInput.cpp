/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <XR/XRInput.h>

namespace XR
{
    AZ::RHI::ResultCode Input::Init(Descriptor descriptor)
    {
        m_descriptor = descriptor;
        return InitInternal();
    }

    const Input::Descriptor& Input::GetDescriptor() const
    {
        return m_descriptor;
    }

    void Input::Shutdown()
    {
        ShutdownInternal();
    }

} // namespace XR
