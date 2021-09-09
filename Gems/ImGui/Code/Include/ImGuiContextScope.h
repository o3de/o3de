/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

struct ImGuiContext;

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace ImGui
{
    ////////////////////////////////////////////////////////////////////////////////////////////////
    //! Utility class to set a new current ImGui context on construction,
    //! then reset the previous ImGui context on destruction.
    class ImGuiContextScope
    {
    public:
        ////////////////////////////////////////////////////////////////////////////////////////////
        explicit ImGuiContextScope([[maybe_unused]] ImGuiContext* newContext)
            : m_previousContext(nullptr)
        {
#if defined(IMGUI_ENABLED)
            m_previousContext = ImGui::GetCurrentContext();
            ImGui::SetCurrentContext(newContext);
#endif
        }

        ////////////////////////////////////////////////////////////////////////////////////////////
        ~ImGuiContextScope()
        {
#if defined(IMGUI_ENABLED)
            ImGui::SetCurrentContext(m_previousContext);
#endif
        }

    private:
        ////////////////////////////////////////////////////////////////////////////////////////////
        [[maybe_unused]] ImGuiContext* m_previousContext = nullptr;
    };
}
