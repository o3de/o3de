/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#ifdef IMGUI_ENABLED
#include <AzCore/std/containers/deque.h>
#include "imgui/imgui.h"
#include <ISystem.h>

namespace ImGui
{
    namespace LYImGuiUtils
    {
        /**
        * A small class to help manage values for an ImGui Histogram ( ImGui doesn't want to manage the values itself ).
        *   Nothing crazy, just helps reduce boiler plate if you are ImGui::PlotHistogram()'ing
        */
        class HistogramContainer
        {
        public:
            HistogramContainer() = default;
            ~HistogramContainer() = default;

            // An Enumeration of different ViewTypes for this histogram
            enum class ViewType : AZ::u8
            {
                FIRST = 0,

                Histogram = FIRST,
                Lines,

                COUNT
            };
            // Static Type to String function
            static const char* ViewTypeToString(ViewType viewType);

            // Do all of the set up via Init
            void Init(const char* histogramName, int maxValueCountSize, ViewType viewType, bool displayOverlays, float minScale, float maxScale
                , bool autoExpandScale, bool startCollapsed = false, bool drawMostRecentValue = true);

            // How many values are in the container currently
            int GetSize() { return static_cast<int>(m_values.size()); }

            // What is the max size of the container
            int GetMaxSize() { return m_maxSize; }

            // Set the Max Size and clear the container
            void SetMaxSize(int size) { m_values.clear();  m_maxSize = size; }

            // Push a value to this histogram container
            void PushValue(float val);

            // Get the last value pushed
            float GetLastValue() { return GetValue(static_cast<int>(m_values.size() - 1)); }

            // Get a Values at a particular index
            float GetValue(int index) { return index < m_values.size() ? m_values.at(index) : 0.0f; }

            // Draw this histogram with ImGui
            void Draw(float histogramWidth, float histogramHeight);

        private:

            AZStd::string m_histogramName;
            AZStd::deque<float> m_values;
            int m_maxSize = 60;
            ViewType m_viewType = ViewType::Histogram;
            float m_minScale;
            float m_maxScale;
            bool m_dispalyOverlays;
            bool m_autoExpandScale;
            bool m_collapsed;
            bool m_drawMostRecentValueText;
        };

        // Getter function lambda. Can be used directly with ImGui if the user would like to skip our cool Draw function and just use the class as a cache
        static auto s_histogramContainerGetter = [](void* histContainerPtr, int idx)
        {
            ImGui::LYImGuiUtils::HistogramContainer* histContainer = static_cast<ImGui::LYImGuiUtils::HistogramContainer*>(histContainerPtr);
            if (histContainer != nullptr)
            {
                return histContainer->GetValue(idx);
            }
            return 0.0f;
        };
    }
}
#endif // #ifdef IMGUI_ENABLED
