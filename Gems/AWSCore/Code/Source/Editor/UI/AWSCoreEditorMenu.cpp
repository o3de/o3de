/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/Process/ProcessWatcher.h>

#include <Editor/Constants/AWSCoreEditorMenuNames.h>

#include <Editor/UI/AWSCoreEditorMenu.h>
#include <Editor/UI/AWSCoreResourceMappingToolAction.h>

#include <AzToolsFramework/ActionManager/Action/ActionManagerInterface.h>
#include <AzToolsFramework/ActionManager/Menu/MenuManagerInterface.h>
#include <AzToolsFramework/ActionManager/Menu/MenuManagerInternalInterface.h>

#include <AzCore/Interface/Interface.h>

#include <QApplication>
#include <QMessageBox>

namespace AWSCore
{
    AWSCoreEditorMenu::AWSCoreEditorMenu()
        : m_resourceMappingToolWatcher(nullptr)
    {
        m_actionManagerInterface = AZ::Interface<AzToolsFramework::ActionManagerInterface>::Get();
        AZ_Assert(m_actionManagerInterface, "AWSCoreEditorSystemComponent - could not get ActionManagerInterface");

        m_menuManagerInterface = AZ::Interface<AzToolsFramework::MenuManagerInterface>::Get();
        AZ_Assert(m_menuManagerInterface, "AWSCoreEditorSystemComponent - could not get MenuManagerInterface");

        m_menuManagerInternalInterface = AZ::Interface<AzToolsFramework::MenuManagerInternalInterface>::Get();
        AZ_Assert(m_menuManagerInterface, "AWSCoreEditorSystemComponent - could not get MenuManagerInternalInterface");

        AzToolsFramework::MenuProperties menuProperties;
        menuProperties.m_name = AWS_MENU_TEXT;
        auto outcome = m_menuManagerInterface->RegisterMenu(AWSMenuIdentifier, menuProperties);
        AZ_Assert(outcome.IsSuccess(), "Failed to register '%s' Menu", AWSMenuIdentifier.data());

    }

    void AWSCoreEditorMenu::UpdateMenuBinding()
    {
        // Get the sort key for the "Help" menu option
        auto sortKeyResult = m_menuManagerInterface->GetSortKeyOfMenuInMenuBar(EditorMainWindowMenuBarIdentifier, HelpMenuIdentifier);
        int sortKey = 1000;
        if (sortKeyResult.IsSuccess())
        {
            sortKey = sortKeyResult.GetValue() - 1;
        }

        auto outcome = m_menuManagerInterface->AddMenuToMenuBar(EditorMainWindowMenuBarIdentifier, AWSMenuIdentifier, sortKey);
        AZ_Assert(outcome.IsSuccess(), "Failed to add '%s' Menu to '%s' MenuBar", AWSMenuIdentifier.data(), EditorMainWindowMenuBarIdentifier.data());

        InitializeAWSDocActions();
        InitializeResourceMappingToolAction();

        m_menuManagerInterface->AddSeparatorToMenu(AWSMenuIdentifier, 0);
    }

    AWSCoreEditorMenu::~AWSCoreEditorMenu()
    {
        if (m_resourceMappingToolWatcher)
        {
            if (m_resourceMappingToolWatcher->IsProcessRunning())
            {
                m_resourceMappingToolWatcher->TerminateProcess(static_cast<AZ::u32>(-1));
            }
            m_resourceMappingToolWatcher.reset();
        }
    }

