/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once
//AZTF-SHARED

#include <AzToolsFramework/ViewportUi/ViewportUiRequestBus.h>
#include <AzToolsFramework/PaintBrush/GlobalPaintBrushSettingsNotificationBus.h>
#include <AzToolsFramework/AzToolsFrameworkAPI.h>

namespace AzToolsFramework
{
    //! Create and manage the Viewport UI SubMode cluster of buttons that enable the user to switch between paint modes.
    class AZTF_API PaintBrushSubModeCluster : public AzToolsFramework::GlobalPaintBrushSettingsNotificationBus::Handler
    {
    public:
        PaintBrushSubModeCluster();
        ~PaintBrushSubModeCluster() override;

        // GlobalPaintBrushSettingsNotificationBus overrides...
        void OnPaintBrushModeChanged(PaintBrushMode newBrushMode) override;

    private:
        AZ::Event<ViewportUi::ButtonId>::Handler m_buttonSelectionHandler;

        ViewportUi::ClusterId m_paintBrushControlClusterId;

        ViewportUi::ButtonId m_paintModeButtonId;
        ViewportUi::ButtonId m_eyedropperModeButtonId;
        ViewportUi::ButtonId m_smoothModeButtonId;
    };
} // namespace AzToolsFramework

