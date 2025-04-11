/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "ImGuiLYCommonMenu.h"

#ifdef IMGUI_ENABLED
#include <AzCore/std/string/conversions.h>
#include <AzCore/std/sort.h>
#include <AzCore/StringFunc/StringFunc.h>
#include <AzCore/Console/IConsole.h>
#include <AzFramework/Input/Buses/Requests/InputSystemCursorRequestBus.h>
#include <AzFramework/Input/Devices/Mouse/InputDeviceMouse.h>
#include <AzFramework/Spawnable/Spawnable.h>
#include <AzFramework/Viewport/ViewportBus.h>
#include <imgui/imgui_internal.h>
#include <ILevelSystem.h>
#include "ImGuiColorDefines.h"
#include "LYImGuiUtils/ImGuiDrawHelpers.h"

// individual menus
#include "ImGuiLYAssetExplorer.h"
#include "ImGuiLYCameraMonitor.h"
#include "ImGuiLYEntityOutliner.h"

namespace ImGui
{
    // Resolution Widths to recommend for usage for both O3DE Rendering and/or ImGui Rendering
    static int s_renderResolutionWidths[7] = { 800, 1280, 1600, 1920, 2560, 3440, 3840 };
    static int s_renderAspectRatios[4][2] = { {16,9}, {16,10}, {43,18}, {4,3} };
    static const char* s_toggleTelemetryConsoleCmd = "radtm_ToggleEnabled 1";

    ImGuiLYCommonMenu::ImGuiLYCommonMenu()
        : m_telemetryCaptureTime(8.0f)
        , m_telemetryCaptureTimeRemaining(-1.0f)
        , m_controllerLegendWindowVisible(false)
    {
    }
    ImGuiLYCommonMenu::~ImGuiLYCommonMenu() 
    {
    }

    void ImGuiLYCommonMenu::Initialize()
    {
        // Connect EBusses
        ImGuiUpdateListenerBus::Handler::BusConnect();

        // init sub menu objects
        m_assetExplorer.Initialize();
        m_cameraMonitor.Initialize();
        m_entityOutliner.Initialize();
        m_inputMonitor.Initialize();

        m_deltaTimeHistogram.Init("onTick Delta Time (Milliseconds)", 250, LYImGuiUtils::HistogramContainer::ViewType::Histogram, true, 0.0f, 60.0f);
        AZ::TickBus::Handler::BusConnect();
    }

    void ImGuiLYCommonMenu::Shutdown()
    {
        // Disconnect EBusses
        AZ::TickBus::Handler::BusDisconnect();
        ImGuiUpdateListenerBus::Handler::BusDisconnect();

        // shutdown sub menu objects
        m_inputMonitor.Shutdown();
        m_assetExplorer.Shutdown();
        m_cameraMonitor.Shutdown();
        m_entityOutliner.Shutdown();
    }

    static auto SystemCursorStateComboGetter = []([[maybe_unused]] void* contextPtr, int idx, const char** out_text)
    {
        AzFramework::SystemCursorState cursorState = static_cast<AzFramework::SystemCursorState>(idx);
        switch (cursorState)
        {
            default:
                break;

            case AzFramework::SystemCursorState::Unknown:
                *out_text = "*unknown*";
                return true;

            case AzFramework::SystemCursorState::ConstrainedAndHidden:
                *out_text = "ConstrainedAndHidden";
                return true;

            case AzFramework::SystemCursorState::ConstrainedAndVisible:
                *out_text = "ConstrainedAndVisible";
                return true;

            case AzFramework::SystemCursorState::UnconstrainedAndHidden:
                *out_text = "UnconstrainedAndHidden";
                return true;

            case AzFramework::SystemCursorState::UnconstrainedAndVisible:
                *out_text = "UnconstrainedAndVisible";
                return true;

        }

        *out_text = "*error_unimplemented*";
        return false;
    };

