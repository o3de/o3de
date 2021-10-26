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

        m_tabWidget = new QTabWidget();
        m_tabWidget->setObjectName("engineTab");
        m_tabWidget->tabBar()->setObjectName("engineTabBar");
        m_tabWidget->tabBar()->setFocusPolicy(Qt::TabFocus);

        m_engineSettingsScreen = new EngineSettingsScreen();
        m_gemRepoScreen = new GemRepoScreen();

        m_tabWidget->addTab(m_engineSettingsScreen, tr("General"));
        m_tabWidget->addTab(m_gemRepoScreen, tr("Gem Repositories"));
        topBarHLayout->addWidget(m_tabWidget);

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

    bool EngineScreenCtrl::ContainsScreen(ProjectManagerScreen screen)
    {
        if (screen == m_engineSettingsScreen->GetScreenEnum() || screen == m_gemRepoScreen->GetScreenEnum())
        {
            return true;
        }

        return false;
    }

    void EngineScreenCtrl::GoToScreen(ProjectManagerScreen screen)
    {
        if (screen == m_engineSettingsScreen->GetScreenEnum())
        {
            m_tabWidget->setCurrentWidget(m_engineSettingsScreen);
            m_engineSettingsScreen->NotifyCurrentScreen();
        }
        else if (screen == m_gemRepoScreen->GetScreenEnum())
        {
            m_tabWidget->setCurrentWidget(m_gemRepoScreen);
            m_gemRepoScreen->NotifyCurrentScreen();
        }
    }

} // namespace O3DE::ProjectManager
