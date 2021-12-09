/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "ImGuiManager.h"
#include <ImGuiContextScope.h>
#include <AzCore/PlatformIncl.h>
#include <OtherActiveImGuiBus.h>

#ifdef IMGUI_ENABLED

#include <AzCore/Interface/Interface.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/std/containers/fixed_unordered_map.h>
#include <AzCore/Time/ITime.h>
#include <AzFramework/Input/Buses/Requests/InputTextEntryRequestBus.h>
#include <AzFramework/Input/Devices/Mouse/InputDeviceMouse.h>
#include <AzFramework/Input/Devices/Keyboard/InputDeviceKeyboard.h>
#include <AzFramework/Input/Devices/Gamepad/InputDeviceGamepad.h>
#include <AzFramework/Input/Devices/Touch/InputDeviceTouch.h>
#include <AzFramework/Input/Devices/VirtualKeyboard/InputDeviceVirtualKeyboard.h>
#include <IConsole.h>
#include <imgui/imgui_internal.h>
#include <sstream>
#include <string>

using namespace AzFramework;
using namespace ImGui;

// Wheel Delta const value.
static const constexpr uint32_t IMGUI_WHEEL_DELTA = 120; // From WinUser.h, for Linux

// Typedef and local static map to hold LyInput->ImGui Nav mappings ( filled up in Initialize() )
using LyButtonImGuiNavIndexPair = AZStd::pair<AzFramework::InputChannelId, ImGuiNavInput_>;
using LyButtonImGuiNavIndexMap = AZStd::fixed_unordered_map<AzFramework::InputChannelId, ImGuiNavInput_, 11, 32>;
static LyButtonImGuiNavIndexMap s_lyInputToImGuiNavIndexMap;

/**
    An anonymous namespace containing helper functions for interoperating with AzFrameworkInput.
 */
namespace
{
    /**
        Utility function to map an AzFrameworkInput device key to its integer index.

        @param inputChannelId the ID for an AzFrameworkInput device key.
        @return the index of the indicated key, or -1 if not found.
     */
    unsigned int GetAzKeyIndex(const InputChannelId& inputChannelId)
    {
        const auto& keys = InputDeviceKeyboard::Key::All;
        const auto& it = AZStd::find(keys.cbegin(), keys.cend(), inputChannelId);
        return it != keys.cend() ? static_cast<unsigned int>(it - keys.cbegin()) : UINT_MAX;
    }

    /**
        Utility function to map an AzFrameworkInput mouse device button to its integer index.

        @param inputChannelId the ID for an AzFrameworkInput mouse device button.
        @return the index of the indicated button, or -1 if not found.
     */
    unsigned int GetAzMouseButtonIndex(const InputChannelId& inputChannelId)
    {
        const auto& buttons = InputDeviceMouse::Button::All;
        const auto& it = AZStd::find(buttons.cbegin(), buttons.cend(), inputChannelId);
        return it != buttons.cend() ? static_cast<unsigned int>(it - buttons.cbegin()) : UINT_MAX;
    }

    /**
        Utility function to map an AzFrameworkInput touch input to its integer index.

        @param inputChannelId the ID for an AzFrameworkInput touch input.
        @return the index of the indicated touch, or -1 if not found.
     */
    unsigned int GetAzTouchIndex(const InputChannelId& inputChannelId)
    {
        const auto& touches = InputDeviceTouch::Touch::All;
        const auto& it = AZStd::find(touches.cbegin(), touches.cend(), inputChannelId);
        return it != touches.cend() ? static_cast<unsigned int>(it - touches.cbegin()) : UINT_MAX;
    }
}

