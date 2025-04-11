/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI.Reflect/DeviceDescriptor.h>
#include <Atom/RHI.Reflect/PlatformLimitsDescriptor.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace AZ::RHI
{
    void DeviceDescriptor::Reflect(AZ::ReflectContext* context)
    {
        if (SerializeContext* serializeContext = azrtti_cast<SerializeContext*>(context))
        {
            serializeContext->Class<DeviceDescriptor>()
                ->Version(2)
                ->Field("m_frameCountMax", &DeviceDescriptor::m_frameCountMax)
                ;
        }
    }

    DeviceDescriptor::~DeviceDescriptor()
    {
        m_platformLimitsDescriptor = nullptr;
    }
}
