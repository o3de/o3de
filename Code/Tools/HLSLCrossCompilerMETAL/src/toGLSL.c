// Modifications copyright Amazon.com, Inc. or its affiliates
// Modifications copyright Crytek GmbH

#include "internal_includes/tokens.h"
#include "internal_includes/structs.h"
#include "internal_includes/decode.h"
#include "stdlib.h"
#include "stdio.h"
#include "bstrlib.h"
#include "internal_includes/toGLSLInstruction.h"
#include "internal_includes/toGLSLOperand.h"
#include "internal_includes/toGLSLDeclaration.h"
#include "internal_includes/languages.h"
#include "internal_includes/debug.h"
#include "internal_includes/hlslcc_malloc.h"

#ifndef GL_VERTEX_SHADER_ARB
#define GL_VERTEX_SHADER_ARB              0x8B31
#endif
#ifndef GL_FRAGMENT_SHADER_ARB
#define GL_FRAGMENT_SHADER_ARB            0x8B30
#endif
#ifndef GL_GEOMETRY_SHADER
#define GL_GEOMETRY_SHADER 0x8DD9
#endif
#ifndef GL_TESS_EVALUATION_SHADER
#define GL_TESS_EVALUATION_SHADER 0x8E87
#endif
#ifndef GL_TESS_CONTROL_SHADER
#define GL_TESS_CONTROL_SHADER 0x8E88
#endif
#ifndef GL_COMPUTE_SHADER
#define GL_COMPUTE_SHADER 0x91B9
#endif


HLSLCC_API void HLSLCC_APIENTRY HLSLcc_SetMemoryFunctions(void* (*malloc_override)(size_t),void* (*calloc_override)(size_t,size_t),void (*free_override)(void *),void* (*realloc_override)(void*,size_t))
{
    hlslcc_malloc = malloc_override;
    hlslcc_calloc = calloc_override;
    hlslcc_free = free_override;    
    hlslcc_realloc = realloc_override;
}

void AddIndentation(HLSLCrossCompilerContext* psContext)
{
    int i;
    int indent = psContext->indent;
    bstring glsl = *psContext->currentShaderString;
    for(i=0; i < indent; ++i)
    {
        bcatcstr(glsl, "    ");
    }
}

