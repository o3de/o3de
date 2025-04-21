/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/RHI.Reflect/Base.h>
#include <Atom/RHI.Reflect/Limits.h>
#include <Atom/RHI.Reflect/PlatformLimitsDescriptor.h>

namespace AZ
{
    class ReflectContext;
}

namespace AZ::RHI
{
    class PlatformLimits;
    class DeviceDescriptor
    {
    public:
        AZ_RTTI(DeviceDescriptor, "{8446A34C-A079-44B8-A20F-45D9CAB1FAFD}");
        static void Reflect(AZ::ReflectContext* context);

        DeviceDescriptor() = default;
        virtual ~DeviceDescriptor();

        uint32_t m_frameCountMax = RHI::Limits::Device::FrameCountMax;
        Ptr<PlatformLimitsDescriptor> m_platformLimitsDescriptor = nullptr;
    };
}
