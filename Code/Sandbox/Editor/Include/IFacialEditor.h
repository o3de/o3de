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

#ifndef CRYINCLUDE_EDITOR_INCLUDE_IFACIALEDITOR_H
#define CRYINCLUDE_EDITOR_INCLUDE_IFACIALEDITOR_H
#pragma once


class IFacialEditor
{
public:
    enum EyeType
    {
        EYE_LEFT,
        EYE_RIGHT
    };

    virtual int GetNumMorphTargets() const = 0;
    virtual const char* GetMorphTargetName(int index) const = 0;
    virtual void PreviewEffector(int index, float value) = 0;
    virtual void ClearAllPreviewEffectors() = 0;
    virtual void SetForcedNeckRotation(const Quat& rotation) = 0;
    virtual void SetForcedEyeRotation(const Quat& rotation, EyeType eye) = 0;
    virtual int GetJoystickCount() const = 0;
    virtual const char* GetJoystickName(int joystickIndex) const = 0;
    virtual void SetJoystickPosition(int joystickIndex, float x, float y) = 0;
    virtual void GetJoystickPosition(int joystickIndex, float& x, float& y) const = 0;
    virtual void LoadJoystickFile(const char* filename) = 0;
    virtual void LoadCharacter(const char* filename) = 0;
    virtual void LoadSequence(const char* filename) = 0;
    virtual void SetVideoFrameResolution(int width, int height, int bpp) = 0;
    virtual int GetVideoFramePitch() = 0;
    virtual void* GetVideoFrameBits() = 0;
    virtual void ShowVideoFramePane() = 0;
};

#endif // CRYINCLUDE_EDITOR_INCLUDE_IFACIALEDITOR_H
