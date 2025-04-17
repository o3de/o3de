/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#ifdef IMGUI_ENABLED
#include "ImGuiLYCameraMonitor.h"
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Component/ComponentBus.h>
#include <AzCore/Component/TransformBus.h>
#include <AzFramework/Components/CameraBus.h>

#include "ImGuiColorDefines.h"


namespace ImGui
{
    ImGuiLYCameraMonitor::ImGuiLYCameraMonitor()
        : m_enabled(false)
        , m_recordCameraData(false)
        , m_camHistorySize(10)
        , m_currentCamera()
        , m_cameraHistory()
    {
    }

    void ImGuiLYCameraMonitor::Initialize()
    {
        // Connect to EBUSes
        AZ::TickBus::Handler::BusConnect();
        ImGuiCameraMonitorRequestBus::Handler::BusConnect();

        // Init Histogram Containers
        m_dofMinZHisto.Init(             "DOF Min Z",            120, LYImGuiUtils::HistogramContainer::ViewType::Histogram, true, 0.0f,  0.0f);
        m_dofMinZBlendMultHisto.Init(    "DOF Min Z Blend Mult", 120, LYImGuiUtils::HistogramContainer::ViewType::Histogram, true, 50.0f, 50.0f);
        m_dofMinZScaleHisto.Init(        "DOF Min Z Scale",      120, LYImGuiUtils::HistogramContainer::ViewType::Histogram, true, 50.0f, 50.0f);

        m_globalActiveCamInfo.m_fovHisto.Init(                  "FOV",                      120, LYImGuiUtils::HistogramContainer::ViewType::Lines,     true, 50.0f, 50.0f);
        m_globalActiveCamInfo.m_facingVectorDeltaHisto.Init(    "Facing Vec Frame Delta",   120, LYImGuiUtils::HistogramContainer::ViewType::Histogram, true, 0.0f,  0.0f);
        m_globalActiveCamInfo.m_positionDeltaHisto.Init(        "Position Frame Delta",     120, LYImGuiUtils::HistogramContainer::ViewType::Histogram, true, 0.0f,  0.0f);
    }

    void ImGuiLYCameraMonitor::Shutdown()
    {
        AZ::TickBus::Handler::BusDisconnect();
        ImGuiCameraMonitorRequestBus::Handler::BusDisconnect();
    }

    void ImGuiLYCameraMonitor::ImGuiUpdate()
    {
        // Manage main window visibility
        if (m_enabled)
        {
            if (ImGui::Begin("Camera Monitor", &m_enabled, ImGuiWindowFlags_MenuBar|ImGuiWindowFlags_HorizontalScrollbar|ImGuiWindowFlags_NoSavedSettings))
            {
                // Draw the Entire Main Menu Window Area
                ImGuiUpdate_DrawMenu();

                //// Draw Menu Bar
                if (ImGui::BeginMenuBar())
                {
                    if (ImGui::BeginMenu("Options##cameraMonitor"))
                    {
                        ImGuiUpdate_DrawOptions();

                        ImGui::EndMenu();
                    }

                    ImGui::EndMenuBar();
                }
            }
            ImGui::End();
        }
    }

