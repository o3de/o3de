// AMD AMDUtils code
// 
// Copyright(c) 2018 Advanced Micro Devices, Inc.All rights reserved.
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files(the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions :
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
#pragma once

#include <d3dCommon.h>
#include "Base/Device.h"
#include "Base/ShaderCompiler.h"

#include <vector>

namespace CAULDRON_DX12
{
    void CreateShaderCache();
    void DestroyShaderCache(Device *pDevice);

    void CompileMacros(const DefineList *pMacros, std::vector<D3D_SHADER_MACRO> *pOut);

    // Does as the function name says and uses a cache
    bool CompileShaderFromString(
        const char *pShaderCode,
        const DefineList* pDefines,
        const char *pEntrypoint,
        const char *pTarget,
        UINT Flags1,
        UINT Flags2,
        D3D12_SHADER_BYTECODE* pOutBytecode);

    bool CompileShaderFromFile(
        const char* pFileName,
        const DefineList* pMacro,
        const char *pEntryPoint,
        const char *pTarget,
        UINT Flags,
        D3D12_SHADER_BYTECODE* pOutBytecode);
}