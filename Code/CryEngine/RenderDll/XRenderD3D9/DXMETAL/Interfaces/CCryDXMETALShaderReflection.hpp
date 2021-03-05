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

// Description : Declaration of the DXGL wrappers for D3D11 shader
//               reflection interfaces

#ifndef __CRYMETALGLSHADERREFLECTION__
#define __CRYMETALGLSHADERREFLECTION__

#include "CCryDXMETALBase.hpp"


////////////////////////////////////////////////////////////////////////////////
// CCryDXGLShaderReflectionVariable
////////////////////////////////////////////////////////////////////////////////

class CCryDXGLShaderReflectionVariable
    : public CCryDXGLBase
{
public:
    DXGL_IMPLEMENT_INTERFACE(CCryDXGLShaderReflectionVariable, D3D11ShaderReflectionVariable)
#if DXGL_FULL_EMULATION
    DXGL_IMPLEMENT_INTERFACE(CCryDXGLShaderReflectionVariable, D3D11ShaderReflectionType)
#endif //DXGL_FULL_EMULATION

    CCryDXGLShaderReflectionVariable();
    virtual ~CCryDXGLShaderReflectionVariable();

    bool Initialize(void* pvData);

    // Implementation of ID3D11ShaderReflectionVariable
    HRESULT GetDesc(D3D11_SHADER_VARIABLE_DESC* pDesc);
    ID3D11ShaderReflectionType* GetType();
    ID3D11ShaderReflectionConstantBuffer* GetBuffer();
    UINT GetInterfaceSlot(UINT uArrayIndex);

    // Implementation of ID3D11ShaderReflectionType
    HRESULT GetDesc(D3D11_SHADER_TYPE_DESC* pDesc);
    ID3D11ShaderReflectionType* GetMemberTypeByIndex(UINT Index);
    ID3D11ShaderReflectionType* GetMemberTypeByName(LPCSTR Name);
    LPCSTR GetMemberTypeName(UINT Index);
    HRESULT IsEqual(ID3D11ShaderReflectionType* pType);
    ID3D11ShaderReflectionType* GetSubType();
    ID3D11ShaderReflectionType* GetBaseClass();
    UINT GetNumInterfaces();
    ID3D11ShaderReflectionType* GetInterfaceByIndex(UINT uIndex);
    HRESULT IsOfType(ID3D11ShaderReflectionType* pType);
    HRESULT ImplementsInterface(ID3D11ShaderReflectionType* pBase);

    struct Impl;
    Impl* m_pImpl;
};


////////////////////////////////////////////////////////////////////////////////
// CCryDXGLShaderReflectionConstBuffer
////////////////////////////////////////////////////////////////////////////////

class CCryDXGLShaderReflectionConstBuffer
    : public CCryDXGLBase
{
public:
    DXGL_IMPLEMENT_INTERFACE(CCryDXGLShaderReflectionConstBuffer, D3D11ShaderReflectionConstantBuffer)

    CCryDXGLShaderReflectionConstBuffer();
    virtual ~CCryDXGLShaderReflectionConstBuffer();

    bool Initialize(void* pvData);

    // Implementation of ID3D11ShaderReflectionConstantBuffer
    HRESULT GetDesc(D3D11_SHADER_BUFFER_DESC* pDesc);
    ID3D11ShaderReflectionVariable* GetVariableByIndex(UINT Index);
    ID3D11ShaderReflectionVariable* GetVariableByName(LPCSTR Name);

    struct Impl;
    Impl* m_pImpl;
};


////////////////////////////////////////////////////////////////////////////////
// CCryDXGLShaderReflection
////////////////////////////////////////////////////////////////////////////////

class CCryDXGLShaderReflection
    : public CCryDXGLBase
{
public:
    DXGL_IMPLEMENT_INTERFACE(CCryDXGLShaderReflection, D3D11ShaderReflection)

    CCryDXGLShaderReflection();
    virtual ~CCryDXGLShaderReflection();

    bool Initialize(const void* pvData);

    // Implementation of ID3D11ShaderReflection
    HRESULT GetDesc(D3D11_SHADER_DESC* pDesc);
    ID3D11ShaderReflectionConstantBuffer* GetConstantBufferByIndex(UINT Index);
    ID3D11ShaderReflectionConstantBuffer* GetConstantBufferByName(LPCSTR Name);
    HRESULT GetResourceBindingDesc(UINT ResourceIndex, D3D11_SHADER_INPUT_BIND_DESC* pDesc);
    HRESULT GetInputParameterDesc(UINT ParameterIndex, D3D11_SIGNATURE_PARAMETER_DESC* pDesc);
    HRESULT GetOutputParameterDesc(UINT ParameterIndex, D3D11_SIGNATURE_PARAMETER_DESC* pDesc);
    HRESULT GetPatchConstantParameterDesc(UINT ParameterIndex, D3D11_SIGNATURE_PARAMETER_DESC* pDesc);
    ID3D11ShaderReflectionVariable* GetVariableByName(LPCSTR Name);
    HRESULT GetResourceBindingDescByName(LPCSTR Name, D3D11_SHADER_INPUT_BIND_DESC* pDesc);
    UINT GetMovInstructionCount();
    UINT GetMovcInstructionCount();
    UINT GetConversionInstructionCount();
    UINT GetBitwiseInstructionCount();
    D3D_PRIMITIVE GetGSInputPrimitive();
    BOOL IsSampleFrequencyShader();
    UINT GetNumInterfaceSlots();
    HRESULT GetMinFeatureLevel(enum D3D_FEATURE_LEVEL* pLevel);
    UINT GetThreadGroupSize(UINT* pSizeX, UINT* pSizeY, UINT* pSizeZ);

    struct Impl;
    Impl* m_pImpl;
};

#endif //__CRYMETALGLSHADERREFLECTION__
