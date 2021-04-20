// Modifications copyright Amazon.com, Inc. or its affiliates
// Modifications copyright Crytek GmbH

#include "serializeReflection.h"
#include "cJSON.h"
#include <string>
#include <sstream>

void* jsonMalloc(size_t sz)
{
    return new char[sz];
}
void jsonFree(void* ptr)
{
    char* charPtr = static_cast<char*>(ptr);
    delete [] charPtr;
}

static void AppendIntToString(std::string& str, uint32_t num)
{
    std::stringstream ss;
    ss << num;
    str += ss.str();
}

static void WriteInOutSignature(InOutSignature* psSignature, cJSON* obj)
{
    cJSON_AddItemToObject(obj, "SemanticName", cJSON_CreateString(psSignature->SemanticName));
    cJSON_AddItemToObject(obj, "ui32SemanticIndex", cJSON_CreateNumber(psSignature->ui32SemanticIndex));
    cJSON_AddItemToObject(obj, "eSystemValueType", cJSON_CreateNumber(psSignature->eSystemValueType));
    cJSON_AddItemToObject(obj, "eComponentType", cJSON_CreateNumber(psSignature->eComponentType));
    cJSON_AddItemToObject(obj, "ui32Register", cJSON_CreateNumber(psSignature->ui32Register));
    cJSON_AddItemToObject(obj, "ui32Mask", cJSON_CreateNumber(psSignature->ui32Mask));
    cJSON_AddItemToObject(obj, "ui32ReadWriteMask", cJSON_CreateNumber(psSignature->ui32ReadWriteMask));
}

static void WriteResourceBinding(ResourceBinding* psBinding, cJSON* obj)
{
    cJSON_AddItemToObject(obj, "Name", cJSON_CreateString(psBinding->Name));
    cJSON_AddItemToObject(obj, "eType", cJSON_CreateNumber(psBinding->eType));
    cJSON_AddItemToObject(obj, "ui32BindPoint", cJSON_CreateNumber(psBinding->ui32BindPoint));
    cJSON_AddItemToObject(obj, "ui32BindCount", cJSON_CreateNumber(psBinding->ui32BindCount));
    cJSON_AddItemToObject(obj, "ui32Flags", cJSON_CreateNumber(psBinding->ui32Flags));
    cJSON_AddItemToObject(obj, "eDimension", cJSON_CreateNumber(psBinding->eDimension));
    cJSON_AddItemToObject(obj, "ui32ReturnType", cJSON_CreateNumber(psBinding->ui32ReturnType));
    cJSON_AddItemToObject(obj, "ui32NumSamples", cJSON_CreateNumber(psBinding->ui32NumSamples));
}

static void WriteShaderVar(ShaderVar* psVar, cJSON* obj)
{
    cJSON_AddItemToObject(obj, "Name", cJSON_CreateString(psVar->Name));
    if(psVar->haveDefaultValue)
    {
        cJSON_AddItemToObject(obj, "aui32DefaultValues", cJSON_CreateIntArray((int*)psVar->pui32DefaultValues, psVar->ui32Size/4));
    }
    cJSON_AddItemToObject(obj, "ui32StartOffset", cJSON_CreateNumber(psVar->ui32StartOffset));
    cJSON_AddItemToObject(obj, "ui32Size", cJSON_CreateNumber(psVar->ui32Size));
}

static void WriteConstantBuffer(ConstantBuffer* psCBuf, cJSON* obj)
{
    cJSON_AddItemToObject(obj, "Name", cJSON_CreateString(psCBuf->Name));
    cJSON_AddItemToObject(obj, "ui32NumVars", cJSON_CreateNumber(psCBuf->ui32NumVars));

    for(uint32_t i = 0; i < psCBuf->ui32NumVars; ++i)
    {
        std::string name;
        name += "var";
        AppendIntToString(name, i);

        cJSON* varObj = cJSON_CreateObject();
        cJSON_AddItemToObject(obj, name.c_str(), varObj);

        WriteShaderVar(&psCBuf->asVars[i], varObj);
    }

    cJSON_AddItemToObject(obj, "ui32TotalSizeInBytes", cJSON_CreateNumber(psCBuf->ui32TotalSizeInBytes));
}

static void WriteClassType(ClassType* psClassType, cJSON* obj)
{
    cJSON_AddItemToObject(obj, "Name", cJSON_CreateString(psClassType->Name));
    cJSON_AddItemToObject(obj, "ui16ID", cJSON_CreateNumber(psClassType->ui16ID));
    cJSON_AddItemToObject(obj, "ui16ConstBufStride", cJSON_CreateNumber(psClassType->ui16ConstBufStride));
    cJSON_AddItemToObject(obj, "ui16Texture", cJSON_CreateNumber(psClassType->ui16Texture));
    cJSON_AddItemToObject(obj, "ui16Sampler", cJSON_CreateNumber(psClassType->ui16Sampler));
}

