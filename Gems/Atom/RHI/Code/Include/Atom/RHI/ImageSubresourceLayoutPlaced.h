/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI.Reflect/ImageSubresource.h>

namespace AZ
{
    namespace RHI
    {
        struct ImageViewDescriptor;

        struct ImageSubresourceLayoutPlaced
        {
            AZ_TYPE_INFO(ImageSubresourceLayoutPlaced, "{8AD0DC97-5AAA-470F-8853-C8A55E023CD1}");

            ImageSubresourceLayoutPlaced() = default;

            DeviceImageSubresourceLayoutPlaced& GetDeviceImageSubresourcePlaced(int deviceIndex)
            {
                return m_deviceImageSubresourceLayoutPlaced[deviceIndex];
            }

            const DeviceImageSubresourceLayoutPlaced& GetDeviceImageSubresourcePlaced(int deviceIndex) const
            {
                AZ_Assert(
                    m_deviceImageSubresourceLayoutPlaced.find(deviceIndex) != m_deviceImageSubresourceLayoutPlaced.end(),
                    "No DeviceImageSubresourceLayoutPlaced found for device index %d\n",
                    deviceIndex);
                return m_deviceImageSubresourceLayoutPlaced.at(deviceIndex);
            }

            AZStd::unordered_map<int, DeviceImageSubresourceLayoutPlaced> m_deviceImageSubresourceLayoutPlaced;
        };
    }
}
