// {BEGIN_LICENSE}
/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
// {END_LICENSE}

#include <AzCore/Serialization/SerializeContext.h>

#include <AzToolsFramework/API/ViewPaneOptions.h>

#include "${Name}Widget.h"
#include "${Name}EditorSystemComponent.h"

#include <${Name}/${Name}TypeIds.h>

namespace ${SanitizedCppName}
{
    AZ_COMPONENT_IMPL(${SanitizedCppName}EditorSystemComponent, "${SanitizedCppName}EditorSystemComponent",
        ${SanitizedCppName}EditorSystemComponentTypeId, BaseSystemComponent);

    void ${SanitizedCppName}EditorSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<${SanitizedCppName}EditorSystemComponent, ${SanitizedCppName}SystemComponent>()
                ->Version(0);
        }
    }

    ${SanitizedCppName}EditorSystemComponent::${SanitizedCppName}EditorSystemComponent() = default;

    ${SanitizedCppName}EditorSystemComponent::~${SanitizedCppName}EditorSystemComponent() = default;

    void ${SanitizedCppName}EditorSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        BaseSystemComponent::GetProvidedServices(provided);
        provided.push_back(AZ_CRC_CE("${SanitizedCppName}EditorService"));
    }

    void ${SanitizedCppName}EditorSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        BaseSystemComponent::GetIncompatibleServices(incompatible);
        incompatible.push_back(AZ_CRC_CE("${SanitizedCppName}EditorService"));
    }

    void ${SanitizedCppName}EditorSystemComponent::GetRequiredServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        BaseSystemComponent::GetRequiredServices(required);
    }

    void ${SanitizedCppName}EditorSystemComponent::GetDependentServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        BaseSystemComponent::GetDependentServices(dependent);
    }

    void ${SanitizedCppName}EditorSystemComponent::Activate()
    {
        ${SanitizedCppName}SystemComponent::Activate();
        AzToolsFramework::EditorEvents::Bus::Handler::BusConnect();
    }

    void ${SanitizedCppName}EditorSystemComponent::Deactivate()
    {
        AzToolsFramework::EditorEvents::Bus::Handler::BusDisconnect();
        ${SanitizedCppName}SystemComponent::Deactivate();
    }

    void ${SanitizedCppName}EditorSystemComponent::NotifyRegisterViews()
    {
        AzToolsFramework::ViewPaneOptions options;
        options.paneRect = QRect(100, 100, 500, 400);
        options.showOnToolsToolbar = true;
        options.toolbarIcon = ":/${Name}/toolbar_icon.svg";

        // Register our custom widget as a dockable tool with the Editor under an Examples sub-menu
        AzToolsFramework::RegisterViewPane<${SanitizedCppName}Widget>("${Name}", "Examples", options);
    }

} // namespace ${SanitizedCppName}
