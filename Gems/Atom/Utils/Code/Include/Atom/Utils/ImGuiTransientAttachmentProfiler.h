/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
            bool Draw(const AZStd::unordered_map<int, RHI::TransientAttachmentStatistics>& statistics);

        private:
            // Draw the stats for one heap.
            void DrawHeap(
                const AZ::RHI::TransientAttachmentStatistics::Heap& heapStats,
                const AZStd::vector<AZ::RHI::TransientAttachmentStatistics::Scope>& scopes);
        };
    } // namespace Render
}  // namespace AZ

#include "ImGuiTransientAttachmentProfiler.inl"