    void ImGuiLYCommonMenu::OnImGuiUpdate()
    {
        float dpiScalingFactor = 1.0f;
        ImGuiManagerBus::BroadcastResult(dpiScalingFactor, &ImGuiManagerBus::Events::GetDpiScalingFactor);

        // Utility function to calculate the size in device pixels based on the current DPI
        const auto dpiAwareSizeFn = [dpiScalingFactor](float size)
        {
            return dpiScalingFactor * size;
        };

        AZStd::optional<AzFramework::ViewportBorderPadding> viewportBorderPaddingOpt;
        AzFramework::ViewportBorderRequestBus::BroadcastResult(
            viewportBorderPaddingOpt, &AzFramework::ViewportBorderRequestBus::Events::GetViewportBorderPadding);

        AzFramework::ViewportBorderPadding viewportBorderPadding = viewportBorderPaddingOpt.value_or(AzFramework::ViewportBorderPadding{});

        auto ctx = ImGui::GetCurrentContext();

        // determine if the window is the drop down and if it is active
        const auto windowIt = AZStd::find_if(
            ctx->Windows.begin(),
            ctx->Windows.end(),
            [](const ImGuiWindow* window)
            {
                // there is a drop down if the BeginOrder is 2 (1 below main menu),
                // drop down items are called "##Menu_00"
                return window->BeginOrderWithinContext == 2 && strcmp(window->Name, "##Menu_00") == 0 && window->WasActive;
            });

        if (windowIt != ctx->Windows.end())
        {
            m_markedForHiding = false;
            // this conditional stops the notification from repeatedly broadcasting
            if (m_dropdownState != ImGuiDropdownState::Shown)
            {
                m_dropdownState = ImGuiDropdownState::Shown;
                AzFramework::ViewportImGuiNotificationBus::Broadcast(
                    &AzFramework::ViewportImGuiNotificationBus::Events::OnImGuiDropDownShown);
            }
        }
        else
        {
            // if it has already been marked that it is hidden, notify that it has done so
            if (m_dropdownState != ImGuiDropdownState::Hidden && m_markedForHiding)
            {
                m_dropdownState = ImGuiDropdownState::Hidden;
                AzFramework::ViewportImGuiNotificationBus::Broadcast(
                    &AzFramework::ViewportImGuiNotificationBus::Events::OnImGuiDropDownHidden);
                m_markedForHiding = false;
            }
            else
            {
                // when the dropdown switches between options it counts as hidden, but it isn't known if it is
                // actually hidden or just switching, this acts as an intermediary for the next update
                // in the above conditional
                m_markedForHiding = true;
            }
        }
        
        // Utility function to return the current offset (scaled by DPI) if a viewport border
        // is active (otherwise 0.0)
        auto dpiAwareBorderOffsetFn = [&viewportBorderPaddingOpt, &dpiAwareSizeFn](float size)
        {
            return viewportBorderPaddingOpt.has_value() ? dpiAwareSizeFn(size) : 0.0f;
        };

        // Shift the menu down if a viewport border is active
        ImVec2 cachedSafeArea = ImGui::GetStyle().DisplaySafeAreaPadding;
        ImGui::GetStyle().DisplaySafeAreaPadding = ImVec2(cachedSafeArea.x, cachedSafeArea.y + dpiAwareSizeFn(viewportBorderPadding.m_top));

        if (ImGui::BeginMainMenuBar())
        {
            // Constant to shift right aligned menu items by (distance to the left) when a viewport border is active
            const float rightAlignedBorderOffset = dpiAwareBorderOffsetFn(36.0f);

            // Get Discrete Input state now, we will use it both inside the ImGui SubMenu, and along the main task bar ( when it is on )
            bool discreteInputEnabled = false;
            ImGuiManagerBus::BroadcastResult(discreteInputEnabled, &IImGuiManager::GetEnableDiscreteInputMode);

            // Input Mode Display
            {
                const float prevCursorPos = ImGui::GetCursorPosX();
                ImGui::SetCursorPosX(
                    ImGui::GetWindowWidth() - dpiAwareSizeFn(300.0f + viewportBorderPadding.m_right) - rightAlignedBorderOffset);

                AZStd::string inputTitle = "Input: ";
                if (!discreteInputEnabled)
                {
                    inputTitle.append("ImGui & Game");
                }
                else
                {
                    // Discrete Input - Control ImGui and Game independently.
                    ImGui::DisplayState state;
                    ImGui::ImGuiManagerBus::BroadcastResult(state, &ImGui::IImGuiManager::GetDisplayState);
                    if (state == DisplayState::Visible)
                    {
                        inputTitle.append("ImGui");
                    }
                    else
                    {
                        inputTitle.append("Game");
                    }
                }

                if (ImGui::BeginMenu(inputTitle.c_str()))
                {
                    ImGui::TextColored(ImGui::Colors::s_PlainLabelColor, "Discrete Input Mode. Currently Enabled:");
                    ImGui::SameLine();
                    ImGui::TextColored(ImGui::Colors::s_NiceLabelColor, discreteInputEnabled ? "True" : "False");
                    ImGui::TextColored(ImGui::Colors::s_PlainLabelColor, " * Discrete Input mode ON: All input goes to both ImGui and the Game, all the time.");
                    ImGui::TextColored(ImGui::Colors::s_PlainLabelColor, " * Discrete Input mode OFF: ImGui has three states 1)ImGui On, Input->ImGui, 2)ImGui On, Input->Game 3) ImGui Off");
                    ImGui::TextColored(ImGui::Colors::s_PlainLabelColor, " * Hot Tip: use the LY Common -> ImGui Menu to toggle on and off discrete input mode, or the CVAR: 'imgui_DiscreteInputMode'");
                    ImGui::Separator();
                    ImGui::Text("Controller Legend ");
                    ImGui::SameLine();
                    if (ImGui::SmallButton("Window"))
                    {
                        m_controllerLegendWindowVisible = true;
                    }
                    ImGui::SameLine();
                    if (ImGui::BeginMenu("Peek"))
                    {
                        OnImGuiUpdate_DrawControllerLegend();
                        ImGui::EndMenu();
                    }

                    ImGui::EndMenu(); // inputTitle
                }

                ImGui::SetCursorPosX(prevCursorPos);
            }

            // Add some space before the first menu so it won't overlap with view control buttons
            ImGui::SetCursorPosX(dpiAwareSizeFn(40.0f + viewportBorderPadding.m_left));

            // Main Open 3D Engine menu
            if (ImGui::BeginMenu("O3DE"))
            {
                AZ::IConsole* console = AZ::Interface<AZ::IConsole>::Get();
                if (console != nullptr)
                {
                    // Get the current console visibility status and cache it
                    bool showConsole = true;
                    console->GetCvarValue("bg_showDebugConsole", showConsole);
                    const bool originalValue = showConsole;

                    ImGui::Checkbox("Console", &showConsole);
                    if (originalValue != showConsole)
                    {
                        // If the visibility state changed then update the console variable
                        // Guard for edges so we don't continuously spam the console with commands
                        console->PerformCommand(showConsole ? "bg_showDebugConsole true" : "bg_showDebugConsole false");
                    }
                }

                if (ImGui::MenuItem("Delta Time Graph"))
                {
                    m_showDeltaTimeGraphs = !m_showDeltaTimeGraphs;
                }
                // Asset Explorer
                if (ImGui::MenuItem("Asset Explorer"))
                {
                    m_assetExplorer.ToggleEnabled();
                }

                // Camera Monitor
                if (ImGui::MenuItem("Camera Monitor"))
                {
                    m_cameraMonitor.ToggleEnabled();
                }

                if (ImGui::MenuItem("Input Monitor"))
                {
                    m_inputMonitor.ToggleEnabled();
                }

                // LY Entity Outliner
                if (ImGui::SmallButton("Launch"))
                {
                    m_entityOutliner.ToggleEnabled();
                }
                ImGui::SameLine();
                if (ImGui::BeginMenu("Entity Outliner"))
                {
                    m_entityOutliner.ImGuiUpdate_DrawComponentViewSubMenu();
                    ImGui::EndMenu();
                }

                // Display Options
                if (ImGui::BeginMenu("Display Info"))
                {
                    // Display Info
                    static ICVar* rDisplayInfoCVar = gEnv->pConsole->GetCVar("r_DisplayInfo");
                    if (rDisplayInfoCVar)
                    {
                        int displayInfoVal = rDisplayInfoCVar->GetIVal();
                        int dragIntVal = displayInfoVal;
                        ImGui::Text("r_DisplayInfo: %d ( View Runtime LY Debug Stats)", displayInfoVal);
                        ImGui::SliderInt("##DisplayInfo", &dragIntVal, 0, 5);

                        if (dragIntVal != displayInfoVal)
                        {
                            rDisplayInfoCVar->Set(dragIntVal);
                        }
                    }

                    // Texel Density
                    static ICVar* eTexelDensityCVAR = gEnv->pConsole->GetCVar("e_texeldensity");
                    if (eTexelDensityCVAR)
                    {
                        int texelDensityValue = eTexelDensityCVAR->GetIVal();
                        int dragIntVal = texelDensityValue;
                        ImGui::Text("e_texeldensity: %d ( Used for Misc. LOD/MipMap debugging )", texelDensityValue);
                        ImGui::SliderInt("##texelDensity", &dragIntVal, 0, 2);
                        if (dragIntVal != texelDensityValue)
                        {
                            eTexelDensityCVAR->Set(dragIntVal);
                        }
                    }

                    ImGui::EndMenu();
                }

                // View Maps ( pending valid ILevelSystem )
                auto lvlSystem = (gEnv && gEnv->pSystem) ? gEnv->pSystem->GetILevelSystem() : nullptr;
                if (lvlSystem && AzFramework::LevelSystemLifecycleInterface::Get() && ImGui::BeginMenu("Levels"))
                {
                    if (AzFramework::LevelSystemLifecycleInterface::Get()->IsLevelLoaded())
                    {
                        ImGui::TextColored(ImGui::Colors::s_PlainLabelColor, "Current Level: ");
                        ImGui::SameLine();
                        ImGui::TextColored(
                            ImGui::Colors::s_NiceLabelColor,
                            "%s",
                            AzFramework::LevelSystemLifecycleInterface::Get()->GetCurrentLevelName());
                    }

                    // Run through all the assets in the asset catalog and gather up the list of level assets

                    AZ::Data::AssetType levelAssetType = lvlSystem->GetLevelAssetType();
                    AZStd::vector<AZStd::string> levelNames;
                    AZStd::set<AZStd::string> networkedLevelNames;
                    auto enumerateCB =
                        [levelAssetType, &levelNames, &networkedLevelNames]([[maybe_unused]] const AZ::Data::AssetId id, const AZ::Data::AssetInfo& assetInfo)
                    {
                        if (assetInfo.m_assetType == levelAssetType)
                        {
                            // A network spawnable is serialized to file as a ".network.spawnable". (See Multiplayer Gem's MultiplayerConstants.h)
                            // Filter out network spawnables from the level list, 
                            // but keep track of which levels require networking so they can be recognized in the level selection menu. 
                            constexpr AZStd::fixed_string<32> networkSpawnablePrefix(".network");
                            constexpr AZStd::fixed_string<32> networkSpawnableFileExtension = networkSpawnablePrefix + AzFramework::Spawnable::DotFileExtension;

                            if (assetInfo.m_relativePath.ends_with(networkSpawnableFileExtension))
                            {   
                                AZStd::string spawnablePath(assetInfo.m_relativePath); 
                                AZ::StringFunc::Replace(spawnablePath, networkSpawnablePrefix.c_str(), "");
                                networkedLevelNames.emplace(spawnablePath);
                            }
                            else
                            {
                                levelNames.emplace_back(assetInfo.m_relativePath);
                            }
                        }
                    };

                    AZ::Data::AssetCatalogRequestBus::Broadcast(
                        &AZ::Data::AssetCatalogRequestBus::Events::EnumerateAssets, nullptr, enumerateCB, nullptr);

                    AZStd::sort(levelNames.begin(), levelNames.end());

                    // Create a menu item for each level asset, with an action to load it if selected.

                    ImGui::TextColored(ImGui::Colors::s_PlainLabelColor, "Load Level: ");
                    for (int i = 0; i < levelNames.size(); i++)
                    {
                        bool isNetworked = networkedLevelNames.contains(levelNames[i]);
                        if (ImGui::MenuItem(AZStd::string::format("%d- %s%s", i, levelNames[i].c_str(), isNetworked ? " (Multiplayer)":"").c_str()))
                        {
                            AZ::TickBus::QueueFunction(
                                [lvlSystem, levelNames, i]()
                                {
                                    lvlSystem->LoadLevel(levelNames[i].c_str());
                                });
                        }
                    }

                    ImGui::EndMenu();
                }

                if (ImGui::BeginMenu("Mouse/Cursor"))
                {
                    AzFramework::SystemCursorState currentCursorState;
                    AzFramework::InputSystemCursorRequestBus::EventResult(currentCursorState, AzFramework::InputDeviceMouse::Id, &AzFramework::InputSystemCursorRequests::GetSystemCursorState);
                    int comboCursorState = static_cast<int>(currentCursorState);
                    ImGui::Combo("System Cursor State", &comboCursorState, SystemCursorStateComboGetter, this, static_cast<int>(AzFramework::SystemCursorState::UnconstrainedAndVisible) + 1);
                    AzFramework::SystemCursorState postComboCursorState = static_cast<AzFramework::SystemCursorState>(comboCursorState);

                    if (postComboCursorState != currentCursorState)
                    {
                        AzFramework::InputSystemCursorRequestBus::Event(AzFramework::InputDeviceMouse::Id, &AzFramework::InputSystemCursorRequests::SetSystemCursorState, postComboCursorState);
                    }

                    ImGui::EndMenu();
                }

                if (ImGui::BeginMenu("Telemetry"))
                {
                    bool telemetryToggleEnabled = false;
                    if (ImGui::MenuItem("Toggle Enabled"))
                    {
                        telemetryToggleEnabled = true;
                        m_telemetryCaptureTimeRemaining = -1;
                    }

                    if (telemetryToggleEnabled)
                    {
                        gEnv->pConsole->ExecuteString(s_toggleTelemetryConsoleCmd);
                    }

                    if (m_telemetryCaptureTimeRemaining <= 0.0f)
                    {
                        if (ImGui::SmallButton(AZStd::string::format("Enable for %.01f seconds \n(ImGui will close and re-open upon completion)", m_telemetryCaptureTime).c_str()))
                        {
                            StartTelemetryCapture();
                        }
                        ImGui::DragFloat("Capture Time", &m_telemetryCaptureTime, 0.1f, 0.1f, 600.0f);
                    }
                    else
                    {
                        ImGui::TextColored(ImGui::Colors::s_NiceLabelColor, "Currently Auto-Capturing for % .01f / % .01f", (m_telemetryCaptureTime - m_telemetryCaptureTimeRemaining), m_telemetryCaptureTime);
                    }

                    ImGui::EndMenu();
                }

                if (ImGui::BeginMenu("Video Options"))
                {
                    // VSync
                    IMGUI_DRAW_CVAR_CHECKBOX("vsync_interval", "VSync");

                    // Max Frame Rate
                    static ICVar* maxFPSCVar = gEnv->pConsole->GetCVar("sys_MaxFPS");
                    if (maxFPSCVar)
                    {
                        // Display max frame rate
                        ImGui::Text("Max FPS: %d", maxFPSCVar->GetIVal());
                        
                        // Shortcut buttons
                        int fpsToSet = 0;
                        if (ImGui::SmallButton("30"))
                        {
                            fpsToSet = 30;
                        }
                        ImGui::SameLine();
                        if (ImGui::SmallButton("60"))
                        {
                            fpsToSet = 60;
                        }
                        ImGui::SameLine();
                        if (ImGui::SmallButton("unlocked"))
                        {
                            fpsToSet = -1;
                        }

                        // fpsToSet will be 0 if no one set it. 
                        if (fpsToSet && fpsToSet != maxFPSCVar->GetIVal())
                        {
                            maxFPSCVar->Set(fpsToSet);
                        }
                    }

                    ImGui::Separator();

                    // FullScreen options
                    IMGUI_DRAW_CVAR_CHECKBOX("r_Fullscreen", "FullScreen");
                    IMGUI_DRAW_CVAR_CHECKBOX("r_FullscreenWindow", "FullScreen Window");
                    IMGUI_DRAW_CVAR_CHECKBOX("r_FullscreenNativeRes", "FullScreen Native Resolution");

                    ImGui::Separator();

                    // Render Resolution ( pending valid CVARS
                    static ICVar* widthCVar = gEnv->pConsole->GetCVar("r_width");
                    static ICVar* heightCVar = gEnv->pConsole->GetCVar("r_height");
                    if (widthCVar && heightCVar)
                    {
                        if (ImGui::BeginMenu(AZStd::string::format("Render Resolution ( %d x %d )", widthCVar->GetIVal(), heightCVar->GetIVal()).c_str()))
                        {
                            ImGui::TextColored(ImGui::Colors::s_PlainLabelColor, "Current Render Resolution: ");
                            ImGui::SameLine();
                            ImGui::TextColored(ImGui::Colors::s_NiceLabelColor, "%d x %d", widthCVar->GetIVal(), heightCVar->GetIVal());

                            const int aspectRatioCount = sizeof(s_renderAspectRatios) / sizeof(s_renderAspectRatios[0]);
                            const int resolutionWidthsCount = sizeof(s_renderResolutionWidths) / sizeof(s_renderResolutionWidths[0]);
                            for (int i = 0; i < aspectRatioCount; i++)
                            {
                                ImGui::TextColored(ImGui::Colors::s_NiceLabelColor, "%d:%d", s_renderAspectRatios[i][0], s_renderAspectRatios[i][1]);
                                for (int j = 0; j < resolutionWidthsCount; j++)
                                {
                                    const int renderHeight = s_renderResolutionWidths[j] * s_renderAspectRatios[i][1] / s_renderAspectRatios[i][0];
                                    if (ImGui::Button(AZStd::string::format("%d x %d", s_renderResolutionWidths[j], renderHeight).c_str(), ImVec2(400, 0)))
                                    {
                                        widthCVar->Set(s_renderResolutionWidths[j]);
                                        heightCVar->Set(renderHeight);
                                    }
                                }
                            }

                            // End Render Resolution menu
                            ImGui::EndMenu();
                        }
                    }

                    // End Video Options Menu
                    ImGui::EndMenu();
                }

                // Frame Rate Cap and VSync CVAR
                if (ImGui::BeginMenu("ImGui Options"))
                {
                    ImGui::TextColored(ImGui::Colors::s_PlainLabelColor, "Input Options:");
                    // Controller Support - Contextual
                    {
                        bool controllerEnabled = false;
                        ImGuiManagerBus::BroadcastResult(controllerEnabled, &IImGuiManager::IsControllerSupportModeEnabled, ImGuiControllerModeFlags::Contextual);

                        bool controllerEnabledCheckbox = controllerEnabled;
                        ImGui::Checkbox(AZStd::string::format("Controller Support (Contextual) %s (Click Checkbox to Toggle)", controllerEnabledCheckbox ? "On" : "Off").c_str(), &controllerEnabledCheckbox);
                        if (controllerEnabledCheckbox != controllerEnabled)
                        {
                            ImGuiManagerBus::Broadcast(&IImGuiManager::EnableControllerSupportMode, ImGuiControllerModeFlags::Contextual, controllerEnabledCheckbox);
                        }
                    }

                    // Controller Support - Mouse
                    {
                        bool controllerMouseEnabled = false;
                        ImGuiManagerBus::BroadcastResult(controllerMouseEnabled, &IImGuiManager::IsControllerSupportModeEnabled, ImGuiControllerModeFlags::Mouse);

                        bool controllerMouseEnabledCheckbox = controllerMouseEnabled;
                        ImGui::Checkbox(AZStd::string::format("Controller Support (Mouse) %s (Click Checkbox to Toggle)", controllerMouseEnabledCheckbox ? "On" : "Off").c_str(), &controllerMouseEnabledCheckbox);
                        if (controllerMouseEnabledCheckbox != controllerMouseEnabled)
                        {
                            ImGuiManagerBus::Broadcast(&IImGuiManager::EnableControllerSupportMode, ImGuiControllerModeFlags::Mouse, controllerMouseEnabledCheckbox);
                        }

                        // Only draw Controller Mouse Sensitivity slider if the mouse is enabled
                        if (controllerMouseEnabled)
                        {
                            float controllerMouseSensitivity = 1.0f;
                            ImGuiManagerBus::BroadcastResult(controllerMouseSensitivity, &IImGuiManager::GetControllerMouseSensitivity);

                            float controllerMouseSensitivitySlider = controllerMouseSensitivity;
                            ImGui::DragFloat("Controller Mouse Sensitivity", &controllerMouseSensitivitySlider, 0.1f, 0.1f, 50.0f);

                            if (controllerMouseSensitivitySlider != controllerMouseSensitivity)
                            {
                                ImGuiManagerBus::Broadcast(&IImGuiManager::SetControllerMouseSensitivity, controllerMouseSensitivitySlider);
                            }
                        }
                    }

                    // Discrete Input Mode
                    // Not available in edit mode because inputs are discrete there anyway.
                    if (!gEnv->IsEditor() || gEnv->IsEditorGameMode())
                    {
                        bool discreteInputEnabledCheckbox = discreteInputEnabled;
                        ImGui::Checkbox(AZStd::string::format("Discrete Input %s (Click Checkbox to Toggle)", discreteInputEnabledCheckbox ? "On" : "Off").c_str(), &discreteInputEnabledCheckbox);
                        if (discreteInputEnabledCheckbox != discreteInputEnabled)
                        {
                            ImGuiManagerBus::Broadcast(&IImGuiManager::SetEnableDiscreteInputMode, discreteInputEnabledCheckbox);
                        }
                    }

                    // Controller Legend
                    ImGui::Text("Controller Legend ");
                    ImGui::SameLine();
                    if (ImGui::SmallButton("Window"))
                    {
                        m_controllerLegendWindowVisible = true;
                    }
                    ImGui::SameLine();
                    if (ImGui::BeginMenu("Peek"))
                    {
                        OnImGuiUpdate_DrawControllerLegend();
                        ImGui::EndMenu();
                    }

                    ImGui::Separator();

                    if (ImGui::BeginMenu("ImGui Resolution"))
                    {
                        // Resolution Mode
                        ImGui::TextColored(ImGui::Colors::s_NiceLabelColor, "ImGui Resolution Mode:");

                        ImGuiResolutionMode resMode = ImGuiResolutionMode::MatchRenderResolution;
                        ImGuiManagerBus::BroadcastResult(resMode, &IImGuiManager::GetResolutionMode);

                        int resModeRadioBtn = static_cast<int>(resMode);
                        ImGui::RadioButton("Force Resolution", &resModeRadioBtn, static_cast<int>(ImGuiResolutionMode::LockToResolution));
                        ImGui::SameLine();
                        ImGui::RadioButton("Match Render Resolution", &resModeRadioBtn, static_cast<int>(ImGuiResolutionMode::MatchRenderResolution));
                        ImGui::SameLine();
                        ImGui::RadioButton("Match Render Resolution To Max", &resModeRadioBtn, static_cast<int>(ImGuiResolutionMode::MatchToMaxRenderResolution));

                        ImGuiResolutionMode resModeRadioBtnResult = static_cast<ImGuiResolutionMode>(resModeRadioBtn);
                        if (resModeRadioBtnResult != resMode)
                        {
                            ImGuiManagerBus::Broadcast(&IImGuiManager::SetResolutionMode, resModeRadioBtnResult);
                        }

                        // Resolutions
                        ImVec2 imGuiRes;
                        ImGuiManagerBus::BroadcastResult(imGuiRes, &IImGuiManager::GetImGuiRenderResolution);

                        ImGui::TextColored(ImGui::Colors::s_PlainLabelColor, "Current ImGui Resolution: ");
                        ImGui::SameLine();
                        ImGui::TextColored(ImGui::Colors::s_NiceLabelColor, "%.0f x %.0f", imGuiRes.x, imGuiRes.y);

                        const int aspectRatioCount = sizeof(s_renderAspectRatios) / sizeof(s_renderAspectRatios[0]);
                        const int resolutionWidthsCount = sizeof(s_renderResolutionWidths) / sizeof(s_renderResolutionWidths[0]);
                        for (int i = 0; i < aspectRatioCount; i++)
                        {
                            ImGui::TextColored(ImGui::Colors::s_NiceLabelColor, "%d:%d", s_renderAspectRatios[i][0], s_renderAspectRatios[i][1]);
                            for (int j = 0; j < resolutionWidthsCount; j++)
                            {
                                const int renderHeight = s_renderResolutionWidths[j] * s_renderAspectRatios[i][1] / s_renderAspectRatios[i][0];
                                if (ImGui::Button(AZStd::string::format("%d x %d", s_renderResolutionWidths[j], renderHeight).c_str(), ImVec2(400, 0)))
                                {
                                    ImVec2 newRenderRes(static_cast<float>(s_renderResolutionWidths[j]), static_cast<float>(renderHeight));
                                    ImGuiManagerBus::Broadcast(&IImGuiManager::SetImGuiRenderResolution, newRenderRes);
                                }
                            }
                        }

                        // End ImGui Render Resolution menu
                        ImGui::EndMenu();
                    }

                    // End ImGui Options Menu
                    ImGui::EndMenu();
                }

                if (ImGui::BeginMenu("Misc."))
                {
                    // Assert Level
                    static ICVar* gAssertLevelCVAR = gEnv->pConsole->GetCVar("sys_asserts");
                    if (gAssertLevelCVAR)
                    {
                        int assertLevelValue = gAssertLevelCVAR->GetIVal();
                        int dragIntVal = assertLevelValue;
                        ImGui::Text("sys_asserts: %d ( 0-off | 1-log | 2-popup | 3-crash )", assertLevelValue);
                        ImGui::SliderInt("##sys_asserts", &dragIntVal, 0, 3);
                        if (dragIntVal != assertLevelValue)
                        {
                            gAssertLevelCVAR->Set(dragIntVal);
                        }
                    }

                    // End Misc Options Menu
                    ImGui::EndMenu();
                }

                if (ImGui::MenuItem("ImGui Demo"))
                {
                    m_showImGuiDemo = true;
                }

                // End LY Common Tools menu
                ImGui::EndMenu();
            }

            if (m_showImGuiDemo)
            {
                ImGui::ShowDemoWindow(&m_showImGuiDemo);
            }

            const float labelSize = dpiAwareSizeFn(100.0f + viewportBorderPadding.m_right) + rightAlignedBorderOffset;
            const float buttonSize = dpiAwareSizeFn(40.0f + viewportBorderPadding.m_right) + rightAlignedBorderOffset;
            ImGuiUpdateListenerBus::Broadcast(&IImGuiUpdateListener::OnImGuiMainMenuUpdate);
            ImGui::SameLine(ImGui::GetWindowContentRegionMax().x - labelSize);
            float backgroundHeight = ImGui::GetTextLineHeight() + dpiAwareSizeFn(3.0f);
            ImVec2 cursorPos = ImGui::GetCursorScreenPos();
            ImGui::GetWindowDrawList()->AddRectFilled(
                cursorPos, ImVec2(cursorPos.x + labelSize, cursorPos.y + backgroundHeight), IM_COL32(0, 115, 187, 255));
            ImGui::SameLine(ImGui::GetWindowContentRegionMax().x - labelSize);
            ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 1);
            ImGui::Text("ImGui:ON");
            ImGui::SameLine(ImGui::GetWindowContentRegionMax().x - buttonSize);
            ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(0, 0, 0, 255));
            ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(255, 255, 255, 255));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(128, 128, 128, 255));
            if (ImGui::SmallButton("home"))
            {
                ImGuiManagerBus::Broadcast(&IImGuiManager::ToggleThroughImGuiVisibleState);
            }
            ImGui::PopStyleColor(3);
            ImGui::EndMainMenuBar();
        }

        // Restore original safe area.
        ImGui::GetStyle().DisplaySafeAreaPadding = cachedSafeArea;

        // Update Contextual Controller Window
        if (m_controllerLegendWindowVisible)
        {
            if (ImGui::Begin("Controller ImGui Input Legend", &m_controllerLegendWindowVisible, ImGuiWindowFlags_NoSavedSettings))
            {
                OnImGuiUpdate_DrawControllerLegend();
            }
            ImGui::End();
        }

        // Update sub menus
        m_assetExplorer.ImGuiUpdate();
        m_cameraMonitor.ImGuiUpdate();
        m_inputMonitor.ImGuiUpdate();
        m_entityOutliner.ImGuiUpdate();
        if (m_showDeltaTimeGraphs)
        {
            ImGui::SetNextWindowSize({ 500, 200 }, ImGuiCond_Once);
            if (ImGui::Begin(
                    "Delta Time Graphs", &m_showDeltaTimeGraphs,
                    ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_NoSavedSettings))
            {
                m_deltaTimeHistogram.Draw(ImGui::GetColumnWidth(), 100.0f);
            }
            ImGui::End();
        }
    }

    void ImGuiLYCommonMenu::OnImGuiUpdate_DrawControllerLegend()
    {
        bool contextualControllerEnabled = false;
        ImGuiManagerBus::BroadcastResult(contextualControllerEnabled, &IImGuiManager::IsControllerSupportModeEnabled, ImGuiControllerModeFlags::Contextual);

        bool controllerMouseEnabled = false;
        ImGuiManagerBus::BroadcastResult(controllerMouseEnabled, &IImGuiManager::IsControllerSupportModeEnabled, ImGuiControllerModeFlags::Mouse);

        ImGui::TextColored(ImGui::Colors::s_PlainLabelColor, "Contextual Controller Input Legend. Currently Enabled:");
        ImGui::SameLine();
        ImGui::TextColored(ImGui::Colors::s_NiceLabelColor, contextualControllerEnabled ? "True" : "False");
        ImGui::NewLine();

        ImGui::Columns(2);
        ImGui::SetColumnWidth(-1, 170.0f);
        ImGui::TextColored(ImGui::Colors::s_NiceLabelColor, "Controller Input");
        ImGui::NextColumn();
        ImGui::TextColored(ImGui::Colors::s_NiceLabelColor, "ImGui Action");
        ImGui::Separator();
        ImGui::NextColumn();

        ImGui::TextColored(ImGui::Colors::s_PlainLabelColor, "D-Pad U/D/L/R");
        ImGui::NextColumn();
        ImGui::Bullet();
        ImGui::TextColored(ImGui::Colors::s_PlainLabelColor, "Move");
        ImGui::Bullet();
        ImGui::TextColored(ImGui::Colors::s_PlainLabelColor, "Tweak Values (when activated with A)");
        ImGui::Bullet();
        ImGui::TextColored(ImGui::Colors::s_PlainLabelColor, "Resize Window (when holding X)");
        ImGui::Separator();
        ImGui::NextColumn();

        ImGui::TextColored(ImGui::Colors::s_PlainLabelColor, "Left Stick");
        ImGui::NextColumn();
        ImGui::Bullet();
        ImGui::TextColored(ImGui::Colors::s_PlainLabelColor, "Scroll");
        ImGui::Bullet();
        ImGui::TextColored(ImGui::Colors::s_PlainLabelColor, "Move Window (when holding X)");
        ImGui::Separator();
        ImGui::NextColumn();

        ImGui::TextColored(ImGui::Colors::s_PlainLabelColor, "X (Left Face Button)");
        ImGui::NextColumn();
        ImGui::Bullet();
        ImGui::TextColored(ImGui::Colors::s_PlainLabelColor, "Tap: Toggle Menu");
        ImGui::Bullet();
        ImGui::TextColored(ImGui::Colors::s_PlainLabelColor, "Hold + L1/R1: Focus Windows");
        ImGui::Bullet();
        ImGui::TextColored(ImGui::Colors::s_PlainLabelColor, "Hold + D-Pad: Resize Window");
        ImGui::Bullet();
        ImGui::TextColored(ImGui::Colors::s_PlainLabelColor, "Hold + Left Stick: Move Window");
        ImGui::Separator();
        ImGui::NextColumn();

        ImGui::TextColored(ImGui::Colors::s_PlainLabelColor, "Y (Upper Face Button)");
        ImGui::NextColumn();
        ImGui::Bullet();
        ImGui::TextColored(ImGui::Colors::s_PlainLabelColor, "Exit text / on-screen keyboard");
        ImGui::Separator();
        ImGui::NextColumn();

        ImGui::TextColored(ImGui::Colors::s_PlainLabelColor, "B (Right Face Button)");
        ImGui::NextColumn();
        ImGui::Bullet();
        ImGui::TextColored(ImGui::Colors::s_PlainLabelColor, "Cancel / Close / Exit");
        ImGui::Separator();
        ImGui::NextColumn();

        ImGui::TextColored(ImGui::Colors::s_PlainLabelColor, "A (Lower Face Button)");
        ImGui::NextColumn();
        ImGui::Bullet();
        ImGui::TextColored(ImGui::Colors::s_PlainLabelColor, "Activate / Open / Toggle");
        ImGui::Bullet();
        ImGui::TextColored(ImGui::Colors::s_PlainLabelColor, "Tweak values with D-Pad");
        ImGui::TextColored(ImGui::Colors::s_PlainLabelColor, "   (+ L1/R1 to tweak slower/faster)");
        ImGui::NextColumn();

        ImGui::Columns(1);
        ImGui::Separator();

        ImGui::NewLine();
        ImGui::TextColored(ImGui::Colors::s_PlainLabelColor, "Controller Mouse Legend. Currently Enabled:");
        ImGui::SameLine();
        ImGui::TextColored(ImGui::Colors::s_NiceLabelColor, controllerMouseEnabled ? "True" : "False");
        ImGui::NewLine();

        ImGui::Columns(2);
        ImGui::SetColumnWidth(-1, 170.0f);
        ImGui::TextColored(ImGui::Colors::s_NiceLabelColor, "Controller Input");
        ImGui::NextColumn();
        ImGui::TextColored(ImGui::Colors::s_NiceLabelColor, "Mouse Action");
        ImGui::Separator();
        ImGui::NextColumn();

        ImGui::TextColored(ImGui::Colors::s_PlainLabelColor, "Left Stick");
        ImGui::NextColumn();
        ImGui::Bullet();
        ImGui::TextColored(ImGui::Colors::s_PlainLabelColor, "Move Mouse Pointer");
        ImGui::Separator();
        ImGui::NextColumn();

        ImGui::TextColored(ImGui::Colors::s_PlainLabelColor, "A");
        ImGui::NextColumn();
        ImGui::Bullet();
        ImGui::TextColored(ImGui::Colors::s_PlainLabelColor, "Left Mouse Button (Btn1)");
        ImGui::Separator();
        ImGui::NextColumn();

        ImGui::TextColored(ImGui::Colors::s_PlainLabelColor, "B");
        ImGui::NextColumn();
        ImGui::Bullet();
        ImGui::TextColored(ImGui::Colors::s_PlainLabelColor, "Right Mouse Button (Btn2)");
        ImGui::NextColumn();

        ImGui::Columns(1);
    }

    void ImGuiLYCommonMenu::StartTelemetryCapture()
    {
        // Start the Capture
        gEnv->pConsole->ExecuteString(s_toggleTelemetryConsoleCmd);

        // Set the timer and connect to tick bus to count down.
        m_telemetryCaptureTimeRemaining = m_telemetryCaptureTime;

        // Get the current ImGui Display state to restore it later.
        ImGuiManagerBus::BroadcastResult(m_telemetryCapturePreCaptureState, &IImGuiManager::GetDisplayState);

        // Turn off the ImGui Manager
        ImGuiManagerBus::Broadcast(&IImGuiManager::SetDisplayState, DisplayState::Hidden);
    }

    void ImGuiLYCommonMenu::StopTelemetryCapture()
    {
        // Stop the Capture
        gEnv->pConsole->ExecuteString(s_toggleTelemetryConsoleCmd);

        // Restore ImGui State
        // Turn off the ImGui Manager
        ImGuiManagerBus::Broadcast(&IImGuiManager::SetDisplayState, m_telemetryCapturePreCaptureState);

        // Reset timer and disconnect tick bus
        m_telemetryCaptureTimeRemaining = 0.0f;
    }

    // OnTick just used for telemetry captures.
    void ImGuiLYCommonMenu::OnTick(float deltaTime, [[maybe_unused]] AZ::ScriptTimePoint time)
    {
        m_deltaTimeHistogram.PushValue(deltaTime*1000.0f); // convert to milliseconds

        if (m_telemetryCaptureTimeRemaining > 0.0f)
        {
            m_telemetryCaptureTimeRemaining -= deltaTime;
            if (m_telemetryCaptureTimeRemaining <= 0.0f)
            {
                StopTelemetryCapture();
            }
        }
    }
} // namespace ImGui

#endif // IMGUI_ENABLED
