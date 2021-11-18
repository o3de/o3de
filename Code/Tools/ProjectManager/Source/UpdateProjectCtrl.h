/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#if !defined(Q_MOC_RUN)
#include <ProjectInfo.h>
#include <ScreenWidget.h>
#endif

QT_FORWARD_DECLARE_CLASS(QStackedWidget)
QT_FORWARD_DECLARE_CLASS(QTabWidget)
QT_FORWARD_DECLARE_CLASS(QPushButton)
QT_FORWARD_DECLARE_CLASS(QFrame)

namespace O3DE::ProjectManager
{
    QT_FORWARD_DECLARE_CLASS(ScreenHeader)
    QT_FORWARD_DECLARE_CLASS(UpdateProjectSettingsScreen)
    QT_FORWARD_DECLARE_CLASS(GemCatalogScreen)
    QT_FORWARD_DECLARE_CLASS(GemRepoScreen)

    class UpdateProjectCtrl
        : public ScreenWidget
    {
        Q_OBJECT
    public:
        explicit UpdateProjectCtrl(QWidget* parent = nullptr);
        ~UpdateProjectCtrl() = default;
        ProjectManagerScreen GetScreenEnum() override;

        bool ContainsScreen(ProjectManagerScreen screen) override;
        void GoToScreen(ProjectManagerScreen screen) override;
        void NotifyCurrentScreen() override;

    protected slots:
        void HandleBackButton();
        void HandleNextButton();
        void HandleGemsButton();
        void OnChangeScreenRequest(ProjectManagerScreen screen);
        void UpdateCurrentProject(const QString& projectPath);

    private:
        void Update();
        void UpdateSettingsScreen();
        bool UpdateProjectSettings(bool shouldConfirm = false);

        enum ScreenOrder
        {
            Settings,
            Gems,
            GemRepos
        };

        ScreenHeader* m_header = nullptr;
        QStackedWidget* m_stack = nullptr;
        UpdateProjectSettingsScreen* m_updateSettingsScreen = nullptr;
        GemCatalogScreen* m_gemCatalogScreen = nullptr;
        GemRepoScreen* m_gemRepoScreen = nullptr;

        QPushButton* m_backButton = nullptr;
        QPushButton* m_nextButton = nullptr;
        QVector<ProjectManagerScreen> m_screensOrder;

        ProjectInfo m_projectInfo;
    };

} // namespace O3DE::ProjectManager