void AddVersionDependentCode(HLSLCrossCompilerContext* psContext)
{
    bstring glsl = *psContext->currentShaderString;

    if(psContext->psShader->ui32MajorVersion > 3 && psContext->psShader->eTargetLanguage != LANG_ES_300 && psContext->psShader->eTargetLanguage != LANG_ES_310 && !(psContext->psShader->eTargetLanguage >= LANG_330))
    {
        //DX10+ bycode format requires the ability to treat registers
        //as raw bits. ES3.0+ has that built-in, also 330 onwards
        bcatcstr(glsl,"#extension GL_ARB_shader_bit_encoding : require\n");
    }

    if(!HaveCompute(psContext->psShader->eTargetLanguage))
    {
        if(psContext->psShader->eShaderType == COMPUTE_SHADER)
        {
            bcatcstr(glsl,"#extension GL_ARB_compute_shader : enable\n");
            bcatcstr(glsl,"#extension GL_ARB_shader_storage_buffer_object : enable\n");
        }
    }

    if (!HaveAtomicMem(psContext->psShader->eTargetLanguage) ||
        !HaveAtomicCounter(psContext->psShader->eTargetLanguage))
    {
        if( psContext->psShader->aiOpcodeUsed[OPCODE_IMM_ATOMIC_ALLOC] ||
            psContext->psShader->aiOpcodeUsed[OPCODE_IMM_ATOMIC_CONSUME] ||
            psContext->psShader->aiOpcodeUsed[OPCODE_DCL_UNORDERED_ACCESS_VIEW_STRUCTURED])
        {
            bcatcstr(glsl,"#extension GL_ARB_shader_atomic_counters : enable\n");

            bcatcstr(glsl,"#extension GL_ARB_shader_storage_buffer_object : enable\n");
        }
    }

    if(!HaveGather(psContext->psShader->eTargetLanguage))
    {
        if(psContext->psShader->aiOpcodeUsed[OPCODE_GATHER4] ||
            psContext->psShader->aiOpcodeUsed[OPCODE_GATHER4_PO_C] ||
            psContext->psShader->aiOpcodeUsed[OPCODE_GATHER4_PO] ||
            psContext->psShader->aiOpcodeUsed[OPCODE_GATHER4_C])
        {
            bcatcstr(glsl,"#extension GL_ARB_texture_gather : enable\n");
        }
    }

    if(!HaveGatherNonConstOffset(psContext->psShader->eTargetLanguage))
    {
        if(psContext->psShader->aiOpcodeUsed[OPCODE_GATHER4_PO_C] ||
            psContext->psShader->aiOpcodeUsed[OPCODE_GATHER4_PO])
        {
            bcatcstr(glsl,"#extension GL_ARB_gpu_shader5 : enable\n");
        }
    }

    if(!HaveQueryLod(psContext->psShader->eTargetLanguage))
    {
        if(psContext->psShader->aiOpcodeUsed[OPCODE_LOD])
        {
            bcatcstr(glsl,"#extension GL_ARB_texture_query_lod : enable\n");
        }
    }

    if(!HaveQueryLevels(psContext->psShader->eTargetLanguage))
    {
        if(psContext->psShader->aiOpcodeUsed[OPCODE_RESINFO])
        {
            bcatcstr(glsl,"#extension GL_ARB_texture_query_levels : enable\n");
        }
    }

    if(!HaveImageLoadStore(psContext->psShader->eTargetLanguage))
    {
        if(psContext->psShader->aiOpcodeUsed[OPCODE_STORE_UAV_TYPED] ||
            psContext->psShader->aiOpcodeUsed[OPCODE_STORE_RAW] ||
            psContext->psShader->aiOpcodeUsed[OPCODE_STORE_STRUCTURED])
        {
            bcatcstr(glsl,"#extension GL_ARB_shader_image_load_store : enable\n");
            bcatcstr(glsl,"#extension GL_ARB_shader_bit_encoding : enable\n");
        }
        else
        if(psContext->psShader->aiOpcodeUsed[OPCODE_LD_UAV_TYPED] ||
            psContext->psShader->aiOpcodeUsed[OPCODE_LD_RAW] ||
            psContext->psShader->aiOpcodeUsed[OPCODE_LD_STRUCTURED])
        {
            bcatcstr(glsl,"#extension GL_ARB_shader_image_load_store : enable\n");
        }
    }
    
    //The fragment language has no default precision qualifier for floating point types.
    if (psContext->psShader->eShaderType == PIXEL_SHADER &&
        psContext->psShader->eTargetLanguage == LANG_ES_100 || psContext->psShader->eTargetLanguage == LANG_ES_300 || psContext->psShader->eTargetLanguage == LANG_ES_310)
    {
        bcatcstr(glsl, "precision highp float;\n");
    }

    /* There is no default precision qualifier for the following sampler types in either the vertex or fragment language: */
    if (psContext->psShader->eTargetLanguage == LANG_ES_300 || psContext->psShader->eTargetLanguage == LANG_ES_310)
    {
        bcatcstr(glsl, "precision lowp sampler3D;\n");
        bcatcstr(glsl, "precision lowp samplerCubeShadow;\n");
        bcatcstr(glsl, "precision lowp sampler2DShadow;\n");
        bcatcstr(glsl, "precision lowp sampler2DArray;\n");
        bcatcstr(glsl, "precision lowp sampler2DArrayShadow;\n");
        bcatcstr(glsl, "precision lowp isampler2D;\n");
        bcatcstr(glsl, "precision lowp isampler3D;\n");
        bcatcstr(glsl, "precision lowp isamplerCube;\n");
        bcatcstr(glsl, "precision lowp isampler2DArray;\n");
        bcatcstr(glsl, "precision lowp usampler2D;\n");
        bcatcstr(glsl, "precision lowp usampler3D;\n");
        bcatcstr(glsl, "precision lowp usamplerCube;\n");
        bcatcstr(glsl, "precision lowp usampler2DArray;\n");

        if (psContext->psShader->eTargetLanguage == LANG_ES_310)
        {
            bcatcstr(glsl, "precision lowp isampler2DMS;\n");
            bcatcstr(glsl, "precision lowp usampler2D;\n");
            bcatcstr(glsl, "precision lowp usampler3D;\n");
            bcatcstr(glsl, "precision lowp usamplerCube;\n");
            bcatcstr(glsl, "precision lowp usampler2DArray;\n");
            bcatcstr(glsl, "precision lowp usampler2DMS;\n");
            bcatcstr(glsl, "precision lowp image2D;\n");
            bcatcstr(glsl, "precision lowp image3D;\n");
            bcatcstr(glsl, "precision lowp imageCube;\n");
            bcatcstr(glsl, "precision lowp image2DArray;\n");
            bcatcstr(glsl, "precision lowp iimage2D;\n");
            bcatcstr(glsl, "precision lowp iimage3D;\n");
            bcatcstr(glsl, "precision lowp iimageCube;\n");
            bcatcstr(glsl, "precision lowp uimage2DArray;\n");
        }
        bcatcstr(glsl, "\n");
    }

    if (SubroutinesSupported(psContext->psShader->eTargetLanguage))
    {
        bcatcstr(glsl, "subroutine void SubroutineType();\n");
    }

    if (psContext->psShader->ui32MajorVersion <= 3)
    {
        bcatcstr(glsl, "int RepCounter;\n");
        bcatcstr(glsl, "int LoopCounter;\n");
        bcatcstr(glsl, "int ZeroBasedCounter;\n");
        if (psContext->psShader->eShaderType == VERTEX_SHADER)
        {
            uint32_t texCoord;
            bcatcstr(glsl, "ivec4 Address;\n");

            if (InOutSupported(psContext->psShader->eTargetLanguage))
            {
                bcatcstr(glsl, "out vec4 OffsetColour;\n");
                bcatcstr(glsl, "out vec4 BaseColour;\n");

                bcatcstr(glsl, "out vec4 Fog;\n");

                for (texCoord = 0; texCoord < 8; ++texCoord)
                {
                    bformata(glsl, "out vec4 TexCoord%d;\n", texCoord);
                }
            }
            else
            {
                bcatcstr(glsl, "varying vec4 OffsetColour;\n");
                bcatcstr(glsl, "varying vec4 BaseColour;\n");

                bcatcstr(glsl, "varying vec4 Fog;\n");

                for (texCoord = 0; texCoord < 8; ++texCoord)
                {
                    bformata(glsl, "varying vec4 TexCoord%d;\n", texCoord);
                }
            }
        }
        else
        {
            uint32_t renderTargets, texCoord;

            if (InOutSupported(psContext->psShader->eTargetLanguage))
            {
                bcatcstr(glsl, "in vec4 OffsetColour;\n");
                bcatcstr(glsl, "in vec4 BaseColour;\n");

                bcatcstr(glsl, "in vec4 Fog;\n");

                for (texCoord = 0; texCoord < 8; ++texCoord)
                {
                    bformata(glsl, "in vec4 TexCoord%d;\n", texCoord);
                }
            }
            else
            {
                bcatcstr(glsl, "varying vec4 OffsetColour;\n");
                bcatcstr(glsl, "varying vec4 BaseColour;\n");

                bcatcstr(glsl, "varying vec4 Fog;\n");

                for (texCoord = 0; texCoord < 8; ++texCoord)
                {
                    bformata(glsl, "varying vec4 TexCoord%d;\n", texCoord);
                }
            }

            if (psContext->psShader->eTargetLanguage > LANG_120)
            {
                bcatcstr(glsl, "out vec4 outFragData[8];\n");
                for (renderTargets = 0; renderTargets < 8; ++renderTargets)
                {
                    bformata(glsl, "#define Output%d outFragData[%d]\n", renderTargets, renderTargets);
                }
            }
            else if (psContext->psShader->eTargetLanguage >= LANG_ES_300 && psContext->psShader->eTargetLanguage < LANG_120)
            {
                // ES 3 supports min 4 rendertargets, I guess this is reasonable lower limit for DX9 shaders
                bcatcstr(glsl, "out vec4 outFragData[4];\n");
                for (renderTargets = 0; renderTargets < 4; ++renderTargets)
                {
                    bformata(glsl, "#define Output%d outFragData[%d]\n", renderTargets, renderTargets);
                }
            }
            else if (psContext->psShader->eTargetLanguage == LANG_ES_100)
            {
                bcatcstr(glsl, "#define Output0 gl_FragColor;\n");
            }
            else
            {
                for (renderTargets = 0; renderTargets < 8; ++renderTargets)
                {
                    bformata(glsl, "#define Output%d gl_FragData[%d]\n", renderTargets, renderTargets);
                }
            }
        }
    }

    if((psContext->flags & HLSLCC_FLAG_ORIGIN_UPPER_LEFT)
        && (psContext->psShader->eTargetLanguage >= LANG_150))
    {
        bcatcstr(glsl,"layout(origin_upper_left) in vec4 gl_FragCoord;\n");
    }

    if((psContext->flags & HLSLCC_FLAG_PIXEL_CENTER_INTEGER)
        && (psContext->psShader->eTargetLanguage >= LANG_150))
    {
        bcatcstr(glsl,"layout(pixel_center_integer) in vec4 gl_FragCoord;\n");
    }

    /* For versions which do not support a vec1 (currently all versions) */
    bcatcstr(glsl,"struct vec1 {\n");
    bcatcstr(glsl,"\tfloat x;\n");
    bcatcstr(glsl,"};\n");

    if(HaveUVec(psContext->psShader->eTargetLanguage))
    {
        bcatcstr(glsl,"struct uvec1 {\n");
        bcatcstr(glsl,"\tuint x;\n");
        bcatcstr(glsl,"};\n");
    }

    bcatcstr(glsl,"struct ivec1 {\n");
    bcatcstr(glsl,"\tint x;\n");
    bcatcstr(glsl,"};\n");

    /*
        OpenGL 4.1 API spec:
        To use any built-in input or output in the gl_PerVertex block in separable
        program objects, shader code must redeclare that block prior to use.
    */
    if(psContext->psShader->eShaderType == VERTEX_SHADER && psContext->psShader->eTargetLanguage >= LANG_410)
    {
        bcatcstr(glsl, "out gl_PerVertex {\n");
        bcatcstr(glsl, "vec4 gl_Position;\n");
        bcatcstr(glsl, "float gl_PointSize;\n");
        bcatcstr(glsl, "float gl_ClipDistance[];");
        bcatcstr(glsl, "};\n");
    }
}

