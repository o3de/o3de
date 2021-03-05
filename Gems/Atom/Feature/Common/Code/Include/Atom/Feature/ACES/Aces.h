// Copyright(c) 2016, NVIDIA CORPORATION.All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met :
//  * Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
//  * Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and / or other materials provided with the distribution.
//  * Neither the name of NVIDIA CORPORATION nor the names of its
//    contributors may be used to endorse or promote products derived
//    from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED.IN NO EVENT SHALL THE COPYRIGHT OWNER OR
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES(INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
// OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Modifications copyright Amazon.com, Inc. or its affiliates. 
//

//
// ACES implementation
// This implementation is partially ported from NVIDIA HDR sample.
//

#pragma once

#include <AzCore/Math/Vector4.h>
#include <AzCore/Math/Vector2.h>
#include <AzCore/Math/Matrix3x3.h>
#include <AzCore/RTTI/RTTI.h>

namespace AZ
{
    namespace Render
    {
        struct SegmentedSplineParamsC9
        {
            Vector4 coefs[10];      // coefs for B-spline between minPoint and midPoint (units of log luminance)
            float minPoint[2];       // {luminance, luminance} linear extension below this
            float midPoint[2];       // {luminance, luminance} 
            float maxPoint[2];       // {luminance, luminance} linear extension above this
            float slopeLow;         // log-log slope of low linear extension
            float slopeHigh;        // log-log slope of high linear extension
        };

        enum OutputDeviceTransformType
        {
            OutputDeviceTransformType_48Nits = 0,
            OutputDeviceTransformType_1000Nits = 1,
            OutputDeviceTransformType_2000Nits = 2,
            OutputDeviceTransformType_4000Nits = 3,
            NumOutputDeviceTransformTypes
        };

        enum ColorConvertionMatrixType
        {
            XYZ_To_Rec709 = 0,
            Rec709_To_XYZ,
            XYZ_To_Bt2020,
            Bt2020_To_XYZ,
            NumColorConvertionMatrixTypes
        };

        enum ShaperType
        {
            Linear = 0,
            Log2 = 1,
            NumShaperTypes
        };

        struct ShaperParams
        {
            ShaperType type = ShaperType::Linear;
            float bias = 0.f;
            float scale = 1.f;
        };

        enum class DisplayMapperOperationType : uint32_t
        {
            Aces = 0,
            AcesLut,
            Passthrough,
            GammaSRGB,
            Reinhard,
            Invalid
        };

        enum class ShaperPresetType
        {
            None = 0,
            Log2_48_nits,
            Log2_1000_nits,
            Log2_2000_nits,
            Log2_4000_nits
        };

        enum class ToneMapperType
        {
            None = 0,
            Reinhard
        };

        enum class TransferFunctionType
        {
            None = 0,
            Gamma22 = 1,
            PerceptualQuantizer = 2
        };

        SegmentedSplineParamsC9 GetAcesODTParameters(OutputDeviceTransformType odtType);
        ShaperParams GetAcesShaperParameters(OutputDeviceTransformType odtType);
        Matrix3x3 GetColorConvertionMatrix(ColorConvertionMatrixType type);

    }   // namespace Render

    AZ_TYPE_INFO_SPECIALIZE(Render::DisplayMapperOperationType, "{41CA80B1-9E0D-41FB-A235-9638D2A905A5}");
}   // namespace AZ
