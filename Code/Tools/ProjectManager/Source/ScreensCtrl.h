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
#pragma once

#if !defined(Q_MOC_RUN)
#include <ScreenDefs.h>

#include <QStackedWidget>
#include <QStack>
#endif

namespace O3DE::ProjectManager
{
    class ScreenWidget;

    class ScreensCtrl
        : public QWidget
    {
        Q_OBJECT

    public:
        explicit ScreensCtrl(QWidget* parent = nullptr);
        ~ScreensCtrl() = default;

        void BuildScreens(QVector<ProjectManagerScreen> screens);
        ScreenWidget* GetScreen(ProjectManagerScreen screen);
        ScreenWidget* GetCurrentScreen();

    public slots:
        bool ChangeToScreen(ProjectManagerScreen screen);
        bool ForceChangeToScreen(ProjectManagerScreen screen, bool addVisit = true);
        bool GotoPreviousScreen();
        void ResetScreen(ProjectManagerScreen screen);
        void ResetAllScreens();
        void DeleteScreen(ProjectManagerScreen screen);
        void DeleteAllScreens();

    private:
        QStackedWidget* m_screenStack;
        QHash<ProjectManagerScreen, ScreenWidget*> m_screenMap;
        QStack<ProjectManagerScreen> m_screenVisitOrder;
    };

} // namespace O3DE::ProjectManager
