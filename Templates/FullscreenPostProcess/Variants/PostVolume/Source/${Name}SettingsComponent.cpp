/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "${Name}SettingsComponent.h"

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>

namespace ${GemName}
{
    void ${SanitizedCppName}SettingsComponent::Reflect(AZ::ReflectContext* context)
    {
        ${SanitizedCppName}Settings::Reflect(context);

        if (auto* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<${SanitizedCppName}SettingsComponent, AZ::Component>()
                ->Version(1)
                ->Field("Settings", &${SanitizedCppName}SettingsComponent::m_settings)
                ;

            if (AZ::EditContext* ec = serialize->GetEditContext())
            {
                ec->Class<${SanitizedCppName}SettingsComponent>(
                    "${Name} Settings",
                    "Per-area parameters for the ${Name} fullscreen post-process pass.")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("Game"))
                        ->Attribute(AZ::Edit::Attributes::Category, "Rendering/PostProcess")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(0, &${SanitizedCppName}SettingsComponent::m_settings,
                        "Settings", "")
                    ;
            }
        }
    }

    void ${SanitizedCppName}SettingsComponent::GetProvidedServices(
        AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("${SanitizedCppName}SettingsService"));
    }

    void ${SanitizedCppName}SettingsComponent::GetIncompatibleServices(
        AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        // Singleton-by-component: refuse a second instance per entity.
        // If you need multiple instances for blending, graduate to Atom's
        // full PostProcessFeatureProcessor-based pattern (see header).
        incompatible.push_back(AZ_CRC_CE("${SanitizedCppName}SettingsService"));
    }

    void ${SanitizedCppName}SettingsComponent::Activate()
    {
        // First-active wins. If another component is already registered we
        // silently skip; the existing one keeps providing settings.
        if (AZ::Interface<${SanitizedCppName}SettingsProviderInterface>::Get() == nullptr)
        {
            AZ::Interface<${SanitizedCppName}SettingsProviderInterface>::Register(this);
        }
    }

    void ${SanitizedCppName}SettingsComponent::Deactivate()
    {
        if (AZ::Interface<${SanitizedCppName}SettingsProviderInterface>::Get() == this)
        {
            AZ::Interface<${SanitizedCppName}SettingsProviderInterface>::Unregister(this);
        }
    }

} // namespace ${GemName}
