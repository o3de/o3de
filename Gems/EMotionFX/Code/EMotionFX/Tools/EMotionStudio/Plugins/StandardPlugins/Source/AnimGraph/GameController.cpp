/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

// include required headers
#include "GameController.h"

#if AZ_TRAIT_EMOTIONFX_HAS_GAME_CONTROLLER

#include <MCore/Source/LogManager.h>
#include <AzCore/std/string/conversions.h>


// joystick enum callback
BOOL CALLBACK GameController::EnumJoysticksCallback(const DIDEVICEINSTANCE* pdidInstance, void* pContext)
{
    GameController* manager = static_cast<GameController*>(pContext);

    // store the name
    AZStd::to_string(manager->m_deviceInfo.m_name, pdidInstance->tszProductName);

    // Skip anything other than the perferred Joystick device as defined by the control panel.
    // Instead you could store all the enumerated Joysticks and let the user pick.
    if (manager->m_enumContext.m_prefJoystickConfigValid && IsEqualGUID(pdidInstance->guidInstance, manager->m_enumContext.m_prefJoystickConfig->guidInstance) == false)
    {
        return DIENUM_CONTINUE;
    }

    // Obtain an interface to the enumerated Joystick.
    HRESULT result = manager->m_directInput->CreateDevice(pdidInstance->guidInstance, &manager->m_joystick, nullptr);

    // If it failed, then we can't use this Joystick. (Maybe the user unplugged
    // it while we were in the middle of enumerating it.)
    if (FAILED(result))
    {
        return DIENUM_CONTINUE;
    }

    // Stop enumeration. Note: we're just taking the first Joystick we get. You
    // could store all the enumerated Joysticks and let the user pick.
    return DIENUM_STOP;
}