    void ImGuiLYCameraMonitor::ImGuiUpdate_DrawMenu()
    {
        ImGui::Checkbox("Record Camera Data", &m_recordCameraData);

        if (ImGui::CollapsingHeader("Current Camera Monitor", ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed))
        {
            // Get Current Camera Name
            AZStd::string camName("*invalid_name*");
            AZ::ComponentApplicationBus::BroadcastResult(camName, &AZ::ComponentApplicationBus::Events::GetEntityName, m_currentCamera);
            ImGui::TextColored(ImGui::Colors::s_NiceLabelColor, " Active Cam:  %s %s", m_currentCamera.ToString().c_str(), camName.c_str());
            ImGui::TextColored(ImGui::Colors::s_NiceLabelColor, " Active Cam frames/time:  %d / %.02f", m_globalActiveCamInfo.m_activeFrames, m_globalActiveCamInfo.m_activeTime);
            
            ImGui::Columns(3);
            m_globalActiveCamInfo.m_fovHisto.Draw(ImGui::GetColumnWidth(), 140.0f);
            ImGui::NextColumn();
            m_globalActiveCamInfo.m_facingVectorDeltaHisto.Draw(ImGui::GetColumnWidth(), 140.0f);
            ImGui::NextColumn();
            m_globalActiveCamInfo.m_positionDeltaHisto.Draw(ImGui::GetColumnWidth(), 140.0f);
            ImGui::NextColumn();
            ImGui::Columns(1);
        }

        if (ImGui::CollapsingHeader("Camera History", ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed))
        {

            for (int i = 0; i < m_cameraHistory.size(); ++i)
            {
                CameraInfo& camInfo = m_cameraHistory[i];
                ImGui::BeginChild(AZStd::string::format("cameraInfo%d", i).c_str(), ImVec2(0, 60.0f), true);
                
                ImGui::TextColored(ImGui::Colors::s_NiceLabelColor, "Previous Cam %d:  %s %s", i, camInfo.m_camId.ToString().c_str(), camInfo.m_camName.c_str());
                ImGui::TextColored(ImGui::Colors::s_NiceLabelColor, "  Active Cam frames/time:  %d / %.02f", camInfo.m_activeFrames, camInfo.m_activeTime);

                if (ImGui::IsWindowHovered())
                {
                    ImGui::BeginTooltip();
                    ImGui::BeginChild(AZStd::string::format("cameraInfoTooltip%d", i).c_str(), ImVec2(500.0f, 140.0f), true);
                    ImGui::Columns(3);
                    camInfo.m_fovHisto.Draw(ImGui::GetColumnWidth(), 130.0f);
                    ImGui::NextColumn();
                    camInfo.m_facingVectorDeltaHisto.Draw(ImGui::GetColumnWidth(), 130.0f);
                    ImGui::NextColumn();
                    camInfo.m_positionDeltaHisto.Draw(ImGui::GetColumnWidth(), 130.0f);
                    ImGui::Columns(1);

                    ImGui::EndChild();

                    ImGui::EndTooltip();
                }

                ImGui::EndChild();
            }
        }
    }

    void ImGuiLYCameraMonitor::ImGuiUpdate_DrawOptions()
    {
        ImGui::SliderInt("Camera History Size", &m_camHistorySize, 1, 100);
        // if we have lowered the camera history size, we should remove oldest here
        while (m_camHistorySize < m_cameraHistory.size())
        {
            m_cameraHistory.pop_back();
        }
    }

    void ImGuiLYCameraMonitor::OnTick(float deltaTime, [[maybe_unused]] AZ::ScriptTimePoint time)
    {
        if (m_recordCameraData)
        {
            OnTick_GatherCameraData(deltaTime);
        }
    }

