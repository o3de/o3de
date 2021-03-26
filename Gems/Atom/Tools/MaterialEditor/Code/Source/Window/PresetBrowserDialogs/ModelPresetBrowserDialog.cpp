/*
 * All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
 * its licensors.
 *
 * For complete copyright and license terms please see the LICENSE at the root of this
 * distribution (the "License"). All use of this software is governed by the License,
 * or, if provided, by the license below or the license accompanying this file. Do not
 * remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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

        QListWidgetItem* selectedItem = nullptr;
        for (const auto& preset : presets)
        {
            QImage image;
            MaterialViewportRequestBus::BroadcastResult(image, &MaterialViewportRequestBus::Events::GetModelPresetPreview, preset);

            QListWidgetItem* item = CreateListItem(preset->m_displayName.c_str(), image);

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
