/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#ifdef IMGUI_ENABLED
#include "ImGuiManager.h"
#include "ImGuiBus.h"
#include "LYImGuiUtils/HistogramContainer.h"
#include <AzCore/Component/TickBus.h>

namespace ImGui
{
    // Per Camera Data and Histograms to record for history
    struct CameraInfo
    {
        AZ::EntityId m_camId;
        AZStd::string m_camName; // cache the name, especially useful if a camera is deleted after use
        ImGui::LYImGuiUtils::HistogramContainer m_fovHisto;
        ImGui::LYImGuiUtils::HistogramContainer m_facingVectorDeltaHisto;
        ImGui::LYImGuiUtils::HistogramContainer m_positionDeltaHisto;

        AZ::Vector3 m_lastWorldPos;
        AZ::Vector3 m_lastFacingVector;

        float m_activeTime;
        int m_activeFrames;
    };

    class ImGuiLYCameraMonitor
        : public AZ::TickBus::Handler
        , public ImGuiCameraMonitorRequestBus::Handler
    {
    public:
        ImGuiLYCameraMonitor();
        virtual ~ImGuiLYCameraMonitor() = default;

        // Called from owner
        void Initialize();
        void Shutdown();

        // Draw the ImGui Menu
        void ImGuiUpdate();

        // -- AZ::TickBus::Handler Interface ---------------------------------------
        void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;
        // -- AZ::TickBus::Handler Interface ---------------------------------------

        // -- ImGuiCameraMonitorRequestBus::Handler Interface ----------------------
        void SetEnabled(bool enabled) override { m_enabled = enabled; m_recordCameraData = enabled; }
        // -- ImGuiCameraMonitorRequestBus::Handler Interface ----------------------

        // Toggle the menu on and off
        void ToggleEnabled() { m_enabled = !m_enabled; }

    private:
        // flag for if the entire Menu enabled / visible
        bool m_enabled;

        // flag for if we should record camera data ( on OnTick.. so menu can be off and we can still record data )
        bool m_recordCameraData;

        // The size of camera History we want to keep ( within reason )
        int m_camHistorySize;

        // The current Camera Entity Id ( so we can easily keep track of camera switches next frame )
        AZ::EntityId m_currentCamera;

        // A history of per camera data. Front of the queue is the current camera
        AZStd::deque<CameraInfo> m_cameraHistory;

        // Additionally, keep 1 history of the active camera ( global = one histogram of data, even between camera switches )
        CameraInfo m_globalActiveCamInfo;
        ImGui::LYImGuiUtils::HistogramContainer m_dofMinZHisto;
        ImGui::LYImGuiUtils::HistogramContainer m_dofMinZBlendMultHisto;
        ImGui::LYImGuiUtils::HistogramContainer m_dofMinZScaleHisto;

        // Helper functions for the ImGui Update
        void ImGuiUpdate_DrawMenu();
        void ImGuiUpdate_DrawOptions();

        // Helper functions for the OnTick Update
        void OnTick_GatherCameraData(float deltaTime);
        void OnTick_GatherCameraData_PushNewCameraToHistory(AZ::EntityId newCamId);

        static float GetAngleBetweenVectors(const AZ::Vector3& v1, const AZ::Vector3& v2);
    };
}
#endif // IMGUI_ENABLED
