/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/Feature/Utils/ModelPreset.h>
#include <Viewport/MaterialCanvasViewportRequestBus.h>
#include <Window/ToolBar/ModelPresetComboBox.h>

namespace MaterialCanvas
{
    ModelPresetComboBox::ModelPresetComboBox(QWidget* parent)
        : QComboBox(parent)
    {
        connect(this, static_cast<void(QComboBox::*)(const int)>(&QComboBox::currentIndexChanged), this, [this](int index) {
            if (index >= 0 && index < m_presets.size())
            {
                MaterialCanvasViewportRequestBus::Broadcast(
                    &MaterialCanvasViewportRequestBus::Events::SelectModelPreset, m_presets[index]);
            }
            });

        Refresh();

        MaterialCanvasViewportNotificationBus::Handler::BusConnect();
    }

    ModelPresetComboBox::~ModelPresetComboBox()
    {
        MaterialCanvasViewportNotificationBus::Handler::BusDisconnect();
    }

    void ModelPresetComboBox::Refresh()
    {
        clear();
        setDuplicatesEnabled(true);

        m_presets.clear();
        MaterialCanvasViewportRequestBus::BroadcastResult(m_presets, &MaterialCanvasViewportRequestBus::Events::GetModelPresets);

        AZStd::sort(m_presets.begin(), m_presets.end(), [](const auto& a, const auto& b) {
            return a->m_displayName < b->m_displayName; });

        blockSignals(true);
        for (const auto& preset : m_presets)
        {
            addItem(preset->m_displayName.c_str());
        }
        blockSignals(false);

        AZ::Render::ModelPresetPtr preset;
        MaterialCanvasViewportRequestBus::BroadcastResult(preset, &MaterialCanvasViewportRequestBus::Events::GetModelPresetSelection);
        OnModelPresetSelected(preset);
    }

    void ModelPresetComboBox::OnModelPresetSelected(AZ::Render::ModelPresetPtr preset)
    {
        auto presetItr = AZStd::find(m_presets.begin(), m_presets.end(), preset);
        if (presetItr != m_presets.end())
        {
            setCurrentIndex(static_cast<int>(AZStd::distance(m_presets.begin(), presetItr)));
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
                setItemText(static_cast<int>(AZStd::distance(m_presets.begin(), presetItr)), preset->m_displayName.c_str());
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

} // namespace MaterialCanvas

#include <Window/ToolBar/moc_ModelPresetComboBox.cpp>
