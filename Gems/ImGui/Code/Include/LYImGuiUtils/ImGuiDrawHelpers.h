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

#ifdef IMGUI_ENABLED
#include "imgui/imgui.h"

namespace ImGui
{
    namespace LYImGuiUtils
    {
        void Draw2DExpCurve(const char* name, const char* id, const ImVec2& size, const float exp, const float val = -1.0f);
        void DrawLYCVarCheckbox(const char* cVarName, const char* title, ICVar* cVar);
    }
}

// Define for DrawLYCVarCheckbox, encapsulates lifetime management of static CVAR pointer
#define IMGUI_DRAW_CVAR_CHECKBOX(cVarName, cVarTitle)                                   \
{                                                                                       \
    if (gEnv && gEnv->pConsole)                                                         \
    {                                                                                   \
        static ICVar* staticCVAR = gEnv->pConsole->GetCVar(cVarName);                   \
        ImGui::LYImGuiUtils::DrawLYCVarCheckbox(cVarName, cVarTitle, staticCVAR);       \
    }                                                                                   \
}
#endif // #ifdef IMGUI_ENABLED
