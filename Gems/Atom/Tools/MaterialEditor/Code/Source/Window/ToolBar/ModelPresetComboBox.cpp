/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Source/Window/ToolBar/ModelPresetComboBox.h>
#include <Atom/Viewport/MaterialViewportRequestBus.h>
#include <Atom/Feature/Utils/ModelPreset.h>

namespace MaterialEditor
{
    ModelPresetComboBox::ModelPresetComboBox(QWidget* parent)
        : QComboBox(parent)
    {
        connect(this, static_cast<void(QComboBox::*)(const int)>(&QComboBox::currentIndexChanged), this, [this](int index) {
            if (index >= 0 && index < m_presets.size())
            {
                MaterialViewportRequestBus::Broadcast(
                    &MaterialViewportRequestBus::Events::SelectModelPreset, m_presets[index]);
            }
            });

        Refresh();

        MaterialViewportNotificationBus::Handler::BusConnect();
    }

    ModelPresetComboBox::~ModelPresetComboBox()
    {
        MaterialViewportNotificationBus::Handler::BusDisconnect();
    }

    void ModelPresetComboBox::Refresh()
    {
        clear();
        setDuplicatesEnabled(true);

        m_presets.clear();
        MaterialViewportRequestBus::BroadcastResult(m_presets, &MaterialViewportRequestBus::Events::GetModelPresets);

        AZStd::sort(m_presets.begin(), m_presets.end(), [](const auto& a, const auto& b) {
            return a->m_displayName < b->m_displayName; });

        blockSignals(true);
        for (const auto& preset : m_presets)
        {
            addItem(preset->m_displayName.c_str());
        }
        blockSignals(false);

        AZ::Render::ModelPresetPtr preset;
        MaterialViewportRequestBus::BroadcastResult(preset, &MaterialViewportRequestBus::Events::GetModelPresetSelection);
        OnModelPresetSelected(preset);
    }

    void ModelPresetComboBox::OnModelPresetSelected(AZ::Render::ModelPresetPtr preset)
    {
        auto presetItr = AZStd::find(m_presets.begin(), m_presets.end(), preset);
        if (presetItr != m_presets.end())
        {
            setCurrentIndex(AZStd::distance(m_presets.begin(), presetItr));
        }
    }

    void ModelPresetComboBox::OnModelPresetAdded(AZ::Render::ModelPresetPtr preset)
    {
        if (!m_reloading)
        {
            Refresh();
        }
    }

    void ModelPresetComboBox::OnModelPresetChanged(AZ::Render::ModelPresetPtr preset)
    {
        if (!m_reloading)
        {
            auto presetItr = AZStd::find(m_presets.begin(), m_presets.end(), preset);
            if (presetItr != m_presets.end())
            {
                setItemText(AZStd::distance(m_presets.begin(), presetItr), preset->m_displayName.c_str());
            }
            else
            {
                Refresh();
            }
        }
    }

    void ModelPresetComboBox::OnBeginReloadContent()
    {
        m_reloading = true;
    }

    void ModelPresetComboBox::OnEndReloadContent()
    {
        m_reloading = false;
        Refresh();
    }

} // namespace MaterialEditor

#include <Source/Window/ToolBar/moc_ModelPresetComboBox.cpp>
