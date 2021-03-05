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
#include "internal_includes/hlslccToolkit.h"
#include "../offline/hash.h"

#if defined(_WIN32) && !defined(PORTABLE)
#include <d3dcompiler.h>
#pragma comment(lib,"d3dcompiler.lib")
#endif //defined(_WIN32) && !defined(PORTABLE)

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


HLSLCC_API void HLSLCC_APIENTRY HLSLcc_SetMemoryFunctions(void* (*malloc_override)(size_t), void* (*calloc_override)(size_t, size_t), void (* free_override)(void*), void* (*realloc_override)(void*, size_t))
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
    bstring glsl = *psContext->currentGLSLString;
    for (i = 0; i < indent; ++i)
    {
        bcatcstr(glsl, "    ");
    }
}

uint32_t AddImport(HLSLCrossCompilerContext* psContext, SYMBOL_TYPE eType, uint32_t ui32ID, uint32_t ui32Default)
{
    bstring glsl = *psContext->currentGLSLString;
    uint32_t ui32Symbol = psContext->psShader->sInfo.ui32NumImports;

    psContext->psShader->sInfo.psImports = (Symbol*)hlslcc_realloc(psContext->psShader->sInfo.psImports, (ui32Symbol + 1) * sizeof(Symbol));
    ++psContext->psShader->sInfo.ui32NumImports;

    bformata(glsl, "#ifndef IMPORT_%d\n", ui32Symbol);
    bformata(glsl, "#define IMPORT_%d %d\n", ui32Symbol, ui32Default);
    bformata(glsl, "#endif\n", ui32Symbol);

    psContext->psShader->sInfo.psImports[ui32Symbol].eType = eType;
    psContext->psShader->sInfo.psImports[ui32Symbol].ui32ID = ui32ID;
    psContext->psShader->sInfo.psImports[ui32Symbol].ui32Value = ui32Default;

    return ui32Symbol;
}

uint32_t AddExport(HLSLCrossCompilerContext* psContext, SYMBOL_TYPE eType, uint32_t ui32ID, uint32_t ui32Value)
{
    uint32_t ui32Param = psContext->psShader->sInfo.ui32NumExports;

    psContext->psShader->sInfo.psExports = (Symbol*)hlslcc_realloc(psContext->psShader->sInfo.psExports, (ui32Param + 1) * sizeof(Symbol));
    ++psContext->psShader->sInfo.ui32NumExports;

    psContext->psShader->sInfo.psExports[ui32Param].eType = eType;
    psContext->psShader->sInfo.psExports[ui32Param].ui32ID = ui32ID;
    psContext->psShader->sInfo.psExports[ui32Param].ui32Value = ui32Value;

    return ui32Param;
}

void AddVersionDependentCode(HLSLCrossCompilerContext* psContext)
{
    bstring glsl = *psContext->currentGLSLString;
    uint32_t ui32DepthClampImp;

    if (!HaveCompute(psContext->psShader->eTargetLanguage))
    {
        if (psContext->psShader->eShaderType == COMPUTE_SHADER)
        {
            bcatcstr(glsl, "#extension GL_ARB_compute_shader : enable\n");
            bcatcstr(glsl, "#extension GL_ARB_shader_storage_buffer_object : enable\n");
        }
    }

    if (!HaveAtomicMem(psContext->psShader->eTargetLanguage) ||
        !HaveAtomicCounter(psContext->psShader->eTargetLanguage))
    {
        if (psContext->psShader->aiOpcodeUsed[OPCODE_IMM_ATOMIC_ALLOC] ||
            psContext->psShader->aiOpcodeUsed[OPCODE_IMM_ATOMIC_CONSUME] ||
            psContext->psShader->aiOpcodeUsed[OPCODE_DCL_UNORDERED_ACCESS_VIEW_STRUCTURED])
        {
            bcatcstr(glsl, "#extension GL_ARB_shader_atomic_counters : enable\n");

            bcatcstr(glsl, "#extension GL_ARB_shader_storage_buffer_object : enable\n");
        }
    }

    if (!HaveGather(psContext->psShader->eTargetLanguage))
    {
        if (psContext->psShader->aiOpcodeUsed[OPCODE_GATHER4] ||
            psContext->psShader->aiOpcodeUsed[OPCODE_GATHER4_PO_C] ||
            psContext->psShader->aiOpcodeUsed[OPCODE_GATHER4_PO] ||
            psContext->psShader->aiOpcodeUsed[OPCODE_GATHER4_C])
        {
            bcatcstr(glsl, "#extension GL_ARB_texture_gather : enable\n");
        }
    }

    if (!HaveGatherNonConstOffset(psContext->psShader->eTargetLanguage))
    {
        if (psContext->psShader->aiOpcodeUsed[OPCODE_GATHER4_PO_C] ||
            psContext->psShader->aiOpcodeUsed[OPCODE_GATHER4_PO])
        {
            bcatcstr(glsl, "#extension GL_ARB_gpu_shader5 : enable\n");
        }
    }

    if (!HaveQueryLod(psContext->psShader->eTargetLanguage))
    {
        if (psContext->psShader->aiOpcodeUsed[OPCODE_LOD])
        {
            bcatcstr(glsl, "#extension GL_ARB_texture_query_lod : enable\n");
        }
    }

    if (!HaveQueryLevels(psContext->psShader->eTargetLanguage))
    {
        if (psContext->psShader->aiOpcodeUsed[OPCODE_RESINFO])
        {
            bcatcstr(glsl, "#extension GL_ARB_texture_query_levels : enable\n");
        }
    }

    if (!HaveImageLoadStore(psContext->psShader->eTargetLanguage) && (psContext->flags & HLSLCC_FLAG_AVOID_SHADER_LOAD_STORE_EXTENSION) == 0)
    {
        if (psContext->psShader->aiOpcodeUsed[OPCODE_STORE_UAV_TYPED] ||
            psContext->psShader->aiOpcodeUsed[OPCODE_STORE_RAW] ||
            psContext->psShader->aiOpcodeUsed[OPCODE_STORE_STRUCTURED])
        {
            bcatcstr(glsl, "#extension GL_ARB_shader_image_load_store : enable\n");
            bcatcstr(glsl, "#extension GL_ARB_shader_bit_encoding : enable\n");
        }
        else
        if (psContext->psShader->aiOpcodeUsed[OPCODE_LD_UAV_TYPED] ||
            psContext->psShader->aiOpcodeUsed[OPCODE_LD_RAW] ||
            psContext->psShader->aiOpcodeUsed[OPCODE_LD_STRUCTURED])
        {
            bcatcstr(glsl, "#extension GL_ARB_shader_image_load_store : enable\n");
        }
    }


    // #extension directive must occur before any non-preprocessor token
    if (EmulateDepthClamp(psContext->psShader->eTargetLanguage) && (psContext->psShader->eShaderType == VERTEX_SHADER || psContext->psShader->eShaderType == PIXEL_SHADER))
    {
        char* szInOut = psContext->psShader->eShaderType == VERTEX_SHADER ? "out" : "in";
        ui32DepthClampImp = AddImport(psContext, SYMBOL_EMULATE_DEPTH_CLAMP, 0, 0);

        bformata(glsl, "#if IMPORT_%d > 0\n", ui32DepthClampImp);
        if (!HaveNoperspectiveInterpolation(psContext->psShader->eTargetLanguage))
        {
            bcatcstr(glsl, "#ifdef GL_NV_shader_noperspective_interpolation\n");
            bcatcstr(glsl, "#extension GL_NV_shader_noperspective_interpolation:enable\n");
            bformata(glsl, "#endif\n");
        }
        bformata(glsl, "#endif\n");
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

            bcatcstr(glsl, "varying vec4 OffsetColour;\n");
            bcatcstr(glsl, "varying vec4 BaseColour;\n");

            bcatcstr(glsl, "varying vec4 Fog;\n");

            for (texCoord = 0; texCoord < 8; ++texCoord)
            {
                bformata(glsl, "varying vec4 TexCoord%d;\n", texCoord);
            }

            for (renderTargets = 0; renderTargets < 8; ++renderTargets)
            {
                bformata(glsl, "#define Output%d gl_FragData[%d]\n", renderTargets, renderTargets);
            }
        }
    }


    if ((psContext->flags & HLSLCC_FLAG_ORIGIN_UPPER_LEFT)
        && (psContext->psShader->eTargetLanguage >= LANG_150)
        && (psContext->psShader->eShaderType == PIXEL_SHADER))
    {
        bcatcstr(glsl, "layout(origin_upper_left) in vec4 gl_FragCoord;\n");
    }

    if ((psContext->flags & HLSLCC_FLAG_PIXEL_CENTER_INTEGER)
        && (psContext->psShader->eTargetLanguage >= LANG_150))
    {
        bcatcstr(glsl, "layout(pixel_center_integer) in vec4 gl_FragCoord;\n");
    }

    /* For versions which do not support a vec1 (currently all versions) */
    bcatcstr(glsl, "struct vec1 {\n");
    if (psContext->psShader->eTargetLanguage == LANG_ES_300 || psContext->psShader->eTargetLanguage == LANG_ES_310 || psContext->psShader->eTargetLanguage == LANG_ES_100)
    {
        bcatcstr(glsl, "\thighp float x;\n");
    }
    else
    {
        bcatcstr(glsl, "\tfloat x;\n");
    }
    bcatcstr(glsl, "};\n");

    if (HaveUVec(psContext->psShader->eTargetLanguage))
    {
        bcatcstr(glsl, "struct uvec1 {\n");
        bcatcstr(glsl, "\tuint x;\n");
        bcatcstr(glsl, "};\n");
    }

    bcatcstr(glsl, "struct ivec1 {\n");
    bcatcstr(glsl, "\tint x;\n");
    bcatcstr(glsl, "};\n");

    /*
    OpenGL 4.1 API spec:
    To use any built-in input or output in the gl_PerVertex block in separable
    program objects, shader code must redeclare that block prior to use.
    */
    if (psContext->psShader->eShaderType == VERTEX_SHADER && psContext->psShader->eTargetLanguage >= LANG_410)
    {
        bcatcstr(glsl, "out gl_PerVertex {\n");
        bcatcstr(glsl, "vec4 gl_Position;\n");
        bcatcstr(glsl, "float gl_PointSize;\n");
        bcatcstr(glsl, "float gl_ClipDistance[];");
        bcatcstr(glsl, "};\n");
    }

    //The fragment language has no default precision qualifier for floating point types.
    if (psContext->psShader->eShaderType == PIXEL_SHADER &&
        psContext->psShader->eTargetLanguage == LANG_ES_100 || psContext->psShader->eTargetLanguage == LANG_ES_300  || psContext->psShader->eTargetLanguage == LANG_ES_310)
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
            //Only highp is valid for atomic_uint
            bcatcstr(glsl, "precision highp atomic_uint;\n");
        }
    }

    if (SubroutinesSupported(psContext->psShader->eTargetLanguage))
    {
        bcatcstr(glsl, "subroutine void SubroutineType();\n");
    }

    if (EmulateDepthClamp(psContext->psShader->eTargetLanguage) && (psContext->psShader->eShaderType == VERTEX_SHADER || psContext->psShader->eShaderType == PIXEL_SHADER))
    {
        char* szInOut = psContext->psShader->eShaderType == VERTEX_SHADER ? "out" : "in";

        bformata(glsl, "#if IMPORT_%d > 0\n", ui32DepthClampImp);
        if (!HaveNoperspectiveInterpolation(psContext->psShader->eTargetLanguage))
        {
            bcatcstr(glsl, "#ifdef GL_NV_shader_noperspective_interpolation\n");
        }
        bcatcstr(glsl, "#define EMULATE_DEPTH_CLAMP 1\n");
        bformata(glsl, "noperspective %s float unclampedDepth;\n", szInOut);
        if (!HaveNoperspectiveInterpolation(psContext->psShader->eTargetLanguage))
        {
            bcatcstr(glsl, "#else\n");
            bcatcstr(glsl, "#define EMULATE_DEPTH_CLAMP 2\n");
            bformata(glsl, "%s float unclampedZ;\n", szInOut);
            bformata(glsl, "#endif\n");
        }
        bformata(glsl, "#endif\n");

        if (psContext->psShader->eShaderType == PIXEL_SHADER)
        {
            bcatcstr(psContext->earlyMain, "#ifdef EMULATE_DEPTH_CLAMP\n");
            bcatcstr(psContext->earlyMain, "#if EMULATE_DEPTH_CLAMP == 2\n");
            bcatcstr(psContext->earlyMain, "\tfloat unclampedDepth = gl_DepthRange.near + unclampedZ *  gl_FragCoord.w;\n");
            bcatcstr(psContext->earlyMain, "#endif\n");
            bcatcstr(psContext->earlyMain, "\tgl_FragDepth = clamp(unclampedDepth, 0.0, 1.0);\n");
            bcatcstr(psContext->earlyMain, "#endif\n");
        }
    }
}

