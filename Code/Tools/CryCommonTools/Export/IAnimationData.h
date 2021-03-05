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

#ifndef CRYINCLUDE_CRYCOMMONTOOLS_EXPORT_IANIMATIONDATA_H
#define CRYINCLUDE_CRYCOMMONTOOLS_EXPORT_IANIMATIONDATA_H
#pragma once


class IAnimationData
{
public:
    virtual ~IAnimationData() {}

    virtual void SetFrameData(int modelIndex, int frameIndex, float translation[3], float rotation[3], float scale[3]) = 0;
    virtual void SetFrameCount(int frameCount) = 0;

    virtual void SetFrameTimePos(int modelIndex, int frameIndex, float time) = 0;
    virtual void SetFrameDataPos(int modelIndex, int frameIndex, float translation[3]) = 0;
    virtual void SetFrameCountPos(int modelIndex, int frameCount) = 0;
    virtual void SetFrameTimeRot(int modelIndex, int frameIndex, float time) = 0;
    virtual void SetFrameDataRot(int modelIndex, int frameIndex, float rotation[3]) = 0;
    virtual void SetFrameCountRot(int modelIndex, int frameCount) = 0;
    virtual void SetFrameTimeScl(int modelIndex, int frameIndex, float time) = 0;
    virtual void SetFrameDataScl(int modelIndex, int frameIndex, float scale[3]) = 0;
    virtual void SetFrameCountScl(int modelIndex, int frameCount) = 0;

    // For TCB & Ease-In/-Out support
    struct TCB
    {
        float tension;
        float continuity;
        float bias;

        TCB()
            : tension(0)
            , continuity(0)
            , bias(0) {}
    };
    struct Ease
    {
        float in;
        float out;

        Ease()
            : in(0)
            , out(0) {}
    };
    virtual void SetFrameTCBPos(int modelIndex, int frameIndex, TCB tcb) = 0;
    virtual void SetFrameTCBRot(int modelIndex, int frameIndex, TCB tcb) = 0;
    virtual void SetFrameTCBScl(int modelIndex, int frameIndex, TCB tcb) = 0;
    virtual void SetFrameEaseInOutPos(int modelIndex, int frameIndex, Ease ease) = 0;
    virtual void SetFrameEaseInOutRot(int modelIndex, int frameIndex, Ease ease) = 0;
    virtual void SetFrameEaseInOutScl(int modelIndex, int frameIndex, Ease ease) = 0;

    enum ModelFlags
    {
        ModelFlags_NoExport = 1 << 0
    };

    virtual void SetModelFlags(int modelIndex, unsigned modelFlags) = 0;

    virtual void GetFrameData(int modelIndex, int frameIndex, const float*& translation, const float*& rotation, const float*& scale) const = 0;
    virtual int GetFrameCount() const = 0;

    virtual float GetFrameTimePos(int modelIndex, int frameIndex) const = 0;
    virtual void GetFrameDataPos(int modelIndex, int frameIndex, const float*& translation) const = 0;
    virtual int GetFrameCountPos(int modelIndex) const = 0;
    virtual float GetFrameTimeRot(int modelIndex, int frameIndex) const = 0;
    virtual void GetFrameDataRot(int modelIndex, int frameIndex, const float*& rotation) const = 0;
    virtual int GetFrameCountRot(int modelIndex) const = 0;
    virtual float GetFrameTimeScl(int modelIndex, int frameIndex) const = 0;
    virtual void GetFrameDataScl(int modelIndex, int frameIndex, const float*& scale) const = 0;
    virtual int GetFrameCountScl(int modelIndex) const = 0;

    // For TCB & Ease-In/-Out support
    virtual void GetFrameTCBPos(int modelIndex, int frameIndex, TCB& tcb) const = 0;
    virtual void GetFrameTCBRot(int modelIndex, int frameIndex, TCB& tcb) const = 0;
    virtual void GetFrameTCBScl(int modelIndex, int frameIndex, TCB& tcb) const = 0;
    virtual void GetFrameEaseInOutPos(int modelIndex, int frameIndex, Ease& ease) const = 0;
    virtual void GetFrameEaseInOutRot(int modelIndex, int frameIndex, Ease& ease) const = 0;
    virtual void GetFrameEaseInOutScl(int modelIndex, int frameIndex, Ease& ease) const = 0;

    virtual unsigned GetModelFlags(int modelIndex) const = 0;
};

#endif // CRYINCLUDE_CRYCOMMONTOOLS_EXPORT_IANIMATIONDATA_H