static void WriteClassInstance(ClassInstance* psClassInst, cJSON* obj)
{
    cJSON_AddItemToObject(obj, "Name", cJSON_CreateString(psClassInst->Name));
    cJSON_AddItemToObject(obj, "ui16ID", cJSON_CreateNumber(psClassInst->ui16ID));
    cJSON_AddItemToObject(obj, "ui16ConstBuf", cJSON_CreateNumber(psClassInst->ui16ConstBuf));
    cJSON_AddItemToObject(obj, "ui16ConstBufOffset", cJSON_CreateNumber(psClassInst->ui16ConstBufOffset));
    cJSON_AddItemToObject(obj, "ui16Texture", cJSON_CreateNumber(psClassInst->ui16Texture));
    cJSON_AddItemToObject(obj, "ui16Sampler", cJSON_CreateNumber(psClassInst->ui16Sampler));
}

const char* SerializeReflection(ShaderInfo* psReflection)
{
    cJSON* root;

    cJSON_Hooks hooks;
    hooks.malloc_fn = jsonMalloc;
    hooks.free_fn = jsonFree;
    cJSON_InitHooks(&hooks);

    root=cJSON_CreateObject();
    cJSON_AddItemToObject(root, "ui32MajorVersion", cJSON_CreateNumber(psReflection->ui32MajorVersion));
    cJSON_AddItemToObject(root, "ui32MinorVersion", cJSON_CreateNumber(psReflection->ui32MinorVersion));

    cJSON_AddItemToObject(root, "ui32NumInputSignatures", cJSON_CreateNumber(psReflection->ui32NumInputSignatures));

    for(uint32_t i = 0; i < psReflection->ui32NumInputSignatures; ++i)
    {
        std::string name;
        name += "input";
        AppendIntToString(name, i);

        cJSON* obj = cJSON_CreateObject();
        cJSON_AddItemToObject(root, name.c_str(), obj);

        WriteInOutSignature(psReflection->psInputSignatures+i, obj);
    }

    cJSON_AddItemToObject(root, "ui32NumOutputSignatures", cJSON_CreateNumber(psReflection->ui32NumOutputSignatures));

    for(uint32_t i = 0; i < psReflection->ui32NumOutputSignatures; ++i)
    {
        std::string name;
        name += "output";
        AppendIntToString(name, i);

        cJSON* obj = cJSON_CreateObject();
        cJSON_AddItemToObject(root, name.c_str(), obj);

        WriteInOutSignature(psReflection->psOutputSignatures+i, obj);
    }

    cJSON_AddItemToObject(root, "ui32NumResourceBindings", cJSON_CreateNumber(psReflection->ui32NumResourceBindings));

    for(uint32_t i = 0; i < psReflection->ui32NumResourceBindings; ++i)
    {
        std::string name;
        name += "resource";
        AppendIntToString(name, i);

        cJSON* obj = cJSON_CreateObject();
        cJSON_AddItemToObject(root, name.c_str(), obj);

        WriteResourceBinding(psReflection->psResourceBindings+i, obj);
    }

    cJSON_AddItemToObject(root, "ui32NumConstantBuffers", cJSON_CreateNumber(psReflection->ui32NumConstantBuffers));

    for(uint32_t i = 0; i < psReflection->ui32NumConstantBuffers; ++i)
    {
        std::string name;
        name += "cbuf";
        AppendIntToString(name, i);

        cJSON* obj = cJSON_CreateObject();
        cJSON_AddItemToObject(root, name.c_str(), obj);

        WriteConstantBuffer(psReflection->psConstantBuffers+i, obj);
    }

    //psThisPointerConstBuffer is a cache. Don't need to write this out.
    //It just points to the $ThisPointer cbuffer within the psConstantBuffers array.

    for(uint32_t i = 0; i < psReflection->ui32NumClassTypes; ++i)
    {
        std::string name;
        name += "classType";
        AppendIntToString(name, i);

        cJSON* obj = cJSON_CreateObject();
        cJSON_AddItemToObject(root, name.c_str(), obj);

        WriteClassType(psReflection->psClassTypes+i, obj);
    }

    for(uint32_t i = 0; i < psReflection->ui32NumClassInstances; ++i)
    {
        std::string name;
        name += "classInst";
        AppendIntToString(name, i);

        cJSON* obj = cJSON_CreateObject();
        cJSON_AddItemToObject(root, name.c_str(), obj);

        WriteClassInstance(psReflection->psClassInstances+i, obj);
    }

    //psReflection->aui32TableIDToTypeID
    //psReflection->aui32ConstBufferBindpointRemap

    cJSON_AddItemToObject(root, "eTessPartitioning", cJSON_CreateNumber(psReflection->eTessPartitioning));
    cJSON_AddItemToObject(root, "eTessOutPrim", cJSON_CreateNumber(psReflection->eTessOutPrim));


    const char* jsonString = cJSON_Print(root);

    cJSON_Delete(root);

    return jsonString;
}