void ImGuiManager::Initialize()
{
    // Register for Buses
    ImGuiManagerBus::Handler::BusConnect();

    // Register for Input Notifications
    InputChannelEventListener::Connect();
    InputTextEventListener::Connect();

    // Dynamically Load ImGui
#if defined(LOAD_IMGUI_LIB_DYNAMICALLY) && !defined(AZ_MONOLITHIC_BUILD)

    AZ::OSString imgGuiLibPath = "imguilib";

    // Let the application process the path
    AZ::ComponentApplicationBus::Broadcast(&AZ::ComponentApplicationBus::Events::ResolveModulePath, imgGuiLibPath);
    m_imgSharedLib = AZ::DynamicModuleHandle::Create(imgGuiLibPath.c_str());
    if (!m_imgSharedLib->Load(false))
    {
        AZ_Warning("ImGuiManager", false, "%s %s", __func__, "Unable to load " AZ_DYNAMIC_LIBRARY_PREFIX "imguilib" AZ_DYNAMIC_LIBRARY_EXTENSION "-- Skipping ImGui Initialization.");
        return;
    }

#endif // defined(LOAD_IMGUI_LIB_DYNAMICALLY)  && !defined(AZ_MONOLITHIC_BUILD)

    // Create ImGui Context
    m_imguiContext = ImGui::CreateContext();
    ImGui::ImGuiContextScope contextScope(m_imguiContext);

    // Set config file
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = "imgui.ini";

    // Enable Nav Keyboard by default and allow 
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableSetMousePos;

    // Config Keyboard Mapping keys.
    io.KeyMap[ImGuiKey_Tab] = GetAzKeyIndex(InputDeviceKeyboard::Key::EditTab);
    io.KeyMap[ImGuiKey_LeftArrow] = GetAzKeyIndex(InputDeviceKeyboard::Key::NavigationArrowLeft);
    io.KeyMap[ImGuiKey_RightArrow] = GetAzKeyIndex(InputDeviceKeyboard::Key::NavigationArrowRight);
    io.KeyMap[ImGuiKey_UpArrow] = GetAzKeyIndex(InputDeviceKeyboard::Key::NavigationArrowUp);
    io.KeyMap[ImGuiKey_DownArrow] = GetAzKeyIndex(InputDeviceKeyboard::Key::NavigationArrowDown);
    io.KeyMap[ImGuiKey_PageUp] = GetAzKeyIndex(InputDeviceKeyboard::Key::NavigationPageUp);
    io.KeyMap[ImGuiKey_PageDown] = GetAzKeyIndex(InputDeviceKeyboard::Key::NavigationPageDown);
    io.KeyMap[ImGuiKey_Home] = GetAzKeyIndex(InputDeviceKeyboard::Key::NavigationHome);
    io.KeyMap[ImGuiKey_End] = GetAzKeyIndex(InputDeviceKeyboard::Key::NavigationEnd);
    io.KeyMap[ImGuiKey_Insert] = GetAzKeyIndex(InputDeviceKeyboard::Key::NavigationInsert);
    io.KeyMap[ImGuiKey_Delete] = GetAzKeyIndex(InputDeviceKeyboard::Key::NavigationDelete);
    io.KeyMap[ImGuiKey_Backspace] = GetAzKeyIndex(InputDeviceKeyboard::Key::EditBackspace);
    io.KeyMap[ImGuiKey_Space] = GetAzKeyIndex(InputDeviceKeyboard::Key::EditSpace);
    io.KeyMap[ImGuiKey_Enter] = GetAzKeyIndex(InputDeviceKeyboard::Key::EditEnter);
    io.KeyMap[ImGuiKey_Escape] = GetAzKeyIndex(InputDeviceKeyboard::Key::Escape);
    io.KeyMap[ImGuiKey_A] = GetAzKeyIndex(InputDeviceKeyboard::Key::AlphanumericA);
    io.KeyMap[ImGuiKey_C] = GetAzKeyIndex(InputDeviceKeyboard::Key::AlphanumericC);
    io.KeyMap[ImGuiKey_V] = GetAzKeyIndex(InputDeviceKeyboard::Key::AlphanumericV);
    io.KeyMap[ImGuiKey_X] = GetAzKeyIndex(InputDeviceKeyboard::Key::AlphanumericX);
    io.KeyMap[ImGuiKey_Y] = GetAzKeyIndex(InputDeviceKeyboard::Key::AlphanumericY);
    io.KeyMap[ImGuiKey_Z] = GetAzKeyIndex(InputDeviceKeyboard::Key::AlphanumericZ);

    // Initialize controller button mapping
    s_lyInputToImGuiNavIndexMap.insert(LyButtonImGuiNavIndexPair(InputDeviceGamepad::Button::A, ImGuiNavInput_Activate));
    s_lyInputToImGuiNavIndexMap.insert(LyButtonImGuiNavIndexPair(InputDeviceGamepad::Button::B, ImGuiNavInput_Cancel));
    s_lyInputToImGuiNavIndexMap.insert(LyButtonImGuiNavIndexPair(InputDeviceGamepad::Button::X, ImGuiNavInput_Menu));
    s_lyInputToImGuiNavIndexMap.insert(LyButtonImGuiNavIndexPair(InputDeviceGamepad::Button::Y, ImGuiNavInput_Input));
    s_lyInputToImGuiNavIndexMap.insert(LyButtonImGuiNavIndexPair(InputDeviceGamepad::Button::DU, ImGuiNavInput_DpadUp));
    s_lyInputToImGuiNavIndexMap.insert(LyButtonImGuiNavIndexPair(InputDeviceGamepad::Button::DD, ImGuiNavInput_DpadDown));
    s_lyInputToImGuiNavIndexMap.insert(LyButtonImGuiNavIndexPair(InputDeviceGamepad::Button::DL, ImGuiNavInput_DpadLeft));
    s_lyInputToImGuiNavIndexMap.insert(LyButtonImGuiNavIndexPair(InputDeviceGamepad::Button::DR, ImGuiNavInput_DpadRight));
    s_lyInputToImGuiNavIndexMap.insert(LyButtonImGuiNavIndexPair(InputDeviceGamepad::Button::R1, ImGuiNavInput_FocusNext));
    s_lyInputToImGuiNavIndexMap.insert(LyButtonImGuiNavIndexPair(InputDeviceGamepad::Button::L1, ImGuiNavInput_FocusPrev));
    s_lyInputToImGuiNavIndexMap.insert(LyButtonImGuiNavIndexPair(InputDeviceGamepad::Trigger::L2, ImGuiNavInput_TweakSlow));
    s_lyInputToImGuiNavIndexMap.insert(LyButtonImGuiNavIndexPair(InputDeviceGamepad::Trigger::R2, ImGuiNavInput_TweakFast));
    s_lyInputToImGuiNavIndexMap.insert(LyButtonImGuiNavIndexPair(InputDeviceGamepad::ThumbStickDirection::LU, ImGuiNavInput_LStickUp));
    s_lyInputToImGuiNavIndexMap.insert(LyButtonImGuiNavIndexPair(InputDeviceGamepad::ThumbStickDirection::LD, ImGuiNavInput_LStickDown));
    s_lyInputToImGuiNavIndexMap.insert(LyButtonImGuiNavIndexPair(InputDeviceGamepad::ThumbStickDirection::LL, ImGuiNavInput_LStickLeft));
    s_lyInputToImGuiNavIndexMap.insert(LyButtonImGuiNavIndexPair(InputDeviceGamepad::ThumbStickDirection::LR, ImGuiNavInput_LStickRight));

    // Set the initial Display Size (gets updated each frame anyway)
    io.DisplaySize.x = 1920;
    io.DisplaySize.y = 1080;

    // Create a default font
    io.Fonts->AddFontDefault();
    io.Fonts->Build();

    // Broadcast ImGui Ready to Listeners
    ImGuiUpdateListenerBus::Broadcast(&IImGuiUpdateListener::OnImGuiInitialize);
    m_button1Pressed = m_button2Pressed = false;
    m_menuBarStatusChanged = false;

    // See if a hardware mouse device is connected on startup, we will use it to help determine if we should draw the mouse cursor and turn on controller support by default if one is not found.
    //  Future work here could include responding to the mouse being connected and disconnected at run-time, but this is fine for now.
    const AzFramework::InputDevice* mouseDevice = AzFramework::InputDeviceRequests::FindInputDevice(AzFramework::InputDeviceMouse::Id);
    m_hardwardeMouseConnected = mouseDevice && mouseDevice->IsConnected();

    AZ::Interface<ImGui::IImGuiManager>::Register(this);
}

