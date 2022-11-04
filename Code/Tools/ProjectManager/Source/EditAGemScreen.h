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
#include <CreateAGemScreen.h>
#include <PythonBindings.h>
#include <ScreenHeaderWidget.h>
#include <ScreenDefs.h>

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
    class EditGem : public CreateGem
    {
        Q_OBJECT
    public:
        explicit EditGem(QWidget* parent = nullptr);
        ~EditGem() = default;

        void ResetWorkflow(const GemInfo& oldGemInfo);

        ProjectManagerScreen GetScreenEnum() override
        {
            return ProjectManagerScreen::EditGem;
        }

    signals:
        void GemEdited(const GemInfo& newGemInfo);
        
    private: 
        void GemAction() override;

        bool ValidateGemLocation(QDir chosenGemLocation) override
        {
            return chosenGemLocation.exists();
        }

        //Edit Gem workflow
        bool m_isEditGem = false;
        GemInfo m_oldGemInfo;

    };


} // namespace O3DE::ProjectManager
