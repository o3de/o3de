// Modifications copyright Amazon.com, Inc. or its affiliates
// Modifications copyright Crytek GmbH

#ifndef REFLECT_H
#define REFLECT_H

#include "hlslcc.h"

ResourceGroup ResourceTypeToResourceGroup(ResourceType);

int GetResourceFromBindingPoint(const ResourceGroup eGroup, const uint32_t ui32BindPoint, const ShaderInfo* psShaderInfo, ResourceBinding** ppsOutBinding);

void GetConstantBufferFromBindingPoint(const ResourceGroup eGroup, const uint32_t ui32BindPoint, const ShaderInfo* psShaderInfo, ConstantBuffer** ppsConstBuf);

int GetInterfaceVarFromOffset(uint32_t ui32Offset, ShaderInfo* psShaderInfo, ShaderVar** ppsShaderVar);

int GetInputSignatureFromRegister(const uint32_t ui32Register, const ShaderInfo* psShaderInfo, InOutSignature** ppsOut);
int GetOutputSignatureFromRegister(const uint32_t ui32Register, const uint32_t ui32Stream, const uint32_t ui32CompMask, ShaderInfo* psShaderInfo, InOutSignature** ppsOut);

int GetOutputSignatureFromSystemValue(SPECIAL_NAME eSystemValueType, uint32_t ui32SemanticIndex, ShaderInfo* psShaderInfo, InOutSignature** ppsOut);

int GetShaderVarFromOffset(const uint32_t ui32Vec4Offset, const uint32_t* pui32Swizzle, ConstantBuffer* psCBuf, ShaderVarType** ppsShaderVar, int32_t* pi32Index, int32_t* pi32Rebase);

typedef struct
{
    uint32_t* pui32Inputs;
    uint32_t* pui32Outputs;
    uint32_t* pui32Resources;
    uint32_t* pui32Interfaces;
    uint32_t* pui32Inputs11;
    uint32_t* pui32Outputs11;
    uint32_t* pui32OutputsWithStreams;
} ReflectionChunks;

void LoadShaderInfo(const uint32_t ui32MajorVersion, const uint32_t ui32MinorVersion, const ReflectionChunks* psChunks, ShaderInfo* psInfo);

void LoadD3D9ConstantTable(const char* data, ShaderInfo* psInfo);

void FreeShaderInfo(ShaderInfo* psShaderInfo);

#endif

