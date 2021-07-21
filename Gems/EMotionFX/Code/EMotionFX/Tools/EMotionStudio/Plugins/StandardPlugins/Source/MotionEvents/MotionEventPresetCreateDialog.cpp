/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
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
        Init();
        resize(450, 300);
    }

    MotionEventPreset& MotionEventPresetCreateDialog::GetPreset()
    {
        m_eventDataEditor.MoveEventDataSet(m_preset.GetEventDatas());
        return m_preset;
    }

    void MotionEventPresetCreateDialog::Init()
    {
        setWindowTitle("Motion Event Preset Creation");

        AZ::SerializeContext* context;
        AZ::ComponentApplicationBus::BroadcastResult(context, &AZ::ComponentApplicationBus::Events::GetSerializeContext);

        m_editor = new EMotionFX::ObjectEditor(context, this);
        m_editor->AddInstance(&m_preset, azrtti_typeid<MotionEventPreset>());

        QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::StandardButton::Ok | QDialogButtonBox::StandardButton::Cancel);
        connect(buttonBox, &QDialogButtonBox::accepted, this, &MotionEventPresetCreateDialog::OnCreateButton);
        connect(buttonBox, &QDialogButtonBox::rejected, this, &MotionEventPresetCreateDialog::reject);

        QVBoxLayout* verticalLayout = new QVBoxLayout(this);
        verticalLayout->setSpacing(5);
        verticalLayout->setSizeConstraint(QLayout::SizeConstraint::SetMinAndMaxSize);
        verticalLayout->addWidget(m_editor);
        verticalLayout->addWidget(&m_eventDataEditor);
        verticalLayout->addStretch();
        verticalLayout->addWidget(buttonBox);
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
