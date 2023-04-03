/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#if !defined(Q_MOC_RUN)
#include <ScreenDefs.h>
#include <ProjectInfo.h>

#include <QWidget>
#include <QStyleOption>
#include <QPainter>
#endif

namespace O3DE::ProjectManager
{
    class ScreenWidget
        : public QFrame
    {
        Q_OBJECT

    public:
        explicit ScreenWidget(QWidget* parent = nullptr)
            : QFrame(parent)
        {
        }
        ~ScreenWidget() = default;

        virtual ProjectManagerScreen GetScreenEnum()
        {
            return ProjectManagerScreen::Empty;
        }
        virtual bool IsReadyForNextScreen()
        {
            return true;
        }
        virtual bool IsTab()
        {
            return false;
        }
        virtual QString GetTabText()
        {
            return tr("Missing");
        }

        virtual bool ContainsScreen(ProjectManagerScreen screen)
        {
            return GetScreenEnum() == screen;
        }
        virtual void GoToScreen([[maybe_unused]] ProjectManagerScreen screen)
        {
        }
        virtual void Init()
        {
        }
        //! Notify this screen it is the current screen 
        virtual void NotifyCurrentScreen()
        {
        }

    signals:
        void ChangeScreenRequest(ProjectManagerScreen screen);
        void GoToPreviousScreenRequest();
        void ResetScreenRequest(ProjectManagerScreen screen);
        void NotifyCurrentProject(const QString& projectPath);
        void NotifyBuildProject(const ProjectInfo& projectInfo);

    };

} // namespace O3DE::ProjectManager
