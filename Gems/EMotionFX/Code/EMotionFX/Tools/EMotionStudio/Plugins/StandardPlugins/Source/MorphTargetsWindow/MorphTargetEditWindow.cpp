/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "MorphTargetEditWindow.h"

#include <AzCore/Serialization/Locale.h>

#include "../../../../EMStudioSDK/Source/EMStudioManager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QGridLayout>
#include <QLabel>


namespace EMStudio
{
    MorphTargetEditWindow::MorphTargetEditWindow(EMotionFX::ActorInstance* actorInstance, EMotionFX::MorphTarget* morphTarget, QWidget* parent)
        : QDialog(parent)
    {
        // keep values
        m_actorInstance = actorInstance;
        m_morphTarget = morphTarget;

        // init the phoneme selection window
        m_phonemeSelectionWindow = nullptr;

        // set the window name
        setWindowTitle(QString("Edit Morph Target: %1").arg(morphTarget->GetName()));

        // create the layout
        QVBoxLayout* layout = new QVBoxLayout();
        layout->setAlignment(Qt::AlignVCenter);

        // get the morph target range min/max
        const float morphTargetRangeMin = m_morphTarget->GetRangeMin();
        const float morphTargetRangeMax = m_morphTarget->GetRangeMax();

        // create the range min label
        QLabel* rangeMinLabel = new QLabel("Range Min");

        // create the range min double spinbox
        m_rangeMin = new AzQtComponents::DoubleSpinBox();
        m_rangeMin->setSingleStep(0.1);
        m_rangeMin->setRange(std::numeric_limits<int32>::lowest(), morphTargetRangeMax);
        m_rangeMin->setValue(morphTargetRangeMin);
        connect(m_rangeMin, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &MorphTargetEditWindow::MorphTargetRangeMinValueChanged);

        // create the range max label
        QLabel* rangeMaxLabel = new QLabel("Range Max");

        // create the range max double spinbox
        m_rangeMax = new AzQtComponents::DoubleSpinBox();
        m_rangeMax->setSingleStep(0.1);
        m_rangeMax->setRange(morphTargetRangeMin, std::numeric_limits<int32>::max());
        m_rangeMax->setValue(morphTargetRangeMax);
        connect(m_rangeMax, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &MorphTargetEditWindow::MorphTargetRangeMaxValueChanged);

        // create the grid layout
        QGridLayout* gridLayout = new QGridLayout();
        gridLayout->addWidget(rangeMinLabel, 0, 0);
        gridLayout->addWidget(m_rangeMin, 0, 1);
        gridLayout->addWidget(rangeMaxLabel, 1, 0);
        gridLayout->addWidget(m_rangeMax, 1, 1);

        // create the buttons layout
        QHBoxLayout* buttonsLayout = new QHBoxLayout();
        buttonsLayout->setMargin(0);

        // create the OK button
        QPushButton* OKButton = new QPushButton("OK");
        connect(OKButton, &QPushButton::clicked, this, &MorphTargetEditWindow::Accepted);

        // create the cancel button
        QPushButton* cancelButton = new QPushButton("Cancel");
        connect(cancelButton, &QPushButton::clicked, this, &MorphTargetEditWindow::reject);

        // add widgets in the buttons layout
        buttonsLayout->addWidget(OKButton);
        buttonsLayout->addWidget(cancelButton);

        // add widgets in the layout
        layout->addLayout(gridLayout);
        layout->addLayout(buttonsLayout);

        // set the layout
        setLayout(layout);

        // set the default size
        resize(300, minimumHeight());
    }


    MorphTargetEditWindow::~MorphTargetEditWindow()
    {
        delete m_phonemeSelectionWindow;
    }


    void MorphTargetEditWindow::UpdateInterface()
    {
        // get the morph target range min/max
        const float morphTargetRangeMin = m_morphTarget->GetRangeMin();
        const float morphTargetRangeMax = m_morphTarget->GetRangeMax();

        // disable signals
        m_rangeMin->blockSignals(true);
        m_rangeMax->blockSignals(true);

        // update the range min
        m_rangeMin->setRange(std::numeric_limits<int32>::lowest(), morphTargetRangeMax);
        m_rangeMin->setValue(morphTargetRangeMin);

        // update the range max
        m_rangeMax->setRange(morphTargetRangeMin, std::numeric_limits<int32>::max());
        m_rangeMax->setValue(morphTargetRangeMax);

        // enable signals
        m_rangeMin->blockSignals(false);
        m_rangeMax->blockSignals(false);

        // update the phoneme selection window
        if (m_phonemeSelectionWindow)
        {
            m_phonemeSelectionWindow->UpdateInterface();
        }
    }


    void MorphTargetEditWindow::MorphTargetRangeMinValueChanged(double value)
    {
        const float rangeMin = (float)value;
        m_rangeMax->setRange(rangeMin, std::numeric_limits<int32>::max());
    }


    void MorphTargetEditWindow::MorphTargetRangeMaxValueChanged(double value)
    {
        const float rangeMax = (float)value;
        m_rangeMin->setRange(std::numeric_limits<int32>::lowest(), rangeMax);
    }


    void MorphTargetEditWindow::Accepted()
    {
        AZ::Locale::ScopedSerializationLocale scopedLocale; // Ensures that %f uses "." as decimal separator

        const float rangeMin = (float)m_rangeMin->value();
        const float rangeMax = (float)m_rangeMax->value();

        AZStd::string result;
        AZStd::string command = AZStd::string::format("AdjustMorphTarget -actorInstanceID %i -lodLevel %zu -name \"%s\" -rangeMin %f -rangeMax %f", m_actorInstance->GetID(), m_actorInstance->GetLODLevel(), m_morphTarget->GetNameString().c_str(), rangeMin, rangeMax);
        if (EMStudio::GetCommandManager()->ExecuteCommand(command, result) == false)
        {
            AZ_Error("EMotionFX", false, result.c_str());
        }

        accept();
    }


    void MorphTargetEditWindow::EditPhonemeButtonClicked()
    {
        delete m_phonemeSelectionWindow;
        m_phonemeSelectionWindow =  new PhonemeSelectionWindow(m_actorInstance->GetActor(), m_actorInstance->GetLODLevel(), m_morphTarget, this);
        m_phonemeSelectionWindow->exec();
    }
} // namespace EMStudio

#include <EMotionFX/Tools/EMotionStudio/Plugins/StandardPlugins/Source/MorphTargetsWindow/moc_MorphTargetEditWindow.cpp>
