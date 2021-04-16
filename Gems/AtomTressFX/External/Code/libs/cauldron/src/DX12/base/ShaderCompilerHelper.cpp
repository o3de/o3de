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

#include "stdafx.h"
#include <D3DCompiler.h>
#include "ShaderCompilerHelper.h"
#include "Misc/Misc.h"
#include "Misc/Cache.h"
#include <dxcapi.h>

namespace CAULDRON_DX12
{
#define SHADER_LIB_DIR "ShaderLibDX"
#define SHADER_CACHE_DIR SHADER_LIB_DIR"\\ShaderCacheDX"

#define USE_MULTITHREADED_CACHE 
#define USE_SPIRV_FROM_DISK   


    interface Includer : public ID3DInclude
    {
    public:
        virtual ~Includer() {}

        HRESULT Open(D3D_INCLUDE_TYPE IncludeType, LPCSTR pFileName, LPCVOID pParentData, LPCVOID *ppData, UINT *pBytes)
        {
            char fullpath[1024];
            sprintf_s(fullpath, SHADER_LIB_DIR"\\%s", pFileName);
            return ReadFile(fullpath, (char**)ppData, (size_t*)pBytes, false) ? S_OK : E_FAIL;
        }
        HRESULT Close(LPCVOID pData)
        {
            free((void*)pData);
            return S_OK;
        }
    };

    interface IncluderDxc : public IDxcIncludeHandler
    {
        IDxcLibrary *m_pLibrary; 
    public:
        IncluderDxc(IDxcLibrary *pLibrary) : m_pLibrary(pLibrary) {}
        HRESULT QueryInterface(const IID &, void **) { return S_OK; }
        ULONG AddRef() { return 0; }
        ULONG Release() { return 0; }
        HRESULT LoadSource(LPCWSTR pFilename, IDxcBlob **ppIncludeSource)
        {
            char fullpath[1024];
            sprintf_s<1024>(fullpath, (SHADER_LIB_DIR"\\%S"), pFilename);

            LPCVOID pData;
            size_t bytes;
            HRESULT hr = ReadFile(fullpath, (char**)&pData, (size_t*)&bytes, false) ? S_OK : E_FAIL;

            IDxcBlobEncoding *pSource;
            ThrowIfFailed(m_pLibrary->CreateBlobWithEncodingFromPinned((LPBYTE)pData, (UINT32)bytes, CP_UTF8, &pSource));

            *ppIncludeSource = pSource;

            return S_OK;
        }
    };

