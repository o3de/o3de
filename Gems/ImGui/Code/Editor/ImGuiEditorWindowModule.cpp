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

// DO NOT MODIFY THIS LINE UNLESS YOU RENAME THE GEM
// The first parameter should be GemName_GemIdLower
// The second should be the fully qualified name of the class above
AZ_DECLARE_MODULE_CLASS(Gem_ImGuiEditorWindow, ImGui::ImGuiEditorWindowModule)
