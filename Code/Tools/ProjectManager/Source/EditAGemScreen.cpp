/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <EditAGemScreen.h>

#include <QDir>
#include <QLineEdit>
#include <QPushButton>
#include <QRadioButton>
#include <QStackedWidget>
#include <QMessageBox>

namespace O3DE::ProjectManager
{

    EditGem::EditGem(QWidget* parent)
        : CreateGem(parent)
    {
        m_header->setSubTitle(tr("Edit gem"));
           
        //we will only have two pages: details page and creator details page
        m_gemTemplateSelectionTab->setChecked(false);
        m_gemTemplateSelectionTab->setVisible(false);

        m_gemDetailsTab->setEnabled(true);
        m_gemDetailsTab->setChecked(true);

        m_gemCreatorDetailsTab->setEnabled(true);

        m_gemDetailsTab->setText(tr("1.  Gem Details"));
        m_gemCreatorDetailsTab->setText(tr("2.  Creator Details"));

        m_stackWidget->setCurrentIndex(GemDetailsScreen);

        m_gemLocation->setEnabled(false);

        m_nextButton->setText(tr("Next"));

        m_gemActionString = tr("Edit");
        m_indexBackLimit = GemDetailsScreen;
    }

    void EditGem::Init()
    {
        //hookup connections
        connect(
            m_header->backButton(),
            &QPushButton::clicked,
            this,
            [&]()
            {
                // if previously editing a gem, cancel the operation and revert to old creation workflow
                // this way we prevent accidental stale data for an already existing gem.
                ClearFields();
                emit GoToPreviousScreenRequest();
            });

        connect(m_backButton, &QPushButton::clicked, this, &EditGem::HandleBackButton);
        connect(m_nextButton, &QPushButton::clicked, this, &EditGem::HandleNextButton);

     
    }

    bool EditGem::ValidateGemLocation(const QDir& chosenGemLocation) const
    {
        return chosenGemLocation.exists();
    }

    void EditGem::ResetWorkflow(const GemInfo& oldGemInfo)
    {
        //details page
        m_gemDisplayName->lineEdit()->setText(oldGemInfo.m_displayName);
        m_gemDisplayName->setErrorLabelVisible(false);

        m_gemName->lineEdit()->setText(oldGemInfo.m_name);
        m_gemName->setErrorLabelVisible(false);

        m_gemSummary->lineEdit()->setText(oldGemInfo.m_summary);
        m_requirements->lineEdit()->setText(oldGemInfo.m_requirement);
        
        m_license->lineEdit()->setText(oldGemInfo.m_licenseText);
        m_license->setErrorLabelVisible(false);

        m_licenseURL->lineEdit()->setText(oldGemInfo.m_licenseLink);
        m_documentationURL->lineEdit()->setText(oldGemInfo.m_documentationLink);
        
        m_gemLocation->lineEdit()->setText(oldGemInfo.m_path);
        m_gemLocation->setErrorLabelVisible(false);

        m_gemIconPath->lineEdit()->setText(oldGemInfo.m_iconPath);

        //Gem name is included in tags. Since the user can override this by setting the name field
        //we should remove it from the tag list to prevent unintended consequences
        QStringList tagsToEdit = oldGemInfo.m_features;
        tagsToEdit.removeAll(oldGemInfo.m_name);
        m_userDefinedGemTags->setTags(tagsToEdit);

        //creator details page
        m_origin->lineEdit()->setText(oldGemInfo.m_origin);
        m_origin->setErrorLabelVisible(false);

        m_originURL->lineEdit()->setText(oldGemInfo.m_originURL);
        m_repositoryURL->lineEdit()->setText(oldGemInfo.m_repoUri);

        m_oldGemName = oldGemInfo.m_name;
    }


    //here we handle the actual edit gem logic
    void EditGem::GemAction()
    {
        //during editing, we remove the gem name tag to prevent the user accidentally altering it
        //so add it back here before submission
        if(!m_gemInfo.m_features.contains(m_gemInfo.m_name))
        {
            m_gemInfo.m_features << m_gemInfo.m_name;
        }

        auto result = PythonBindingsInterface::Get()->EditGem(m_oldGemName, m_gemInfo);
        if(result.IsSuccess())
        {
            ClearFields();
            emit GemEdited(result.GetValue<GemInfo>());
            emit GoToPreviousScreenRequest();
        }
        else
        {
            QMessageBox::critical(
                this,
                tr("Failed to edit gem"),
                tr("The gem failed to be edited"));
        }
    }

} // namespace O3DE::ProjectManager
