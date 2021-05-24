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

#include <ScreensCtrl.h>
#include <ScreenFactory.h>
#include <ScreenWidget.h>

#include <QTabWidget>
#include <QVBoxLayout>

namespace O3DE::ProjectManager
{
    ScreensCtrl::ScreensCtrl(QWidget* parent)
        : QWidget(parent)
    {
        setObjectName("ScreensCtrl");

        QVBoxLayout* vLayout = new QVBoxLayout();
        vLayout->setContentsMargins(0, 0, 0, 0);
        setLayout(vLayout);

        m_screenStack = new QStackedWidget();
        vLayout->addWidget(m_screenStack);

        // add a tab widget at the bottom of the stack
        m_tabWidget = new QTabWidget();
        m_screenStack->addWidget(m_tabWidget);
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
            return reinterpret_cast<ScreenWidget*>(m_tabWidget->currentWidget());
        }
        else
        {
            return reinterpret_cast<ScreenWidget*>(m_screenStack->currentWidget());
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
        const auto iterator = m_screenMap.find(screen);
        if (iterator != m_screenMap.end())
        {
            ScreenWidget* currentScreen = GetCurrentScreen();
            ScreenWidget* newScreen = iterator.value();

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
                return true;
            }
        }

        return false;
    }

    bool ScreensCtrl::GotoPreviousScreen()
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

        // Delete old screen if it exists to start fresh
        DeleteScreen(screen);

        // Add new screen
        ScreenWidget* newScreen = BuildScreen(this, screen);
        if (newScreen->IsTab())
        {
            m_tabWidget->addTab(newScreen, newScreen->GetTabText());
            if (shouldRestoreCurrentScreen)
            {
                m_tabWidget->setCurrentWidget(newScreen);
                m_screenStack->setCurrentWidget(m_tabWidget);
            }
        }
        else
        {
            m_screenStack->addWidget(newScreen);
            if (shouldRestoreCurrentScreen)
            {
                m_screenStack->setCurrentWidget(newScreen);
            }
        }

        m_screenMap.insert(screen, newScreen);

        connect(newScreen, &ScreenWidget::ChangeScreenRequest, this, &ScreensCtrl::ChangeToScreen);
        connect(newScreen, &ScreenWidget::GotoPreviousScreenRequest, this, &ScreensCtrl::GotoPreviousScreen);
        connect(newScreen, &ScreenWidget::ResetScreenRequest, this, &ScreensCtrl::ResetScreen);

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

} // namespace O3DE::ProjectManager
