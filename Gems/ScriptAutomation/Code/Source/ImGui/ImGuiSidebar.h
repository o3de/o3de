/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzFramework/Windowing/WindowBus.h>

struct ImGuiContext;

namespace ScriptAutomation
{
    class ImGuiSidebar
    {
    public:
        static void Reflect(AZ::ReflectContext* context);

        ImGuiSidebar() = default;

        /// @param configFilePath - Path to a local json file for maintaining state between runs. Should start with "@user@/"
        explicit ImGuiSidebar(AZStd::string_view configFilePath);

        void Activate();
        void Deactivate();

        void SetHideSidebar(bool isHidden);

        bool Begin();
        void End();

    private:

        struct ConfigFile final
        {
            AZ_TYPE_INFO(ImGuiSidebar::ConfigFile, "{305046DC-C0AC-4971-A900-75EA9AD0E4F4}");
            AZ_CLASS_ALLOCATOR(ConfigFile, AZ::SystemAllocator, 0);

            static void Reflect(AZ::ReflectContext* context);

            float m_width = 300.0f;
        };

        AzFramework::WindowSize BeginFrame();
        void EndFrame();

        bool LoadConfigFile();
        void SaveConfigFile();

        static const float s_widthMin;
        static const float s_widthMax;
        static const float s_widthStepSmall;
        static const float s_widthStepBig;

        bool m_hideSidebar = false;
        bool m_isSidebarReady = false;

        AZStd::string m_configFilePath;
        ConfigFile m_configFile;
    };
} // namespace ScriptAutomation
