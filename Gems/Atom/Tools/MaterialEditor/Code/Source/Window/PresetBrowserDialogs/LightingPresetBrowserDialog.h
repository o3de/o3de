/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
