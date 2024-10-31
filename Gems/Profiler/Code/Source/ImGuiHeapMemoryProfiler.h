/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if defined(IMGUI_ENABLED)
#include <imgui/imgui.h>
#include <AzCore/Memory/AllocatorManager.h>

namespace Profiler
{
    //! Profiler for displaying information about Memory Heaps
    class ImGuiHeapMemoryProfiler
    {
    public:
        ImGuiHeapMemoryProfiler() = default;
        ~ImGuiHeapMemoryProfiler() = default;

        void Draw(bool& draw);

    private:
        ImGuiTextFilter m_filter;
    };
}
#endif
