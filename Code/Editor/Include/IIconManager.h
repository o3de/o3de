/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

struct IMaterial;
class CBitmap;

class QImage;

// Note: values are used as array indices
enum EStatObject
{
    eStatObject_Arrow = 0,
    eStatObject_Axis,
    eStatObject_Sphere,
    eStatObject_Anchor,
    eStatObject_Entrance,
    eStatObject_HidePoint,
    eStatObject_HidePointSecondary,
    eStatObject_ReinforcementSpot,

    eStatObject_COUNT
};

// Note: values are used as array indices
enum EIcon
{
    eIcon_ScaleWarning = 0,
    eIcon_RotationWarning,

    eIcon_COUNT
};

// Note: image effects to apply to image
enum EIconEffect
{
    eIconEffect_Dim = 1 << 0,
    eIconEffect_HalfAlpha = 1 << 1,
    eIconEffect_TintRed = 1 << 2,
    eIconEffect_TintGreen = 1 << 3,
    eIconEffect_TintYellow = 1 << 4,
    eIconEffect_ColorEnabled = 1 << 5,
    eIconEffect_ColorDisabled = 1 << 6,
};

struct IIconManager
{
    virtual ~IIconManager() = default;
    virtual int GetIconTexture(EIcon icon) = 0;
    virtual int GetIconTexture(const char* iconName) = 0;
    virtual QImage* GetIconBitmap(const char* filename, bool& haveAlpha, uint32 effects = 0) = 0;
    // Register an Icon for the specific command
    virtual void RegisterCommandIcon([[maybe_unused]] const char* filename, [[maybe_unused]] int nCommandId) {}
};
