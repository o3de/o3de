// Modifications copyright Amazon.com, Inc. or its affiliates
// Modifications copyright Crytek GmbH

#include "internal_includes/tokens.h"
#include "internal_includes/structs.h"
#include "internal_includes/decode.h"
#include "stdlib.h"
#include "stdio.h"
#include "bstrlib.h"
#include "internal_includes/toMETALInstruction.h"
#include "internal_includes/toMETALOperand.h"
#include "internal_includes/toMETALDeclaration.h"
#include "internal_includes/debug.h"
#include "internal_includes/hlslcc_malloc.h"
#include "internal_includes/structsMetal.h"

extern void AddIndentation(HLSLCrossCompilerContext* psContext);
extern void UpdateFullName(ShaderVarType* psParentVarType);
extern void MangleIdentifiersPerStage(ShaderData* psShader);


void TranslateToMETAL(HLSLCrossCompilerContext* psContext, ShaderLang* planguage)
{
    bstring metal;
    uint32_t i;
    ShaderData* psShader = psContext->psShader;
    ShaderLang language = *planguage;
    uint32_t ui32InstCount = 0;
    uint32_t ui32DeclCount = 0;

    psContext->indent = 0;

    /*psShader->sPhase[MAIN_PHASE].ui32InstanceCount = 1;
    psShader->sPhase[MAIN_PHASE].ppsDecl = hlslcc_malloc(sizeof(Declaration*));
    psShader->sPhase[MAIN_PHASE].ppsInst = hlslcc_malloc(sizeof(Instruction*));
    psShader->sPhase[MAIN_PHASE].pui32DeclCount = hlslcc_malloc(sizeof(uint32_t));
    psShader->sPhase[MAIN_PHASE].pui32InstCount = hlslcc_malloc(sizeof(uint32_t));*/

    if(language == LANG_DEFAULT)
    {
        language = LANG_METAL;
        *planguage = language;
    }

    metal = bfromcstralloc (1024, "");

    psContext->mainShader = metal;
    psContext->stagedInputDeclarations = bfromcstralloc(1024, "");
    psContext->parameterDeclarations = bfromcstralloc(1024, "");
    psContext->declaredOutputs = bfromcstralloc(1024, "");
    psContext->earlyMain = bfromcstralloc (1024, "");
    for(i=0; i<NUM_PHASES;++i)
    {
        psContext->postShaderCode[i] = bfromcstralloc (1024, "");
    }

    psContext->needsFragmentTestHint = 0;

    for (i = 0; i < MAX_COLOR_MRT; i++)
        psContext->gmemOutputNumElements[i] = 0;

    psContext->currentShaderString = &metal;
    psShader->eTargetLanguage = language;
    psContext->currentPhase = MAIN_PHASE;
    
    bcatcstr(metal, "#include <metal_stdlib>\n");
    bcatcstr(metal, "using namespace metal;\n");


    bcatcstr(metal, "struct float1 {\n");
    bcatcstr(metal, "\tfloat x;\n");
    bcatcstr(metal, "};\n");

    bcatcstr(metal, "struct uint1 {\n");
    bcatcstr(metal, "\tuint x;\n");
    bcatcstr(metal, "};\n");

    bcatcstr(metal, "struct int1 {\n");
    bcatcstr(metal, "\tint x;\n");
    bcatcstr(metal, "};\n");


    ui32InstCount = psShader->asPhase[MAIN_PHASE].pui32InstCount[0];
    ui32DeclCount = psShader->asPhase[MAIN_PHASE].pui32DeclCount[0];
    
    AtomicVarList atomicList;
    atomicList.Filled = 0;
    atomicList.Size = ui32InstCount;
    atomicList.AtomicVars = (const ShaderVarType**)hlslcc_malloc(ui32InstCount * sizeof(ShaderVarType*));

    for (i = 0; i < ui32InstCount; ++i)
    {
        DetectAtomicInstructionMETAL(psContext, psShader->asPhase[MAIN_PHASE].ppsInst[0] + i, i + 1 < ui32InstCount ? psShader->asPhase[MAIN_PHASE].ppsInst[0] + i + 1 : 0, &atomicList);
    }

    for(i=0; i < ui32DeclCount; ++i)
    {
        TranslateDeclarationMETAL(psContext, psShader->asPhase[MAIN_PHASE].ppsDecl[0] + i, &atomicList);
    }

    if(psContext->psShader->ui32NumDx9ImmConst)
    {
        bformata(psContext->mainShader, "float4 ImmConstArray [%d];\n", psContext->psShader->ui32NumDx9ImmConst);
    }
    
    MarkIntegerImmediatesMETAL(psContext);

    SetDataTypesMETAL(psContext, psShader->asPhase[MAIN_PHASE].ppsInst[0], ui32InstCount);

    switch (psShader->eShaderType)
    {
    case VERTEX_SHADER:
    {
        int hasStageInput = 0;
        int hasOutput = 0;
        int inputDeclLength = blength(psContext->parameterDeclarations);
        if (blength(psContext->stagedInputDeclarations) > 0)
        {
            hasStageInput = 1;
            bcatcstr(metal, "struct metalVert_stageIn\n{\n");
            bconcat(metal, psContext->stagedInputDeclarations);
            bcatcstr(metal, "};\n");
        }
        if (blength(psContext->declaredOutputs) > 0)
        {
            hasOutput = 1;
            bcatcstr(metal, "struct metalVert_out\n{\n");
            bconcat(metal, psContext->declaredOutputs);
            bcatcstr(metal, "};\n");
        }

        bformata(metal, "vertex %s metalMain(\n%s", 
            hasOutput ? "metalVert_out" : "void",
            hasStageInput ? "\tmetalVert_stageIn stageIn [[ stage_in ]]" : "");

        int userInputDeclLength = blength(psContext->parameterDeclarations);
        if (userInputDeclLength > 2)
        {
            if (hasStageInput)
                bformata(metal, ",\n");
            bdelete(psContext->parameterDeclarations, userInputDeclLength - 2, 2); // remove ",\n"
        }

        bconcat(metal, psContext->parameterDeclarations);
        bcatcstr(metal, hasOutput ? "\t)\n{\n\tmetalVert_out output;\n" : ")\n{\n");
        break;
    }
    case PIXEL_SHADER:
    {
        int hasStageInput = 0;
        int hasOutput = 0;
        int userInputDeclLength = blength(psContext->parameterDeclarations);
        if (blength(psContext->stagedInputDeclarations) > 0)
        {
            hasStageInput = 1;
            bcatcstr(metal, "struct metalFrag_stageIn\n{\n");
            bconcat(metal, psContext->stagedInputDeclarations);
            bcatcstr(metal, "};\n");
        }
        if (blength(psContext->declaredOutputs) > 0)
        {
            hasOutput = 1;
            bcatcstr(metal, "struct metalFrag_out\n{\n");
            bconcat(metal, psContext->declaredOutputs);
            bcatcstr(metal, "};\n");
        }

        bcatcstr(metal, "fragment ");
        if (psContext->needsFragmentTestHint)
        {        
             bcatcstr(metal, "\n#ifndef MTLLanguage1_1\n");
             bcatcstr(metal, "[[ early_fragment_tests ]]\n");
             bcatcstr(metal, "#endif\n");            
         }

        bformata(metal, "%s metalMain(\n%s", hasOutput ? "metalFrag_out" : "void",
            hasStageInput ? "\tmetalFrag_stageIn stageIn [[ stage_in ]]" : "");
        if (userInputDeclLength > 2)
        {
            if (hasStageInput)
                bcatcstr(metal, ",\n");
            bdelete(psContext->parameterDeclarations, userInputDeclLength - 2, 2); // remove the trailing comma and space
        }
        bconcat(metal, psContext->parameterDeclarations);
        bcatcstr(metal, hasOutput ? ")\n{\n\tmetalFrag_out output;\n" : ")\n{\n");
        break;
    }
    case COMPUTE_SHADER:
    {
        int hasStageInput = 0;
        int hasOutput = 0;
        int inputDeclLength = blength(psContext->parameterDeclarations);
        if (blength(psContext->stagedInputDeclarations) > 0)
        {
            hasStageInput = 1;
            bcatcstr(metal, "struct metalCompute_stageIn\n{\n");
            bconcat(metal, psContext->stagedInputDeclarations);
            bcatcstr(metal, "};\n");
        }
        if (blength(psContext->declaredOutputs) > 0)
        {
            hasOutput = 1;
            bcatcstr(metal, "struct metalCompute_out\n{\n");
            bconcat(metal, psContext->declaredOutputs);
            bcatcstr(metal, "};\n");
        }

        bformata(metal, "kernel %s metalMain(\n%s",
            hasOutput ? "metalCompute_out" : "void",
            hasStageInput ? "\tmetalCompute_stageIn stageIn [[ stage_in ]]" : "");

        int userInputDeclLength = blength(psContext->parameterDeclarations);
        if (userInputDeclLength > 2)
        {
            if (hasStageInput)
                bformata(metal, ",\n");
            bdelete(psContext->parameterDeclarations, userInputDeclLength - 2, 2); // remove ",\n"
        }

        bconcat(metal, psContext->parameterDeclarations);
        bcatcstr(metal, hasOutput ? "\t)\n{\n\tmetalCompute_out output;\n" : ")\n{\n");
        break;
    }
    default:
    {
        ASSERT(0); 
        // Geometry, Hull, and Domain shaders unsupported by Metal
        //         int userInputDeclLength = blength(psContext->parameterDeclarations);
        //         if (blength(psContext->outputDeclarations) > 0)
        //         {
        //             bcatcstr(metal, "struct metalComp_out\n{\n");
        //             bconcat(metal, psContext->outputDeclarations);
        // bcatcstr(metal, "};\n");
        //             if (userInputDeclLength > 2)
        //                 bdelete(psContext->parameterDeclarations, userInputDeclLength - 2, 2); // remove the trailing comma and space
        //             bformata(metal, "kernel metalComp_out metalMain(%s)\n{\n\tmetalComp_out output;\n", psContext->parameterDeclarations);
        //         }
        //         else
        //         {
        //             if (userInputDeclLength > 2)
        //                 bdelete(psContext->parameterDeclarations, userInputDeclLength - 2, 2); // remove the trailing comma and space
        //             bformata(metal, "kernel void metalMain(%s)\n{\n", psContext->parameterDeclarations);
        //         }
        break;
    }
    }

    psContext->indent++;

#ifdef _DEBUG
    AddIndentation(psContext);
    bcatcstr(metal, "//--- Start Early Main ---\n");
#endif
    bconcat(metal, psContext->earlyMain);
#ifdef _DEBUG
    AddIndentation(psContext);
    bcatcstr(metal, "//--- End Early Main ---\n");
#endif


    for(i=0; i < ui32InstCount; ++i)
    {
        TranslateInstructionMETAL(psContext, psShader->asPhase[MAIN_PHASE].ppsInst[0]+i, i+1 < ui32InstCount ? psShader->asPhase[MAIN_PHASE].ppsInst[0]+i+1 : 0);
    }

    hlslcc_free((void*)atomicList.AtomicVars);

    psContext->indent--;

    bcatcstr(metal, "}\n");
}

