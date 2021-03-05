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


// determine the new buffer size for downsampling
AZStd::size_t GetDownsampleSize(AZStd::size_t sourceSize, AZ::u32 sourceSampleRate, AZ::u32 targetSampleRate);

// down sample a 16 bit audio buffer from on sample rate frequency to another lower sample rate frequency
// outBuffer must already be allocated and large enough to hold the downsampled result
void Downsample(AZ::s16* inBuffer, AZStd::size_t inBufferSize, AZ::u32 inBufferSampleRate,
                AZ::s16* outBuffer, AZStd::size_t outBufferSize, AZ::u32 outBufferSampleRate);