FRAMEBUFFER_FETCH_TYPE CollectGmemInfo(HLSLCrossCompilerContext* psContext)
{
    FRAMEBUFFER_FETCH_TYPE fetchType = FBF_NONE;
    Shader* psShader = psContext->psShader;
    memset(psContext->rendertargetUse, 0x00, sizeof(psContext->rendertargetUse));
    for (uint32_t i = 0; i < psShader->ui32DeclCount; ++i)
    {
        Declaration* decl = psShader->psDecl + i;
        if (decl->eOpcode == OPCODE_DCL_RESOURCE)
        {
            if (IsGmemReservedSlot(FBF_EXT_COLOR, decl->asOperands[0].ui32RegisterNumber))
            {
                int regNum = GetGmemInputResourceSlot(decl->asOperands[0].ui32RegisterNumber);
                ASSERT(regNum < MAX_COLOR_MRT);
                psContext->rendertargetUse[regNum] |= INPUT_RENDERTARGET;
                fetchType |= FBF_EXT_COLOR;
            }
            else if (IsGmemReservedSlot(FBF_ARM_COLOR, decl->asOperands[0].ui32RegisterNumber))
            {
                fetchType |= FBF_ARM_COLOR;
            }
            else if (IsGmemReservedSlot(FBF_ARM_DEPTH, decl->asOperands[0].ui32RegisterNumber))
            {
                fetchType |= FBF_ARM_DEPTH;
            }
            else if (IsGmemReservedSlot(FBF_ARM_STENCIL, decl->asOperands[0].ui32RegisterNumber))
            {
                fetchType |= FBF_ARM_STENCIL;
            }
        }
        else if (decl->eOpcode == OPCODE_DCL_OUTPUT && psShader->eShaderType == PIXEL_SHADER && decl->asOperands[0].eType != OPERAND_TYPE_OUTPUT_DEPTH)
        {
            ASSERT(decl->asOperands[0].ui32RegisterNumber < MAX_COLOR_MRT);
            psContext->rendertargetUse[decl->asOperands[0].ui32RegisterNumber] |= OUTPUT_RENDERTARGET;
        }
    }

    return fetchType;
}

