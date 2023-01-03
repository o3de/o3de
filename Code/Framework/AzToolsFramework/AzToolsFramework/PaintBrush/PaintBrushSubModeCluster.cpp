/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/PaintBrush/PaintBrushSubModeCluster.h>
#include <AzToolsFramework/PaintBrush/GlobalPaintBrushSettingsRequestBus.h>
#include <AzToolsFramework/PaintBrush/GlobalPaintBrushSettingsWindow.h>

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
                PaintBrushMode brushMode = PaintBrushMode::Paintbrush;

                if (buttonId == m_eyedropperModeButtonId)
                {
                    brushMode = PaintBrushMode::Eyedropper;
                }
                else if (buttonId == m_smoothModeButtonId)
                {
                    brushMode = PaintBrushMode::Smooth;
                }

                AzToolsFramework::OpenViewPane(AzToolsFramework::s_paintBrushSettingsName);

                AzToolsFramework::GlobalPaintBrushSettingsRequestBus::Broadcast(
                    &AzToolsFramework::GlobalPaintBrushSettingsRequestBus::Events::SetBrushMode, brushMode);
            });
        AzToolsFramework::ViewportUi::ViewportUiRequestBus::Event(
            AzToolsFramework::ViewportUi::DefaultViewportId,
            &AzToolsFramework::ViewportUi::ViewportUiRequestBus::Events::RegisterClusterEventHandler,
            m_paintBrushControlClusterId,
            m_buttonSelectionHandler);

        AzToolsFramework::GlobalPaintBrushSettingsNotificationBus::Handler::BusConnect();

        // Set the initially-active brush mode button.
        PaintBrushMode brushMode = PaintBrushMode::Paintbrush;
        AzToolsFramework::GlobalPaintBrushSettingsRequestBus::BroadcastResult(
            brushMode, &AzToolsFramework::GlobalPaintBrushSettingsRequestBus::Events::GetBrushMode);
        OnPaintBrushModeChanged(brushMode);
    }

    PaintBrushSubModeCluster::~PaintBrushSubModeCluster()
    {
        AzToolsFramework::GlobalPaintBrushSettingsNotificationBus::Handler::BusDisconnect();

        AzToolsFramework::ViewportUi::ViewportUiRequestBus::Event(
            AzToolsFramework::ViewportUi::DefaultViewportId,
            &AzToolsFramework::ViewportUi::ViewportUiRequestBus::Events::RemoveCluster,
            m_paintBrushControlClusterId);
    }

    void PaintBrushSubModeCluster::OnPaintBrushModeChanged(PaintBrushMode newBrushMode)
    {
        // Change the active brush mode button based on the newly-active brush mode.

        AzToolsFramework::ViewportUi::ButtonId buttonId;

        switch (newBrushMode)
        {
        case PaintBrushMode::Paintbrush:
            buttonId = m_paintModeButtonId;
            break;
        case PaintBrushMode::Eyedropper:
            buttonId = m_eyedropperModeButtonId;
            break;
        case PaintBrushMode::Smooth:
            buttonId = m_smoothModeButtonId;
            break;
        default:
            AZ_Assert(false, "Unknown brush mode type.");
            break;
        }

        ViewportUi::ViewportUiRequestBus::Event(
            ViewportUi::DefaultViewportId,
            &ViewportUi::ViewportUiRequestBus::Events::SetClusterActiveButton,
            m_paintBrushControlClusterId,
            buttonId);
    }

} // namespace AzToolsFramework
