/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <AzCore/Component/Component.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>

#include <AWSCoreEditorSystemComponent.h>

#include <QMainWindow>
#include <QMenuBar>
#include <QAction>
#include <QList>
#include <QString>

#include <AzToolsFramework/ActionManager/Action/ActionManagerInterface.h>
#include <AzToolsFramework/ActionManager/Menu/MenuManagerInterface.h>
#include <AzToolsFramework/ActionManager/Menu/MenuManagerInternalInterface.h>
#include <AzToolsFramework/Editor/ActionManagerUtils.h>

#include <Editor/Constants/AWSCoreEditorMenuNames.h>
#include <Editor/UI/AWSCoreEditorMenu.h>
#include <QDesktopServices>
#include <QUrl>

namespace AWSCore
{
    void AWSCoreEditorSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<AWSCoreEditorSystemComponent, AZ::Component>()
                ->Version(0)
                ;

            if (AZ::EditContext* ec = serialize->GetEditContext())
            {
                ec->Class<AWSCoreEditorSystemComponent>("AWSCoreEditor", "Adds supporting for working with AWS features in the Editor")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ;
            }
        }
    }

    void AWSCoreEditorSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("AWSCoreEditorService"));
    }

    void AWSCoreEditorSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("AWSCoreEditorService"));
    }

    void AWSCoreEditorSystemComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        AZ_UNUSED(required);
    }

    void AWSCoreEditorSystemComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        AZ_UNUSED(dependent);
    }

    void AWSCoreEditorSystemComponent::Init()
    {
    }

    void AWSCoreEditorSystemComponent::Activate()
    {
        AzToolsFramework::ActionManagerRegistrationNotificationBus::Handler::BusConnect();

        m_actionManagerInterface = AZ::Interface<AzToolsFramework::ActionManagerInterface>::Get();
        AZ_Assert(m_actionManagerInterface, "AWSCoreEditorSystemComponent - could not get ActionManagerInterface");

        m_menuManagerInterface = AZ::Interface<AzToolsFramework::MenuManagerInterface>::Get();
        AZ_Assert(m_menuManagerInterface, "AWSCoreEditorSystemComponent - could not get MenuManagerInterface");

        m_menuManagerInternalInterface = AZ::Interface<AzToolsFramework::MenuManagerInternalInterface>::Get();
        AZ_Assert(m_menuManagerInterface, "AWSCoreEditorSystemComponent - could not get MenuManagerInternalInterface");

        AWSCoreEditorRequestBus::Handler::BusConnect();
    }

    void AWSCoreEditorSystemComponent::Deactivate()
    {
        AzToolsFramework::ActionManagerRegistrationNotificationBus::Handler::BusDisconnect();

        AWSCoreEditorRequestBus::Handler::BusDisconnect();

        m_awsCoreEditorMenu.reset();
    }

    void AWSCoreEditorSystemComponent::OnMenuBarRegistrationHook()
    {
        m_awsCoreEditorMenu = AZStd::make_unique<AWSCoreEditorMenu>();
    }

    void AWSCoreEditorSystemComponent::OnMenuBindingHook()
    {
        m_awsCoreEditorMenu->UpdateMenuBinding();
    }

    void AWSCoreEditorSystemComponent::AddExternalLinkAction(const AZStd::string& menuIdentifier, const char* const actionDetails[], int sort)
    {
        const auto& identifier = actionDetails[IdentIndex];
        const auto& text = actionDetails[NameIndex];
        const auto& icon = actionDetails[IconIndex];
        const auto& url = actionDetails[URLIndex];

        AzToolsFramework::ActionProperties actionProperties;
        actionProperties.m_name = text;
        actionProperties.m_iconPath = icon;
        auto outcome = m_actionManagerInterface->RegisterAction(ActionContext, identifier, actionProperties, [url]()
            {
                QDesktopServices::openUrl(QUrl(url));
            });
        AZ_Assert(outcome.IsSuccess(), "Failed to register action %s", identifier);

        outcome = m_menuManagerInterface->AddActionToMenu(menuIdentifier, identifier, sort);
        AZ_Assert(outcome.IsSuccess(), "Failed to add action %s to menu %s", identifier, menuIdentifier.c_str());
    }

    void AWSCoreEditorSystemComponent::CreateSubMenu(const AZStd::string& parentMenuIdentifier, const char* const menuDetails[], int sort)
    {
        AzToolsFramework::MenuProperties menuProperties;
        menuProperties.m_name = menuDetails[NameIndex];
        auto outcome = m_menuManagerInterface->RegisterMenu(menuDetails[IdentIndex], menuProperties);
        AZ_Assert(outcome.IsSuccess(), "Failed to register '%s' Menu", menuDetails[IdentIndex]);

        QMenu* menu = m_menuManagerInternalInterface->GetMenu(menuDetails[IdentIndex]);
        menu->setProperty("noHover", true);

        outcome = m_menuManagerInterface->AddSubMenuToMenu(parentMenuIdentifier, menuDetails[IdentIndex], sort);
        AZ_Assert(outcome.IsSuccess(), "Failed to add '%s' SubMenu to '%s' Menu", menuDetails[IdentIndex], parentMenuIdentifier.c_str());

    }
} // namespace AWSCore
