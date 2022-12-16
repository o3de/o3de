/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI/MultiDeviceObject.h>
#include <Atom/RHI/RHISystemInterface.h>

namespace AZ
{
    namespace RHI
    {
        bool MultiDeviceObject::IsInitialized() const
        {
            return m_deviceMask != 0;
        }

        DeviceMask MultiDeviceObject::GetDeviceMask() const
        {
            return m_deviceMask;
        }

        void MultiDeviceObject::Init(DeviceMask deviceMask)
        {
            m_deviceMask = deviceMask;
        }

        void MultiDeviceObject::Shutdown()
        {
            m_deviceMask = 0;
        }

        int MultiDeviceObject::GetDeviceCount() const
        {
            return RHI::RHISystemInterface::Get()->GetDeviceCount();
        }
    }
}
