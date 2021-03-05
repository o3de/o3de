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
#pragma once
#ifndef __CCRYDX12INPUTLAYOUT__
#define __CCRYDX12INPUTLAYOUT__

#include "DX12/Device/CCryDX12DeviceChild.hpp"

class CCryDX12InputLayout
    : public CCryDX12DeviceChild<ID3D11InputLayout>
{
public:
    DX12_OBJECT(CCryDX12InputLayout, CCryDX12DeviceChild<ID3D11InputLayout>);

    static CCryDX12InputLayout* Create(CCryDX12Device* pDevice, const D3D11_INPUT_ELEMENT_DESC* pInputElementDescs11, UINT NumElements, const void* pShaderBytecodeWithInputSignature, SIZE_T BytecodeLength);

    virtual ~CCryDX12InputLayout();

    typedef std::vector<D3D12_INPUT_ELEMENT_DESC> TDescriptors;

    const TDescriptors& GetDescriptors() const
    {
        return m_Descriptors;
    }

    AZ::u32 GetHash() const
    {
        return m_hash;
    }

protected:
    CCryDX12InputLayout();

private:
    TDescriptors m_Descriptors;
    std::vector<std::string> m_SemanticNames;
    AZ::Crc32 m_hash;
};

#endif // __CCRYDX12INPUTLAYOUT__
