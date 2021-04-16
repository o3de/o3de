// AMD AMDUtils code
// 
// Copyright(c) 2019 Advanced Micro Devices, Inc.All rights reserved.
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files(the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions :
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#include "Freesync2.h"
#include "Misc\Error.h"

#include <string.h>
#include <assert.h>

namespace CAULDRON_DX12
{
    bool            m_isFS2Display;
    bool            m_isHDR10Display;
    int             m_displayIndex;
    AGSDisplayInfo  m_AGSDisplayInfo;
    AGSContext     *m_pAGSContext;
    AGSGPUInfo     *m_pGPUInfo;

    void fs2Init(AGSContext *pAGSContext, AGSGPUInfo *pGPUInfo)
    {
        m_displayIndex = -1;
        m_isFS2Display = false;
        m_isHDR10Display = false;
        m_AGSDisplayInfo = {};
        m_pAGSContext = pAGSContext;
        m_pGPUInfo = pGPUInfo;

        if (m_pAGSContext == NULL)
            return;

        // Assert that primary monitor supports freesync2 mode
        // Store that monitor index if it exists

        int numDevices = pGPUInfo->numDevices;
        AGSDeviceInfo* devices = pGPUInfo->devices;

        int numDisplays = devices[0].numDisplays;

        m_displayIndex = -1;
        for (int i = 0; i < numDisplays; i++)
        {
            if (devices[0].displays[i].displayFlags & AGSDisplayFlags::AGS_DISPLAYFLAG_FREESYNC_2
                &&
                devices[0].displays[i].displayFlags & AGSDisplayFlags::AGS_DISPLAYFLAG_PRIMARY_DISPLAY)
            {
                m_displayIndex = i;
                m_isFS2Display = true;
                m_isHDR10Display = true;
                break;
            }
        }

        if (!m_isFS2Display)
        {
            for (int i = 0; i < numDisplays; i++)
            {
                if (devices[0].displays[i].displayFlags & AGSDisplayFlags::AGS_DISPLAYFLAG_HDR10
                    &&
                    devices[0].displays[i].displayFlags & AGSDisplayFlags::AGS_DISPLAYFLAG_PRIMARY_DISPLAY)
                {
                    m_displayIndex = i;
                    m_isHDR10Display = true;
                    break;
                }
            }
        }

        if (m_displayIndex == -1)
        {
            // Set primary display as display to render to
            for (int i = 0; i < numDisplays; i++)
            {
                if (devices[0].displays[i].displayFlags & AGSDisplayFlags::AGS_DISPLAYFLAG_PRIMARY_DISPLAY)
                {
                    m_displayIndex = i;
                }
            }
        }
    }

    DXGI_FORMAT fs2GetFormat(DisplayModes displayMode)
    {
        if (m_pAGSContext==NULL)
            displayMode = DISPLAYMODE_SDR;

        switch (displayMode)
        {
        case DISPLAYMODE_SDR:
            return DXGI_FORMAT_R8G8B8A8_UNORM;

        case DISPLAYMODE_FS2_Gamma22:
            return DXGI_FORMAT_R10G10B10A2_UNORM;

        case DISPLAYMODE_FS2_SCRGB:
            return DXGI_FORMAT_R16G16B16A16_FLOAT;

        case DISPLAYMODE_HDR10_2084:
            return DXGI_FORMAT_R10G10B10A2_UNORM;

        case DISPLAYMODE_HDR10_SCRGB:
            return DXGI_FORMAT_R16G16B16A16_FLOAT;

        default:
            return DXGI_FORMAT_UNKNOWN;
        }
    }

    void fs2SetDisplayMode(DisplayModes displayMode, bool disableLocalDimming)
    {
        if (m_pAGSContext == NULL)
            return;

        m_AGSDisplayInfo = m_pGPUInfo->devices[0].displays[m_displayIndex];

        AGSDisplaySettings agsDisplaySettings = {};

        switch (displayMode)
        {
        case DISPLAYMODE_SDR:
        {
            // rec 709 primaries
            m_AGSDisplayInfo.chromaticityRedX = 0.64;
            m_AGSDisplayInfo.chromaticityRedY = 0.33;
            m_AGSDisplayInfo.chromaticityGreenX = 0.30;
            m_AGSDisplayInfo.chromaticityGreenY = 0.60;
            m_AGSDisplayInfo.chromaticityBlueX = 0.15;
            m_AGSDisplayInfo.chromaticityBlueY = 0.06;
            m_AGSDisplayInfo.chromaticityWhitePointX = 0.3127;
            m_AGSDisplayInfo.chromaticityWhitePointY = 0.3290;

            agsDisplaySettings.mode = AGSDisplaySettings::Mode::Mode_SDR;
            agsDisplaySettings.flags = 0; // Local dimming always enabled for SDR, therefore 'disableLocalDimming' flag should be set to false ie 0.
            break;
        }

        case DISPLAYMODE_FS2_Gamma22:
        {
            agsDisplaySettings.mode = AGSDisplaySettings::Mode::Mode_Freesync2_Gamma22;
            agsDisplaySettings.flags = disableLocalDimming; // Local dimming could be enabled or disabled for FS2 based on preference.
            if (disableLocalDimming)
                m_AGSDisplayInfo.maxLuminance = m_AGSDisplayInfo.avgLuminance;
            break;
        }

        case DISPLAYMODE_FS2_SCRGB:
        {
            agsDisplaySettings.mode = AGSDisplaySettings::Mode::Mode_Freesync2_scRGB;
            agsDisplaySettings.flags = disableLocalDimming; // Local dimming could be enabled or disabled for FS2 based on preference.
            if (disableLocalDimming)
                m_AGSDisplayInfo.maxLuminance = m_AGSDisplayInfo.avgLuminance;
            break;
        }

        case DISPLAYMODE_HDR10_2084:
        {
            // rec 2020 primaries
            m_AGSDisplayInfo.chromaticityRedX = 0.708;
            m_AGSDisplayInfo.chromaticityRedY = 0.292;
            m_AGSDisplayInfo.chromaticityGreenX = 0.170;
            m_AGSDisplayInfo.chromaticityGreenY = 0.797;
            m_AGSDisplayInfo.chromaticityBlueX = 0.131;
            m_AGSDisplayInfo.chromaticityBlueY = 0.046;
            m_AGSDisplayInfo.chromaticityWhitePointX = 0.3127;
            m_AGSDisplayInfo.chromaticityWhitePointY = 0.3290;
            m_AGSDisplayInfo.minLuminance = 0.0;
            m_AGSDisplayInfo.maxLuminance = 1000.0; // This will cause tonemapping to happen on display end as long as it's greater than display's actual queried max luminance. The look will change and it will be display dependent!
            agsDisplaySettings.maxContentLightLevel = 1000.0;
            agsDisplaySettings.maxFrameAverageLightLevel = 400.0; // max and average content ligt level data will be used to do tonemapping on display
            agsDisplaySettings.mode = AGSDisplaySettings::Mode::Mode_HDR10_PQ;
            agsDisplaySettings.flags = 0; // Local dimming always enabled for HDR, therefore 'disableLocalDimming' flag should be set to false ie 0.
            break;
        }

        case DISPLAYMODE_HDR10_SCRGB:
        {
            // rec 2020 primaries
            m_AGSDisplayInfo.chromaticityRedX = 0.708;
            m_AGSDisplayInfo.chromaticityRedY = 0.292;
            m_AGSDisplayInfo.chromaticityGreenX = 0.170;
            m_AGSDisplayInfo.chromaticityGreenY = 0.797;
            m_AGSDisplayInfo.chromaticityBlueX = 0.131;
            m_AGSDisplayInfo.chromaticityBlueY = 0.046;
            m_AGSDisplayInfo.chromaticityWhitePointX = 0.3127;
            m_AGSDisplayInfo.chromaticityWhitePointY = 0.3290;
            m_AGSDisplayInfo.minLuminance = 0.0;
            m_AGSDisplayInfo.maxLuminance = 1000.0; // This will cause tonemapping to happen on display end as long as it's greater than display's actual queried max luminance. The look will change and it will be display dependent!
            agsDisplaySettings.maxContentLightLevel = 1000.0;
            agsDisplaySettings.maxFrameAverageLightLevel = 400.0; // max and average content ligt level data will be used to do tonemapping on display
            agsDisplaySettings.mode = AGSDisplaySettings::Mode::Mode_HDR10_scRGB;
            agsDisplaySettings.flags = 0; // Local dimming always enabled for HDR, therefore 'disableLocalDimming' flag should be set to false ie 0.
            break;
        }
        }

        agsDisplaySettings.chromaticityRedX = m_AGSDisplayInfo.chromaticityRedX;
        agsDisplaySettings.chromaticityRedY = m_AGSDisplayInfo.chromaticityRedY;
        agsDisplaySettings.chromaticityGreenX = m_AGSDisplayInfo.chromaticityGreenX;
        agsDisplaySettings.chromaticityGreenY = m_AGSDisplayInfo.chromaticityGreenY;
        agsDisplaySettings.chromaticityBlueX = m_AGSDisplayInfo.chromaticityBlueX;
        agsDisplaySettings.chromaticityBlueY = m_AGSDisplayInfo.chromaticityBlueY;
        agsDisplaySettings.chromaticityWhitePointX = m_AGSDisplayInfo.chromaticityWhitePointX;
        agsDisplaySettings.chromaticityWhitePointY = m_AGSDisplayInfo.chromaticityWhitePointY;
        agsDisplaySettings.minLuminance = m_AGSDisplayInfo.minLuminance;
        agsDisplaySettings.maxLuminance = m_AGSDisplayInfo.maxLuminance;

        AGSReturnCode rc = agsSetDisplayMode(m_pAGSContext, 0, m_displayIndex, &agsDisplaySettings);
        assert(rc == AGS_SUCCESS);
    }

    const AGSDisplayInfo* fs2GetDisplayInfo()
    {
        if (m_pAGSContext == NULL)
            return NULL;

        return &m_AGSDisplayInfo;
    };

    bool fs2IsFreesync2Display()
    {
        return m_isFS2Display;
    };

    bool fs2IsHDR10Display()
    {
        return m_isHDR10Display;
    };

}
