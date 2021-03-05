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
        AZ_RTTI(ImGuiModule, "{ECA9F41C-716E-4395-A096-5A519227F9A4}", CryHooksModule);

        void OnSystemEvent(ESystemEvent event, UINT_PTR wparam, UINT_PTR lparam) override;

    private:
        #ifdef IMGUI_ENABLED
        ImGuiLYCommonMenu lyCommonMenu;
        ImGuiManager manager;
        #endif
    };
}
