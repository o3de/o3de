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

namespace AWSCore
{

    static constexpr int IconSize = 16;

    AWSCoreEditorMenu::AWSCoreEditorMenu(const QString& text)
        : QMenu(text)
        , m_resourceMappingToolWatcher(nullptr)
    {
        InitializeAWSDocActions();
        InitializeResourceMappingToolAction();
        this->addSeparator();
        InitializeAWSFeatureGemActions();
        AddSpaceForIcon(this);

        AWSCoreEditorRequestBus::Handler::BusConnect();
    }

    AWSCoreEditorMenu::~AWSCoreEditorMenu()
    {
        AWSCoreEditorRequestBus::Handler::BusDisconnect();
        if (m_resourceMappingToolWatcher)
        {
            if (m_resourceMappingToolWatcher->IsProcessRunning())
            {
                m_resourceMappingToolWatcher->TerminateProcess(static_cast<AZ::u32>(-1));
            }
            m_resourceMappingToolWatcher.reset();
        }
        this->clear();
    }

    QAction* AWSCoreEditorMenu::AddExternalLinkAction(
        const AZStd::string& name, const AZStd::string& url, const AZStd::string& icon)
    {
        QAction* linkAction = new QAction(QObject::tr(name.c_str()));
        QObject::connect(linkAction, &QAction::triggered, this,
            [url]() {
                QDesktopServices::openUrl(QUrl(url.c_str()));
            });
        if (!icon.empty())
        {
            linkAction->setIcon(QIcon(icon.c_str()));
        }
        return linkAction;
    }

    void AWSCoreEditorMenu::InitializeResourceMappingToolAction()
    {
#if AWSCORE_EDITOR_RESOURCE_MAPPING_TOOL_ENABLED
        AWSCoreResourceMappingToolAction* resourceMappingTool =
            new AWSCoreResourceMappingToolAction(QObject::tr(AWSResourceMappingToolActionText), this);
        QObject::connect(resourceMappingTool, &QAction::triggered, this,
            [resourceMappingTool, this]() {
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
                m_resourceMappingToolWatcher = AZStd::unique_ptr<AzFramework::ProcessWatcher>(
                    AzFramework::ProcessWatcher::LaunchProcess(processLaunchInfo, AzFramework::ProcessCommunicationType::COMMUNICATOR_TYPE_NONE));

                if (!m_resourceMappingToolWatcher || !m_resourceMappingToolWatcher->IsProcessRunning())
                {
                    AZStd::string resourceMappingToolLogPath = resourceMappingTool->GetToolLogFilePath();
                    AZStd::string message = AZStd::string::format(AWSResourceMappingToolLogWarningText, resourceMappingToolLogPath.c_str());
                    QMessageBox::warning(QApplication::activeWindow(), "Warning", message.c_str(), QMessageBox::Ok);
                }
        });
        this->addAction(resourceMappingTool);
        this->addSeparator();
#endif
    }

    void AWSCoreEditorMenu::InitializeAWSDocActions()
    {
        this->addAction(AddExternalLinkAction(NewToAWSActionText, NewToAWSUrl, ":/Notifications/link.svg"));

        InitializeAWSGlobalDocsSubMenu();

        this->addAction(AddExternalLinkAction(
            AWSCredentialConfigurationActionText, AWSCredentialConfigurationUrl, ":/Notifications/link.svg"));
    }

