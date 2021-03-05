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

// This class is deprecated as it requires r_graphicsPipeline > 0 to function properly, which is not supported on all platforms.
class CMotionBlurPass
    : public GraphicsPipelinePass
{
public:
    virtual ~CMotionBlurPass() {}

    void Init() override;
    void Shutdown() override;
    void Reset() override;
    void Execute();

private:
    float ComputeMotionScale();

private:
    CFullscreenPass   m_passPacking;
    CFullscreenPass   m_passTileGen1;
    CFullscreenPass   m_passTileGen2;
    CFullscreenPass   m_passNeighborMax;
    CStretchRectPass  m_passCopy;
    CFullscreenPass   m_passMotionBlur;
};
