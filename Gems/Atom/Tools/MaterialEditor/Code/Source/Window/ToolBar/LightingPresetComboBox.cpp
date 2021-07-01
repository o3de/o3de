/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Source/Window/ToolBar/LightingPresetComboBox.h>
#include <Atom/Viewport/MaterialViewportRequestBus.h>
#include <Atom/Feature/Utils/LightingPreset.h>

namespace MaterialEditor
{
    LightingPresetComboBox::LightingPresetComboBox(QWidget* parent)
        : QComboBox(parent)
    {
        connect(this, static_cast<void(QComboBox::*)(const int)>(&QComboBox::currentIndexChanged), this, [this](int index) {
            if (index >= 0 && index < m_presets.size())
            {
                MaterialViewportRequestBus::Broadcast(
                    &MaterialViewportRequestBus::Events::SelectLightingPreset, m_presets[index]);
            }
            });

        Refresh();

        MaterialViewportNotificationBus::Handler::BusConnect();
    }

    LightingPresetComboBox::~LightingPresetComboBox()
    {
        MaterialViewportNotificationBus::Handler::BusDisconnect();
    }

    void LightingPresetComboBox::Refresh()
    {
        clear();
        setDuplicatesEnabled(true);

        m_presets.clear();
        MaterialViewportRequestBus::BroadcastResult(m_presets, &MaterialViewportRequestBus::Events::GetLightingPresets);

        AZStd::sort(m_presets.begin(), m_presets.end(), [](const auto& a, const auto& b) {
            return a->m_displayName < b->m_displayName; });

        blockSignals(true);
        for (const auto& preset : m_presets)
        {
            addItem(preset->m_displayName.c_str());
        }
        blockSignals(false);

        AZ::Render::LightingPresetPtr preset;
        MaterialViewportRequestBus::BroadcastResult(preset, &MaterialViewportRequestBus::Events::GetLightingPresetSelection);
        OnLightingPresetSelected(preset);
    }

    void LightingPresetComboBox::OnLightingPresetSelected(AZ::Render::LightingPresetPtr preset)
    {
        auto presetItr = AZStd::find(m_presets.begin(), m_presets.end(), preset);
        if (presetItr != m_presets.end())
        {
            setCurrentIndex(AZStd::distance(m_presets.begin(), presetItr));
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
                setItemText(AZStd::distance(m_presets.begin(), presetItr), preset->m_displayName.c_str());
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

} // namespace MaterialEditor

#include <Source/Window/ToolBar/moc_LightingPresetComboBox.cpp>
