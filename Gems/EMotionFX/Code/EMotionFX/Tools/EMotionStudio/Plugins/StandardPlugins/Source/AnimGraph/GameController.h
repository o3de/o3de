/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
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

    GameController()  { m_directInput = nullptr; m_joystick = nullptr; m_hWnd = nullptr; m_deadZone = 0.15f; m_valid = false; }
    ~GameController() { Shutdown(); }

    bool Init(HWND hWnd);
    bool Update();      // call this every frame
    void Calibrate();
    void Shutdown();

    MCORE_INLINE IDirectInputDevice8* GetJoystick() const                       { return m_joystick; }       // returns nullptr when no joystick found during init

    MCORE_INLINE const char* GetDeviceName() const                              { return m_deviceInfo.m_name.c_str(); }
    MCORE_INLINE const AZStd::string& GetDeviceNameString() const               { return m_deviceInfo.m_name; }
    MCORE_INLINE uint32 GetNumButtons() const                                   { return m_deviceInfo.m_numButtons; }
    MCORE_INLINE uint32 GetNumSliders() const                                   { return m_deviceInfo.m_numSliders; }
    MCORE_INLINE uint32 GetNumPOVs() const                                      { return m_deviceInfo.m_numPoVs; }
    MCORE_INLINE uint32 GetNumAxes() const                                      { return m_deviceInfo.m_numAxes; }
    void SetDeadZone(float deadZone)                                            { m_deadZone = deadZone; }
    MCORE_INLINE float GetDeadZone() const                                      { return m_deadZone; }
    const char* GetElementEnumName(uint32 index);
    uint32 FindElementIDByName(const AZStd::string& elementEnumName);

    MCORE_INLINE bool GetIsPresent(uint32 elementID) const                      { return m_deviceElements[elementID].m_present; }
    MCORE_INLINE bool GetIsButtonPressed(uint8 buttonIndex) const
    {
        if (buttonIndex < 128)
        {
            return (m_joystickState.rgbButtons[buttonIndex] & 0x80) != 0;
        }
        return false;
    }
    MCORE_INLINE float GetValue(uint32 elementID) const                         { return m_deviceElements[elementID].m_value; }
    MCORE_INLINE const char* GetElementName(uint32 elementID) const             { return m_deviceElements[elementID].m_name.c_str(); }
    MCORE_INLINE bool GetIsValid() const                                        { return m_valid; }

private:
    struct DeviceInfo
    {
        MCORE_MEMORYOBJECTCATEGORY(GameController::DeviceInfo, EMFX_DEFAULT_ALIGNMENT, MEMCATEGORY_STANDARDPLUGINS_ANIMGRAPH);

        AZStd::string   m_name;
        uint32          m_numButtons;
        uint32          m_numAxes;
        uint32          m_numPoVs;
        uint32          m_numSliders;
    };

    struct DeviceElement
    {
        MCORE_MEMORYOBJECTCATEGORY(GameController::DeviceElement, EMFX_DEFAULT_ALIGNMENT, MEMCATEGORY_STANDARDPLUGINS_ANIMGRAPH);

        AZStd::string   m_name;
        float           m_value;
        float           m_calibrationValue;
        ElementType     m_type;
        bool            m_present;
    };

    struct EnumContext
    {
        MCORE_MEMORYOBJECTCATEGORY(GameController::EnumContext, EMFX_DEFAULT_ALIGNMENT, MEMCATEGORY_STANDARDPLUGINS_ANIMGRAPH);

        DIJOYCONFIG*    m_prefJoystickConfig;
        bool            m_prefJoystickConfigValid;
    };

    MCORE_INLINE void SetButtonPressed(uint8 buttonIndex, bool isPressed)
    {
        if (buttonIndex > 127)
        {
            return;
        }
        if (isPressed)
        {
            m_joystickState.rgbButtons[buttonIndex] |= 0x80;
        }
        else
        {
            m_joystickState.rgbButtons[buttonIndex] &= ~0x80;
        }
    }

    IDirectInput8*              m_directInput;
    IDirectInputDevice8*        m_joystick;
    DIJOYSTATE2                 m_joystickState;               // DInput Joystick state
    EnumContext                 m_enumContext;
    HWND                        m_hWnd;
    DeviceInfo                  m_deviceInfo;
    DeviceElement               m_deviceElements[NUM_ELEMENTS];
    float                       m_deadZone;
    bool                        m_valid;

    static BOOL CALLBACK EnumJoysticksCallback(const DIDEVICEINSTANCE* pdidInstance, void* pContext);
    static BOOL CALLBACK EnumObjectsCallback(const DIDEVICEOBJECTINSTANCE* pdidoi, void* pContext);
    HRESULT InitDirectInput(HWND hWnd);
    void LogError(HRESULT value, const char* text);
};

#endif

#endif
