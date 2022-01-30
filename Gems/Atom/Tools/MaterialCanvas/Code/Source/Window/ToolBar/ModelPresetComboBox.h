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
#include <Viewport/MaterialCanvasViewportNotificationBus.h>
#endif

namespace MaterialCanvas
{
    class ModelPresetComboBox
        : public QComboBox
        , public MaterialCanvasViewportNotificationBus::Handler
    {
        Q_OBJECT
    public:
        ModelPresetComboBox(QWidget* parent = 0);
        ~ModelPresetComboBox();
        void Refresh();
    private:
        // MaterialCanvasViewportNotificationBus::Handler overrides...
        void OnModelPresetSelected(AZ::Render::ModelPresetPtr preset) override;
        void OnModelPresetAdded(AZ::Render::ModelPresetPtr preset) override;
        void OnModelPresetChanged(AZ::Render::ModelPresetPtr preset) override;
        void OnBeginReloadContent() override;
        void OnEndReloadContent() override;

        bool m_reloading = false;
        AZ::Render::ModelPresetPtrVector m_presets;
    };
} // namespace MaterialCanvas
