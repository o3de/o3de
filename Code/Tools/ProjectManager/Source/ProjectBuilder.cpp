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

#include <ProjectBuilder.h>
#include <ProjectButtonWidget.h>
#include <PythonBindingsInterface.h>

#include <QProcess>

//#define MOCK_BUILD_PROJECT true 

namespace O3DE::ProjectManager
{
    ProjectBuilderWorker::ProjectBuilderWorker(const QString& projectPath)
        : QObject()
        , m_projectPath(projectPath)
    {
    }

    void ProjectBuilderWorker::BuildProject()
    {
#ifdef MOCK_BUILD_PROJECT
        for (int i = 0; i < 10; ++i)
        {
            QThread::sleep(1);
            UpdateProgress(i * 10);
        }
        Done(m_projectPath);
#else
        static QString standardBuildPath("/windows_vs2019");

        EngineInfo engineInfo;

        AZ::Outcome<EngineInfo> engineInfoResult = PythonBindingsInterface::Get()->GetEngineInfo();
        if (engineInfoResult.IsSuccess())
        {
            engineInfo = engineInfoResult.GetValue();
        }

        QProcess configProjectProcess;
        configProjectProcess.setProcessChannelMode(QProcess::MergedChannels);

        configProjectProcess.start(
            "cmake",
            QStringList
            {
                "-B",
                m_projectPath + standardBuildPath,
                "-S",
                m_projectPath,
                "-G",
                "Visual Studio 16",
                "-DLY_3RDPARTY_PATH=" + engineInfo.m_thirdPartyPath,
            });

        if (!configProjectProcess.waitForStarted())
        {
            emit Done("Failed");
            return;
        }

        while (configProjectProcess.waitForReadyRead())
        {}

        QString vsWhereOutput(configProjectProcess.readAllStandardOutput());
        if (1)
        {
            emit Done("Succeeded");
            return;
        }
#endif
    }

    ProjectBuilderController::ProjectBuilderController(const QString& projectPath, ProjectButton* projectButton, QWidget* parent)
        : QObject()
        , m_projectPath(projectPath)
        , m_projectButton(projectButton)
        , m_parent(parent)
    {
        ProjectBuilderWorker* worker = new ProjectBuilderWorker(projectPath);
        worker->moveToThread(&m_workerThread);

        connect(&m_workerThread, &QThread::finished, worker, &ProjectBuilderWorker::deleteLater);
        connect(&m_workerThread, &QThread::started, worker, &ProjectBuilderWorker::BuildProject);
        connect(worker, &ProjectBuilderWorker::Done, this, &ProjectBuilderController::HandleResults);
        connect(worker, &ProjectBuilderWorker::UpdateProgress, this, &ProjectBuilderController::UpdateUIProgress);
    }

    ProjectBuilderController::~ProjectBuilderController()
    {
        m_workerThread.quit();
        m_workerThread.wait();
    }

    void ProjectBuilderController::Start()
    {
        m_workerThread.start();
        UpdateUIProgress(0);
    }

    void ProjectBuilderController::SetProjectButton(ProjectButton* projectButton)
    {
        m_projectButton = projectButton;
    }

    QString ProjectBuilderController::ProjectPath()
    {
        return m_projectPath;
    }

    void ProjectBuilderController::UpdateUIProgress(int progress)
    {
        m_projectButton->SetButtonOverlayText(QString(tr("Building Project... (%1%)\n\n")).arg(progress));
        m_projectButton->SetProgressBarValue(progress);
    }

    void ProjectBuilderController::HandleResults([[maybe_unused]] const QString& result)
    {
        emit Done();
        return;
    }

} // namespace O3DE::ProjectManager
