/*
 * Copyright (c) Contributors to the Open 3D Engine Project
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI/DeviceObject.h>
#include <Atom/RHI/Device.h>

namespace AZ
{
    namespace RHI
    {
        bool DeviceObject::IsInitialized() const
        {
            return m_device != nullptr;
        }

        Device& DeviceObject::GetDevice() const
        {
            return *m_device;
        }

        void DeviceObject::Init(Device& device)
        {
            m_device = &device;
        }

        void DeviceObject::Shutdown()
        {
            m_device = nullptr;
        }
    }
}
