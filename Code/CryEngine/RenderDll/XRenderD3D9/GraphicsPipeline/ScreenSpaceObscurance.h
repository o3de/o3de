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


class CScreenSpaceObscurancePass
    : public GraphicsPipelinePass
{
public:
    virtual ~CScreenSpaceObscurancePass() {}

    void Init() override;
    void Shutdown() override;
    void Reset() override;
    void Execute();

private:
    CFullscreenPass    m_passObscurance;
    CFullscreenPass    m_passFilter;
    CStretchRectPass   m_passAlbedoDownsample0;
    CStretchRectPass   m_passAlbedoDownsample1;
    CStretchRectPass   m_passAlbedoDownsample2;
    CGaussianBlurPass  m_passAlbedoBlur;
};
