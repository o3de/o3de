/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <ScreensCtrl.h>
#include <ScreenFactory.h>
#include <ScreenWidget.h>
#include <UpdateProjectCtrl.h>

#include <QTabWidget>
#include <QVBoxLayout>

namespace O3DE::ProjectManager
{
    ScreensCtrl::ScreensCtrl(QWidget* parent, DownloadController* downloadController)
        : QWidget(parent)
        , m_downloadController(downloadController)
    {
        setObjectName("ScreensCtrl");

        QVBoxLayout* vLayout = new QVBoxLayout();
        vLayout->setContentsMargins(0, 0, 0, 0);
        setLayout(vLayout);

        m_screenStack = new QStackedWidget();
        vLayout->addWidget(m_screenStack);

        // add a tab widget at the bottom of the stack
        m_tabWidget = new QTabWidget();
        m_tabWidget->tabBar()->setFocusPolicy(Qt::TabFocus);
        m_screenStack->addWidget(m_tabWidget);
        connect(m_tabWidget, &QTabWidget::currentChanged, this, &ScreensCtrl::TabChanged);
    }

    void ScreensCtrl::BuildScreens(QVector<ProjectManagerScreen> screens)
    {
        for (ProjectManagerScreen screen : screens)
        {
            ResetScreen(screen);
        }
    }

    ScreenWidget* ScreensCtrl::FindScreen(ProjectManagerScreen screen)
    {
        const auto iterator = m_screenMap.find(screen);
        if (iterator != m_screenMap.end())
        {
            return iterator.value();
        }
        else
        {
            return nullptr;
        }
    }

    ScreenWidget* ScreensCtrl::GetCurrentScreen()
    {
        if (m_screenStack->currentWidget() == m_tabWidget)
        {
            return static_cast<ScreenWidget*>(m_tabWidget->currentWidget());
        }
        else
        {
            return static_cast<ScreenWidget*>(m_screenStack->currentWidget());
        }
    }

    bool ScreensCtrl::ChangeToScreen(ProjectManagerScreen screen)
    {
        if (m_screenStack->currentWidget())
        {
            ScreenWidget* currentScreenWidget = GetCurrentScreen();
            if (currentScreenWidget->IsReadyForNextScreen())
            {
                return ForceChangeToScreen(screen);
            }
        }
        return false;
    }

    bool ScreensCtrl::ForceChangeToScreen(ProjectManagerScreen screen, bool addVisit)
    {
        ScreenWidget* newScreen = nullptr;

        const auto iterator = m_screenMap.find(screen);
        if (iterator != m_screenMap.end())
        {
            newScreen = iterator.value();
        }
        else
        {
            // Check if screen is contained by another screen
            for (ScreenWidget* checkingScreen : m_screenMap)
            {
                if (checkingScreen->ContainsScreen(screen))
                {
                    newScreen = checkingScreen;
                    break;
                }
            }
        }
        if (newScreen)
        {
            ScreenWidget* currentScreen = GetCurrentScreen();

            if (currentScreen != newScreen)
            {
                if (addVisit)
                {
                    ProjectManagerScreen oldScreen = currentScreen->GetScreenEnum();
                    m_screenVisitOrder.push(oldScreen);
                }

                if (newScreen->IsTab())
                {
                    m_tabWidget->setCurrentWidget(newScreen);
                    m_screenStack->setCurrentWidget(m_tabWidget);
                }
                else
                {
                    m_screenStack->setCurrentWidget(newScreen);
                }

                newScreen->NotifyCurrentScreen();

                if (iterator == m_screenMap.end())
                {
                    newScreen->GoToScreen(screen);
                }

                return true;
            }
            else
            {
                // If we are already on this screen still notify we are on this screen to refresh it
                newScreen->NotifyCurrentScreen();
            }
        }

        return false;
    }

    bool ScreensCtrl::GoToPreviousScreen()
    {
        if (!m_screenVisitOrder.isEmpty())
        {
            // We do not check with screen if we can go back, we should always be able to go back
            ProjectManagerScreen previousScreen = m_screenVisitOrder.pop();
            return ForceChangeToScreen(previousScreen, false);
        }
        return false;
    }