void ImGuiManager::Shutdown()
{
    AZ::Interface<ImGui::IImGuiManager>::Unregister(this);

    if (!gEnv)
    {
        AZ_Warning("ImGuiManager", false, "%s %s", __func__, "gEnv Invalid -- Skipping ImGui Shutdown.");
        return;
    }
#if defined(LOAD_IMGUI_LIB_DYNAMICALLY)  && !defined(AZ_MONOLITHIC_BUILD)
    if (m_imgSharedLib->IsLoaded())
    {
        m_imgSharedLib->Unload();
    }
#endif

    // Unregister from Buses
    ImGuiManagerBus::Handler::BusDisconnect();
    InputChannelEventListener::Disconnect();
    InputTextEventListener::Disconnect();
    AzFramework::WindowNotificationBus::Handler::BusDisconnect();

    // Finally, destroy the ImGui Context.
    ImGui::DestroyContext(m_imguiContext);
}

void ImGui::ImGuiManager::OverrideRenderWindowSize(uint32_t width, uint32_t height)
{
    m_windowSize.m_width = width;
    m_windowSize.m_height = height;
    m_overridingWindowSize = true;
    // Don't listen for window updates if our window size is being overridden
    AzFramework::WindowNotificationBus::Handler::BusDisconnect();
}

void ImGui::ImGuiManager::RestoreRenderWindowSizeToDefault()
{
    m_overridingWindowSize = false;
    InitWindowSize();
}

void ImGui::ImGuiManager::SetDpiScalingFactor(float dpiScalingFactor)
{
    ImGui::ImGuiContextScope contextScope(m_imguiContext);
    ImGuiIO& io = ImGui::GetIO();
    // Set the global font scale to size our UI to the scaling factor
    // Note: Currently we use the default, 13px fixed-size IMGUI font, so this can get somewhat blurry
    io.FontGlobalScale = dpiScalingFactor;
}

float ImGui::ImGuiManager::GetDpiScalingFactor() const
{
    ImGui::ImGuiContextScope contextScope(m_imguiContext);
    ImGuiIO& io = ImGui::GetIO();
    return io.FontGlobalScale;
}

