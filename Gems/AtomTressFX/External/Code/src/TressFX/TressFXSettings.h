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
#include <TressFX/TressFXCommon.h>
#include <AzCore/RTTI/ReflectContext.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/Vector4.h>
#include <AzCore/Math/Color.h>
#include <AzCore/std/string/string.h>

// Atom
#include <Atom/RPI.Reflect/Image/StreamingImageAsset.h>

#include <memory>

namespace AMD
{
    class TressFXSimulationSettings
    {
    public:
        AZ_TYPE_INFO(TressFXSimulationSettings, "{B16E92B3-C859-4421-9170-65C2C6A60062}");
        static void Reflect(AZ::ReflectContext* context);

        // VSPf
        float m_vspCoeff = 0.758f;
        float m_vspAccelThreshold = 1.208f;

        // local constraint
        float m_localConstraintStiffness = 0.908f;
        int   m_localConstraintsIterations = 3;

        // global constraint
        float m_globalConstraintStiffness = 0.408f;
        float m_globalConstraintsRange = 0.308f;

        // length constraint
        int m_lengthConstraintsIterations = 3;

        // damping
        float m_damping = 0.08f;

        // gravity
        float m_gravityMagnitude = 0.19f;

        // tip separation for follow hair from its guide
        float m_tipSeparation = 0.1f;

        // wind
        float m_windMagnitude = 0.0f;
        AZ::Vector3 m_windDirection{1.0f, 0.0f, 0.0f};
        float m_windAngleRadians = 3.1415926f / 180.0f * 40.0f;
        float m_clampPositionDelta = 20.0f;
    };

    class TressFXRenderingSettings
    {
    public:
        AZ_TYPE_INFO(TressFXRenderingSettings, "{7EFD9317-4DE8-455D-A2E5-B5B62FF1F5D7}");
        static void Reflect(AZ::ReflectContext* context);

        // LOD settings
        float	m_LODStartDistance = 1.f;
        float	m_LODEndDistance = 5.f;
        float	m_LODPercent = 0.5f;
        float	m_LODWidthMultiplier = 2.f;

        // General information
        float   m_FiberRadius = 0.002f;
        float   m_TipPercentage = 0.0f;
        float   m_StrandUVTilingFactor = 1.f;
        float   m_FiberRatio = 0.06f;

        // Original TressFX Kajiya lighting model parameters
        AZ::Color m_HairMatBaseColor{ 1.f, 1.f, 1.f, 0.63f };
        AZ::Color m_HairMatTipColor{1.f, 1.f, 1.f, 0.63f};
        float   m_HairKDiffuse = 0.22f;
        float   m_HairKSpec1 = 0.0012f;
        float   m_HairSpecExp1 = 14.40f;

        float   m_HairKSpec2 = 0.136f;
        float   m_HairSpecExp2 = 11.80f;

        // Marschner lighting model parameters 
        float   m_HairRoughness = 0.65f;
        float   m_HairCuticleTilt = 0.08;   // Tilt angle in radians roughly 5-6 degrees tilt

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
        bool    m_EnableThinTip = true;
        bool	m_EnableHairLOD = false;
        bool	m_EnableShadowLOD = false;

        // Legacy settings, replaced by assets. Only reserved as a fallback option.
        AZStd::string m_BaseAlbedoName = "<none>";
        AZStd::string m_StrandAlbedoName = "<none>";

        AZ::Data::Asset<AZ::RPI::StreamingImageAsset> m_baseAlbedoAsset;
        AZ::Data::Asset<AZ::RPI::StreamingImageAsset> m_strandAlbedoAsset;
        bool m_imgDirty = false;	// marks if the image assets require update

    private:
        void OnImgChanged();		// Per image asset handling callback
    };

} // namespace AMD

//#endif // _TRESSFXSIMULATIONPARAMS_H_
