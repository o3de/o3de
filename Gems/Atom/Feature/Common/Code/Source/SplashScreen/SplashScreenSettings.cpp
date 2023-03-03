/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/SerializeContext.h>
#include <Atom/Feature/SplashScreen/SplashScreenSettings.h>

namespace AZ::Render
{
    void SplashScreenSettings::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
        {
            serializeContext->Class<SplashScreenSettings>()
                ->Version(1)
                ->Field("ImagePath", &SplashScreenSettings::m_imagePath)
                ->Field("DurationSeconds", &SplashScreenSettings::m_durationSeconds)
                ->Field("Fading", &SplashScreenSettings::m_fading);
        }
    }
} // namespace AZ::Render
