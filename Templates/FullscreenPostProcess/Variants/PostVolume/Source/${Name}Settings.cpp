/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "${Name}Settings.h"

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>

namespace ${GemName}
{
    void ${SanitizedCppName}Settings::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<${SanitizedCppName}Settings>()
                ->Version(1)
                ->Field("Enabled",     &${SanitizedCppName}Settings::m_enabled)
                ->Field("Intensity",   &${SanitizedCppName}Settings::m_intensity)
                ->Field("Tint",        &${SanitizedCppName}Settings::m_tint)
                ->Field("ExposureEv",  &${SanitizedCppName}Settings::m_exposureEv)
                ;

            if (AZ::EditContext* ec = serialize->GetEditContext())
            {
                ec->Class<${SanitizedCppName}Settings>("${Name} Settings", "Per-area parameters for the ${Name} post-process pass")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(AZ::Edit::UIHandlers::CheckBox, &${SanitizedCppName}Settings::m_enabled,
                        "Enabled", "Master on/off for the effect.")
                    ->DataElement(AZ::Edit::UIHandlers::Slider, &${SanitizedCppName}Settings::m_intensity,
                        "Intensity", "0 = no effect, 1 = full strength.")
                        ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                        ->Attribute(AZ::Edit::Attributes::Max, 1.0f)
                    ->DataElement(AZ::Edit::UIHandlers::Color, &${SanitizedCppName}Settings::m_tint,
                        "Tint", "Per-channel multiplier applied to source color.")
                    ->DataElement(AZ::Edit::UIHandlers::Slider, &${SanitizedCppName}Settings::m_exposureEv,
                        "Exposure (EV)", "Stop-style exposure adjustment. 0 = no change.")
                        ->Attribute(AZ::Edit::Attributes::Min, -4.0f)
                        ->Attribute(AZ::Edit::Attributes::Max,  4.0f)
                    ;
            }
        }
    }

} // namespace ${GemName}
