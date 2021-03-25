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
#include <AzFramework/StringFunc/StringFunc.h>

#include <AWSCoreEditor_Traits_Platform.h>
#include <Editor/UI/AWSCoreEditorMenu.h>

#include <QAction>
#include <QDesktopServices>
#include <QIcon>
#include <QList>
#include <QObject>
#include <QProcess>
#include <QString>
#include <QUrl>

namespace AWSCore
{
    AWSCoreEditorMenu::AWSCoreEditorMenu(const QString& text)
        : QMenu(text)
        , m_engineRootFolder("")
        , m_resourceMappintToolIsRunning(false)
    {
        InitializeEngineRootFolder();

#ifdef AWSCORE_EDITOR_RESOURCE_MAPPING_TOOL_ENABLED
        InitializeResourceMappingToolAction();
        this->addSeparator();
#endif
        InitializeAWSDocActions();
        this->addSeparator();
        InitializeAWSFeatureGemActions();

        AWSCoreEditorRequestBus::Handler::BusConnect();
    }

    AWSCoreEditorMenu::~AWSCoreEditorMenu()
    {
        AWSCoreEditorRequestBus::Handler::BusDisconnect();

        if (m_resourceMappingToolThread.joinable())
        {
            m_resourceMappingToolThread.join();
        }
    }

    void AWSCoreEditorMenu::InitializeEngineRootFolder()
    {
        auto engineRootFolder = AZ::IO::FileIOBase::GetInstance()->GetAlias("@engroot@");
        if (!engineRootFolder)
        {
            AZ_Error("AWSCoreEditorMenu", false, "Failed to initialize engine root folder path.");
        }
        else
        {
            m_engineRootFolder = engineRootFolder;
        }
    }

    void AWSCoreEditorMenu::InitializeResourceMappingToolAction()
    {
        QAction* resourceMappingAction = new QAction(QObject::tr(AWSResourceMappingToolActionText));
        QObject::connect(resourceMappingAction, &QAction::triggered, this, [this]() {
            AZStd::lock_guard<AZStd::mutex> lockGuard{m_resourceMappingToolMutex};
            if (!m_resourceMappintToolIsRunning)
            {
                m_resourceMappintToolIsRunning = true;
                if (m_resourceMappingToolThread.joinable())
                {
                    m_resourceMappingToolThread.join();
                }
                m_resourceMappingToolThread = AZStd::thread(AZStd::bind(&AWSCoreEditorMenu::StartResourceMappingProcess, this));
            }
            else
            {
                AZ_Warning("AWSCoreEditorMenu", false, "Resource Mapping Tool is already running...");
            }
        });
        this->addAction(resourceMappingAction);
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

    void AWSCoreEditorMenu::StartResourceMappingProcess()
    {
        AZStd::string toolScriptPath = AZStd::string::format("%s/%s", m_engineRootFolder.c_str(), ResourceMappingToolPath);
        AzFramework::StringFunc::Path::Normalize(toolScriptPath);
        QProcess resourceMappingToolProcess;
        resourceMappingToolProcess.setProgram("cmd.exe");
        resourceMappingToolProcess.setArguments({"/C", toolScriptPath.c_str()});

        resourceMappingToolProcess.start();
        while (!resourceMappingToolProcess.waitForFinished())
        {
            if (resourceMappingToolProcess.state() != QProcess::Running)
            {
                break;
            }
        }
        if (resourceMappingToolProcess.exitCode() != 0)
        {
            AZ_Error("AWSCoreEditorMenu", false,
                "Failed to launch Resource Mapping Tool, please follow README to setup tool before using it.");
        }
        AZStd::lock_guard<AZStd::mutex> lockGuard{m_resourceMappingToolMutex};
        m_resourceMappintToolIsRunning = false;
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
