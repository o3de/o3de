// Modifications copyright Amazon.com, Inc. or its affiliates
#include "internal_includes/hlslccToolkit.h"
#include "internal_includes/debug.h"
#include "internal_includes/languages.h"

bool DoAssignmentDataTypesMatch(SHADER_VARIABLE_TYPE dest, SHADER_VARIABLE_TYPE src)
{
    if (src == dest)
        return true;

    if ((dest == SVT_FLOAT || dest == SVT_FLOAT10 || dest == SVT_FLOAT16) &&
        (src == SVT_FLOAT || src == SVT_FLOAT10 || src == SVT_FLOAT16))
        return true;

    if ((dest == SVT_INT || dest == SVT_INT12 || dest == SVT_INT16) &&
        (src == SVT_INT || src == SVT_INT12 || src == SVT_INT16))
        return true;

    if ((dest == SVT_UINT || dest == SVT_UINT16) &&
        (src == SVT_UINT || src == SVT_UINT16))
        return true;

    return false;
}

const char * GetConstructorForTypeGLSL(HLSLCrossCompilerContext* psContext, const SHADER_VARIABLE_TYPE eType, const int components, bool useGLSLPrecision)
{
    const bool usePrecision = useGLSLPrecision && HavePrecisionQualifers(psContext->psShader->eTargetLanguage);

    static const char * const uintTypes[] = { " ", "uint", "uvec2", "uvec3", "uvec4" };
    static const char * const uint16Types[] = { " ", "mediump uint", "mediump uvec2", "mediump uvec3", "mediump uvec4" };
    static const char * const intTypes[] = { " ", "int", "ivec2", "ivec3", "ivec4" };
    static const char * const int16Types[] = { " ", "mediump int", "mediump ivec2", "mediump ivec3", "mediump ivec4" };
    static const char * const int12Types[] = { " ", "lowp int", "lowp ivec2", "lowp ivec3", "lowp ivec4" };
    static const char * const floatTypes[] = { " ", "float", "vec2", "vec3", "vec4" };
    static const char * const float16Types[] = { " ", "mediump float", "mediump vec2", "mediump vec3", "mediump vec4" };
    static const char * const float10Types[] = { " ", "lowp float", "lowp vec2", "lowp vec3", "lowp vec4" };
    static const char * const boolTypes[] = { " ", "bool", "bvec2", "bvec3", "bvec4" };

    ASSERT(components >= 1 && components <= 4);

    switch (eType)
    {
    case SVT_UINT:
        return uintTypes[components];
    case SVT_UINT16:
        return usePrecision ? uint16Types[components] : uintTypes[components];
    case SVT_INT:
        return intTypes[components];
    case SVT_INT16:
        return usePrecision ? int16Types[components] : intTypes[components];
    case SVT_INT12:
        return usePrecision ? int12Types[components] : intTypes[components];
    case SVT_FLOAT:
        return floatTypes[components];
    case SVT_FLOAT16:
        return usePrecision ? float16Types[components] : floatTypes[components];
    case SVT_FLOAT10:
        return usePrecision ? float10Types[components] : floatTypes[components];
    case SVT_BOOL:
        return boolTypes[components];
    default:
        ASSERT(0);
        return "";
    }
}

SHADER_VARIABLE_TYPE TypeFlagsToSVTType(const uint32_t typeflags)
{
    if (typeflags & TO_FLAG_INTEGER)
        return SVT_INT;
    if (typeflags & TO_FLAG_UNSIGNED_INTEGER)
        return SVT_UINT;
    return SVT_FLOAT;
}

uint32_t SVTTypeToFlag(const SHADER_VARIABLE_TYPE eType)
{
    if (eType == SVT_FLOAT16 || eType == SVT_FLOAT10 || eType == SVT_FLOAT)
    {
        return TO_FLAG_FLOAT;
    }
    if (eType == SVT_UINT || eType == SVT_UINT16)
    {
        return TO_FLAG_UNSIGNED_INTEGER;
    }
    else if (eType == SVT_INT || eType == SVT_INT16 || eType == SVT_INT12)
    {
        return TO_FLAG_INTEGER;
    }
    else
    {
        return TO_FLAG_NONE;
    }
}

bool CanDoDirectCast(SHADER_VARIABLE_TYPE src, SHADER_VARIABLE_TYPE dest)
{
    // uint<->int<->bool conversions possible
    if ((src == SVT_INT || src == SVT_UINT || src == SVT_BOOL || src == SVT_INT12 || src == SVT_INT16 || src == SVT_UINT16) &&
        (dest == SVT_INT || dest == SVT_UINT || dest == SVT_BOOL || dest == SVT_INT12 || dest == SVT_INT16 || dest == SVT_UINT16))
        return true;

    // float<->double possible
    if ((src == SVT_FLOAT || src == SVT_DOUBLE || src == SVT_FLOAT16 || src == SVT_FLOAT10) &&
        (dest == SVT_FLOAT || dest == SVT_DOUBLE || dest == SVT_FLOAT16 || dest == SVT_FLOAT10))
        return true;

    return false;
}

const char* GetBitcastOp(SHADER_VARIABLE_TYPE from, SHADER_VARIABLE_TYPE to)
{
    static const char* intToFloat = "intBitsToFloat";
    static const char* uintToFloat = "uintBitsToFloat";
    static const char* floatToInt = "floatBitsToInt";
    static const char* floatToUint = "floatBitsToUint";

    if ((to == SVT_FLOAT || to == SVT_FLOAT16 || to == SVT_FLOAT10) && from == SVT_INT)
        return intToFloat;
    else if ((to == SVT_FLOAT || to == SVT_FLOAT16 || to == SVT_FLOAT10) && from == SVT_UINT)
        return uintToFloat;
    else if (to == SVT_INT && (from == SVT_FLOAT || from == SVT_FLOAT16 || from == SVT_FLOAT10))
        return floatToInt;
    else if (to == SVT_UINT && (from == SVT_FLOAT || from == SVT_FLOAT16 || from == SVT_FLOAT10))
        return floatToUint;

    ASSERT(0);
    return "";
}

bool IsGmemReservedSlot(FRAMEBUFFER_FETCH_TYPE typeMask, const uint32_t regNumber)
{
    if (((typeMask & FBF_ARM_COLOR) && regNumber == GMEM_ARM_COLOR_SLOT) ||
        ((typeMask & FBF_ARM_DEPTH) && regNumber == GMEM_ARM_DEPTH_SLOT) ||
        ((typeMask & FBF_ARM_STENCIL) && regNumber == GMEM_ARM_STENCIL_SLOT) || 
        ((typeMask & FBF_EXT_COLOR) && regNumber >= GMEM_FLOAT_START_SLOT))
    {
        return true;
    }

    return false;
}

const char * GetAuxArgumentName(const SHADER_VARIABLE_TYPE varType)
{
    switch (varType)
    {
    case SVT_UINT:
    case SVT_UINT8:
    case SVT_UINT16:
        return "uArg";
    case SVT_INT:
    case SVT_INT16:
    case SVT_INT12:
        return "iArg";
    case SVT_FLOAT:
    case SVT_FLOAT16:
    case SVT_FLOAT10:
        return "fArg";
    case SVT_BOOL:
        return "bArg";
    default:
        ASSERT(0);
        return "";
    }
}