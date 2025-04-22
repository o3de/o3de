/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "MorphTargetGroupWidget.h"
#include <AzCore/Casting/numeric_cast.h>
#include <AzCore/Serialization/Locale.h>
#include <EMotionStudio/EMStudioSDK/Source/EMStudioManager.h>
#include <MCore/Source/StringConversions.h>
#include <QVBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <AzQtComponents/Components/Widgets/SliderCombo.h>

namespace EMStudio
{

    MorphTargetGroupWidget::MorphTargetGroupWidget(const char* name, EMotionFX::ActorInstance* actorInstance, const AZStd::vector<EMotionFX::MorphTarget*>& morphTargets, const AZStd::vector<EMotionFX::MorphSetupInstance::MorphTarget*>& morphTargetInstances, QWidget* parent)
        : QWidget(parent)
    {
        // keep values
        m_name = name;
        m_actorInstance = actorInstance;

        // init the edit window to nullptr
        m_editWindow = nullptr;

        // create the layout
        QVBoxLayout* layout = new QVBoxLayout();
        layout->setSpacing(2);
        layout->setMargin(0);

        // checkbox to enable/disable manual mode for all morph targets
        m_selectAll = new QCheckBox("Select All");
        m_selectAll->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Fixed);
        connect(m_selectAll, &QCheckBox::stateChanged, this, &MorphTargetGroupWidget::SetManualModeForAll);

        // button for resetting all morph targets
        QPushButton* resetAll = new QPushButton("Reset All");
        resetAll->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
        connect(resetAll, &QPushButton::clicked, this, &MorphTargetGroupWidget::ResetAll);

        // add controls to the top layout
        QHBoxLayout* topControlLayout = new QHBoxLayout();
        topControlLayout->addWidget(m_selectAll);
        topControlLayout->addWidget(resetAll);
        topControlLayout->setSpacing(5);
        topControlLayout->setMargin(0);

        // add the top control layout in the main layout
        layout->addLayout(topControlLayout);

        // create the grid layout to add all morph target in it
        QGridLayout* gridLayout = new QGridLayout();
        gridLayout->setHorizontalSpacing(5);
        gridLayout->setVerticalSpacing(2);

        const size_t numMorphTargets = morphTargets.size();
        m_morphTargets.resize(numMorphTargets);

        for (size_t i=0; i<numMorphTargets; ++i)
        {
            // keep values
            m_morphTargets[i].m_morphTarget = morphTargets[i];
            m_morphTargets[i].m_morphTargetInstance = morphTargetInstances[i];

            // add the number label
            const int intIndex = aznumeric_caster(i);
            QLabel* numberLabel = new QLabel(QString("%1").arg(intIndex + 1));
            gridLayout->addWidget(numberLabel, intIndex, 0);

            // add the manual mode checkbox
            m_morphTargets[i].m_manualMode = new QCheckBox();
            m_morphTargets[i].m_manualMode->setMaximumWidth(15);
            m_morphTargets[i].m_manualMode->setProperty("MorphTargetIndex", intIndex);
            m_morphTargets[i].m_manualMode->setStyleSheet("QCheckBox{ spacing: 0px; }");
            gridLayout->addWidget(m_morphTargets[i].m_manualMode, intIndex, 1);
            connect(m_morphTargets[i].m_manualMode, &QCheckBox::clicked, this, &MorphTargetGroupWidget::ManualModeClicked);

            // create slider to adjust the morph target
            m_morphTargets[i].m_sliderWeight = new AzQtComponents::SliderDoubleCombo();
            m_morphTargets[i].m_sliderWeight->setMinimumWidth(50);
            m_morphTargets[i].m_sliderWeight->setProperty("MorphTargetIndex", intIndex);
            m_morphTargets[i].m_sliderWeight->spinbox()->setMinimumWidth(40);
            m_morphTargets[i].m_sliderWeight->spinbox()->setMaximumWidth(40);
            gridLayout->addWidget(m_morphTargets[i].m_sliderWeight, intIndex, 2);
            connect(m_morphTargets[i].m_sliderWeight, &AzQtComponents::SliderDoubleCombo::valueChanged, this, &MorphTargetGroupWidget::SliderWeightMoved);
            connect(m_morphTargets[i].m_sliderWeight, &AzQtComponents::SliderDoubleCombo::editingFinished, this, &MorphTargetGroupWidget::SliderWeightReleased);

            // create the name label
            QLabel* nameLabel = new QLabel(morphTargets[i]->GetName());
            gridLayout->addWidget(nameLabel, intIndex, 3);

            // create the edit button
            QPushButton* edit = new QPushButton("Edit");
            edit->setProperty("MorphTargetIndex", intIndex);
            gridLayout->addWidget(edit, intIndex, 4);
            connect(edit, &QPushButton::clicked, this, &MorphTargetGroupWidget::EditClicked);
        }