    bool DXCompileToDXO(size_t hash,
        const char *pSrcCode,
        const DefineList* pDefines,
        const char *pEntryPoint,
        const char *pTarget,
        UINT Flags1,
        UINT Flags2,
        char **outSpvData,
        size_t *outSpvSize)
    {
        std::string filenameSpv = format(SHADER_CACHE_DIR"\\%p.dxo", hash);

#ifdef USE_SPIRV_FROM_DISK
        //std::string shader = GenerateSource(sourceType, shader_type, pshader, pEntryPoint, pDefines);
        if (ReadFile(filenameSpv.c_str(), outSpvData, outSpvSize, true) == true)
        {
            Trace(format("thread 0x%04x compile: %p disk\n", GetCurrentThreadId(), hash));
            return true;
        }
#endif
        // create hlsl file for shader compiler to compile
        //
        std::string filenameHlsl = format(SHADER_CACHE_DIR"\\%p.hlsl", hash);
        std::ofstream ofs(filenameHlsl, std::ofstream::out);
        ofs << pSrcCode;
        ofs.close();

        // Choose compiler depending on the shader model
        //
        if (isdigit(pTarget[3]) && pTarget[3] - '0' < 6)
        {
            std::string filenamePdb = format(SHADER_CACHE_DIR"\\%p.pdb", hash);
            std::vector<D3D_SHADER_MACRO> macros;
            CompileMacros(pDefines, &macros);
            macros.push_back(D3D_SHADER_MACRO{ NULL, NULL });

            Includer Include;
            Microsoft::WRL::ComPtr<ID3DBlob> pError1, pCode1; // ComPtr auto-releases
            HRESULT res1 = D3DPreprocess(pSrcCode, strlen(pSrcCode), NULL, macros.data(), &Include, pCode1.GetAddressOf(), pError1.GetAddressOf());
            if (res1 == S_OK)
            {
                SaveFile(filenameHlsl.c_str(), pCode1->GetBufferPointer(), pCode1->GetBufferSize(), false);

                Microsoft::WRL::ComPtr<ID3DBlob> pError, pCode; // ComPtr auto-releases
                HRESULT res = D3DCompile(pCode1->GetBufferPointer(), pCode1->GetBufferSize(), NULL, NULL, NULL, pEntryPoint, pTarget, Flags1, Flags2, pCode.GetAddressOf(), pError.GetAddressOf());
                if (res == S_OK)
                {
                    *outSpvSize = pCode->GetBufferSize();
                    *outSpvData = (char *)malloc(*outSpvSize);

                    memcpy(*outSpvData, pCode->GetBufferPointer(), *outSpvSize);

                    // Check if we have debug information
                    Microsoft::WRL::ComPtr<ID3DBlob> pPDB;
                    if (D3DGetBlobPart(pCode->GetBufferPointer(), pCode->GetBufferSize(), D3D_BLOB_PDB, 0, pPDB.GetAddressOf()) == S_OK)
                    {
                        // Now retrieve the suggested name for the debug data file if we have one (Requires D3DCOMPILE_DEBUG_NAME_FOR_SOURCE or D3DCOMPILE_DEBUG_NAME_FOR_BINARY be passed in as flags)
                        Microsoft::WRL::ComPtr<ID3DBlob> pPDBName;
                        if (D3DGetBlobPart(pCode->GetBufferPointer(), pCode->GetBufferSize(), D3D_BLOB_DEBUG_NAME, 0, pPDBName.GetAddressOf()) == S_OK)
                        {
                            // This struct represents the first four bytes of the name blob:
                            struct ShaderDebugName
                            {
                                uint16_t Flags;       // Reserved, must be set to zero.
                                uint16_t NameLength;  // Length of the debug name, without null terminator.
                                                      // Followed by NameLength bytes of the UTF-8-encoded name.
                                                      // Followed by a null terminator.
                                                      // Followed by [0-3] zero bytes to align to a 4-byte boundary.
                            };

                            auto pDebugNameData = reinterpret_cast<const ShaderDebugName*>(pPDBName->GetBufferPointer());
                            auto pName = reinterpret_cast<const char*>(pDebugNameData + 1);

                            // Now write the contents of the blob pPDB to a file named the value of pName
                            filenamePdb = format(SHADER_CACHE_DIR"\\%s", pName);
                        }

                        // Save the pdb with the proper filename
                        SaveFile(filenamePdb.c_str(), pPDB->GetBufferPointer(), pPDB->GetBufferSize(), true);
                    }

                    SaveFile(filenameSpv.c_str(), *outSpvData, *outSpvSize, true);
                    return true;
                }
                else
                {
                    const char *msg = (const char *)pError->GetBufferPointer();
                    std::string err = format("*** Error compiling %p.hlsl ***\n%s\n", hash, msg);
                    Trace(err);

                    MessageBoxA(0, err.c_str(), "Error", 0);
                }
            }
            else
            {
                const char *msg = (const char *)pError1->GetBufferPointer();
                std::string err = format("*** Error preprocessing %p.hlsl ***\n%s\n", hash, msg);

                MessageBoxA(0, err.c_str(), "Error", 0);
            }
        }
        else
        {       
            std::string filenamePdb = format(SHADER_CACHE_DIR"\\%p.lld", hash);
            wchar_t names[50][128];
            wchar_t values[50][128];
            DxcDefine defines[50];
            int defineCount = 0;
            for (auto it = pDefines->begin(); it != pDefines->end(); it++)
            {
                swprintf_s<128>(names[defineCount], L"%S", it->first.c_str());
                swprintf_s<128>(values[defineCount], L"%S", it->second.c_str());
                defines[defineCount].Name = names[defineCount];
                defines[defineCount].Value = values[defineCount];
                defineCount++;
            }

            IDxcLibrary *pLibrary;
            ThrowIfFailed(DxcCreateInstance(CLSID_DxcLibrary, IID_PPV_ARGS(&pLibrary)));

            IDxcBlobEncoding *pSource;
            ThrowIfFailed(pLibrary->CreateBlobWithEncodingFromPinned((LPBYTE)pSrcCode, (UINT32)strlen(pSrcCode), CP_UTF8, &pSource));

            IDxcCompiler2* pCompiler;
            ThrowIfFailed(DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&pCompiler)));

