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
#include "CCryDX12InputLayout.hpp"

CCryDX12InputLayout* CCryDX12InputLayout::Create([[maybe_unused]] CCryDX12Device* pDevice, const D3D11_INPUT_ELEMENT_DESC* pInputElementDescs11, UINT NumElements, [[maybe_unused]] const void* pShaderBytecodeWithInputSignature, [[maybe_unused]] SIZE_T BytecodeLength)
{
    CCryDX12InputLayout* pResult = DX12::PassAddRef(new CCryDX12InputLayout());
    pResult->m_Descriptors.resize(NumElements);

    for (UINT i = 0; i < NumElements; ++i)
    {
        pResult->m_SemanticNames.push_back(pInputElementDescs11[i].SemanticName);
    }

    for (UINT i = 0; i < NumElements; ++i)
    {
        D3D12_INPUT_ELEMENT_DESC& desc12 = pResult->m_Descriptors[i];
        ZeroMemory(&desc12, sizeof(D3D12_INPUT_ELEMENT_DESC));

        desc12.SemanticIndex = pInputElementDescs11[i].SemanticIndex;
        desc12.Format = pInputElementDescs11[i].Format;
        desc12.InputSlot = pInputElementDescs11[i].InputSlot;
        desc12.AlignedByteOffset = pInputElementDescs11[i].AlignedByteOffset;
        desc12.InputSlotClass = static_cast<D3D12_INPUT_CLASSIFICATION>(pInputElementDescs11[i].InputSlotClass);
        desc12.InstanceDataStepRate = pInputElementDescs11[i].InstanceDataStepRate;

        // Hash everything except the pointers. Hashing pointers would make the hash inconsistent between multiple program runs.
        pResult->m_hash.Add(&desc12, sizeof(desc12));

        desc12.SemanticName = pResult->m_SemanticNames[i].c_str();
    }

    return pResult;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

CCryDX12InputLayout::CCryDX12InputLayout()
    : Super(nullptr, nullptr)
{
}

CCryDX12InputLayout::~CCryDX12InputLayout()
{
}
