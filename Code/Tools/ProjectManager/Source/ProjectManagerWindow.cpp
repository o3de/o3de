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

        ConnectSlotsAndSignals();

        QDir rootDir = QString::fromUtf8(engineRootPath.Native().data(), aznumeric_cast<int>(engineRootPath.Native().size()));
        const auto pathOnDisk = rootDir.absoluteFilePath("Code/Tools/ProjectManager/Resources");
        const auto qrcPath = QStringLiteral(":/ProjectManagerWindow");
        AzQtComponents::StyleManager::addSearchPaths("projectmanagerwindow", pathOnDisk, qrcPath, engineRootPath);

        AzQtComponents::StyleManager::setStyleSheet(this, QStringLiteral("projectlauncherwindow:ProjectManagerWindow.qss"));

        BuildScreens();

        ChangeToScreen(ProjectManagerScreen::FirstTimeUse);
    }

    ProjectManagerWindow::~ProjectManagerWindow()
    {
    }

    void ProjectManagerWindow::BuildScreens()
    {
        // Basically just iterate over the ProjectManagerScreen enum creating each screen
        // Could add some fancy to do this but there are few screens right now

        ResetScreen(ProjectManagerScreen::FirstTimeUse);
        ResetScreen(ProjectManagerScreen::NewProjectSettings);
        ResetScreen(ProjectManagerScreen::GemCatalog);
        ResetScreen(ProjectManagerScreen::ProjectsHome);
        ResetScreen(ProjectManagerScreen::ProjectSettings);
        ResetScreen(ProjectManagerScreen::EngineSettings);
    }

    QStackedWidget* ProjectManagerWindow::GetScreenStack()
    {
        return m_ui->stackedScreens;
    }

    void ProjectManagerWindow::ChangeToScreen(ProjectManagerScreen screen)
    {
        int index = aznumeric_cast<int, ProjectManagerScreen>(screen);
        m_ui->stackedScreens->setCurrentIndex(index);
    }

    void ProjectManagerWindow::ResetScreen(ProjectManagerScreen screen)
    {
        int index = aznumeric_cast<int, ProjectManagerScreen>(screen);

        // Fine the old screen if it exists and get rid of it so we can start fresh
        QWidget* oldScreen = m_ui->stackedScreens->widget(index);

        if (oldScreen)
        {
            m_ui->stackedScreens->removeWidget(oldScreen);
            oldScreen->deleteLater();
        }

        // Add new screen
        QWidget* newScreen = BuildScreen(this, screen);
        m_ui->stackedScreens->insertWidget(index, newScreen);
    }

    void ProjectManagerWindow::ConnectSlotsAndSignals()
    {
        QObject::connect(m_ui->projectsMenu, &QMenu::aboutToShow, this, &ProjectManagerWindow::HandleProjectsMenu);
        QObject::connect(m_ui->engineMenu, &QMenu::aboutToShow, this, &ProjectManagerWindow::HandleEngineMenu);
    }

    void ProjectManagerWindow::HandleProjectsMenu()
    {
        ChangeToScreen(ProjectManagerScreen::ProjectsHome);
    }
    void ProjectManagerWindow::HandleEngineMenu()
    {
        ChangeToScreen(ProjectManagerScreen::EngineSettings);
    }

} // namespace O3DE::ProjectManager
