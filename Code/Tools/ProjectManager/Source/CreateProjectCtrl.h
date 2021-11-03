/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#if !defined(Q_MOC_RUN)
#include <ScreenWidget.h>
#include <ProjectInfo.h>
#endif

#define TEMPLATE_GEM_CONFIGURATION_ENABLED

QT_FORWARD_DECLARE_CLASS(QStackedWidget)
QT_FORWARD_DECLARE_CLASS(QPushButton)
QT_FORWARD_DECLARE_CLASS(QLabel)

namespace O3DE::ProjectManager
{
    QT_FORWARD_DECLARE_CLASS(ScreenHeader)
    QT_FORWARD_DECLARE_CLASS(NewProjectSettingsScreen)
    QT_FORWARD_DECLARE_CLASS(GemCatalogScreen)

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
        void HandlePrimaryButton();

#ifdef TEMPLATE_GEM_CONFIGURATION_ENABLED
        void OnChangeScreenRequest(ProjectManagerScreen screen);
        void HandleSecondaryButton();
        void ReinitGemCatalogForSelectedTemplate();
#endif // TEMPLATE_GEM_CONFIGURATION_ENABLED

    private:
#ifdef TEMPLATE_GEM_CONFIGURATION_ENABLED
        void Update();
        void NextScreen();
        void PreviousScreen();
#endif // TEMPLATE_GEM_CONFIGURATION_ENABLED

        bool CurrentScreenIsValid();
        void CreateProject();

        QStackedWidget* m_stack = nullptr;
        ScreenHeader* m_header = nullptr;

        QPushButton* m_primaryButton = nullptr;

#ifdef TEMPLATE_GEM_CONFIGURATION_ENABLED
        QPushButton* m_secondaryButton = nullptr;
#endif // TEMPLATE_GEM_CONFIGURATION_ENABLED

        NewProjectSettingsScreen* m_newProjectSettingsScreen = nullptr;
        GemCatalogScreen* m_gemCatalogScreen = nullptr;
    };

} // namespace O3DE::ProjectManager
