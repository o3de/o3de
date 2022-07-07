/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>

#include <Material/BlastMaterialConfiguration.h>

namespace Blast
{
    void MaterialConfiguration::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<Blast::MaterialConfiguration>()
                ->Version(1)
                ->Field("Health", &MaterialConfiguration::m_health)
                ->Field("ForceDivider", &MaterialConfiguration::m_forceDivider)
                ->Field("MinDamageThreshold", &MaterialConfiguration::m_minDamageThreshold)
                ->Field("MaxDamageThreshold", &MaterialConfiguration::m_maxDamageThreshold)
                ->Field("StressLinearFactor", &MaterialConfiguration::m_stressLinearFactor)
                ->Field("StressAngularFactor", &MaterialConfiguration::m_stressAngularFactor);

            if (auto* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<Blast::MaterialConfiguration>("", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "Blast Material")
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &MaterialConfiguration::m_health, "Health",
                        "All damage is subtracted from this value")
                        ->Attribute(AZ::Edit::Attributes::Min, 0.f)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &MaterialConfiguration::m_forceDivider, "Force divider",
                        "All damage which originates with force is divided by this amount")
                        ->Attribute(AZ::Edit::Attributes::Min, 0.f)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &MaterialConfiguration::m_minDamageThreshold,
                        "Minimum damage threshold", "Incoming damage is discarded if it is less than this value")
                        ->Attribute(AZ::Edit::Attributes::Min, 0.f)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &MaterialConfiguration::m_maxDamageThreshold,
                        "Maximum damage threshold", "Incoming damage is capped at this value")
                        ->Attribute(AZ::Edit::Attributes::Min, 0.f)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &MaterialConfiguration::m_stressLinearFactor,
                        "Stress linear factor",
                        "Factor with which linear stress such as gravity, direct impulse, collision is applied")
                        ->Attribute(AZ::Edit::Attributes::Min, 0.f)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &MaterialConfiguration::m_stressAngularFactor,
                        "Stress angular factor", "Factor with which angular stress is applied")
                        ->Attribute(AZ::Edit::Attributes::Min, 0.f);
            }
        }
    }
} // namespace Blast
