/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "ImGuiGem.h"

namespace ImGui
{
    void ImGuiModule::OnSystemEvent([[maybe_unused]] ESystemEvent event, [[maybe_unused]] UINT_PTR wparam, [[maybe_unused]] UINT_PTR lparam)
    {
#ifdef IMGUI_ENABLED
        switch (event)
        {
            case ESYSTEM_EVENT_GAME_POST_INIT:
            {
                manager.Initialize();
                lyCommonMenu.Initialize();
                break;
            }
            case ESYSTEM_EVENT_FULL_SHUTDOWN:
            case ESYSTEM_EVENT_FAST_SHUTDOWN:
                manager.Shutdown();
                lyCommonMenu.Shutdown();
                break;
            case ESYSTEM_EVENT_GAME_POST_INIT_DONE:
                // Register CVARS after Init is done
                manager.RegisterImGuiCVARs();
                break;
        }
#endif //IMGUI_ENABLED
    }
}

#if !defined(IMGUI_GEM_EDITOR)
#if defined(O3DE_GEM_NAME)
AZ_DECLARE_MODULE_CLASS(AZ_JOIN(Gem_, O3DE_GEM_NAME), ImGui::ImGuiModule)
#else
AZ_DECLARE_MODULE_CLASS(Gem_ImGui, ImGui::ImGuiModule)
#endif
#endif // IMGUI_GEM_EDITOR
