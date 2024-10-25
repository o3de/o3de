/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/RHI.Reflect/AllocatorManager.h>

namespace AZ::Render
{
    //! Profiler for displaying information about Atom Memory Heaps
    //! Must be run with argument -rhi-memory-profile=enable
    class ImGuiHeapProfiler
    {
    public:
        ImGuiHeapProfiler() = default;
        ~ImGuiHeapProfiler() = default;

        void Draw(bool& draw);
    };
}

#include "ImGuiHeapProfiler.inl"
