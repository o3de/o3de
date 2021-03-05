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

#include "../Common/DevBuffer.h"

class CD3D9Renderer;

class MultiLayerAlphaBlendPass
{

public:

    static void InstallInstance();
    static void ReleaseInstance();
    static MultiLayerAlphaBlendPass& GetInstance();

    bool IsSupported();
    bool SetLayerCount(uint32_t count);
    uint32_t GetLayerCount();

    void ConfigureShaderFlags(uint64& flags);

    void BindResources();
    void UnBindResources();
    void Resolve(CD3D9Renderer& renderer);

    static const uint32 MAX_LAYERS;

protected:

    MultiLayerAlphaBlendPass();
    ~MultiLayerAlphaBlendPass();

private:

    static MultiLayerAlphaBlendPass* s_pInstance; // TODO: This (and related singleton functions) should be removed when there is a system in place for managing passes.

    enum class SupportLevel
    {
        NOT_SUPPORTED,
        SUPPORTED,
        UNKNOWN
    };

    WrappedDX11Buffer  m_alphaLayersBuffer;
    uint32_t m_layerCount;
    SupportLevel m_supported;
    
};
