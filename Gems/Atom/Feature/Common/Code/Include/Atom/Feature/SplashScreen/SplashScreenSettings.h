/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace AZ::Render
{
    class SplashScreenSettings
    {
    public:
        AZ_RTTI(AZ::Render::SplashScreenSettings, "{4B441F73-8F72-4DCA-9451-416C40693487}");
        AZ_CLASS_ALLOCATOR(AZ::Render::SplashScreenSettings, AZ::SystemAllocator, 0);

        SplashScreenSettings() = default;
        virtual ~SplashScreenSettings() = default;

        static void Reflect(AZ::ReflectContext* context);

        bool m_enbaled = true;
        AZStd::string m_imagePath;
        float m_lastingTime = 10.0f;
        bool m_fading = true;
    };
} // namespace AZ::Render
