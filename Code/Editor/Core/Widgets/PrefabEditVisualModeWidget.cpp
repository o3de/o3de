/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Core/Widgets/PrefabEditVisualModeWidget.h>

#include <AzToolsFramework/Editor/EditorSettingsAPIBus.h>
#include <AzToolsFramework/Viewport/ViewportSettings.h>
#include <EditorModeFeedback/EditorStateRequestsBus.h>

#include <QHBoxLayout>

PrefabEditVisualModeWidget::PrefabEditVisualModeWidget()
{
    // Create Label
    m_label = new QLabel(this);
    m_label->setText("Prefab Edit:");

    // Create ComboBox
    m_comboBox = new QComboBox(this);
    m_comboBox->setMinimumWidth(120);

    // Follow the same order as the PrefabEditModeUXSetting enum.
    m_comboBox->addItem(QObject::tr("Normal"));
    m_comboBox->addItem(QObject::tr("Monochromatic"));

    m_prefabEditMode =
        AzToolsFramework::PrefabEditModeEffectEnabled() ? PrefabEditModeUXSetting::Monochromatic : PrefabEditModeUXSetting::Normal;
    UpdatePrefabEditMode();

    connect(m_comboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [&](int index){
        OnComboBoxValueChanged(index);
    });

    // Add to Layout
    QHBoxLayout* layout = new QHBoxLayout(this);
    layout->addWidget(m_label);
    layout->addWidget(m_comboBox);
}

PrefabEditVisualModeWidget::~PrefabEditVisualModeWidget()
{
}

void PrefabEditVisualModeWidget::OnComboBoxValueChanged(int index)
{
    m_prefabEditMode = static_cast<PrefabEditModeUXSetting>(index);
    UpdatePrefabEditMode();
}

void PrefabEditVisualModeWidget::UpdatePrefabEditMode()
{
    m_comboBox->setCurrentIndex(aznumeric_cast<int>(m_prefabEditMode));

    switch (m_prefabEditMode)
    {
    case PrefabEditModeUXSetting::Normal:
        {
            AZ::Render::EditorStateRequestsBus::Event(
                AZ::Render::EditorState::FocusMode, &AZ::Render::EditorStateRequestsBus::Events::SetEnabled, false);
            AzToolsFramework::SetPrefabEditModeEffectEnabled(false);
            AzToolsFramework::EditorSettingsAPIBus::Broadcast(
                &AzToolsFramework::EditorSettingsAPIBus::Events::SaveSettingsRegistryFile);
            break;
        }
    case PrefabEditModeUXSetting::Monochromatic:
        {
            AZ::Render::EditorStateRequestsBus::Event(
                AZ::Render::EditorState::FocusMode, &AZ::Render::EditorStateRequestsBus::Events::SetEnabled, true);
            AzToolsFramework::SetPrefabEditModeEffectEnabled(true);
            AzToolsFramework::EditorSettingsAPIBus::Broadcast(
                &AzToolsFramework::EditorSettingsAPIBus::Events::SaveSettingsRegistryFile);
            break;
        }
    default:
        AZ_Error(
            "PrefabEditMode", false, AZStd::string::format("Unexpected edit mode: %zu", static_cast<size_t>(m_prefabEditMode)).c_str());
    }
}