void ImGuiManager::Render()
{
    if (m_clientMenuBarState == DisplayState::Hidden && m_editorWindowState == DisplayState::Hidden)
    {
        return;
    }

    ImGui::ImGuiContextScope contextScope(m_imguiContext);

    // Update Display Size
    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = m_lastRenderResolution;

    if ((m_clientMenuBarState == DisplayState::Visible) || (m_editorWindowState != DisplayState::Hidden))
    {
        if (IsControllerSupportModeEnabled(ImGuiControllerModeFlags::Mouse))
        {
            // Update Mouse Position from Stick Position
            m_controllerMousePosition[0] = AZ::GetClamp(m_controllerMousePosition[0] + ((io.NavInputs[ImGuiNavInput_LStickRight] - io.NavInputs[ImGuiNavInput_LStickLeft]) * m_controllerMouseSensitivity), 0.0f, m_renderResolution.x);
            m_controllerMousePosition[1] = AZ::GetClamp(m_controllerMousePosition[1] + ((io.NavInputs[ImGuiNavInput_LStickDown] - io.NavInputs[ImGuiNavInput_LStickUp]) * m_controllerMouseSensitivity), 0.0f, m_renderResolution.y);
            io.MousePos.x = m_controllerMousePosition[0];
            io.MousePos.y = m_controllerMousePosition[1];
            io.MouseDown[0] = (io.NavInputs[ImGuiNavInput_Activate] > 0.1f);
            io.MouseDown[1] = (io.NavInputs[ImGuiNavInput_Cancel] > 0.1f);
        }
        else if (m_useLastPrimaryTouchPosition)
        {
            io.MousePos.x = m_lastPrimaryTouchPosition[0];
            io.MousePos.y = m_lastPrimaryTouchPosition[1];
            m_controllerMousePosition[0] = io.MousePos.x;
            m_controllerMousePosition[1] = io.MousePos.y;
            m_useLastPrimaryTouchPosition = false;
        }
        else
        {
            AZ::Vector2 systemCursorPositionNormalized = AZ::Vector2::CreateZero();
            InputSystemCursorRequestBus::EventResult(
                systemCursorPositionNormalized,
                InputDeviceMouse::Id,
                &InputSystemCursorRequests::GetSystemCursorPositionNormalized);
                       
            io.MousePos.x = systemCursorPositionNormalized.GetX() * m_lastRenderResolution.x;
            io.MousePos.y = systemCursorPositionNormalized.GetY() * m_lastRenderResolution.y;
            m_controllerMousePosition[0] = io.MousePos.x;
            m_controllerMousePosition[1] = io.MousePos.y;
        }

        // Clear NavInputs if either the mouse is explicitly enabled, or if the contextual controller is explicitly disabled
        if (IsControllerSupportModeEnabled(ImGuiControllerModeFlags::Mouse) || !IsControllerSupportModeEnabled(ImGuiControllerModeFlags::Contextual))
        {
            memset(io.NavInputs, 0, sizeof(io.NavInputs));
        }
    }

    // If no item and no window is focused, we should artificially add focus to the Main Menu Bar, to save 1 step when navigating with a controller.
    if (!ImGui::IsAnyItemFocused() && !ImGui::IsWindowFocused(ImGuiFocusedFlags_AnyWindow))
    {
        ImGuiWindow* mainMenuWin = ImGui::FindWindowByName("##MainMenuBar");
        if (mainMenuWin)
        {
            ImGui::GetCurrentContext()->NavLayer = ImGuiNavLayer_Menu;
            ImGui::GetCurrentContext()->NavWindow = mainMenuWin;
            ImGui::NavInitWindow(mainMenuWin, true);
        }
    }

    // Show or hide the virtual keyboard as necessary
    bool hasTextEntryStarted = false;
    AzFramework::InputTextEntryRequestBus::EventResult(hasTextEntryStarted,
                                                       AzFramework::InputDeviceVirtualKeyboard::Id,
                                                       &AzFramework::InputTextEntryRequests::HasTextEntryStarted);
    if (io.WantTextInput && !hasTextEntryStarted)
    {
        AzFramework::InputTextEntryRequests::VirtualKeyboardOptions options;
        AzFramework::InputTextEntryRequestBus::Broadcast(&AzFramework::InputTextEntryRequests::TextEntryStart, options);
    }
    else if (!io.WantTextInput && hasTextEntryStarted)
    {
        AzFramework::InputTextEntryRequestBus::Broadcast(&AzFramework::InputTextEntryRequests::TextEntryStop);
        io.KeysDown[GetAzKeyIndex(InputDeviceKeyboard::Key::EditEnter)] = false;
    }

    // Start New Frame
    ImGui::NewFrame();

    //// START FROM PREUPDATE
    ICVar* consoleDisabled = gEnv->pConsole->GetCVar("sys_DeactivateConsole");
    if (consoleDisabled && consoleDisabled->GetIVal() != 0)
    {
        m_clientMenuBarState = DisplayState::Hidden;
        m_editorWindowState = DisplayState::Hidden;
    }

    // Advance ImGui by Elapsed Frame Time
    const AZ::TimeUs gameTickTimeUs = AZ::GetSimulationTickDeltaTimeUs();
    io.DeltaTime = AZ::TimeUsToSeconds(gameTickTimeUs);
    //// END FROM PREUPDATE

    AZ::u32 backBufferWidth = m_windowSize.m_width;
    AZ::u32 backBufferHeight = m_windowSize.m_height;

    // Find ImGui Render Resolution. 
    int renderRes[2];
    switch (m_resolutionMode)
    {
        default:
            break;

        case ImGuiResolutionMode::LockToResolution:
            renderRes[0] = static_cast<int>(m_renderResolution.x);
            renderRes[1] = static_cast<int>(m_renderResolution.y);
            break;

        case ImGuiResolutionMode::MatchRenderResolution:
            renderRes[0] = backBufferWidth;
            renderRes[1] = backBufferHeight;
            break;

        case ImGuiResolutionMode::MatchToMaxRenderResolution:
            if (backBufferWidth <= static_cast<AZ::u32>(m_renderResolution.x))
            {
                renderRes[0] = backBufferWidth;
                renderRes[1] = backBufferHeight;
            }
            else
            {
                renderRes[0] = static_cast<int>(m_renderResolution.x);
                renderRes[1] = static_cast<int>(m_renderResolution.y);
            }
            break;
    }

    ImVec2 scaleRects(  static_cast<float>(backBufferWidth) / static_cast<float>(renderRes[0]),
                        static_cast<float>(backBufferHeight / static_cast<float>(renderRes[1])));

    // Save off the last render resolution for input
    m_lastRenderResolution.x = static_cast<float>(renderRes[0]);
    m_lastRenderResolution.y = static_cast<float>(renderRes[1]);

    // Render!
    RenderImGuiBuffers(scaleRects);

    // Clear the simulated backspace key
    if (m_simulateBackspaceKeyPressed)
    {
        io.KeysDown[GetAzKeyIndex(InputDeviceKeyboard::Key::EditBackspace)] = false;
        m_simulateBackspaceKeyPressed = false;
    }
}

/**
    @return true if input should be consumed, false otherwise.
 */
