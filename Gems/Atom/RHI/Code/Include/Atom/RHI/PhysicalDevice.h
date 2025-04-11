/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/RHI.Reflect/PhysicalDeviceDescriptor.h>
#include <Atom/RHI/Object.h>
#include <AzCore/std/containers/vector.h>

namespace AZ::RHI
{
    class PhysicalDevice
        : public Object
    {
    public:
        AZ_RTTI(PhysicalDevice, "{B881F2FA-C588-4332-BB4A-D81AC8BF30E9}", Object);
        virtual ~PhysicalDevice() = default;

        /// Returns the descriptor for the physical device.
        const PhysicalDeviceDescriptor& GetDescriptor() const;

    protected:
        PhysicalDeviceDescriptor m_descriptor;
    };

    using PhysicalDeviceList = AZStd::vector<Ptr<PhysicalDevice>>;
}
