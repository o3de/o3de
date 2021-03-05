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

// Description : Contains portable definition of structs and enums to match
//               those in D3DCompiler.h in the DirectX SDK


#ifndef CRYINCLUDE_CRYENGINE_RENDERDLL_XRENDERD3D9_DXGL_DEFINITIONS_DXGL_D3DCOMPILER_H
#define CRYINCLUDE_CRYENGINE_RENDERDLL_XRENDERD3D9_DXGL_DEFINITIONS_DXGL_D3DCOMPILER_H
#pragma once


#include "DXGL_D3D11Shader.h"

#define D3DCOMPILE_DEBUG                          (1 << 0)
#define D3DCOMPILE_SKIP_VALIDATION                (1 << 1)
#define D3DCOMPILE_SKIP_OPTIMIZATION              (1 << 2)
#define D3DCOMPILE_PACK_MATRIX_ROW_MAJOR          (1 << 3)
#define D3DCOMPILE_PACK_MATRIX_COLUMN_MAJOR       (1 << 4)
#define D3DCOMPILE_PARTIAL_PRECISION              (1 << 5)
#define D3DCOMPILE_FORCE_VS_SOFTWARE_NO_OPT       (1 << 6)
#define D3DCOMPILE_FORCE_PS_SOFTWARE_NO_OPT       (1 << 7)
#define D3DCOMPILE_NO_PRESHADER                   (1 << 8)
#define D3DCOMPILE_AVOID_FLOW_CONTROL             (1 << 9)
#define D3DCOMPILE_PREFER_FLOW_CONTROL            (1 << 10)
#define D3DCOMPILE_ENABLE_STRICTNESS              (1 << 11)
#define D3DCOMPILE_ENABLE_BACKWARDS_COMPATIBILITY (1 << 12)
#define D3DCOMPILE_IEEE_STRICTNESS                (1 << 13)
#define D3DCOMPILE_OPTIMIZATION_LEVEL0            (1 << 14)
#define D3DCOMPILE_OPTIMIZATION_LEVEL1            0
#define D3DCOMPILE_OPTIMIZATION_LEVEL2            ((1 << 14) | (1 << 15))
#define D3DCOMPILE_OPTIMIZATION_LEVEL3            (1 << 15)
#define D3DCOMPILE_RESERVED16                     (1 << 16)
#define D3DCOMPILE_RESERVED17                     (1 << 17)
#define D3DCOMPILE_WARNINGS_ARE_ERRORS            (1 << 18)

typedef HRESULT (WINAPI * pD3DCompile)
    (LPCVOID                         pSrcData,
    SIZE_T                          SrcDataSize,
    LPCSTR                          pFileName,
    CONST D3D_SHADER_MACRO*         pDefines,
    ID3DInclude*                    pInclude,
    LPCSTR                          pEntrypoint,
    LPCSTR                          pTarget,
    UINT                            Flags1,
    UINT                            Flags2,
    ID3DBlob**                      ppCode,
    ID3DBlob**                      ppErrorMsgs);

#endif // CRYINCLUDE_CRYENGINE_RENDERDLL_XRENDERD3D9_DXGL_DEFINITIONS_DXGL_D3DCOMPILER_H
