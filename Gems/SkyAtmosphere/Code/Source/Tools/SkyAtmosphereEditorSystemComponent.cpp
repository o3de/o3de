/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/SerializeContext.h>
#include "SkyAtmosphereEditorSystemComponent.h"

#include <SkyAtmosphere/SkyAtmosphereTypeIds.h>

namespace SkyAtmosphere
{
    AZ_COMPONENT_IMPL(SkyAtmosphereEditorSystemComponent, "SkyAtmosphereEditorSystemComponent",
        SkyAtmosphereEditorSystemComponentTypeId, BaseSystemComponent);

    void SkyAtmosphereEditorSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<SkyAtmosphereEditorSystemComponent, SkyAtmosphereSystemComponent>()
                ->Version(0);
        }
    }

    SkyAtmosphereEditorSystemComponent::SkyAtmosphereEditorSystemComponent() = default;

    SkyAtmosphereEditorSystemComponent::~SkyAtmosphereEditorSystemComponent() = default;

    void SkyAtmosphereEditorSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        BaseSystemComponent::GetProvidedServices(provided);
        provided.push_back(AZ_CRC_CE("SkyAtmosphereSystemEditorService"));
    }

    void SkyAtmosphereEditorSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        BaseSystemComponent::GetIncompatibleServices(incompatible);
        incompatible.push_back(AZ_CRC_CE("SkyAtmosphereSystemEditorService"));
    }

    void SkyAtmosphereEditorSystemComponent::GetRequiredServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        BaseSystemComponent::GetRequiredServices(required);
    }

    void SkyAtmosphereEditorSystemComponent::GetDependentServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        BaseSystemComponent::GetDependentServices(dependent);
    }

    void SkyAtmosphereEditorSystemComponent::Activate()
    {
        SkyAtmosphereSystemComponent::Activate();
        AzToolsFramework::EditorEvents::Bus::Handler::BusConnect();
    }

    void SkyAtmosphereEditorSystemComponent::Deactivate()
    {
        AzToolsFramework::EditorEvents::Bus::Handler::BusDisconnect();
        SkyAtmosphereSystemComponent::Deactivate();
    }

} // namespace SkyAtmosphere
