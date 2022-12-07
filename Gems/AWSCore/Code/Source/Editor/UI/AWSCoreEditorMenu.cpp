/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Debug/Trace.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/Jobs/JobFunction.h>
#include <AzCore/Settings/SettingsRegistry.h>
#include <AzFramework/Process/ProcessWatcher.h>
#include <AzFramework/StringFunc/StringFunc.h>

#include <AWSCoreEditor_Traits_Platform.h>
#include <Editor/Constants/AWSCoreEditorMenuLinks.h>
#include <Editor/Constants/AWSCoreEditorMenuNames.h>
#include <Editor/UI/AWSCoreEditorMenu.h>
#include <Editor/UI/AWSCoreResourceMappingToolAction.h>

#include <QAction>
#include <QApplication>
#include <QDesktopServices>
#include <QIcon>
#include <QList>
#include <QMessageBox>
#include <QObject>
#include <QString>
#include <QUrl>

#include <AzToolsFramework/ActionManager/Action/ActionManagerInterface.h>
#include <AzToolsFramework/ActionManager/Menu/MenuManagerInterface.h>
#include <AzToolsFramework/ActionManager/Menu/MenuManagerInternalInterface.h>

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
        AZ_Assert(outcome.IsSuccess(), "Failed to register '%s' Menu", AWSMenuIdentifier);

        outcome = m_menuManagerInterface->AddMenuToMenuBar(EditorMainWindowMenuBarIdentifier, AWSMenuIdentifier, 1000);
        AZ_Assert(outcome.IsSuccess(), "Failed to add '%s' Menu to '%s' MenuBar", AWSMenuIdentifier, EditorMainWindowMenuBarIdentifier);

        InitializeAWSDocActions();
        InitializeResourceMappingToolAction();

        m_menuManagerInterface->AddSeparatorToMenu(AWSMenuIdentifier, 0);
        
        InitializeAWSFeatureGemActions();
        //AddSpaceForIcon(this);

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
        //this->clear();
    }

    void AWSCoreEditorMenu::InitializeResourceMappingToolAction()
    {
//#if AWSCORE_EDITOR_RESOURCE_MAPPING_TOOL_ENABLED
//        AWSCoreResourceMappingToolAction* resourceMappingTool =
//            new AWSCoreResourceMappingToolAction(QObject::tr(AWSResourceMappingToolActionText), this);
//        QObject::connect(resourceMappingTool, &QAction::triggered, this,
//            [resourceMappingTool, this]() {
//                AZStd::string launchCommand = resourceMappingTool->GetToolLaunchCommand();
//                if (launchCommand.empty())
//                {
//                    AZStd::string resourceMappingToolReadMePath = resourceMappingTool->GetToolReadMePath();
//                    AZStd::string message = AZStd::string::format(AWSResourceMappingToolReadMeWarningText, resourceMappingToolReadMePath.c_str());
//                    QMessageBox::warning(QApplication::activeWindow(), "Warning", message.c_str(), QMessageBox::Ok);
//                    return;
//                }
//                if (m_resourceMappingToolWatcher && m_resourceMappingToolWatcher->IsProcessRunning())
//                {
//                    QMessageBox::information(QApplication::activeWindow(), "Info", AWSResourceMappingToolIsRunningText, QMessageBox::Ok);
//                    return;
//                }
//                if (m_resourceMappingToolWatcher)
//                {
//                    m_resourceMappingToolWatcher.reset();
//                }
//
//                AzFramework::ProcessLauncher::ProcessLaunchInfo processLaunchInfo;
//                processLaunchInfo.m_commandlineParameters = launchCommand;
//                processLaunchInfo.m_showWindow = false;
//                m_resourceMappingToolWatcher = AZStd::unique_ptr<AzFramework::ProcessWatcher>(
//                    AzFramework::ProcessWatcher::LaunchProcess(processLaunchInfo, AzFramework::ProcessCommunicationType::COMMUNICATOR_TYPE_NONE));
//
//                if (!m_resourceMappingToolWatcher || !m_resourceMappingToolWatcher->IsProcessRunning())
//                {
//                    AZStd::string resourceMappingToolLogPath = resourceMappingTool->GetToolLogFilePath();
//                    AZStd::string message = AZStd::string::format(AWSResourceMappingToolLogWarningText, resourceMappingToolLogPath.c_str());
//                    QMessageBox::warning(QApplication::activeWindow(), "Warning", message.c_str(), QMessageBox::Ok);
//                }
//        });
        //this->addAction(resourceMappingTool);
        //this->addSeparator();
//#endif
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

        // todo-ls: still need this
        //AddSpaceForIcon(globalDocsMenu);
    }

    void AWSCoreEditorMenu::InitializeAWSFeatureGemActions()
    {
        //QAction* clientAuth = new QAction(QObject::tr(AWSClientAuth[NameIndex]));
        //clientAuth->setIcon(QIcon(QString(AWSClientAuth[IconIndex])));
        //clientAuth->setDisabled(true);
        //this->addAction(clientAuth);

        /*AzToolsFramework::ActionProperties actionProperties;
        actionProperties.m_name = AWSClientAuth[NameIndex];
        actionProperties.m_iconPath = AWSClientAuth[IconIndex];
        actionProperties.m_hideFromMenusWhenDisabled = false;

        auto outcome = m_actionManagerInterface->RegisterAction(ActionContext, AWSClientAuth[IdentIndex], actionProperties, []()
            {
                
            });
        AZ_Assert(outcome.IsSuccess(), "Failed to register action %s", AWSClientAuth[IdentIndex]);

        outcome = m_menuManagerInterface->AddActionToMenu(AWSMenuIdentifier, AWSClientAuth[IdentIndex], 0);
        AZ_Assert(outcome.IsSuccess(), "Failed to add action %s to menu %s", AWSClientAuth[IdentIndex], AWSMenuIdentifier);

        m_actionManagerInterface->InstallEnabledStateCallback(
            AWSClientAuth[IdentIndex],
            []() -> bool
            {
                return false;
            }
        );



        actionProperties.m_name = AWSClientAuth[NameIndex];
        actionProperties.m_iconPath = AWSClientAuth[IconIndex];
        actionProperties.m_hideFromMenusWhenDisabled = false;

        outcome = m_actionManagerInterface->RegisterAction(ActionContext, AWSClientAuth[IdentIndex], actionProperties, []()
            {

            });
        AZ_Assert(outcome.IsSuccess(), "Failed to register action %s", AWSClientAuth[IdentIndex]);

        outcome = m_menuManagerInterface->AddActionToMenu(AWSMenuIdentifier, AWSClientAuth[IdentIndex], 0);
        AZ_Assert(outcome.IsSuccess(), "Failed to add action %s to menu %s", AWSClientAuth[IdentIndex], AWSMenuIdentifier);

        m_actionManagerInterface->InstallEnabledStateCallback(
            AWSClientAuth[IdentIndex],
            []() -> bool
            {
                return false;
            }
        );*/


        //CreateSubMenu(AWSMenuIdentifier, AWSMetrics);

        //QAction* metrics = new QAction(QObject::tr(AWSMetricsActionText));
        //metrics->setIcon(QIcon(QString(":/Notifications/download.svg")));
        //metrics->setDisabled(true);
        //this->addAction(metrics);

        //QAction* gamelift = new QAction(QObject::tr(AWSGameLiftActionText));
        //gamelift->setIcon(QIcon(QString(":/Notifications/download.svg")));
        //gamelift->setDisabled(true);
        //this->addAction(gamelift);
    }

    //void AWSCoreEditorMenu::SetAWSClientAuthEnabled()
    //{
    //    // Record that this is enabled
    //    m_awsClientAuthEnabled = true;


    //    //// TODO: instead of creating submenu in core editor, aws feature gem should return submenu component directly
    //    //QMenu* subMenu = SetAWSFeatureSubMenu(AWSClientAuthActionText);

    //    //CreateSubMenu(AWSMenuIdentifier, AWSClientAuth);

    //    //const auto& submenuIdentifier = AWSClientAuth[IdentIndex];

    //    //AddExternalLinkAction(submenuIdentifier, AWSClientAuthGemOverview);

    //    ///*subMenu->addAction(AddExternalLinkAction(
    //    //    AWSClientAuthGemOverviewActionText, AWSClientAuthGemOverviewUrl, ":/Notifications/link.svg"));
    //    //subMenu->addAction(AddExternalLinkAction(
    //    //    AWSClientAuthGemSetupActionText, AWSClientAuthGemSetupUrl, ":/Notifications/link.svg"));
    //    //subMenu->addAction(AddExternalLinkAction(
    //    //    AWSClientAuthCDKAndResourcesActionText, AWSClientAuthCDKAndResourcesUrl, ":/Notifications/link.svg"));
    //    //subMenu->addAction(AddExternalLinkAction(
    //    //    AWSClientAuthScriptCanvasAndLuaActionText, AWSClientAuthScriptCanvasAndLuaUrl, ":/Notifications/link.svg"));
    //    //subMenu->addAction(AddExternalLinkAction(
    //    //    AWSClientAuth3rdPartyAuthProviderActionText, AWSClientAuth3rdPartyAuthProviderUrl, ":/Notifications/link.svg"));
    //    //subMenu->addAction(AddExternalLinkAction(
    //    //    AWSClientAuthCustomAuthProviderActionText, AWSClientAuthCustomAuthProviderUrl, ":/Notifications/link.svg"));
    //    //subMenu->addAction(AddExternalLinkAction(
    //    //    AWSClientAuthAPIReferenceActionText, AWSClientAuthAPIReferenceUrl, ":/Notifications/link.svg"));*/

    //    //AddSpaceForIcon(subMenu);
    //}

    //void AWSCoreEditorMenu::SetAWSGameLiftEnabled()
    //{
    //    // TODO: instead of creating submenu in core editor, aws feature gem should return submenu component directly
    //    QMenu* subMenu = SetAWSFeatureSubMenu(AWSGameLiftActionText);

    //    CreateSubMenu(AWSMenuIdentifier, AWSGameLift);

    //    const auto& submenuIdentifier = AWSGameLift[IdentIndex];

    //    AddExternalLinkAction(submenuIdentifier, AWSGameLiftOverview);
    //    /*subMenu->addAction(AddExternalLinkAction(AWSGameLiftGemOverviewActionText, AWSGameLiftGemOverviewUrl, ":/Notifications/link.svg"));
    //    subMenu->addAction(AddExternalLinkAction(AWSGameLiftGemSetupActionText, AWSGameLiftGemSetupUrl, ":/Notifications/link.svg"));
    //    subMenu->addAction(AddExternalLinkAction(AWSMGameLiftScriptingActionText, AWSGameLiftScriptingUrl, ":/Notifications/link.svg"));
    //    subMenu->addAction(AddExternalLinkAction(AWSGameLiftAPIReferenceActionText, AWSGameLiftAPIReferenceUrl, ":/Notifications/link.svg"));
    //    subMenu->addAction(AddExternalLinkAction(AWSGameLiftAdvancedTopicsActionText, AWSGameLiftAdvancedTopicsUrl, ":/Notifications/link.svg"));
    //    subMenu->addAction(AddExternalLinkAction(AWSGameLiftLocalTestingActionText, AWSGameLiftLocalTestingUrl, ":/Notifications/link.svg"));
    //    subMenu->addAction(AddExternalLinkAction(AWSGameLiftBuildPackagingActionText, AWSGameLiftBuildPackagingUrl, ":/Notifications/link.svg"));
    //    subMenu->addAction(AddExternalLinkAction(AWSGameLiftResourceManagementActionText, AWSGameLiftResourceManagementUrl, ":/Notifications/link.svg"));*/

    //    AddSpaceForIcon(subMenu);
    //}

    //void AWSCoreEditorMenu::SetAWSMetricsEnabled()
    //{
    //    // TODO: instead of creating submenu in core editor, aws feature gem should return submenu component directly
    //    //QMenu* subMenu = SetAWSFeatureSubMenu(AWSMetricsActionText);

    //    CreateSubMenu(AWSMenuIdentifier, AWSMetrics);

    //    const auto& submenuIdentifier = AWSMetrics[IdentIndex];

    //    AddExternalLinkAction(submenuIdentifier, AWSMetricsGemOverview);

    //    /*subMenu->addAction(AddExternalLinkAction(
    //        AWSMetricsGemOverviewActionText, AWSMetricsGemOverviewUrl, ":/Notifications/link.svg"));
    //    subMenu->addAction(AddExternalLinkAction(
    //        AWSMetricsSetupGemActionText, AWSMetricsSetupGemUrl, ":/Notifications/link.svg"));
    //    subMenu->addAction(AddExternalLinkAction(
    //        AWSMetricsScriptingActionText, AWSMetricsScriptingUrl, ":/Notifications/link.svg"));
    //    subMenu->addAction(AddExternalLinkAction(
    //        AWSMetricsAPIReferenceActionText, AWSMetricsAPIReferenceUrl, ":/Notifications/link.svg"));
    //    subMenu->addAction(AddExternalLinkAction(
    //        AWSMetricsAdvancedTopicsActionText, AWSMetricsAdvancedTopicsUrl, ":/Notifications/link.svg"));*/

    //    AZStd::string priorAlias = AZ::IO::FileIOBase::GetInstance()->GetAlias("@engroot@");
    //    AZStd::string configFilePath = priorAlias + "\\Gems\\AWSMetrics\\Code\\" + AZ::SettingsRegistryInterface::RegistryFolder;
    //    AzFramework::StringFunc::Path::Normalize(configFilePath);

    //    /*QAction* settingsAction = new QAction(QObject::tr(AWSMetricsSettingsActionText));
    //    QObject::connect(settingsAction, &QAction::triggered, this,
    //        [configFilePath](){
    //            QDesktopServices::openUrl(QUrl::fromLocalFile(configFilePath.c_str()));
    //        });

    //    subMenu->addAction(settingsAction);*/
    //    //AddSpaceForIcon(subMenu);
    //}

    //QMenu* AWSCoreEditorMenu::SetAWSFeatureSubMenu(const AZStd::string& menuText)
    //{
    //    (void)menuText;
    //    //auto actionList = this->actions();
    //    //for (QList<QAction*>::iterator itr = actionList.begin(); itr != actionList.end(); ++itr)
    //    //{
    //    //    if (QString::compare((*itr)->text(), menuText.c_str()) == 0)
    //    //    {
    //    //        QMenu* subMenu = new QMenu(QObject::tr(menuText.c_str()));
    //    //        subMenu->setIcon(QIcon(QString(":/Notifications/checkmark.svg")));
    //    //        subMenu->setProperty("noHover", true);
    //    //        this->insertMenu(*itr, subMenu);
    //    //        this->removeAction(*itr);
    //    //        return subMenu;
    //    //    }
    //    //}
    //    return nullptr;
    //}

    void AWSCoreEditorMenu::AddSpaceForIcon(QMenu *menu)
    {
        QSize size = menu->sizeHint();
        size.setWidth(size.width() + IconSize);
        menu->setFixedSize(size);
    }
} // namespace AWSCore
