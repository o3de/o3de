/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


// determine the new buffer size for downsampling
AZStd::size_t GetDownsampleSize(AZStd::size_t sourceSize, AZ::u32 sourceSampleRate, AZ::u32 targetSampleRate);

// down sample a 16 bit audio buffer from on sample rate frequency to another lower sample rate frequency
// outBuffer must already be allocated and large enough to hold the downsampled result
void Downsample(AZ::s16* inBuffer, AZStd::size_t inBufferSize, AZ::u32 inBufferSampleRate,
                AZ::s16* outBuffer, AZStd::size_t outBufferSize, AZ::u32 outBufferSampleRate);
