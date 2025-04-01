// ACES code derived from the nVidia HDR Display Demo Project
// (https://developer.nvidia.com/high-dynamic-range-display-development)
// -----------------------------------------------------------------------------
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
// -----------------------------------------------------------------------------
// License Terms for Academy Color Encoding System Components
//
// Academy Color Encoding System (ACES) software and tools are provided by the
//  Academy under the following terms and conditions: A worldwide, royalty-free,
//  non-exclusive right to copy, modify, create derivatives, and use, in source and
//  binary forms, is hereby granted, subject to acceptance of this license.
//
// Copyright © 2015 Academy of Motion Picture Arts and Sciences (A.M.P.A.S.).
// Portions contributed by others as indicated. All rights reserved.
//
// Performance of any of the aforementioned acts indicates acceptance to be bound
//  by the following terms and conditions:
//
// * Copies of source code, in whole or in part, must retain the above copyright
//   notice, this list of conditions and the Disclaimer of Warranty.
// * Use in binary form must retain the above copyright notice, this list of
//   conditions and the Disclaimer of Warranty in the documentation and/or other
//   materials provided with the distribution.
// * Nothing in this license shall be deemed to grant any rights to trademarks,
//   copyrights, patents, trade secrets or any other intellectual property of
//   A.M.P.A.S. or any contributors, except as expressly stated herein.
// * Neither the name "A.M.P.A.S." nor the name of any other contributors to this
//   software may be used to endorse or promote products derivative of or based on
//   this software without express prior written permission of A.M.P.A.S. or the
//   contributors, as appropriate.
//
// This license shall be construed pursuant to the laws of the State of California,
// and any disputes related thereto shall be subject to the jurisdiction of the
// courts therein.
//
// Disclaimer of Warranty: THIS SOFTWARE IS PROVIDED BY A.M.P.A.S. AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
// THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE,
// AND NON-INFRINGEMENT ARE DISCLAIMED. IN NO EVENT SHALL A.M.P.A.S., OR ANY
// CONTRIBUTORS OR DISTRIBUTORS, BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, RESITUTIONARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
// OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
// ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
//////////////////////////////////////////////////////////////////////////////////
// WITHOUT LIMITING THE GENERALITY OF THE FOREGOING, THE ACADEMY SPECIFICALLY
// DISCLAIMS ANY REPRESENTATIONS OR WARRANTIES WHATSOEVER RELATED TO PATENT OR
// OTHER INTELLECTUAL PROPERTY RIGHTS IN THE ACADEMY COLOR ENCODING SYSTEM, OR
// APPLICATIONS THEREOF, HELD BY PARTIES OTHER THAN A.M.P.A.S.,WHETHER DISCLOSED OR
// UNDISCLOSED.
//
// -----------------------------------------------------------------------------
// Modified from original
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
#include <AzCore/Preprocessor/Enum.h>

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

        enum class ShaperType  : uint32_t
        {
            Linear = 0,
            Log2 = 1,
            PqSmpteSt2084 = 2,
            NumShaperTypes
        };

        struct ShaperParams
        {
            ShaperType m_type = ShaperType::Linear;
            float m_bias = 0.0f;
            float m_scale = 1.0f;
        };

        AZ_ENUM_CLASS_WITH_UNDERLYING_TYPE(DisplayMapperOperationType, uint32_t,
            Aces,
            AcesLut,
            Passthrough,
            GammaSRGB,
            Reinhard,
            AcesFitted,
            AcesFilmic,
            Filmic
        );

        enum class ShaperPresetType
        {
            None = 0,
            LinearCustomRange,
            Log2_48Nits,
            Log2_1000Nits,
            Log2_2000Nits,
            Log2_4000Nits,
            Log2CustomRange,
            PqSmpteSt2084,
            NumShaperTypes
        };

        enum class ToneMapperType
        {
            None = 0,
            Reinhard,
            AcesFitted,
            AcesFilmic,
            Filmic
        };

        enum class TransferFunctionType
        {
            None = 0,
            Gamma22 = 1,
            PerceptualQuantizer = 2
        };

        SegmentedSplineParamsC9 GetAcesODTParameters(OutputDeviceTransformType odtType);
        ShaperParams GetLog2ShaperParameters(float minStops, float maxStops);
        ShaperParams GetAcesShaperParameters(OutputDeviceTransformType odtType);
        Matrix3x3 GetColorConvertionMatrix(ColorConvertionMatrixType type);

    }   // namespace Render

    AZ_TYPE_INFO_SPECIALIZE(Render::DisplayMapperOperationType, "{41CA80B1-9E0D-41FB-A235-9638D2A905A5}");
    AZ_TYPE_INFO_SPECIALIZE(Render::OutputDeviceTransformType, "{B94085B7-C0D4-466A-A791-188A4559EC8D}");
}   // namespace AZ