ShaderLang ChooseLanguage(ShaderData* psShader)
{
    // Depends on the HLSL shader model extracted from bytecode.
    switch(psShader->ui32MajorVersion)
    {
        case 5:
        {
            return LANG_430;
        }
        case 4:
        {
            return LANG_330;
        }
        default:
        {
            return LANG_120;
        }
    }
}

const char* GetVersionString(ShaderLang language)
{
    switch(language)
    {
        case LANG_ES_100:
        {
            return "#version 100\n";
            break;
        }
        case LANG_ES_300:
        {
            return "#version 300 es\n";
            break;
        }
        case LANG_ES_310:
        {
            return "#version 310 es\n";
            break;
        }
        case LANG_120:
        {
            return "#version 120\n";
            break;
        }
        case LANG_130:
        {
            return "#version 130\n";
            break;
        }
        case LANG_140:
        {
            return "#version 140\n";
            break;
        }
        case LANG_150:
        {
            return "#version 150\n";
            break;
        }
        case LANG_330:
        {
            return "#version 330\n";
            break;
        }
        case LANG_400:
        {
            return "#version 400\n";
            break;
        }
        case LANG_410:
        {
            return "#version 410\n";
            break;
        }
        case LANG_420:
        {
            return "#version 420\n";
            break;
        }
        case LANG_430:
        {
            return "#version 430\n";
            break;
        }
        case LANG_440:
        {
            return "#version 440\n";
            break;
        }
        default:
        {
            return "";
            break;
        }
    }
}

