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
#include <ScreensCtrl.h>

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

        ScreensCtrl* GetScreensCtrl(QObject* widget)
        {
            if (!widget)
            {
                return nullptr;
            }

            ScreensCtrl* screensCtrl = qobject_cast<ScreensCtrl*> (widget);
            return screensCtrl ? screensCtrl : GetScreensCtrl(widget->parent());
        }

        //! Returns true if this screen is the current screen 
        virtual bool IsCurrentScreen()
        {
            ScreensCtrl* screensCtrl = GetScreensCtrl(this);
            return screensCtrl ? screensCtrl->GetCurrentScreen() == this : false;
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
        void NotifyProjectRemoved(const QString& projectPath);
        void NotifyRemoteContentRefreshed();
    };

} // namespace O3DE::ProjectManager
