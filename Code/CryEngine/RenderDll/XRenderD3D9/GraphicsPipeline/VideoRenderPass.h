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

#pragma once

#include "Common/GraphicsPipelinePass.h"
#include "TypedConstantBuffer.h"

// Renders video data to a texture. Video data can be provided as any number of texture planes,
// the textures are composited together based on inputs passed in. See IVideoRenderer.h for more information.
class VideoRenderPass
    : public GraphicsPipelinePass
{
public:
    VideoRenderPass();
    ~VideoRenderPass();

    void Init() override;
    void Shutdown() override;
    void Reset() override;
    void Execute(const AZ::VideoRenderer::DrawArguments& drawArguments);

protected:
    struct VideoPassConstants
    {
        Vec4 VideoTexture0Scale;
        Vec4 VideoTexture1Scale;
        Vec4 VideoTexture2Scale;
        Vec4 VideoTexture3Scale;
        Vec4 VideoColorAdjustment;
    };

    CTypedConstantBuffer<VideoPassConstants> m_passConstants;
    int m_samplerState;
};