void TranslateToGLSL(HLSLCrossCompilerContext* psContext, ShaderLang* planguage,const GlExtensions *extensions)
{
    bstring glsl;
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
        language = ChooseLanguage(psShader);
        *planguage = language;
    }

        glsl = bfromcstralloc (1024, GetVersionString(language));

    psContext->mainShader = glsl;
    psContext->earlyMain = bfromcstralloc (1024, "");
    for(i=0; i<NUM_PHASES;++i)
    {
        psContext->postShaderCode[i] = bfromcstralloc (1024, "");
    }
    psContext->currentShaderString = &glsl;
    psShader->eTargetLanguage = language;
    psShader->extensions = (const struct GlExtensions*)extensions;
    psContext->currentPhase = MAIN_PHASE;

    if(extensions)
    {
        if(extensions->ARB_explicit_attrib_location)
            bcatcstr(glsl,"#extension GL_ARB_explicit_attrib_location : require\n");
        if(extensions->ARB_explicit_uniform_location)
            bcatcstr(glsl,"#extension GL_ARB_explicit_uniform_location : require\n");
        if(extensions->ARB_shading_language_420pack)
            bcatcstr(glsl,"#extension GL_ARB_shading_language_420pack : require\n");
    }

    AddVersionDependentCode(psContext);

    if(psContext->flags & HLSLCC_FLAG_UNIFORM_BUFFER_OBJECT)
    {
        bcatcstr(glsl, "layout(std140) uniform;\n");
    }

    //Special case. Can have multiple phases.
    if(psShader->eShaderType == HULL_SHADER)
    {
        int haveInstancedForkPhase = 0;            // Do we have an instanced fork phase?
        int isCurrentForkPhasedInstanced = 0;    // Is the current fork phase instanced?
        const char* asPhaseFuncNames[NUM_PHASES];
        uint32_t ui32PhaseFuncCallOrder[3];
        uint32_t ui32PhaseCallIndex;

        uint32_t ui32Phase;
        uint32_t ui32Instance;

        asPhaseFuncNames[MAIN_PHASE] = "";
        asPhaseFuncNames[HS_GLOBAL_DECL] = "";
        asPhaseFuncNames[HS_FORK_PHASE] = "fork_phase";
        asPhaseFuncNames[HS_CTRL_POINT_PHASE] = "control_point_phase";
        asPhaseFuncNames[HS_JOIN_PHASE] = "join_phase";

        ConsolidateHullTempVars(psShader);

        for(i=0; i < psShader->asPhase[HS_GLOBAL_DECL].pui32DeclCount[0]; ++i)
        {
            TranslateDeclaration(psContext, psShader->asPhase[HS_GLOBAL_DECL].ppsDecl[0]+i);
        }

        for(ui32Phase=HS_CTRL_POINT_PHASE; ui32Phase<NUM_PHASES; ui32Phase++)
        {
            psContext->currentPhase = ui32Phase;
            for(ui32Instance = 0; ui32Instance < psShader->asPhase[ui32Phase].ui32InstanceCount; ++ui32Instance)
            {
                isCurrentForkPhasedInstanced = 0; //reset for each fork phase for cases we don't have a fork phase instance count opcode.
                bformata(glsl, "//%s declarations\n", asPhaseFuncNames[ui32Phase]);
                for(i=0; i < psShader->asPhase[ui32Phase].pui32DeclCount[ui32Instance]; ++i)
                {
                    TranslateDeclaration(psContext, psShader->asPhase[ui32Phase].ppsDecl[ui32Instance]+i);
                    if(psShader->asPhase[ui32Phase].ppsDecl[ui32Instance][i].eOpcode == OPCODE_DCL_HS_FORK_PHASE_INSTANCE_COUNT)
                    {
                        haveInstancedForkPhase = 1;
                        isCurrentForkPhasedInstanced = 1;
                    }
                }

                bformata(glsl, "void %s%d()\n{\n", asPhaseFuncNames[ui32Phase], ui32Instance);
                psContext->indent++;

                SetDataTypes(psContext, psShader->asPhase[ui32Phase].ppsInst[ui32Instance], psShader->asPhase[ui32Phase].pui32InstCount[ui32Instance]-1);

                    if(isCurrentForkPhasedInstanced)
                    {
                        AddIndentation(psContext);
                        bformata(glsl, "for(int forkInstanceID = 0; forkInstanceID < HullPhase%dInstanceCount; ++forkInstanceID) {\n", ui32Instance);
                        psContext->indent++;
                    }

                        //The minus one here is remove the return statement at end of phases.
                        //This is needed otherwise the for loop will only run once.
                        ASSERT(psShader->asPhase[ui32Phase].ppsInst[ui32Instance]  [psShader->asPhase[ui32Phase].pui32InstCount[ui32Instance]-1].eOpcode == OPCODE_RET);
                        for(i=0; i < psShader->asPhase[ui32Phase].pui32InstCount[ui32Instance]-1; ++i)
                        {
                            TranslateInstruction(psContext, psShader->asPhase[ui32Phase].ppsInst[ui32Instance]+i, NULL);
                        }

                    if(haveInstancedForkPhase)
                    {
                        psContext->indent--;
                        AddIndentation(psContext);

                        if(isCurrentForkPhasedInstanced)
                        {
                            bcatcstr(glsl, "}\n");
                        }

                        if(psContext->havePostShaderCode[psContext->currentPhase])
                        {
    #ifdef _DEBUG
                            AddIndentation(psContext);
                            bcatcstr(glsl, "//--- Post shader code ---\n");
    #endif
                            bconcat(glsl, psContext->postShaderCode[psContext->currentPhase]);
    #ifdef _DEBUG
                            AddIndentation(psContext);
                            bcatcstr(glsl, "//--- End post shader code ---\n");
    #endif
                        }
                }

                psContext->indent--;
                bcatcstr(glsl, "}\n");
            }
        }

        bcatcstr(glsl, "void main()\n{\n");

            psContext->indent++;

#ifdef _DEBUG
            AddIndentation(psContext);
            bcatcstr(glsl, "//--- Start Early Main ---\n");
#endif
            bconcat(glsl, psContext->earlyMain);
#ifdef _DEBUG
            AddIndentation(psContext);
            bcatcstr(glsl, "//--- End Early Main ---\n");
#endif

            ui32PhaseFuncCallOrder[0] = HS_CTRL_POINT_PHASE;
            ui32PhaseFuncCallOrder[1] = HS_FORK_PHASE;
            ui32PhaseFuncCallOrder[2] = HS_JOIN_PHASE;

            for(ui32PhaseCallIndex=0; ui32PhaseCallIndex<3; ui32PhaseCallIndex++)
            {
                ui32Phase = ui32PhaseFuncCallOrder[ui32PhaseCallIndex];
                for(ui32Instance = 0; ui32Instance < psShader->asPhase[ui32Phase].ui32InstanceCount; ++ui32Instance)
                {
                    AddIndentation(psContext);
                    bformata(glsl, "%s%d();\n", asPhaseFuncNames[ui32Phase], ui32Instance);

                    if(ui32Phase == HS_FORK_PHASE)
                    {
                        if(psShader->asPhase[HS_JOIN_PHASE].ui32InstanceCount ||
                            (ui32Instance+1 < psShader->asPhase[HS_FORK_PHASE].ui32InstanceCount))
                        {
                            AddIndentation(psContext);
                            bcatcstr(glsl, "barrier();\n");
                        }
                    }
                }
            }

            psContext->indent--;

        bcatcstr(glsl, "}\n");

        return;
    }


    ui32InstCount = psShader->asPhase[MAIN_PHASE].pui32InstCount[0];
    ui32DeclCount = psShader->asPhase[MAIN_PHASE].pui32DeclCount[0];

    for(i=0; i < ui32DeclCount; ++i)
    {
        TranslateDeclaration(psContext, psShader->asPhase[MAIN_PHASE].ppsDecl[0]+i);
    }

    if(psContext->psShader->ui32NumDx9ImmConst)
    {
        bformata(psContext->mainShader, "vec4 ImmConstArray [%d];\n", psContext->psShader->ui32NumDx9ImmConst);
    }

    bcatcstr(glsl, "void main()\n{\n");

    psContext->indent++;

