/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/Feature/Utils/ModelPreset.h>
#include <Atom/Viewport/MaterialViewportRequestBus.h>
#include <AtomToolsFramework/Util/Util.h>
#include <AzFramework/Application/Application.h>
#include <Window/PresetBrowserDialogs/ModelPresetBrowserDialog.h>

namespace MaterialEditor
{
    ModelPresetBrowserDialog::ModelPresetBrowserDialog(QWidget* parent)
        : PresetBrowserDialog(parent)
    {
        QSignalBlocker signalBlocker(this);

        setWindowTitle("Model Preset Browser");

        MaterialViewportRequestBus::BroadcastResult(m_initialPreset, &MaterialViewportRequestBus::Events::GetModelPresetSelection);

        AZ::Render::ModelPresetPtrVector presets;
        MaterialViewportRequestBus::BroadcastResult(presets, &MaterialViewportRequestBus::Events::GetModelPresets);
        AZStd::sort(presets.begin(), presets.end(), [](const auto& a, const auto& b) { return a->m_displayName < b->m_displayName; });

        const int itemSize = aznumeric_cast<int>(
            AtomToolsFramework::GetSettingOrDefault<AZ::u64>("/O3DE/Atom/MaterialEditor/PresetBrowserDialog/ModelItemSize", 90));

        QListWidgetItem* selectedItem = nullptr;
        for (const auto& preset : presets)
        {
            QListWidgetItem* item = CreateListItem(preset->m_displayName.c_str(), preset->m_modelAsset.GetId(), QSize(itemSize, itemSize));

            m_listItemToPresetMap[item] = preset;

            if (m_initialPreset == preset)
            {
                selectedItem = item;
            }
        }

        if (selectedItem)
        {
            m_ui->m_presetList->setCurrentItem(selectedItem);
            m_ui->m_presetList->scrollToItem(selectedItem);
        }
    }

    void ModelPresetBrowserDialog::SelectCurrentPreset()
    {
        auto presetItr = m_listItemToPresetMap.find(m_ui->m_presetList->currentItem());
        if (presetItr != m_listItemToPresetMap.end())
        {
            MaterialViewportRequestBus::Broadcast(&MaterialViewportRequestBus::Events::SelectModelPreset, presetItr->second);
        }
    }

    void ModelPresetBrowserDialog::SelectInitialPreset()
    {
        MaterialViewportRequestBus::Broadcast(&MaterialViewportRequestBus::Events::SelectModelPreset, m_initialPreset);
    }

} // namespace MaterialEditor

#include <Window/PresetBrowserDialogs/moc_ModelPresetBrowserDialog.cpp>
