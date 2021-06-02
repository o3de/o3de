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
#include <ScreenWidget.h>
#include <ProjectInfo.h>
#endif

QT_FORWARD_DECLARE_CLASS(QStackedWidget)
QT_FORWARD_DECLARE_CLASS(QPushButton)
QT_FORWARD_DECLARE_CLASS(QLabel)

namespace O3DE::ProjectManager
{
    QT_FORWARD_DECLARE_CLASS(ScreenHeader)

    class CreateProjectCtrl
        : public ScreenWidget
    {
    public:
        explicit CreateProjectCtrl(QWidget* parent = nullptr);
        ~CreateProjectCtrl() = default;
        ProjectManagerScreen GetScreenEnum() override;
        void NotifyCurrentScreen() override;

    protected slots:
        void HandleBackButton();
        void HandleNextButton();

    private:
        void Update();

        QStackedWidget* m_stack;
        ScreenHeader* m_header;

        QPushButton* m_backButton;
        QPushButton* m_nextButton;

        QString m_projectTemplatePath;
        ProjectInfo m_projectInfo;
    };

} // namespace O3DE::ProjectManager
