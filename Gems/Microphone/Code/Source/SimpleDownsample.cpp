/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "Microphone_precompiled.h"

#include "SimpleDownsample.h"

#include <cmath>

AZStd::size_t GetDownsampleSize(AZStd::size_t sourceSize, AZ::u32 sourceSampleRate, AZ::u32 targetSampleRate)
{
    return (AZStd::size_t)round((float)sourceSize / ((float)sourceSampleRate / (float)targetSampleRate));
}

void Downsample(AZ::s16* inBuffer, AZStd::size_t inBufferSize, AZ::u32 inBufferSampleRate,
                AZ::s16* outBuffer, AZStd::size_t outBufferSize, AZ::u32 outBufferSampleRate)
{
    if(inBufferSampleRate == outBufferSampleRate)
    {
        return;    // nothing to do here!
    }

    if(inBufferSampleRate < outBufferSampleRate)
    {
        AZ_Error("SimpleDownsample", false, "Out buffer sample rate is higher than in buffer sample rate, must be lower");
        return;
    }

    float sampleRateRatio = (float)inBufferSampleRate / (float)outBufferSampleRate;
    AZStd::size_t offsetResult = 0;
    AZStd::size_t offsetBuffer = 0;
    while(offsetResult < outBufferSize)
    {
        AZStd::size_t nextOffsetBuffer = static_cast<AZStd::size_t>(round((float)(offsetResult + 1) * sampleRateRatio));
        AZ::s32 accum = 0;    // this must be int 32 so it doesn't overflow with multiple int 16's possibly at max
        AZStd::size_t count = 0;
        for(AZStd::size_t i = offsetBuffer; i < nextOffsetBuffer && i < inBufferSize; ++i)
        {
            accum += inBuffer[i];
            count++;
        }
        // do the math in floating point then convert to signed 16
        outBuffer[offsetResult] = (AZ::s16)((float)accum / (float)count);
        offsetResult++;
        offsetBuffer = nextOffsetBuffer;
    }
}
