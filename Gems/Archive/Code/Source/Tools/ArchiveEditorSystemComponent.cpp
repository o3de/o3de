/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/SerializeContext.h>
#include "ArchiveEditorSystemComponent.h"

#include <Archive/ArchiveTypeIds.h>

namespace Archive
{
    AZ_COMPONENT_IMPL(ArchiveEditorSystemComponent, "ArchiveEditorSystemComponent",
        ArchiveEditorSystemComponentTypeId, BaseSystemComponent);

    void ArchiveEditorSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<ArchiveEditorSystemComponent, ArchiveSystemComponent>()
                ->Version(0);
        }
    }

    ArchiveEditorSystemComponent::ArchiveEditorSystemComponent() = default;

    ArchiveEditorSystemComponent::~ArchiveEditorSystemComponent() = default;

    void ArchiveEditorSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        BaseSystemComponent::GetProvidedServices(provided);
        provided.push_back(AZ_CRC_CE("ArchiveEditorService"));
    }

    void ArchiveEditorSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        BaseSystemComponent::GetIncompatibleServices(incompatible);
        incompatible.push_back(AZ_CRC_CE("ArchiveEditorService"));
    }

    void ArchiveEditorSystemComponent::GetRequiredServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        BaseSystemComponent::GetRequiredServices(required);
    }

    void ArchiveEditorSystemComponent::GetDependentServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        BaseSystemComponent::GetDependentServices(dependent);
    }

    void ArchiveEditorSystemComponent::Activate()
    {
        ArchiveSystemComponent::Activate();
        AzToolsFramework::EditorEvents::Bus::Handler::BusConnect();
    }

    void ArchiveEditorSystemComponent::Deactivate()
    {
        AzToolsFramework::EditorEvents::Bus::Handler::BusDisconnect();
        ArchiveSystemComponent::Deactivate();
    }

} // namespace Archive