    void AWSCoreEditorMenu::InitializeResourceMappingToolAction()
    {
#if AWSCORE_EDITOR_RESOURCE_MAPPING_TOOL_ENABLED
        AWSCoreResourceMappingToolAction* resourceMappingTool =
            new AWSCoreResourceMappingToolAction(QObject::tr(AWSResourceMappingTool[NameIndex]), nullptr);

        AzToolsFramework::ActionProperties actionProperties;
        actionProperties.m_name = AWSResourceMappingTool[NameIndex];
        auto outcome = m_actionManagerInterface->RegisterAction(AWSCore::ActionContext, AWSResourceMappingTool[IdentIndex],
            actionProperties,
            [resourceMappingTool, this]()
            {
                AZStd::string launchCommand = resourceMappingTool->GetToolLaunchCommand();
                if (launchCommand.empty())
                {
                    AZStd::string resourceMappingToolReadMePath = resourceMappingTool->GetToolReadMePath();
                    AZStd::string message = AZStd::string::format(AWSResourceMappingToolReadMeWarningText, resourceMappingToolReadMePath.c_str());
                    QMessageBox::warning(QApplication::activeWindow(), "Warning", message.c_str(), QMessageBox::Ok);
                    return;
                }
                if (m_resourceMappingToolWatcher && m_resourceMappingToolWatcher->IsProcessRunning())
                {
                    QMessageBox::information(QApplication::activeWindow(), "Info", AWSResourceMappingToolIsRunningText, QMessageBox::Ok);
                    return;
                }
                if (m_resourceMappingToolWatcher)
                {
                    m_resourceMappingToolWatcher.reset();
                }

                AzFramework::ProcessLauncher::ProcessLaunchInfo processLaunchInfo;
                processLaunchInfo.m_commandlineParameters = launchCommand;
                processLaunchInfo.m_showWindow = false;
                processLaunchInfo.m_tetherLifetime = true;
                m_resourceMappingToolWatcher = AZStd::unique_ptr<AzFramework::ProcessWatcher>(
                    AzFramework::ProcessWatcher::LaunchProcess(processLaunchInfo, AzFramework::ProcessCommunicationType::COMMUNICATOR_TYPE_NONE));

                if (!m_resourceMappingToolWatcher || !m_resourceMappingToolWatcher->IsProcessRunning())
                {
                    AZStd::string resourceMappingToolLogPath = resourceMappingTool->GetToolLogFilePath();
                    AZStd::string message = AZStd::string::format(AWSResourceMappingToolLogWarningText, resourceMappingToolLogPath.c_str());
                    QMessageBox::warning(QApplication::activeWindow(), "Warning", message.c_str(), QMessageBox::Ok);
                }
            }
        );
        AZ_Assert(outcome.IsSuccess(), "Failed to register action %s", AWSResourceMappingTool[IdentIndex]);

        m_menuManagerInterface->AddActionToMenu(AWSMenuIdentifier, AWSResourceMappingTool[IdentIndex], 0);

        m_menuManagerInterface->AddSeparatorToMenu(AWSMenuIdentifier, 0);

#endif
    }

    void AWSCoreEditorMenu::InitializeAWSDocActions()
    {
        AWSCore::AWSCoreEditorRequestBus::Broadcast(&AWSCore::AWSCoreEditorRequests::AddExternalLinkAction, AWSMenuIdentifier, NewToAWS, 0);

        InitializeAWSGlobalDocsSubMenu();

        AWSCore::AWSCoreEditorRequestBus::Broadcast(&AWSCore::AWSCoreEditorRequests::AddExternalLinkAction, AWSMenuIdentifier, AWSCredentialConfiguration, 0);

    }

    void AWSCoreEditorMenu::InitializeAWSGlobalDocsSubMenu()
    {
        AWSCore::AWSCoreEditorRequestBus::Broadcast(&AWSCore::AWSCoreEditorRequests::CreateSubMenu, AWSMenuIdentifier, O3DEAndAWS, 0);

        const auto& submenuIdentifier = O3DEAndAWS[IdentIndex];

        AWSCore::AWSCoreEditorRequestBus::Broadcast(&AWSCore::AWSCoreEditorRequests::AddExternalLinkAction, submenuIdentifier, AWSAndO3DEGettingStarted, 0);
        AWSCore::AWSCoreEditorRequestBus::Broadcast(&AWSCore::AWSCoreEditorRequests::AddExternalLinkAction, submenuIdentifier, AWSAndO3DEMappingsFile, 0);
        AWSCore::AWSCoreEditorRequestBus::Broadcast(&AWSCore::AWSCoreEditorRequests::AddExternalLinkAction, submenuIdentifier, AWSAndO3DEMappingsTool, 0);
        AWSCore::AWSCoreEditorRequestBus::Broadcast(&AWSCore::AWSCoreEditorRequests::AddExternalLinkAction, submenuIdentifier, AWSAndO3DEScripting, 0);
        AWSCore::AWSCoreEditorRequestBus::Broadcast(&AWSCore::AWSCoreEditorRequests::AddExternalLinkAction, submenuIdentifier, AWSGameLift, 0);
        AWSCore::AWSCoreEditorRequestBus::Broadcast(&AWSCore::AWSCoreEditorRequests::AddExternalLinkAction, submenuIdentifier, AWSSupport, 0);

    }

} // namespace AWSCore
