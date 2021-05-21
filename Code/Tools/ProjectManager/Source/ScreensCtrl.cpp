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

#include <QVBoxLayout>

namespace O3DE::ProjectManager
{
    ScreensCtrl::ScreensCtrl(QWidget* parent)
        : QWidget(parent)
        , m_currentProject("")
    {
        QVBoxLayout* vLayout = new QVBoxLayout();
        vLayout->setMargin(0);
        vLayout->setSpacing(0);
        vLayout->setContentsMargins(0, 0, 0, 0);
        setLayout(vLayout);

        m_screenStack = new QStackedWidget();
        vLayout->addWidget(m_screenStack);

        //Track the bottom of the stack
        m_screenVisitOrder.push(ProjectManagerScreen::Invalid);
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
        return reinterpret_cast<ScreenWidget*>(m_screenStack->currentWidget());
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
            if (currentScreen != iterator.value())
            {
                if (addVisit)
                {
                    m_screenVisitOrder.push(currentScreen->GetScreenEnum());
                }
                m_screenStack->setCurrentWidget(iterator.value());
                return true;
            }
        }

        return false;
    }

    bool ScreensCtrl::GotoPreviousScreen()
    {
        // Don't go back if we are on the first set screen
        if (m_screenVisitOrder.top() != ProjectManagerScreen::Invalid)
        {
            // We do not check with screen if we can go back, we should always be able to go back
            return ForceChangeToScreen(m_screenVisitOrder.pop(), false);
        }
        return false;
    }

    void ScreensCtrl::ResetScreen(ProjectManagerScreen screen)
    {
        // Delete old screen if it exists to start fresh
        DeleteScreen(screen);

        // Add new screen
        ScreenWidget* newScreen = BuildScreen(this, screen, m_currentProject);
        m_screenStack->addWidget(newScreen);
        m_screenMap.insert(screen, newScreen);

        connect(newScreen, &ScreenWidget::ChangeScreenRequest, this, &ScreensCtrl::ChangeToScreen);
        connect(newScreen, &ScreenWidget::GotoPreviousScreenRequest, this, &ScreensCtrl::GotoPreviousScreen);
        connect(newScreen, &ScreenWidget::ResetScreenRequest, this, &ScreensCtrl::ResetScreen);
        connect(newScreen, &ScreenWidget::NotifyCurrentProject, this, &ScreensCtrl::SetCurrentProject);
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
            m_screenStack->removeWidget(iter.value());
            iter.value()->deleteLater();

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

    void ScreensCtrl::SetCurrentProject(const QString& projectName)
    {
        m_currentProject = projectName;
    }

} // namespace O3DE::ProjectManager
