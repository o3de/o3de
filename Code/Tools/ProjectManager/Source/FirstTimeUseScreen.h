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
#include <QPixmap>
#endif

QT_FORWARD_DECLARE_CLASS(QIcon)
QT_FORWARD_DECLARE_CLASS(QPushButton)
QT_FORWARD_DECLARE_CLASS(QPaintEvent)

namespace O3DE::ProjectManager
{
    class FirstTimeUseScreen
        : public ScreenWidget
    {
    public:
        explicit FirstTimeUseScreen(QWidget* parent = nullptr);
        ~FirstTimeUseScreen() = default;
        ProjectManagerScreen GetScreenEnum() override;
        QString GetTabText() override;
        bool IsTab() override;

    protected slots:
        void HandleNewProjectButton();
        void HandleAddProjectButton();

        void paintEvent(QPaintEvent* event) override;

    private:
        QPushButton* m_createProjectButton;
        QPushButton* m_addProjectButton;
        QPixmap m_background;

        inline constexpr static int s_contentMargins = 80;
        inline constexpr static int s_buttonSpacing = 30;
        inline constexpr static int s_iconSize = 24;
        inline constexpr static int s_spacerSize = 20;
        inline constexpr static int s_boxButtonWidth = 210;
        inline constexpr static int s_boxButtonHeight = 280;
    };

} // namespace O3DE::ProjectManager
