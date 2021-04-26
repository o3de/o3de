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

#include <Qt/LauncherWindow.h>
#include <Qt/FirstTimeUse.h>

#include <AzQtComponents/Components/StyleManager.h>
#include <AzCore/IO/Path/Path.h>

#include <QDir>

#include <Qt/ui_LauncherWindow.h>

namespace ProjectLauncher
{
    LauncherWindow::LauncherWindow(QWidget* parent, const AZ::IO::PathView& engineRootPath)
        : QMainWindow(parent)
        , m_ui(new Ui::LauncherWindowClass())
    {
        m_ui->setupUi(this);

        QDir rootDir = QString::fromUtf8(engineRootPath.Native().data(), aznumeric_cast<int>(engineRootPath.Native().size()));
        const auto pathOnDisk = rootDir.absoluteFilePath("Code/Tools/ProjectLauncher/Resources");
        const auto qrcPath = QStringLiteral(":/LauncherWindow");
        AzQtComponents::StyleManager::addSearchPaths("launcherwindow", pathOnDisk, qrcPath, engineRootPath);

        AzQtComponents::StyleManager::setStyleSheet(this, QStringLiteral("launcherwindow:LauncherWindow.qss"));


        FirstTimeUse* firstTimeUse = new FirstTimeUse(m_ui->centralwidget);

        firstTimeUse->show();
    }

    LauncherWindow::~LauncherWindow()
    {
    }

    //#include <Qt/moc_LauncherWindow.cpp>
} // namespace ProjectLauncher
