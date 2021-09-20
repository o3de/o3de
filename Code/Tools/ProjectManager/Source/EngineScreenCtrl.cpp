/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <EngineScreenCtrl.h>
#include <GemRepo/GemRepoScreen.h>
#include <EngineSettingsScreen.h>

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTabWidget>

namespace O3DE::ProjectManager
{
    EngineScreenCtrl::EngineScreenCtrl(QWidget* parent)
        : ScreenWidget(parent)
    {
        QVBoxLayout* vLayout = new QVBoxLayout();
        vLayout->setContentsMargins(0, 0, 0, 0);

        QFrame* topBarFrameWidget = new QFrame(this);
        topBarFrameWidget->setObjectName("engineTopFrame");
        QHBoxLayout* topBarHLayout = new QHBoxLayout();
        topBarHLayout->setContentsMargins(0, 0, 0, 0);
        
        topBarFrameWidget->setLayout(topBarHLayout);

        QTabWidget* tabWidget = new QTabWidget();
        tabWidget->setObjectName("engineTab");
        tabWidget->tabBar()->setObjectName("engineTabBar");
        tabWidget->tabBar()->setFocusPolicy(Qt::TabFocus);

        m_engineSettingsScreen = new EngineSettingsScreen();
        m_gemRepoScreen = new GemRepoScreen();

        tabWidget->addTab(m_engineSettingsScreen, tr("General"));
        tabWidget->addTab(m_gemRepoScreen, tr("Gem Repositories"));
        topBarHLayout->addWidget(tabWidget);

        vLayout->addWidget(topBarFrameWidget);

        setLayout(vLayout);
    }

    ProjectManagerScreen EngineScreenCtrl::GetScreenEnum()
    {
        return ProjectManagerScreen::UpdateProject;
    }

    QString EngineScreenCtrl::GetTabText()
    {
        return tr("Engine");
    }

    bool EngineScreenCtrl::IsTab()
    {
        return true;
    }

} // namespace O3DE::ProjectManager
