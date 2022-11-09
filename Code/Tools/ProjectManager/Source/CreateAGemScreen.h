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
#include <FormFolderBrowseEditWidget.h>
#include <FormLineEditWidget.h>
#include <FormLineEditTagsWidget.h>
#include <FormComboBoxWidget.h>
#include <GemCatalog/GemInfo.h>
#include <PythonBindings.h>
#include <ScreenHeaderWidget.h>
#endif

QT_FORWARD_DECLARE_CLASS(QButtonGroup)
QT_FORWARD_DECLARE_CLASS(QDialogButtonBox)
QT_FORWARD_DECLARE_CLASS(QRadioButton)
QT_FORWARD_DECLARE_CLASS(QScrollArea)
QT_FORWARD_DECLARE_CLASS(QVBoxLayout)
QT_FORWARD_DECLARE_CLASS(QStackedWidget)
QT_FORWARD_DECLARE_CLASS(QDir)

namespace O3DE::ProjectManager
{
    class CreateGem : public ScreenWidget
    {
        Q_OBJECT
    public:
        explicit CreateGem(QWidget* parent = nullptr);
        ~CreateGem() = default;

        ProjectManagerScreen GetScreenEnum() override
        {
            return ProjectManagerScreen::CreateGem;
        }

        void Init() override;

    signals:
        void GemCreated(const GemInfo& gemInfo);

    protected slots:
        void HandleBackButton();
        void HandleNextButton();
        

    private slots:
        void HandleGemTemplateSelectionTab();
        void HandleGemDetailsTab();
        void HandleGemCreatorDetailsTab();

    protected:
        void ClearFields();

        virtual void GemAction();

        virtual bool ValidateGemLocation(const QDir& chosenGemLocation) const;

        //Gem Setup
        QVector<TemplateInfo> m_gemTemplates;
        QButtonGroup* m_radioButtonGroup = nullptr;
        QRadioButton* m_formFolderRadioButton = nullptr;
        FormFolderBrowseEditWidget* m_gemTemplateLocation = nullptr;

        ScreenHeader* m_header = nullptr;

        //Gem Details
        FormLineEditWidget* m_gemDisplayName = nullptr;
        FormLineEditWidget* m_gemName = nullptr;
        FormLineEditWidget* m_gemSummary = nullptr;
        FormLineEditWidget* m_requirements = nullptr;
        FormLineEditWidget* m_license = nullptr;
        FormLineEditWidget* m_licenseURL = nullptr;
        FormLineEditTagsWidget* m_userDefinedGemTags = nullptr;
        FormFolderBrowseEditWidget* m_gemLocation = nullptr;
        FormLineEditWidget* m_gemIconPath = nullptr;
        FormLineEditWidget* m_documentationURL = nullptr;

        //Gem Creator
        FormLineEditWidget* m_origin = nullptr;
        FormLineEditWidget* m_originURL = nullptr;
        FormLineEditWidget* m_repositoryURL = nullptr;

        QStackedWidget* m_stackWidget = nullptr;

        QDialogButtonBox* m_backNextButtons = nullptr;
        QPushButton* m_backButton = nullptr;
        QPushButton* m_nextButton = nullptr;

        
        QRadioButton* m_gemTemplateSelectionTab = nullptr;
        QRadioButton* m_gemDetailsTab = nullptr;
        QRadioButton* m_gemCreatorDetailsTab = nullptr;

        GemInfo m_gemInfo;

        static constexpr int GemTemplateSelectionScreen = 0;
        static constexpr int GemDetailsScreen = 1;
        static constexpr int GemCreatorDetailsScreen = 2;

        int m_indexBackLimit = 0;

        QString m_gemActionString;

    private:
        void LoadButtonsFromGemTemplatePaths(QVBoxLayout* gemSetupLayout);
        QScrollArea* CreateGemSetupScrollArea();
        QScrollArea* CreateGemDetailsScrollArea();
        QScrollArea* CreateGemCreatorScrollArea();
        QFrame* CreateTabButtonsFrame();
        QFrame* CreateTabPaneFrame();
        void SetupCreateWorkflow();

        void ProceedToGemDetailsPage();
        void ProceedToGemCreatorDetailsPage();
        void ProceedToGemAction();

        bool ValidateGemTemplateLocation();
        bool ValidateGemDisplayName();
        bool ValidateGemName();
        bool ValidateGemPath();
        bool ValidateFormNotEmpty(FormLineEditWidget* form);
        bool ValidateRepositoryURL();
    };
} // namespace O3DE::ProjectManager