// check what elements are on the controller (buttons, sliders, etc)
BOOL CALLBACK GameController::EnumObjectsCallback(const DIDEVICEOBJECTINSTANCE* pdidoi, void* pContext)
{
    GameController* manager = static_cast<GameController*>(pContext);

    // For axes that are returned, set the DIPROP_RANGE property for the
    // enumerated axis in order to scale min/max values.
    if (pdidoi->dwType & DIDFT_AXIS)
    {
        DIPROPRANGE diprg;
        diprg.diph.dwSize       = sizeof(DIPROPRANGE);
        diprg.diph.dwHeaderSize = sizeof(DIPROPHEADER);
        diprg.diph.dwHow        = DIPH_BYID;
        diprg.diph.dwObj        = pdidoi->dwType; // Specify the enumerated axis
        diprg.lMin              = -1000;
        diprg.lMax              = +1000;

        // Set the range for the axis
        if (FAILED(manager->m_joystick->SetProperty(DIPROP_RANGE, &diprg.diph)))
        {
            return DIENUM_STOP;
        }
    }

    if (pdidoi->guidType == GUID_XAxis)
    {
        AZStd::to_string(manager->m_deviceElements[ ELEM_POS_X ].m_name, pdidoi->tszName);
        manager->m_deviceElements[ ELEM_POS_X ].m_present             = true;
        manager->m_deviceElements[ ELEM_POS_X ].m_value               = 0.0f;
        manager->m_deviceElements[ ELEM_POS_X ].m_type                = ELEMTYPE_AXIS;
        manager->m_deviceInfo.m_numAxes++;
    }

    if (pdidoi->guidType == GUID_YAxis)
    {
        AZStd::to_string(manager->m_deviceElements[ ELEM_POS_Y ].m_name, pdidoi->tszName);
        manager->m_deviceElements[ ELEM_POS_Y ].m_present             = true;
        manager->m_deviceElements[ ELEM_POS_Y ].m_value               = 0.0f;
        manager->m_deviceElements[ ELEM_POS_Y ].m_type                = ELEMTYPE_AXIS;
        manager->m_deviceInfo.m_numAxes++;
    }

    if (pdidoi->guidType == GUID_ZAxis)
    {
        AZStd::to_string(manager->m_deviceElements[ ELEM_POS_Z ].m_name, pdidoi->tszName);
        manager->m_deviceElements[ ELEM_POS_Z ].m_present             = true;
        manager->m_deviceElements[ ELEM_POS_Z ].m_value               = 0.0f;
        manager->m_deviceElements[ ELEM_POS_Z ].m_type                = ELEMTYPE_AXIS;
        manager->m_deviceInfo.m_numAxes++;
    }

    if (pdidoi->guidType == GUID_RxAxis)
    {
        AZStd::to_string(manager->m_deviceElements[ ELEM_ROT_X ].m_name, pdidoi->tszName);
        manager->m_deviceElements[ ELEM_ROT_X ].m_present             = true;
        manager->m_deviceElements[ ELEM_ROT_X ].m_value               = 0.0f;
        manager->m_deviceElements[ ELEM_ROT_X ].m_type                = ELEMTYPE_AXIS;
        manager->m_deviceInfo.m_numAxes++;
    }

    if (pdidoi->guidType == GUID_RyAxis)
    {
        AZStd::to_string(manager->m_deviceElements[ ELEM_ROT_Y ].m_name, pdidoi->tszName);
        manager->m_deviceElements[ ELEM_ROT_Y ].m_present             = true;
        manager->m_deviceElements[ ELEM_ROT_Y ].m_value               = 0.0f;
        manager->m_deviceElements[ ELEM_ROT_Y ].m_type                = ELEMTYPE_AXIS;
        manager->m_deviceInfo.m_numAxes++;
    }

    if (pdidoi->guidType == GUID_RzAxis)
    {
        AZStd::to_string(manager->m_deviceElements[ ELEM_ROT_Z ].m_name, pdidoi->tszName);
        manager->m_deviceElements[ ELEM_ROT_Z ].m_present             = true;
        manager->m_deviceElements[ ELEM_ROT_Z ].m_value               = 0.0f;
        manager->m_deviceElements[ ELEM_ROT_Z ].m_type                = ELEMTYPE_AXIS;
        manager->m_deviceInfo.m_numAxes++;
    }

    // a slider
    if (pdidoi->guidType == GUID_Slider)
    {
        if (manager->m_deviceInfo.m_numSliders == 0)
        {
            AZStd::to_string(manager->m_deviceElements[ ELEM_SLIDER_1 ].m_name, pdidoi->tszName);
            manager->m_deviceElements[ ELEM_SLIDER_1 ].m_present              = true;
            manager->m_deviceElements[ ELEM_SLIDER_1 ].m_value                = 0.0f;
            manager->m_deviceElements[ ELEM_SLIDER_1 ].m_type                 = ELEMTYPE_SLIDER;
        }
        else
        {
            AZStd::to_string(manager->m_deviceElements[ ELEM_SLIDER_2 ].m_name, pdidoi->tszName);
            manager->m_deviceElements[ ELEM_SLIDER_2 ].m_present              = true;
            manager->m_deviceElements[ ELEM_SLIDER_2 ].m_value                = 0.0f;
            manager->m_deviceElements[ ELEM_SLIDER_2 ].m_type                 = ELEMTYPE_SLIDER;
        }

        manager->m_deviceInfo.m_numSliders++;
    }

    // a POV
    if (pdidoi->guidType == GUID_POV)
    {
        const uint32 povIndex = manager->m_deviceInfo.m_numPoVs;
        AZStd::to_string(manager->m_deviceElements[ ELEM_POV_1 + povIndex ].m_name, pdidoi->tszName);
        manager->m_deviceElements[ ELEM_POV_1 + povIndex ].m_present              = true;
        manager->m_deviceElements[ ELEM_POV_1 + povIndex ].m_value                = 0.0f;
        manager->m_deviceElements[ ELEM_POV_1 + povIndex ].m_type                 = ELEMTYPE_POV;
        manager->m_deviceInfo.m_numPoVs++;
    }

    // a button
    if (pdidoi->guidType == GUID_Button)
    {
        manager->m_deviceInfo.m_numButtons++;
    }

    return DIENUM_CONTINUE;
}


// init the game controller manager
bool GameController::Init(HWND hWnd)
{
    // clean up existing stuff
    Shutdown();

    // reinit
    if (FAILED(InitDirectInput(hWnd)))
    {
        m_valid = false;
        return false;
    }

    // update to reset the values
    Update();
    return true;
}


