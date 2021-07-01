/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

// include required headers
#include "GameController.h"

#if AZ_TRAIT_EMOTIONFX_HAS_GAME_CONTROLLER

#include <MCore/Source/LogManager.h>


// joystick enum callback
BOOL CALLBACK GameController::EnumJoysticksCallback(const DIDEVICEINSTANCE* pdidInstance, void* pContext)
{
    GameController* manager = static_cast<GameController*>(pContext);

    // store the name
    manager->mDeviceInfo.mName = pdidInstance->tszProductName;

    // Skip anything other than the perferred Joystick device as defined by the control panel.
    // Instead you could store all the enumerated Joysticks and let the user pick.
    if (manager->mEnumContext.mPrefJoystickConfigValid && IsEqualGUID(pdidInstance->guidInstance, manager->mEnumContext.mPrefJoystickConfig->guidInstance) == false)
    {
        return DIENUM_CONTINUE;
    }

    // Obtain an interface to the enumerated Joystick.
    HRESULT result = manager->mDirectInput->CreateDevice(pdidInstance->guidInstance, &manager->mJoystick, nullptr);

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
        if (FAILED(manager->mJoystick->SetProperty(DIPROP_RANGE, &diprg.diph)))
        {
            return DIENUM_STOP;
        }
    }

    if (pdidoi->guidType == GUID_XAxis)
    {
        manager->mDeviceElements[ ELEM_POS_X ].mName                = pdidoi->tszName;
        manager->mDeviceElements[ ELEM_POS_X ].mPresent             = true;
        manager->mDeviceElements[ ELEM_POS_X ].mValue               = 0.0f;
        //manager->mDeviceElements[ ELEM_POS_X ].mCalibrationValue  = 0.0f;
        manager->mDeviceElements[ ELEM_POS_X ].mType                = ELEMTYPE_AXIS;
        manager->mDeviceInfo.mNumAxes++;
    }

    if (pdidoi->guidType == GUID_YAxis)
    {
        manager->mDeviceElements[ ELEM_POS_Y ].mName                = pdidoi->tszName;
        manager->mDeviceElements[ ELEM_POS_Y ].mPresent             = true;
        manager->mDeviceElements[ ELEM_POS_Y ].mValue               = 0.0f;
        //manager->mDeviceElements[ ELEM_POS_Y ].mCalibrationValue  = 0.0f;
        manager->mDeviceElements[ ELEM_POS_Y ].mType                = ELEMTYPE_AXIS;
        manager->mDeviceInfo.mNumAxes++;
    }

    if (pdidoi->guidType == GUID_ZAxis)
    {
        manager->mDeviceElements[ ELEM_POS_Z ].mName                = pdidoi->tszName;
        manager->mDeviceElements[ ELEM_POS_Z ].mPresent             = true;
        manager->mDeviceElements[ ELEM_POS_Z ].mValue               = 0.0f;
        //manager->mDeviceElements[ ELEM_POS_Z ].mCalibrationValue  = 0.0f;
        manager->mDeviceElements[ ELEM_POS_Z ].mType                = ELEMTYPE_AXIS;
        manager->mDeviceInfo.mNumAxes++;
    }

    if (pdidoi->guidType == GUID_RxAxis)
    {
        manager->mDeviceElements[ ELEM_ROT_X ].mName                = pdidoi->tszName;
        manager->mDeviceElements[ ELEM_ROT_X ].mPresent             = true;
        manager->mDeviceElements[ ELEM_ROT_X ].mValue               = 0.0f;
        //manager->mDeviceElements[ ELEM_ROT_X ].mCalibrationValue  = 0.0f;
        manager->mDeviceElements[ ELEM_ROT_X ].mType                = ELEMTYPE_AXIS;
        manager->mDeviceInfo.mNumAxes++;
    }

    if (pdidoi->guidType == GUID_RyAxis)
    {
        manager->mDeviceElements[ ELEM_ROT_Y ].mName                = pdidoi->tszName;
        manager->mDeviceElements[ ELEM_ROT_Y ].mPresent             = true;
        manager->mDeviceElements[ ELEM_ROT_Y ].mValue               = 0.0f;
        //manager->mDeviceElements[ ELEM_ROT_Y ].mCalibrationValue  = 0.0f;
        manager->mDeviceElements[ ELEM_ROT_Y ].mType                = ELEMTYPE_AXIS;
        manager->mDeviceInfo.mNumAxes++;
    }

    if (pdidoi->guidType == GUID_RzAxis)
    {
        manager->mDeviceElements[ ELEM_ROT_Z ].mName                = pdidoi->tszName;
        manager->mDeviceElements[ ELEM_ROT_Z ].mPresent             = true;
        manager->mDeviceElements[ ELEM_ROT_Z ].mValue               = 0.0f;
        //manager->mDeviceElements[ ELEM_ROT_Z ].mCalibrationValue  = 0.0f;
        manager->mDeviceElements[ ELEM_ROT_Z ].mType                = ELEMTYPE_AXIS;
        manager->mDeviceInfo.mNumAxes++;
    }

    // a slider
    if (pdidoi->guidType == GUID_Slider)
    {
        if (manager->mDeviceInfo.mNumSliders == 0)
        {
            manager->mDeviceElements[ ELEM_SLIDER_1 ].mName                 = pdidoi->tszName;
            manager->mDeviceElements[ ELEM_SLIDER_1 ].mPresent              = true;
            manager->mDeviceElements[ ELEM_SLIDER_1 ].mValue                = 0.0f;
            //manager->mDeviceElements[ ELEM_SLIDER_1 ].mCalibrationValue   = 0.0f;
            manager->mDeviceElements[ ELEM_SLIDER_1 ].mType                 = ELEMTYPE_SLIDER;
        }
        else
        {
            manager->mDeviceElements[ ELEM_SLIDER_2 ].mName                 = pdidoi->tszName;
            manager->mDeviceElements[ ELEM_SLIDER_2 ].mPresent              = true;
            manager->mDeviceElements[ ELEM_SLIDER_2 ].mValue                = 0.0f;
            //manager->mDeviceElements[ ELEM_SLIDER_2 ].mCalibrationValue   = 0.0f;
            manager->mDeviceElements[ ELEM_SLIDER_2 ].mType                 = ELEMTYPE_SLIDER;
        }

        manager->mDeviceInfo.mNumSliders++;
    }

    // a POV
    if (pdidoi->guidType == GUID_POV)
    {
        const uint32 povIndex = manager->mDeviceInfo.mNumPOVs;
        manager->mDeviceElements[ ELEM_POV_1 + povIndex ].mName                 = pdidoi->tszName;
        manager->mDeviceElements[ ELEM_POV_1 + povIndex ].mPresent              = true;
        manager->mDeviceElements[ ELEM_POV_1 + povIndex ].mValue                = 0.0f;
        //manager->mDeviceElements[ ELEM_POV_1 + povIndex ].mCalibrationValue   = 0.0f;
        manager->mDeviceElements[ ELEM_POV_1 + povIndex ].mType                 = ELEMTYPE_POV;
        manager->mDeviceInfo.mNumPOVs++;
    }

    // a button
    if (pdidoi->guidType == GUID_Button)
    {
        manager->mDeviceInfo.mNumButtons++;
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
        mValid = false;
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
    mDeviceInfo.mName       = "";
    mDeviceInfo.mNumAxes    = 0;
    mDeviceInfo.mNumButtons = 0;
    mDeviceInfo.mNumPOVs    = 0;
    mDeviceInfo.mNumSliders = 0;

    uint32 i;
    for (i = 0; i < NUM_ELEMENTS; ++i)
    {
        mDeviceElements[i].mPresent = false;
        mDeviceElements[i].mValue   = 0.0f;
    }

    // register with the DirectInput subsystem and get a pointer to a IDirectInput interface we can use
    result = DirectInput8Create(GetModuleHandle(nullptr), DIRECTINPUT_VERSION, IID_IDirectInput8, ( VOID** )&mDirectInput, nullptr);
    if (FAILED(result))
    {
        mValid = false;
        return result;
    }

    DIJOYCONFIG prefJoystickConfig = { 0 };
    memset(&prefJoystickConfig, 0, sizeof(DIJOYCONFIG));
    prefJoystickConfig.dwSize = sizeof(DIJOYCONFIG);

    mEnumContext.mPrefJoystickConfig        = &prefJoystickConfig;
    mEnumContext.mPrefJoystickConfigValid   = false;

    IDirectInputJoyConfig8* joystickConfig = nullptr;
    result = mDirectInput->QueryInterface(IID_IDirectInputJoyConfig8, (void**)&joystickConfig);
    if (FAILED(result))
    {
        mValid = false;
        return result;
    }

    result = joystickConfig->GetConfig(0, &prefJoystickConfig, DIJC_GUIDINSTANCE);
    if (SUCCEEDED(result)) // this function is expected to fail if no joystick is attached
    {
        mEnumContext.mPrefJoystickConfigValid = true;
    }

    if (joystickConfig)
    {
        joystickConfig->Release();
        joystickConfig = nullptr;
    }

    // look for a simple Joystick we can use for this sample program.
    result = mDirectInput->EnumDevices(DI8DEVCLASS_GAMECTRL, EnumJoysticksCallback, this, DIEDFL_ATTACHEDONLY);
    if (FAILED(result))
    {
        mValid = false;
        return result;
    }

    // make sure we got a Joystick
    if (mJoystick == nullptr)
    {
        // No joystick found.
        mValid = false;
        return S_OK;
    }

    // Set the data format to "simple Joystick" - a predefined data format
    //
    // A data format specifies which controls on a device we are interested in,
    // and how they should be reported. This tells DInput that we will be
    // passing a DIJOYSTATE2 structure to IDirectInputDevice::GetDeviceState().
    result = mJoystick->SetDataFormat(&c_dfDIJoystick2);
    if (FAILED(result))
    {
        mValid = false;
        return result;
    }

    // Set the cooperative level to let DInput know how this device should
    // interact with the system and with other DInput applications.
    if (hWnd)
    {
        result = mJoystick->SetCooperativeLevel(hWnd, DISCL_EXCLUSIVE | DISCL_BACKGROUND);
        if (FAILED(result))
        {
            mValid = false;
            return result;
        }
    }

    // Enumerate the Joystick objects. The callback function enabled user
    // interface elements for objects that are found, and sets the min/max
    // values property for discovered axes.
    result = mJoystick->EnumObjects(EnumObjectsCallback, (VOID*)this, DIDFT_ALL);
    if (FAILED(result))
    {
        mValid = false;
        return result;
    }

    // acquire the joystick
    mJoystick->Acquire();

    // display the device info
    MCore::LogDetailedInfo("- Controller = %s", mDeviceInfo.mName.c_str());
    MCore::LogDetailedInfo("   + Num buttons = %d", mDeviceInfo.mNumButtons);
    MCore::LogDetailedInfo("   + Num axes    = %d", mDeviceInfo.mNumAxes);
    MCore::LogDetailedInfo("   + Num sliders = %d", mDeviceInfo.mNumSliders);
    MCore::LogDetailedInfo("   + Num POVs    = %d", mDeviceInfo.mNumPOVs);

    // display all elements
    uint32 numPresentElements = 0;
    for (i = 0; i < NUM_ELEMENTS; ++i)
    {
        if (mDeviceElements[i].mPresent == false)
        {
            continue;
        }

        numPresentElements++;
        MCore::LogDetailedInfo("   + Element #%d  = %s", numPresentElements, mDeviceElements[i].mName.c_str());
    }

    mValid = true;
    return S_OK;
}


