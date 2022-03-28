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
        * A small class to help manage values for an ImGui Histogram (ImGui is not managing values itself).
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

            //! Horizontal move direction of the histogram when pushing new values.
            enum MoveDirection : AZ::u8
            {
                PushLeftMoveRight = 0, //! Push new values to the front of the buffer, which corresponds to the left side, and make the histogram move to the right.
                PushRightMoveLeft = 1, //! Push new values to the back of the buffer, which corresponds to the right side, and make the histogram move to the left.
            };

            //! Mode determining the min and max values for the visible range of the vertical axis for the histogram.
            enum ScaleMode : AZ::u8
            {
                NoAutoScale = 0, //! Use the min and max values given by Init() as visible range.
                AutoExpand = 1, //! Expand scale in case a sample is out of the current bounds. Does only expand the scale but not decrease it back again.
                AutoScale = 2, //! Use a running average to expand and shrink the visible range.
            };

            // Do all of the set up via Init
            void Init(const char* histogramName, int maxValueCountSize, ViewType viewType, bool displayOverlays, float minScale, float maxScale,
                ScaleMode scaleMode = AutoScale, bool startCollapsed = false, bool drawMostRecentValue = true);

            // How many values are in the container currently
            int GetSize() { return static_cast<int>(m_values.size()); }

            // What is the max size of the container
            int GetMaxSize() { return m_maxSize; }

            // Push a value to this histogram container
            void PushValue(float val);

            // Get the last value pushed
            float GetLastValue() { return GetValue(static_cast<int>(m_values.size() - 1)); }

            // Get a Values at a particular index
            float GetValue(int index) { return index < m_values.size() ? m_values.at(index) : 0.0f; }

            // Draw this histogram with ImGui
            void Draw(float histogramWidth, float histogramHeight);

            //! Adjust the scale mode to determine the min and max values for the visible range of the vertical axis for the histogram.
            void SetScaleMode(ScaleMode scaleMode) { m_scaleMode = scaleMode; }

            //! Adjust the horizontal move direction of the histogram when pushing new values.
            void SetMoveDirection(MoveDirection moveDirection) { m_moveDirection = moveDirection; }

            //! Calculate the min and maximum values for the present samples.
            void CalcMinMaxValues(float& outMin, float& outMax);

            //! Set/get color used by either the lines in case ViewType is Lines or bars in case or Histogram.
            void SetBarLineColor(const ImColor& color) { m_barLineColor = color; }
            ImColor GetBarLineColor() const  { return m_barLineColor; }

        private:
            // Set the Max Size and clear the container
            void SetMaxSize(int size);

            AZStd::string m_histogramName;
            AZStd::deque<float> m_values;
            int m_maxSize = 60;
            ViewType m_viewType = ViewType::Histogram;
            float m_minScale;
            float m_maxScale;
            MoveDirection m_moveDirection = PushLeftMoveRight; //! Specify if values will be added on the left and the histogram moves right or the other way around.
            bool m_dispalyOverlays;
            ScaleMode m_scaleMode; //! Determines if the vertical range of the histogram will be manually specified, auto-expanded or automatically scaled based on the samples.
            float m_autoScaleSpeed = 0.05f; //! Indicates how fast the min max values and the visible vertical range are adapting to new samples.
            ImColor m_barLineColor = ImColor(66, 166, 178); //! Color used by either the lines in case ViewType is Lines or bars in case or Histogram.
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