    void ImGuiLYCameraMonitor::OnTick_GatherCameraData(float deltaTime)
    {
        // Get the Active Camera 
        AZ::EntityId activeCam;
        Camera::CameraSystemRequestBus::BroadcastResult(activeCam, &Camera::CameraSystemRequests::GetActiveCamera);

        // if this is the first time we are seeing this camera, move the previous camera to the history and start recording this camera history
        if (activeCam != m_currentCamera)
        {
            OnTick_GatherCameraData_PushNewCameraToHistory(activeCam);
        }

        // Catch corner cases where no camera History has been added due to startup conditions ( Camera with Invalid Entity Id occurs during startup, which we don't add to the history )
        if (m_cameraHistory.empty())
        {
            return;
        }

        // Grab the current cameraInfo stuct ( may be the one that was just created above! Whatev! )
        CameraInfo& currentCam = m_cameraHistory.front();

        float fov;
        Camera::CameraRequestBus::EventResult(fov, currentCam.m_camId, &Camera::CameraComponentRequests::GetFovDegrees);
        currentCam.m_fovHisto.PushValue(fov);

        // Grab the current Transform to figure out Position and Orientation Drame Deltas, then push those into some Histogram Containers
        AZ::Transform camTransform;
        AZ::TransformBus::EventResult(camTransform, currentCam.m_camId, &AZ::TransformBus::Events::GetWorldTM);
        const AZ::Vector3& camFacing = camTransform.GetBasisY();
        const float positionFrameDelta = (camTransform.GetTranslation() - currentCam.m_lastWorldPos).GetLength();
        currentCam.m_lastWorldPos = camTransform.GetTranslation();
        currentCam.m_positionDeltaHisto.PushValue(positionFrameDelta);
        const float facingDirFrameDelta = GetAngleBetweenVectors(camFacing, currentCam.m_lastFacingVector);
        currentCam.m_lastFacingVector = camFacing;
        currentCam.m_facingVectorDeltaHisto.PushValue(facingDirFrameDelta);

        // increment frame count and timer
        ++currentCam.m_activeFrames;
        currentCam.m_activeTime += deltaTime;

        // Copy some of the info into the global Cam
        m_globalActiveCamInfo.m_lastWorldPos = currentCam.m_lastWorldPos;
        m_globalActiveCamInfo.m_lastFacingVector = currentCam.m_lastFacingVector;
        m_globalActiveCamInfo.m_fovHisto.PushValue(fov);
        m_globalActiveCamInfo.m_facingVectorDeltaHisto.PushValue(facingDirFrameDelta);
        m_globalActiveCamInfo.m_positionDeltaHisto.PushValue(positionFrameDelta);
        m_globalActiveCamInfo.m_activeFrames = currentCam.m_activeFrames;
        m_globalActiveCamInfo.m_activeTime = currentCam.m_activeTime;
    }

    float ImGuiLYCameraMonitor::GetAngleBetweenVectors(const AZ::Vector3& v1, const AZ::Vector3& v2)
    {
        float dot = v1.Dot(v2) / (v1.GetLength() * v2.GetLength());
        return !AZStd::isnan(dot) ? acosf(AZ::GetClamp(dot, -1.0f, 1.0f)) : 0.0f;
    }

    void ImGuiLYCameraMonitor::OnTick_GatherCameraData_PushNewCameraToHistory(AZ::EntityId newCamId)
    {
        // see if we are already at max history capacity, and if so, pop the back
        while (m_cameraHistory.size() >= m_camHistorySize - 1)
        {
            m_cameraHistory.pop_back();
        }
        // save this cam off as the current one
        m_currentCamera = newCamId;

        // create a new empty CameraInfo in the queue
        m_cameraHistory.push_front(CameraInfo());

        // init the new front of the queue
        CameraInfo& newCam = m_cameraHistory.front();
        newCam.m_camId = m_currentCamera;
        AZ::ComponentApplicationBus::BroadcastResult(newCam.m_camName, &AZ::ComponentApplicationBus::Events::GetEntityName, m_currentCamera);
        newCam.m_activeTime = 0.0f;
        newCam.m_activeFrames = 0;
        newCam.m_fovHisto.Init(                 "FOV",                      120, LYImGuiUtils::HistogramContainer::ViewType::Lines,     true, 50.0f, 50.0f);
        newCam.m_facingVectorDeltaHisto.Init(   "Facing Vec Frame Delta",   120, LYImGuiUtils::HistogramContainer::ViewType::Histogram, true, 0.0f,  0.0f);
        newCam.m_positionDeltaHisto.Init(       "Position Frame Delta",     120, LYImGuiUtils::HistogramContainer::ViewType::Histogram, true, 0.0f,  0.0f);

        // reset a few variables on the global camera info
        m_globalActiveCamInfo.m_camId = newCam.m_camId;
        m_globalActiveCamInfo.m_camName = newCam.m_camName;
        m_globalActiveCamInfo.m_activeFrames = 0;
        m_globalActiveCamInfo.m_activeTime = 0.0f;
    }
} // namespace ImGui

#endif // IMGUI_ENABLED
