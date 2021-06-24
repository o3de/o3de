/*
 * Copyright (c) Contributors to the Open 3D Engine Project
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
