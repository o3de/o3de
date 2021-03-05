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

#ifndef CRYINCLUDE_CRYENGINE_RENDERDLL_COMMON_RENDELEMENTS_TIMELINE_H
#define CRYINCLUDE_CRYENGINE_RENDERDLL_COMMON_RENDELEMENTS_TIMELINE_H
#pragma once

#include "Interpolator.h"

template <class T>
class Timeline
{
public:
    enum LoopMode
    {
        NORMAL, REVERSE, LOOP
    };

protected:
    T prevYValue;
    float prevXValue;

public:
    Interpolator<T>* pInterp;
    T startValue;
    T endValue;
    LoopMode loopMode;
    float duration;         // in milliseconds
    float currentTime;  // in milliseconds

    void init(T start, T end, float _duration, Interpolator<T>* interp)
    {
        SetInterpolationRange(start, end);
        prevYValue = start;
        prevXValue = 0;
        this->duration = _duration;
        loopMode = NORMAL;

        pInterp = interp;
    }

protected:
    float computeTimeStepping(float curTime, float elapsedMs)
    {
        switch (loopMode)
        {
        case NORMAL:
            curTime += elapsedMs;
            if (curTime > duration)
            {
                return duration;
            }
        case REVERSE:
            curTime -= elapsedMs;
            if (curTime < 0)
            {
                return 0;
            }
        case LOOP:
            curTime += elapsedMs;
            if (curTime > duration)
            {
                return fmod(curTime, duration);
            }
        }

        return curTime;
    }

    void positCursorByRatio(float ratio)
    {
        switch (loopMode)
        {
        case NORMAL:
            currentTime = ratio * duration;
            break;
        case REVERSE:
            currentTime = (1 - ratio) * duration;
            break;
        case LOOP:
            break;
        }
    }

public:

    virtual ~Timeline() {}
    Timeline(T start, T end, float _duration, Interpolator<T>* interp)
    {
        currentTime = 0;
        init(start, end, _duration, interp);
    }

    Timeline(Interpolator<T>* interp)
    {
        currentTime = 0;
        init(0, 1, 1000, interp);
    }

    // Accumulate time and produce the interpolated value accordingly
    virtual T step(float elapsedMs)
    {
        currentTime = computeTimeStepping(currentTime, elapsedMs);
        prevXValue = currentTime / duration;

        prevYValue = pInterp->compute(startValue, endValue, prevXValue);
        return prevYValue;
    }

    // Rewind the current time to initial position
    virtual void rewind()
    {
        positCursorByRatio(0);
    }

    void SetInterpolationRange(T start, T end)
    {
        startValue = start;
        endValue = end;
    }

    float GetPrevXValue() { return prevXValue; }
    T GetPrevYValue() { return prevYValue; }
};

namespace {
    float tempFloat0 = 0;
    float tempFloat1 = 1;
    int tempInt0 = 0;
    int tempInt1 = 1;
}

class TimelineFloat
    : public Timeline<float>
{
public:
    TimelineFloat()
        : Timeline<float>(0, 1, 1000, &InterpPredef::CUBIC_FLOAT) {}
};

class TimelineInt
    : public Timeline<int>
{
public:
    TimelineInt()
        : Timeline<int>(0, 1, 1000, &InterpPredef::CUBIC_INT) {}
};

#endif // CRYINCLUDE_CRYENGINE_RENDERDLL_COMMON_RENDELEMENTS_TIMELINE_H
