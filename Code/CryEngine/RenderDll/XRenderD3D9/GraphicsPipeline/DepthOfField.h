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

#pragma once

#include "Common/GraphicsPipelinePass.h"
#include "Common/FullscreenPass.h"
#include "Common/PostProcess/PostEffects.h"
#include "Common/TypedConstantBuffer.h"

class DepthOfFieldPass
    : public GraphicsPipelinePass
{
public:
    virtual ~DepthOfFieldPass() {}

    void Init() override;
    void Shutdown() override;
    void Reset() override;
    void Execute();

private:
    void UpdatePassConstants(const DepthOfFieldParameters& dofParams);
    void UpdateGatherSubPassConstants(AZ::u32 targetWidth, AZ::u32 targetHeight, AZ::u32 squareTapCount);
    void UpdateMinCoCSubPassConstants(AZ::u32 targetWidth, AZ::u32 targetHeight);

    struct PassConstants
    {
        Vec4 m_focusParams0;
        Vec4 m_focusParams1;
        Matrix44 m_reprojection;
    };

    static const AZ::u32 SquareTapSizeMax = 7;

    struct GatherSubPassConstants
    {
        Vec4 m_ScreenSize;
        Vec4 m_taps[SquareTapSizeMax * SquareTapSizeMax];
        Vec4 m_tapCount; //x = tapCount y,z,w = unused        
    };

    struct MinCoCSubPassConstants
    {
        Vec4 m_ScreenSize;
    };

    CTypedConstantBuffer<PassConstants> m_PassConstantBuffer;
    CTypedConstantBuffer<GatherSubPassConstants> m_GatherSubPassConstantBuffer;
    CTypedConstantBuffer<MinCoCSubPassConstants> m_MinCoCSubPassConstantBuffer;
};