bool ImGuiManager::OnInputChannelEventFiltered(const InputChannel& inputChannel)
{
    ImGui::ImGuiContextScope contextScope(m_imguiContext);

    ImGuiIO& io = ImGui::GetIO();
    const InputChannelId& inputChannelId = inputChannel.GetInputChannelId();
    const InputDeviceId& inputDeviceId = inputChannel.GetInputDevice().GetInputDeviceId();

    // Handle Keyboard Hotkeys
    if (InputDeviceKeyboard::IsKeyboardDevice(inputDeviceId) && inputChannel.IsStateBegan())
    {
        // Cycle through ImGui Menu Bar States on Home button press
        if (inputChannelId == InputDeviceKeyboard::Key::NavigationHome)
        {
            ToggleThroughImGuiVisibleState();
        }

        // Cycle through Standalone Editor Window States
        if (inputChannel.GetInputChannelId() == InputDeviceKeyboard::Key::NavigationEnd)
        {
            if (gEnv->IsEditor() && m_editorWindowState == DisplayState::Hidden)
            {
                ImGuiUpdateListenerBus::Broadcast(&IImGuiUpdateListener::OnOpenEditorWindow);
            }
            else
            {
                m_editorWindowState = m_editorWindowState == DisplayState::Visible
                                          ? DisplayState::VisibleNoMouse
                                          : DisplayState::Visible;
            }
        }
    }

    // Handle Keyboard Modifier Keys
    if (InputDeviceKeyboard::IsKeyboardDevice(inputDeviceId))
    {
        if (inputChannelId == InputDeviceKeyboard::Key::ModifierShiftL
            || inputChannelId == InputDeviceKeyboard::Key::ModifierShiftR)
        {
            io.KeyShift = inputChannel.IsActive();
        }
        else if (inputChannelId == InputDeviceKeyboard::Key::ModifierAltL
                 || inputChannelId == InputDeviceKeyboard::Key::ModifierAltR)
        {
            io.KeyAlt = inputChannel.IsActive();
        }
        else if (inputChannelId == InputDeviceKeyboard::Key::ModifierCtrlL
                 || inputChannelId == InputDeviceKeyboard::Key::ModifierCtrlR)
        {
            io.KeyCtrl = inputChannel.IsActive();
        }

        // Set Keydown Flag in ImGui Keys Array
        const int keyIndex = GetAzKeyIndex(inputChannelId);
        if (0 <= keyIndex  && keyIndex < AZ_ARRAY_SIZE(io.KeysDown))
        {
            io.KeysDown[keyIndex] = inputChannel.IsActive();
        }
    }

    // Handle Controller Inputs
    if (InputDeviceGamepad::IsGamepadDevice(inputDeviceId))
    {
        // Only pipe in Controller Nav Inputs when at least 1 of the two controller modes are enabled.
        if (m_controllerModeFlags)
        {
            const auto lyButtonToImGuiNav = s_lyInputToImGuiNavIndexMap.find(inputChannelId);
            if (lyButtonToImGuiNav != s_lyInputToImGuiNavIndexMap.end())
            {
                const ImGuiNavInput_ imGuiNavInput = lyButtonToImGuiNav->second;
                io.NavInputs[imGuiNavInput] = inputChannel.GetValue();
            }
        }

        //Switch menu bar display only if two buttons are pressed at the same time
        if (inputChannelId == InputDeviceGamepad::Button::L1)
        {
            if (inputChannel.IsStateBegan())
            {
                m_button1Pressed = true;
            }
            if (inputChannel.IsStateEnded())
            {
                m_button1Pressed = false;
                m_menuBarStatusChanged = false;
            }
        }
        if (inputChannelId == InputDeviceGamepad::Button::R1)
        {
            if (inputChannel.IsStateBegan())
            {
                m_button2Pressed = true;
            }
            if (inputChannel.IsStateEnded())
            {
                m_button2Pressed = false;
                m_menuBarStatusChanged = false;
            }
        }
        if (!m_menuBarStatusChanged && m_button1Pressed && m_button2Pressed)
        {
            ToggleThroughImGuiVisibleState();
        }

        // If we have the Discrete Input Mode Enabled.. and we are in the Visible State, then consume input here
        if (m_enableDiscreteInputMode && m_clientMenuBarState == DisplayState::Visible)
        {
            return true;
        }

        return false;
    }

    // Handle Mouse Inputs
    if (InputDeviceMouse::IsMouseDevice(inputDeviceId))
    {
        const int mouseButtonIndex = GetAzMouseButtonIndex(inputChannelId);
        if (0 <= mouseButtonIndex && mouseButtonIndex < AZ_ARRAY_SIZE(io.MouseDown))
        {
            io.MouseDown[mouseButtonIndex] = inputChannel.IsActive();
        }
        else if (inputChannelId == InputDeviceMouse::Movement::Z)
        {
            io.MouseWheel = inputChannel.GetValue() / static_cast<float>(IMGUI_WHEEL_DELTA);
        }
    }

    // Handle Touch Inputs
    if (InputDeviceTouch::IsTouchDevice(inputDeviceId))
    {
        const int touchIndex = GetAzTouchIndex(inputChannelId);
        if (0 <= touchIndex && touchIndex < AZ_ARRAY_SIZE(io.MouseDown))
        {
            io.MouseDown[touchIndex] = inputChannel.IsActive();
        }

        if (touchIndex == 0)
        {
            const AzFramework::InputChannel::PositionData2D* positionData2D = inputChannel.GetCustomData<AzFramework::InputChannel::PositionData2D>();
            if (positionData2D)
            {
                m_lastPrimaryTouchPosition[0] = positionData2D->m_normalizedPosition.GetX() * m_lastRenderResolution.x;
                m_lastPrimaryTouchPosition[1] = positionData2D->m_normalizedPosition.GetY() * m_lastRenderResolution.y;
                m_useLastPrimaryTouchPosition = true;
            }
        }
    }

    // Handle Virtual Keyboard Inputs
    if (InputDeviceVirtualKeyboard::IsVirtualKeyboardDevice(inputDeviceId))
    {
        if (inputChannelId == AzFramework::InputDeviceVirtualKeyboard::Command::EditEnter)
        {
            // Simulate the enter key being pressed
            io.KeysDown[GetAzKeyIndex(InputDeviceKeyboard::Key::EditEnter)] = true;
        }
    }

    if (m_clientMenuBarState == DisplayState::Visible
        || m_editorWindowState == DisplayState::Visible)
    {
        // If we have the Discrete Input Mode Enabled.. then consume the input here. 
        if (m_enableDiscreteInputMode)
        {
            return true;
        }
    }

    return false;
}