// init direct input
HRESULT GameController::InitDirectInput(HWND hWnd)
{
    HRESULT result;

    // reset the device info
    m_deviceInfo.m_name       = "";
    m_deviceInfo.m_numAxes    = 0;
    m_deviceInfo.m_numButtons = 0;
    m_deviceInfo.m_numPoVs    = 0;
    m_deviceInfo.m_numSliders = 0;

    uint32 i;
    for (i = 0; i < NUM_ELEMENTS; ++i)
    {
        m_deviceElements[i].m_present = false;
        m_deviceElements[i].m_value   = 0.0f;
    }

    // register with the DirectInput subsystem and get a pointer to a IDirectInput interface we can use
    result = DirectInput8Create(GetModuleHandle(nullptr), DIRECTINPUT_VERSION, IID_IDirectInput8, ( VOID** )&m_directInput, nullptr);
    if (FAILED(result))
    {
        m_valid = false;
        return result;
    }

    DIJOYCONFIG prefJoystickConfig = { 0 };
    memset(&prefJoystickConfig, 0, sizeof(DIJOYCONFIG));
    prefJoystickConfig.dwSize = sizeof(DIJOYCONFIG);

    m_enumContext.m_prefJoystickConfig        = &prefJoystickConfig;
    m_enumContext.m_prefJoystickConfigValid   = false;

    IDirectInputJoyConfig8* joystickConfig = nullptr;
    result = m_directInput->QueryInterface(IID_IDirectInputJoyConfig8, (void**)&joystickConfig);
    if (FAILED(result))
    {
        m_valid = false;
        return result;
    }

    result = joystickConfig->GetConfig(0, &prefJoystickConfig, DIJC_GUIDINSTANCE);
    if (SUCCEEDED(result)) // this function is expected to fail if no joystick is attached
    {
        m_enumContext.m_prefJoystickConfigValid = true;
    }

    if (joystickConfig)
    {
        joystickConfig->Release();
        joystickConfig = nullptr;
    }

    // look for a simple Joystick we can use for this sample program.
    result = m_directInput->EnumDevices(DI8DEVCLASS_GAMECTRL, EnumJoysticksCallback, this, DIEDFL_ATTACHEDONLY);
    if (FAILED(result))
    {
        m_valid = false;
        return result;
    }

    // make sure we got a Joystick
    if (m_joystick == nullptr)
    {
        // No joystick found.
        m_valid = false;
        return S_OK;
    }

    // Set the data format to "simple Joystick" - a predefined data format
    //
    // A data format specifies which controls on a device we are interested in,
    // and how they should be reported. This tells DInput that we will be
    // passing a DIJOYSTATE2 structure to IDirectInputDevice::GetDeviceState().
    result = m_joystick->SetDataFormat(&c_dfDIJoystick2);
    if (FAILED(result))
    {
        m_valid = false;
        return result;
    }

    // Set the cooperative level to let DInput know how this device should
    // interact with the system and with other DInput applications.
    if (hWnd)
    {
        result = m_joystick->SetCooperativeLevel(hWnd, DISCL_EXCLUSIVE | DISCL_BACKGROUND);
        if (FAILED(result))
        {
            m_valid = false;
            return result;
        }
    }

    // Enumerate the Joystick objects. The callback function enabled user
    // interface elements for objects that are found, and sets the min/max
    // values property for discovered axes.
    result = m_joystick->EnumObjects(EnumObjectsCallback, (VOID*)this, DIDFT_ALL);
    if (FAILED(result))
    {
        m_valid = false;
        return result;
    }

    // acquire the joystick
    m_joystick->Acquire();

    // display the device info
    MCore::LogDetailedInfo("- Controller = %s", m_deviceInfo.m_name.c_str());
    MCore::LogDetailedInfo("   + Num buttons = %d", m_deviceInfo.m_numButtons);
    MCore::LogDetailedInfo("   + Num axes    = %d", m_deviceInfo.m_numAxes);
    MCore::LogDetailedInfo("   + Num sliders = %d", m_deviceInfo.m_numSliders);
    MCore::LogDetailedInfo("   + Num POVs    = %d", m_deviceInfo.m_numPoVs);

    // display all elements
    uint32 numPresentElements = 0;
    for (i = 0; i < NUM_ELEMENTS; ++i)
    {
        if (m_deviceElements[i].m_present == false)
        {
            continue;
        }

        numPresentElements++;
        MCore::LogDetailedInfo("   + Element #%d  = %s", numPresentElements, m_deviceElements[i].m_name.c_str());
    }

    m_valid = true;
    return S_OK;
}


// shutdown
void GameController::Shutdown()
{
    // unacquire the device one last time just in case
    if (m_joystick)
    {
        m_joystick->Unacquire();
        m_joystick->Release();
        m_joystick = nullptr;
    }

    // release any DirectInput objects
    if (m_directInput)
    {
        m_directInput->Release();
        m_directInput = nullptr;
    }
}


