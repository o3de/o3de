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

#pragma once

#if !defined(Q_MOC_RUN)
#include <Atom/Feature/Utils/LightingPreset.h>
#include <Atom/Viewport/MaterialViewportNotificationBus.h>
#include <AzCore/std/containers/vector.h>
#endif

#include <Window/PresetBrowserDialogs/PresetBrowserDialog.h>

namespace MaterialEditor
{
    //! Widget for managing and selecting from a library of preset assets
    class LightingPresetBrowserDialog : public PresetBrowserDialog
    {
        Q_OBJECT
    public:
        LightingPresetBrowserDialog(QWidget* parent = nullptr);
        ~LightingPresetBrowserDialog() = default;

    private:
        void SelectCurrentPreset() override;
        void SelectInitialPreset() override;

        AZ::Render::LightingPresetPtr m_initialPreset;
        AZStd::unordered_map<QListWidgetItem*, AZ::Render::LightingPresetPtr> m_listItemToPresetMap;
    };
} // namespace MaterialEditor