            IncluderDxc Includer(pLibrary);

            wchar_t debugArg[] = L"/Zi";
            wchar_t debugLevel0[] = L"/O0";
            wchar_t debugSkipOptimization[] = L"/0d";
            wchar_t debugSource[] = L"/Zss";
            wchar_t debugBinary[] = L"/Zsb";

            std::vector<LPCWSTR> ppArgs;

            if (Flags1 & D3DCOMPILE_DEBUG)
                ppArgs.push_back(debugArg);

            if (Flags1 & D3DCOMPILE_OPTIMIZATION_LEVEL0)
                ppArgs.push_back(debugLevel0);

            if (Flags1 & D3DCOMPILE_SKIP_OPTIMIZATION)
                ppArgs.push_back(debugSkipOptimization);

            // These are mutually exclusive ...
            if (Flags1 & D3DCOMPILE_DEBUG_NAME_FOR_SOURCE)
                ppArgs.push_back(debugSource);
            else if (Flags1 & D3DCOMPILE_DEBUG_NAME_FOR_BINARY)
                ppArgs.push_back(debugBinary);

            wchar_t  pEntryPointW[256];
            swprintf_s<256>(pEntryPointW, L"%S", pEntryPoint);

            wchar_t  pTargetW[256];
            swprintf_s<256>(pTargetW, L"%S", pTarget);

