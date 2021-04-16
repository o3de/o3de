// ----------------------------------------------------------------------------
// Wrappers for setting values that end up in constant buffers.
// ----------------------------------------------------------------------------
//
// Copyright (c) 2019 Advanced Micro Devices, Inc. All rights reserved.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

#pragma once


//#ifndef _TRESSFXSIMULATIONPARAMS_H_
//#define _TRESSFXSIMULATIONPARAMS_H_
/*
#pragma once
*/
//#include <TressFX/AMD_Types.h>
#include <TressFX/TressFXCommon.h>


#include <memory>

// Probably want to unify or rename these.
namespace AMD
{
    class TressFXSimulationSettings
    {
    public:
        TressFXSimulationSettings() :
            m_vspCoeff(0.8f),
            m_vspAccelThreshold(1.4f),
            m_localConstraintStiffness(0.9f),
            m_localConstraintsIterations(2),
            m_globalConstraintStiffness(0.0f),
            m_globalConstraintsRange(0.0f),
            m_lengthConstraintsIterations(2),
            m_damping(0.08f),
            m_gravityMagnitude(0.0),
            m_tipSeparation(0.0f),
            m_windMagnitude(0),
            m_windAngleRadians(3.1415926f / 180.0f * 40.0f),
            m_clampPositionDelta(20.f)
        {
            m_windDirection[0] = 1.0f;
            m_windDirection[1] = 0.0f;
            m_windDirection[2] = 0.0f;
        }

        // VSPf
        float m_vspCoeff;
        float m_vspAccelThreshold;

        // local constraint
        float m_localConstraintStiffness;
        int   m_localConstraintsIterations;

        // global constraint
        float m_globalConstraintStiffness;
        float m_globalConstraintsRange;

        // length constraint
        int m_lengthConstraintsIterations;

        // damping
        float m_damping;

        // gravity
        float m_gravityMagnitude;

        // tip separation for follow hair from its guide
        float m_tipSeparation;

        // wind
        float m_windMagnitude;
        float m_windDirection[3];
        float m_windAngleRadians;
        float m_clampPositionDelta;
    };

    class TressFXRenderingSettings
    {
    public:
        // LOD settings
        float	m_LODStartDistance = 1.f;
        float	m_LODEndDistance = 5.f;
        float	m_LODPercent = 0.5f;
        float	m_LODWidthMultiplier = 2.f;

        // General information
        float   m_FiberRadius = 0.0021f;
        float   m_TipPercentage = 0.f;
        float   m_StrandUVTilingFactor = 1.f;
        float   m_FiberRatio = 0.463f;

        // For lighting/shading
        AMD::float4  m_HairMatBaseColor = { 1.f, 1.f, 1.f, 0.63f };
        AMD::float4  m_HairMatTipColor = { 1.f, 1.f, 1.f, 0.63f };
        float   m_HairKDiffuse = 0.07f;
        float   m_HairKSpec1 = 0.0017f;
        float   m_HairSpecExp1 = 14.40f;

        float   m_HairKSpec2 = 0.072f;
        float   m_HairSpecExp2 = 11.80f;

        // For deep approximated shadow lookup
        float   m_HairShadowAlpha = 0.35f;
        float   m_HairFiberSpacing = 0.4f;
        int     m_HairMaxShadowFibers = 50;
        float	m_ShadowLODStartDistance = 1.f;
        float	m_ShadowLODEndDistance = 5.f;
        float	m_ShadowLODPercent = 0.5f;
        float	m_ShadowLODWidthMultiplier = 2.f;

        bool    m_EnableStrandUV = false;
        bool    m_EnableStrandTangent = false;
        bool    m_EnableThinTip = false;
        bool	m_EnableHairLOD = false;
        bool	m_EnableShadowLOD = false;

        std::string m_BaseAlbedoName = "<none>";
        std::string m_StrandAlbedoName = "<none>";
    };

} // namespace AMD

//#endif // _TRESSFXSIMULATIONPARAMS_H_