// calibrate
void GameController::Calibrate()
{
    m_deviceElements[ELEM_POS_X].m_calibrationValue       = -(m_joystickState.lX / 1000.0f);
    m_deviceElements[ELEM_POS_Y].m_calibrationValue       = -(m_joystickState.lY / 1000.0f);
    m_deviceElements[ELEM_POS_Z].m_calibrationValue       = -(m_joystickState.lZ / 1000.0f);
    m_deviceElements[ELEM_ROT_X].m_calibrationValue       = -(m_joystickState.lRx / 1000.0f);
    m_deviceElements[ELEM_ROT_Y].m_calibrationValue       = -(m_joystickState.lRy / 1000.0f);
    m_deviceElements[ELEM_ROT_Z].m_calibrationValue       = -(m_joystickState.lRz / 1000.0f);
    m_deviceElements[ELEM_SLIDER_1].m_calibrationValue    = -(m_joystickState.rglSlider[0] / 1000.0f);
    m_deviceElements[ELEM_SLIDER_2].m_calibrationValue    = -(m_joystickState.rglSlider[1] / 1000.0f);
}


// update the controller
bool GameController::Update()
{
    if (m_joystick == nullptr)
    {
        m_valid = false;
        return false;
    }

    // poll the device to read the current state
    HRESULT result = m_joystick->Poll();
    if (FAILED(result))
    {
        // DInput is telling us that the input stream has been
        // interrupted. We aren't tracking any state between polls, so
        // we don't have any special reset that needs to be done. We
        // just re-acquire and try again.
        result = m_joystick->Acquire();
        while (result == DIERR_INPUTLOST)
        {
            result = m_joystick->Acquire();
        }

        // reset all buttons
        const uint32 numButtons = GetNumButtons();
        for (uint32 i = 0; i < numButtons; ++i)
        {
            SetButtonPressed(static_cast<uint8>(i), false);
        }

        // hr may be DIERR_OTHERAPPHASPRIO or other errors.  This
        // may occur when the app is minimized or in the process of
        // switching, so just try again later
        if (result == DIERR_OTHERAPPHASPRIO)
        {
            m_valid = true;
            return true;
        }

        if (FAILED(result))
        {
            m_valid = false;
            return false;
        }
    }

    // Get the input's device state
    result = m_joystick->GetDeviceState(sizeof(DIJOYSTATE2), &m_joystickState);
    if (FAILED(result))
    {
        m_valid = false;
        return false; // The device should have been acquired during the Poll()
    }

    // update the values of the elements
    m_deviceElements[ELEM_POS_X].m_value      = m_joystickState.lX / 1000.0f;
    m_deviceElements[ELEM_POS_Y].m_value      = m_joystickState.lY / 1000.0f;
    m_deviceElements[ELEM_POS_Z].m_value      = m_joystickState.lZ / 1000.0f;
    m_deviceElements[ELEM_ROT_X].m_value      = m_joystickState.lRx / 1000.0f;
    m_deviceElements[ELEM_ROT_Y].m_value      = m_joystickState.lRy / 1000.0f;
    m_deviceElements[ELEM_ROT_Z].m_value      = m_joystickState.lRz / 1000.0f;
    m_deviceElements[ELEM_SLIDER_1].m_value   = m_joystickState.rglSlider[0] / 1000.0f;
    m_deviceElements[ELEM_SLIDER_2].m_value   = m_joystickState.rglSlider[1] / 1000.0f;

    if (m_joystickState.rgdwPOV[0] == MCORE_INVALIDINDEX32)
    {
        m_deviceElements[ELEM_POV_1].m_value = 0.0f;
    }
    else
    {
        m_deviceElements[ELEM_POV_1].m_value = (m_joystickState.rgdwPOV[0] / 100.0f) / 360.0f;
    }
    if (m_joystickState.rgdwPOV[1] == MCORE_INVALIDINDEX32)
    {
        m_deviceElements[ELEM_POV_2].m_value = 0.0f;
    }
    else
    {
        m_deviceElements[ELEM_POV_2].m_value = (m_joystickState.rgdwPOV[1] / 100.0f) / 360.0f;
    }
    if (m_joystickState.rgdwPOV[2] == MCORE_INVALIDINDEX32)
    {
        m_deviceElements[ELEM_POV_3].m_value = 0.0f;
    }
    else
    {
        m_deviceElements[ELEM_POV_3].m_value = (m_joystickState.rgdwPOV[2] / 100.0f) / 360.0f;
    }
    if (m_joystickState.rgdwPOV[3] == MCORE_INVALIDINDEX32)
    {
        m_deviceElements[ELEM_POV_4].m_value = 0.0f;
    }
    else
    {
        m_deviceElements[ELEM_POV_4].m_value = (m_joystickState.rgdwPOV[3] / 100.0f) / 360.0f;
    }

    // apply the dead zone
    const float minValue    = m_deadZone;
    const float maxValue    = 1.0f;
    const float range       = maxValue - minValue;

    // iterate through all axes
    for (uint32 i = 0; i < 8; ++i)
    {
        // get the current normalized value
        float value = m_deviceElements[i].m_value;

        // ignore all values that are smaller than the dead zone
        if (value > -m_deadZone && value < m_deadZone)
        {
            m_deviceElements[i].m_value = 0.0f;
        }
        else
        {
            // check if the value is negated, get the absolute in this case and remember it
            bool negativeValue = false;
            if (value < 0.0f)
            {
                negativeValue = true;
                value = -value;
            }

            // calculate the value in the new range excluding the dead zone range
            const float newValue = (value - m_deadZone) / range;

            // set it back in normal or negated version
            m_deviceElements[i].m_value = negativeValue == false ? newValue : -newValue;
        }
    }

    m_valid = true;
    return true;
}


