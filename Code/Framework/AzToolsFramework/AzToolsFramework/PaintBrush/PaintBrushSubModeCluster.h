/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzToolsFramework/ViewportUi/ViewportUiRequestBus.h>

namespace AzToolsFramework
{
    //! Create and manage the Viewport UI SubMode cluster of buttons that enable the user to switch between paint modes.
    class PaintBrushSubModeCluster
    {
    public:
        PaintBrushSubModeCluster();
        ~PaintBrushSubModeCluster();

    private:
        AZ::Event<ViewportUi::ButtonId>::Handler m_buttonSelectionHandler;

        ViewportUi::ClusterId m_paintBrushControlClusterId;

        ViewportUi::ButtonId m_paintModeButtonId;
        ViewportUi::ButtonId m_eyedropperModeButtonId;
        ViewportUi::ButtonId m_smoothModeButtonId;
    };
} // namespace AzToolsFramework

