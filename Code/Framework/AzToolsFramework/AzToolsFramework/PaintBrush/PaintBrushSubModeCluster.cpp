/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/PaintBrush/PaintBrushSubModeCluster.h>
#include <AzToolsFramework/PaintBrushSettings/PaintBrushSettingsRequestBus.h>
#include <AzToolsFramework/PaintBrushSettings/PaintBrushSettingsWindow.h>

namespace AzToolsFramework
{
    PaintBrushSubModeCluster::PaintBrushSubModeCluster()
    {
        auto RegisterClusterButton = [](AzToolsFramework::ViewportUi::ClusterId clusterId,
                                        const char* iconName,
                                        const char* tooltip) -> AzToolsFramework::ViewportUi::ButtonId
        {
            AzToolsFramework::ViewportUi::ButtonId buttonId;
            AzToolsFramework::ViewportUi::ViewportUiRequestBus::EventResult(
                buttonId,
                AzToolsFramework::ViewportUi::DefaultViewportId,
                &AzToolsFramework::ViewportUi::ViewportUiRequestBus::Events::CreateClusterButton,
                clusterId,
                AZStd::string::format(":/stylesheet/img/UI20/toolbar/%s.svg", iconName));

            AzToolsFramework::ViewportUi::ViewportUiRequestBus::Event(
                AzToolsFramework::ViewportUi::DefaultViewportId,
                &AzToolsFramework::ViewportUi::ViewportUiRequestBus::Events::SetClusterButtonTooltip,
                clusterId,
                buttonId,
                tooltip);

            return buttonId;
        };

        // create the cluster for showing the Paint Brush Settings window
        AzToolsFramework::ViewportUi::ViewportUiRequestBus::EventResult(
            m_paintBrushControlClusterId,
            AzToolsFramework::ViewportUi::DefaultViewportId,
            &AzToolsFramework::ViewportUi::ViewportUiRequestBus::Events::CreateCluster,
            AzToolsFramework::ViewportUi::Alignment::TopLeft);

        // create and register the "Show Paint Brush Settings" button.
        // This button is needed because the window is only shown while in component mode, and the window can be closed by the user,
        // so we need to provide an alternate way for the user to re-open the window.
        m_paintModeButtonId = RegisterClusterButton(m_paintBrushControlClusterId, "Paint", "Switch to Paint Mode");
        m_eyedropperModeButtonId = RegisterClusterButton(m_paintBrushControlClusterId, "Eyedropper", "Switch to Eyedropper Mode");
        m_smoothModeButtonId = RegisterClusterButton(m_paintBrushControlClusterId, "Smooth", "Switch to Smooth Mode");

        m_buttonSelectionHandler = AZ::Event<AzToolsFramework::ViewportUi::ButtonId>::Handler(
            [this](AzToolsFramework::ViewportUi::ButtonId buttonId)
            {
                AzToolsFramework::PaintBrushMode brushMode = AzToolsFramework::PaintBrushMode::Paintbrush;

                if (buttonId == m_eyedropperModeButtonId)
                {
                    brushMode = AzToolsFramework::PaintBrushMode::Eyedropper;
                }
                else if (buttonId == m_smoothModeButtonId)
                {
                    brushMode = AzToolsFramework::PaintBrushMode::Smooth;
                }

                AzToolsFramework::OpenViewPane(::PaintBrush::s_paintBrushSettingsName);

                AzToolsFramework::PaintBrushSettingsRequestBus::Broadcast(
                    &AzToolsFramework::PaintBrushSettingsRequestBus::Events::SetBrushMode, brushMode);
            });
        AzToolsFramework::ViewportUi::ViewportUiRequestBus::Event(
            AzToolsFramework::ViewportUi::DefaultViewportId,
            &AzToolsFramework::ViewportUi::ViewportUiRequestBus::Events::RegisterClusterEventHandler,
            m_paintBrushControlClusterId,
            m_buttonSelectionHandler);
    }

    PaintBrushSubModeCluster::~PaintBrushSubModeCluster()
    {
        AzToolsFramework::ViewportUi::ViewportUiRequestBus::Event(
            AzToolsFramework::ViewportUi::DefaultViewportId,
            &AzToolsFramework::ViewportUi::ViewportUiRequestBus::Events::RemoveCluster,
            m_paintBrushControlClusterId);
    }
} // namespace AzToolsFramework