uint16_t GetOpcodeWriteMask(OPCODE_TYPE eOpcode)
{
    switch (eOpcode)
    {
    default:
        ASSERT(0);

    // No writes
    case OPCODE_ENDREP:
    case OPCODE_REP:
    case OPCODE_BREAK:
    case OPCODE_BREAKC:
    case OPCODE_CALL:
    case OPCODE_CALLC:
    case OPCODE_CASE:
    case OPCODE_CONTINUE:
    case OPCODE_CONTINUEC:
    case OPCODE_CUT:
    case OPCODE_DISCARD:
    case OPCODE_ELSE:
    case OPCODE_EMIT:
    case OPCODE_EMITTHENCUT:
    case OPCODE_ENDIF:
    case OPCODE_ENDLOOP:
    case OPCODE_ENDSWITCH:
    case OPCODE_IF:
    case OPCODE_LABEL:
    case OPCODE_LOOP:
    case OPCODE_NOP:
    case OPCODE_RET:
    case OPCODE_RETC:
    case OPCODE_SWITCH:
    case OPCODE_HS_DECLS:
    case OPCODE_HS_CONTROL_POINT_PHASE:
    case OPCODE_HS_FORK_PHASE:
    case OPCODE_HS_JOIN_PHASE:
    case OPCODE_EMIT_STREAM:
    case OPCODE_CUT_STREAM:
    case OPCODE_EMITTHENCUT_STREAM:
    case OPCODE_INTERFACE_CALL:
    case OPCODE_STORE_UAV_TYPED:
    case OPCODE_STORE_RAW:
    case OPCODE_STORE_STRUCTURED:
    case OPCODE_ATOMIC_AND:
    case OPCODE_ATOMIC_OR:
    case OPCODE_ATOMIC_XOR:
    case OPCODE_ATOMIC_CMP_STORE:
    case OPCODE_ATOMIC_IADD:
    case OPCODE_ATOMIC_IMAX:
    case OPCODE_ATOMIC_IMIN:
    case OPCODE_ATOMIC_UMAX:
    case OPCODE_ATOMIC_UMIN:
    case OPCODE_SYNC:
    case OPCODE_ABORT:
    case OPCODE_DEBUG_BREAK:
        return 0;

    // Write to 0
    case OPCODE_POW:
    case OPCODE_DP2ADD:
    case OPCODE_LRP:
    case OPCODE_ADD:
    case OPCODE_AND:
    case OPCODE_DERIV_RTX:
    case OPCODE_DERIV_RTY:
    case OPCODE_DEFAULT:
    case OPCODE_DIV:
    case OPCODE_DP2:
    case OPCODE_DP3:
    case OPCODE_DP4:
    case OPCODE_EXP:
    case OPCODE_FRC:
    case OPCODE_ITOF:
    case OPCODE_LOG:
    case OPCODE_LT:
    case OPCODE_MAD:
    case OPCODE_MIN:
    case OPCODE_MAX:
    case OPCODE_MUL:
    case OPCODE_ROUND_NE:
    case OPCODE_ROUND_NI:
    case OPCODE_ROUND_PI:
    case OPCODE_ROUND_Z:
    case OPCODE_RSQ:
    case OPCODE_SQRT:
    case OPCODE_UTOF:
    case OPCODE_SAMPLE_POS:
    case OPCODE_SAMPLE_INFO:
    case OPCODE_DERIV_RTX_COARSE:
    case OPCODE_DERIV_RTX_FINE:
    case OPCODE_DERIV_RTY_COARSE:
    case OPCODE_DERIV_RTY_FINE:
    case OPCODE_RCP:
    case OPCODE_F32TOF16:
    case OPCODE_F16TOF32:
    case OPCODE_DTOF:
    case OPCODE_EQ:
    case OPCODE_FTOU:
    case OPCODE_GE:
    case OPCODE_IEQ:
    case OPCODE_IGE:
    case OPCODE_ILT:
    case OPCODE_NE:
    case OPCODE_NOT:
    case OPCODE_OR:
    case OPCODE_ULT:
    case OPCODE_UGE:
    case OPCODE_UMAD:
    case OPCODE_XOR:
    case OPCODE_UMAX:
    case OPCODE_UMIN:
    case OPCODE_USHR:
    case OPCODE_COUNTBITS:
    case OPCODE_FIRSTBIT_HI:
    case OPCODE_FIRSTBIT_LO:
    case OPCODE_FIRSTBIT_SHI:
    case OPCODE_UBFE:
    case OPCODE_BFI:
    case OPCODE_BFREV:
    case OPCODE_IMM_ATOMIC_AND:
    case OPCODE_IMM_ATOMIC_OR:
    case OPCODE_IMM_ATOMIC_XOR:
    case OPCODE_IMM_ATOMIC_EXCH:
    case OPCODE_IMM_ATOMIC_CMP_EXCH:
    case OPCODE_IMM_ATOMIC_UMAX:
    case OPCODE_IMM_ATOMIC_UMIN:
    case OPCODE_DEQ:
    case OPCODE_DGE:
    case OPCODE_DLT:
    case OPCODE_DNE:
    case OPCODE_MSAD:
    case OPCODE_DTOU:
    case OPCODE_FTOI:
    case OPCODE_IADD:
    case OPCODE_IMAD:
    case OPCODE_IMAX:
    case OPCODE_IMIN:
    case OPCODE_IMUL:
    case OPCODE_INE:
    case OPCODE_INEG:
    case OPCODE_ISHL:
    case OPCODE_ISHR:
    case OPCODE_BUFINFO:
    case OPCODE_IBFE:
    case OPCODE_IMM_ATOMIC_ALLOC:
    case OPCODE_IMM_ATOMIC_CONSUME:
    case OPCODE_IMM_ATOMIC_IADD:
    case OPCODE_IMM_ATOMIC_IMAX:
    case OPCODE_IMM_ATOMIC_IMIN:
    case OPCODE_DTOI:
    case OPCODE_DADD:
    case OPCODE_DMAX:
    case OPCODE_DMIN:
    case OPCODE_DMUL:
    case OPCODE_DMOV:
    case OPCODE_DMOVC:
    case OPCODE_FTOD:
    case OPCODE_DDIV:
    case OPCODE_DFMA:
    case OPCODE_DRCP:
    case OPCODE_ITOD:
    case OPCODE_UTOD:
    case OPCODE_LD:
    case OPCODE_LD_MS:
    case OPCODE_RESINFO:
    case OPCODE_SAMPLE:
    case OPCODE_SAMPLE_C:
    case OPCODE_SAMPLE_C_LZ:
    case OPCODE_SAMPLE_L:
    case OPCODE_SAMPLE_D:
    case OPCODE_SAMPLE_B:
    case OPCODE_LOD:
    case OPCODE_GATHER4:
    case OPCODE_GATHER4_C:
    case OPCODE_GATHER4_PO:
    case OPCODE_GATHER4_PO_C:
    case OPCODE_LD_UAV_TYPED:
    case OPCODE_LD_RAW:
    case OPCODE_LD_STRUCTURED:
    case OPCODE_EVAL_SNAPPED:
    case OPCODE_EVAL_SAMPLE_INDEX:
    case OPCODE_EVAL_CENTROID:
    case OPCODE_MOV:
    case OPCODE_MOVC:
        return 1u << 0;

    // Write to 0, 1
    case OPCODE_SINCOS:
    case OPCODE_UDIV:
    case OPCODE_UMUL:
    case OPCODE_UADDC:
    case OPCODE_USUBB:
    case OPCODE_SWAPC:
        return (1u << 0) | (1u << 1);
    }
}

