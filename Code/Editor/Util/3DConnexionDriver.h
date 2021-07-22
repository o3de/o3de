/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#ifndef CRYINCLUDE_EDITOR_UTIL_3DCONNEXIONDRIVER_H
#define CRYINCLUDE_EDITOR_UTIL_3DCONNEXIONDRIVER_H
#pragma once
#include "Include/IPlugin.h"

struct S3DConnexionMessage
{
    bool bGotTranslation;
    bool bGotRotation;

    int raw_translation[3];
    int raw_rotation[3];

    Vec3 vTranslate;
    Vec3 vRotate;

    unsigned char buttons[3];

    S3DConnexionMessage()
        : bGotRotation(false)
        , bGotTranslation(false)
    {
        raw_translation[0] = raw_translation[1] = raw_translation[2] = 0;
        raw_rotation[0] = raw_rotation[1] = raw_rotation[2] = 0;
        vTranslate.Set(0, 0, 0);
        vRotate.Set(0, 0, 0);
        buttons[0] = buttons[1] = buttons[2] = 0;
    };
};

#if defined(AZ_PLATFORM_WINDOWS)
AZ_PUSH_DISABLE_DLL_EXPORT_BASECLASS_WARNING
class SANDBOX_API C3DConnexionDriver
    : public IPlugin
{
AZ_POP_DISABLE_DLL_EXPORT_BASECLASS_WARNING
public:
    C3DConnexionDriver();
    ~C3DConnexionDriver();

    bool InitDevice();
    bool GetInputMessageData(LPARAM lParam, S3DConnexionMessage& msg);

    void Release() { delete this; };
    void ShowAbout() {};
    const char* GetPluginGUID() { return "{AD109901-9128-4ffd-8E67-137CB2B1C41B}"; };
    DWORD GetPluginVersion() { return 1; };
    const char* GetPluginName() { return "3DConnexionDriver"; };
    bool CanExitNow() { return true; };
    void OnEditorNotify([[maybe_unused]] EEditorNotifyEvent aEventId){}

private:
    class C3DConnexionDriverImpl* m_pImpl;
    PRAWINPUTDEVICELIST m_pRawInputDeviceList;
    PRAWINPUTDEVICE m_pRawInputDevices;
    int m_nUsagePage1Usage8Devices;
    float m_fMultiplier;
};
#endif
#endif // CRYINCLUDE_EDITOR_UTIL_3DCONNEXIONDRIVER_H
