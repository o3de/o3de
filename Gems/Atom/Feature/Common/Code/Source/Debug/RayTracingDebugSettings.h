/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/Feature/Debug/RayTracingDebugSettingsInterface.h>

namespace AZ::Render
{
    class RayTracingDebugSettings final : public RayTracingDebugSettingsInterface
    {
    public:
        AZ_CLASS_ALLOCATOR(RayTracingDebugSettings, SystemAllocator);
        AZ_RTTI(RayTracingDebugSettings, "{5EED94CE-79FF-4F78-9ADC-A6D186F346E0}");

        bool GetEnabled() const override
        {
            return m_enabled;
        }

        void SetEnabled(bool enabled) override
        {
            m_enabled = enabled;
        }

        bool m_enabled{ true };
    };
} // namespace AZ::Render