static void FreeSubOperands(Instruction* psInst, const uint32_t ui32NumInsts)
{
    uint32_t ui32Inst;
    for(ui32Inst = 0; ui32Inst < ui32NumInsts; ++ui32Inst)
    {
        Instruction* psCurrentInst = &psInst[ui32Inst];
        const uint32_t ui32NumOperands = psCurrentInst->ui32NumOperands;
        uint32_t ui32Operand;

        for(ui32Operand = 0; ui32Operand < ui32NumOperands; ++ui32Operand)
        {
            uint32_t ui32SubOperand;
            for(ui32SubOperand = 0; ui32SubOperand < MAX_SUB_OPERANDS; ++ui32SubOperand)
            {
                if(psCurrentInst->asOperands[ui32Operand].psSubOperand[ui32SubOperand])
                {
                    hlslcc_free(psCurrentInst->asOperands[ui32Operand].psSubOperand[ui32SubOperand]);
                    psCurrentInst->asOperands[ui32Operand].psSubOperand[ui32SubOperand] = NULL;
                }
            }
        }
    }
}

typedef enum {
    MTLFunctionTypeVertex = 1,
    MTLFunctionTypeFragment = 2,
    MTLFunctionTypeKernel = 3
} MTLFunctionType;

HLSLCC_API int HLSLCC_APIENTRY TranslateHLSLFromMemToMETAL(const char* shader,
    unsigned int flags,
    ShaderLang language,
    Shader* result)
{
    uint32_t* tokens;
    ShaderData* psShader;
    char* glslcstr = NULL;
    int ShaderType = MTLFunctionTypeFragment;
    int success = 0;
    uint32_t i;

    tokens = (uint32_t*)shader;

    psShader = DecodeDXBC(tokens);

    if(psShader)
    {
        HLSLCrossCompilerContext sContext;

        sContext.psShader = psShader;
        sContext.flags = flags;

        for(i=0; i<NUM_PHASES;++i)
        {
            sContext.havePostShaderCode[i] = 0;
        }

        TranslateToMETAL(&sContext, &language);

        switch(psShader->eShaderType)
        {
            case VERTEX_SHADER:
            {
                ShaderType = MTLFunctionTypeVertex;
                break;
            }
            case COMPUTE_SHADER:
            {
                ShaderType = MTLFunctionTypeKernel;
                break;
            }
            default:
            {
                break;
            }
        }

        glslcstr = bstr2cstr(sContext.mainShader, '\0');

        bdestroy(sContext.mainShader);
        bdestroy(sContext.earlyMain);
        for(i=0; i<NUM_PHASES; ++i)
        {
            bdestroy(sContext.postShaderCode[i]);
        }

        for(i=0; i<NUM_PHASES;++i)
        {
            if(psShader->asPhase[i].ppsDecl != 0)
            {
                uint32_t k;
                for(k=0; k < psShader->asPhase[i].ui32InstanceCount; ++k)
                {
                    hlslcc_free(psShader->asPhase[i].ppsDecl[k]);
                }
                hlslcc_free(psShader->asPhase[i].ppsDecl);
            }
            if(psShader->asPhase[i].ppsInst != 0)
            {
                uint32_t k;
                for(k=0; k < psShader->asPhase[i].ui32InstanceCount; ++k)
                {
                    FreeSubOperands(psShader->asPhase[i].ppsInst[k], psShader->asPhase[i].pui32InstCount[k]);
                    hlslcc_free(psShader->asPhase[i].ppsInst[k]);
                }
                hlslcc_free(psShader->asPhase[i].ppsInst);
            }
        }
        
        memcpy(&result->reflection,&psShader->sInfo,sizeof(psShader->sInfo));
        
        result->textureSamplerInfo.ui32NumTextureSamplerPairs = psShader->textureSamplerInfo.ui32NumTextureSamplerPairs;
        for (i=0; i<result->textureSamplerInfo.ui32NumTextureSamplerPairs; i++)
            strcpy(result->textureSamplerInfo.aTextureSamplerPair[i].Name, psShader->textureSamplerInfo.aTextureSamplerPair[i].Name);

        hlslcc_free(psShader);

        success = 1;
    }

    shader = 0;
    tokens = 0;

    /* Fill in the result struct */

    result->shaderType = ShaderType;
    result->sourceCode = glslcstr;
    result->GLSLLanguage = language;

    return success;
}

HLSLCC_API int HLSLCC_APIENTRY TranslateHLSLFromFileToMETAL(const char* filename,
    unsigned int flags,
    ShaderLang language,
    Shader* result)
{
    FILE* shaderFile;
    int length;
    size_t readLength;
    char* shader;
    int success = 0;

    shaderFile = fopen(filename, "rb");

    if(!shaderFile)
    {
        return 0;
    }

    fseek(shaderFile, 0, SEEK_END);
    length = ftell(shaderFile);
    fseek(shaderFile, 0, SEEK_SET);

    shader = (char*)hlslcc_malloc(length+1);

    readLength = fread(shader, 1, length, shaderFile);

    fclose(shaderFile);
    shaderFile = 0;

    shader[readLength] = '\0';

    success = TranslateHLSLFromMemToMETAL(shader, flags, language, result);

    hlslcc_free(shader);

    return success;
}