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

    signals:
        void ChangeScreenRequest(ProjectManagerScreen screen);
        void GotoPreviousScreenRequest();
        void ResetScreenRequest(ProjectManagerScreen screen);
        void NotifyCurrentProject(const QString& projectPath);
    };

} // namespace O3DE::ProjectManager
