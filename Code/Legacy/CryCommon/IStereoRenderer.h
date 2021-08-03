/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#ifndef __ISTEREORENDERER_H__
#define __ISTEREORENDERER_H__

#pragma once

#include "StereoRendererBus.h"

enum EStereoEye
{
    STEREO_EYE_LEFT  = 0,
    STEREO_EYE_RIGHT = 1,
    STEREO_EYE_COUNT
};

enum EStereoDevice
{
    STEREO_DEVICE_NONE      = 0,
    STEREO_DEVICE_FRAMECOMP = 1,
    STEREO_DEVICE_HDMI      = 2,
    STEREO_DEVICE_DRIVER    = 3,  //!< Nvidia and AMD drivers.
    STEREO_DEVICE_DUALHEAD  = 4,
    STEREO_DEVICE_COUNT,

    STEREO_DEVICE_DEFAULT = 100  //!< Auto-detect device.
};

enum EStereoMode
{
    STEREO_MODE_NO_STEREO       = 0,  //!< Stereo disabled.
    STEREO_MODE_DUAL_RENDERING  = 1,
    STEREO_MODE_COUNT,
};

enum EStereoOutput
{
    STEREO_OUTPUT_STANDARD          = 0,
    STEREO_OUTPUT_IZ3D              = 1,
    STEREO_OUTPUT_CHECKERBOARD      = 2,
    STEREO_OUTPUT_ABOVE_AND_BELOW   = 3,
    STEREO_OUTPUT_SIDE_BY_SIDE      = 4,
    STEREO_OUTPUT_LINE_BY_LINE      = 5,
    STEREO_OUTPUT_ANAGLYPH          = 6,
    STEREO_OUTPUT_HMD               = 7,
    STEREO_OUTPUT_COUNT,
};

enum EStereoDeviceState
{
    STEREO_DEVSTATE_OK = 0,
    STEREO_DEVSTATE_UNSUPPORTED_DEVICE,
    STEREO_DEVSTATE_REQ_1080P,
    STEREO_DEVSTATE_REQ_FRAMEPACKED,
    STEREO_DEVSTATE_BAD_DRIVER,
    STEREO_DEVSTATE_REQ_FULLSCREEN
};

class IStereoRenderer
    : public AZ::StereoRendererRequestBus::Handler
{
public:
    enum EHmdRender
    {
        eHR_Eyes = 0,
        eHR_Latency
    };

    // <interfuscator:shuffle>
    virtual ~IStereoRenderer(){}

    virtual EStereoDevice GetDevice() = 0;
    virtual EStereoDeviceState GetDeviceState() = 0;
    virtual void GetInfo(EStereoDevice* device, EStereoMode* mode, EStereoOutput* output, EStereoDeviceState* state) const = 0;

    virtual bool GetStereoEnabled() = 0;
    virtual float GetStereoStrength() = 0;
    virtual float GetMaxSeparationScene(bool half = true) = 0;
    virtual float GetZeroParallaxPlaneDist() = 0;

    virtual void OnHmdDeviceChanged() = 0;

    virtual void OnResolutionChanged() {}

    virtual void GetNVControlValues(bool& stereoEnabled, float& stereoStrength) = 0;
    // </interfuscator:shuffle>

    enum class Status
    {
        kRenderingFirstEye,
        kRenderingSecondEye,
        kIdle ///< Not currently rendering to either eye.
    };

    virtual Status GetStatus() const = 0;
};

#endif
