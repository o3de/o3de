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
#include "RenderDll_precompiled.h"
#include "CCryDX12Shader.hpp"

CCryDX12Shader* CCryDX12Shader::Create(CCryDX12Device* pDevice, const void* pShaderBytecode, SIZE_T BytecodeLength, [[maybe_unused]] ID3D11ClassLinkage* pClassLinkage)
{
    CCryDX12Shader* pResult = DX12::PassAddRef(new CCryDX12Shader(pShaderBytecode, BytecodeLength));
    pResult->m_pShader = DX12::Shader::CreateFromD3D11(pDevice->GetDX12Device(), pResult->GetD3D12ShaderBytecode());

    return pResult;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

CCryDX12Shader::CCryDX12Shader(const void* pShaderBytecode, SIZE_T BytecodeLength)
    : Super(nullptr, nullptr)
    , m_pShaderBytecodeData(NULL)
{
    if (pShaderBytecode && BytecodeLength)
    {
        m_pShaderBytecodeData = new uint8_t[BytecodeLength];
        memcpy(m_pShaderBytecodeData, pShaderBytecode, BytecodeLength);

        m_ShaderBytecode12.pShaderBytecode = m_pShaderBytecodeData;
        m_ShaderBytecode12.BytecodeLength = BytecodeLength;
    }
    else
    {
        m_ShaderBytecode12.pShaderBytecode = NULL;
        m_ShaderBytecode12.BytecodeLength = 0;
    }
}

CCryDX12Shader::~CCryDX12Shader()
{
    if (m_pShaderBytecodeData)
    {
        delete [] m_pShaderBytecodeData;
    }
}
