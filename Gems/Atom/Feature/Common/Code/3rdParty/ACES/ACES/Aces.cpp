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
// https://developer.nvidia.com/high-dynamic-range-display-development
//

#include <ACES/Aces.h>

namespace AZ
{
    namespace Render
    {
        SegmentedSplineParamsC9 GetAcesODTParameters(OutputDeviceTransformType odtType)
        {
            // ACES reference values for ODT
            // https://github.com/ampas/aces-dev/blob/master/transforms/ctl/lib/ACESlib.Tonescales.ctl
            static const SegmentedSplineParamsC9 ODT_48nits =
            {
                // coefs
                {
                    Vector4(-1.69896996, 0.515438676, 0, 0),
                    Vector4(-1.69896996, 0.847043753, 0, 0),
                    Vector4(-1.47790003, 1.1358, 0, 0),
                    Vector4(-1.22909999, 1.38020003, 0, 0),
                    Vector4(-0.864799976, 1.51970005, 0, 0),
                    Vector4(-0.448000014, 1.59850001, 0, 0),
                    Vector4(0.00517999986, 1.64670002, 0, 0),
                    Vector4(0.451108038, 1.67460918, 0, 0),
                    Vector4(0.911374450, 1.68787336, 0, 0),
                    Vector4(0.911374450, 1.68787336, 0, 0)
                },
                { 0.0028798957, 0.02 },    // minPoint
                { 4.79999924, 4.80000019 },    // midPoint  
                { 1005.71912, 48.0 },    // maxPoint
                0.0,  // slopeLow
                0.04  // slopeHigh
            };
            static const SegmentedSplineParamsC9 ODT_1000nits =
            {
                // coefs
                {
                    Vector4(-4.9706219331, 0.8089132070, 0, 0),
                    Vector4(-3.0293780669, 1.1910867930, 0, 0),
                    Vector4(-2.1262, 1.5683, 0, 0),
                    Vector4(-1.5105, 1.9483, 0, 0),
                    Vector4(-1.0578, 2.3083, 0, 0),
                    Vector4(-0.4668, 2.6384, 0, 0),
                    Vector4(0.11938, 2.8595, 0, 0),
                    Vector4(0.7088134201, 2.9872608805, 0, 0),
                    Vector4(1.2911865799, 3.0127391195, 0, 0),
                    Vector4(1.2911865799, 3.0127391195, 0, 0)
                },
                { 0.000141798664, 0.00499999989 },    // minPoint
                { 4.79999924, 10.0 },    // midPoint  
                { 4505.08252, 1000.0 },    // maxPoint
                0.0,  // slopeLow
                0.0599999987  // slopeHigh
            };
            static const SegmentedSplineParamsC9 ODT_2000nits =
            {
                // coefs
                {
                    Vector4(-4.9706219331, 0.8019952042, 0, 0),
                    Vector4(-3.0293780669, 1.1980047958, 0, 0),
                    Vector4(-2.1262, 1.5943000000, 0, 0),
                    Vector4(-1.5105, 1.9973000000, 0, 0),
                    Vector4(-1.0578, 2.3783000000, 0, 0),
                    Vector4(-0.4668, 2.7684000000, 0, 0),
                    Vector4(0.11938, 3.0515000000, 0, 0),
                    Vector4(0.7088134201, 3.2746293562, 0, 0),
                    Vector4(1.2911865799, 3.3274306351, 0, 0),
                    Vector4(1.2911865799, 3.3274306351, 0, 0)
                },
                { 0.000141798664, 0.00499999989 },    // minPoint
                { 4.79999924, 10.0 },    // midPoint  
                { 5771.86377, 2000.0 },    // maxPoint
                0.0,  // slopeLow
                0.119999997  // slopeHigh
            };
            static const SegmentedSplineParamsC9 ODT_4000nits =
            {
                // coefs
                {
                    Vector4(-4.9706219331, 0.7973186613, 0, 0),
                    Vector4(-3.0293780669, 1.2026813387, 0, 0),
                    Vector4(-2.1262, 1.6093000000, 0, 0),
                    Vector4(-1.5105, 2.0108000000, 0, 0),
                    Vector4(-1.0578, 2.4148000000, 0, 0),
                    Vector4(-0.4668, 2.8179000000, 0, 0),
                    Vector4(0.11938, 3.1725000000, 0, 0),
                    Vector4(0.7088134201, 3.5344995451, 0, 0),
                    Vector4(1.2911865799, 3.6696204376, 0, 0),
                    Vector4(1.2911865799, 3.6696204376 , 0, 0)
                },
                { 0.000141798664, 0.00499999989 },    // minPoint
                { 4.79999924, 10.0 },    // midPoint  
                { 6824.36279, 4000.0 },    // maxPoint
                0.0,  // slopeLow
                0.300000023  // slopeHigh
            };

            AZ_Assert(static_cast<uint32_t>(odtType) < static_cast<uint32_t>(NumOutputDeviceTransformTypes), "Invalid ODT type specified.");
            switch(odtType)
            {
            case OutputDeviceTransformType_48Nits:
                return ODT_48nits;
                break;
            case OutputDeviceTransformType_1000Nits:
                return ODT_1000nits;
                break;
            case OutputDeviceTransformType_2000Nits:
                return ODT_2000nits;
                break;
            case OutputDeviceTransformType_4000Nits:
                return ODT_4000nits;
                break;
            default:
                AZ_Assert(false, "Invalid ODT type specified.");
                break;
            }
            return ODT_48nits;
        }

