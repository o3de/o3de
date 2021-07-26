/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI/PhysicalDevice.h>

namespace AZ
{
    namespace RHI
    {
        const PhysicalDeviceDescriptor& PhysicalDevice::GetDescriptor() const
        {
            return m_descriptor;
        }
    }
}
