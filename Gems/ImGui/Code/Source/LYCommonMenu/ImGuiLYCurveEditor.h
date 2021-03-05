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