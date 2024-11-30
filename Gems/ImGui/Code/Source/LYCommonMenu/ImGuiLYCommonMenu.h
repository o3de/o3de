/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once
#include "ImGuiManager.h"

#ifdef IMGUI_ENABLED
#include <AzCore/Component/TickBus.h>
#include "ImGuiBus.h"
#include "ImGuiLYAssetExplorer.h"
#include "ImGuiLYCameraMonitor.h"
#include "ImGuiLYEntityOutliner.h"
#include "ImGuiInputMonitor.h"

namespace ImGui
{
    //! ImGuiDropDownState refers to the state of dropdowns of the main menu bar.
    enum class ImGuiDropdownState
    {
        Shown,
        Hidden
    };

    class ImGuiLYCommonMenu
        : public ImGuiUpdateListenerBus::Handler
        , public AZ::TickBus::Handler
    {
    public:
        ImGuiLYCommonMenu();
        ~ImGuiLYCommonMenu();

        void Initialize();
        void Shutdown();

        // -- ImGuiUpdateListenerBus::Handler Interface ----------------------------
        void OnImGuiUpdate() override;
        // -- ImGuiUpdateListenerBus::Handler Interface ----------------------------

        // -- AZ::TickBus::Handler Interface ---------------------------------------
        void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;
        // -- AZ::TickBus::Handler Interface ---------------------------------------

    private:
        void StartTelemetryCapture();
        void StopTelemetryCapture();
        void OnImGuiUpdate_DrawControllerLegend();

        float m_telemetryCaptureTime;
        float m_telemetryCaptureTimeRemaining;
        DisplayState m_telemetryCapturePreCaptureState;
        bool m_controllerLegendWindowVisible;

        ImGuiLYAssetExplorer m_assetExplorer;
        ImGuiLYCameraMonitor m_cameraMonitor;
        ImGuiLYEntityOutliner m_entityOutliner;
        ImGuiInputMonitor m_inputMonitor;
        bool m_showDeltaTimeGraphs = false;
        ImGui::LYImGuiUtils::HistogramContainer m_deltaTimeHistogram;
        ImGuiDropdownState m_dropdownState = ImGuiDropdownState::Hidden; //!< Keeps the state of the ImGui main menu dropdowns.
        //! Mark the dropdown for being hidden - this is used to prevent broadcasting that the dropdowns have been hidden
        //! in the case that the ImGui dropdown context has switched options.
        bool m_markedForHiding = false; 
        bool m_showImGuiDemo = false;

    };
}

#endif // IMGUI_ENABLED
