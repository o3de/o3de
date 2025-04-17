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
    //! Splash screen settings that describe how to render.
    //! The settings will be retrieved from setting registry: splash_screen.setreg.
    class SplashScreenSettings
    {
    public:
        AZ_RTTI(AZ::Render::SplashScreenSettings, "{4B441F73-8F72-4DCA-9451-416C40693487}");
        AZ_CLASS_ALLOCATOR(AZ::Render::SplashScreenSettings, AZ::SystemAllocator, 0);

        SplashScreenSettings() = default;
        virtual ~SplashScreenSettings() = default;

        static void Reflect(AZ::ReflectContext* context);

        //! The image path to be rendered on the screen.
        AZStd::string m_imagePath;

        //! The time that the splash screen will last.
        //! Number in seconds.
        float m_durationSeconds = 10.0f;

        //! Fading effect flag.
        bool m_fading = true;
    };
} // namespace AZ::Render
