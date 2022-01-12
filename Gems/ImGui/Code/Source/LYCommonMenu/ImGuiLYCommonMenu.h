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

namespace ImGui
{
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
        bool m_showDeltaTimeGraphs = false;
        ImGui::LYImGuiUtils::HistogramContainer m_deltaTimeHistogram;
    };
}

#endif // IMGUI_ENABLED
