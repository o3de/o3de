/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <QComboBox>
#include <Atom/Viewport/MaterialViewportNotificationBus.h>
#endif

namespace MaterialEditor
{
    class LightingPresetComboBox
        : public QComboBox
        , public MaterialViewportNotificationBus::Handler
    {
        Q_OBJECT
    public:
        LightingPresetComboBox(QWidget* parent = 0);
        ~LightingPresetComboBox();
        void Refresh();
    private:
        // MaterialViewportNotificationBus::Handler overrides...
        void OnLightingPresetSelected(AZ::Render::LightingPresetPtr preset) override;
        void OnLightingPresetAdded(AZ::Render::LightingPresetPtr preset) override;
        void OnLightingPresetChanged(AZ::Render::LightingPresetPtr preset) override;
        void OnBeginReloadContent() override;
        void OnEndReloadContent() override;

        bool m_reloading = false;
        AZ::Render::LightingPresetPtrVector m_presets;
    };
} // namespace MaterialEditor
