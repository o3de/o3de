/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once
#include "ImGuiManager.h"

#ifdef IMGUI_ENABLED
#include "ImGuiLYCurveEditorBus.h"
#include <AzCore/std/containers/map.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace ImGui
{
    class ImGuiCurveEditor
        : public ImGuiCurveEditorRequestBus::Handler
    {
        ImGuiCurveEditor();
        ~ImGuiCurveEditor();

        void CreateCurveEditor(const char* label, float* points, int num_points, int max_points);

    };
}
#endif // IMGUI_ENABLED