// shutdown
void GameController::Shutdown()
{
    // unacquire the device one last time just in case
    if (mJoystick)
    {
        mJoystick->Unacquire();
        mJoystick->Release();
        mJoystick = nullptr;
    }

    // release any DirectInput objects
    if (mDirectInput)
    {
        mDirectInput->Release();
        mDirectInput = nullptr;
    }
}


// calibrate
void GameController::Calibrate()
{
    mDeviceElements[ELEM_POS_X].mCalibrationValue       = -(mJoystickState.lX / 1000.0f);
    mDeviceElements[ELEM_POS_Y].mCalibrationValue       = -(mJoystickState.lY / 1000.0f);
    mDeviceElements[ELEM_POS_Z].mCalibrationValue       = -(mJoystickState.lZ / 1000.0f);
    mDeviceElements[ELEM_ROT_X].mCalibrationValue       = -(mJoystickState.lRx / 1000.0f);
    mDeviceElements[ELEM_ROT_Y].mCalibrationValue       = -(mJoystickState.lRy / 1000.0f);
    mDeviceElements[ELEM_ROT_Z].mCalibrationValue       = -(mJoystickState.lRz / 1000.0f);
    //mDeviceElements[ELEM_POV_1].mCalibrationValue     = -(mJoystickState.rgdwPOV[0] / 1000.0f);
    //mDeviceElements[ELEM_POV_2].mCalibrationValue     = -(mJoystickState.rgdwPOV[1] / 1000.0f);
    //mDeviceElements[ELEM_POV_3].mCalibrationValue     = -(mJoystickState.rgdwPOV[2] / 1000.0f);
    //mDeviceElements[ELEM_POV_4].mCalibrationValue     = -(mJoystickState.rgdwPOV[3] / 1000.0f);
    mDeviceElements[ELEM_SLIDER_1].mCalibrationValue    = -(mJoystickState.rglSlider[0] / 1000.0f);
    mDeviceElements[ELEM_SLIDER_2].mCalibrationValue    = -(mJoystickState.rglSlider[1] / 1000.0f);
}


