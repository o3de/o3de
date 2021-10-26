/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <RHI/Device.h>

namespace AZ
{
    namespace Null
    {
        RHI::Ptr<Device> Device::Create()
        {
            return aznew Device();
        }

        Device::Device()
        {
            m_descriptor.m_platformLimitsDescriptor = aznew RHI::PlatformLimitsDescriptor;
        }

        void Device::FillFormatsCapabilitiesInternal(FormatCapabilitiesList& formatsCapabilities)
        {
            formatsCapabilities.fill(static_cast<RHI::FormatCapabilities>(~0));
        }

        void Device::ObjectCollectionNotify(RHI::ObjectCollectorNotifyFunction notifyFunction)
        {
            notifyFunction();
        }
    }
}
