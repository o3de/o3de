/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include "EditorDefs.h"

#include "3DConnexionDriver.h"

#if defined(AZ_PLATFORM_WINDOWS)

//////////////////////////////////////////////////////////////////////////
C3DConnexionDriver::C3DConnexionDriver()
{
    m_pRawInputDeviceList = 0;
    m_pRawInputDevices = 0;
    m_nUsagePage1Usage8Devices = 0;
    m_fMultiplier = 1.0f;

    InitDevice();
}

//////////////////////////////////////////////////////////////////////////
C3DConnexionDriver::~C3DConnexionDriver()
{
}

bool C3DConnexionDriver::InitDevice()
{
    // Find the Raw Devices
    UINT nDevices;
    // Get Number of devices attached
    if (GetRawInputDeviceList(nullptr, &nDevices, sizeof(RAWINPUTDEVICELIST)) != 0)
    {
        return false;
    }
    // Create list large enough to hold all RAWINPUTDEVICE structs
    if ((m_pRawInputDeviceList = (PRAWINPUTDEVICELIST)malloc(sizeof(RAWINPUTDEVICELIST) * nDevices)) == nullptr)
    {
        return false;
    }
    // Now get the data on the attached devices
    if (GetRawInputDeviceList(m_pRawInputDeviceList, &nDevices, sizeof(RAWINPUTDEVICELIST)) == -1)
    {
        return false;
    }

    m_pRawInputDevices = (PRAWINPUTDEVICE)malloc(nDevices * sizeof(RAWINPUTDEVICE));
    m_nUsagePage1Usage8Devices = 0;

    // Look through device list for RIM_TYPEHID devices with UsagePage == 1, Usage == 8
    for (UINT i = 0; i < nDevices; i++)
    {
        //Doc says RIM_TYPEHID: Data comes from an HID that is not a keyboard or a mouse.
        if (m_pRawInputDeviceList[i].dwType == RIM_TYPEHID)
        {
            RID_DEVICE_INFO dinfo;
            UINT sizeofdinfo = sizeof(dinfo);
            dinfo.cbSize = sizeofdinfo;
            GetRawInputDeviceInfo(m_pRawInputDeviceList[i].hDevice,
                RIDI_DEVICEINFO, &dinfo, &sizeofdinfo);
            if (dinfo.dwType == RIM_TYPEHID)
            {
                RID_DEVICE_INFO_HID* phidInfo = &dinfo.hid;
                // Add this one to the list of interesting devices?
                // Actually only have to do this once to get input from all usage 1, usagePage 8 devices
                // This just keeps out the other usages.
                // You might want to put up a list for users to select amongst the different devices.
                // In particular, to assign separate functionality to the different devices.
                if (phidInfo->usUsagePage == 1 && phidInfo->usUsage == 8)
                {
                    m_pRawInputDevices[m_nUsagePage1Usage8Devices].usUsagePage = phidInfo->usUsagePage;
                    m_pRawInputDevices[m_nUsagePage1Usage8Devices].usUsage     = phidInfo->usUsage;
                    m_pRawInputDevices[m_nUsagePage1Usage8Devices].dwFlags     = 0;
                    m_pRawInputDevices[m_nUsagePage1Usage8Devices].hwndTarget  = nullptr;
                    m_nUsagePage1Usage8Devices++;
                }
            }
        }
    }

    // Register for input from the devices in the list
    if (RegisterRawInputDevices(m_pRawInputDevices, m_nUsagePage1Usage8Devices, sizeof(RAWINPUTDEVICE)) == false)
    {
        return false;
    }

    return true;
}

//////////////////////////////////////////////////////////////////////////
bool C3DConnexionDriver::GetInputMessageData(LPARAM lParam, S3DConnexionMessage& msg)
{
    ZeroStruct(msg);

    RAWINPUTHEADER header;
    UINT size = sizeof(header);
    if (GetRawInputData((HRAWINPUT)lParam, RID_HEADER, &header,  &size, sizeof(RAWINPUTHEADER)) == -1)
    {
        return false;
    }

    // Set aside enough memory for the full event
    char rawbuffer[128];
    LPRAWINPUT event = (LPRAWINPUT)rawbuffer;
    size = sizeof(rawbuffer);
    if (GetRawInputData((HRAWINPUT)lParam, RID_INPUT, event, &size, sizeof(RAWINPUTHEADER)) == -1)
    {
        return false;
    }
    else
    {
        if (event->header.dwType == RIM_TYPEHID)
        {
            static bool bGotTranslation = false,
                        bGotRotation    = false;
            static int all6DOFs[6] = {0};
            LPRAWHID pRawHid = &event->data.hid;

            // Translation or Rotation packet?  They come in two different packets.
            if (pRawHid->bRawData[0] == 1) // Translation vector
            {
                msg.raw_translation[0] = (pRawHid->bRawData[1] & 0x000000ff) | ((signed short)(pRawHid->bRawData[2] << 8) & 0xffffff00);
                msg.raw_translation[1] = (pRawHid->bRawData[3] & 0x000000ff) | ((signed short)(pRawHid->bRawData[4] << 8) & 0xffffff00);
                msg.raw_translation[2] = (pRawHid->bRawData[5] & 0x000000ff) | ((signed short)(pRawHid->bRawData[6] << 8) & 0xffffff00);
                msg.vTranslate.x = msg.raw_translation[0] / 255.f * m_fMultiplier;
                msg.vTranslate.y = msg.raw_translation[1] / 255.f * m_fMultiplier;
                msg.vTranslate.z = msg.raw_translation[2] / 255.f * m_fMultiplier;
                msg.bGotTranslation = true;
            }
            else if (pRawHid->bRawData[0] == 2) // Rotation vector
            {
                msg.raw_rotation[0] = (pRawHid->bRawData[1] & 0x000000ff) | ((signed short)(pRawHid->bRawData[2] << 8) & 0xffffff00);
                msg.raw_rotation[1] = (pRawHid->bRawData[3] & 0x000000ff) | ((signed short)(pRawHid->bRawData[4] << 8) & 0xffffff00);
                msg.raw_rotation[2] = (pRawHid->bRawData[5] & 0x000000ff) | ((signed short)(pRawHid->bRawData[6] << 8) & 0xffffff00);
                msg.vRotate.x = msg.raw_rotation[0] / 255.f * m_fMultiplier;
                msg.vRotate.y = msg.raw_rotation[1] / 255.f * m_fMultiplier;
                msg.vRotate.z = msg.raw_rotation[2] / 255.f * m_fMultiplier;
                msg.bGotRotation = true;
            }
            else if (pRawHid->bRawData[0] == 3) // Buttons (display most significant byte to least)
            {
                msg.buttons[0] = (unsigned char)pRawHid->bRawData[1];
                msg.buttons[1] = (unsigned char)pRawHid->bRawData[2];
                msg.buttons[2] = (unsigned char)pRawHid->bRawData[3];

                CryLog("Button mask: %.2x %.2x %.2x\n", (unsigned char)pRawHid->bRawData[3], (unsigned char)pRawHid->bRawData[2], (unsigned char)pRawHid->bRawData[1]);

                if (msg.buttons[0] == 1)
                {
                    m_fMultiplier /= 2.0f;
                }
                else if (msg.buttons[0] == 2)
                {
                    m_fMultiplier *= 2.0f;
                }
            }
        }
    }
    return true;
}

#endif