void CreateTracingInfo(Shader* psShader)
{
    VariableTraceInfo asInputVarsInfo[MAX_SHADER_VEC4_INPUT * 4];
    uint32_t ui32NumInputVars = 0;
    uint32_t uInputVec, uInstruction;

    psShader->sInfo.ui32NumTraceSteps = psShader->ui32InstCount + 1;
    psShader->sInfo.psTraceSteps = hlslcc_malloc(sizeof(StepTraceInfo) * psShader->sInfo.ui32NumTraceSteps);

    for (uInputVec = 0; uInputVec < psShader->sInfo.ui32NumInputSignatures; ++uInputVec)
    {
        uint32_t ui32RWMask = psShader->sInfo.psInputSignatures[uInputVec].ui32ReadWriteMask;
        uint8_t ui8Component = 0;

        while (ui32RWMask != 0)
        {
            if (ui32RWMask & 1)
            {
                TRACE_VARIABLE_TYPE eType;
                switch (psShader->sInfo.psInputSignatures[uInputVec].eComponentType)
                {
                default:
                    ASSERT(0);
                case INOUT_COMPONENT_UNKNOWN:
                case INOUT_COMPONENT_UINT32:
                    eType = TRACE_VARIABLE_UINT;
                    break;
                case INOUT_COMPONENT_SINT32:
                    eType = TRACE_VARIABLE_SINT;
                    break;
                case INOUT_COMPONENT_FLOAT32:
                    eType = TRACE_VARIABLE_FLOAT;
                    break;
                }

                asInputVarsInfo[ui32NumInputVars].eGroup = TRACE_VARIABLE_INPUT;
                asInputVarsInfo[ui32NumInputVars].eType = eType;
                asInputVarsInfo[ui32NumInputVars].ui8Index = psShader->sInfo.psInputSignatures[uInputVec].ui32Register;
                asInputVarsInfo[ui32NumInputVars].ui8Component = ui8Component;
                ++ui32NumInputVars;
            }
            ui32RWMask >>= 1;
            ++ui8Component;
        }
    }

    psShader->sInfo.psTraceSteps[0].ui32NumVariables = ui32NumInputVars;
    psShader->sInfo.psTraceSteps[0].psVariables = hlslcc_malloc(sizeof(VariableTraceInfo) * ui32NumInputVars);
    memcpy(psShader->sInfo.psTraceSteps[0].psVariables, asInputVarsInfo, sizeof(VariableTraceInfo) * ui32NumInputVars);

    for (uInstruction = 0; uInstruction < psShader->ui32InstCount; ++uInstruction)
    {
        VariableTraceInfo* psStepVars = NULL;
        uint32_t ui32StepVarsCapacity = 0;
        uint32_t ui32StepVarsSize = 0;
        uint32_t auStepDirtyVecMask[MAX_TEMP_VEC4 + MAX_SHADER_VEC4_OUTPUT] = {0};
        uint8_t auStepCompTypeMask[4 * (MAX_TEMP_VEC4 + MAX_SHADER_VEC4_OUTPUT)] = {0};
        uint32_t uOpcodeWriteMask = GetOpcodeWriteMask(psShader->psInst[uInstruction].eOpcode);
        uint32_t uOperand, uStepVec;

        for (uOperand = 0; uOperand < psShader->psInst[uInstruction].ui32NumOperands; ++uOperand)
        {
            if (uOpcodeWriteMask & (1 << uOperand))
            {
                uint32_t ui32OperandCompMask = ConvertOperandSwizzleToComponentMask(&psShader->psInst[uInstruction].asOperands[uOperand]);
                uint32_t ui32Register = psShader->psInst[uInstruction].asOperands[uOperand].ui32RegisterNumber;
                uint32_t ui32VecOffset = 0;
                uint8_t ui8Component = 0;
                switch (psShader->psInst[uInstruction].asOperands[uOperand].eType)
                {
                case OPERAND_TYPE_TEMP:
                    ui32VecOffset = 0;
                    break;
                case OPERAND_TYPE_OUTPUT:
                    ui32VecOffset = MAX_TEMP_VEC4;
                    break;
                default:
                    continue;
                }

                auStepDirtyVecMask[ui32VecOffset + ui32Register] |= ui32OperandCompMask;
                while (ui32OperandCompMask)
                {
                    ASSERT(ui8Component < 4);
                    if (ui32OperandCompMask & 1)
                    {
                        TRACE_VARIABLE_TYPE eOperandCompType = TRACE_VARIABLE_UNKNOWN;
                        switch (psShader->psInst[uInstruction].asOperands[uOperand].aeDataType[ui8Component])
                        {
                        case SVT_INT:
                            eOperandCompType = TRACE_VARIABLE_SINT;
                            break;
                        case SVT_FLOAT:
                            eOperandCompType = TRACE_VARIABLE_FLOAT;
                            break;
                        case SVT_UINT:
                            eOperandCompType = TRACE_VARIABLE_UINT;
                            break;
                        case SVT_DOUBLE:
                            eOperandCompType = TRACE_VARIABLE_DOUBLE;
                            break;
                        }
                        if (auStepCompTypeMask[4 * (ui32VecOffset + ui32Register) + ui8Component] == 0)
                        {
                            auStepCompTypeMask[4 * (ui32VecOffset + ui32Register) + ui8Component] = 1u + (uint8_t)eOperandCompType;
                        }
                        else if (auStepCompTypeMask[4 * (ui32VecOffset + ui32Register) + ui8Component] != eOperandCompType)
                        {
                            auStepCompTypeMask[4 * (ui32VecOffset + ui32Register) + ui8Component] = 1u + (uint8_t)TRACE_VARIABLE_UNKNOWN;
                        }
                    }
                    ui32OperandCompMask >>= 1;
                    ++ui8Component;
                }
            }
        }

        for (uStepVec = 0; uStepVec < MAX_TEMP_VEC4 + MAX_SHADER_VEC4_OUTPUT; ++uStepVec)
        {
            TRACE_VARIABLE_GROUP eGroup;
            uint32_t uBase;
            uint8_t ui8Component = 0;
            if (uStepVec < MAX_TEMP_VEC4)
            {
                eGroup = TRACE_VARIABLE_TEMP;
                uBase = 0;
            }
            else
            {
                eGroup = TRACE_VARIABLE_OUTPUT;
                uBase = MAX_TEMP_VEC4;
            }

            while (auStepDirtyVecMask[uStepVec] != 0)
            {
                if (auStepDirtyVecMask[uStepVec] & 1)
                {
                    if (ui32StepVarsCapacity == ui32StepVarsSize)
                    {
                        ui32StepVarsCapacity = (1 > ui32StepVarsCapacity ? 1 : ui32StepVarsCapacity) * 16;
                        if (psStepVars == NULL)
                        {
                            psStepVars = hlslcc_malloc(ui32StepVarsCapacity * sizeof(VariableTraceInfo));
                        }
                        else
                        {
                            psStepVars = hlslcc_realloc(psStepVars, ui32StepVarsCapacity * sizeof(VariableTraceInfo));
                        }
                    }
                    ASSERT(ui32StepVarsSize < ui32StepVarsCapacity);

                    psStepVars[ui32StepVarsSize].eGroup = eGroup;
                    psStepVars[ui32StepVarsSize].eType = auStepCompTypeMask[4 * uStepVec + ui8Component] == 0 ? TRACE_VARIABLE_UNKNOWN : (TRACE_VARIABLE_TYPE)(auStepCompTypeMask[4 * uStepVec + ui8Component] - 1);
                    psStepVars[ui32StepVarsSize].ui8Component = ui8Component;
                    psStepVars[ui32StepVarsSize].ui8Index = uStepVec - uBase;
                    ++ui32StepVarsSize;
                }

                ++ui8Component;
                auStepDirtyVecMask[uStepVec] >>= 1;
            }
        }

        psShader->sInfo.psTraceSteps[1 + uInstruction].ui32NumVariables = ui32StepVarsSize;
        psShader->sInfo.psTraceSteps[1 + uInstruction].psVariables = psStepVars;
    }
}

void WriteTraceDeclarations(HLSLCrossCompilerContext* psContext)
{
    bstring glsl = *psContext->currentGLSLString;

    AddIndentation(psContext);
    bcatcstr(glsl, "layout (std430) buffer Trace\n");
    AddIndentation(psContext);
    bcatcstr(glsl, "{\n");
    ++psContext->indent;
    AddIndentation(psContext);
    bcatcstr(glsl, "uint uTraceSize;\n");
    AddIndentation(psContext);
    bcatcstr(glsl, "uint uTraceStride;\n");
    AddIndentation(psContext);
    bcatcstr(glsl, "uint uTraceCapacity;\n");
    switch (psContext->psShader->eShaderType)
    {
    case PIXEL_SHADER:
        AddIndentation(psContext);
        bcatcstr(glsl, "float fTracePixelCoordX;\n");
        AddIndentation(psContext);
        bcatcstr(glsl, "float fTracePixelCoordY;\n");
        break;
    case VERTEX_SHADER:
        AddIndentation(psContext);
        bcatcstr(glsl, "uint uTraceVertexID;\n");
        break;
    default:
        AddIndentation(psContext);
        bcatcstr(glsl, "// Trace ID not implelemented for this shader type\n");
        break;
    }
    AddIndentation(psContext);
    bcatcstr(glsl, "uint auTraceValues[];\n");
    --psContext->indent;
    AddIndentation(psContext);
    bcatcstr(glsl, "};\n");
}

void WritePreStepsTrace(HLSLCrossCompilerContext* psContext, StepTraceInfo* psStep)
{
    uint32_t uVar;
    bstring glsl = *psContext->currentGLSLString;

    AddIndentation(psContext);
    bcatcstr(glsl, "bool bRecord = ");
    switch (psContext->psShader->eShaderType)
    {
    case VERTEX_SHADER:
        bcatcstr(glsl, "uint(gl_VertexID) == uTraceVertexID");
        break;
    case PIXEL_SHADER:
        bcatcstr(glsl, "max(abs(gl_FragCoord.x - fTracePixelCoordX), abs(gl_FragCoord.y - fTracePixelCoordY)) <= 0.5");
        break;
    default:
        bcatcstr(glsl, "/* Trace condition not implelemented for this shader type */");
        bcatcstr(glsl, "false");
        break;
    }
    bcatcstr(glsl, ";\n");

    AddIndentation(psContext);
    bcatcstr(glsl, "uint uTraceIndex = atomicAdd(uTraceSize, uTraceStride * (bRecord ? 1 : 0));\n");
    AddIndentation(psContext);
    bcatcstr(glsl, "uint uTraceEnd = uTraceIndex + uTraceStride;\n");
    AddIndentation(psContext);
    bcatcstr(glsl, "bRecord = bRecord && uTraceEnd <= uTraceCapacity;\n");
    AddIndentation(psContext);
    bcatcstr(glsl, "uTraceEnd *= (bRecord ? 1 : 0);\n");

    if (psStep->ui32NumVariables > 0)
    {
        AddIndentation(psContext);
        bformata(glsl, "auTraceValues[min(++uTraceIndex, uTraceEnd)] = uint(0);\n"); // Adreno can't handle 0u (it's treated as int)

        for (uVar = 0; uVar < psStep->ui32NumVariables; ++uVar)
        {
            VariableTraceInfo* psVar = &psStep->psVariables[uVar];
            ASSERT(psVar->eGroup == TRACE_VARIABLE_INPUT);
            if (psVar->eGroup == TRACE_VARIABLE_INPUT)
            {
                AddIndentation(psContext);
                bcatcstr(glsl, "auTraceValues[min(++uTraceIndex, uTraceEnd)] = ");

                switch (psVar->eType)
                {
                case TRACE_VARIABLE_FLOAT:
                    bcatcstr(glsl, "floatBitsToUint(");
                    break;
                case TRACE_VARIABLE_SINT:
                    bcatcstr(glsl, "uint(");
                    break;
                case TRACE_VARIABLE_DOUBLE:
                    ASSERT(0);
                    // Not implemented yet;
                    break;
                }

                bformata(glsl, "Input%d.%c", psVar->ui8Index, "xyzw"[psVar->ui8Component]);

                switch (psVar->eType)
                {
                case TRACE_VARIABLE_FLOAT:
                case TRACE_VARIABLE_SINT:
                    bcatcstr(glsl, ")");
                    break;
                }

                bcatcstr(glsl, ";\n");
            }
        }
    }
}

