/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <XR/XRInstance.h>

namespace XR
{
    AZ::RHI::ResultCode Instance::Init(AZ::RHI::ValidationMode validationMode)
    {
        m_validationMode = validationMode;
        return InitInstanceInternal();
    }

    void Instance::Shutdown()
    {
        ShutdownInternal();
    }
} // namespace XR
