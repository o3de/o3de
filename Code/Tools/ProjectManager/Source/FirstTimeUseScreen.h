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
#endif

QT_FORWARD_DECLARE_CLASS(QIcon)
QT_FORWARD_DECLARE_CLASS(QPushButton)

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

    private:
        QPushButton* CreateLargeBoxButton(const QIcon& icon, const QString& text, QWidget* parent = nullptr);

        QPushButton* m_createProjectButton;
        QPushButton* m_addProjectButton;
    };

} // namespace O3DE::ProjectManager
