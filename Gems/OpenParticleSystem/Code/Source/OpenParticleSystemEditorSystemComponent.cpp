/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/SerializeContext.h>
#include <OpenParticleSystemEditorSystemComponent.h>
#include <AzCore/Name/Name.h>

#include <Atom/RHI/Factory.h>
#include <AzCore/Utils/Utils.h>
#include <AzToolsFramework/API/ViewPaneOptions.h>
#include <OpenParticleSystemEditor/Window/OpenParticleSystemEditorWindow.h>
#include <LyViewPaneNames.h>

namespace OpenParticleSystem
{
    void OpenParticleSystemEditorSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<OpenParticleSystemEditorSystemComponent, OpenParticleSystemSystemComponent>()
                ->Version(0);
        }
    }

    OpenParticleSystemEditorSystemComponent::OpenParticleSystemEditorSystemComponent() = default;

    OpenParticleSystemEditorSystemComponent::~OpenParticleSystemEditorSystemComponent() = default;

    void OpenParticleSystemEditorSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        BaseSystemComponent::GetProvidedServices(provided);
        provided.push_back(AZ_CRC_CE("OpenParticleSystemEditorService"));
    }

    void OpenParticleSystemEditorSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        BaseSystemComponent::GetIncompatibleServices(incompatible);
        incompatible.push_back(AZ_CRC_CE("OpenParticleSystemEditorService"));
    }

    void OpenParticleSystemEditorSystemComponent::GetRequiredServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        BaseSystemComponent::GetRequiredServices(required);
    }

    void OpenParticleSystemEditorSystemComponent::GetDependentServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        BaseSystemComponent::GetDependentServices(dependent);
    }

    void OpenParticleSystemEditorSystemComponent::Activate()
    {
        OpenParticleSystemSystemComponent::Activate();
        AzToolsFramework::EditorEvents::Bus::Handler::BusConnect();

        AzToolsFramework::EditorMenuNotificationBus::Handler::BusConnect();
    }

    void OpenParticleSystemEditorSystemComponent::Deactivate()
    {
        AzToolsFramework::EditorEvents::Bus::Handler::BusDisconnect();
        OpenParticleSystemSystemComponent::Deactivate();

        AzToolsFramework::EditorMenuNotificationBus::Handler::BusDisconnect();
    }

    void OpenParticleSystemEditorSystemComponent::OnResetToolMenuItems()
    {
    }

    void OpenParticleSystemEditorSystemComponent::NotifyRegisterViews()
    {
        QtViewOptions options;
        options.canHaveMultipleInstances = false;
        options.isPreview = true;
        options.showInMenu = true;
        options.showOnToolsToolbar = true;
        options.toolbarIcon = ":/stylesheet/img/UI20/toolbar/particle.svg";

        AzToolsFramework::RegisterViewPane<OpenParticleSystemEditor::OpenParticleSystemEditorWindow>(
            "(Preview) Particle Editor", LyViewPane::CategoryTools, options);
    }

} // namespace OpenParticleSystem
