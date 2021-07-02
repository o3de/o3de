/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#ifndef __GAMECONTROLLER_H
#define __GAMECONTROLLER_H


// include the required headers
#if !defined(Q_MOC_RUN)
#include <AzCore/PlatformIncl.h>
#include <AzCore/std/string/string.h>
#include <MCore/Source/StandardHeaders.h>
#include "../StandardPluginsConfig.h"
#endif

#if AZ_TRAIT_EMOTIONFX_HAS_GAME_CONTROLLER

//#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>
#include <dinputd.h>


/**
 * The game controller (joystick) class.
 * This will initialize DirectInput and use the first available valid joystick.
 *
 */
class GameController
{
    MCORE_MEMORYOBJECTCATEGORY(GameController, EMFX_DEFAULT_ALIGNMENT, MEMCATEGORY_STANDARDPLUGINS_ANIMGRAPH);

public:
    enum Axis
    {
        ELEM_POS_X      = 0,
        ELEM_POS_Y      = 1,
        ELEM_POS_Z      = 2,
        ELEM_ROT_X      = 3,
        ELEM_ROT_Y      = 4,
        ELEM_ROT_Z      = 5,
        ELEM_SLIDER_1   = 6,
        ELEM_SLIDER_2   = 7,
        ELEM_POV_1      = 8,
        ELEM_POV_2      = 9,
        ELEM_POV_3      = 10,
        ELEM_POV_4      = 11,
        NUM_ELEMENTS
    };

    enum ElementType
    {
        ELEMTYPE_AXIS   = 0,
        ELEMTYPE_SLIDER = 1,
        ELEMTYPE_POV    = 2
    };

    GameController()  { mDirectInput = nullptr; mJoystick = nullptr; mHWnd = nullptr; mDeadZone = 0.15f; mValid = false; }
    ~GameController() { Shutdown(); }

    bool Init(HWND hWnd);
    bool Update();      // call this every frame
    void Calibrate();
    void Shutdown();

    MCORE_INLINE IDirectInputDevice8* GetJoystick() const                       { return mJoystick; }       // returns nullptr when no joystick found during init

    MCORE_INLINE const char* GetDeviceName() const                              { return mDeviceInfo.mName.c_str(); }
    MCORE_INLINE const AZStd::string& GetDeviceNameString() const               { return mDeviceInfo.mName; }
    MCORE_INLINE uint32 GetNumButtons() const                                   { return mDeviceInfo.mNumButtons; }
    MCORE_INLINE uint32 GetNumSliders() const                                   { return mDeviceInfo.mNumSliders; }
    MCORE_INLINE uint32 GetNumPOVs() const                                      { return mDeviceInfo.mNumPOVs; }
    MCORE_INLINE uint32 GetNumAxes() const                                      { return mDeviceInfo.mNumAxes; }
    void SetDeadZone(float deadZone)                                            { mDeadZone = deadZone; }
    MCORE_INLINE float GetDeadZone() const                                      { return mDeadZone; }
    const char* GetElementEnumName(uint32 index);
    uint32 FindElemendIDByName(const AZStd::string& elementEnumName);

    MCORE_INLINE bool GetIsPresent(uint32 elementID) const                      { return mDeviceElements[elementID].mPresent; }
    MCORE_INLINE bool GetIsButtonPressed(uint8 buttonIndex) const
    {
        if (buttonIndex < 128)
        {
            return (mJoystickState.rgbButtons[buttonIndex] & 0x80) != 0;
        }
        return false;
    }
    MCORE_INLINE float GetValue(uint32 elementID) const                         { return mDeviceElements[elementID].mValue; }
    MCORE_INLINE const char* GetElementName(uint32 elementID) const             { return mDeviceElements[elementID].mName.c_str(); }
    MCORE_INLINE bool GetIsValid() const                                        { return mValid; }

private:
    struct DeviceInfo
    {
        MCORE_MEMORYOBJECTCATEGORY(GameController::DeviceInfo, EMFX_DEFAULT_ALIGNMENT, MEMCATEGORY_STANDARDPLUGINS_ANIMGRAPH);

        AZStd::string   mName;
        uint32          mNumButtons;
        uint32          mNumAxes;
        uint32          mNumPOVs;
        uint32          mNumSliders;
    };

    struct DeviceElement
    {
        MCORE_MEMORYOBJECTCATEGORY(GameController::DeviceElement, EMFX_DEFAULT_ALIGNMENT, MEMCATEGORY_STANDARDPLUGINS_ANIMGRAPH);

        AZStd::string   mName;
        float           mValue;
        float           mCalibrationValue;
        ElementType     mType;
        bool            mPresent;
    };

    struct EnumContext
    {
        MCORE_MEMORYOBJECTCATEGORY(GameController::EnumContext, EMFX_DEFAULT_ALIGNMENT, MEMCATEGORY_STANDARDPLUGINS_ANIMGRAPH);

        DIJOYCONFIG*    mPrefJoystickConfig;
        bool            mPrefJoystickConfigValid;
    };

    MCORE_INLINE void SetButtonPressed(uint8 buttonIndex, bool isPressed)
    {
        if (buttonIndex > 127)
        {
            return;
        }
        if (isPressed)
        {
            mJoystickState.rgbButtons[buttonIndex] |= 0x80;
        }
        else
        {
            mJoystickState.rgbButtons[buttonIndex] &= ~0x80;
        }
    }

    IDirectInput8*              mDirectInput;
    IDirectInputDevice8*        mJoystick;
    DIJOYSTATE2                 mJoystickState;               // DInput Joystick state
    EnumContext                 mEnumContext;
    HWND                        mHWnd;
    DeviceInfo                  mDeviceInfo;
    DeviceElement               mDeviceElements[NUM_ELEMENTS];
    float                       mDeadZone;
    bool                        mValid;

    static BOOL CALLBACK EnumJoysticksCallback(const DIDEVICEINSTANCE* pdidInstance, void* pContext);
    static BOOL CALLBACK EnumObjectsCallback(const DIDEVICEOBJECTINSTANCE* pdidoi, void* pContext);
    HRESULT InitDirectInput(HWND hWnd);
    void LogError(HRESULT value, const char* text);
};

#endif

#endif
