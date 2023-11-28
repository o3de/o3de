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
    class ImGuiEditorWindowModule
        : public ImGuiModule
    {
    public:
        AZ_RTTI(ImGuiEditorWindowModule, "{DDC7A763-A36F-46D8-9885-43E0293C1D03}", ImGuiModule);

        ImGuiEditorWindowModule()
            : ImGuiModule()
        {
        }
    };
}

#if defined(O3DE_GEM_NAME)
AZ_DECLARE_MODULE_CLASS(AZ_JOIN(Gem_, O3DE_GEM_NAME), ImGui::ImGuiEditorWindowModule)
#else
AZ_DECLARE_MODULE_CLASS(Gem_ImGuiEditorWindow, ImGui::ImGuiEditorWindowModule)
#endif
