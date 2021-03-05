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
#ifndef __CCRYDX12SHADER__
#define __CCRYDX12SHADER__

#include "DX12/Device/CCryDX12DeviceChild.hpp"
#include "DX12/API/DX12Shader.hpp"

class CCryDX12Shader
    : public CCryDX12DeviceChild<ID3D11DeviceChild>
{
public:
    DX12_OBJECT(CCryDX12Shader, CCryDX12DeviceChild<ID3D11DeviceChild>);

    static CCryDX12Shader* Create(CCryDX12Device* pDevice, const void* pShaderBytecode, SIZE_T BytecodeLength, ID3D11ClassLinkage* pClassLinkage);

    virtual ~CCryDX12Shader();

    DX12::Shader* GetDX12Shader() const
    {
        return m_pShader;
    }

    const D3D12_SHADER_BYTECODE& GetD3D12ShaderBytecode() const
    {
        return m_ShaderBytecode12;
    }

protected:
    CCryDX12Shader(const void* pShaderBytecode, SIZE_T BytecodeLength);

private:
    DX12::SmartPtr<DX12::Shader> m_pShader;

    uint8_t* m_pShaderBytecodeData;

    D3D12_SHADER_BYTECODE m_ShaderBytecode12;
};

#endif // __CCRYDX12SHADER__