// update the controller
bool GameController::Update()
{
    if (mJoystick == nullptr)
    {
        mValid = false;
        return false;
    }

    // poll the device to read the current state
    HRESULT result = mJoystick->Poll();
    if (FAILED(result))
    {
        // DInput is telling us that the input stream has been
        // interrupted. We aren't tracking any state between polls, so
        // we don't have any special reset that needs to be done. We
        // just re-acquire and try again.
        result = mJoystick->Acquire();
        while (result == DIERR_INPUTLOST)
        {
            result = mJoystick->Acquire();
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
            mValid = true;
            return true;
        }

        if (FAILED(result))
        {
            mValid = false;
            return false;
        }
    }

    // Get the input's device state
    result = mJoystick->GetDeviceState(sizeof(DIJOYSTATE2), &mJoystickState);
    if (FAILED(result))
    {
        mValid = false;
        return false; // The device should have been acquired during the Poll()
    }

    // update the values of the elements
    mDeviceElements[ELEM_POS_X].mValue      = mJoystickState.lX / 1000.0f;
    mDeviceElements[ELEM_POS_Y].mValue      = mJoystickState.lY / 1000.0f;
    mDeviceElements[ELEM_POS_Z].mValue      = mJoystickState.lZ / 1000.0f;
    mDeviceElements[ELEM_ROT_X].mValue      = mJoystickState.lRx / 1000.0f;
    mDeviceElements[ELEM_ROT_Y].mValue      = mJoystickState.lRy / 1000.0f;
    mDeviceElements[ELEM_ROT_Z].mValue      = mJoystickState.lRz / 1000.0f;
    mDeviceElements[ELEM_SLIDER_1].mValue   = mJoystickState.rglSlider[0] / 1000.0f;
    mDeviceElements[ELEM_SLIDER_2].mValue   = mJoystickState.rglSlider[1] / 1000.0f;

    if (mJoystickState.rgdwPOV[0] == MCORE_INVALIDINDEX32)
    {
        mDeviceElements[ELEM_POV_1].mValue = 0.0f;
    }
    else
    {
        mDeviceElements[ELEM_POV_1].mValue = (mJoystickState.rgdwPOV[0] / 100.0f) / 360.0f;
    }
    if (mJoystickState.rgdwPOV[1] == MCORE_INVALIDINDEX32)
    {
        mDeviceElements[ELEM_POV_2].mValue = 0.0f;
    }
    else
    {
        mDeviceElements[ELEM_POV_2].mValue = (mJoystickState.rgdwPOV[1] / 100.0f) / 360.0f;
    }
    if (mJoystickState.rgdwPOV[2] == MCORE_INVALIDINDEX32)
    {
        mDeviceElements[ELEM_POV_3].mValue = 0.0f;
    }
    else
    {
        mDeviceElements[ELEM_POV_3].mValue = (mJoystickState.rgdwPOV[2] / 100.0f) / 360.0f;
    }
    if (mJoystickState.rgdwPOV[3] == MCORE_INVALIDINDEX32)
    {
        mDeviceElements[ELEM_POV_4].mValue = 0.0f;
    }
    else
    {
        mDeviceElements[ELEM_POV_4].mValue = (mJoystickState.rgdwPOV[3] / 100.0f) / 360.0f;
    }

    // apply the dead zone
    const float minValue    = mDeadZone;
    const float maxValue    = 1.0f;
    const float range       = maxValue - minValue;

    // iterate through all axes
    for (uint32 i = 0; i < 8; ++i)
    {
        // get the current normalized value
        float value = mDeviceElements[i].mValue;

        // ignore all values that are smaller than the dead zone
        if (value > -mDeadZone && value < mDeadZone)
        {
            mDeviceElements[i].mValue = 0.0f;
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
            const float newValue = (value - mDeadZone) / range;

            // set it back in normal or negated version
            mDeviceElements[i].mValue = negativeValue == false ? newValue : -newValue;
        }
    }

    mValid = true;
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


uint32 GameController::FindElemendIDByName(const AZStd::string& elementEnumName)
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

