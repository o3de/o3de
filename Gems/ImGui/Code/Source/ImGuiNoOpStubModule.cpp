/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

namespace ImGui
{
    /*!
     * The ImGui::Module class coordinates with the application
     * to reflect classes and create system components.
     */
    class ImGuiModule : public AZ::Module
    {
    public:
        AZ_RTTI(ImGuiModule, "{ECA9F41C-716E-4395-A096-5A519227F9A4}", AZ::Module);
        ImGuiModule() : AZ::Module() {}  
    };
}

#if defined(O3DE_GEM_NAME)
AZ_DECLARE_MODULE_CLASS(AZ_JOIN(Gem_, O3DE_GEM_NAME), ImGui::ImGuiModule)
#else
AZ_DECLARE_MODULE_CLASS(Gem_ImGui, ImGui::ImGuiModule)
#endif