    void AWSCoreEditorMenu::InitializeAWSGlobalDocsSubMenu()
    {
        QMenu* globalDocsMenu = this->addMenu(QObject::tr(AWSAndO3DEGlobalDocsText));

        globalDocsMenu->addAction(
            AddExternalLinkAction(AWSAndO3DEGettingStartedActionText, AWSAndGettingStartedUrl, ":/Notifications/link.svg"));
        globalDocsMenu->addAction(
            AddExternalLinkAction(AWSAndO3DEMappingsFileActionText, AWSAndResourceMappingsUrl, ":/Notifications/link.svg"));
        globalDocsMenu->addAction(
            AddExternalLinkAction(AWSAndO3DEResourceToolActionText, AWSAndResourceMappingToolUrl, ":/Notifications/link.svg"));
        globalDocsMenu->addAction(
            AddExternalLinkAction(AWSAndO3DEScriptingActionText, AWSAndScriptingUrl, ":/Notifications/link.svg"));

        AddSpaceForIcon(globalDocsMenu);
    }

    void AWSCoreEditorMenu::InitializeAWSFeatureGemActions()
    {
        QAction* clientAuth = new QAction(QObject::tr(AWSClientAuthActionText));
        clientAuth->setIcon(QIcon(QString(":/Notifications/download.svg")));
        clientAuth->setDisabled(true);
        this->addAction(clientAuth);

        QAction* metrics = new QAction(QObject::tr(AWSMetricsActionText));
        metrics->setIcon(QIcon(QString(":/Notifications/download.svg")));
        metrics->setDisabled(true);
        this->addAction(metrics);

        QAction* gamelift = new QAction(QObject::tr(AWSGameLiftActionText));
        gamelift->setIcon(QIcon(QString(":/Notifications/download.svg")));
        gamelift->setDisabled(true);
        this->addAction(gamelift);
    }

    void AWSCoreEditorMenu::SetAWSClientAuthEnabled()
    {
        // TODO: instead of creating submenu in core editor, aws feature gem should return submenu component directly
        QMenu* subMenu = SetAWSFeatureSubMenu(AWSClientAuthActionText);

        subMenu->addAction(AddExternalLinkAction(
            AWSClientAuthGemOverviewActionText, AWSClientAuthGemOverviewUrl, ":/Notifications/link.svg"));
        subMenu->addAction(AddExternalLinkAction(
            AWSClientAuthGemSetupActionText, AWSClientAuthGemSetupUrl, ":/Notifications/link.svg"));
        subMenu->addAction(AddExternalLinkAction(
            AWSClientAuthCDKAndResourcesActionText, AWSClientAuthCDKAndResourcesUrl, ":/Notifications/link.svg"));
        subMenu->addAction(AddExternalLinkAction(
            AWSClientAuthScriptCanvasAndLuaActionText, AWSClientAuthScriptCanvasAndLuaUrl, ":/Notifications/link.svg"));
        subMenu->addAction(AddExternalLinkAction(
            AWSClientAuth3rdPartyAuthProviderActionText, AWSClientAuth3rdPartyAuthProviderUrl, ":/Notifications/link.svg"));
        subMenu->addAction(AddExternalLinkAction(
            AWSClientAuthCustomAuthProviderActionText, AWSClientAuthCustomAuthProviderUrl, ":/Notifications/link.svg"));
        subMenu->addAction(AddExternalLinkAction(
            AWSClientAuthAPIReferenceActionText, AWSClientAuthAPIReferenceUrl, ":/Notifications/link.svg"));

        AddSpaceForIcon(subMenu);
    }