void GameController::LogError(HRESULT value, [[maybe_unused]] const char* text)
{
    // only log errors
    if (FAILED(value) == false)
    {
        return;
    }

    /*  switch (value)
        {
            case DIERR_INPUTLOST:               { MCore::LogError("%s: %s", text, "DIERR_INPUTLOST"); break; }
            case DIERR_INVALIDPARAM:            { MCore::LogError("%s: %s", text, "DIERR_INVALIDPARAM"); break; }
            case DIERR_NOINTERFACE:             { MCore::LogError("%s: %s", text, "DIERR_NOINTERFACE"); break; }
            case DIERR_NOTACQUIRED:             { MCore::LogError("%s: %s", text, "DIERR_NOTACQUIRED"); break; }
            case DIERR_NOTBUFFERED:             { MCore::LogError("%s: %s", text, "DIERR_NOTBUFFERED"); break; }
            case DIERR_NOTEXCLUSIVEACQUIRED:    { MCore::LogError("%s: %s", text, "DIERR_NOTEXCLUSIVEACQUIRED"); break; }
            case DIERR_NOTFOUND:                { MCore::LogError("%s: %s", text, "DIERR_NOTFOUND"); break; }
            case DIERR_NOTINITIALIZED:          { MCore::LogError("%s: %s", text, "DIERR_NOTINITIALIZED"); break; }
            case DIERR_OTHERAPPHASPRIO:         { MCore::LogError("%s: %s", text, "DIERR_OTHERAPPHASPRIO"); break; }
            case DIERR_UNPLUGGED:               { MCore::LogError("%s: %s", text, "DIERR_UNPLUGGED"); break; }
            case DIERR_UNSUPPORTED:             { MCore::LogError("%s: %s", text, "DIERR_UNSUPPORTED"); break; }
            default:                            MCore::LogError("Unkown error value.");
        }*/
}


const char* GameController::GetElementEnumName(uint32 index)
{
    switch (index)
    {
    case ELEM_POS_X:
        return "Pos X";
    case ELEM_POS_Y:
        return "Pos Y";
    case ELEM_POS_Z:
        return "Pos Z";
    case ELEM_ROT_X:
        return "Rot X";
    case ELEM_ROT_Y:
        return "Rot Y";
    case ELEM_ROT_Z:
        return "Rot Z";
    case ELEM_SLIDER_1:
        return "Slider 1";
    case ELEM_SLIDER_2:
        return "Slider 2";
    case ELEM_POV_1:
        return "POV 1";
    case ELEM_POV_2:
        return "POV 2";
    case ELEM_POV_3:
        return "POV 3";
    case ELEM_POV_4:
        return "POV 4";
    }

    return "";
}


uint32 GameController::FindElementIDByName(const AZStd::string& elementEnumName)
{
    if (elementEnumName == "Pos X")
    {
        return ELEM_POS_X;
    }
    else if (elementEnumName == "Pos Y")
    {
        return ELEM_POS_Y;
    }
    else if (elementEnumName == "Pos Z")
    {
        return ELEM_POS_Z;
    }
    else if (elementEnumName == "Rot X")
    {
        return ELEM_ROT_X;
    }
    else if (elementEnumName == "Rot Y")
    {
        return ELEM_ROT_Y;
    }
    else if (elementEnumName == "Rot Z")
    {
        return ELEM_ROT_Z;
    }
    else if (elementEnumName == "Slider 1")
    {
        return ELEM_SLIDER_1;
    }
    else if (elementEnumName == "Slider 2")
    {
        return ELEM_SLIDER_2;
    }
    else if (elementEnumName == "POV 1")
    {
        return ELEM_POV_1;
    }
    else if (elementEnumName == "POV 2")
    {
        return ELEM_POV_2;
    }
    else if (elementEnumName == "POV 3")
    {
        return ELEM_POV_3;
    }
    else if (elementEnumName == "POV 4")
    {
        return ELEM_POV_4;
    }

    return MCORE_INVALIDINDEX32;
}

#endif

