/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates, or 
* a third party where indicated.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,  
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  
*
*/

// Original file Copyright Crytek GMBH or its affiliates, used under license.
#ifndef _PER_FRAME_RESOURCE_GROUP_
#define _PER_FRAME_RESOURCE_GROUP_

struct PerFrameParameters
{
    int m_FrameID;

    Vec3 m_WaterLevel;
    Vec4 m_HDRParams;

    float m_SunSpecularMultiplier;
    float m_MidDayIndicator;      // during day [0..1] where noon is 1.0 and going towards 0 at night

    Vec3 m_DecalZFightingRemedy;

    Vec3 m_CloudShadingColorSun;
    Vec3 m_CloudShadingColorSky;
    Vec4 m_CloudShadowParams;
    Vec4 m_CloudShadowAnimParams;

    Vec4 m_CausticsParams;
    Vec3 m_CausticsSunDirection;

    Vec4 m_VolumetricFogParams;
    Vec4 m_VolumetricFogRampParams;
    Vec4 m_VolumetricFogColorGradientBase;
    Vec4 m_VolumetricFogColorGradientDelta;
    Vec4 m_VolumetricFogColorGradientParams;
    Vec4 m_VolumetricFogColorGradientRadial;
    Vec4 m_VolumetricFogSamplingParams;
    Vec4 m_VolumetricFogDistributionParams;
    Vec4 m_VolumetricFogScatteringParams;
    Vec4 m_VolumetricFogScatteringBlendParams;
    Vec4 m_VolumetricFogScatteringColor;
    Vec4 m_VolumetricFogScatteringSecondaryColor;
    Vec4 m_VolumetricFogHeightDensityParams;
    Vec4 m_VolumetricFogHeightDensityRampParams;
    Vec4 m_VolumetricFogDistanceParams;
};

#endif // _PER_FRAME_RESOURCE_GROUP_