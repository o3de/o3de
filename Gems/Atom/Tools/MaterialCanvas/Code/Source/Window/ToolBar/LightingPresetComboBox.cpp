/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/Feature/Utils/LightingPreset.h>
#include <Viewport/MaterialCanvasViewportRequestBus.h>
#include <Window/ToolBar/LightingPresetComboBox.h>

namespace MaterialCanvas
{
    LightingPresetComboBox::LightingPresetComboBox(QWidget* parent)
        : QComboBox(parent)
    {
        connect(this, static_cast<void(QComboBox::*)(const int)>(&QComboBox::currentIndexChanged), this, [this](int index) {
            if (index >= 0 && index < m_presets.size())
            {
                MaterialCanvasViewportRequestBus::Broadcast(
                    &MaterialCanvasViewportRequestBus::Events::SelectLightingPreset, m_presets[index]);
            }
            });

        Refresh();

        MaterialCanvasViewportNotificationBus::Handler::BusConnect();
    }

    LightingPresetComboBox::~LightingPresetComboBox()
    {
        MaterialCanvasViewportNotificationBus::Handler::BusDisconnect();
    }

    void LightingPresetComboBox::Refresh()
    {
        clear();
        setDuplicatesEnabled(true);

        m_presets.clear();
        MaterialCanvasViewportRequestBus::BroadcastResult(m_presets, &MaterialCanvasViewportRequestBus::Events::GetLightingPresets);

        AZStd::sort(m_presets.begin(), m_presets.end(), [](const auto& a, const auto& b) {
            return a->m_displayName < b->m_displayName; });

        blockSignals(true);
        for (const auto& preset : m_presets)
        {
            addItem(preset->m_displayName.c_str());
        }
        blockSignals(false);

        AZ::Render::LightingPresetPtr preset;
        MaterialCanvasViewportRequestBus::BroadcastResult(preset, &MaterialCanvasViewportRequestBus::Events::GetLightingPresetSelection);
        OnLightingPresetSelected(preset);
    }

    void LightingPresetComboBox::OnLightingPresetSelected(AZ::Render::LightingPresetPtr preset)
    {
        auto presetItr = AZStd::find(m_presets.begin(), m_presets.end(), preset);
        if (presetItr != m_presets.end())
        {
            setCurrentIndex(static_cast<int>(AZStd::distance(m_presets.begin(), presetItr)));
        }
    }

    void LightingPresetComboBox::OnLightingPresetAdded(AZ::Render::LightingPresetPtr preset)
    {
        if (!m_reloading)
        {
            Refresh();
        }
    }

    void LightingPresetComboBox::OnLightingPresetChanged(AZ::Render::LightingPresetPtr preset)
    {
        if (!m_reloading)
        {
            auto presetItr = AZStd::find(m_presets.begin(), m_presets.end(), preset);
            if (presetItr != m_presets.end())
            {
                setItemText(static_cast<int>(AZStd::distance(m_presets.begin(), presetItr)), preset->m_displayName.c_str());
            }
            else
            {
                Refresh();
            }
        }
    }

    void LightingPresetComboBox::OnBeginReloadContent()
    {
        m_reloading = true;
    }

    void LightingPresetComboBox::OnEndReloadContent()
    {
        m_reloading = false;
        Refresh();
    }

} // namespace MaterialCanvas

#include <Window/ToolBar/moc_LightingPresetComboBox.cpp>
