/*
 * All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
 * its licensors.
 *
 * For complete copyright and license terms please see the LICENSE at the root of this
 * distribution (the "License"). All use of this software is governed by the License,
 * or, if provided, by the license below or the license accompanying this file. Do not
 * remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *
 */

#include <AzCore/Debug/Trace.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/Jobs/JobFunction.h>
#include <AzFramework/Process/ProcessWatcher.h>
#include <AzFramework/StringFunc/StringFunc.h>

#include <AWSCoreEditor_Traits_Platform.h>
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
    AWSCoreEditorMenu::AWSCoreEditorMenu(const QString& text)
        : QMenu(text)
        , m_resourceMappingToolWatcher(nullptr)
    {
        InitializeResourceMappingToolAction();
        InitializeAWSDocActions();
        this->addSeparator();
        InitializeAWSFeatureGemActions();

        AWSCoreEditorRequestBus::Handler::BusConnect();
    }

    AWSCoreEditorMenu::~AWSCoreEditorMenu()
    {
        AWSCoreEditorRequestBus::Handler::BusDisconnect();
        if (m_resourceMappingToolWatcher)
        {
            if (m_resourceMappingToolWatcher->IsProcessRunning())
            {
                m_resourceMappingToolWatcher->TerminateProcess(AZ::u32(-1));
            }
            m_resourceMappingToolWatcher.reset();
        }
        this->clear();
    }

    void AWSCoreEditorMenu::InitializeResourceMappingToolAction()
    {
#ifdef AWSCORE_EDITOR_RESOURCE_MAPPING_TOOL_ENABLED
        AWSCoreResourceMappingToolAction* resourceMappingTool =
            new AWSCoreResourceMappingToolAction(QObject::tr(AWSResourceMappingToolActionText));
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
                    AZStd::string resourceMappingToolLogPath = resourceMappingTool->GetToolLogPath();
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
        QAction* credentialConfiguration = new QAction(QObject::tr(CredentialConfigurationActionText));
        QObject::connect(credentialConfiguration, &QAction::triggered, this, []() {
            QDesktopServices::openUrl(QUrl(CredentialConfigurationUrl));
        });
        this->addAction(credentialConfiguration);

        QAction* newToAWS = new QAction(QObject::tr(NewToAWSActionText));
        QObject::connect(newToAWS, &QAction::triggered, this, []() {
            QDesktopServices::openUrl(QUrl(NewToAWSUrl)); });
        this->addAction(newToAWS);

        QAction* awsAndScriptCanvas = new QAction(QObject::tr(AWSAndScriptCanvasActionText));
        QObject::connect(awsAndScriptCanvas, &QAction::triggered, this, []() {
            QDesktopServices::openUrl(QUrl(AWSAndScriptCanvasUrl)); });
        this->addAction(awsAndScriptCanvas);
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
    }

    void AWSCoreEditorMenu::SetAWSClientAuthEnabled()
    {
        SetAWSFeatureActionsEnabled(AWSClientAuthActionText);
    }

    void AWSCoreEditorMenu::SetAWSMetricsEnabled()
    {
        SetAWSFeatureActionsEnabled(AWSMetricsActionText);
    }

    void AWSCoreEditorMenu::SetAWSFeatureActionsEnabled(const AZStd::string actionText)
    {
        auto actionList = this->actions();
        for (QList<QAction*>::iterator itr = actionList.begin(); itr != actionList.end(); itr++)
        {
            if (QString::compare((*itr)->text(), actionText.c_str()) == 0)
            {
                (*itr)->setIcon(QIcon(QString(":/Notifications/checkmark.svg")));
                (*itr)->setEnabled(true);
                break;
            }
        }
    }
} // namespace AWSCore