    void ScreensCtrl::ResetScreen(ProjectManagerScreen screen)
    {
        bool shouldRestoreCurrentScreen = false;
        if (GetCurrentScreen() && GetCurrentScreen()->GetScreenEnum() == screen)
        {
            shouldRestoreCurrentScreen = true;
        }
        int tabIndex = GetScreenTabIndex(screen);

        // Delete old screen if it exists to start fresh
        DeleteScreen(screen);

        // Add new screen
        ScreenWidget* newScreen = BuildScreen(this, screen, m_downloadController);
        if (newScreen->IsTab())
        {
            if (tabIndex > -1)
            {
                m_tabWidget->insertTab(tabIndex, newScreen, newScreen->GetTabText());
            }
            else
            {
                m_tabWidget->addTab(newScreen, newScreen->GetTabText());
            }
            if (shouldRestoreCurrentScreen)
            {
                m_tabWidget->setCurrentWidget(newScreen);
                m_screenStack->setCurrentWidget(m_tabWidget);
                newScreen->NotifyCurrentScreen();
            }
        }
        else
        {
            m_screenStack->addWidget(newScreen);
            if (shouldRestoreCurrentScreen)
            {
                m_screenStack->setCurrentWidget(newScreen);
                newScreen->NotifyCurrentScreen();
            }
        }

        m_screenMap.insert(screen, newScreen);

        connect(newScreen, &ScreenWidget::ChangeScreenRequest, this, &ScreensCtrl::ChangeToScreen);
        connect(newScreen, &ScreenWidget::GoToPreviousScreenRequest, this, &ScreensCtrl::GoToPreviousScreen);
        connect(newScreen, &ScreenWidget::ResetScreenRequest, this, &ScreensCtrl::ResetScreen);
        connect(newScreen, &ScreenWidget::NotifyCurrentProject, this, &ScreensCtrl::NotifyCurrentProject);
        connect(newScreen, &ScreenWidget::NotifyBuildProject, this, &ScreensCtrl::NotifyBuildProject);
        connect(newScreen, &ScreenWidget::NotifyProjectRemoved, this, &ScreensCtrl::NotifyProjectRemoved);
    }

    void ScreensCtrl::ResetAllScreens()
    {
        for (auto iter = m_screenMap.begin(); iter != m_screenMap.end(); ++iter)
        {
            ResetScreen(iter.key());
        }
    }

    void ScreensCtrl::DeleteScreen(ProjectManagerScreen screen)
    {
        // Find the old screen if it exists and get rid of it
        const auto iter = m_screenMap.find(screen);
        if (iter != m_screenMap.end())
        {
            ScreenWidget* screenToDelete = iter.value();
            if (screenToDelete->IsTab())
            {
                int tabIndex = m_tabWidget->indexOf(screenToDelete);
                if (tabIndex > -1)
                {
                    m_tabWidget->removeTab(tabIndex);
                }
            }
            else
            {
                // if the screen we delete is the current widget, a new one will
                // be selected automatically (randomly?)
                m_screenStack->removeWidget(screenToDelete);
            }

            // Erase does not cause a rehash so interators remain valid
            m_screenMap.erase(iter);
        }
    }

    void ScreensCtrl::DeleteAllScreens()
    {
        for (auto iter = m_screenMap.begin(); iter != m_screenMap.end(); ++iter)
        {
            DeleteScreen(iter.key());
        }
    }

    void ScreensCtrl::TabChanged([[maybe_unused]] int index)
    {
        ScreenWidget* screen = static_cast<ScreenWidget*>(m_tabWidget->currentWidget());
        if (screen)
        {
            screen->NotifyCurrentScreen();
        }
    }

    int ScreensCtrl::GetScreenTabIndex(ProjectManagerScreen screen)
    {
        const auto iter = m_screenMap.find(screen);
        if (iter != m_screenMap.end())
        {
            ScreenWidget* screenWidget = iter.value();
            if (screenWidget->IsTab())
            {
                return m_tabWidget->indexOf(screenWidget);
            }
        }
        
        return -1;
    }
} // namespace O3DE::ProjectManager
