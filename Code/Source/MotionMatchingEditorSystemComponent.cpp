/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/SerializeContext.h>
#include <MotionMatchingEditorSystemComponent.h>

namespace EMotionFX::MotionMatching
{
    void MotionMatchingEditorSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<MotionMatchingEditorSystemComponent, MotionMatchingSystemComponent>()
                ->Version(0);
        }
    }

    MotionMatchingEditorSystemComponent::MotionMatchingEditorSystemComponent() = default;

    MotionMatchingEditorSystemComponent::~MotionMatchingEditorSystemComponent() = default;

    void MotionMatchingEditorSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        BaseSystemComponent::GetProvidedServices(provided);
        provided.push_back(AZ_CRC_CE("MotionMatchingEditorService"));
    }

    void MotionMatchingEditorSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        BaseSystemComponent::GetIncompatibleServices(incompatible);
        incompatible.push_back(AZ_CRC_CE("MotionMatchingEditorService"));
    }

    void MotionMatchingEditorSystemComponent::GetRequiredServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        BaseSystemComponent::GetRequiredServices(required);
    }

    void MotionMatchingEditorSystemComponent::GetDependentServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        BaseSystemComponent::GetDependentServices(dependent);
    }

    void MotionMatchingEditorSystemComponent::Activate()
    {
        MotionMatchingSystemComponent::Activate();
        AzToolsFramework::EditorEvents::Bus::Handler::BusConnect();
    }

    void MotionMatchingEditorSystemComponent::Deactivate()
    {
        AzToolsFramework::EditorEvents::Bus::Handler::BusDisconnect();
        MotionMatchingSystemComponent::Deactivate();
    }
} // namespace EMotionFX::MotionMatching
