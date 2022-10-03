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
#include <FormComboBoxWidget.h>
#include <GemCatalog/GemInfo.h>
#include <PythonBindings.h>
#endif

class QButtonGroup;
class QDialogButtonBox;
class QRadioButton;
class QScrollArea;
class QVBoxLayout;
class QStackedWidget;

namespace O3DE::ProjectManager
{
    class CreateGem : public ScreenWidget
    {
        Q_OBJECT
    public:
        explicit CreateGem(QWidget* parent = nullptr);
        ~CreateGem() = default;

    signals:
        void GemCreated(const GemInfo& gemInfo);

    private slots:
        void HandleBackButton();
        void HandleNextButton();
        void HandleGemTemplateSelectionTab();
        void HandleGemDetailsTab();
        void HandleGemCreatorDetailsTab();

    private:
        void LoadButtonsFromGemTemplatePaths(QVBoxLayout* gemSetupLayout);
        QScrollArea* CreateGemSetupScrollArea();
        QScrollArea* CreateGemDetailsScrollArea();
        QScrollArea* CreateGemCreatorScrollArea();
        QFrame* CreateLHSTabs();
        QFrame* CreateRHSPane();
        bool ValidateGemTemplateLocation();
        bool ValidateGemDisplayName();
        bool ValidateGemName();
        bool ValidateGemPath();
        bool ValidateFormNotEmpty(FormLineEditWidget* form);
        bool ValidateRepositoryURL();

        //Gem Setup
        QVector<TemplateInfo> m_gemTemplates;
        QButtonGroup* m_radioButtonGroup = nullptr;
        QRadioButton* m_formFolderRadioButton = nullptr;
        FormFolderBrowseEditWidget* m_gemTemplateLocation = nullptr;

        //Gem Details
        FormLineEditWidget* m_gemDisplayName = nullptr;
        FormLineEditWidget* m_gemName = nullptr;
        FormLineEditWidget* m_gemSummary = nullptr;
        FormLineEditWidget* m_requirements = nullptr;
        FormLineEditWidget* m_license = nullptr;
        FormLineEditWidget* m_licenseURL = nullptr;
        FormLineEditWidget* m_userDefinedGemTags = nullptr;
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

        GemInfo m_createGemInfo;

        const int gemTemplateSelectionScreen = 0;
        const int gemDetailsScreen = 1;
        const int gemCreatorDetailsScreen = 2;
    };


} // namespace O3DE::ProjectManager
