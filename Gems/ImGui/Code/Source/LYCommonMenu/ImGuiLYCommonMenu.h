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
    };
}

#endif // IMGUI_ENABLED
