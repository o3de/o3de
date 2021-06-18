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

// due to current limitations, customizing template Gems is disabled 
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

        QString m_projectTemplatePath;
        ProjectInfo m_projectInfo;
        
        NewProjectSettingsScreen* m_newProjectSettingsScreen = nullptr;
        GemCatalogScreen* m_gemCatalogScreen = nullptr;
    };

} // namespace O3DE::ProjectManager
