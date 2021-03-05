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

#ifndef CRYINCLUDE_CRYCOMMONTOOLS_EXPORT_ANIMATIONDATA_H
#define CRYINCLUDE_CRYCOMMONTOOLS_EXPORT_ANIMATIONDATA_H
#pragma once


#include "IAnimationData.h"

#include <vector>

// Animation data class for skeletal animations
// It has a same count of samples for all models(bones)
// and always has translation/rotation/scaling data together as a set.
class AnimationData
    : public IAnimationData
{
public:
    AnimationData(int modelCount, float fps, float startTime);
    virtual ~AnimationData() {}

    // IAnimationData
    virtual void SetFrameData(int modelIndex, int frameIndex, float translation[3], float rotation[3], float scale[3]);
    virtual void SetFrameCount(int frameCount);
    virtual void SetModelFlags(int modelIndex, unsigned modelFlags);

    virtual void SetFrameTimePos(int modelIndex, int frameIndex, float time)
    { assert(0); }
    virtual void SetFrameDataPos(int modelIndex, int frameIndex, float translation[3])
    { assert(0); }
    virtual void SetFrameCountPos(int modelIndex, int frameCount)
    { assert(0); }
    virtual void SetFrameTimeRot(int modelIndex, int frameIndex, float time)
    { assert(0); }
    virtual void SetFrameDataRot(int modelIndex, int frameIndex, float rotation[3])
    { assert(0); }
    virtual void SetFrameCountRot(int modelIndex, int frameCount)
    { assert(0); }
    virtual void SetFrameTimeScl(int modelIndex, int frameIndex, float time)
    { assert(0); }
    virtual void SetFrameDataScl(int modelIndex, int frameIndex, float scale[3])
    { assert(0); }
    virtual void SetFrameCountScl(int modelIndex, int frameCount)
    { assert(0); }

    virtual void GetFrameData(int modelIndex, int frameIndex, const float*& translation, const float*& rotation, const float*& scale) const;
    virtual int GetFrameCount() const;
    virtual unsigned GetModelFlags(int modelIndex) const;

    virtual float GetFrameTimePos(int modelIndex, int frameIndex) const
    { return m_startTime + frameIndex / m_fps; }
    virtual void GetFrameDataPos(int modelIndex, int frameIndex, const float*& translation) const;
    virtual int GetFrameCountPos(int) const
    { return GetFrameCount(); }
    virtual float GetFrameTimeRot(int modelIndex, int frameIndex) const
    { return m_startTime + frameIndex / m_fps; }
    virtual void GetFrameDataRot(int modelIndex, int frameIndex, const float*& rotation) const;
    virtual int GetFrameCountRot(int) const
    { return GetFrameCount(); }
    virtual float GetFrameTimeScl(int modelIndex, int frameIndex) const
    { return m_startTime + frameIndex / m_fps; }
    virtual void GetFrameDataScl(int modelIndex, int frameIndex, const float*& scale) const;
    virtual int GetFrameCountScl(int) const
    { return GetFrameCount(); }

    // TCB & Ease-In/-Out not supported for the skeletal animation.
    virtual void SetFrameTCBPos(int modelIndex, int frameIndex, TCB tcb)
    { assert(0); }
    virtual void SetFrameTCBRot(int modelIndex, int frameIndex, TCB tcb)
    { assert(0); }
    virtual void SetFrameTCBScl(int modelIndex, int frameIndex, TCB tcb)
    { assert(0); }
    virtual void SetFrameEaseInOutPos(int modelIndex, int frameIndex, Ease ease)
    { assert(0); }
    virtual void SetFrameEaseInOutRot(int modelIndex, int frameIndex, Ease ease)
    { assert(0); }
    virtual void SetFrameEaseInOutScl(int modelIndex, int frameIndex, Ease ease)
    { assert(0); }
    virtual void GetFrameTCBPos(int modelIndex, int frameIndex, TCB& tcb) const
    { assert(0); }
    virtual void GetFrameTCBRot(int modelIndex, int frameIndex, TCB& tcb) const
    { assert(0); }
    virtual void GetFrameTCBScl(int modelIndex, int frameIndex, TCB& tcb) const
    { assert(0); }
    virtual void GetFrameEaseInOutPos(int modelIndex, int frameIndex, Ease& ease) const
    { assert(0); }
    virtual void GetFrameEaseInOutRot(int modelIndex, int frameIndex, Ease& ease) const
    { assert(0); }
    virtual void GetFrameEaseInOutScl(int modelIndex, int frameIndex, Ease& ease) const
    { assert(0); }

private:
    struct State
    {
    public:
        State();
        float translation[3];
        float rotation[3];
        float scale[3];
    };

    struct ModelEntry
    {
        ModelEntry();

        unsigned flags;
        std::vector<State> samples;
    };

    std::vector<ModelEntry> m_entries;
    int m_frameCount;
    float m_startTime;
    float m_fps;
};