        // add the grid layout in the main layout
        layout->addLayout(gridLayout);

        // set the layout
        setLayout(layout);

        // set the size policy
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);
    }


    // the destructor
    MorphTargetGroupWidget::~MorphTargetGroupWidget()
    {
        delete m_editWindow;
    }


    // set manual mode for all morph targets
    void MorphTargetGroupWidget::SetManualModeForAll(int value)
    {
        // create our command group
        MCore::CommandGroup commandGroup("Adjust morph targets");
        AZStd::string command;

        // loop trough all morph targets and enable/disable manual mode
        const size_t numMorphTargets = m_morphTargets.size();
        for (size_t i = 0; i < numMorphTargets; ++i)
        {
            EMotionFX::MorphTarget* morphTarget = m_morphTargets[i].m_morphTarget;

            command = AZStd::string::format("AdjustMorphTarget -actorInstanceID %i -lodLevel %zu -name \"%s\" -manualMode ", m_actorInstance->GetID(), m_actorInstance->GetLODLevel(), morphTarget->GetName());
            command += AZStd::to_string(value == Qt::Checked);
            commandGroup.AddCommandString(command);
        }

        AZStd::string result;
        if (CommandSystem::GetCommandManager()->ExecuteCommandGroup(commandGroup, result) == false)
        {
            AZ_Error("EMotionFX", false, result.c_str());
        }
    }


    // reset all morph targets
    void MorphTargetGroupWidget::ResetAll()
    {
        AZ::Locale::ScopedSerializationLocale scopedLocale; // Ensures that %f uses "." as decimal separator

        // create our command group
        MCore::CommandGroup commandGroup("Adjust morph targets");
        AZStd::string command;

        // loop trough all morph targets and enable/disable manual mode
        const size_t numMorphTargets = m_morphTargets.size();
        for (size_t i = 0; i < numMorphTargets; ++i)
        {
            EMotionFX::MorphTarget* morphTarget = m_morphTargets[i].m_morphTarget;

            command = AZStd::string::format("AdjustMorphTarget -actorInstanceID %i -lodLevel %zu -name \"%s\" -weight %f", m_actorInstance->GetID(), m_actorInstance->GetLODLevel(), morphTarget->GetName(), morphTarget->CalcZeroInfluenceWeight());
            commandGroup.AddCommandString(command);
        }

        AZStd::string result;
        if (CommandSystem::GetCommandManager()->ExecuteCommandGroup(commandGroup, result) == false)
        {
            AZ_Error("EMotionFX", false, result.c_str());
        }
    }


    // manual mode
    void MorphTargetGroupWidget::ManualModeClicked()
    {
        AZ::Locale::ScopedSerializationLocale scopedLocale; // Ensures that %f uses "." as decimal separator

        QCheckBox* checkBox = static_cast<QCheckBox*>(sender());
        const int morphTargetIndex = checkBox->property("MorphTargetIndex").toInt();
        EMotionFX::MorphTarget* morphTarget = m_morphTargets[morphTargetIndex].m_morphTarget;

        AZStd::string result;
        const AZStd::string command = AZStd::string::format("AdjustMorphTarget -actorInstanceID %i -lodLevel %zu -name \"%s\" -weight %f -manualMode %s", 
            m_actorInstance->GetID(), 
            m_actorInstance->GetLODLevel(), 
            morphTarget->GetName(), 
            0.0f, 
            AZStd::to_string(checkBox->isChecked()).c_str());
        if (EMStudio::GetCommandManager()->ExecuteCommand(command, result) == false)
        {
            AZ_Error("EMotionFX", false, result.c_str());
        }
    }


    // slider weight moved
    void MorphTargetGroupWidget::SliderWeightMoved()
    {
        // get the morph target
        AzQtComponents::SliderDoubleCombo* floatSlider = static_cast<AzQtComponents::SliderDoubleCombo*>(sender());
        const int morphTargetIndex = floatSlider->property("MorphTargetIndex").toInt();
        EMotionFX::MorphSetupInstance::MorphTarget* morphTargetInstance = m_morphTargets[morphTargetIndex].m_morphTargetInstance;

        // update the weight
        morphTargetInstance->SetWeight(aznumeric_cast<float>(floatSlider->value()));
    }


    // slider weight released
    void MorphTargetGroupWidget::SliderWeightReleased()
    {
        AZ::Locale::ScopedSerializationLocale scopedLocale; // Ensures that %f uses "." as decimal separator

        // get the morph target and the morph target instance
        AzQtComponents::SliderDoubleCombo* floatSlider = static_cast<AzQtComponents::SliderDoubleCombo*>(sender());
        const int morphTargetIndex = floatSlider->property("MorphTargetIndex").toInt();
        EMotionFX::MorphTarget* morphTarget = m_morphTargets[morphTargetIndex].m_morphTarget;
        if (!morphTarget)
        {
            return;
        }

        EMotionFX::MorphSetupInstance::MorphTarget* morphTargetInstance = m_morphTargets[morphTargetIndex].m_morphTargetInstance;

        // set the old weight to have the undo correct
        morphTargetInstance->SetWeight(m_morphTargets[morphTargetIndex].m_oldWeight);

        // execute command
        AZStd::string result;
        const AZStd::string command = AZStd::string::format("AdjustMorphTarget -actorInstanceID %i -lodLevel %zu -name \"%s\" -weight %f", m_actorInstance->GetID(), m_actorInstance->GetLODLevel(), morphTarget->GetName(), floatSlider->value());
        if (EMStudio::GetCommandManager()->ExecuteCommand(command, result) == false)
        {
            AZ_Error("EMotionFX", false, result.c_str());
        }

        // set the new old weight value
        m_morphTargets[morphTargetIndex].m_oldWeight = aznumeric_cast<float>(floatSlider->value());
    }


    // edit
    void MorphTargetGroupWidget::EditClicked()
    {
        // get the morph target
        QPushButton* button = static_cast<QPushButton*>(sender());
        const int morphTargetIndex = button->property("MorphTargetIndex").toInt();
        EMotionFX::MorphTarget* morphTarget = m_morphTargets[morphTargetIndex].m_morphTarget;

        // show the edit window
        delete m_editWindow;
        m_editWindow = new MorphTargetEditWindow(m_actorInstance, morphTarget, this);
        m_editWindow->exec();
    }


    // updates the interface of the morph target group
    void MorphTargetGroupWidget::UpdateInterface()
    {
        bool selectAllChecked = true;

        const size_t numMorphTargets = m_morphTargets.size();
        for (size_t i = 0; i < numMorphTargets; ++i)
        {
            const float rangeMin    = m_morphTargets[i].m_morphTarget->GetRangeMin();
            const float rangeMax    = m_morphTargets[i].m_morphTarget->GetRangeMax();
            const float weight      = m_morphTargets[i].m_morphTargetInstance->GetWeight();
            const bool  manualMode  = m_morphTargets[i].m_morphTargetInstance->GetIsInManualMode();

            // check if the select all should not be checked
            if (manualMode == false)
            {
                selectAllChecked = false;
            }

            // disable signals
            QSignalBlocker sb(m_morphTargets[i].m_sliderWeight);
            m_morphTargets[i].m_manualMode->blockSignals(true);

            // update the manual mode
            m_morphTargets[i].m_manualMode->setChecked(manualMode);

            // update the slider weight
            m_morphTargets[i].m_sliderWeight->setDisabled(!manualMode);
            m_morphTargets[i].m_sliderWeight->setRange(rangeMin, rangeMax);
            // enforce single step of 0.1
            m_morphTargets[i].m_sliderWeight->slider()->setNumSteps(aznumeric_cast<int>((rangeMax - rangeMin) / 0.1));
            m_morphTargets[i].m_sliderWeight->setValue(weight);

            // enable signals
            m_morphTargets[i].m_manualMode->blockSignals(false);

            // store the current weight
            // the weight is updated in realtime but before to execute the adjust command it has to be reset to have the undo correct
            m_morphTargets[i].m_oldWeight = weight;
        }

        // update the select all
        m_selectAll->blockSignals(true);
        m_selectAll->setChecked(selectAllChecked);
        m_selectAll->blockSignals(false);

        // update the edit window
        if (m_editWindow)
        {
            m_editWindow->UpdateInterface();
        }
    }

    void MorphTargetGroupWidget::UpdateMorphTarget(const char* name)
    {
        // update the row
        const size_t numMorphTargets = m_morphTargets.size();
        for (size_t i = 0; i < numMorphTargets; ++i)
        {
            // continue of the name is not the same
            if (m_morphTargets[i].m_morphTarget->GetNameString() != name)
            {
                continue;
            }

            // get values
            const float rangeMin    = m_morphTargets[i].m_morphTarget->GetRangeMin();
            const float rangeMax    = m_morphTargets[i].m_morphTarget->GetRangeMax();
            const float weight      = m_morphTargets[i].m_morphTargetInstance->GetWeight();
            const bool  manualMode  = m_morphTargets[i].m_morphTargetInstance->GetIsInManualMode();

            // disable signals
            QSignalBlocker sb(m_morphTargets[i].m_sliderWeight);
            m_morphTargets[i].m_manualMode->blockSignals(true);

            // update the manual mode
            m_morphTargets[i].m_manualMode->setChecked(manualMode);

            // update the slider weight
            m_morphTargets[i].m_sliderWeight->setDisabled(!manualMode);
            m_morphTargets[i].m_sliderWeight->setRange(rangeMin, rangeMax);
            // enforce single step of 0.1
            m_morphTargets[i].m_sliderWeight->slider()->setNumSteps(aznumeric_cast<int>((rangeMax - rangeMin) / 0.1));
            m_morphTargets[i].m_sliderWeight->setValue(weight);

            // enable signals
            m_morphTargets[i].m_manualMode->blockSignals(false);

            // store the current weight
            // the weight is updated in realtime but before to execute the adjust command it has to be reset to have the undo correct
            m_morphTargets[i].m_oldWeight = weight;

            // update edit window in case it's the edit of this morph target
            if (m_editWindow && m_editWindow->GetMorphTarget() == m_morphTargets[i].m_morphTarget)
            {
                m_editWindow->UpdateInterface();
            }

            // stop here because we found it
            break;
        }

        // update the select all
        bool selectAllChecked = true;
        for (uint32 i = 0; i < numMorphTargets; ++i)
        {
            if (m_morphTargets[i].m_morphTargetInstance->GetIsInManualMode() == false)
            {
                selectAllChecked = false;
                break;
            }
        }
        m_selectAll->blockSignals(true);
        m_selectAll->setChecked(selectAllChecked);
        m_selectAll->blockSignals(false);
    }
} // namespace EMStudio


#include <EMotionFX/Tools/EMotionStudio/Plugins/StandardPlugins/Source/MorphTargetsWindow/moc_MorphTargetGroupWidget.cpp>
