/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <AzCore/Serialization/SerializeContext.h>
#include "AFREditorSystemComponent.h"

#include <AFR/AFRTypeIds.h>

namespace AFR
{
    AZ_COMPONENT_IMPL(AFREditorSystemComponent, "AFREditorSystemComponent",
        AFREditorSystemComponentTypeId, BaseSystemComponent);

    void AFREditorSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<AFREditorSystemComponent, AFRSystemComponent>()
                ->Version(0);
        }
    }

    AFREditorSystemComponent::AFREditorSystemComponent() = default;

    AFREditorSystemComponent::~AFREditorSystemComponent() = default;

    void AFREditorSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        BaseSystemComponent::GetProvidedServices(provided);
        provided.push_back(AZ_CRC_CE("AFREditorService"));
    }

    void AFREditorSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        BaseSystemComponent::GetIncompatibleServices(incompatible);
        incompatible.push_back(AZ_CRC_CE("AFREditorService"));
    }

    void AFREditorSystemComponent::GetRequiredServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        BaseSystemComponent::GetRequiredServices(required);
    }

    void AFREditorSystemComponent::GetDependentServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        BaseSystemComponent::GetDependentServices(dependent);
    }

    void AFREditorSystemComponent::Activate()
    {
        AFRSystemComponent::Activate();
        AzToolsFramework::EditorEvents::Bus::Handler::BusConnect();
    }

    void AFREditorSystemComponent::Deactivate()
    {
        AzToolsFramework::EditorEvents::Bus::Handler::BusDisconnect();
        AFRSystemComponent::Deactivate();
    }

} // namespace AFR
