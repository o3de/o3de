/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "MotionEventPresetCreateDialog.h"
#include <Source/Editor/ObjectEditor.h>
#include <AzCore/Component/ComponentApplication.h>
#include <QVBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QDialogButtonBox>

namespace EMStudio
{
    MotionEventPresetCreateDialog::MotionEventPresetCreateDialog(const MotionEventPreset& preset, QWidget* parent)
        : QDialog(parent)
        , m_preset(preset)
        , m_eventDataEditor(nullptr, nullptr, &m_preset.GetEventDatas(), this)
    {
        setWindowTitle("Motion Event Preset Creation");
        AZ::SerializeContext* context = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(context, &AZ::ComponentApplicationBus::Events::GetSerializeContext);

        m_editor = new EMotionFX::ObjectEditor(context, this);
        m_editor->AddInstance(&m_preset, azrtti_typeid<MotionEventPreset>());
        m_editor->setFixedWidth(450);

        QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::StandardButton::Ok | QDialogButtonBox::StandardButton::Cancel);
        connect(buttonBox, &QDialogButtonBox::accepted, this, &MotionEventPresetCreateDialog::OnCreateButton);
        connect(buttonBox, &QDialogButtonBox::rejected, this, &MotionEventPresetCreateDialog::reject);
        
        QVBoxLayout* mainLayout = new QVBoxLayout();
        mainLayout->setSizeConstraint(QLayout::SetFixedSize);
        mainLayout->setSpacing(5);
        mainLayout->addWidget(m_editor);
        mainLayout->addWidget(&m_eventDataEditor);
        mainLayout->addStretch(0);
        mainLayout->addWidget(buttonBox);
        mainLayout->setAlignment(Qt::AlignTop);
        setLayout(mainLayout);
    }

    MotionEventPreset& MotionEventPresetCreateDialog::GetPreset()
    {
        m_eventDataEditor.MoveEventDataSet(m_preset.GetEventDatas());
        return m_preset;
    }
    
    void MotionEventPresetCreateDialog::OnCreateButton()
    {
        if (m_preset.GetName().empty())
        {
            QMessageBox::critical(this, "Missing Information", "Please enter at least preset name.");
            return;
        }

        accept();
    }
} // namespace EMStudio

#include <EMotionFX/Tools/EMotionStudio/Plugins/StandardPlugins/Source/MotionEvents/moc_MotionEventPresetCreateDialog.cpp>