            IDxcOperationResult *pResultPre;
            HRESULT res1 = pCompiler->Preprocess(pSource, L"", NULL, 0, defines, defineCount, &Includer, &pResultPre);
            if (res1 == S_OK)
            {
                Microsoft::WRL::ComPtr<IDxcBlob> pCode1;
                pResultPre->GetResult(pCode1.GetAddressOf());
                SaveFile(filenameHlsl.c_str(), pCode1->GetBufferPointer(), pCode1->GetBufferSize(), false);

                Microsoft::WRL::ComPtr<IDxcBlob> pPDB;
                IDxcOperationResult* pOpRes;
                wchar_t* pPDBNameW;
                HRESULT res;

                if (Flags1 & D3DCOMPILE_DEBUG)
                    res = pCompiler->CompileWithDebug(pSource, NULL, pEntryPointW, pTargetW, ppArgs.data(), (UINT32)ppArgs.size(), defines, defineCount, &Includer, &pOpRes, &pPDBNameW, pPDB.GetAddressOf());
                else
                    res = pCompiler->Compile(pSource, NULL, pEntryPointW, pTargetW, ppArgs.data(), (UINT32)ppArgs.size(), defines, defineCount, &Includer, &pOpRes);

                if (pPDB)
                {
                    if (pPDBNameW)
                    {
                        // Setup the correct name for the PDB
                        char pPDBName[512];
                        size_t nameSize;
                        wcstombs_s(&nameSize, pPDBName, 512 * sizeof(byte), pPDBNameW, wcslen(pPDBNameW));
                        filenamePdb = format(SHADER_CACHE_DIR"\\%s", pPDBName);
                    }

                    // Now write the contents of the blob pPDB to a file named the value of pName
                    SaveFile(filenamePdb.c_str(), pPDB->GetBufferPointer(), pPDB->GetBufferSize(), true);
                }

                pSource->Release();
                pLibrary->Release();
                pCompiler->Release();

                IDxcBlob *pResult = NULL;
                IDxcBlobEncoding *pError = NULL;
                if (pOpRes != NULL)
                {
                    pOpRes->GetResult(&pResult);
                    pOpRes->GetErrorBuffer(&pError);
                    pOpRes->Release();
                }

                if (pResult!=NULL && pResult->GetBufferSize()>0)
                {
                    *outSpvSize = pResult->GetBufferSize();
                    *outSpvData = (char *)malloc(*outSpvSize);

                    memcpy(*outSpvData, pResult->GetBufferPointer(), *outSpvSize);

                    pResult->Release();

                    SaveFile(filenameSpv.c_str(), *outSpvData, *outSpvSize, true);

                    return true;
                }
                else
                {
                    IDxcBlobEncoding *pErrorUtf8;
                    pLibrary->GetBlobAsUtf8(pError, &pErrorUtf8);

                    Trace("*** Error compiling %p.hlsl ***\n", hash);

                    std::string filenameErr = format(SHADER_CACHE_DIR"\\%p.err", hash);
                    SaveFile(filenameErr.c_str(), pErrorUtf8->GetBufferPointer(), pErrorUtf8->GetBufferSize(), false);
                    
                    std::string errMsg = std::string((char*)pErrorUtf8->GetBufferPointer(), pErrorUtf8->GetBufferSize());
                    Trace(errMsg);

                    pErrorUtf8->Release();
                }
            }
        }

        return false;
    }

    Cache<D3D12_SHADER_BYTECODE> s_shaderCache;

    void DestroyShadersInTheCache()
    {
        auto *database = s_shaderCache.GetDatabase();
        for (auto it = database->begin(); it != database->end(); it++)
        {
            free((void*)(it->second.m_data.pShaderBytecode));
        }
    }

    bool DXCompile(const char *pSrcCode,
        const DefineList* pDefines,
        const char *pEntryPoint,
        const char *pTarget,
        UINT Flags1,
        UINT Flags2,
        D3D12_SHADER_BYTECODE* pOutBytecode)
    {
        //compute hash
        //
        size_t hash;
        hash = HashShaderString(SHADER_LIB_DIR"\\", pSrcCode);
        hash = Hash(pEntryPoint, strlen(pEntryPoint), hash);
        hash = Hash(pTarget, strlen(pTarget), hash);
        if (pDefines != NULL)
        {
            hash = pDefines->Hash(hash);
        }

#ifdef USE_MULTITHREADED_CACHE
        // Compile if not in cache
        //
        if (s_shaderCache.CacheMiss(hash, pOutBytecode))
#endif
        {
            char *SpvData = NULL;
            size_t SpvSize = 0;

#ifdef USE_SPIRV_FROM_DISK
            std::string filenameSpv = format(SHADER_CACHE_DIR"\\%p.dxo", hash);
            if (ReadFile(filenameSpv.c_str(), &SpvData, &SpvSize, true) == false)
#endif
            {
                DXCompileToDXO(hash, pSrcCode, pDefines, pEntryPoint, pTarget, Flags1, Flags2, &SpvData, &SpvSize);
            }

            assert(SpvSize != 0);
            pOutBytecode->BytecodeLength = SpvSize;
            pOutBytecode->pShaderBytecode = SpvData;

#ifdef USE_MULTITHREADED_CACHE
            s_shaderCache.UpdateCache(hash, pOutBytecode);
#endif
        }

        return true;
    }

    bool CompileShaderFromString(
        const char *pShaderCode,
        const DefineList* pDefines,
        const char *pEntrypoint,
        const char *pTarget,
        UINT Flags1,
        UINT Flags2,
        D3D12_SHADER_BYTECODE* pOutBytecode)
    {
        assert(strlen(pShaderCode) > 0);

        return DXCompile(pShaderCode, pDefines, pEntrypoint, pTarget, Flags1, Flags2, pOutBytecode);
    }

    bool CompileShaderFromFile(
        const char *pFilename,
        const DefineList* pDefines,
        const char *pEntryPoint,
        const char *pTarget,
        UINT Flags,
        D3D12_SHADER_BYTECODE* pOutBytecode)
    {
        char *pShaderCode;
        size_t size;

        //append path
        char fullpath[1024];
        sprintf_s(fullpath, SHADER_LIB_DIR"\\%s", pFilename);

        if (ReadFile(fullpath, &pShaderCode, &size, false))
        {
            return CompileShaderFromString(pShaderCode, pDefines, pEntryPoint, pTarget, Flags, 0, pOutBytecode);
        }

        assert(!"Some of the shaders have not been copied to the bin folder, try rebuilding the solution.");

        return false;
    }


    void CreateShaderCache()
    {        
        CreateDirectoryA(SHADER_CACHE_DIR, 0);
    }

    void DestroyShaderCache(Device *pDevice)
    {
        pDevice;
        DestroyShadersInTheCache();
    }

    void CompileMacros(const DefineList *pMacros, std::vector<D3D_SHADER_MACRO> *pOut)
    {
        if (pMacros != NULL)
        {
            for (auto it = pMacros->begin(); it != pMacros->end(); it++)
            {
                D3D_SHADER_MACRO macro;
                macro.Name = it->first.c_str();
                macro.Definition = it->second.c_str();
                pOut->push_back(macro);
            }
        }
    }
}