// Animation data class for non-skeletal animations
// It can have different counts of samples for each model
// and each channel of transformation data.
class NonSkeletalAnimationData
    : public IAnimationData
{
public:
    NonSkeletalAnimationData(int modelCount);
    virtual ~NonSkeletalAnimationData() {}

    // IAnimationData
    virtual void SetFrameData(int modelIndex, int frameIndex, float translation[3], float rotation[3], float scale[3])
    { assert(0); }
    virtual void SetFrameCount(int frameCount)
    { assert(0); }
    virtual void SetModelFlags(int modelIndex, unsigned modelFlags);

    virtual void SetFrameTimePos(int modelIndex, int frameIndex, float time);
    virtual void SetFrameDataPos(int modelIndex, int frameIndex, float translation[3]);
    virtual void SetFrameCountPos(int modelIndex, int frameCount);
    virtual void SetFrameTimeRot(int modelIndex, int frameIndex, float time);
    virtual void SetFrameDataRot(int modelIndex, int frameIndex, float rotation[3]);
    virtual void SetFrameCountRot(int modelIndex, int frameCount);
    virtual void SetFrameTimeScl(int modelIndex, int frameIndex, float time);
    virtual void SetFrameDataScl(int modelIndex, int frameIndex, float scale[3]);
    virtual void SetFrameCountScl(int modelIndex, int frameCount);

    virtual void GetFrameData(int modelIndex, int frameIndex, const float*& translation, const float*& rotation, const float*& scale) const
    { assert(0); }
    virtual int GetFrameCount() const
    {
        assert(0);
        return 0;
    }
    virtual unsigned GetModelFlags(int modelIndex) const;

    virtual float GetFrameTimePos(int modelIndex, int frameIndex) const;
    virtual void GetFrameDataPos(int modelIndex, int frameIndex, const float*& translation) const;
    virtual int GetFrameCountPos(int) const;
    virtual float GetFrameTimeRot(int modelIndex, int frameIndex) const;
    virtual void GetFrameDataRot(int modelIndex, int frameIndex, const float*& rotation) const;
    virtual int GetFrameCountRot(int) const;
    virtual float GetFrameTimeScl(int modelIndex, int frameIndex) const;
    virtual void GetFrameDataScl(int modelIndex, int frameIndex, const float*& scale) const;
    virtual int GetFrameCountScl(int) const;

    virtual void SetFrameTCBPos(int modelIndex, int frameIndex, TCB tcb);
    virtual void SetFrameTCBRot(int modelIndex, int frameIndex, TCB tcb);
    virtual void SetFrameTCBScl(int modelIndex, int frameIndex, TCB tcb);
    virtual void SetFrameEaseInOutPos(int modelIndex, int frameIndex, Ease ease);
    virtual void SetFrameEaseInOutRot(int modelIndex, int frameIndex, Ease ease);
    virtual void SetFrameEaseInOutScl(int modelIndex, int frameIndex, Ease ease);

    virtual void GetFrameTCBPos(int modelIndex, int frameIndex, TCB& tcb) const;
    virtual void GetFrameTCBRot(int modelIndex, int frameIndex, TCB& tcb) const;
    virtual void GetFrameTCBScl(int modelIndex, int frameIndex, TCB& tcb) const;
    virtual void GetFrameEaseInOutPos(int modelIndex, int frameIndex, Ease& ease) const;
    virtual void GetFrameEaseInOutRot(int modelIndex, int frameIndex, Ease& ease) const;
    virtual void GetFrameEaseInOutScl(int modelIndex, int frameIndex, Ease& ease) const;

private:
    struct State
    {
    public:
        State();
        float time;
        float data[3];
        TCB tcb;
        Ease ease;
    };

    struct ModelEntry
    {
        ModelEntry();

        unsigned flags;
        std::vector<State> samplesPos;
        std::vector<State> samplesRot;
        std::vector<State> samplesScl;
    };

    std::vector<ModelEntry> m_entries;
};

#endif // CRYINCLUDE_CRYCOMMONTOOLS_EXPORT_ANIMATIONDATA_H
