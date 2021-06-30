/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#ifdef IMGUI_ENABLED
#include "imgui/imgui.h"

#include <IConsole.h>

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