void WritePostStepTrace(HLSLCrossCompilerContext* psContext, uint32_t uStep)
{
    Instruction* psInstruction = psContext->psShader->psInst + uStep;
    StepTraceInfo* psStep = psContext->psShader->sInfo.psTraceSteps + (1 + uStep);

    if (psStep->ui32NumVariables > 0)
    {
        uint32_t uVar;

        AddIndentation(psContext);
        bformata(psContext->glsl, "auTraceValues[min(++uTraceIndex, uTraceEnd)] = %du;\n", uStep + 1);

        for (uVar = 0; uVar < psStep->ui32NumVariables; ++uVar)
        {
            VariableTraceInfo* psVar = &psStep->psVariables[uVar];
            uint16_t uOpcodeWriteMask = GetOpcodeWriteMask(psInstruction->eOpcode);
            uint8_t uOperand = 0;
            OPERAND_TYPE eOperandType = OPERAND_TYPE_NULL;
            SHADER_VARIABLE_TYPE eVarToFlags = TO_FLAG_NONE;
            Operand* psOperand = NULL;
            uint32_t uiIgnoreSwizzle = 0;

            switch (psVar->eGroup)
            {
            case TRACE_VARIABLE_TEMP:
                eOperandType = OPERAND_TYPE_TEMP;
                break;
            case TRACE_VARIABLE_OUTPUT:
                eOperandType = OPERAND_TYPE_OUTPUT;
                break;
            }

            if (psVar->eType == TRACE_VARIABLE_DOUBLE)
            {
                ASSERT(0);
                // Not implemented yet
                continue;
            }
            while (uOpcodeWriteMask)
            {
                if (uOpcodeWriteMask & 1)
                {
                    if (eOperandType == psInstruction->asOperands[uOperand].eType &&
                        psVar->ui8Index == psInstruction->asOperands[uOperand].ui32RegisterNumber)
                    {
                        psOperand = &psInstruction->asOperands[uOperand];
                        break;
                    }
                }
                uOpcodeWriteMask >>= 1;
                ++uOperand;
            }

            if (psOperand == NULL)
            {
                ASSERT(0);
                continue;
            }

            AddIndentation(psContext);
            bcatcstr(psContext->glsl, "auTraceValues[min(++uTraceIndex, uTraceEnd)] = ");

            TranslateVariableName(psContext, psOperand, TO_FLAG_UNSIGNED_INTEGER, &uiIgnoreSwizzle);
            ASSERT(uiIgnoreSwizzle == 0);

            bformata(psContext->glsl, ".%c;\n", "xyzw"[psVar->ui8Component]);
        }
    }
}

void WriteEndTrace(HLSLCrossCompilerContext* psContext)
{
    AddIndentation(psContext);
    bcatcstr(psContext->glsl, "auTraceValues[min(++uTraceIndex, uTraceEnd)] = 0xFFFFFFFFu;\n");
}

int FindEmbeddedResourceName(EmbeddedResourceName* psEmbeddedName, HLSLCrossCompilerContext* psContext, bstring name)
{
    int offset = binstr(psContext->glsl, 0, name);
    int size = name->slen;

    if (offset == BSTR_ERR || size > 0x3FF || offset > 0x7FFFF)
    {
        return 0;
    }

    psEmbeddedName->ui20Offset = offset;
    psEmbeddedName->ui12Size = size;
    return 1;
}

void IgnoreSampler(ShaderInfo* psInfo, uint32_t index)
{
    if (index + 1 < psInfo->ui32NumSamplers)
    {
        psInfo->asSamplers[index] = psInfo->asSamplers[psInfo->ui32NumSamplers - 1];
    }
    --psInfo->ui32NumSamplers;
}

void IgnoreResource(Resource* psResources, uint32_t* puSize, uint32_t index)
{
    if (index + 1 < *puSize)
    {
        psResources[index] = psResources[*puSize - 1];
    }
    --*puSize;
}

void FillInResourceDescriptions(HLSLCrossCompilerContext* psContext)
{
    uint32_t i;
    bstring resourceName = bfromcstralloc(MAX_REFLECT_STRING_LENGTH, "");
    Shader* psShader = psContext->psShader;

    for (i = 0; i < psShader->sInfo.ui32NumSamplers; ++i)
    {
        Sampler* psSampler = psShader->sInfo.asSamplers + i;
        SamplerMask* psMask = &psSampler->sMask;
        if (psMask->bNormalSample || psMask->bCompareSample)
        {
            if (psMask->bNormalSample)
            {
                btrunc(resourceName, 0);
                TextureName(resourceName, psShader, psMask->ui10TextureBindPoint, psMask->ui10SamplerBindPoint, 0);
                if (!FindEmbeddedResourceName(&psSampler->sNormalName, psContext, resourceName))
                {
                    psMask->bNormalSample = 0;
                }
            }
            if (psMask->bCompareSample)
            {
                btrunc(resourceName, 0);
                TextureName(resourceName, psShader, psMask->ui10TextureBindPoint, psMask->ui10SamplerBindPoint, 1);
                if (!FindEmbeddedResourceName(&psSampler->sCompareName, psContext, resourceName))
                {
                    psMask->bCompareSample = 0;
                }
            }
            if (!psMask->bNormalSample && !psMask->bCompareSample)
            {
                IgnoreSampler(&psShader->sInfo, i); // Not used in the shader - ignore
            }
        }
        else
        {
            btrunc(resourceName, 0);
            TextureName(resourceName, psShader, psMask->ui10TextureBindPoint, psMask->ui10SamplerBindPoint, 0);
            if (!FindEmbeddedResourceName(&psSampler->sNormalName, psContext, resourceName))
            {
                IgnoreSampler(&psShader->sInfo, i); // Not used in the shader - ignore
            }
        }
    }

    for (i = 0; i < psShader->sInfo.ui32NumImages; ++i)
    {
        Resource* psResources = psShader->sInfo.asImages;
        uint32_t* puSize = &psShader->sInfo.ui32NumImages;

        Resource* psResource = psResources + i;
        ResourceBinding* psBinding = NULL;
        if (!GetResourceFromBindingPoint(psResource->eGroup, psResource->ui32BindPoint, &psShader->sInfo, &psBinding))
        {
            ASSERT(0);
            IgnoreResource(psResources, puSize, i);
        }

        btrunc(resourceName, 0);
        ConvertToUAVName(resourceName, psShader, psBinding->Name);
        if (!FindEmbeddedResourceName(&psResource->sName, psContext, resourceName))
        {
            IgnoreResource(psResources, puSize, i);
        }
    }

    for (i = 0; i < psShader->sInfo.ui32NumUniformBuffers; ++i)
    {
        Resource* psResources = psShader->sInfo.asUniformBuffers;
        uint32_t* puSize = &psShader->sInfo.ui32NumUniformBuffers;

        Resource* psResource = psResources + i;
        ConstantBuffer* psCB = NULL;
        GetConstantBufferFromBindingPoint(psResource->eGroup, psResource->ui32BindPoint, &psShader->sInfo, &psCB);

        btrunc(resourceName, 0);
        ConvertToUniformBufferName(resourceName, psShader, psCB->Name);
        if (!FindEmbeddedResourceName(&psResource->sName, psContext, resourceName))
        {
            IgnoreResource(psResources, puSize, i);
        }
    }

    for (i = 0; i < psShader->sInfo.ui32NumStorageBuffers; ++i)
    {
        Resource* psResources = psShader->sInfo.asStorageBuffers;
        uint32_t* puSize = &psShader->sInfo.ui32NumStorageBuffers;

        Resource* psResource = psResources + i;
        ConstantBuffer* psCB = NULL;
        GetConstantBufferFromBindingPoint(psResource->eGroup, psResource->ui32BindPoint, &psShader->sInfo, &psCB);

        btrunc(resourceName, 0);
        if (psResource->eGroup == RGROUP_UAV)
        {
            ConvertToUAVName(resourceName, psShader, psCB->Name);
        }
        else
        {
            ConvertToTextureName(resourceName, psShader, psCB->Name, NULL, 0);
        }
        if (!FindEmbeddedResourceName(&psResource->sName, psContext, resourceName))
        {
            IgnoreResource(psResources, puSize, i);
        }
    }

    bdestroy(resourceName);
}