bool ImGuiManager::IsControllerSupportModeEnabled(ImGuiControllerModeFlags::FlagType controllerMode) const
{
    return !!(m_controllerModeFlags & controllerMode);
}

void ImGuiManager::EnableControllerSupportMode(ImGuiControllerModeFlags::FlagType controllerMode, bool enable)
{
    ImGui::ImGuiContextScope contextScope(m_imguiContext);

    if (enable)
    {
        // Enable - Add flag by or'ing in.
        m_controllerModeFlags |= controllerMode;
    }
    else
    {
        // Disable - Remove flag by and'ing the flag inverse
        m_controllerModeFlags &= ~controllerMode;
    }

    const bool controllerMouseEnabled = IsControllerSupportModeEnabled(ImGuiControllerModeFlags::Mouse);

    // Draw the ImGui Mouse cursor if either the hardware mouse is connected, or the controller mouse is enabled.
    ImGui::GetIO().MouseDrawCursor = m_hardwardeMouseConnected || controllerMouseEnabled;

    // Set or unset ImGui Config flags based on which modes are enabled. 
    if (controllerMouseEnabled)
    {
        ImGui::GetIO().ConfigFlags &= ~ImGuiConfigFlags_NavEnableGamepad;
        ImGui::GetIO().ConfigFlags &= ~ImGuiConfigFlags_NavEnableKeyboard;
    }
    else if (IsControllerSupportModeEnabled(ImGuiControllerModeFlags::Contextual))
    {
        ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
        ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    }
}

bool ImGuiManager::OnInputTextEventFiltered(const AZStd::string& textUTF8)
{
    ImGui::ImGuiContextScope contextScope(m_imguiContext);

    ImGuiIO& io = ImGui::GetIO();
    io.AddInputCharactersUTF8(textUTF8.c_str());

    if (textUTF8 == "\b" && !io.KeysDown[GetAzKeyIndex(InputDeviceKeyboard::Key::EditBackspace)])
    {
        // Simulate the backspace key being pressed
        io.KeysDown[GetAzKeyIndex(InputDeviceKeyboard::Key::EditBackspace)] = true;
        m_simulateBackspaceKeyPressed = true;
    }

    return io.WantTextInput && m_clientMenuBarState == DisplayState::Visible;;
}

void ImGuiManager::ToggleThroughImGuiVisibleState()
{
    ImGui::ImGuiContextScope contextScope(m_imguiContext);

    switch (m_clientMenuBarState)
    {
        case DisplayState::Hidden:
            m_clientMenuBarState = DisplayState::Visible;
            
            // Draw the ImGui Mouse cursor if either the hardware mouse is connected, or the controller mouse is enabled.
            ImGui::GetIO().MouseDrawCursor = m_hardwardeMouseConnected || IsControllerSupportModeEnabled(ImGuiControllerModeFlags::Mouse);

            // Disable system cursor if it's in editor and it's not editor game mode
            if (gEnv->IsEditor() && !gEnv->IsEditorGameMode())
            {
                // Constrain and hide the system cursor
                AzFramework::InputSystemCursorRequestBus::Event(
                    AzFramework::InputDeviceMouse::Id, &AzFramework::InputSystemCursorRequests::SetSystemCursorState,
                    AzFramework::SystemCursorState::ConstrainedAndHidden);

                // Disable discrete input mode
                m_enableDiscreteInputMode = false;
            }

            // get window size if it wasn't initialized
            InitWindowSize();
            break;

        case DisplayState::Visible:
            m_clientMenuBarState = DisplayState::VisibleNoMouse;
            ImGui::GetIO().MouseDrawCursor = false;

            if (m_enableDiscreteInputMode)
            {
                // if we ARE Enabling the Discrete Input Mode, then we want to bail here, if not, we want to just fall below to the default case. 
                //    no worries on setting m_clientMenuBarState twice..
                break;
            }

        default:
            m_clientMenuBarState = DisplayState::Hidden;

            // Enable system cursor if it's in editor and it's not editor game mode
            if (gEnv->IsEditor() && !gEnv->IsEditorGameMode())
            {
                // unconstrain and show the system cursor
                AzFramework::InputSystemCursorRequestBus::Event(
                    AzFramework::InputDeviceMouse::Id, &AzFramework::InputSystemCursorRequests::SetSystemCursorState,
                    AzFramework::SystemCursorState::UnconstrainedAndVisible);
            }
            break;
    }

    m_menuBarStatusChanged = true;
    m_setEnabledEvent.Signal(m_clientMenuBarState == DisplayState::Hidden);
}

