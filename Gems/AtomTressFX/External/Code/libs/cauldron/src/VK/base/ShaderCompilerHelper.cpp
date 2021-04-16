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
#include "ShaderCompilerHelper.h"

#include "Misc/Misc.h"
#include "Misc/Cache.h"

namespace CAULDRON_VK
{
#define SHADER_LIB_DIR "ShaderLibVK"
#define SHADER_CACHE_DIR SHADER_LIB_DIR"\\ShaderCacheVK"

    //
    // Compiles a shader into SpirV
    //
    bool VKCompileToSpirv(size_t hash, ShaderSourceType sourceType, const VkShaderStageFlagBits shader_type, const std::string &shaderCode, const char *pEntryPoint, const DefineList *pDefines, char **outSpvData, size_t *outSpvSize)
    {
        std::string filenameSpv = format(SHADER_CACHE_DIR"\\%p.spv", hash);

        // create glsl file for shader compiler to compile
        //
        std::string filenameGlsl;
        if (sourceType == SST_GLSL)
            filenameGlsl = format(SHADER_CACHE_DIR"\\%p.glsl", hash);
        else if (sourceType == SST_HLSL)
            filenameGlsl = format(SHADER_CACHE_DIR"\\%p.hlsl", hash);
        else
            assert(!"unknown shader extension");

        std::ofstream ofs(filenameGlsl, std::ofstream::out);
        ofs << shaderCode;
        ofs.close();

        // compute command line to invoke the shader compiler
        //
        char *stage = NULL;
        switch (shader_type)
        {
        case VK_SHADER_STAGE_VERTEX_BIT:  stage = "vertex"; break;
        case VK_SHADER_STAGE_FRAGMENT_BIT:  stage = "fragment"; break;
        case VK_SHADER_STAGE_COMPUTE_BIT:  stage = "compute"; break;
        }
        
        // add the #defines
        //
        std::string defines;
        for (auto it = pDefines->begin(); it != pDefines->end(); it++)
            defines += "-D" + it->first + "=" + it->second + " ";

        std::string commandLine = format("glslc --target-env=vulkan1.1 -fshader-stage=%s -fentry-point=%s %s -o %s -I %s %s", stage, pEntryPoint, filenameGlsl.c_str(), filenameSpv.c_str(), SHADER_LIB_DIR, defines.c_str());
        std::string filenameErr = format(SHADER_CACHE_DIR"\\%p.err", hash);

        if (LaunchProcess(commandLine, filenameErr) == true)
        {
            ReadFile(filenameSpv.c_str(), outSpvData, outSpvSize, true);
            assert(*outSpvSize != 0);
            return true;
        }

        return false;
    }

    //
    // Generate sources from the input data
    //
    std::string GenerateSource(ShaderSourceType sourceType, const VkShaderStageFlagBits shader_type, const char *pshader, const char *pEntryPoint, const DefineList *pDefines)
    {
        std::string shaderCode(pshader);
        std::string code;

        if (sourceType == SST_GLSL)
        {
            // the first line in a GLSL shader must be the #version, insert the #defines right after this line
            size_t index = shaderCode.find_first_of('\n');
            code = shaderCode.substr(index, shaderCode.size() - index);

            shaderCode = shaderCode.substr(0, index) + "\n";
        }
        else if (sourceType == SST_HLSL)
        {
            code = shaderCode;
            shaderCode = "";
        }

        // add the #defines to the code to help debugging
        for (auto it = pDefines->begin(); it != pDefines->end(); it++)
            shaderCode += "#define " + it->first + " " + it->second + "\n";

        // concat the actual shader code
        shaderCode += code;

        return shaderCode;
    }

    Cache<VkShaderModule> s_shaderCache;

    void DestroyShadersInTheCache(VkDevice device)
    {
        auto *database = s_shaderCache.GetDatabase();
        for (auto it = database->begin(); it != database->end(); it++)
        {
            vkDestroyShaderModule(device, it->second.m_data, NULL);
        }
    }

    VkResult CreateModule(VkDevice device, char *SpvData, size_t SpvSize, VkShaderModule* pShaderModule)
    {
        VkShaderModuleCreateInfo moduleCreateInfo = {};
        moduleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        moduleCreateInfo.pCode = (uint32_t*)SpvData;
        moduleCreateInfo.codeSize = SpvSize;
        return vkCreateShaderModule(device, &moduleCreateInfo, NULL, pShaderModule);
    }

