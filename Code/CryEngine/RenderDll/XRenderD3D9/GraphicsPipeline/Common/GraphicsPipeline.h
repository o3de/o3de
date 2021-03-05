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

#include "GraphicsPipelinePass.h"

class CGraphicsPipeline
{
public:
    virtual ~CGraphicsPipeline();

    // Allocate resources needed by the pipeline & passes
    virtual void Init() = 0;
    // Free resources needed by the pipeline & passes
    virtual void Shutdown() = 0;

    // Prepare all passes before actual drawing starts
    virtual void Prepare() = 0;
    // Execute the pipeline and its passes
    virtual void Execute() = 0;

    // Reset all render passes and their PSOs
    // Needed if shaders need to be reloaded
    virtual void Reset() = 0;

protected:
    template<class T>
    void RegisterPass(T*& pPass)
    {
        pPass = new T();
        pPass->Init();
        m_passes.emplace_back(pPass);
    }

protected:
    std::vector<std::unique_ptr<GraphicsPipelinePass>>  m_passes;
};
