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

#include <Atom/RHI.Reflect/TransientAttachmentStatistics.h>

namespace AZ
{
    namespace Render
    {
        //! Visual profiler for Transient Attachments.
        //! It uses ImGui as the library for displaying the Attachments and Heaps.
        //! It shows all heaps that are being used by the RHI and how the
        //! resources are allocated in each heap.
        class ImGuiTransientAttachmentProfiler
        {
        public:
            ImGuiTransientAttachmentProfiler() = default;
            ~ImGuiTransientAttachmentProfiler() = default;

            //! Draws the stats for the provided transient attachments.
            bool Draw(const AZ::RHI::TransientAttachmentStatistics& stats);

        private:
            // Draw the stats for one heap.
            void DrawHeap(
                const AZ::RHI::TransientAttachmentStatistics::Heap& heapStats,
                const AZStd::vector<AZ::RHI::TransientAttachmentStatistics::Scope>& scopes);
        };
    } // namespace Render
}  // namespace AZ

#include "ImGuiTransientAttachmentProfiler.inl"
