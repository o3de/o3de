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

// Description : Contains portable definition of structs and enums from
//               Direct3D 9 still required by the Direct3D 11 renderer

#ifndef __CRYMETALGLLEGACY__
#define __CRYMETALGLLEGACY__

#define D3D10_SHADER_DEBUG                              D3DCOMPILE_DEBUG
#define D3D10_SHADER_SKIP_VALIDATION                    D3DCOMPILE_SKIP_VALIDATION
#define D3D10_SHADER_SKIP_OPTIMIZATION                  D3DCOMPILE_SKIP_OPTIMIZATION
#define D3D10_SHADER_PACK_MATRIX_ROW_MAJOR              D3DCOMPILE_PACK_MATRIX_ROW_MAJOR
#define D3D10_SHADER_PACK_MATRIX_COLUMN_MAJOR           D3DCOMPILE_PACK_MATRIX_COLUMN_MAJOR
#define D3D10_SHADER_PARTIAL_PRECISION                  D3DCOMPILE_PARTIAL_PRECISION
#define D3D10_SHADER_FORCE_VS_SOFTWARE_NO_OPT           D3DCOMPILE_FORCE_VS_SOFTWARE_NO_OPT
#define D3D10_SHADER_FORCE_PS_SOFTWARE_NO_OPT           D3DCOMPILE_FORCE_PS_SOFTWARE_NO_OPT
#define D3D10_SHADER_NO_PRESHADER                       D3DCOMPILE_NO_PRESHADER
#define D3D10_SHADER_AVOID_FLOW_CONTROL                 D3DCOMPILE_AVOID_FLOW_CONTROL
#define D3D10_SHADER_PREFER_FLOW_CONTROL                D3DCOMPILE_PREFER_FLOW_CONTROL
#define D3D10_SHADER_ENABLE_STRICTNESS                  D3DCOMPILE_ENABLE_STRICTNESS
#define D3D10_SHADER_ENABLE_BACKWARDS_COMPATIBILITY     D3DCOMPILE_ENABLE_BACKWARDS_COMPATIBILITY
#define D3D10_SHADER_IEEE_STRICTNESS                    D3DCOMPILE_IEEE_STRICTNESS
#define D3D10_SHADER_WARNINGS_ARE_ERRORS                D3DCOMPILE_WARNINGS_ARE_ERRORS

#define D3DCOLOR_ARGB(a, r, g, b) ((D3DCOLOR)((((a) & 0xff) << 24) | (((r) & 0xff) << 16) | (((g) & 0xff) << 8) | ((b) & 0xff)))
#define D3DCOLOR_RGBA(r, g, b, a) D3DCOLOR_ARGB(a, r, g, b)
#define D3DCOLOR_XRGB(r, g, b)   D3DCOLOR_ARGB(0xff, r, g, b)

#define D3DSTREAMSOURCE_INDEXEDDATA  (1 << 30)
#define D3DSTREAMSOURCE_INSTANCEDATA (2 << 30)

typedef uint32 D3DCOLOR;

typedef uint32 D3DFORMAT;

typedef ID3D11Device ID3D10Device;
typedef ID3D10Blob* LPD3D10BLOB;
typedef D3D_SHADER_MACRO D3D10_SHADER_MACRO;
typedef D3D10_SHADER_MACRO* LPD3D10_SHADER_MACRO;
typedef D3D_SHADER_MACRO D3D11_SHADER_MACRO;
typedef D3D10_SHADER_MACRO* LPD3D11_SHADER_MACRO;

typedef struct ID3DInclude ID3D10Include;
typedef struct ID3DInclude* LPD3D10INCLUDE;

#define IID_ID3D11ShaderReflection __uuidof(ID3D11ShaderReflection)

typedef
    enum D3D10_RESOURCE_DIMENSION
{
    D3D10_RESOURCE_DIMENSION_UNKNOWN    = 0,
    D3D10_RESOURCE_DIMENSION_BUFFER = 1,
    D3D10_RESOURCE_DIMENSION_TEXTURE1D  = 2,
    D3D10_RESOURCE_DIMENSION_TEXTURE2D  = 3,
    D3D10_RESOURCE_DIMENSION_TEXTURE3D  = 4
}   D3D10_RESOURCE_DIMENSION;


#endif //__CRYMETALGLLEGACY__

