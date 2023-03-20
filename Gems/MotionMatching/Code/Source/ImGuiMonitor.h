/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once
#ifdef IMGUI_ENABLED

#include <AzCore/Component/EntityId.h>
#include <AzCore/Memory/Memory.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/containers/unordered_map.h>

#include <EMotionFX/Source/EMotionFXConfig.h>

#include <imgui/imgui.h>
#include <ImGuiBus.h>
#include <ImGuiMonitorBus.h>
#include <LYImGuiUtils/HistogramGroup.h>

namespace EMotionFX::MotionMatching
{
    class EMFX_API ImGuiMonitor
        : public ImGui::ImGuiUpdateListenerBus::Handler
        , public ImGuiMonitorRequestBus::Handler
    {
    public:
        AZ_RTTI(ImGuiMonitor, "{BF1B85A4-215C-4E3A-8FD8-CE3233E5C779}")
        AZ_CLASS_ALLOCATOR_DECL

        ImGuiMonitor();
        ~ImGuiMonitor();

        // ImGui::ImGuiUpdateListenerBus::Handler
        void OnImGuiUpdate() override;
        void OnImGuiMainMenuUpdate() override;

        // ImGuiMonitorRequestBus::Handler
        void PushPerformanceHistogramValue(const char* performanceMetricName, float value) override;
        void PushCostHistogramValue(const char* costName, float value, const AZ::Color& color) override;

        void SetFrameDatabaseInfo(const ImGuiMonitorRequests::FrameDatabaseInfo& info) override { m_frameDatabaseInfo = info; }
        void SetFeatureMatrixInfo(const ImGuiMonitorRequests::FeatureMatrixInfo& info) override { m_featurMatrixInfo = info; }
        void SetKdTreeInfo(const ImGuiMonitorRequests::KdTreeInfo& info) override { m_kdTreeInfo = info; }

    private:
        ImGui::LYImGuiUtils::HistogramGroup m_performanceStats;
        ImGui::LYImGuiUtils::HistogramGroup m_featureCosts;

        ImGuiMonitorRequests::FrameDatabaseInfo m_frameDatabaseInfo;
        ImGuiMonitorRequests::FeatureMatrixInfo m_featurMatrixInfo;
        ImGuiMonitorRequests::KdTreeInfo m_kdTreeInfo;
    };
} // namespace EMotionFX::MotionMatching

#endif // IMGUI_ENABLED