#ifdef _DEBUG
    AddIndentation(psContext);
    bcatcstr(glsl, "//--- Start Early Main ---\n");
#endif
    bconcat(glsl, psContext->earlyMain);
#ifdef _DEBUG
    AddIndentation(psContext);
    bcatcstr(glsl, "//--- End Early Main ---\n");
#endif

    MarkIntegerImmediates(psContext);

    SetDataTypes(psContext, psShader->asPhase[MAIN_PHASE].ppsInst[0], ui32InstCount);

    for(i=0; i < ui32InstCount; ++i)
    {
        TranslateInstruction(psContext, psShader->asPhase[MAIN_PHASE].ppsInst[0]+i, i+1 < ui32InstCount ? psShader->asPhase[MAIN_PHASE].ppsInst[0]+i+1 : 0);
    }

    psContext->indent--;

    bcatcstr(glsl, "}\n");
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

HLSLCC_API int HLSLCC_APIENTRY TranslateHLSLFromMemToGLSL(const char* shader,
    unsigned int flags,
    ShaderLang language,
    const GlExtensions *extensions,
    Shader* result)
{
    uint32_t* tokens;
    ShaderData* psShader;
    char* glslcstr = NULL;
    int GLSLShaderType = GL_FRAGMENT_SHADER_ARB;
    int success = 0;
    uint32_t i;

    tokens = (uint32_t*)shader;

    psShader = DecodeDXBC(tokens);

    if(psShader)
    {
        HLSLCrossCompilerContext sContext;

        if(psShader->ui32MajorVersion <= 3)
        {
            flags &= ~HLSLCC_FLAG_COMBINE_TEXTURE_SAMPLERS;
        }

        sContext.psShader = psShader;
        sContext.flags = flags;

        for(i=0; i<NUM_PHASES;++i)
        {
            sContext.havePostShaderCode[i] = 0;
        }

        TranslateToGLSL(&sContext, &language,extensions);

        switch(psShader->eShaderType)
        {
            case VERTEX_SHADER:
            {
                GLSLShaderType = GL_VERTEX_SHADER_ARB;
                break;
            }
            case GEOMETRY_SHADER:
            {
                GLSLShaderType = GL_GEOMETRY_SHADER;
                break;
            }
            case DOMAIN_SHADER:
            {
                GLSLShaderType = GL_TESS_EVALUATION_SHADER;
                break;
            }
            case HULL_SHADER:
            {
                GLSLShaderType = GL_TESS_CONTROL_SHADER;
                break;
            }
            case COMPUTE_SHADER:
            {
                GLSLShaderType = GL_COMPUTE_SHADER;
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

    result->shaderType = GLSLShaderType;
    result->sourceCode = glslcstr;
    result->GLSLLanguage = language;

    return success;
}

HLSLCC_API int HLSLCC_APIENTRY TranslateHLSLFromFileToGLSL(const char* filename,
    unsigned int flags,
    ShaderLang language,
    const GlExtensions *extensions,
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

    success = TranslateHLSLFromMemToGLSL(shader, flags, language, extensions, result);

    hlslcc_free(shader);

    return success;
}

HLSLCC_API void HLSLCC_APIENTRY FreeShader(Shader* s)
{
    bcstrfree(s->sourceCode);
    s->sourceCode = NULL;
    FreeShaderInfo(&s->reflection);
}