GLLang ChooseLanguage(Shader* psShader)
{
    // Depends on the HLSL shader model extracted from bytecode.
    switch (psShader->ui32MajorVersion)
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

const char* GetVersionString(GLLang language)
{
    switch (language)
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

// Force precision of vertex output position to highp.
// Using mediump or lowp for the position of the vertex can cause rendering artifacts in OpenGL ES.
void ForcePositionOutputToHighp(Shader* shader)
{
    // Only sensible in vertex shaders
    if (shader->eShaderType != VERTEX_SHADER)
    {
        return;
    }

    // Find the output position declaration
    Declaration* posDeclaration = NULL;
    for (uint32_t i = 0; i < shader->ui32DeclCount; ++i)
    {
        Declaration* decl = shader->psDecl + i;
        if (decl->eOpcode == OPCODE_DCL_OUTPUT_SIV)
        {
            if (decl->asOperands[0].eSpecialName == NAME_POSITION)
            {
                posDeclaration = decl;
                break;
            }

            if (decl->asOperands[0].eSpecialName != NAME_UNDEFINED)
            {
                continue;
            }

            // This might be SV_Position (because d3dcompiler is weird). Get signature and check
            InOutSignature *sig = NULL;
            GetOutputSignatureFromRegister(decl->asOperands[0].ui32RegisterNumber, decl->asOperands[0].ui32CompMask, 0, &shader->sInfo, &sig);
            ASSERT(sig != NULL);
            if ((sig->eSystemValueType == NAME_POSITION || strcmp(sig->SemanticName, "POS") == 0) && sig->ui32SemanticIndex == 0)
            {
                sig->eMinPrec = MIN_PRECISION_DEFAULT;
                posDeclaration = decl;
                break;
            }
        }
        else if (decl->eOpcode == OPCODE_DCL_OUTPUT)
        {
            InOutSignature *sig = NULL;
            GetOutputSignatureFromRegister(decl->asOperands[0].ui32RegisterNumber, decl->asOperands[0].ui32CompMask, 0, &shader->sInfo, &sig);
            ASSERT(sig != NULL);
            if ((sig->eSystemValueType == NAME_POSITION || strcmp(sig->SemanticName, "POS") == 0) && sig->ui32SemanticIndex == 0)
            {
                sig->eMinPrec = MIN_PRECISION_DEFAULT;
                posDeclaration = decl;
                break;
            }
        }
    }

    // Do nothing if we don't find suitable output. This may well be INTERNALTESSPOS for tessellation etc.
    if (!posDeclaration)
    {
        return;
    }

    posDeclaration->asOperands[0].eMinPrecision = OPERAND_MIN_PRECISION_DEFAULT;
    posDeclaration->asOperands[0].eSpecialName = NAME_POSITION;
    // Go through all the instructions and update the operand.
    for (uint32_t i = 0; i < shader->ui32InstCount; ++i)
    {
        Instruction *inst = shader->psInst + i;
        for (uint32_t j = 0; j < inst->ui32FirstSrc; ++j)
        {
            Operand op = inst->asOperands[j];
            // Since it's an output declaration we know that there's only one
            // operand and it's in the first slot.
            if (op.eType == OPERAND_TYPE_OUTPUT && op.ui32RegisterNumber == posDeclaration->asOperands[0].ui32RegisterNumber)
            {
                op.eMinPrecision = OPERAND_MIN_PRECISION_DEFAULT;
                op.eSpecialName = NAME_POSITION;
            }
        }
    }
}

void TranslateToGLSL(HLSLCrossCompilerContext* psContext, GLLang* planguage, const GlExtensions* extensions)
{
    bstring glsl;
    uint32_t i;
    Shader* psShader = psContext->psShader;
    GLLang language = *planguage;
    const uint32_t ui32InstCount = psShader->ui32InstCount;
    const uint32_t ui32DeclCount = psShader->ui32DeclCount;

    psContext->indent = 0;

    if (language == LANG_DEFAULT)
    {
        language = ChooseLanguage(psShader);
        *planguage = language;
    }

    glsl = bfromcstralloc (1024, "");
    if (!(psContext->flags & HLSLCC_FLAG_NO_VERSION_STRING))
    {
        bcatcstr(glsl, GetVersionString(language));
    }

    if (psContext->flags & HLSLCC_FLAG_ADD_DEBUG_HEADER)
    {
        bstring version = glsl;
        glsl = psContext->debugHeader;
        bconcat(glsl, version);
        bdestroy(version);
    }

    psContext->glsl = glsl;
    psContext->earlyMain = bfromcstralloc (1024, "");
    for (i = 0; i < NUM_PHASES; ++i)
    {
        psContext->postShaderCode[i] = bfromcstralloc (1024, "");
    }

    psContext->currentGLSLString = &glsl;
    psShader->eTargetLanguage = language;
    psShader->extensions = (const struct GlExtensions*)extensions;
    psContext->currentPhase = MAIN_PHASE;

    if (extensions)
    {
        if (extensions->ARB_explicit_attrib_location)
        {
            bcatcstr(glsl, "#extension GL_ARB_explicit_attrib_location : require\n");
        }
        if (extensions->ARB_explicit_uniform_location)
        {
            bcatcstr(glsl, "#extension GL_ARB_explicit_uniform_location : require\n");
        }
        if (extensions->ARB_shading_language_420pack)
        {
            bcatcstr(glsl, "#extension GL_ARB_shading_language_420pack : require\n");
        }
    }

    psContext->psShader->sInfo.ui32SymbolsOffset = blength(glsl);

    FRAMEBUFFER_FETCH_TYPE fetchType = CollectGmemInfo(psContext);
    if (fetchType & FBF_EXT_COLOR)
    {
        bcatcstr(glsl, "#extension GL_EXT_shader_framebuffer_fetch : require\n");
    }
    if (fetchType & FBF_ARM_COLOR)
    {
        bcatcstr(glsl, "#extension GL_ARM_shader_framebuffer_fetch : require\n");
    }
    if (fetchType & (FBF_ARM_DEPTH | FBF_ARM_STENCIL))
    {
        bcatcstr(glsl, "#extension GL_ARM_shader_framebuffer_fetch_depth_stencil : require\n");
    }
    psShader->eGmemType = fetchType;

    AddVersionDependentCode(psContext);

    if (psContext->flags & HLSLCC_FLAG_UNIFORM_BUFFER_OBJECT)
    {
        bcatcstr(glsl, "layout(std140) uniform;\n");
    }

    //Special case. Can have multiple phases.
    if (psShader->eShaderType == HULL_SHADER)
    {
        int haveInstancedForkPhase = 0;
        uint32_t forkIndex = 0;

        ConsolidateHullTempVars(psShader);

        for (i = 0; i < psShader->ui32HSDeclCount; ++i)
        {
            TranslateDeclaration(psContext, psShader->psHSDecl + i);
        }

        //control
        psContext->currentPhase = HS_CTRL_POINT_PHASE;

        if (psShader->ui32HSControlPointDeclCount)
        {
            bcatcstr(glsl, "//Control point phase declarations\n");
            for (i = 0; i < psShader->ui32HSControlPointDeclCount; ++i)
            {
                TranslateDeclaration(psContext, psShader->psHSControlPointPhaseDecl + i);
            }
        }

        if (psShader->ui32HSControlPointInstrCount)
        {
            SetDataTypes(psContext, psShader->psHSControlPointPhaseInstr, psShader->ui32HSControlPointInstrCount, NULL);

            bcatcstr(glsl, "void control_point_phase()\n{\n");
            psContext->indent++;

            for (i = 0; i < psShader->ui32HSControlPointInstrCount; ++i)
            {
                TranslateInstruction(psContext, psShader->psHSControlPointPhaseInstr + i);
            }
            psContext->indent--;
            bcatcstr(glsl, "}\n");
        }

        //fork
        psContext->currentPhase = HS_FORK_PHASE;
        for (forkIndex = 0; forkIndex < psShader->ui32ForkPhaseCount; ++forkIndex)
        {
            bcatcstr(glsl, "//Fork phase declarations\n");
            for (i = 0; i < psShader->aui32HSForkDeclCount[forkIndex]; ++i)
            {
                TranslateDeclaration(psContext, psShader->apsHSForkPhaseDecl[forkIndex] + i);
                if (psShader->apsHSForkPhaseDecl[forkIndex][i].eOpcode == OPCODE_DCL_HS_FORK_PHASE_INSTANCE_COUNT)
                {
                    haveInstancedForkPhase = 1;
                }
            }

            bformata(glsl, "void fork_phase%d()\n{\n", forkIndex);
            psContext->indent++;

            SetDataTypes(psContext, psShader->apsHSForkPhaseInstr[forkIndex], psShader->aui32HSForkInstrCount[forkIndex] - 1, NULL);

            if (haveInstancedForkPhase)
            {
                AddIndentation(psContext);
                bformata(glsl, "for(int forkInstanceID = 0; forkInstanceID < HullPhase%dInstanceCount; ++forkInstanceID) {\n", forkIndex);
                psContext->indent++;
            }

            //The minus one here is remove the return statement at end of phases.
            //This is needed otherwise the for loop will only run once.
            ASSERT(psShader->apsHSForkPhaseInstr[forkIndex][psShader->aui32HSForkInstrCount[forkIndex] - 1].eOpcode == OPCODE_RET);
            for (i = 0; i < psShader->aui32HSForkInstrCount[forkIndex] - 1; ++i)
            {
                TranslateInstruction(psContext, psShader->apsHSForkPhaseInstr[forkIndex] + i);
            }

            if (haveInstancedForkPhase)
            {
                psContext->indent--;
                AddIndentation(psContext);
                bcatcstr(glsl, "}\n");

                if (psContext->havePostShaderCode[psContext->currentPhase])
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


        //join
        psContext->currentPhase = HS_JOIN_PHASE;
        if (psShader->ui32HSJoinDeclCount)
        {
            bcatcstr(glsl, "//Join phase declarations\n");
            for (i = 0; i < psShader->ui32HSJoinDeclCount; ++i)
            {
                TranslateDeclaration(psContext, psShader->psHSJoinPhaseDecl + i);
            }
        }

        if (psShader->ui32HSJoinInstrCount)
        {
            SetDataTypes(psContext, psShader->psHSJoinPhaseInstr, psShader->ui32HSJoinInstrCount, NULL);

            bcatcstr(glsl, "void join_phase()\n{\n");
            psContext->indent++;

            for (i = 0; i < psShader->ui32HSJoinInstrCount; ++i)
            {
                TranslateInstruction(psContext, psShader->psHSJoinPhaseInstr + i);
            }

            psContext->indent--;
            bcatcstr(glsl, "}\n");
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

        if (psShader->ui32HSControlPointInstrCount)
        {
            AddIndentation(psContext);
            bcatcstr(glsl, "control_point_phase();\n");

            if (psShader->ui32ForkPhaseCount || psShader->ui32HSJoinInstrCount)
            {
                AddIndentation(psContext);
                bcatcstr(glsl, "barrier();\n");
            }
        }
        for (forkIndex = 0; forkIndex < psShader->ui32ForkPhaseCount; ++forkIndex)
        {
            AddIndentation(psContext);
            bformata(glsl, "fork_phase%d();\n", forkIndex);

            if (psShader->ui32HSJoinInstrCount || (forkIndex + 1 < psShader->ui32ForkPhaseCount))
            {
                AddIndentation(psContext);
                bcatcstr(glsl, "barrier();\n");
            }
        }
        if (psShader->ui32HSJoinInstrCount)
        {
            AddIndentation(psContext);
            bcatcstr(glsl, "join_phase();\n");
        }

        psContext->indent--;

        bcatcstr(glsl, "}\n");

        return;
    }

    if (psShader->eShaderType == DOMAIN_SHADER)
    {
        uint32_t ui32TessOutPrimImp = AddImport(psContext, SYMBOL_TESSELLATOR_OUTPUT_PRIMITIVE, 0, (uint32_t)TESSELLATOR_OUTPUT_TRIANGLE_CCW);
        uint32_t ui32TessPartitioningImp = AddImport(psContext, SYMBOL_TESSELLATOR_PARTITIONING, 0, (uint32_t)TESSELLATOR_PARTITIONING_INTEGER);

        bformata(glsl, "#if IMPORT_%d == %d\n", ui32TessOutPrimImp, (uint32_t)TESSELLATOR_OUTPUT_POINT);
        bcatcstr(glsl, "layout(point_mode) in;\n");
        bformata(glsl, "#elif IMPORT_%d == %d\n", ui32TessOutPrimImp, (uint32_t)TESSELLATOR_OUTPUT_LINE);
        bcatcstr(glsl, "layout(isolines) in;\n");
        bformata(glsl, "#elif IMPORT_%d == %d\n", ui32TessOutPrimImp, (uint32_t)TESSELLATOR_OUTPUT_TRIANGLE_CW);
        bcatcstr(glsl, "layout(cw) in;\n");
        bcatcstr(glsl, "#endif\n");

        bformata(glsl, "#if IMPORT_%d == %d\n", ui32TessPartitioningImp, (uint32_t)TESSELLATOR_PARTITIONING_FRACTIONAL_ODD);
        bcatcstr(glsl, "layout(fractional_odd_spacing) in;\n");
        bformata(glsl, "#elif IMPORT_%d == %d\n", ui32TessPartitioningImp, (uint32_t)TESSELLATOR_PARTITIONING_FRACTIONAL_EVEN);
        bcatcstr(glsl, "layout(fractional_even_spacing) in;\n");
        bcatcstr(glsl, "#endif\n");
    }

    for (i = 0; i < ui32DeclCount; ++i)
    {
        TranslateDeclaration(psContext, psShader->psDecl + i);
    }

    if (psContext->psShader->ui32NumDx9ImmConst)
    {
        bformata(psContext->glsl, "vec4 ImmConstArray [%d];\n", psContext->psShader->ui32NumDx9ImmConst);
    }

    MarkIntegerImmediates(psContext);

    SetDataTypes(psContext, psShader->psInst, ui32InstCount, psContext->psShader->aeCommonTempVecType);

    if (psContext->flags & HLSLCC_FLAG_AVOID_TEMP_REGISTER_ALIASING)
    {
        for (i = 0; i < MAX_TEMP_VEC4; ++i)
        {
            switch (psShader->aeCommonTempVecType[i])
            {
            case SVT_VOID:
                psShader->aeCommonTempVecType[i] = SVT_FLOAT;
            case SVT_FLOAT:
            case SVT_FLOAT10:
            case SVT_FLOAT16:
            case SVT_UINT:
            case SVT_UINT8:
            case SVT_UINT16:
            case SVT_INT:
            case SVT_INT12:
            case SVT_INT16:
                bformata(psContext->glsl, "%s Temp%d", GetConstructorForTypeGLSL(psContext, psShader->aeCommonTempVecType[i], 4, true), i);
                break;
            case SVT_FORCE_DWORD:
                // temp register not used
                continue;
            default:
                continue;
            }

            if (psContext->flags & HLSLCC_FLAG_QUALCOMM_GLES30_DRIVER_WORKAROUND)
            {
                bformata(psContext->glsl, "[1]");
            }
            bformata(psContext->glsl, ";\n");
        }

        if (psContext->psShader->bUseTempCopy)
        {
            bcatcstr(psContext->glsl, "vec4 TempCopy;\n");
            bcatcstr(psContext->glsl, "uvec4 TempCopy_uint;\n");
            bcatcstr(psContext->glsl, "ivec4 TempCopy_int;\n");
        }
    }

    // Declare auxiliary variables used to save intermediate results to bypass driver issues
    SHADER_VARIABLE_TYPE auxVarType = SVT_UINT;
    bformata(psContext->glsl, "highp %s %s1;\n", GetConstructorForTypeGLSL(psContext, auxVarType, 4, false), GetAuxArgumentName(auxVarType));

    if (psContext->flags & HLSLCC_FLAG_TRACING_INSTRUMENTATION)
    {
        CreateTracingInfo(psShader);
        WriteTraceDeclarations(psContext);
    }

    bcatcstr(glsl, "void main()\n{\n");

    psContext->indent++;

#ifdef _DEBUG
    AddIndentation(psContext);
    bcatcstr(glsl, "//--- Start Early Main ---\n");
#endif
    bconcat(glsl, psContext->earlyMain);
    if (psContext->flags & HLSLCC_FLAG_TRACING_INSTRUMENTATION)
    {
        WritePreStepsTrace(psContext, psShader->sInfo.psTraceSteps);
    }
#ifdef _DEBUG
    AddIndentation(psContext);
    bcatcstr(glsl, "//--- End Early Main ---\n");
#endif

    for (i = 0; i < ui32InstCount; ++i)
    {
        TranslateInstruction(psContext, psShader->psInst + i);

        if (psContext->flags & HLSLCC_FLAG_TRACING_INSTRUMENTATION)
        {
            WritePostStepTrace(psContext, i);
        }
    }

    psContext->indent--;

    bcatcstr(glsl, "}\n");

    // Add exports
    if (psShader->eShaderType == PIXEL_SHADER)
    {
        uint32_t ui32Input;
        for (ui32Input = 0; ui32Input < MAX_SHADER_VEC4_INPUT; ++ui32Input)
        {
            INTERPOLATION_MODE eMode = psShader->sInfo.aePixelInputInterpolation[ui32Input];
            if (eMode != INTERPOLATION_LINEAR)
            {
                AddExport(psContext, SYMBOL_INPUT_INTERPOLATION_MODE, ui32Input, (uint32_t)eMode);
            }
        }
    }
    if (psShader->eShaderType == HULL_SHADER)
    {
        AddExport(psContext, SYMBOL_TESSELLATOR_PARTITIONING, 0, psShader->sInfo.eTessPartitioning);
        AddExport(psContext, SYMBOL_TESSELLATOR_OUTPUT_PRIMITIVE, 0, psShader->sInfo.eTessOutPrim);
    }

    FillInResourceDescriptions(psContext);
}

static void FreeSubOperands(Instruction* psInst, const uint32_t ui32NumInsts)
{
    uint32_t ui32Inst;
    for (ui32Inst = 0; ui32Inst < ui32NumInsts; ++ui32Inst)
    {
        Instruction* psCurrentInst = &psInst[ui32Inst];
        const uint32_t ui32NumOperands = psCurrentInst->ui32NumOperands;
        uint32_t ui32Operand;

        for (ui32Operand = 0; ui32Operand < ui32NumOperands; ++ui32Operand)
        {
            uint32_t ui32SubOperand;
            for (ui32SubOperand = 0; ui32SubOperand < MAX_SUB_OPERANDS; ++ui32SubOperand)
            {
                if (psCurrentInst->asOperands[ui32Operand].psSubOperand[ui32SubOperand])
                {
                    hlslcc_free(psCurrentInst->asOperands[ui32Operand].psSubOperand[ui32SubOperand]);
                    psCurrentInst->asOperands[ui32Operand].psSubOperand[ui32SubOperand] = NULL;
                }
            }
        }
    }
}

void RemoveDoubleUnderscores(char* szName)
{
    char* position;
    size_t length;
    length = strlen(szName);
    position = szName;
    while (position = strstr(position, "__"))
    {
        position[1] = '0';
        position += 2;
    }
}

void RemoveDoubleUnderscoresFromIdentifiers(Shader* psShader)
{
    uint32_t i, j;
    for (i = 0; i < psShader->sInfo.ui32NumConstantBuffers; ++i)
    {
        for (j = 0; j < psShader->sInfo.psConstantBuffers[i].ui32NumVars; ++j)
        {
            RemoveDoubleUnderscores(psShader->sInfo.psConstantBuffers[i].asVars[j].sType.Name);
        }
    }
}

HLSLCC_API int HLSLCC_APIENTRY TranslateHLSLFromMem(const char* shader, size_t size, unsigned int flags, GLLang language, const GlExtensions* extensions, GLSLShader* result)
{
    uint32_t* tokens;
    Shader* psShader;
    char* glslcstr = NULL;
    int GLSLShaderType = GL_FRAGMENT_SHADER_ARB;
    int success = 0;
    uint32_t i;

    tokens = (uint32_t*)shader;

    psShader = DecodeDXBC(tokens);

    if (flags & (HLSLCC_FLAG_HASH_INPUT | HLSLCC_FLAG_ADD_DEBUG_HEADER))
    {
        uint64_t ui64InputHash = hash64((const uint8_t*)tokens, tokens[6], 0);
        psShader->sInfo.ui32InputHash = (uint32_t)ui64InputHash ^ (uint32_t)(ui64InputHash >> 32);
    }

    RemoveDoubleUnderscoresFromIdentifiers(psShader);

    if (psShader)
    {
        ForcePositionOutputToHighp(psShader);
        HLSLCrossCompilerContext sContext;

        sContext.psShader = psShader;
        sContext.flags = flags;

        for (i = 0; i < NUM_PHASES; ++i)
        {
            sContext.havePostShaderCode[i] = 0;
        }

        if (flags & HLSLCC_FLAG_ADD_DEBUG_HEADER)
        {
#if defined(_WIN32) && !defined(PORTABLE)
            ID3DBlob* pDisassembly = NULL;
#endif //defined(_WIN32) && !defined(PORTABLE)

            sContext.debugHeader = bformat("// HASH = 0x%08X\n", psShader->sInfo.ui32InputHash);

#if defined(_WIN32) && !defined(PORTABLE)
            D3DDisassemble(shader, size, 0, "", &pDisassembly);
            bcatcstr(sContext.debugHeader, "/*\n");
            bcatcstr(sContext.debugHeader, (const char*)pDisassembly->lpVtbl->GetBufferPointer(pDisassembly));
            bcatcstr(sContext.debugHeader, "\n*/\n");
            pDisassembly->lpVtbl->Release(pDisassembly);
#endif //defined(_WIN32) && !defined(PORTABLE)
        }

        TranslateToGLSL(&sContext, &language, extensions);

        switch (psShader->eShaderType)
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

        glslcstr = bstr2cstr(sContext.glsl, '\0');

        bdestroy(sContext.glsl);
        bdestroy(sContext.earlyMain);
        for (i = 0; i < NUM_PHASES; ++i)
        {
            bdestroy(sContext.postShaderCode[i]);
        }

        hlslcc_free(psShader->psHSControlPointPhaseDecl);
        FreeSubOperands(psShader->psHSControlPointPhaseInstr, psShader->ui32HSControlPointInstrCount);
        hlslcc_free(psShader->psHSControlPointPhaseInstr);

        for (i = 0; i < psShader->ui32ForkPhaseCount; ++i)
        {
            hlslcc_free(psShader->apsHSForkPhaseDecl[i]);
            FreeSubOperands(psShader->apsHSForkPhaseInstr[i], psShader->aui32HSForkInstrCount[i]);
            hlslcc_free(psShader->apsHSForkPhaseInstr[i]);
        }
        hlslcc_free(psShader->psHSJoinPhaseDecl);
        FreeSubOperands(psShader->psHSJoinPhaseInstr, psShader->ui32HSJoinInstrCount);
        hlslcc_free(psShader->psHSJoinPhaseInstr);

        hlslcc_free(psShader->psDecl);
        FreeSubOperands(psShader->psInst, psShader->ui32InstCount);
        hlslcc_free(psShader->psInst);

        memcpy(&result->reflection, &psShader->sInfo, sizeof(psShader->sInfo));


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

HLSLCC_API int HLSLCC_APIENTRY TranslateHLSLFromFile(const char* filename, unsigned int flags, GLLang language, const GlExtensions* extensions, GLSLShader* result)
{
    FILE* shaderFile;
    int length;
    size_t readLength;
    char* shader;
    int success = 0;

    shaderFile = fopen(filename, "rb");

    if (!shaderFile)
    {
        return 0;
    }

    fseek(shaderFile, 0, SEEK_END);
    length = ftell(shaderFile);
    fseek(shaderFile, 0, SEEK_SET);

    shader = (char*)hlslcc_malloc(length + 1);

    readLength = fread(shader, 1, length, shaderFile);

    fclose(shaderFile);
    shaderFile = 0;

    shader[readLength] = '\0';

    success = TranslateHLSLFromMem(shader, readLength, flags, language, extensions, result);

    hlslcc_free(shader);

    return success;
}

HLSLCC_API void HLSLCC_APIENTRY FreeGLSLShader(GLSLShader* s)
{
    bcstrfree(s->sourceCode);
    s->sourceCode = NULL;
    FreeShaderInfo(&s->reflection);
}

