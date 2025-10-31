/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <IGem.h>
#include "ImGuiManager.h"
//#include "Private/ImGuiManagerImpl.h"
#ifdef IMGUI_ENABLED
#include "LYCommonMenu/ImGuiLYCommonMenu.h"
#endif //IMGUI_ENABLED

namespace ImGui
{
    /*!
    * The ImGui::Module class coordinates with the application
    * to reflect classes and create system components.
    */
    class ImGuiModule : public CryHooksModule
    {
    public:
        AZ_CLASS_ALLOCATOR(ImGuiModule, AZ::SystemAllocator)
        AZ_RTTI(ImGuiModule, "{ECA9F41C-716E-4395-A096-5A519227F9A4}", CryHooksModule);

        void OnSystemEvent(ESystemEvent event, UINT_PTR wparam, UINT_PTR lparam) override;

    private:
        #ifdef IMGUI_ENABLED
        ImGuiLYCommonMenu lyCommonMenu;
        ImGuiManager manager;
        #endif
    };
}
