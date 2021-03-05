/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
// Original file Copyright Crytek GMBH or its affiliates, used under license.

#ifndef CRYINCLUDE_EDITOR_INCLUDE_IICONMANAGER_H
#define CRYINCLUDE_EDITOR_INCLUDE_IICONMANAGER_H
#pragma once

struct IStatObj;
struct IMaterial;
class CBitmap;

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
    virtual IStatObj*   GetObject(EStatObject object) = 0;
    virtual int GetIconTexture(EIcon icon) = 0;
    virtual int GetIconTexture(const char* iconName) = 0;
    virtual _smart_ptr<IMaterial> GetHelperMaterial() = 0;
    virtual QImage* GetIconBitmap(const char* filename, bool& haveAlpha, uint32 effects = 0) = 0;
    // Register an Icon for the specific command
    virtual void RegisterCommandIcon([[maybe_unused]] const char* filename, [[maybe_unused]] int nCommandId) {}
};

#endif // CRYINCLUDE_EDITOR_INCLUDE_IICONMANAGER_H
