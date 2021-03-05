/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#pragma once

#include <Atom/RHI.Reflect/Base.h>
#include <Atom/RHI.Reflect/Limits.h>
#include <Atom/RHI.Reflect/PlatformLimitsDescriptor.h>

namespace AZ
{
    class ReflectContext;

    namespace RHI
    {
        class PlatformLimits;
        class DeviceDescriptor
        {
        public:
            virtual ~DeviceDescriptor() = default;
            AZ_RTTI(DeviceDescriptor, "{8446A34C-A079-44B8-A20F-45D9CAB1FAFD}");
            static void Reflect(AZ::ReflectContext* context);

            DeviceDescriptor() = default;

            uint32_t m_frameCountMax = RHI::Limits::Device::FrameCountMax;
            ConstPtr<PlatformLimitsDescriptor> m_platformLimitsDescriptor = nullptr;
        };
    }
}
