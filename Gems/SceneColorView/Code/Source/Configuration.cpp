/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: MIT
 */

#include <SceneColorView/Configuration.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>

namespace SceneColorView
{
    void Configuration::Reflect(AZ::ReflectContext* context)
    {
        if (auto* sc = azrtti_cast<AZ::SerializeContext*>(context))
        {
            sc->Enum<CaptureResolution>()
                ->Value("Full", CaptureResolution::Full)
                ->Value("Half", CaptureResolution::Half)
                ->Value("Quarter", CaptureResolution::Quarter);
            sc->Class<Configuration>()->Version(0)
                ->Field("Enabled", &Configuration::m_enabled)
                ->Field("ResolutionDivisor", &Configuration::m_resolutionDivisor);
            if (auto* ec = sc->GetEditContext())
            {
                ec->Class<Configuration>("Scene Color Capture", "Settings shared by the supported views in this render scene")
                    ->DataElement(AZ::Edit::UIHandlers::CheckBox, &Configuration::m_enabled,
                        "Enable capture", "Off by default so an existing scene-color setup is not duplicated automatically")
                    ->DataElement(AZ::Edit::UIHandlers::ComboBox, &Configuration::m_resolutionDivisor,
                        "Capture resolution", "Scale applies to width and height; HDR format stays RGBA16F")
                    ->EnumAttribute(CaptureResolution::Full, "Full width and height")
                    ->EnumAttribute(CaptureResolution::Half, "Half width and height (25% pixels)")
                    ->EnumAttribute(CaptureResolution::Quarter, "Quarter width and height (6.25% pixels)");
            }
        }
    }
}
