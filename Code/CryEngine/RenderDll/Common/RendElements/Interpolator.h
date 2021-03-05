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


// Interpolator deals with animations' interpolation.
// These are bare bones interpolators: they take 3 key values as inputs.
// The first two specify the two end points(start and end), and the last one specify the interpolation tweenness (x-value).
// The output is the y-value.
// For additional time-based playback type animation, see Timeline.


#ifndef CRYINCLUDE_CRYENGINE_RENDERDLL_COMMON_RENDELEMENTS_INTERPOLATOR_H
#define CRYINCLUDE_CRYENGINE_RENDERDLL_COMMON_RENDELEMENTS_INTERPOLATOR_H
#pragma once

template<class T>
class Interpolator
{
protected:
    virtual T mix(T v0, T v1, float mixRatio)
    {
        return (T)(v0 * (1 - mixRatio) + v1 * mixRatio);
    }
public:
    virtual ~Interpolator(){}
    virtual T compute(T start, T end, float xValue) = 0;
};

template<class T>
class LinearInterp
    : public Interpolator<T>
{
public:
    virtual T compute(T start, T end, float xValue)
    {
        return Interpolator<T>::mix(start, end, xValue);
    }
};

template<class T>
class CubicInterp
    : public Interpolator<T>
{
public:
    virtual T compute(T start, T end, float xValue)
    {
        float x = xValue * xValue * (3 - 2 * xValue);
        return Interpolator<T>::mix(start, end, x);
    }
};

namespace InterpPredef
{
    static LinearInterp<float> Linear_FLOAT;
    static LinearInterp<int> Linear_INT;

    static CubicInterp<float> CUBIC_FLOAT;
    static CubicInterp<int> CUBIC_INT;
}
#endif // CRYINCLUDE_CRYENGINE_RENDERDLL_COMMON_RENDELEMENTS_INTERPOLATOR_H
