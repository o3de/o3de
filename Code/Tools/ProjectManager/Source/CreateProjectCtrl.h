/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Outcome/Outcome.h>

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
    QT_FORWARD_DECLARE_CLASS(NewProjectSettingsScreen)
    QT_FORWARD_DECLARE_CLASS(ProjectGemCatalogScreen)
    QT_FORWARD_DECLARE_CLASS(GemRepoScreen)
    QT_FORWARD_DECLARE_CLASS(DownloadController);

    class CreateProjectCtrl
        : public ScreenWidget
    {
    public:
        explicit CreateProjectCtrl(DownloadController* downloadController, QWidget* parent = nullptr);
        ~CreateProjectCtrl() = default;
        ProjectManagerScreen GetScreenEnum() override;
        void NotifyCurrentScreen() override;

    protected slots:
        void HandleBackButton();
        void HandlePrimaryButton();

        void OnChangeScreenRequest(ProjectManagerScreen screen);
        void HandleSecondaryButton();
        void ReinitGemCatalogForSelectedTemplate();

    private:
        void Update();
        void NextScreen();
        void PreviousScreen();

        
        AZ::Outcome<void, QString> CurrentScreenIsValid();
        void CreateProject();

        QStackedWidget* m_stack = nullptr;
        ScreenHeader* m_header = nullptr;

        QPushButton* m_primaryButton = nullptr;
        QPushButton* m_secondaryButton = nullptr;

        NewProjectSettingsScreen* m_newProjectSettingsScreen = nullptr;
        ProjectGemCatalogScreen* m_projectGemCatalogScreen = nullptr;
        GemRepoScreen* m_gemRepoScreen = nullptr;
    };

} // namespace O3DE::ProjectManager
