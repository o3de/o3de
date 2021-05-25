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

#include <ProjectManagerWindow.h>
#include <ScreenFactory.h>

#include <AzQtComponents/Components/StyleManager.h>
#include <AzCore/IO/Path/Path.h>

#include <QDir>

#include <Source/ui_ProjectManagerWindow.h>

namespace O3DE::ProjectManager
{
    ProjectManagerWindow::ProjectManagerWindow(QWidget* parent, const AZ::IO::PathView& engineRootPath)
        : QMainWindow(parent)
        , m_ui(new Ui::ProjectManagerWindowClass())
    {
        m_ui->setupUi(this);
        QLayout* layout = m_ui->centralWidget->layout();
        layout->setMargin(0);
        layout->setSpacing(0);
        layout->setContentsMargins(0, 0, 0, 0);

        setFixedSize(this->geometry().width(), this->geometry().height());

        m_pythonBindings = AZStd::make_unique<PythonBindings>(engineRootPath);

        m_screensCtrl = new ScreensCtrl();
        m_ui->verticalLayout->addWidget(m_screensCtrl);

        connect(m_ui->projectsMenu, &QMenu::aboutToShow, this, &ProjectManagerWindow::HandleProjectsMenu);
        connect(m_ui->engineMenu, &QMenu::aboutToShow, this, &ProjectManagerWindow::HandleEngineMenu);

        QDir rootDir = QString::fromUtf8(engineRootPath.Native().data(), aznumeric_cast<int>(engineRootPath.Native().size()));
        const auto pathOnDisk = rootDir.absoluteFilePath("Code/Tools/ProjectManager/Resources");
        const auto qrcPath = QStringLiteral(":/ProjectManager/style");
        AzQtComponents::StyleManager::addSearchPaths("style", pathOnDisk, qrcPath, engineRootPath);

        AzQtComponents::StyleManager::setStyleSheet(this, QStringLiteral("style:ProjectManager.qss"));

        QVector<ProjectManagerScreen> screenEnums =
        {
            ProjectManagerScreen::FirstTimeUse,
            ProjectManagerScreen::CreateProject,
            ProjectManagerScreen::ProjectsHome,
            ProjectManagerScreen::UpdateProject,
            ProjectManagerScreen::EngineSettings
        };
        m_screensCtrl->BuildScreens(screenEnums);
        m_screensCtrl->ForceChangeToScreen(ProjectManagerScreen::FirstTimeUse, false);
    }

    ProjectManagerWindow::~ProjectManagerWindow()
    {
        m_pythonBindings.reset();
    }

    void ProjectManagerWindow::HandleProjectsMenu()
    {
        m_screensCtrl->ChangeToScreen(ProjectManagerScreen::ProjectsHome);
    }
    void ProjectManagerWindow::HandleEngineMenu()
    {
        m_screensCtrl->ChangeToScreen(ProjectManagerScreen::EngineSettings);
    }

} // namespace O3DE::ProjectManager
