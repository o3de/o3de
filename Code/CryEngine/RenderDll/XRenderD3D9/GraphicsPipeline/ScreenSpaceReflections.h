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
#include "Common/UtilityPasses.h"

class CScreenSpaceReflectionsPass
    : public GraphicsPipelinePass
{
public:
    virtual ~CScreenSpaceReflectionsPass() {}

    void Init() override;
    void Shutdown() override;
    void Reset() override;
    void Execute();

private:
    CFullscreenPass    m_passRaytracing;
    CFullscreenPass    m_passComposition;
    CStretchRectPass   m_passCopy;
    CStretchRectPass   m_passDownsample0;
    CStretchRectPass   m_passDownsample1;
    CStretchRectPass   m_passDownsample2;
    CGaussianBlurPass  m_passBlur0;
    CGaussianBlurPass  m_passBlur1;
    CGaussianBlurPass  m_passBlur2;

#if AZ_RENDER_TO_TEXTURE_GEM_ENABLED
    // render to texture supports multiple cameras
    AZStd::unordered_map<AZ::EntityId, Matrix44> m_prevViewProj[MAX_GPU_NUM];
#else
    Matrix44           m_prevViewProj[MAX_GPU_NUM];
#endif // if AZ_RENDER_TO_TEXTURE_GEM_ENABLED
};