    //
    // Compile a GLSL or a HLSL, will cache binaries to disk
    //
    VkResult VKCompile(VkDevice device, ShaderSourceType sourceType, const VkShaderStageFlagBits shader_type, const char *pshader, const char *pEntryPoint, const DefineList *pDefines, VkPipelineShaderStageCreateInfo *pShader)
    {
        VkResult res = VK_SUCCESS;

        //compute hash
        //
        size_t hash;
        hash = HashShaderString(SHADER_LIB_DIR"\\", pshader);
        hash = Hash(pEntryPoint, strlen(pEntryPoint), hash);
        hash = Hash((char*)&shader_type, sizeof(shader_type), hash);
        if (pDefines != NULL)
        {
            hash = pDefines->Hash(hash);
        }

#define USE_MULTITHREADED_CACHE 
#define USE_SPIRV_FROM_DISK   

#ifdef USE_MULTITHREADED_CACHE
        // Compile if not in cache
        //
        if (s_shaderCache.CacheMiss(hash, &pShader->module))
#endif
        {
            char *SpvData = NULL;
            size_t SpvSize = 0;

#ifdef USE_SPIRV_FROM_DISK
            std::string filenameSpv = format(SHADER_CACHE_DIR"\\%p.spv", hash);
            if (ReadFile(filenameSpv.c_str(), &SpvData, &SpvSize, true) == false)
#endif
            {
                std::string &shader = GenerateSource(sourceType, shader_type, pshader, pEntryPoint, pDefines);
                VKCompileToSpirv(hash, sourceType, shader_type, shader.c_str(), pEntryPoint, pDefines, &SpvData, &SpvSize);
            }

            assert(SpvSize != 0);
            CreateModule(device, SpvData, SpvSize, &pShader->module);

#ifdef USE_MULTITHREADED_CACHE
            s_shaderCache.UpdateCache(hash, &pShader->module);
#endif
        }

        pShader->sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        pShader->pNext = NULL;
        pShader->pSpecializationInfo = NULL;
        pShader->flags = 0;
        pShader->stage = shader_type;
        pShader->pName = pEntryPoint;

        return res;
    }

    //
    // VKCompileFromString
    //
    VkResult VKCompileFromString(VkDevice device, ShaderSourceType sourceType, const VkShaderStageFlagBits shader_type, const char *pShaderCode, const char *pEntryPoint, const DefineList *pDefines, VkPipelineShaderStageCreateInfo *pShader)
    {
        assert(strlen(pShaderCode) > 0);

        VkResult res = VKCompile(device, sourceType, shader_type, pShaderCode, pEntryPoint, pDefines, pShader);
        assert(res == VK_SUCCESS);

        return res;
    }

    //
    // VKCompileFromFile
    //
    VkResult VKCompileFromFile(VkDevice device, const VkShaderStageFlagBits shader_type, const char *pFilename, const char *pEntryPoint, const DefineList *pDefines, VkPipelineShaderStageCreateInfo *pShader)
    {
        char *pShaderCode;
        size_t size;

        ShaderSourceType sourceType;

        const char *pExtension = pFilename + std::max<size_t>(strlen(pFilename) - 4, 0);
        if (strcmp(pExtension, "glsl") == 0)
            sourceType = SST_GLSL;
        else if (strcmp(pExtension, "hlsl") == 0)
            sourceType = SST_HLSL;
        else
            assert(!"Can't tell shader type from its extension");

        //append path
        char fullpath[1024];
        sprintf_s(fullpath, SHADER_LIB_DIR"\\%s", pFilename);

        if (ReadFile(fullpath, &pShaderCode, &size, false))
        {
            return VKCompileFromString(device, sourceType, shader_type, pShaderCode, pEntryPoint, pDefines, pShader);
        }

        return VK_NOT_READY;
    }

    //
    // Creates the shader cache
    //
    void CreateShaderCache()
    {
        CreateDirectoryA(SHADER_LIB_DIR, 0);
        CreateDirectoryA(SHADER_CACHE_DIR, 0);
    }

    //
    // Destroys the shader cache object (not the cached data in disk)
    //
    void DestroyShaderCache(Device *pDevice)
    {
        DestroyShadersInTheCache(pDevice->GetDevice());
    }
}