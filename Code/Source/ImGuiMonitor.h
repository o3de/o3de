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
#include <LYImGuiUtils/HistogramContainer.h>

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

        void SetFeatureMatrixMemoryUsage(size_t sizeInBytes) override { m_featureMatrixMemoryUsageInBytes = sizeInBytes; }
        void SetFeatureMatrixNumFrames(size_t numFrames) override { m_featureMatrixNumFrames = numFrames; }
        void SetFeatureMatrixNumComponents(size_t numFeatureComponents) override { m_featureMatrixNumComponents = numFeatureComponents; }

        void SetKdTreeMemoryUsage(size_t sizeInBytes) override { m_kdTreeMemoryUsageInBytes = sizeInBytes; }
        void SetKdTreeNumNodes(size_t numNodes) override { m_kdTreeNumNodes = numNodes; }
        void SetKdTreeNumDimensions(size_t numDimensions) override { m_kdTreeNumDimensions = numDimensions; }

    private:
        //! Named and sub-divided group containing several histograms.
        struct HistogramGroup
        {
            void OnImGuiUpdate();
            void PushHistogramValue(const char* valueName, float value, const AZ::Color& color);

            bool m_show = true;
            AZStd::string m_name;
            using HistogramIndexByNames = AZStd::unordered_map<const char*, size_t>;
            HistogramIndexByNames m_histogramIndexByName;
            AZStd::vector<ImGui::LYImGuiUtils::HistogramContainer> m_histograms;
            int m_histogramContainerCount = 500;

            static constexpr float s_histogramHeight = 95.0f;
        };

        HistogramGroup m_performanceStats;
        HistogramGroup m_featureCosts;

        size_t m_featureMatrixMemoryUsageInBytes = 0;
        size_t m_featureMatrixNumFrames = 0;
        size_t m_featureMatrixNumComponents = 0;

        size_t m_kdTreeMemoryUsageInBytes = 0;
        size_t m_kdTreeNumNodes = 0;
        size_t m_kdTreeNumDimensions = 0;
    };
} // namespace EMotionFX::MotionMatching

#endif // IMGUI_ENABLED
