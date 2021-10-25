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
#include <Editor/UI/AWSCoreEditorMenu.h>

#include <QMainWindow>
#include <QMenuBar>
#include <QAction>
#include <QList>
#include <QString>

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
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("System"))
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
        m_awsCoreEditorManager = AZStd::make_unique<AWSCoreEditorManager>();
    }

    void AWSCoreEditorSystemComponent::Activate()
    {
        AzToolsFramework::EditorEvents::Bus::Handler::BusConnect();
    }

    void AWSCoreEditorSystemComponent::Deactivate()
    {
        AzToolsFramework::EditorEvents::Bus::Handler::BusDisconnect();
    }

    void AWSCoreEditorSystemComponent::NotifyMainWindowInitialized(QMainWindow* mainWindow)
    {
        QMenuBar* menuBar = mainWindow->menuBar();
        QList<QAction*> actionList = menuBar->actions();
        QAction* insertPivot = nullptr;
        for (QList<QAction*>::iterator itr = actionList.begin(); itr != actionList.end(); ++itr)
        {
            if (QString::compare((*itr)->text(), EDITOR_HELP_MENU_TEXT) == 0)
            {
                insertPivot = (*itr);
                break;
            }
        }

        const auto menu = m_awsCoreEditorManager->GetAWSCoreEditorMenu();
        if (insertPivot)
        {
            menuBar->insertMenu(insertPivot, menu);
        }
        else
        {
            menuBar->addMenu(menu);
            AZ_Warning("AWSCoreEditorSystemComponent", false, "Failed to find Help menu, append cloud services menu to menubar directly.");
        }
    }
} // namespace AWSCore