    void AWSCoreEditorMenu::SetAWSGameLiftEnabled()
    {
        // TODO: instead of creating submenu in core editor, aws feature gem should return submenu component directly
        QMenu* subMenu = SetAWSFeatureSubMenu(AWSGameLiftActionText);

        subMenu->addAction(AddExternalLinkAction(AWSGameLiftGemOverviewActionText, AWSGameLiftGemOverviewUrl, ":/Notifications/link.svg"));
        subMenu->addAction(AddExternalLinkAction(AWSGameLiftGemSetupActionText, AWSGameLiftGemSetupUrl, ":/Notifications/link.svg"));
        subMenu->addAction(AddExternalLinkAction(AWSMGameLiftScriptingActionText, AWSGameLiftScriptingUrl, ":/Notifications/link.svg"));
        subMenu->addAction(AddExternalLinkAction(AWSGameLiftAPIReferenceActionText, AWSGameLiftAPIReferenceUrl, ":/Notifications/link.svg"));
        subMenu->addAction(AddExternalLinkAction(AWSGameLiftAdvancedTopicsActionText, AWSGameLiftAdvancedTopicsUrl, ":/Notifications/link.svg"));
        subMenu->addAction(AddExternalLinkAction(AWSGameLiftLocalTestingActionText, AWSGameLiftLocalTestingUrl, ":/Notifications/link.svg"));
        subMenu->addAction(AddExternalLinkAction(AWSGameLiftBuildPackagingActionText, AWSGameLiftBuildPackagingUrl, ":/Notifications/link.svg"));
        subMenu->addAction(AddExternalLinkAction(AWSGameLiftResourceManagementActionText, AWSGameLiftResourceManagementUrl, ":/Notifications/link.svg"));

        AddSpaceForIcon(subMenu);
    }

    void AWSCoreEditorMenu::SetAWSMetricsEnabled()
    {
        // TODO: instead of creating submenu in core editor, aws feature gem should return submenu component directly
        QMenu* subMenu = SetAWSFeatureSubMenu(AWSMetricsActionText);

        subMenu->addAction(AddExternalLinkAction(
            AWSMetricsGemOverviewActionText, AWSMetricsGemOverviewUrl, ":/Notifications/link.svg"));
        subMenu->addAction(AddExternalLinkAction(
            AWSMetricsSetupGemActionText, AWSMetricsSetupGemUrl, ":/Notifications/link.svg"));
        subMenu->addAction(AddExternalLinkAction(
            AWSMetricsScriptingActionText, AWSMetricsScriptingUrl, ":/Notifications/link.svg"));
        subMenu->addAction(AddExternalLinkAction(
            AWSMetricsAPIReferenceActionText, AWSMetricsAPIReferenceUrl, ":/Notifications/link.svg"));
        subMenu->addAction(AddExternalLinkAction(
            AWSMetricsAdvancedTopicsActionText, AWSMetricsAdvancedTopicsUrl, ":/Notifications/link.svg"));

        AZStd::string priorAlias = AZ::IO::FileIOBase::GetInstance()->GetAlias("@engroot@");
        AZStd::string configFilePath = priorAlias + "\\Gems\\AWSMetrics\\Code\\" + AZ::SettingsRegistryInterface::RegistryFolder;
        AzFramework::StringFunc::Path::Normalize(configFilePath);

        QAction* settingsAction = new QAction(QObject::tr(AWSMetricsSettingsActionText));
        QObject::connect(settingsAction, &QAction::triggered, this,
            [configFilePath](){
                QDesktopServices::openUrl(QUrl::fromLocalFile(configFilePath.c_str()));
            });

        subMenu->addAction(settingsAction);
        AddSpaceForIcon(subMenu);
    }

    QMenu* AWSCoreEditorMenu::SetAWSFeatureSubMenu(const AZStd::string& menuText)
    {
        auto actionList = this->actions();
        for (QList<QAction*>::iterator itr = actionList.begin(); itr != actionList.end(); ++itr)
        {
            if (QString::compare((*itr)->text(), menuText.c_str()) == 0)
            {
                QMenu* subMenu = new QMenu(QObject::tr(menuText.c_str()));
                subMenu->setIcon(QIcon(QString(":/Notifications/checkmark.svg")));
                subMenu->setProperty("noHover", true);
                this->insertMenu(*itr, subMenu);
                this->removeAction(*itr);
                return subMenu;
            }
        }
        return nullptr;
    }

    void AWSCoreEditorMenu::AddSpaceForIcon(QMenu *menu)
    {
        QSize size = menu->sizeHint();
        size.setWidth(size.width() + IconSize);
        menu->setFixedSize(size);
    }
} // namespace AWSCore
