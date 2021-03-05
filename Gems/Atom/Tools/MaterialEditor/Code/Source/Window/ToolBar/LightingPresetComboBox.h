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
