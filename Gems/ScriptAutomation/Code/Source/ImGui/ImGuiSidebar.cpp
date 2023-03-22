/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/Feature/ImGui/SystemBus.h>

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/Utils.h>
#include <AzCore/IO/SystemFile.h>
#include <AzCore/IO/FileIO.h>

#include <ImGui/ImGuiSidebar.h>
#include <ScriptableImGui.h>

namespace ScriptAutomation
{
    const float ImGuiSidebar::s_widthMin = 200.0f;
    const float ImGuiSidebar::s_widthMax = 1000.0f;
    const float ImGuiSidebar::s_widthStepSmall = 25.0f;
    const float ImGuiSidebar::s_widthStepBig = 100.0f;

    void ImGuiSidebar::Reflect(AZ::ReflectContext* context)
    {
        ImGuiSidebar::ConfigFile::Reflect(context);
    }

    void ImGuiSidebar::ConfigFile::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<ConfigFile>()
                ->Version(0)
                ->Field("Width", &ConfigFile::m_width)
                ;
        }
    }

    ImGuiSidebar::ImGuiSidebar(AZStd::string_view configFilePath)
    {
        char configFileFullPath[AZ_MAX_PATH_LEN] = {0};
        AZ::IO::FileIOBase::GetInstance()->ResolvePath(configFilePath.data(), configFileFullPath, AZ_MAX_PATH_LEN);

        m_configFilePath = configFileFullPath;
    }

    bool ImGuiSidebar::LoadConfigFile()
    {
        // skip loading config file if the config file is not specified.
        if (m_configFilePath.empty())
        {
            return false;
        }

        AZStd::unique_ptr<ConfigFile> configFile(AZ::Utils::LoadObjectFromFile<ConfigFile>(m_configFilePath));

        if (configFile)
        {
            m_configFile = *configFile;
            return true;
        }
        else
        {
            return false;
        }
    }

    void ImGuiSidebar::SaveConfigFile()
    {
        if (!m_configFilePath.empty())
        {        
            if (!AZ::Utils::SaveObjectToFile(m_configFilePath, AZ::DataStream::ST_XML, &m_configFile))
            {
                AZ_Error("ImGuiSidebar", false, "Failed to save '%s'", m_configFilePath.c_str());
            }
        }
    }

    void ImGuiSidebar::Activate()
    {
        // only load the config file if it's specified
        if ( (!m_configFilePath.empty()) && AZ::IO::SystemFile::Exists(m_configFilePath.c_str()) )
        {
            if (!LoadConfigFile())
            {
                AZ_Warning("ImGuiSidebar", false, "Failed to load sidebar config from %s.", m_configFilePath.c_str());
            }
        }
    }

    void ImGuiSidebar::Deactivate()
    {
        if (!m_configFilePath.empty())
        {
            // We only report this message in Deactivate(), not inside SaveConfigFile(), to avoid spamming
            // this message when SaveConfigFile() is called in OnTick()
            AZ_TracePrintf("ImGuiSidebar", "Saving settings to '%s'\n", m_configFilePath.c_str());

            SaveConfigFile();
        }
    }

    void ImGuiSidebar::SetHideSidebar(bool isHidden)
    {
        m_hideSidebar = isHidden;
    }

    AzFramework::WindowSize ImGuiSidebar::BeginFrame()
    {
        AzFramework::NativeWindowHandle windowHandle = nullptr;
        AzFramework::WindowSystemRequestBus::BroadcastResult(
            windowHandle,
            &AzFramework::WindowSystemRequestBus::Events::GetDefaultWindowHandle);

        AzFramework::WindowSize windowSize;
        AzFramework::WindowRequestBus::EventResult(
            windowSize,
            windowHandle,
            &AzFramework::WindowRequestBus::Events::GetClientAreaSize);

        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);

        return windowSize;
    }

    void ImGuiSidebar::EndFrame()
    {
        ImGui::PopStyleVar();
    }

    bool ImGuiSidebar::Begin()
    {
        AZ_Assert(!m_isSidebarReady, "End() was not called");

        AzFramework::WindowSize windowSize = BeginFrame();

        // Apply some offset so we don't collide with the menu bar
        const float menuBarOffset = 18.0f; 

        const float scale = ImGui::GetIO().FontGlobalScale;
        const float imguiWindowWidth = m_configFile.m_width * scale;
        const float imguiWindowHeight = windowSize.m_height - menuBarOffset;

        const float revealWindowWidth = 120.0f * scale;
        const float revealWindowHeight = 40.0f * scale;

        const ImGuiWindowFlags windowFlags =
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove;

        bool configChanged = false;

        // We can't append to the main menu so instead we have a 
        // smaller window that takes up less space
        if (m_hideSidebar)
        {
            ImGui::SetNextWindowPos(ImVec2(windowSize.m_width - revealWindowWidth, menuBarOffset));
            ImGui::SetNextWindowSize(ImVec2(revealWindowWidth, revealWindowHeight));

            if (ImGui::Begin("##RevealSidebar", nullptr, windowFlags))
            {
                if (ScriptableImGui::Button("Reveal Sidebar"))
                {
                    m_hideSidebar = false;
                }
                ImGui::End();
            }
        }
        else
        {
            ImGui::SetNextWindowPos(ImVec2(windowSize.m_width - imguiWindowWidth, menuBarOffset));
            ImGui::SetNextWindowSize(ImVec2(imguiWindowWidth, imguiWindowHeight));

            if (ImGui::Begin("##Sidebar", nullptr, windowFlags))
            {
                const float itemSpacing = ImGui::GetStyle().ItemSpacing.x;

                // Make the "Hide Sidebar" button show up in about the same position as the "Reveal Sidebar" button
                const float rightMargin = 16.0f;
                const float buttonWidth = revealWindowWidth - rightMargin;
                ImGui::SameLine(imguiWindowWidth - revealWindowWidth + itemSpacing);

                if (ScriptableImGui::Button("Hide Sidebar", ImVec2(buttonWidth, 0)))
                {
                    m_hideSidebar = true;
                }

                // Make the sidebar size buttons appear on the same line, aligned to the right, so they don't move.
                ImGui::NewLine();
                static float resizeButtonWidthSmall = 20.0f;
                static float resizeButtonWidthBig = 20.0f;
                float pos = resizeButtonWidthBig + itemSpacing;
                ImGui::SameLine(imguiWindowWidth - pos);

                if (ImGui::Button(" >> "))
                {
                    m_configFile.m_width -= s_widthStepBig;
                    m_configFile.m_width = AZStd::max(m_configFile.m_width, s_widthMin);
                    configChanged = true;
                }
                resizeButtonWidthBig = ImGui::GetItemRectSize().x; // Get the actual width for the next frame

                pos += resizeButtonWidthSmall + itemSpacing;
                ImGui::SameLine(imguiWindowWidth - pos);
                if (ImGui::Button(" > "))
                {
                    m_configFile.m_width -= s_widthStepSmall;
                    m_configFile.m_width = AZStd::max(m_configFile.m_width, s_widthMin);
                    configChanged = true;
                }
                resizeButtonWidthSmall = ImGui::GetItemRectSize().x; // Get the actual width for the next frame

                pos += resizeButtonWidthSmall + itemSpacing;
                ImGui::SameLine(imguiWindowWidth - pos);
                if (ImGui::Button(" < "))
                {
                    m_configFile.m_width += s_widthStepSmall;
                    m_configFile.m_width = AZStd::min(m_configFile.m_width, s_widthMax);
                    configChanged = true;
                }

                pos += resizeButtonWidthBig + itemSpacing;
                ImGui::SameLine(imguiWindowWidth - pos);
                if (ImGui::Button(" << "))
                {
                    m_configFile.m_width += s_widthStepBig;
                    m_configFile.m_width = AZStd::min(m_configFile.m_width, s_widthMax);
                    configChanged = true;
                }

                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();

                m_isSidebarReady = true;
            }
        }

        if (configChanged)
        {
            SaveConfigFile();
        }

        if (!m_isSidebarReady)
        {
            EndFrame();
        }
        
        return m_isSidebarReady;
    }

    void ImGuiSidebar::End()
    {
        AZ_Assert(m_isSidebarReady, "Begin() was not called, or it returned false");

        ImGui::End(); // Ends the ImGui::Begin("##Sidebar",...)

        m_isSidebarReady = false;

        EndFrame();
    }
} // namespace ScriptAutomation