        ShaperParams GetLog2ShaperParameters(float minStops, float maxStops)
        {
            ShaperParams shaperParams;

            constexpr float Log2MediumGray = -2.47393118833f; // log2f(0.18f);
            shaperParams.m_type = ShaperType::Log2;
            shaperParams.m_scale = 1.0f / (maxStops - minStops);
            shaperParams.m_bias = -((minStops + Log2MediumGray) * shaperParams.m_scale);

            return shaperParams;
        }

        ShaperParams GetAcesShaperParameters(OutputDeviceTransformType odtType)
        {
            AZ_Assert(static_cast<uint32_t>(odtType) < static_cast<uint32_t>(NumOutputDeviceTransformTypes), "Invalid ODT type specified.");

            switch (odtType)
            {
            case OutputDeviceTransformType_48Nits:
                return GetLog2ShaperParameters(-6.5f, 6.5f);
            case OutputDeviceTransformType_1000Nits:
                return GetLog2ShaperParameters(-12.0f, 10.0f);
            case OutputDeviceTransformType_2000Nits:
                return GetLog2ShaperParameters(-12.0f, 11.0f);
            case OutputDeviceTransformType_4000Nits:
                return GetLog2ShaperParameters(-12.0f, 12.0f);
            default:
                AZ_Assert(false, "Invalid output device transform type.");
                break;
            }
            return ShaperParams();
        }

        Matrix3x3 GetColorConvertionMatrix(ColorConvertionMatrixType type)
        {
            static const Matrix3x3 ColorConvertionMatrices[] =
            {
                // XYZ to rec709
                Matrix3x3::CreateFromRows(
                    Vector3(3.24096942f, -1.53738296f, -0.49861076f),
                    Vector3(-0.96924388f, 1.87596786f, 0.04155510f),
                    Vector3(0.05563002f, -0.20397684f, 1.05697131f)
                ),
                // rec709 to XYZ
                Matrix3x3::CreateFromRows(
                    Vector3(0.41239089f, 0.35758430f, 0.18048084f),
                    Vector3(0.21263906f, 0.71516860f, 0.07219233f),
                    Vector3(0.01933082f, 0.11919472f, 0.95053232f)
                ),
                // XYZ to bt2020
                Matrix3x3::CreateFromRows(
                    Vector3(1.71665096f, -0.35567081f, -0.25336623f),
                    Vector3(-0.66668433f, 1.61648130f, 0.01576854f),
                    Vector3(0.01763985f, -0.04277061f, 0.94210327f)
                ),
                // bt2020 to XYZ
                Matrix3x3::CreateFromRows(
                    Vector3(0.63695812f, 0.14461692f, 0.16888094f),
                    Vector3(0.26270023f, 0.67799807f, 0.05930171f),
                    Vector3(0.00000000f, 0.02807269f, 1.06098485f)
                )
            };

            AZ_Assert(static_cast<uint32_t>(type) < static_cast<uint32_t>(NumColorConvertionMatrixTypes), "Invalid color convertion matrix type specified.");

            if (type < NumColorConvertionMatrixTypes)
            {
                return ColorConvertionMatrices[type];
            }
            else
            {
                return ColorConvertionMatrices[0];
            }
        }

    }   // namespace Render
}   // namespace AZ
