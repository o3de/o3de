/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include "ImGuiManager.h"

#ifdef IMGUI_ENABLED

// A place for ImGui Color Define
#define ImGui_Col_Black         ImColor(0.000f, 0.000f, 0.000f) // 0xFF000000   RGB: 0, 0, 0
#define ImGui_Col_Blue          ImColor(0.000f, 0.000f, 1.000f) // 0xFF0000FF   RGB: 0, 0, 255
#define ImGui_Col_Green         ImColor(0.000f, 0.502f, 0.000f) // 0xFF008000   RGB: 0, 128, 0
#define ImGui_Col_GreenYellow   ImColor(0.678f, 1.000f, 0.184f) // 0xFFADFF2F   RGB: 173, 255, 47
#define ImGui_Col_LightBlue     ImColor(0.678f, 0.847f, 0.902f) // 0xFFADD8E6   RGB: 173, 216, 230
#define ImGui_Col_Red           ImColor(1.000f, 0.000f, 0.000f) // 0xFFFF0000   RGB: 255, 0, 0
#define ImGui_Col_Salmon        ImColor(0.980f, 0.502f, 0.447f) // 0xFFFA8072   RGB: 250, 128, 114
#define ImGui_Col_White         ImColor(1.000f, 1.000f, 1.000f) // 0xFFFFFFFF   RGB: 255, 255, 255
#define ImGui_Col_Gray          ImColor(0.500f, 0.500f, 0.500f) // 0xFF808080   RGB: 128, 128, 128
#define ImGui_Col_Yellow        ImColor(1.000f, 1.000f, 0.500f) // 0xFFFFFF80   RGB: 255, 255, 128
#define ImGui_Col_LightGreen    ImColor(0.500f, 1.000f, 0.500f) // 0xFF80FF80   RGB: 128, 255, 128
#define ImGui_Col_Cyan          ImColor(0.500f, 1.000f, 1.000f) // 0xFFFF8080   RGB: 128, 255, 255
#define ImGui_Col_DarkBlue      ImColor(0.500f, 0.500f, 1.000f) // 0xFF8080FF   RGB: 128, 128, 255

namespace ImGui
{
    namespace Colors
    {
        static const ImVec4 s_NiceLabelColor = ImColor(1.0f, 0.47f, 0.12f, 1.0f);
        static const ImVec4 s_PlainLabelColor = ImColor(1.0f, 1.0f, 1.0f, 0.56f);
    }
}
#endif // ~IMGUI_ENABLED