void ImGuiManager::RenderImGuiBuffers(const ImVec2& scaleRects)
{
    ImGui::ImGuiContextScope contextScope(m_imguiContext);

    // Trigger all listeners to run their updates
    EBUS_EVENT(ImGuiUpdateListenerBus, OnImGuiUpdate);

    // Run imgui's internal render and retrieve resulting draw data
    ImGui::Render();
    ImDrawData* drawData = ImGui::GetDrawData();
    if (!drawData)
    {
        return;
    }

    // Supply Scale Rects
    drawData->ScaleClipRects(scaleRects);

    //@rky: Only render the main ImGui if it is visible
    if (m_clientMenuBarState != DisplayState::Hidden)
    {
        OtherActiveImGuiRequestBus::Broadcast(&OtherActiveImGuiRequestBus::Events::RenderImGuiBuffers, *drawData);
    }
}

void ImGuiManager::OnWindowResized(uint32_t width, uint32_t height)
{
    m_windowSize.m_width = width;
    m_windowSize.m_height = height;
}

void ImGuiManager::InitWindowSize()
{
    // We only need to initialize the window size by querying the window the first time.
    // After that we will get OnWindowResize notifications
    if (!m_overridingWindowSize && !AzFramework::WindowNotificationBus::Handler::BusIsConnected())
    {
        AzFramework::NativeWindowHandle windowHandle = nullptr;
        AzFramework::WindowSystemRequestBus::BroadcastResult(windowHandle, &AzFramework::WindowSystemRequestBus::Events::GetDefaultWindowHandle);

        if (windowHandle)
        {
            AzFramework::WindowRequestBus::EventResult(m_windowSize, windowHandle, &AzFramework::WindowRequestBus::Events::GetClientAreaSize);
            AzFramework::WindowNotificationBus::Handler::BusConnect(windowHandle);
        }
    }
}


//////  IMGUI CVAR STUFF /////////

namespace ImGuiCVARNames
{
    static const char* s_imgui_AutoEnableComponents_Name =          "imgui_AutoEnableComponents";
    static const char* s_imgui_DiscreteInputMode_Name =             "imgui_DiscreteInputMode";
    static const char* s_imgui_EnableAssetExplorer_Name =           "imgui_EnableAssetExplorer";
    static const char* s_imgui_EnableCameraMonitor_Name =           "imgui_EnableCameraMonitor";
    static const char* s_imgui_EnableEntityOutliner_Name =          "imgui_EnableEntityOutliner";
    static const char* s_imgui_EnableImGui_Name =                   "imgui_EnableImGui";
    static const char* s_imgui_EnableController_Name =              "imgui_EnableController";
    static const char* s_imgui_EnableControllerMouse_Name =         "imgui_EnableControllerMouse";
    static const char* s_imgui_ControllerMouseSensitivity_Name =    "imgui_ControllerMouseSensitivity";
}

void OnAutoEnableComponentsCBFunc(ICVar* pArgs)
{
    std::string token;
    std::istringstream tokenStream(pArgs->GetString());
    while (std::getline(tokenStream, token, ','))
    {
        AZStd::string enableSearchString = token.c_str();
        ImGui::ImGuiEntityOutlinerRequestBus::Broadcast(&ImGui::IImGuiEntityOutlinerRequests::AddAutoEnableSearchString, enableSearchString);
    }
}

void OnEnableEntityOutlinerCBFunc(ICVar* pArgs)
{
    ImGui::ImGuiEntityOutlinerRequestBus::Broadcast(&ImGui::IImGuiEntityOutlinerRequests::SetEnabled, pArgs->GetIVal() != 0);
}

void OnEnableAssetExplorerCBFunc(ICVar* pArgs)
{
    ImGui::ImGuiAssetExplorerRequestBus::Broadcast(&ImGui::IImGuiAssetExplorerRequests::SetEnabled, pArgs->GetIVal() != 0);
}

void OnEnableCameraMonitorCBFunc(ICVar* pArgs)
{
    ImGui::ImGuiCameraMonitorRequestBus::Broadcast(&ImGui::IImGuiCameraMonitorRequests::SetEnabled, pArgs->GetIVal() != 0);
}

void OnShowImGuiCBFunc(ICVar* pArgs)
{
    ImGui::ImGuiManagerBus::Broadcast(&ImGui::IImGuiManager::SetClientMenuBarState, pArgs->GetIVal() != 0 ? ImGui::DisplayState::Visible : ImGui::DisplayState::Hidden);
}

void OnDiscreteInputModeCBFunc(ICVar* pArgs)
{
    ImGui::ImGuiManagerBus::Broadcast(&ImGui::IImGuiManager::SetEnableDiscreteInputMode, pArgs->GetIVal() != 0 );
}

void OnEnableControllerCBFunc(ICVar* pArgs)
{
    ImGui::ImGuiManagerBus::Broadcast(&ImGui::IImGuiManager::EnableControllerSupportMode, ImGuiControllerModeFlags::Contextual, (pArgs->GetIVal() != 0));
}

void OnEnableControllerMouseCBFunc(ICVar* pArgs)
{
    ImGui::ImGuiManagerBus::Broadcast(&ImGui::IImGuiManager::EnableControllerSupportMode, ImGuiControllerModeFlags::Mouse, (pArgs->GetIVal() != 0));
}

void OnControllerMouseSensitivityCBFunc(ICVar* pArgs)
{
    ImGui::ImGuiManagerBus::Broadcast(&ImGui::IImGuiManager::SetControllerMouseSensitivity, pArgs->GetFVal());
}


void ImGuiManager::RegisterImGuiCVARs()
{
    // These are already checked before we enter this function, but lets make double sure and prevent crashes.
    if (!gEnv || !gEnv->pConsole)
    {
        return;
    }

    // We should also just make sure we aren't registering these twice. Check by just checking the first one.
    if (gEnv->pConsole->GetCVar(ImGuiCVARNames::s_imgui_EnableImGui_Name) != nullptr)
    {
        return;
    }

    // Register CVARs
    gEnv->pConsole->RegisterString(ImGuiCVARNames::s_imgui_AutoEnableComponents_Name, 0, VF_DEV_ONLY, CVARHELP("Enable ImGui Components by search string, as they are added to the Scene. Comma delimited list."), OnAutoEnableComponentsCBFunc);
    gEnv->pConsole->RegisterInt(ImGuiCVARNames::s_imgui_EnableImGui_Name, 0, VF_DEV_ONLY, CVARHELP("Enable ImGui on Startup"), OnShowImGuiCBFunc);
    gEnv->pConsole->RegisterInt(ImGuiCVARNames::s_imgui_EnableEntityOutliner_Name, 0, VF_DEV_ONLY, CVARHELP("Enable ImGui Entity Outliner on Startup"), OnEnableEntityOutlinerCBFunc);
    gEnv->pConsole->RegisterInt(ImGuiCVARNames::s_imgui_EnableCameraMonitor_Name, 0, VF_DEV_ONLY, CVARHELP("Enable ImGui Camera Monitor on Startup"), OnEnableCameraMonitorCBFunc);
    gEnv->pConsole->RegisterInt(ImGuiCVARNames::s_imgui_EnableAssetExplorer_Name, 0, VF_DEV_ONLY, CVARHELP("Enable ImGui Asset Explorer on Startup"), OnEnableAssetExplorerCBFunc);
    gEnv->pConsole->RegisterInt(ImGuiCVARNames::s_imgui_DiscreteInputMode_Name, 0, VF_DEV_ONLY, CVARHELP("Enable ImGui Discrete Input Mode, adds a 2nd Visibility Mode, with the 1st having input going toward ImGui and the 2nd having input going toward the game. If not set, Input will go to both ImGui and the game when ImGui is enabled."), OnDiscreteInputModeCBFunc);
    // Enable the Contextual Controller support by default when the hardware mouse is not detected.
    gEnv->pConsole->RegisterInt(ImGuiCVARNames::s_imgui_EnableController_Name, (m_hardwardeMouseConnected ? 0 : 1), VF_DEV_ONLY, CVARHELP("Enable ImGui Controller support. Default to Off on PC, On on Console."), OnEnableControllerCBFunc);
    gEnv->pConsole->RegisterInt(ImGuiCVARNames::s_imgui_EnableControllerMouse_Name, 0, VF_DEV_ONLY, CVARHELP("Enable ImGui Controller Mouse support. Default to Off on PC, On on Console."), OnEnableControllerMouseCBFunc);
    gEnv->pConsole->RegisterFloat(ImGuiCVARNames::s_imgui_ControllerMouseSensitivity_Name, 5.0f, VF_DEV_ONLY, CVARHELP("ImGui Controller Mouse Sensitivty. Frame Multiplier for stick mouse sensitivity"), OnControllerMouseSensitivityCBFunc);

    // Init CVARs to current values
    OnAutoEnableComponentsCBFunc(gEnv->pConsole->GetCVar(ImGuiCVARNames::s_imgui_AutoEnableComponents_Name));
    OnShowImGuiCBFunc(gEnv->pConsole->GetCVar(ImGuiCVARNames::s_imgui_EnableImGui_Name));
    OnEnableAssetExplorerCBFunc(gEnv->pConsole->GetCVar(ImGuiCVARNames::s_imgui_EnableAssetExplorer_Name));
    OnEnableCameraMonitorCBFunc(gEnv->pConsole->GetCVar(ImGuiCVARNames::s_imgui_EnableCameraMonitor_Name));
    OnEnableEntityOutlinerCBFunc(gEnv->pConsole->GetCVar(ImGuiCVARNames::s_imgui_EnableEntityOutliner_Name));
    OnDiscreteInputModeCBFunc(gEnv->pConsole->GetCVar(ImGuiCVARNames::s_imgui_DiscreteInputMode_Name));
    OnEnableControllerCBFunc(gEnv->pConsole->GetCVar(ImGuiCVARNames::s_imgui_EnableController_Name));
    OnEnableControllerMouseCBFunc(gEnv->pConsole->GetCVar(ImGuiCVARNames::s_imgui_EnableControllerMouse_Name));
    OnControllerMouseSensitivityCBFunc(gEnv->pConsole->GetCVar(ImGuiCVARNames::s_imgui_ControllerMouseSensitivity_Name));
}
//////  IMGUI CVAR STUFF /////////

void AddMenuItemHelper(bool& control, const char* show, const char* hide)
{
    if (control)
    {
        if (ImGui::MenuItem(hide))
        {
            control = false;
        }
    }
    else if (ImGui::MenuItem(show))
    {
        control = true;
    }
}

#endif // ~IMGUI_ENABLED
