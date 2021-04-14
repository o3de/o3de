// Modifications copyright Amazon.com, Inc. or its affiliates
// Modifications copyright Crytek GmbH

#include "internal_includes/toGLSLInstruction.h"
#include "internal_includes/toGLSLOperand.h"
#include "internal_includes/languages.h"
#include "internal_includes/hlslccToolkit.h"
#include "bstrlib.h"
#include "stdio.h"
#include "internal_includes/debug.h"

#include <stdbool.h>

#ifndef min
#define min(a, b)            (((a) < (b)) ? (a) : (b))
#endif

extern void AddIndentation(HLSLCrossCompilerContext* psContext);
extern void WriteEndTrace(HLSLCrossCompilerContext* psContext);

typedef enum
{
    CMP_EQ,
    CMP_LT,
    CMP_GE,
    CMP_NE,
} ComparisonType;

void BeginAssignmentEx(HLSLCrossCompilerContext* psContext, const Operand* psDestOperand, uint32_t uSrcToFlag, uint32_t bSaturate, const char* szDestSwizzle)
{
    if (psContext->flags & HLSLCC_FLAG_AVOID_TEMP_REGISTER_ALIASING && psContext->psShader->eShaderType != HULL_SHADER)
    {
        const char* szCastFunction = "";
        SHADER_VARIABLE_TYPE eSrcType;
        SHADER_VARIABLE_TYPE eDestType = GetOperandDataType(psContext, psDestOperand);
        uint32_t uDestElemCount = GetNumSwizzleElements(psDestOperand);

        eSrcType = TypeFlagsToSVTType(uSrcToFlag);
        if (bSaturate)
        {
            eSrcType = SVT_FLOAT;
        }

        if (!DoAssignmentDataTypesMatch(eDestType, eSrcType))
        {
            switch (eDestType)
            {
            case SVT_INT:
            case SVT_INT12:
            case SVT_INT16:
            {
                switch (eSrcType)
                {
                case SVT_UINT:
                case SVT_UINT16:
                    szCastFunction = GetConstructorForTypeGLSL(psContext, eDestType, uDestElemCount, false);
                    break;
                case SVT_FLOAT:
                    szCastFunction = "floatBitsToInt";
                    break;
                default:
                    // Bitcasts from lower precisions floats are ambiguous
                    ASSERT(0);
                    break;
                }
            }
            break;
            case SVT_UINT:
            case SVT_UINT16:
            {
                switch (eSrcType)
                {
                case SVT_INT:
                case SVT_INT12:
                case SVT_INT16:
                    szCastFunction = GetConstructorForTypeGLSL(psContext, eDestType, uDestElemCount, false);
                    break;
                case SVT_FLOAT:
                    szCastFunction = "floatBitsToUint";
                    break;
                default:
                    // Bitcasts from lower precisions floats are ambiguous
                    ASSERT(0);
                    break;
                }
            }
            break;
            case SVT_FLOAT:
            case SVT_FLOAT10:
            case SVT_FLOAT16:
            {
                switch (eSrcType)
                {
                case SVT_UINT:
                    szCastFunction = "uintBitsToFloat";
                    break;
                case SVT_INT:
                    szCastFunction = "intBitsToFloat";
                    break;
                default:
                    // Bitcasts from lower precisions int/uint are ambiguous
                    ASSERT(0);
                    break;
                }
            }
            break;
            default:
                ASSERT(0);
                break;
            }
        }

        TranslateOperand(psContext, psDestOperand, TO_FLAG_DESTINATION);
        if (szDestSwizzle)
        {
            bformata(*psContext->currentGLSLString, ".%s = %s(", szDestSwizzle, szCastFunction);
        }
        else
        {
            bformata(*psContext->currentGLSLString, " = %s(", szCastFunction);
        }
    }
    else
    {
        TranslateOperand(psContext, psDestOperand, TO_FLAG_DESTINATION | uSrcToFlag);
        if (szDestSwizzle)
        {
            bformata(*psContext->currentGLSLString, ".%s = ", szDestSwizzle);
        }
        else
        {
            bcatcstr(*psContext->currentGLSLString, " = ");
        }
    }
    if (bSaturate)
    {
        bcatcstr(*psContext->currentGLSLString, "clamp(");
    }
}

void BeginAssignment(HLSLCrossCompilerContext* psContext, const Operand* psDestOperand, uint32_t uSrcToFlag, uint32_t bSaturate)
{
    BeginAssignmentEx(psContext, psDestOperand, uSrcToFlag, bSaturate, NULL);
}

void EndAssignment(HLSLCrossCompilerContext* psContext, const Operand* psDestOperand, uint32_t uSrcToFlag, uint32_t bSaturate)
{
    (void)psDestOperand;
    (void)uSrcToFlag;

    if (bSaturate)
    {
        bcatcstr(*psContext->currentGLSLString, ", 0.0, 1.0)");
    }

    if (psContext->flags & HLSLCC_FLAG_AVOID_TEMP_REGISTER_ALIASING && psContext->psShader->eShaderType != HULL_SHADER)
    {
        bcatcstr(*psContext->currentGLSLString, ")");
    }
}

static void AddComparision(HLSLCrossCompilerContext* psContext, Instruction* psInst, ComparisonType eType,
    uint32_t typeFlag)
{
    bstring glsl = *psContext->currentGLSLString;
    const uint32_t destElemCount = GetNumSwizzleElements(&psInst->asOperands[0]);
    const uint32_t s0ElemCount = GetNumSwizzleElements(&psInst->asOperands[1]);
    const uint32_t s1ElemCount = GetNumSwizzleElements(&psInst->asOperands[2]);

    uint32_t minElemCount = destElemCount < s0ElemCount ? destElemCount : s0ElemCount;

    minElemCount = s1ElemCount < minElemCount ? s1ElemCount : minElemCount;

    if (typeFlag == TO_FLAG_NONE)
    {
        const SHADER_VARIABLE_TYPE e0Type = GetOperandDataType(psContext, &psInst->asOperands[1]);
        const SHADER_VARIABLE_TYPE e1Type = GetOperandDataType(psContext, &psInst->asOperands[2]);
        if (e0Type != e1Type)
        {
            typeFlag = TO_FLAG_INTEGER;
        }
        else
        {
            switch (e0Type)
            {
            case SVT_INT:
            case SVT_INT12:
            case SVT_INT16:
                typeFlag = TO_FLAG_INTEGER;
                break;
            case SVT_UINT:
            case SVT_UINT8:
            case SVT_UINT16:
                typeFlag = TO_FLAG_UNSIGNED_INTEGER;
                break;
            default:
                typeFlag = TO_FLAG_FLOAT;
            }
        }
    }

    if (destElemCount > 1)
    {
        const char* glslOpcode [] = {
            "equal",
            "lessThan",
            "greaterThanEqual",
            "notEqual",
        };
        char* constructor = "vec";

        if (typeFlag & TO_FLAG_INTEGER)
        {
            constructor = "ivec";
        }
        else if (typeFlag & TO_FLAG_UNSIGNED_INTEGER)
        {
            constructor = "uvec";
        }

        bstring varName = bfromcstr(GetAuxArgumentName(SVT_UINT));
        bcatcstr(varName, "1");

        //Component-wise compare
        AddIndentation(psContext);
        if (psContext->psShader->ui32MajorVersion < 4)
        {
            BeginAssignment(psContext, &psInst->asOperands[0], TO_FLAG_FLOAT, psInst->bSaturate);
        }
        else
        {
            // Qualcomm driver workaround. Save the operation result into
            // a temporary variable before assigning it to the register.
            bconcat(glsl, varName);
            AddSwizzleUsingElementCount(psContext, minElemCount);
            bcatcstr(glsl, " = ");
        }

        bformata(glsl, "uvec%d(%s(%s4(", minElemCount, glslOpcode[eType], constructor);
        TranslateOperand(psContext, &psInst->asOperands[1], typeFlag);
        bcatcstr(glsl, ")");
        TranslateOperandSwizzle(psContext, &psInst->asOperands[0]);
        //AddSwizzleUsingElementCount(psContext, minElemCount);
        bformata(glsl, ", %s4(", constructor);
        TranslateOperand(psContext, &psInst->asOperands[2], typeFlag);
        bcatcstr(glsl, ")");
        TranslateOperandSwizzle(psContext, &psInst->asOperands[0]);
        //AddSwizzleUsingElementCount(psContext, minElemCount);
        if (psContext->psShader->ui32MajorVersion < 4)
        {
            //Result is 1.0f or 0.0f
            bcatcstr(glsl, "))");
            EndAssignment(psContext, &psInst->asOperands[0], TO_FLAG_FLOAT, psInst->bSaturate);
        }
        else
        {
            bcatcstr(glsl, ")) * 0xFFFFFFFFu;\n");
            AddIndentation(psContext);
            BeginAssignment(psContext, &psInst->asOperands[0], TO_FLAG_UNSIGNED_INTEGER, psInst->bSaturate);
            bconcat(glsl, varName);
            AddSwizzleUsingElementCount(psContext, minElemCount);
            EndAssignment(psContext, &psInst->asOperands[0], TO_FLAG_UNSIGNED_INTEGER, psInst->bSaturate);
        }
        bcatcstr(glsl, ";\n");
    }
    else
    {
        const char* glslOpcode [] = {
            "==",
            "<",
            ">=",
            "!=",
        };

        bool qualcommWorkaround = (psContext->flags & HLSLCC_FLAG_QUALCOMM_GLES30_DRIVER_WORKAROUND) != 0;
        const char* tempVariableName = "cond";
        //Scalar compare
        AddIndentation(psContext);
        // There's a bug with Qualcomm OpenGLES 3.0 drivers that
        // makes something like this: "temp1.x = temp2.x == 0 ? 1.0f : 0.0f" always return 0.0f
        // The workaround is saving the result in a temp variable: bool cond = temp2.x == 0; temp1.x = !!cond ? 1.0f : 0.0f
        if (qualcommWorkaround)
        {
            bcatcstr(glsl, "{\n");
            ++psContext->indent;
            AddIndentation(psContext);
            bformata(glsl, "bool %s = ", tempVariableName);
            bcatcstr(glsl, "(");
            TranslateOperand(psContext, &psInst->asOperands[1], typeFlag);
            bcatcstr(glsl, ")");
            if (s0ElemCount > minElemCount)
            {
                AddSwizzleUsingElementCount(psContext, minElemCount);
            }
            bformata(glsl, " %s (", glslOpcode[eType]);
            TranslateOperand(psContext, &psInst->asOperands[2], typeFlag);
            bcatcstr(glsl, ")");
            if (s1ElemCount > minElemCount)
            {
                AddSwizzleUsingElementCount(psContext, minElemCount);
            }
            bcatcstr(glsl, ";\n");
            AddIndentation(psContext);
        }

        if (psContext->psShader->ui32MajorVersion < 4)
        {
            BeginAssignment(psContext, &psInst->asOperands[0], TO_FLAG_FLOAT, psInst->bSaturate);
        }
        else
        {
            BeginAssignment(psContext, &psInst->asOperands[0], TO_FLAG_UNSIGNED_INTEGER, psInst->bSaturate);
        }

        if (qualcommWorkaround)
        {
            // Using the temporary variable where we stored the result of the comparison for the ternary operator.
            bformata(glsl, "!!%s ", tempVariableName);
        }
        else
        {
            bcatcstr(glsl, "((");
            TranslateOperand(psContext, &psInst->asOperands[1], typeFlag);
            bcatcstr(glsl, ")");
            if (s0ElemCount > minElemCount)
            {
                AddSwizzleUsingElementCount(psContext, minElemCount);
            }
            bformata(glsl, " %s (", glslOpcode[eType]);
            TranslateOperand(psContext, &psInst->asOperands[2], typeFlag);
            bcatcstr(glsl, ")");
            if (s1ElemCount > minElemCount)
            {
                AddSwizzleUsingElementCount(psContext, minElemCount);
            }
            bcatcstr(glsl, ") ");
        }

        if (psContext->psShader->ui32MajorVersion < 4)
        {
            bcatcstr(glsl, "? 1.0f : 0.0f");
            EndAssignment(psContext, &psInst->asOperands[0], TO_FLAG_FLOAT, psInst->bSaturate);
        }
        else
        {
            bcatcstr(glsl, "? 0xFFFFFFFFu : uint(0)"); // Adreno can't handle 0u (it's treated as int)
            EndAssignment(psContext, &psInst->asOperands[0], TO_FLAG_UNSIGNED_INTEGER, psInst->bSaturate);
        }
        bcatcstr(glsl, ";\n");
        if (qualcommWorkaround)
        {
            --psContext->indent;
            AddIndentation(psContext);
            bcatcstr(glsl, "}\n");
        }
    }
}

static void AddMOVBinaryOp(HLSLCrossCompilerContext* psContext, const Operand* pDst, const Operand* pSrc, uint32_t bSrcCopy, uint32_t bSaturate)
{
    bstring glsl = *psContext->currentGLSLString;

    const SHADER_VARIABLE_TYPE eSrcType = GetOperandDataType(psContext, pSrc);
    uint32_t srcCount = GetNumSwizzleElements(pSrc);
    uint32_t dstCount = GetNumSwizzleElements(pDst);
    uint32_t bMismatched = 0;

    uint32_t ui32SrcFlags = TO_FLAG_FLOAT;
    if (!bSaturate)
    {
        switch (eSrcType)
        {
        case SVT_INT:
        case SVT_INT12:
        case SVT_INT16:
            ui32SrcFlags = TO_FLAG_INTEGER;
            break;
        case SVT_UINT:
        case SVT_UINT8:
        case SVT_UINT16:
            ui32SrcFlags = TO_FLAG_UNSIGNED_INTEGER;
            break;
        }
    }
    if (bSrcCopy)
    {
        ui32SrcFlags |= TO_FLAG_COPY;
    }

    AddIndentation(psContext);
    BeginAssignment(psContext, pDst, ui32SrcFlags, bSaturate);

    //Mismatched element count or destination has any swizzle
    if (srcCount != dstCount || (GetFirstOperandSwizzle(psContext, pDst) != -1))
    {
        bMismatched = 1;

        // Special case for immediate operands that can be folded into *vec4
        if (srcCount == 1)
        {
            switch (ui32SrcFlags)
            {
            case TO_FLAG_INTEGER:
                bcatcstr(glsl, "ivec4");
                break;
            case TO_FLAG_UNSIGNED_INTEGER:
                bcatcstr(glsl, "uvec4");
                break;
            default:
                bcatcstr(glsl, "vec4");
            }
        }

        bcatcstr(glsl, "(");
    }

    TranslateOperand(psContext, pSrc, ui32SrcFlags);

    if (bMismatched)
    {
        bcatcstr(glsl, ")");

        if (GetFirstOperandSwizzle(psContext, pDst) != -1)
        {
            TranslateOperandSwizzle(psContext, pDst);
        }
        else
        {
            AddSwizzleUsingElementCount(psContext, dstCount);
        }
    }

    EndAssignment(psContext, pDst, ui32SrcFlags, bSaturate);
    bcatcstr(glsl, ";\n");
}

static void AddMOVCBinaryOp(HLSLCrossCompilerContext* psContext, const Operand* pDest, uint32_t bDestCopy, const Operand* src0, const Operand* src1, const Operand* src2)
{
    bstring glsl = *psContext->currentGLSLString;

    uint32_t destElemCount = GetNumSwizzleElements(pDest);
    uint32_t s0ElemCount = GetNumSwizzleElements(src0);
    uint32_t s1ElemCount = GetNumSwizzleElements(src1);
    uint32_t s2ElemCount = GetNumSwizzleElements(src2);
    uint32_t destElem;
    int qualcommWorkaround = psContext->flags & HLSLCC_FLAG_QUALCOMM_GLES30_DRIVER_WORKAROUND;

    const char* swizzles = "xyzw";
    uint32_t eDstDataType;
    const char* szVecType;

    uint32_t uDestFlags = TO_FLAG_DESTINATION;
    if (bDestCopy)
    {
        uDestFlags |= TO_FLAG_COPY;
    }

    AddIndentation(psContext);
    // Qualcomm OpenGLES 3.0 bug that makes something likes this: 
    // temp4.xyz = vec3(floatsToInt(temp1).x != 0 ? temp2.x : temp2.x, floatsToInt(temp1).y != 0 ? temp2.y : temp2.y, floatsToInt(temp1).z != 0 ? temp2.z : temp2.z)
    // to fail in the ternary operator. The workaround is to save the floatToInt(temp1) into a temp variable:
    // { ivec4 cond = floatsToInt(temp1); temp4.xyz = vec3(cond.x != 0 ? temp2.x : temp2.x, cond.y != 0 ? temp2.y : temp2.y, cond.z != 0 ? temp2.z : temp2.z); }
    if (qualcommWorkaround)
    {
        bformata(glsl, "{\n");
        ++psContext->indent;
        AddIndentation(psContext);
        if (s0ElemCount > 1)
            bformata(glsl, "ivec%d cond = ", s0ElemCount);
        else
            bformata(glsl, "int cond = ");
        TranslateOperand(psContext, src0, TO_FLAG_INTEGER);
        bformata(glsl, ";\n");
        AddIndentation(psContext);
    }

    TranslateOperand(psContext, pDest, uDestFlags);

    switch (GetOperandDataType(psContext, pDest))
    {
    case SVT_UINT:
    case SVT_UINT8:
    case SVT_UINT16:
        szVecType = "uvec";
        eDstDataType = TO_FLAG_UNSIGNED_INTEGER;
        break;
    case SVT_INT:
    case SVT_INT12:
    case SVT_INT16:
        szVecType = "ivec";
        eDstDataType = TO_FLAG_INTEGER;
        break;
    default:
        szVecType = "vec";
        eDstDataType = TO_FLAG_FLOAT;
        break;
    }

    if (destElemCount > 1)
    {
        bformata(glsl, " = %s%d(", szVecType, destElemCount);
    }
    else
    {
        bcatcstr(glsl, " = ");
    }

    for (destElem = 0; destElem < destElemCount; ++destElem)
    {
        if (destElem > 0)
        {
            bcatcstr(glsl, ", ");
        }

        if (qualcommWorkaround)
        {
            bcatcstr(glsl, "cond");
        }
        else
        {
            TranslateOperand(psContext, src0, TO_FLAG_INTEGER);
        }

        if (s0ElemCount > 1)
        {
            TranslateOperandSwizzle(psContext, pDest);
            bformata(glsl, ".%c", swizzles[destElem]);
        }

        bcatcstr(glsl, " != 0 ? ");

        TranslateOperand(psContext, src1, eDstDataType);
        if (s1ElemCount > 1)
        {
            TranslateOperandSwizzle(psContext, pDest);
            bformata(glsl, ".%c", swizzles[destElem]);
        }

        bcatcstr(glsl, " : ");

        TranslateOperand(psContext, src2, eDstDataType);
        if (s2ElemCount > 1)
        {
            TranslateOperandSwizzle(psContext, pDest);
            bformata(glsl, ".%c", swizzles[destElem]);
        }
    }
    if (destElemCount > 1)
    {
        bcatcstr(glsl, ");\n");
    }
    else
    {
        bcatcstr(glsl, ";\n");
    }

    if (qualcommWorkaround)
    {
        --psContext->indent;
        AddIndentation(psContext);
        bcatcstr(glsl, "}\n");
    }
}

void CallBinaryOp(HLSLCrossCompilerContext* psContext, const char* name, Instruction* psInst,
    int dest, int src0, int src1, uint32_t dataType)
{
    bstring glsl = *psContext->currentGLSLString;
    uint32_t src1SwizCount = GetNumSwizzleElements(&psInst->asOperands[src1]);
    uint32_t src0SwizCount = GetNumSwizzleElements(&psInst->asOperands[src0]);
    uint32_t dstSwizCount = GetNumSwizzleElements(&psInst->asOperands[dest]);

    AddIndentation(psContext);
    // Qualcomm OpenGLES 3.0 drivers don't support bitwise operators for vectors.
    // Because of this we need to do the operation per component.
    bool qualcommWorkaround = (psContext->flags & HLSLCC_FLAG_QUALCOMM_GLES30_DRIVER_WORKAROUND) != 0;
    bool isBitwiseOperator = psInst->eOpcode == OPCODE_AND || psInst->eOpcode == OPCODE_OR || psInst->eOpcode == OPCODE_XOR;
    const char* swizzleString[] = { ".x", ".y", ".z", ".w" };
    if (src1SwizCount == src0SwizCount == dstSwizCount)
    {
        BeginAssignment(psContext, &psInst->asOperands[dest], dataType, psInst->bSaturate);
        if (qualcommWorkaround && isBitwiseOperator && src0SwizCount > 1)
        {
            for (uint32_t i = 0; i < src0SwizCount; ++i)
            {
                if (i > 0)
                {
                    bcatcstr(glsl, ", ");
                }
                TranslateOperand(psContext, &psInst->asOperands[src0], TO_FLAG_NONE | dataType);
                bformata(glsl, "%s", swizzleString[i]);
                bformata(glsl, " %s ", name);
                TranslateOperand(psContext, &psInst->asOperands[src1], TO_FLAG_NONE | dataType);
                bformata(glsl, "%s", swizzleString[i]);
            }
        }
        else
        {
            TranslateOperand(psContext, &psInst->asOperands[src0], TO_FLAG_NONE | dataType);
            bformata(glsl, " %s ", name);
            TranslateOperand(psContext, &psInst->asOperands[src1], TO_FLAG_NONE | dataType);
        }
        EndAssignment(psContext, &psInst->asOperands[dest], dataType, psInst->bSaturate);
        bcatcstr(glsl, ";\n");
    }
    else
    {
        //Upconvert the inputs to vec4 then apply the dest swizzle.
        BeginAssignment(psContext, &psInst->asOperands[dest], dataType, psInst->bSaturate);
        if (dataType == TO_FLAG_UNSIGNED_INTEGER)
        {
            bcatcstr(glsl, "uvec4(");
        }
        else if (dataType == TO_FLAG_INTEGER)
        {
            bcatcstr(glsl, "ivec4(");
        }
        else
        {
            bcatcstr(glsl, "vec4(");
        }

        if (qualcommWorkaround && isBitwiseOperator && src0SwizCount > 1)
        {
            for (uint32_t i = 0; i < src0SwizCount; ++i)
            {
                if (i > 0)
                {
                    bcatcstr(glsl, ", ");
                }
                TranslateOperand(psContext, &psInst->asOperands[src0], TO_FLAG_NONE | dataType);
                bformata(glsl, "%s", swizzleString[i]);
                bformata(glsl, " %s ", name);
                TranslateOperand(psContext, &psInst->asOperands[src1], TO_FLAG_NONE | dataType);
                bformata(glsl, "%s", swizzleString[i]);
            }
        }
        else
        {
            TranslateOperand(psContext, &psInst->asOperands[src0], TO_FLAG_NONE | dataType);
            bformata(glsl, " %s ", name);
            TranslateOperand(psContext, &psInst->asOperands[src1], TO_FLAG_NONE | dataType);
        }
        bcatcstr(glsl, ")");
        //Limit src swizzles based on dest swizzle
        //e.g. given hlsl asm: add r0.xy, v0.xyxx, l(0.100000, 0.000000, 0.000000, 0.000000)
        //the two sources must become vec2
        //Temp0.xy = Input0.xyxx + vec4(0.100000, 0.000000, 0.000000, 0.000000);
        //becomes
        //Temp0.xy = vec4(Input0.xyxx + vec4(0.100000, 0.000000, 0.000000, 0.000000)).xy;

        TranslateOperandSwizzle(psContext, &psInst->asOperands[dest]);
        EndAssignment(psContext, &psInst->asOperands[dest], dataType, psInst->bSaturate);
        bcatcstr(glsl, ";\n");
    }
}

void CallTernaryOp(HLSLCrossCompilerContext* psContext, const char* op1, const char* op2, Instruction* psInst,
    int dest, int src0, int src1, int src2, uint32_t dataType)
{
    bstring glsl = *psContext->currentGLSLString;
    uint32_t src2SwizCount = GetNumSwizzleElements(&psInst->asOperands[src2]);
    uint32_t src1SwizCount = GetNumSwizzleElements(&psInst->asOperands[src1]);
    uint32_t src0SwizCount = GetNumSwizzleElements(&psInst->asOperands[src0]);
    uint32_t dstSwizCount = GetNumSwizzleElements(&psInst->asOperands[dest]);

    AddIndentation(psContext);

    if (src1SwizCount == src0SwizCount == src2SwizCount == dstSwizCount)
    {
        BeginAssignment(psContext, &psInst->asOperands[dest], dataType, psInst->bSaturate);
        TranslateOperand(psContext, &psInst->asOperands[src0], TO_FLAG_NONE | dataType);
        bformata(glsl, " %s ", op1);
        TranslateOperand(psContext, &psInst->asOperands[src1], TO_FLAG_NONE | dataType);
        bformata(glsl, " %s ", op2);
        TranslateOperand(psContext, &psInst->asOperands[src2], TO_FLAG_NONE | dataType);
        EndAssignment(psContext, &psInst->asOperands[dest], dataType, psInst->bSaturate);
        bcatcstr(glsl, ";\n");
    }
    else
    {
        BeginAssignment(psContext, &psInst->asOperands[dest], dataType, psInst->bSaturate);
        if (dataType == TO_FLAG_UNSIGNED_INTEGER)
        {
            bcatcstr(glsl, "uvec4(");
        }
        else if (dataType == TO_FLAG_INTEGER)
        {
            bcatcstr(glsl, "ivec4(");
        }
        else
        {
            bcatcstr(glsl, "vec4(");
        }
        TranslateOperand(psContext, &psInst->asOperands[src0], TO_FLAG_NONE | dataType);
        bformata(glsl, " %s ", op1);
        TranslateOperand(psContext, &psInst->asOperands[src1], TO_FLAG_NONE | dataType);
        bformata(glsl, " %s ", op2);
        TranslateOperand(psContext, &psInst->asOperands[src2], TO_FLAG_NONE | dataType);
        bcatcstr(glsl, ")");
        //Limit src swizzles based on dest swizzle
        //e.g. given hlsl asm: add r0.xy, v0.xyxx, l(0.100000, 0.000000, 0.000000, 0.000000)
        //the two sources must become vec2
        //Temp0.xy = Input0.xyxx + vec4(0.100000, 0.000000, 0.000000, 0.000000);
        //becomes
        //Temp0.xy = vec4(Input0.xyxx + vec4(0.100000, 0.000000, 0.000000, 0.000000)).xy;
        TranslateOperandSwizzle(psContext, &psInst->asOperands[dest]);
        EndAssignment(psContext, &psInst->asOperands[dest], dataType, psInst->bSaturate);
        bcatcstr(glsl, ";\n");
    }
}

void CallHelper3(HLSLCrossCompilerContext* psContext, const char* name, Instruction* psInst,
    int dest, int src0, int src1, int src2)
{
    bstring glsl = *psContext->currentGLSLString;
    AddIndentation(psContext);

    BeginAssignment(psContext, &psInst->asOperands[dest], TO_FLAG_FLOAT, psInst->bSaturate);

    bcatcstr(glsl, "vec4(");

    bcatcstr(glsl, name);
    bcatcstr(glsl, "(");
    TranslateOperand(psContext, &psInst->asOperands[src0], TO_FLAG_DESTINATION);
    bcatcstr(glsl, ", ");
    TranslateOperand(psContext, &psInst->asOperands[src1], TO_FLAG_FLOAT);
    bcatcstr(glsl, ", ");
    TranslateOperand(psContext, &psInst->asOperands[src2], TO_FLAG_FLOAT);
    bcatcstr(glsl, "))");
    TranslateOperandSwizzle(psContext, &psInst->asOperands[dest]);
    EndAssignment(psContext, &psInst->asOperands[dest], TO_FLAG_FLOAT, psInst->bSaturate);
    bcatcstr(glsl, ";\n");
}

void CallHelper2(HLSLCrossCompilerContext* psContext, const char* name, Instruction* psInst,
    int dest, int src0, int src1)
{
    bstring glsl = *psContext->currentGLSLString;
    AddIndentation(psContext);

    BeginAssignment(psContext, &psInst->asOperands[dest], TO_FLAG_FLOAT, psInst->bSaturate);

    bcatcstr(glsl, "vec4(");

    bcatcstr(glsl, name);
    bcatcstr(glsl, "(");
    TranslateOperand(psContext, &psInst->asOperands[src0], TO_FLAG_FLOAT);
    bcatcstr(glsl, ", ");
    TranslateOperand(psContext, &psInst->asOperands[src1], TO_FLAG_FLOAT);
    bcatcstr(glsl, "))");
    TranslateOperandSwizzle(psContext, &psInst->asOperands[dest]);
    EndAssignment(psContext, &psInst->asOperands[dest], TO_FLAG_FLOAT, psInst->bSaturate);
    bcatcstr(glsl, ";\n");
}

void CallHelper2Int(HLSLCrossCompilerContext* psContext, const char* name, Instruction* psInst,
    int dest, int src0, int src1)
{
    bstring glsl = *psContext->currentGLSLString;
    AddIndentation(psContext);

    BeginAssignment(psContext, &psInst->asOperands[dest], TO_FLAG_INTEGER, psInst->bSaturate);

    bcatcstr(glsl, "ivec4(");

    bcatcstr(glsl, name);
    bcatcstr(glsl, "(int(");
    TranslateOperand(psContext, &psInst->asOperands[src0], TO_FLAG_INTEGER);
    bcatcstr(glsl, "), int(");
    TranslateOperand(psContext, &psInst->asOperands[src1], TO_FLAG_INTEGER);
    bcatcstr(glsl, ")))");
    TranslateOperandSwizzle(psContext, &psInst->asOperands[dest]);
    EndAssignment(psContext, &psInst->asOperands[dest], TO_FLAG_INTEGER, psInst->bSaturate);
    bcatcstr(glsl, ";\n");
}
void CallHelper2UInt(HLSLCrossCompilerContext* psContext, const char* name, Instruction* psInst,
    int dest, int src0, int src1)
{
    bstring glsl = *psContext->currentGLSLString;
    AddIndentation(psContext);

    BeginAssignment(psContext, &psInst->asOperands[dest], TO_FLAG_UNSIGNED_INTEGER, psInst->bSaturate);

    bcatcstr(glsl, "uvec4(");

    bcatcstr(glsl, name);
    bcatcstr(glsl, "(uint(");
    TranslateOperand(psContext, &psInst->asOperands[src0], TO_FLAG_UNSIGNED_INTEGER);
    bcatcstr(glsl, "), uint(");
    TranslateOperand(psContext, &psInst->asOperands[src1], TO_FLAG_UNSIGNED_INTEGER);
    bcatcstr(glsl, ")))");
    TranslateOperandSwizzle(psContext, &psInst->asOperands[dest]);
    EndAssignment(psContext, &psInst->asOperands[dest], TO_FLAG_UNSIGNED_INTEGER, psInst->bSaturate);
    bcatcstr(glsl, ";\n");
}

void CallHelper1(HLSLCrossCompilerContext* psContext, const char* name, Instruction* psInst,
    int dest, int src0)
{
    bstring glsl = *psContext->currentGLSLString;

    AddIndentation(psContext);

    BeginAssignment(psContext, &psInst->asOperands[dest], TO_FLAG_FLOAT, psInst->bSaturate);

    // Qualcomm driver workaround
    // Example: Instead of Temp1.xyz = (vec4(log2(Temp0[0].xyzx)).xyz); we write 
    // Temp1.xyz = (log2(vec4(Temp0[0].xyzx).xyz));
    if (psContext->flags & HLSLCC_FLAG_QUALCOMM_GLES30_DRIVER_WORKAROUND)
    {
        bcatcstr(glsl, name);
        bcatcstr(glsl, "(");
        bcatcstr(glsl, "vec4(");
        TranslateOperand(psContext, &psInst->asOperands[src0], TO_FLAG_FLOAT);
        bcatcstr(glsl, ")");
        TranslateOperandSwizzle(psContext, &psInst->asOperands[dest]);
        bcatcstr(glsl, ")");
    }
    else
    {
        bcatcstr(glsl, "vec4(");
        bcatcstr(glsl, name);
        bcatcstr(glsl, "(");
        TranslateOperand(psContext, &psInst->asOperands[src0], TO_FLAG_FLOAT);
        bcatcstr(glsl, "))");
        TranslateOperandSwizzle(psContext, &psInst->asOperands[dest]);
    }
    EndAssignment(psContext, &psInst->asOperands[dest], TO_FLAG_FLOAT, psInst->bSaturate);
    bcatcstr(glsl, ";\n");
}

//Makes sure the texture coordinate swizzle is appropriate for the texture type.
//i.e. vecX for X-dimension texture.
//Currently supports floating point coord only, so not used for texelFetch.
static void TranslateTexCoord(HLSLCrossCompilerContext* psContext,
    const RESOURCE_DIMENSION eResDim,
    Operand* psTexCoordOperand)
{
    unsigned int uNumCoords = psTexCoordOperand->iNumComponents;
    int constructor = 0;
    bstring glsl = *psContext->currentGLSLString;

    switch (eResDim)
    {
    case RESOURCE_DIMENSION_TEXTURE1D:
    {
        //Vec1 texcoord. Mask out the other components.
        psTexCoordOperand->aui32Swizzle[1] = 0xFFFFFFFF;
        psTexCoordOperand->aui32Swizzle[2] = 0xFFFFFFFF;
        psTexCoordOperand->aui32Swizzle[3] = 0xFFFFFFFF;
        if (psTexCoordOperand->eType == OPERAND_TYPE_IMMEDIATE32 ||
            psTexCoordOperand->eType == OPERAND_TYPE_IMMEDIATE64)
        {
            psTexCoordOperand->iNumComponents = 1;
        }
        break;
    }
    case RESOURCE_DIMENSION_TEXTURE2D:
    case RESOURCE_DIMENSION_TEXTURE1DARRAY:
    {
        //Vec2 texcoord. Mask out the other components.
        psTexCoordOperand->aui32Swizzle[2] = 0xFFFFFFFF;
        psTexCoordOperand->aui32Swizzle[3] = 0xFFFFFFFF;
        if (psTexCoordOperand->eType == OPERAND_TYPE_IMMEDIATE32 ||
            psTexCoordOperand->eType == OPERAND_TYPE_IMMEDIATE64)
        {
            psTexCoordOperand->iNumComponents = 2;
        }
        if (psTexCoordOperand->eSelMode == OPERAND_4_COMPONENT_SELECT_1_MODE)
        {
            constructor = 1;
            bcatcstr(glsl, "vec2(");
        }
        break;
    }
    case RESOURCE_DIMENSION_TEXTURECUBE:
    case RESOURCE_DIMENSION_TEXTURE3D:
    case RESOURCE_DIMENSION_TEXTURE2DARRAY:
    {
        //Vec3 texcoord. Mask out the other component.
        psTexCoordOperand->aui32Swizzle[3] = 0xFFFFFFFF;
        if (psTexCoordOperand->eType == OPERAND_TYPE_IMMEDIATE32 ||
            psTexCoordOperand->eType == OPERAND_TYPE_IMMEDIATE64)
        {
            psTexCoordOperand->iNumComponents = 3;
        }
        if (psTexCoordOperand->eSelMode == OPERAND_4_COMPONENT_SELECT_1_MODE)
        {
            constructor = 1;
            bcatcstr(glsl, "vec3(");
        }
        break;
    }
    case RESOURCE_DIMENSION_TEXTURECUBEARRAY:
    {
        uNumCoords = 4;
        if (psTexCoordOperand->eSelMode == OPERAND_4_COMPONENT_SELECT_1_MODE)
        {
            constructor = 1;
            bcatcstr(glsl, "vec4(");
        }
        break;
    }
    default:
    {
        ASSERT(0);
        break;
    }
    }

    //Mask out the other components.
    switch (psTexCoordOperand->eSelMode)
    {
    case OPERAND_4_COMPONENT_SELECT_1_MODE:
        ASSERT(uNumCoords == 1);
        break;
    case OPERAND_4_COMPONENT_SWIZZLE_MODE:
        while (uNumCoords < 4)
        {
            psTexCoordOperand->aui32Swizzle[uNumCoords] = 0xFFFFFFFF;
            ++uNumCoords;
        }
        break;
    case OPERAND_4_COMPONENT_MASK_MODE:
        if (psTexCoordOperand->ui32CompMask < 4)
        {
            psTexCoordOperand->ui32CompMask =
                (uNumCoords > 0) * OPERAND_4_COMPONENT_MASK_X |
                (uNumCoords > 1) * OPERAND_4_COMPONENT_MASK_Y |
                (uNumCoords > 2) * OPERAND_4_COMPONENT_MASK_Z;
        }
        break;
    }
    TranslateOperand(psContext, psTexCoordOperand, TO_FLAG_FLOAT);

    if (constructor)
    {
        bcatcstr(glsl, ")");
    }
}

static int GetNumTextureDimensions(HLSLCrossCompilerContext* psContext,
    const RESOURCE_DIMENSION eResDim)
{
    (void)(psContext);

    switch (eResDim)
    {
    case RESOURCE_DIMENSION_TEXTURE1D:
    {
        return 1;
    }
    case RESOURCE_DIMENSION_TEXTURE2D:
    case RESOURCE_DIMENSION_TEXTURE1DARRAY:
    case RESOURCE_DIMENSION_TEXTURECUBE:
    {
        return 2;
    }

    case RESOURCE_DIMENSION_TEXTURE3D:
    case RESOURCE_DIMENSION_TEXTURE2DARRAY:
    case RESOURCE_DIMENSION_TEXTURECUBEARRAY:
    {
        return 3;
    }
    default:
    {
        ASSERT(0);
        break;
    }
    }
    return 0;
}

void GetResInfoData(HLSLCrossCompilerContext* psContext, Instruction* psInst, int index)
{
    bstring glsl = *psContext->currentGLSLString;
    const RESINFO_RETURN_TYPE eResInfoReturnType = psInst->eResInfoReturnType;
    const RESOURCE_DIMENSION eResDim = psContext->psShader->aeResourceDims[psInst->asOperands[2].ui32RegisterNumber];

    //[width, height, depth or array size, total-mip-count]
    if (index < 3)
    {
        int dim = GetNumTextureDimensions(psContext, eResDim);

        if (dim < (index + 1))
        {
            bcatcstr(glsl, "0");
        }
        else
        {
            if (eResInfoReturnType == RESINFO_INSTRUCTION_RETURN_UINT)
            {
                bformata(glsl, "ivec%d(textureSize(", dim);
            }
            else if (eResInfoReturnType == RESINFO_INSTRUCTION_RETURN_RCPFLOAT)
            {
                bformata(glsl, "vec%d(1.0f) / vec%d(textureSize(", dim, dim);
            }
            else
            {
                bformata(glsl, "vec%d(textureSize(", dim);
            }
            TranslateOperand(psContext, &psInst->asOperands[2], TO_FLAG_NONE);
            bcatcstr(glsl, ", ");
            TranslateOperand(psContext, &psInst->asOperands[1], TO_FLAG_INTEGER);
            bcatcstr(glsl, "))");

            switch (index)
            {
            case 0:
                bcatcstr(glsl, ".x");
                break;
            case 1:
                bcatcstr(glsl, ".y");
                break;
            case 2:
                bcatcstr(glsl, ".z");
                break;
            }
        }
    }
    else
    {
        bcatcstr(glsl, "textureQueryLevels(");
        TranslateOperand(psContext, &psInst->asOperands[2], TO_FLAG_NONE);
        bcatcstr(glsl, ")");
    }
}

uint32_t GetReturnTypeToFlags(RESOURCE_RETURN_TYPE eReturnType)
{
    switch (eReturnType)
    {
    case RETURN_TYPE_FLOAT:
        return TO_FLAG_FLOAT;
    case RETURN_TYPE_UINT:
        return TO_FLAG_UNSIGNED_INTEGER;
    case RETURN_TYPE_SINT:
        return TO_FLAG_INTEGER;
    case RETURN_TYPE_DOUBLE:
        return TO_FLAG_DOUBLE;
    }
    ASSERT(0);
    return TO_FLAG_NONE;
}

uint32_t GetResourceReturnTypeToFlags(ResourceGroup eGroup, uint32_t ui32BindPoint, HLSLCrossCompilerContext* psContext)
{
    ResourceBinding* psBinding;
    if (GetResourceFromBindingPoint(eGroup, ui32BindPoint, &psContext->psShader->sInfo, &psBinding))
    {
        return GetReturnTypeToFlags(psBinding->ui32ReturnType);
    }
    ASSERT(0);
    return TO_FLAG_NONE;
}

#define TEXSMP_FLAG_NONE 0x0
#define TEXSMP_FLAG_LOD 0x1 //LOD comes from operand
#define TEXSMP_FLAG_COMPARE 0x2
#define TEXSMP_FLAG_FIRSTLOD 0x4 //LOD is 0
#define TEXSMP_FLAG_BIAS 0x8
#define TEXSMP_FLAGS_GRAD 0x10
static void TranslateTextureSample(HLSLCrossCompilerContext* psContext, Instruction* psInst, uint32_t ui32Flags)
{
    bstring glsl = *psContext->currentGLSLString;

    const char* funcName = "texture";
    const char* offset = "";
    const char* depthCmpCoordType = "";
    const char* gradSwizzle = "";
    uint32_t sampleTypeToFlags = TO_FLAG_FLOAT;

    uint32_t ui32NumOffsets = 0;

    const RESOURCE_DIMENSION eResDim = psContext->psShader->aeResourceDims[psInst->asOperands[2].ui32RegisterNumber];

    const int iHaveOverloadedTexFuncs = HaveOverloadedTextureFuncs(psContext->psShader->eTargetLanguage);

    ASSERT(psInst->asOperands[2].ui32RegisterNumber < MAX_TEXTURES);

    if (psInst->bAddressOffset)
    {
        offset = "Offset";
    }

    switch (eResDim)
    {
    case RESOURCE_DIMENSION_TEXTURE1D:
    {
        depthCmpCoordType = "vec2";
        gradSwizzle = ".x";
        ui32NumOffsets = 1;
        if (!iHaveOverloadedTexFuncs)
        {
            funcName = "texture1D";
            if (ui32Flags & TEXSMP_FLAG_COMPARE)
            {
                funcName = "shadow1D";
            }
        }
        break;
    }
    case RESOURCE_DIMENSION_TEXTURE2D:
    {
        depthCmpCoordType = "vec3";
        gradSwizzle = ".xy";
        ui32NumOffsets = 2;
        if (!iHaveOverloadedTexFuncs)
        {
            funcName = "texture2D";
            if (ui32Flags & TEXSMP_FLAG_COMPARE)
            {
                funcName = "shadow2D";
            }
        }
        break;
    }
    case RESOURCE_DIMENSION_TEXTURECUBE:
    {
        depthCmpCoordType = "vec3";
        gradSwizzle = ".xyz";
        ui32NumOffsets = 3;
        if (!iHaveOverloadedTexFuncs)
        {
            funcName = "textureCube";
        }
        break;
    }
    case RESOURCE_DIMENSION_TEXTURE3D:
    {
        depthCmpCoordType = "vec4";
        gradSwizzle = ".xyz";
        ui32NumOffsets = 3;
        if (!iHaveOverloadedTexFuncs)
        {
            funcName = "texture3D";
        }
        break;
    }
    case RESOURCE_DIMENSION_TEXTURE1DARRAY:
    {
        depthCmpCoordType = "vec3";
        gradSwizzle = ".x";
        ui32NumOffsets = 1;
        break;
    }
    case RESOURCE_DIMENSION_TEXTURE2DARRAY:
    {
        depthCmpCoordType = "vec4";
        gradSwizzle = ".xy";
        ui32NumOffsets = 2;
        break;
    }
    case RESOURCE_DIMENSION_TEXTURECUBEARRAY:
    {
        gradSwizzle = ".xyz";
        ui32NumOffsets = 3;
        if (ui32Flags & TEXSMP_FLAG_COMPARE)
        {
            //Special. Reference is a separate argument.
            AddIndentation(psContext);
            sampleTypeToFlags = TO_FLAG_FLOAT;
            BeginAssignment(psContext, &psInst->asOperands[0], sampleTypeToFlags, psInst->bSaturate);
            if (ui32Flags & (TEXSMP_FLAG_LOD | TEXSMP_FLAG_FIRSTLOD))
            {
                bcatcstr(glsl, "(vec4(textureLod(");
            }
            else
            {
                bcatcstr(glsl, "(vec4(texture(");
            }
            TextureName(*psContext->currentGLSLString, psContext->psShader, psInst->asOperands[2].ui32RegisterNumber, psInst->asOperands[3].ui32RegisterNumber, 1);
            bcatcstr(glsl, ",");
            TranslateTexCoord(psContext, eResDim, &psInst->asOperands[1]);
            bcatcstr(glsl, ",");
            //.z = reference.
            TranslateOperand(psContext, &psInst->asOperands[4], TO_FLAG_FLOAT);

            if (ui32Flags & TEXSMP_FLAG_FIRSTLOD)
            {
                bcatcstr(glsl, ", 0.0");
            }

            bcatcstr(glsl, "))");
            // iWriteMaskEnabled is forced off during DecodeOperand because swizzle on sampler uniforms
            // does not make sense. But need to re-enable to correctly swizzle this particular instruction.
            psInst->asOperands[2].iWriteMaskEnabled = 1;
            TranslateOperandSwizzle(psContext, &psInst->asOperands[2]);
            bcatcstr(glsl, ")");

            TranslateOperandSwizzle(psContext, &psInst->asOperands[0]);
            EndAssignment(psContext, &psInst->asOperands[0], sampleTypeToFlags, psInst->bSaturate);
            bcatcstr(glsl, ";\n");
            return;
        }

        break;
    }
    default:
    {
        ASSERT(0);
        break;
    }
    }

    if (ui32Flags & TEXSMP_FLAG_COMPARE)
    {
        //For non-cubeMap Arrays the reference value comes from the
        //texture coord vector in GLSL. For cubmap arrays there is a
        //separate parameter.
        //It is always separate paramter in HLSL.
        AddIndentation(psContext);
        sampleTypeToFlags = TO_FLAG_FLOAT;
        BeginAssignment(psContext, &psInst->asOperands[0], sampleTypeToFlags, psInst->bSaturate);

        if (ui32Flags & (TEXSMP_FLAG_LOD | TEXSMP_FLAG_FIRSTLOD))
        {
            bformata(glsl, "(vec4(%sLod%s(", funcName, offset);
        }
        else
        {
            bformata(glsl, "(vec4(%s%s(", funcName, offset);
        }
        TextureName(*psContext->currentGLSLString, psContext->psShader, psInst->asOperands[2].ui32RegisterNumber, psInst->asOperands[3].ui32RegisterNumber, 1);
        bformata(glsl, ", %s(", depthCmpCoordType);
        TranslateTexCoord(psContext, eResDim, &psInst->asOperands[1]);
        bcatcstr(glsl, ",");
        //.z = reference.
        TranslateOperand(psContext, &psInst->asOperands[4], TO_FLAG_FLOAT);
        bcatcstr(glsl, ")");

        if (ui32Flags & TEXSMP_FLAG_FIRSTLOD)
        {
            bcatcstr(glsl, ", 0.0");
        }

        bcatcstr(glsl, "))");
    }
    else
    {
        AddIndentation(psContext);
        sampleTypeToFlags = GetResourceReturnTypeToFlags(RGROUP_TEXTURE, psInst->asOperands[2].ui32RegisterNumber, psContext);
        BeginAssignment(psContext, &psInst->asOperands[0], sampleTypeToFlags, psInst->bSaturate);
        if (ui32Flags & (TEXSMP_FLAG_LOD | TEXSMP_FLAG_FIRSTLOD))
        {
            bformata(glsl, "(%sLod%s(", funcName, offset);
        }
        else
        if (ui32Flags & TEXSMP_FLAGS_GRAD)
        {
            bformata(glsl, "(%sGrad%s(", funcName, offset);
        }
        else
        {
            bformata(glsl, "(%s%s(", funcName, offset);
        }
        TextureName(*psContext->currentGLSLString, psContext->psShader, psInst->asOperands[2].ui32RegisterNumber, psInst->asOperands[3].ui32RegisterNumber, 0);
        bcatcstr(glsl, ", ");
        TranslateTexCoord(psContext, eResDim, &psInst->asOperands[1]);

        if (ui32Flags & (TEXSMP_FLAG_LOD))
        {
            bcatcstr(glsl, ", ");
            TranslateOperand(psContext, &psInst->asOperands[4], TO_FLAG_FLOAT);
            if (psContext->psShader->ui32MajorVersion < 4)
            {
                bcatcstr(glsl, ".w");
            }
        }
        else
        if (ui32Flags & TEXSMP_FLAG_FIRSTLOD)
        {
            bcatcstr(glsl, ", 0.0");
        }
        else
        if (ui32Flags & TEXSMP_FLAGS_GRAD)
        {
            bcatcstr(glsl, ", vec4(");
            TranslateOperand(psContext, &psInst->asOperands[4], TO_FLAG_FLOAT);//dx
            bcatcstr(glsl, ")");
            bcatcstr(glsl, gradSwizzle);
            bcatcstr(glsl, ", vec4(");
            TranslateOperand(psContext, &psInst->asOperands[5], TO_FLAG_FLOAT);//dy
            bcatcstr(glsl, ")");
            bcatcstr(glsl, gradSwizzle);
        }

        if (psInst->bAddressOffset)
        {
            if (ui32NumOffsets == 1)
            {
                bformata(glsl, ", %d",
                    psInst->iUAddrOffset);
            }
            else
            if (ui32NumOffsets == 2)
            {
                bformata(glsl, ", ivec2(%d, %d)",
                    psInst->iUAddrOffset,
                    psInst->iVAddrOffset);
            }
            else
            if (ui32NumOffsets == 3)
            {
                bformata(glsl, ", ivec3(%d, %d, %d)",
                    psInst->iUAddrOffset,
                    psInst->iVAddrOffset,
                    psInst->iWAddrOffset);
            }
        }

        if (ui32Flags & (TEXSMP_FLAG_BIAS))
        {
            bcatcstr(glsl, ", ");
            TranslateOperand(psContext, &psInst->asOperands[4], TO_FLAG_FLOAT);
        }

        bcatcstr(glsl, ")");
    }

    // iWriteMaskEnabled is forced off during DecodeOperand because swizzle on sampler uniforms
    // does not make sense. But need to re-enable to correctly swizzle this particular instruction.
    psInst->asOperands[2].iWriteMaskEnabled = 1;
    TranslateOperandSwizzle(psContext, &psInst->asOperands[2]);
    bcatcstr(glsl, ")");

    TranslateOperandSwizzle(psContext, &psInst->asOperands[0]);
    EndAssignment(psContext, &psInst->asOperands[0], sampleTypeToFlags, psInst->bSaturate);
    bcatcstr(glsl, ";\n");
}

static ShaderVarType* LookupStructuredVarExtended(HLSLCrossCompilerContext* psContext,
    Operand* psResource,
    Operand* psByteOffset,
    uint32_t ui32Component,
    uint32_t* swizzle)
{
    ConstantBuffer* psCBuf = NULL;
    ShaderVarType* psVarType = NULL;
    uint32_t aui32Swizzle[4] = {OPERAND_4_COMPONENT_X};
    int byteOffset = psByteOffset ? ((int*)psByteOffset->afImmediates)[0] + 4 * ui32Component : 0;
    int vec4Offset = byteOffset >> 4;
    int32_t index = -1;
    int32_t rebase = -1;
    int found;
    //TODO: multi-component stores and vector writes need testing.

    //aui32Swizzle[0] = psInst->asOperands[0].aui32Swizzle[component];

    switch (byteOffset % 16)
    {
    case 0:
        aui32Swizzle[0] = 0;
        break;
    case 4:
        aui32Swizzle[0] = 1;
        break;
    case 8:
        aui32Swizzle[0] = 2;
        break;
    case 12:
        aui32Swizzle[0] = 3;
        break;
    }

    switch (psResource->eType)
    {
    case OPERAND_TYPE_RESOURCE:
        GetConstantBufferFromBindingPoint(RGROUP_TEXTURE, psResource->ui32RegisterNumber, &psContext->psShader->sInfo, &psCBuf);
        break;
    case OPERAND_TYPE_UNORDERED_ACCESS_VIEW:
        GetConstantBufferFromBindingPoint(RGROUP_UAV, psResource->ui32RegisterNumber, &psContext->psShader->sInfo, &psCBuf);
        break;
    case OPERAND_TYPE_THREAD_GROUP_SHARED_MEMORY:
    {
        //dcl_tgsm_structured defines the amount of memory and a stride.
        ASSERT(psResource->ui32RegisterNumber < MAX_GROUPSHARED);
        ASSERT(swizzle == NULL);
        return &psContext->psShader->sGroupSharedVarType[psResource->ui32RegisterNumber];
    }
    default:
        ASSERT(0);
        break;
    }

    found = GetShaderVarFromOffset(vec4Offset, aui32Swizzle, psCBuf, &psVarType, &index, &rebase);
    ASSERT(found);

    if (swizzle)
    {
        // Assuming the components are 4 bytes in length
        const int bytesPerComponent = 4; 
        // Calculate the variable swizzling based on the byteOffset and the position of the variable in the structure
        ASSERT((byteOffset - psVarType->Offset) % 4 == 0);
        *swizzle = (byteOffset - psVarType->Offset) / bytesPerComponent;
        ASSERT(*swizzle < 4);
    }

    return psVarType;
}

static ShaderVarType* LookupStructuredVar(HLSLCrossCompilerContext* psContext,
    Operand* psResource,
    Operand* psByteOffset,
    uint32_t ui32Component)
{
    return LookupStructuredVarExtended(psContext, psResource, psByteOffset, ui32Component, NULL);
}

static void TranslateShaderStorageVarName(bstring output, Shader* psShader, const Operand* operand, int structured)
{
    bstring varName = bfromcstr("");
    if (operand->eType == OPERAND_TYPE_RESOURCE)
    {
        if (structured)
        {
            bformata(varName, "StructuredRes%d", operand->ui32RegisterNumber);
        }
        else
        {
            bformata(varName, "RawRes%d", operand->ui32RegisterNumber);
        }
    }
    else if(operand->eType == OPERAND_TYPE_UNORDERED_ACCESS_VIEW)
    {
        bformata(varName, "UAV%d", operand->ui32RegisterNumber);
    }
    else
    {
        ASSERT(0);
    }
    ShaderVarName(output, psShader, bstr2cstr(varName, '\0'));
    bdestroy(varName);
}

static void TranslateShaderStorageStore(HLSLCrossCompilerContext* psContext, Instruction* psInst)
{
    bstring glsl = *psContext->currentGLSLString;
    ShaderVarType* psVarType = NULL;
    int component;
    int srcComponent = 0;

    Operand* psDest = 0;
    Operand* psDestAddr = 0;
    Operand* psDestByteOff = 0;
    Operand* psSrc = 0;
    int structured = 0;

    switch (psInst->eOpcode)
    {
    case OPCODE_STORE_STRUCTURED:
        psDest = &psInst->asOperands[0];
        psDestAddr = &psInst->asOperands[1];
        psDestByteOff = &psInst->asOperands[2];
        psSrc = &psInst->asOperands[3];
        structured = 1;
        break;
    case OPCODE_STORE_RAW:
        psDest = &psInst->asOperands[0];
        psDestByteOff = &psInst->asOperands[1];
        psSrc = &psInst->asOperands[2];
        break;
    }

    for (component = 0; component < 4; component++)
    {
        const char* swizzleString[] = { ".x", ".y", ".z", ".w" };
        ASSERT(psInst->asOperands[0].eSelMode == OPERAND_4_COMPONENT_MASK_MODE);
        if (psInst->asOperands[0].ui32CompMask & (1 << component))
        {
            uint32_t swizzle = 0;
            if (structured && psDest->eType != OPERAND_TYPE_THREAD_GROUP_SHARED_MEMORY)
            {
                psVarType = LookupStructuredVarExtended(psContext, psDest, psDestByteOff, component, &swizzle);
            }

            AddIndentation(psContext);
            TranslateShaderStorageVarName(glsl, psContext->psShader, psDest, structured);
            bformata(glsl, "[");
            if (structured) //Dest address and dest byte offset
            {
                if (psDest->eType == OPERAND_TYPE_THREAD_GROUP_SHARED_MEMORY)
                {
                    TranslateOperand(psContext, psDestAddr, TO_FLAG_INTEGER | TO_FLAG_UNSIGNED_INTEGER);
                    bformata(glsl, "].value[");
                    TranslateOperand(psContext, psDestByteOff, TO_FLAG_INTEGER | TO_FLAG_UNSIGNED_INTEGER);
                    bformata(glsl, " >> 2u ");//bytes to floats
                }
                else
                {
                    TranslateOperand(psContext, psDestAddr, TO_FLAG_INTEGER | TO_FLAG_UNSIGNED_INTEGER);
                }
            }
            else
            {
                TranslateOperand(psContext, psDestByteOff, TO_FLAG_INTEGER | TO_FLAG_UNSIGNED_INTEGER);
            }

            //RAW: change component using index offset
            if (!structured || (psDest->eType == OPERAND_TYPE_THREAD_GROUP_SHARED_MEMORY))
            {
                bformata(glsl, " + %d", component);
            }

            bformata(glsl, "]");

            if (structured && psDest->eType != OPERAND_TYPE_THREAD_GROUP_SHARED_MEMORY)
            {
                if (strcmp(psVarType->Name, "$Element") != 0)
                {
                    bcatcstr(glsl, ".");
                    ShaderVarName(glsl, psContext->psShader, psVarType->Name);
                }

                if (psVarType->Columns > 1)
                {
                    bformata(glsl, swizzleString[swizzle]);
                }
            }


            if (structured)
            {
                uint32_t flags = TO_FLAG_UNSIGNED_INTEGER;
                if (psVarType)
                {
                    if (psVarType->Type == SVT_INT)
                    {
                        flags = TO_FLAG_INTEGER;
                    }
                    else if (psVarType->Type == SVT_FLOAT)
                    {
                        flags = TO_FLAG_NONE;
                    }
                }
                //TGSM always uint
                bformata(glsl, " = (");
                TranslateOperand(psContext, psSrc, flags);
            }
            else
            {
                //Dest type is currently always a uint array.
                bformata(glsl, " = (");
                TranslateOperand(psContext, psSrc, TO_FLAG_UNSIGNED_INTEGER);
            }

            if (GetNumSwizzleElements(psSrc) > 1)
            {
                bformata(glsl, swizzleString[srcComponent++]);
            }

            //Double takes an extra slot.
            if (psVarType && psVarType->Type == SVT_DOUBLE)
            {
                if (structured && psDest->eType == OPERAND_TYPE_THREAD_GROUP_SHARED_MEMORY)
                {
                    bcatcstr(glsl, ")");
                }
                component++;
            }

            bformata(glsl, ");\n");
        }
    }
}

static void TranslateShaderPLSStore(HLSLCrossCompilerContext* psContext, Instruction* psInst)
{
    bstring glsl = *psContext->currentGLSLString;
    ShaderVarType* psVarType = NULL;
    int component;
    int srcComponent = 0;

    Operand* psDest = 0;
    Operand* psDestAddr = 0;
    Operand* psDestByteOff = 0;
    Operand* psSrc = 0;
    int structured = 0;

    switch (psInst->eOpcode)
    {
    case OPCODE_STORE_STRUCTURED:
        psDest = &psInst->asOperands[0];
        psDestAddr = &psInst->asOperands[1];
        psDestByteOff = &psInst->asOperands[2];
        psSrc = &psInst->asOperands[3];
        structured = 1;
        break;
    case OPCODE_STORE_RAW:
    default:
        ASSERT(0);
    }

    ASSERT(structured);

    for (component = 0; component < 4; component++)
    {
        const char* swizzleString[] = { ".x", ".y", ".z", ".w" };
        ASSERT(psInst->asOperands[0].eSelMode == OPERAND_4_COMPONENT_MASK_MODE);
        if (psInst->asOperands[0].ui32CompMask & (1 << component))
        {

            ASSERT(psDest->eType != OPERAND_TYPE_THREAD_GROUP_SHARED_MEMORY);

            psVarType = LookupStructuredVar(psContext, psDest, psDestByteOff, component);

            AddIndentation(psContext);

            if (structured && psDest->eType == OPERAND_TYPE_RESOURCE)
            {
                bstring varName = bfromcstralloc(16, "");
                bformata(varName, "StructuredRes%d", psDest->ui32RegisterNumber);
                ShaderVarName(glsl, psContext->psShader, bstr2cstr(varName, '\0'));
                bdestroy(varName);
            }
            else
            {
                TranslateOperand(psContext, psDest, TO_FLAG_DESTINATION | TO_FLAG_NAME_ONLY);
            }

            ASSERT(strcmp(psVarType->Name, "$Element") != 0);

            bcatcstr(glsl, ".");
            ShaderVarName(glsl, psContext->psShader, psVarType->Name);

            if (psVarType->Class == SVC_VECTOR)
            {
                int byteOffset = ((int*)psDestByteOff->afImmediates)[0] + 4 * (psDest->eSelMode == OPERAND_4_COMPONENT_SWIZZLE_MODE ? psDest->aui32Swizzle[component] : component);
                int byteOffsetOfVar = psVarType->Offset;
                unsigned int startComponent = (byteOffset - byteOffsetOfVar) >> 2;
                unsigned int s = startComponent;

                bformata(glsl, "%s", swizzleString[s]);
            }

            uint32_t flags = TO_FLAG_UNSIGNED_INTEGER;
            if (psVarType)
            {
                if (psVarType->Type == SVT_INT)
                {
                    flags = TO_FLAG_INTEGER;
                }
                else if (psVarType->Type == SVT_FLOAT)
                {
                    flags = TO_FLAG_NONE;
                }
                else
                {
                    ASSERT(0);
                }
            }
            //TGSM always uint
            bformata(glsl, " = (");
            TranslateOperand(psContext, psSrc, flags);



            if (GetNumSwizzleElements(psSrc) > 1)
            {
                bformata(glsl, swizzleString[srcComponent++]);
            }

            //Double takes an extra slot.
            if (psVarType && psVarType->Type == SVT_DOUBLE)
            {
                if (structured && psDest->eType == OPERAND_TYPE_THREAD_GROUP_SHARED_MEMORY)
                {
                    bcatcstr(glsl, ")");
                }
                component++;
            }

            bformata(glsl, ");\n");
        }
    }
}

static void TranslateShaderStorageLoad(HLSLCrossCompilerContext* psContext, Instruction* psInst)
{
    bstring glsl = *psContext->currentGLSLString;
    ShaderVarType* psVarType = NULL;
    uint32_t aui32Swizzle[4] = {OPERAND_4_COMPONENT_X};
    uint32_t ui32DataTypeFlag = TO_FLAG_INTEGER;
    int component;
    int destComponent = 0;

    Operand* psDest = 0;
    Operand* psSrcAddr = 0;
    Operand* psSrcByteOff = 0;
    Operand* psSrc = 0;
    int structured = 0;

    switch (psInst->eOpcode)
    {
    case OPCODE_LD_STRUCTURED:
        psDest = &psInst->asOperands[0];
        psSrcAddr = &psInst->asOperands[1];
        psSrcByteOff = &psInst->asOperands[2];
        psSrc = &psInst->asOperands[3];
        structured = 1;
        break;
    case OPCODE_LD_RAW:
        psDest = &psInst->asOperands[0];
        psSrcByteOff = &psInst->asOperands[1];
        psSrc = &psInst->asOperands[2];
        break;
    }

    if (psInst->eOpcode == OPCODE_LD_RAW)
    {
        unsigned int ui32CompNum = GetNumSwizzleElements(psDest);

        for (component = 0; component < 4; component++)
        {
            const char* swizzleString [] = { "x", "y", "z", "w" };
            ASSERT(psDest->eSelMode == OPERAND_4_COMPONENT_MASK_MODE);
            if (psDest->ui32CompMask & (1 << component))
            {
                int addedBitcast = 0;

                if (structured)
                {
                    psVarType = LookupStructuredVar(psContext, psSrc, psSrcByteOff, psSrc->aui32Swizzle[component]);
                }

                AddIndentation(psContext);

                aui32Swizzle[0] = psSrc->aui32Swizzle[component];

                if (ui32CompNum > 1)
                {
                    BeginAssignmentEx(psContext, psDest, TO_FLAG_FLOAT, psInst->bSaturate, swizzleString[destComponent++]);
                }
                else
                {
                    BeginAssignment(psContext, psDest, TO_FLAG_FLOAT, psInst->bSaturate);
                }

                if (psSrc->eType == OPERAND_TYPE_THREAD_GROUP_SHARED_MEMORY)
                {
                    // unknown how to make this without TO_FLAG_NAME_ONLY
                    bcatcstr(glsl, "uintBitsToFloat(");
                    addedBitcast = 1;

                    TranslateOperand(psContext, psSrc, ui32DataTypeFlag & TO_FLAG_NAME_ONLY);

                    if (((int*)psSrcByteOff->afImmediates)[0] == 0)
                    {
                        bformata(glsl, "[0");
                    }
                    else
                    {
                        bformata(glsl, "[((");
                        TranslateOperand(psContext, psSrcByteOff, TO_FLAG_INTEGER);
                        bcatcstr(glsl, ") >> 2u)");
                    }
                }
                else
                {
                    bstring varName = bfromcstralloc(16, "");
                    bformata(varName, "RawRes%d", psSrc->ui32RegisterNumber);

                    ShaderVarName(glsl, psContext->psShader, bstr2cstr(varName, '\0'));
                    bcatcstr(glsl, "[((");
                    TranslateOperand(psContext, psSrcByteOff, TO_FLAG_INTEGER);
                    bcatcstr(glsl, ") >> 2u)");

                    bdestroy(varName);
                }

                if (psSrc->eSelMode == OPERAND_4_COMPONENT_SWIZZLE_MODE && psSrc->aui32Swizzle[component] != 0)
                {
                    bformata(glsl, " + %d", psSrc->aui32Swizzle[component]);
                }
                bcatcstr(glsl, "]");

                if (addedBitcast)
                {
                    bcatcstr(glsl, ")");
                }

                EndAssignment(psContext, psDest, TO_FLAG_FLOAT, psInst->bSaturate);
                bformata(glsl, ";\n");
            }
        }
    }
    else
    {
        unsigned int ui32CompNum = GetNumSwizzleElements(psDest);

        //(int)GetNumSwizzleElements(&psInst->asOperands[0])
        for (component = 0; component < 4; component++)
        {
            const char* swizzleString [] = { "x", "y", "z", "w" };
            ASSERT(psDest->eSelMode == OPERAND_4_COMPONENT_MASK_MODE);
            if (psDest->ui32CompMask & (1 << component))
            {
                int addedBitcast = 0;

                psVarType = LookupStructuredVar(psContext, psSrc, psSrcByteOff, psSrc->aui32Swizzle[component]);

                AddIndentation(psContext);

                aui32Swizzle[0] = psSrc->aui32Swizzle[component];

                if (ui32CompNum > 1)
                {
                    BeginAssignmentEx(psContext, psDest, TO_FLAG_FLOAT, psInst->bSaturate, swizzleString[destComponent++]);
                }
                else
                {
                    BeginAssignment(psContext, psDest, TO_FLAG_FLOAT, psInst->bSaturate);
                }

                if (psSrc->eType == OPERAND_TYPE_THREAD_GROUP_SHARED_MEMORY)
                {
                    // unknown how to make this without TO_FLAG_NAME_ONLY
                    if (psVarType->Type == SVT_UINT)
                    {
                        bcatcstr(glsl, "uintBitsToFloat(");
                        addedBitcast = 1;
                    }
                    else if (psVarType->Type == SVT_INT)
                    {
                        bcatcstr(glsl, "intBitsToFloat(");
                        addedBitcast = 1;
                    }
                    else if (psVarType->Type == SVT_DOUBLE)
                    {
                        bcatcstr(glsl, "unpackDouble2x32(");
                        addedBitcast = 1;
                    }

                    // input already in uints
                    TranslateOperand(psContext, psSrc, TO_FLAG_NAME_ONLY);
                    bcatcstr(glsl, "[");
                    TranslateOperand(psContext, psSrcAddr, TO_FLAG_INTEGER);
                    bcatcstr(glsl, "].value[(");
                    TranslateOperand(psContext, psSrcByteOff, TO_FLAG_UNSIGNED_INTEGER);
                    bformata(glsl, " >> 2u)]");
                }
                else
                {
                    ConstantBuffer* psCBuf = NULL;
                    uint32_t swizzle = 0;
                    psVarType = LookupStructuredVarExtended(psContext, psSrc, psSrcByteOff, psSrc->eSelMode == OPERAND_4_COMPONENT_SWIZZLE_MODE ? psSrc->aui32Swizzle[component] : component, &swizzle);
                    GetConstantBufferFromBindingPoint(RGROUP_UAV, psSrc->ui32RegisterNumber, &psContext->psShader->sInfo, &psCBuf);

                    if (psVarType->Type == SVT_UINT)
                    {
                        bcatcstr(glsl, "uintBitsToFloat(");
                        addedBitcast = 1;
                    }
                    else if (psVarType->Type == SVT_INT)
                    {
                        bcatcstr(glsl, "intBitsToFloat(");
                        addedBitcast = 1;
                    }
                    else if (psVarType->Type == SVT_DOUBLE)
                    {
                        bcatcstr(glsl, "unpackDouble2x32(");
                        addedBitcast = 1;
                    }

                    if (psSrc->eType == OPERAND_TYPE_UNORDERED_ACCESS_VIEW)
                    {
                        TranslateShaderStorageVarName(glsl, psContext->psShader, psSrc, 1);
                        bformata(glsl, "[");
                        TranslateOperand(psContext, psSrcAddr, TO_FLAG_INTEGER);
                        bcatcstr(glsl, "]");
                        if (strcmp(psVarType->Name, "$Element") != 0)
                        {
                            bcatcstr(glsl, ".");
                            ShaderVarName(glsl, psContext->psShader, psVarType->Name);
                        }

                        if (psVarType->Columns > 1)
                        {
                            bformata(glsl, ".%s", swizzleString[swizzle]);
                        }
                    }
                    else if (psSrc->eType == OPERAND_TYPE_RESOURCE)
                    {
                        TranslateShaderStorageVarName(glsl, psContext->psShader, psSrc, 1);
                        bcatcstr(glsl, "[");
                        TranslateOperand(psContext, psSrcAddr, TO_FLAG_INTEGER);
                        bcatcstr(glsl, "]");

                        if (strcmp(psVarType->Name, "$Element") != 0)
                        {
                            bcatcstr(glsl, ".");
                            ShaderVarName(glsl, psContext->psShader, psVarType->Name);
                        }

                        if (psVarType->Class == SVC_SCALAR)
                        {
                        }
                        else if (psVarType->Class == SVC_VECTOR)
                        {
                            int byteOffset = ((int*)psSrcByteOff->afImmediates)[0] + 4 * (psSrc->eSelMode == OPERAND_4_COMPONENT_SWIZZLE_MODE ? psSrc->aui32Swizzle[component] : component);
                            int byteOffsetOfVar = psVarType->Offset;
                            unsigned int startComponent = (byteOffset - byteOffsetOfVar) >> 2;
                            unsigned int s = startComponent;

                            bcatcstr(glsl, ".");
#if 0
                            for (s = startComponent; s < min(min(psVarType->Columns, 4U - component), ui32CompNum); ++s)
#endif
                            bformata(glsl, "%s", swizzleString[s]);
                        }
                        else if (psVarType->Class == SVC_MATRIX_ROWS)
                        {
                            int byteOffset = ((int*)psSrcByteOff->afImmediates)[0] + 4 * (psSrc->eSelMode == OPERAND_4_COMPONENT_SWIZZLE_MODE ? psSrc->aui32Swizzle[component] : component);
                            int byteOffsetOfVar = psVarType->Offset;
                            unsigned int startRow       = ((byteOffset - byteOffsetOfVar) >> 2) / psVarType->Columns;
                            unsigned int startComponent = ((byteOffset - byteOffsetOfVar) >> 2) % psVarType->Columns;
                            unsigned int s = startComponent;

                            bformata(glsl, "[%d]", startRow);
                            bcatcstr(glsl, ".");
#if 0
                            for (s = startComponent; s < min(min(psVarType->Rows, 4U - component), ui32CompNum); ++s)
#endif
                            bformata(glsl, "%s", swizzleString[s]);
                        }
                        else if (psVarType->Class == SVC_MATRIX_COLUMNS)
                        {
                            int byteOffset = ((int*)psSrcByteOff->afImmediates)[0] + 4 * (psSrc->eSelMode == OPERAND_4_COMPONENT_SWIZZLE_MODE ? psSrc->aui32Swizzle[component] : component);
                            int byteOffsetOfVar = psVarType->Offset;
                            unsigned int startCol       = ((byteOffset - byteOffsetOfVar) >> 2) / psVarType->Rows;
                            unsigned int startComponent = ((byteOffset - byteOffsetOfVar) >> 2) % psVarType->Rows;
                            unsigned int s = startComponent;

                            bformata(glsl, "[%d]", startCol);
                            bcatcstr(glsl, ".");
#if 0
                            for (s = startComponent; s < min(min(psVarType->Columns, 4U - component), ui32CompNum); ++s)
#endif
                            bformata(glsl, "%s", swizzleString[s]);
                        }
                        else
                        {
                            //assert(0);
                        }
                    }
                    else
                    {
                        TranslateOperand(psContext, psSrc, ui32DataTypeFlag & TO_FLAG_NAME_ONLY);
                        bformata(glsl, "[");
                        TranslateOperand(psContext, psSrcAddr, TO_FLAG_INTEGER);
                        bcatcstr(glsl, "].");

                        ShaderVarName(glsl, psContext->psShader, psVarType->Name);
                    }

                    if (psVarType->Type == SVT_DOUBLE)
                    {
                        component++; // doubles take up 2 slots
                    }
#if 0
                    if (psVarType->Class == SVC_VECTOR)
                    {
                        component += min(psVarType->Columns, ui32CompNum) - 1; // vector take up various slots
                    }
                    if (psVarType->Class == SVC_MATRIX_ROWS)
                    {
                        component += min(psVarType->Columns * psVarType->Rows, ui32CompNum) - 1; // matrix take up various slots
                    }
                    if (psVarType->Class == SVC_MATRIX_COLUMNS)
                    {
                        component += min(psVarType->Columns * psVarType->Rows, ui32CompNum) - 1; // matrix take up various slots
                    }
#endif
                }

                if (addedBitcast)
                {
                    bcatcstr(glsl, ")");
                }

                EndAssignment(psContext, psDest, TO_FLAG_FLOAT, psInst->bSaturate);
                bformata(glsl, ";\n");
            }
        }
    }
}

static void TranslateShaderPLSLoad(HLSLCrossCompilerContext* psContext, Instruction* psInst)
{
    bstring glsl = *psContext->currentGLSLString;
    ShaderVarType* psVarType = NULL;
    uint32_t aui32Swizzle[4] = { OPERAND_4_COMPONENT_X };
    int component;
    int destComponent = 0;

    Operand* psDest = 0;
    Operand* psSrcAddr = 0;
    Operand* psSrcByteOff = 0;
    Operand* psSrc = 0;

    switch (psInst->eOpcode)
    {
    case OPCODE_LD_STRUCTURED:
        psDest = &psInst->asOperands[0];
        psSrcAddr = &psInst->asOperands[1];
        psSrcByteOff = &psInst->asOperands[2];
        psSrc = &psInst->asOperands[3];
        break;
    case OPCODE_LD_RAW:
    default:
        ASSERT(0);
    }

    unsigned int ui32CompNum = GetNumSwizzleElements(psDest);

    for (component = 0; component < 4; component++)
    {
        const char* swizzleString[] = { "x", "y", "z", "w" };
        ASSERT(psDest->eSelMode == OPERAND_4_COMPONENT_MASK_MODE);
        if (psDest->ui32CompMask & (1 << component))
        {
            int addedBitcast = 0;

            psVarType = LookupStructuredVar(psContext, psSrc, psSrcByteOff, psSrc->aui32Swizzle[component]);

            AddIndentation(psContext);

            aui32Swizzle[0] = psSrc->aui32Swizzle[component];

            if (ui32CompNum > 1)
            {
                BeginAssignmentEx(psContext, psDest, TO_FLAG_FLOAT, psInst->bSaturate, swizzleString[destComponent++]);
            }
            else
            {
                BeginAssignment(psContext, psDest, TO_FLAG_FLOAT, psInst->bSaturate);
            }

            ASSERT(psSrc->eType != OPERAND_TYPE_THREAD_GROUP_SHARED_MEMORY);

            ConstantBuffer* psCBuf = NULL;
            psVarType = LookupStructuredVar(psContext, psSrc, psSrcByteOff, psSrc->eSelMode == OPERAND_4_COMPONENT_SWIZZLE_MODE ? psSrc->aui32Swizzle[component] : component);
            GetConstantBufferFromBindingPoint(RGROUP_UAV, psSrc->ui32RegisterNumber, &psContext->psShader->sInfo, &psCBuf);

            if (psVarType->Type == SVT_UINT)
            {
                bcatcstr(glsl, "uintBitsToFloat(");
                addedBitcast = 1;
            }
            else if (psVarType->Type == SVT_INT)
            {
                bcatcstr(glsl, "intBitsToFloat(");
                addedBitcast = 1;
            }
            else if (psVarType->Type == SVT_DOUBLE)
            {
                ASSERT(0);
            }

            ASSERT(psSrc->eType == OPERAND_TYPE_UNORDERED_ACCESS_VIEW);

            TranslateOperand(psContext, psSrc, TO_FLAG_DESTINATION | TO_FLAG_NAME_ONLY);
            ASSERT(strcmp(psVarType->Name, "$Element") != 0);

            bcatcstr(glsl, ".");
            ShaderVarName(glsl, psContext->psShader, psVarType->Name);

            ASSERT(psVarType->Type != SVT_DOUBLE);
            ASSERT(psVarType->Class != SVC_MATRIX_ROWS);
            ASSERT(psVarType->Class != SVC_MATRIX_COLUMNS);

            if (psVarType->Class == SVC_VECTOR)
            {
                int byteOffset = ((int*)psSrcByteOff->afImmediates)[0] + 4 * (psSrc->eSelMode == OPERAND_4_COMPONENT_SWIZZLE_MODE ? psSrc->aui32Swizzle[component] : component);
                int byteOffsetOfVar = psVarType->Offset;
                unsigned int startComponent = (byteOffset - byteOffsetOfVar) >> 2;
                unsigned int s = startComponent;

                bcatcstr(glsl, ".");
                bformata(glsl, "%s", swizzleString[s]);
            }

            if (addedBitcast)
            {
                bcatcstr(glsl, ")");
            }

            EndAssignment(psContext, psDest, TO_FLAG_FLOAT, psInst->bSaturate);
            bformata(glsl, ";\n");
        }
    }
}

void TranslateAtomicMemOp(HLSLCrossCompilerContext* psContext, Instruction* psInst)
{
    bstring glsl = *psContext->currentGLSLString;
    ShaderVarType* psVarType = NULL;
    uint32_t ui32DataTypeFlag = TO_FLAG_INTEGER;
    const char* func = "";
    Operand* dest = 0;
    Operand* previousValue = 0;
    Operand* destAddr = 0;
    Operand* src = 0;
    Operand* compare = 0;

    switch (psInst->eOpcode)
    {
    case OPCODE_IMM_ATOMIC_IADD:
    {
#ifdef _DEBUG
        AddIndentation(psContext);
        bcatcstr(glsl, "//IMM_ATOMIC_IADD\n");
#endif
        func = "atomicAdd";
        previousValue = &psInst->asOperands[0];
        dest = &psInst->asOperands[1];
        destAddr = &psInst->asOperands[2];
        src = &psInst->asOperands[3];
        break;
    }
    case OPCODE_ATOMIC_IADD:
    {
#ifdef _DEBUG
        AddIndentation(psContext);
        bcatcstr(glsl, "//ATOMIC_IADD\n");
#endif
        func = "atomicAdd";
        dest = &psInst->asOperands[0];
        destAddr = &psInst->asOperands[1];
        src = &psInst->asOperands[2];
        break;
    }
    case OPCODE_IMM_ATOMIC_AND:
    {
#ifdef _DEBUG
        AddIndentation(psContext);
        bcatcstr(glsl, "//IMM_ATOMIC_AND\n");
#endif
        func = "atomicAnd";
        previousValue = &psInst->asOperands[0];
        dest = &psInst->asOperands[1];
        destAddr = &psInst->asOperands[2];
        src = &psInst->asOperands[3];
        break;
    }
    case OPCODE_ATOMIC_AND:
    {
#ifdef _DEBUG
        AddIndentation(psContext);
        bcatcstr(glsl, "//ATOMIC_AND\n");
#endif
        func = "atomicAnd";
        dest = &psInst->asOperands[0];
        destAddr = &psInst->asOperands[1];
        src = &psInst->asOperands[2];
        break;
    }
    case OPCODE_IMM_ATOMIC_OR:
    {
#ifdef _DEBUG
        AddIndentation(psContext);
        bcatcstr(glsl, "//IMM_ATOMIC_OR\n");
#endif
        func = "atomicOr";
        previousValue = &psInst->asOperands[0];
        dest = &psInst->asOperands[1];
        destAddr = &psInst->asOperands[2];
        src = &psInst->asOperands[3];
        break;
    }
    case OPCODE_ATOMIC_OR:
    {
#ifdef _DEBUG
        AddIndentation(psContext);
        bcatcstr(glsl, "//ATOMIC_OR\n");
#endif
        func = "atomicOr";
        dest = &psInst->asOperands[0];
        destAddr = &psInst->asOperands[1];
        src = &psInst->asOperands[2];
        break;
    }
    case OPCODE_IMM_ATOMIC_XOR:
    {
#ifdef _DEBUG
        AddIndentation(psContext);
        bcatcstr(glsl, "//IMM_ATOMIC_XOR\n");
#endif
        func = "atomicXor";
        previousValue = &psInst->asOperands[0];
        dest = &psInst->asOperands[1];
        destAddr = &psInst->asOperands[2];
        src = &psInst->asOperands[3];
        break;
    }
    case OPCODE_ATOMIC_XOR:
    {
#ifdef _DEBUG
        AddIndentation(psContext);
        bcatcstr(glsl, "//ATOMIC_XOR\n");
#endif
        func = "atomicXor";
        dest = &psInst->asOperands[0];
        destAddr = &psInst->asOperands[1];
        src = &psInst->asOperands[2];
        break;
    }

    case OPCODE_IMM_ATOMIC_EXCH:
    {
#ifdef _DEBUG
        AddIndentation(psContext);
        bcatcstr(glsl, "//IMM_ATOMIC_EXCH\n");
#endif
        func = "atomicExchange";
        previousValue = &psInst->asOperands[0];
        dest = &psInst->asOperands[1];
        destAddr = &psInst->asOperands[2];
        src = &psInst->asOperands[3];
        break;
    }
    case OPCODE_IMM_ATOMIC_CMP_EXCH:
    {
#ifdef _DEBUG
        AddIndentation(psContext);
        bcatcstr(glsl, "//IMM_ATOMIC_CMP_EXC\n");
#endif
        func = "atomicCompSwap";
        previousValue = &psInst->asOperands[0];
        dest = &psInst->asOperands[1];
        destAddr = &psInst->asOperands[2];
        compare = &psInst->asOperands[3];
        src = &psInst->asOperands[4];
        break;
    }
    case OPCODE_ATOMIC_CMP_STORE:
    {
#ifdef _DEBUG
        AddIndentation(psContext);
        bcatcstr(glsl, "//ATOMIC_CMP_STORE\n");
#endif
        func = "atomicCompSwap";
        previousValue = 0;
        dest = &psInst->asOperands[0];
        destAddr = &psInst->asOperands[1];
        compare = &psInst->asOperands[2];
        src = &psInst->asOperands[3];
        break;
    }
    case OPCODE_IMM_ATOMIC_UMIN:
    {
#ifdef _DEBUG
        AddIndentation(psContext);
        bcatcstr(glsl, "//IMM_ATOMIC_UMIN\n");
#endif
        func = "atomicMin";
        previousValue = &psInst->asOperands[0];
        dest = &psInst->asOperands[1];
        destAddr = &psInst->asOperands[2];
        src = &psInst->asOperands[3];
        break;
    }
    case OPCODE_ATOMIC_UMIN:
    {
#ifdef _DEBUG
        AddIndentation(psContext);
        bcatcstr(glsl, "//ATOMIC_UMIN\n");
#endif
        func = "atomicMin";
        dest = &psInst->asOperands[0];
        destAddr = &psInst->asOperands[1];
        src = &psInst->asOperands[2];
        break;
    }
    case OPCODE_IMM_ATOMIC_IMIN:
    {
#ifdef _DEBUG
        AddIndentation(psContext);
        bcatcstr(glsl, "//IMM_ATOMIC_IMIN\n");
#endif
        func = "atomicMin";
        previousValue = &psInst->asOperands[0];
        dest = &psInst->asOperands[1];
        destAddr = &psInst->asOperands[2];
        src = &psInst->asOperands[3];
        break;
    }
    case OPCODE_ATOMIC_IMIN:
    {
#ifdef _DEBUG
        AddIndentation(psContext);
        bcatcstr(glsl, "//ATOMIC_IMIN\n");
#endif
        func = "atomicMin";
        dest = &psInst->asOperands[0];
        destAddr = &psInst->asOperands[1];
        src = &psInst->asOperands[2];
        break;
    }
    case OPCODE_IMM_ATOMIC_UMAX:
    {
#ifdef _DEBUG
        AddIndentation(psContext);
        bcatcstr(glsl, "//IMM_ATOMIC_UMAX\n");
#endif
        func = "atomicMax";
        previousValue = &psInst->asOperands[0];
        dest = &psInst->asOperands[1];
        destAddr = &psInst->asOperands[2];
        src = &psInst->asOperands[3];
        break;
    }
    case OPCODE_ATOMIC_UMAX:
    {
#ifdef _DEBUG
        AddIndentation(psContext);
        bcatcstr(glsl, "//ATOMIC_UMAX\n");
#endif
        func = "atomicMax";
        dest = &psInst->asOperands[0];
        destAddr = &psInst->asOperands[1];
        src = &psInst->asOperands[2];
        break;
    }
    case OPCODE_IMM_ATOMIC_IMAX:
    {
#ifdef _DEBUG
        AddIndentation(psContext);
        bcatcstr(glsl, "//IMM_ATOMIC_IMAX\n");
#endif
        func = "atomicMax";
        previousValue = &psInst->asOperands[0];
        dest = &psInst->asOperands[1];
        destAddr = &psInst->asOperands[2];
        src = &psInst->asOperands[3];
        break;
    }
    case OPCODE_ATOMIC_IMAX:
    {
#ifdef _DEBUG
        AddIndentation(psContext);
        bcatcstr(glsl, "//ATOMIC_IMAX\n");
#endif
        func = "atomicMax";
        dest = &psInst->asOperands[0];
        destAddr = &psInst->asOperands[1];
        src = &psInst->asOperands[2];
        break;
    }
    }

    AddIndentation(psContext);

    psVarType = LookupStructuredVar(psContext, dest, NULL, 0);

    if (psVarType->Type == SVT_UINT)
    {
        ui32DataTypeFlag = TO_FLAG_INTEGER | TO_FLAG_UNSIGNED_INTEGER;
    }
    else if (psVarType->Type == SVT_INT)
    {
        ui32DataTypeFlag = TO_FLAG_INTEGER;
    }

    if (previousValue)
    {
        BeginAssignment(psContext, previousValue, ui32DataTypeFlag, psInst->bSaturate);
    }

    if (dest->eType == OPERAND_TYPE_THREAD_GROUP_SHARED_MEMORY)
    {
        bcatcstr(glsl, func);
        bcatcstr(glsl, "(");
        TranslateOperand(psContext, dest, ui32DataTypeFlag & TO_FLAG_NAME_ONLY);
        bformata(glsl, "[%d]", 0);
    }
    else
    {
        bcatcstr(glsl, func);
        bcatcstr(glsl, "(");
        TranslateShaderStorageVarName(glsl, psContext->psShader, dest, 1);
        bformata(glsl, "[");
        TranslateOperand(psContext, destAddr, TO_FLAG_INTEGER | TO_FLAG_UNSIGNED_INTEGER);
        // For some reason the destAddr with the swizzle doesn't translate to an index
        // I'm not sure if ".x" is the correct behavior.
        bformata(glsl, ".x]");
    }

    if (strcmp(psVarType->Name, "$Element") != 0)
    {
        bcatcstr(glsl, ".");
        ShaderVarName(glsl, psContext->psShader, psVarType->Name);
    }
    bcatcstr(glsl, ", ");

    if (compare)
    {
        TranslateOperand(psContext, compare, ui32DataTypeFlag);
        bcatcstr(glsl, ", ");
    }

    TranslateOperand(psContext, src, ui32DataTypeFlag);
    bcatcstr(glsl, ")");

    if (previousValue)
    {
        EndAssignment(psContext, previousValue, ui32DataTypeFlag, psInst->bSaturate);
    }

    bcatcstr(glsl, ";\n");
}

static void TranslateConditional(HLSLCrossCompilerContext* psContext,
    Instruction* psInst,
    bstring glsl)
{
    const char* statement = "";
    uint32_t bWriteTraceEnd = 0;
    if (psInst->eOpcode == OPCODE_BREAKC)
    {
        statement = "break";
    }
    else if (psInst->eOpcode == OPCODE_CONTINUEC)
    {
        statement = "continue";
    }
    else if (psInst->eOpcode == OPCODE_RETC)
    {
        statement = "return";
        bWriteTraceEnd = (psContext->flags & HLSLCC_FLAG_TRACING_INSTRUMENTATION) != 0;
    }

    if (psContext->psShader->ui32MajorVersion < 4)
    {
        bcatcstr(glsl, "if(");

        TranslateOperand(psContext, &psInst->asOperands[0], TO_FLAG_INTEGER | TO_FLAG_UNSIGNED_INTEGER);
        switch (psInst->eDX9TestType)
        {
        case D3DSPC_GT:
        {
            bcatcstr(glsl, " > ");
            break;
        }
        case D3DSPC_EQ:
        {
            bcatcstr(glsl, " == ");
            break;
        }
        case D3DSPC_GE:
        {
            bcatcstr(glsl, " >= ");
            break;
        }
        case D3DSPC_LT:
        {
            bcatcstr(glsl, " < ");
            break;
        }
        case D3DSPC_NE:
        {
            bcatcstr(glsl, " != ");
            break;
        }
        case D3DSPC_LE:
        {
            bcatcstr(glsl, " <= ");
            break;
        }
        case D3DSPC_BOOLEAN:
        {
            bcatcstr(glsl, " != 0");
            break;
        }
        default:
        {
            break;
        }
        }

        if (psInst->eDX9TestType != D3DSPC_BOOLEAN)
        {
            TranslateOperand(psContext, &psInst->asOperands[1], TO_FLAG_NONE);
        }

        if (psInst->eOpcode != OPCODE_IF && !bWriteTraceEnd)
        {
            bformata(glsl, "){ %s; }\n", statement);
        }
        else
        {
            bcatcstr(glsl, "){\n");
        }
    }
    else
    {
        if (psInst->eBooleanTestType == INSTRUCTION_TEST_ZERO)
        {
            bcatcstr(glsl, "if((");
            TranslateOperand(psContext, &psInst->asOperands[0], TO_FLAG_INTEGER | TO_FLAG_UNSIGNED_INTEGER);

            if (psInst->eOpcode != OPCODE_IF && !bWriteTraceEnd)
            {
                if (GetOperandDataType(psContext, &psInst->asOperands[0]) == SVT_UINT)
                {
                    bformata(glsl, ")==uint(0){%s;}\n", statement); // Adreno can't handle 0u (it's treated as int)
                }
                else
                {
                    bformata(glsl, ")==0){%s;}\n", statement);
                }
            }
            else
            {
                if (GetOperandDataType(psContext, &psInst->asOperands[0]) == SVT_UINT)
                {
                    bcatcstr(glsl, ")==uint(0){\n"); // Adreno can't handle 0u (it's treated as int)
                }
                else
                {
                    bcatcstr(glsl, ")==0){\n");
                }
            }
        }
        else
        {
            ASSERT(psInst->eBooleanTestType == INSTRUCTION_TEST_NONZERO);
            bcatcstr(glsl, "if((");
            TranslateOperand(psContext, &psInst->asOperands[0], TO_FLAG_INTEGER | TO_FLAG_UNSIGNED_INTEGER);

            if (psInst->eOpcode != OPCODE_IF && !bWriteTraceEnd)
            {
                if (GetOperandDataType(psContext, &psInst->asOperands[0]) == SVT_UINT)
                {
                    bformata(glsl, ")!=uint(0)){%s;}\n", statement); // Adreno can't handle 0u (it's treated as int)
                }
                else
                {
                    bformata(glsl, ")!=0){%s;}\n", statement);
                }
            }
            else
            {
                if (GetOperandDataType(psContext, &psInst->asOperands[0]) == SVT_UINT)
                {
                    bcatcstr(glsl, ")!=uint(0)){\n"); // Adreno can't handle 0u (it's treated as int)
                }
                else
                {
                    bcatcstr(glsl, ")!=0){\n");
                }
            }
        }
    }

    if (bWriteTraceEnd)
    {
        ASSERT(*psContext->currentGLSLString == glsl);
        ++psContext->indent;
        WriteEndTrace(psContext);
        AddIndentation(psContext);
        bformata(glsl, "%s;\n", statement);
        AddIndentation(psContext);
        --psContext->indent;
        bcatcstr(glsl, "}\n");
    }
}

void UpdateCommonTempVecType(SHADER_VARIABLE_TYPE* peCommonTempVecType, SHADER_VARIABLE_TYPE eNewType)
{
    if (*peCommonTempVecType == SVT_FORCE_DWORD)
    {
        *peCommonTempVecType = eNewType;
    }
    else if (*peCommonTempVecType != eNewType)
    {
        *peCommonTempVecType = SVT_VOID;
    }
}

bool IsFloatType(SHADER_VARIABLE_TYPE type)
{
    switch (type)
    {
    case SVT_FLOAT:
    case SVT_FLOAT10:
    case SVT_FLOAT16:
        return true;
    default:
        return false;
    }
}

void SetDataTypes(HLSLCrossCompilerContext* psContext, Instruction* psInst, const int32_t i32InstCount, SHADER_VARIABLE_TYPE* aeCommonTempVecType)
{
    int32_t i;

    SHADER_VARIABLE_TYPE aeTempVecType[MAX_TEMP_VEC4 * 4];

    for (i = 0; i < MAX_TEMP_VEC4 * 4; ++i)
    {
        aeTempVecType[i] = SVT_FLOAT;
    }
    if (aeCommonTempVecType != NULL)
    {
        for (i = 0; i < MAX_TEMP_VEC4; ++i)
        {
            aeCommonTempVecType[i] = SVT_FORCE_DWORD;
        }
    }

    for (i = 0; i < i32InstCount; ++i, psInst++)
    {
        int k = 0;

        if (psInst->ui32NumOperands == 0)
        {
            continue;
        }

        //Preserve the current type on dest array index
        if (psInst->asOperands[0].eType == OPERAND_TYPE_INDEXABLE_TEMP)
        {
            Operand* psSubOperand = psInst->asOperands[0].psSubOperand[1];
            if (psSubOperand != 0)
            {
                const uint32_t ui32RegIndex = psSubOperand->ui32RegisterNumber * 4;
                ASSERT(psSubOperand->eType == OPERAND_TYPE_TEMP);

                if (psSubOperand->eSelMode == OPERAND_4_COMPONENT_SELECT_1_MODE)
                {
                    psSubOperand->aeDataType[psSubOperand->aui32Swizzle[0]] = aeTempVecType[ui32RegIndex + psSubOperand->aui32Swizzle[0]];
                }
                else if (psSubOperand->eSelMode == OPERAND_4_COMPONENT_SWIZZLE_MODE)
                {
                    if (psSubOperand->ui32Swizzle == (NO_SWIZZLE))
                    {
                        psSubOperand->aeDataType[0] = aeTempVecType[ui32RegIndex];
                        psSubOperand->aeDataType[1] = aeTempVecType[ui32RegIndex];
                        psSubOperand->aeDataType[2] = aeTempVecType[ui32RegIndex];
                        psSubOperand->aeDataType[3] = aeTempVecType[ui32RegIndex];
                    }
                    else
                    {
                        psSubOperand->aeDataType[psSubOperand->aui32Swizzle[0]] = aeTempVecType[ui32RegIndex + psSubOperand->aui32Swizzle[0]];
                    }
                }
                else if (psSubOperand->eSelMode == OPERAND_4_COMPONENT_MASK_MODE)
                {
                    int c = 0;
                    uint32_t ui32CompMask = psSubOperand->ui32CompMask;
                    if (!psSubOperand->ui32CompMask)
                    {
                        ui32CompMask = OPERAND_4_COMPONENT_MASK_ALL;
                    }

                    for (; c < 4; ++c)
                    {
                        if (ui32CompMask & (1 << c))
                        {
                            psSubOperand->aeDataType[c] = aeTempVecType[ui32RegIndex + c];
                        }
                    }
                }
            }
        }

        //Preserve the current type on sources.
        for (k = psInst->ui32NumOperands - 1; k >= (int)psInst->ui32FirstSrc; --k)
        {
            int32_t subOperand;
            Operand* psOperand = &psInst->asOperands[k];

            if (psOperand->eType == OPERAND_TYPE_TEMP)
            {
                const uint32_t ui32RegIndex = psOperand->ui32RegisterNumber * 4;

                if (psOperand->eSelMode == OPERAND_4_COMPONENT_SELECT_1_MODE)
                {
                    psOperand->aeDataType[psOperand->aui32Swizzle[0]] = aeTempVecType[ui32RegIndex + psOperand->aui32Swizzle[0]];
                }
                else if (psOperand->eSelMode == OPERAND_4_COMPONENT_SWIZZLE_MODE)
                {
                    if (psOperand->ui32Swizzle == (NO_SWIZZLE))
                    {
                        psOperand->aeDataType[0] = aeTempVecType[ui32RegIndex];
                        psOperand->aeDataType[1] = aeTempVecType[ui32RegIndex];
                        psOperand->aeDataType[2] = aeTempVecType[ui32RegIndex];
                        psOperand->aeDataType[3] = aeTempVecType[ui32RegIndex];
                    }
                    else
                    {
                        psOperand->aeDataType[psOperand->aui32Swizzle[0]] = aeTempVecType[ui32RegIndex + psOperand->aui32Swizzle[0]];
                    }
                }
                else if (psOperand->eSelMode == OPERAND_4_COMPONENT_MASK_MODE)
                {
                    int c = 0;
                    uint32_t ui32CompMask = psOperand->ui32CompMask;
                    if (!psOperand->ui32CompMask)
                    {
                        ui32CompMask = OPERAND_4_COMPONENT_MASK_ALL;
                    }

                    for (; c < 4; ++c)
                    {
                        if (ui32CompMask & (1 << c))
                        {
                            psOperand->aeDataType[c] = aeTempVecType[ui32RegIndex + c];
                        }
                    }
                }
            }

            for (subOperand = 0; subOperand < MAX_SUB_OPERANDS; subOperand++)
            {
                if (psOperand->psSubOperand[subOperand] != 0)
                {
                    Operand* psSubOperand = psOperand->psSubOperand[subOperand];
                    if (psSubOperand->eType == OPERAND_TYPE_TEMP)
                    {
                        const uint32_t ui32RegIndex = psSubOperand->ui32RegisterNumber * 4;

                        if (psSubOperand->eSelMode == OPERAND_4_COMPONENT_SELECT_1_MODE)
                        {
                            psSubOperand->aeDataType[psSubOperand->aui32Swizzle[0]] = aeTempVecType[ui32RegIndex + psSubOperand->aui32Swizzle[0]];
                        }
                        else if (psSubOperand->eSelMode == OPERAND_4_COMPONENT_SWIZZLE_MODE)
                        {
                            if (psSubOperand->ui32Swizzle == (NO_SWIZZLE))
                            {
                                psSubOperand->aeDataType[0] = aeTempVecType[ui32RegIndex];
                                psSubOperand->aeDataType[1] = aeTempVecType[ui32RegIndex];
                                psSubOperand->aeDataType[2] = aeTempVecType[ui32RegIndex];
                                psSubOperand->aeDataType[3] = aeTempVecType[ui32RegIndex];
                            }
                            else
                            {
                                psSubOperand->aeDataType[psSubOperand->aui32Swizzle[0]] = aeTempVecType[ui32RegIndex + psSubOperand->aui32Swizzle[0]];
                            }
                        }
                        else if (psSubOperand->eSelMode == OPERAND_4_COMPONENT_MASK_MODE)
                        {
                            int c = 0;
                            uint32_t ui32CompMask = psSubOperand->ui32CompMask;
                            if (!psSubOperand->ui32CompMask)
                            {
                                ui32CompMask = OPERAND_4_COMPONENT_MASK_ALL;
                            }


                            for (; c < 4; ++c)
                            {
                                if (ui32CompMask & (1 << c))
                                {
                                    psSubOperand->aeDataType[c] = aeTempVecType[ui32RegIndex + c];
                                }
                            }
                        }
                    }
                }
            }
        }

        SHADER_VARIABLE_TYPE eNewType = SVT_FORCE_DWORD;

        switch (psInst->eOpcode)
        {
        case OPCODE_RESINFO:
        {
            if (psInst->eResInfoReturnType == RESINFO_INSTRUCTION_RETURN_UINT)
            {
                eNewType = SVT_INT;
            }
            else
            {
                eNewType = SVT_FLOAT;
            }
            break;
        }
        case OPCODE_AND:
        case OPCODE_OR:
        case OPCODE_XOR:
        case OPCODE_NOT:
        {
            eNewType = SVT_UINT;
            break;
        }
        case OPCODE_IADD:
        case OPCODE_IMAD:
        case OPCODE_IMAX:
        case OPCODE_IMIN:
        case OPCODE_IMUL:
        case OPCODE_INEG:
        case OPCODE_ISHL:
        case OPCODE_ISHR:
        {
            eNewType = SVT_UINT;

            //If the rhs evaluates to signed then that is the dest type picked.
            for (uint32_t kk = psInst->ui32FirstSrc; kk < psInst->ui32NumOperands; ++kk)
            {
                if (GetOperandDataType(psContext, &psInst->asOperands[kk]) == SVT_INT ||
                    psInst->asOperands[kk].eModifier == OPERAND_MODIFIER_NEG ||
                    psInst->asOperands[kk].eModifier == OPERAND_MODIFIER_ABSNEG)
                {
                    eNewType = SVT_INT;
                    break;
                }
            }

            break;
        }
        case OPCODE_IMM_ATOMIC_AND:
        case OPCODE_IMM_ATOMIC_IADD:
        case OPCODE_IMM_ATOMIC_IMAX:
        case OPCODE_IMM_ATOMIC_IMIN:
        case OPCODE_IMM_ATOMIC_UMAX:
        case OPCODE_IMM_ATOMIC_UMIN:
        case OPCODE_IMM_ATOMIC_OR:
        case OPCODE_IMM_ATOMIC_XOR:
        case OPCODE_IMM_ATOMIC_EXCH:
        case OPCODE_IMM_ATOMIC_CMP_EXCH:
        {
            Operand* dest = &psInst->asOperands[1];
            ShaderVarType* type = LookupStructuredVar(psContext, dest, NULL, 0);
            eNewType = type->Type;
            break;
        }

        case OPCODE_IEQ:
        case OPCODE_IGE:
        case OPCODE_ILT:
        case OPCODE_INE:
        case OPCODE_EQ:
        case OPCODE_GE:
        case OPCODE_LT:
        case OPCODE_NE:
        case OPCODE_UDIV:
        case OPCODE_ULT:
        case OPCODE_UGE:
        case OPCODE_UMUL:
        case OPCODE_UMAD:
        case OPCODE_UMAX:
        case OPCODE_UMIN:
        case OPCODE_USHR:
        case OPCODE_IMM_ATOMIC_ALLOC:
        case OPCODE_IMM_ATOMIC_CONSUME:
        {
            if (psContext->psShader->ui32MajorVersion < 4)
            {
                //SLT and SGE are translated to LT and GE respectively.
                //But SLT and SGE have a floating point 1.0f or 0.0f result
                //instead of setting all bits on or all bits off.
                eNewType = SVT_FLOAT;
            }
            else
            {
                eNewType = SVT_UINT;
            }
            break;
        }

        case OPCODE_SAMPLE:
        case OPCODE_SAMPLE_L:
        case OPCODE_SAMPLE_D:
        case OPCODE_SAMPLE_B:
        case OPCODE_LD:
        case OPCODE_LD_MS:
        case OPCODE_LD_UAV_TYPED:
        {
            ResourceBinding* psRes = NULL;
            if (psInst->eOpcode == OPCODE_LD_UAV_TYPED)
            {
                GetResourceFromBindingPoint(RGROUP_UAV, psInst->asOperands[2].ui32RegisterNumber, &psContext->psShader->sInfo, &psRes);
            }
            else
            {
                GetResourceFromBindingPoint(RGROUP_TEXTURE, psInst->asOperands[2].ui32RegisterNumber, &psContext->psShader->sInfo, &psRes);
            }
            switch (psRes->ui32ReturnType)
            {
            case RETURN_TYPE_SINT:
                eNewType = SVT_INT;
                break;
            case RETURN_TYPE_UINT:
                eNewType = SVT_UINT;
                break;
            case RETURN_TYPE_FLOAT:
                eNewType = SVT_FLOAT;
                break;
            default:
                ASSERT(0);
                break;
            }
            break;
        }

        case OPCODE_MOV:
        {
            //Inherit the type of the source operand
            const Operand* psOperand = &psInst->asOperands[0];
            if (psOperand->eType == OPERAND_TYPE_TEMP)
            {
                eNewType = GetOperandDataType(psContext, &psInst->asOperands[1]);
            }
            else
            {
                continue;
            }
            break;
        }
        case OPCODE_MOVC:
        {
            //Inherit the type of the source operand
            const Operand* psOperand = &psInst->asOperands[0];
            if (psOperand->eType == OPERAND_TYPE_TEMP)
            {
                eNewType = GetOperandDataType(psContext, &psInst->asOperands[2]);
                //Check assumption that both the values which MOVC might pick have the same basic data type.
                if (!psContext->flags & HLSLCC_FLAG_AVOID_TEMP_REGISTER_ALIASING)
                {
                    ASSERT(GetOperandDataType(psContext, &psInst->asOperands[2]) == GetOperandDataType(psContext, &psInst->asOperands[3]));
                }
            }
            else
            {
                continue;
            }
            break;
        }
        case OPCODE_FTOI:
        {
            ASSERT(IsFloatType(GetOperandDataType(psContext, &psInst->asOperands[1])) ||
                GetOperandDataType(psContext, &psInst->asOperands[1]) == SVT_VOID);
            eNewType = SVT_INT;
            break;
        }
        case OPCODE_FTOU:
        {
            ASSERT(IsFloatType(GetOperandDataType(psContext, &psInst->asOperands[1])) ||
                GetOperandDataType(psContext, &psInst->asOperands[1]) == SVT_VOID);
            eNewType = SVT_UINT;
            break;
        }

        case OPCODE_UTOF:
        case OPCODE_ITOF:
        {
            eNewType = SVT_FLOAT;
            break;
        }
        case OPCODE_IF:
        case OPCODE_SWITCH:
        case OPCODE_BREAKC:
        {
            const Operand* psOperand = &psInst->asOperands[0];
            if (psOperand->eType == OPERAND_TYPE_TEMP)
            {
                const uint32_t ui32RegIndex = psOperand->ui32RegisterNumber * 4;

                if (psOperand->eSelMode == OPERAND_4_COMPONENT_SELECT_1_MODE)
                {
                    eNewType = aeTempVecType[ui32RegIndex + psOperand->aui32Swizzle[0]];
                }
                else if (psOperand->eSelMode == OPERAND_4_COMPONENT_SWIZZLE_MODE)
                {
                    if (psOperand->ui32Swizzle == (NO_SWIZZLE))
                    {
                        eNewType = aeTempVecType[ui32RegIndex];
                    }
                    else
                    {
                        eNewType = aeTempVecType[ui32RegIndex + psOperand->aui32Swizzle[0]];
                    }
                }
                else if (psOperand->eSelMode == OPERAND_4_COMPONENT_MASK_MODE)
                {
                    uint32_t ui32CompMask = psOperand->ui32CompMask;
                    if (!psOperand->ui32CompMask)
                    {
                        ui32CompMask = OPERAND_4_COMPONENT_MASK_ALL;
                    }
                    for (; k < 4; ++k)
                    {
                        if (ui32CompMask & (1 << k))
                        {
                            eNewType = aeTempVecType[ui32RegIndex + k];
                        }
                    }
                }
            }
            else
            {
                continue;
            }
            break;
        }
        case OPCODE_DADD:
        {
            eNewType = SVT_DOUBLE;
            break;
        }
        case OPCODE_STORE_RAW:
        {
            eNewType = SVT_FLOAT;
            break;
        }
        default:
        {
            eNewType = SVT_FLOAT;
            break;
        }
        }

        if (eNewType == SVT_UINT && HaveUVec(psContext->psShader->eTargetLanguage) == 0)
        {
            //Fallback to signed int if unsigned int is not supported.
            eNewType = SVT_INT;
        }

        //Process the destination last in order to handle instructions
        //where the destination register is also used as a source.
        for (k = 0; k < (int)psInst->ui32FirstSrc; ++k)
        {
            Operand* psOperand = &psInst->asOperands[k];
            if (psOperand->eType == OPERAND_TYPE_TEMP)
            {
                const uint32_t ui32RegIndex = psOperand->ui32RegisterNumber * 4;
                if (HavePrecisionQualifers(psContext->psShader->eTargetLanguage))
                {
                    switch (psOperand->eMinPrecision)
                    {
                    case OPERAND_MIN_PRECISION_DEFAULT:
                        break;
                    case OPERAND_MIN_PRECISION_SINT_16:
                        eNewType = SVT_INT16;
                        break;
                    case OPERAND_MIN_PRECISION_UINT_16:
                        eNewType = SVT_UINT16;
                        break;
                    case OPERAND_MIN_PRECISION_FLOAT_2_8:
                        eNewType = SVT_FLOAT10;
                        break;
                    case OPERAND_MIN_PRECISION_FLOAT_16:
                        eNewType = SVT_FLOAT16;
                        break;
                    default:
                        break;
                    }
                }

                if (aeCommonTempVecType != NULL)
                {
                    UpdateCommonTempVecType(aeCommonTempVecType + psOperand->ui32RegisterNumber, eNewType);
                }

                if (psOperand->eSelMode == OPERAND_4_COMPONENT_SELECT_1_MODE)
                {
                    aeTempVecType[ui32RegIndex + psOperand->aui32Swizzle[0]] = eNewType;
                    psOperand->aeDataType[psOperand->aui32Swizzle[0]] = eNewType;
                }
                else if (psOperand->eSelMode == OPERAND_4_COMPONENT_SWIZZLE_MODE)
                {
                    if (psOperand->ui32Swizzle == (NO_SWIZZLE))
                    {
                        aeTempVecType[ui32RegIndex] = eNewType;
                        psOperand->aeDataType[0] = eNewType;
                        psOperand->aeDataType[1] = eNewType;
                        psOperand->aeDataType[2] = eNewType;
                        psOperand->aeDataType[3] = eNewType;
                    }
                    else
                    {
                        aeTempVecType[ui32RegIndex + psOperand->aui32Swizzle[0]] = eNewType;
                        psOperand->aeDataType[psOperand->aui32Swizzle[0]] = eNewType;
                    }
                }
                else if (psOperand->eSelMode == OPERAND_4_COMPONENT_MASK_MODE)
                {
                    int c = 0;
                    uint32_t ui32CompMask = psOperand->ui32CompMask;
                    if (!psOperand->ui32CompMask)
                    {
                        ui32CompMask = OPERAND_4_COMPONENT_MASK_ALL;
                    }

                    for (; c < 4; ++c)
                    {
                        if (ui32CompMask & (1 << c))
                        {
                            aeTempVecType[ui32RegIndex + c] = eNewType;
                            psOperand->aeDataType[c] = eNewType;
                        }
                    }
                }
            }
        }
        ASSERT(eNewType != SVT_FORCE_DWORD);
    }
}

void TranslateInstruction(HLSLCrossCompilerContext* psContext, Instruction* psInst)
{
    bstring glsl = *psContext->currentGLSLString;

#ifdef _DEBUG
    AddIndentation(psContext);
    bformata(glsl, "//Instruction %d\n", psInst->id);
#if 0
    if (psInst->id == 73)
    {
        ASSERT(1);     //Set breakpoint here to debug an instruction from its ID.
    }
#endif
#endif

    switch (psInst->eOpcode)
    {
    case OPCODE_FTOI:     //Fall-through to MOV
    case OPCODE_FTOU:     //Fall-through to MOV
    case OPCODE_MOV:
    {
        uint32_t srcCount = GetNumSwizzleElements(&psInst->asOperands[1]);
        uint32_t dstCount = GetNumSwizzleElements(&psInst->asOperands[0]);
        uint32_t ui32DstFlags = TO_FLAG_NONE;

        if (psInst->eOpcode == OPCODE_FTOU)
        {
#ifdef _DEBUG
            AddIndentation(psContext);
            bcatcstr(glsl, "//FTOU\n");
#endif
            ui32DstFlags |= TO_FLAG_UNSIGNED_INTEGER;

            ASSERT(IsFloatType(GetOperandDataType(psContext, &psInst->asOperands[1])));
        }
        else if (psInst->eOpcode == OPCODE_FTOI)
        {
#ifdef _DEBUG
            AddIndentation(psContext);
            bcatcstr(glsl, "//FTOI\n");
#endif
            ui32DstFlags |= TO_FLAG_INTEGER;

            ASSERT(IsFloatType(GetOperandDataType(psContext, &psInst->asOperands[1])));
        }
        else
        {
#ifdef _DEBUG
            AddIndentation(psContext);
            bcatcstr(glsl, "//MOV\n");
#endif
        }

        if (psInst->eOpcode == OPCODE_FTOU)
        {
            AddIndentation(psContext);
            BeginAssignment(psContext, &psInst->asOperands[0], ui32DstFlags, psInst->bSaturate);

            if (srcCount == 1)
            {
                bcatcstr(glsl, "uint(");
            }
            if (srcCount == 2)
            {
                bcatcstr(glsl, "uvec2(");
            }
            if (srcCount == 3)
            {
                bcatcstr(glsl, "uvec3(");
            }
            if (srcCount == 4)
            {
                bcatcstr(glsl, "uvec4(");
            }

            TranslateOperand(psContext, &psInst->asOperands[1], TO_FLAG_FLOAT);
            if (srcCount != dstCount)
            {
                bcatcstr(glsl, ")");
                TranslateOperandSwizzle(psContext, &psInst->asOperands[0]);
                EndAssignment(psContext, &psInst->asOperands[0], ui32DstFlags, psInst->bSaturate);
                bcatcstr(glsl, ";\n");
            }
            else
            {
                bcatcstr(glsl, ")");
                EndAssignment(psContext, &psInst->asOperands[0], ui32DstFlags, psInst->bSaturate);
                bcatcstr(glsl, ";\n");
            }
        }
        else
        if (psInst->eOpcode == OPCODE_FTOI)
        {
            AddIndentation(psContext);
            BeginAssignment(psContext, &psInst->asOperands[0], ui32DstFlags, psInst->bSaturate);

            if (srcCount == 1)
            {
                bcatcstr(glsl, "int(");
            }
            if (srcCount == 2)
            {
                bcatcstr(glsl, "ivec2(");
            }
            if (srcCount == 3)
            {
                bcatcstr(glsl, "ivec3(");
            }
            if (srcCount == 4)
            {
                bcatcstr(glsl, "ivec4(");
            }

            TranslateOperand(psContext, &psInst->asOperands[1], TO_FLAG_FLOAT);

            if (srcCount != dstCount)
            {
                bcatcstr(glsl, ")");
                TranslateOperandSwizzle(psContext, &psInst->asOperands[0]);
                EndAssignment(psContext, &psInst->asOperands[0], ui32DstFlags, psInst->bSaturate);
                bcatcstr(glsl, ";\n");
            }
            else
            {
                bcatcstr(glsl, ")");
                EndAssignment(psContext, &psInst->asOperands[0], ui32DstFlags, psInst->bSaturate);
                bcatcstr(glsl, ";\n");
            }
        }
        else
        {
            AddMOVBinaryOp(psContext, &psInst->asOperands[0], &psInst->asOperands[1], 0, psInst->bSaturate);
        }
        break;
    }
    case OPCODE_ITOF:    //signed to float
    case OPCODE_UTOF:    //unsigned to float
    {
#ifdef _DEBUG
        AddIndentation(psContext);
        if (psInst->eOpcode == OPCODE_ITOF)
        {
            bcatcstr(glsl, "//ITOF\n");
        }
        else
        {
            bcatcstr(glsl, "//UTOF\n");
        }
#endif

        AddIndentation(psContext);
        BeginAssignment(psContext, &psInst->asOperands[0], TO_FLAG_FLOAT, psInst->bSaturate);
        bcatcstr(glsl, "vec4(");
        TranslateOperand(psContext, &psInst->asOperands[1], (psInst->eOpcode == OPCODE_ITOF) ? TO_FLAG_INTEGER : TO_FLAG_UNSIGNED_INTEGER);
        bcatcstr(glsl, ")");
        EndAssignment(psContext, &psInst->asOperands[0], TO_FLAG_FLOAT, psInst->bSaturate);
        TranslateOperandSwizzle(psContext, &psInst->asOperands[0]);
        bcatcstr(glsl, ";\n");
        break;
    }
    case OPCODE_MAD:
    {
#ifdef _DEBUG
        AddIndentation(psContext);
        bcatcstr(glsl, "//MAD\n");
#endif
        CallTernaryOp(psContext, "*", "+", psInst, 0, 1, 2, 3, TO_FLAG_FLOAT);
        break;
    }
    case OPCODE_IMAD:
    {
        uint32_t ui32Flags = TO_FLAG_INTEGER;
#ifdef _DEBUG
        AddIndentation(psContext);
        bcatcstr(glsl, "//IMAD\n");
#endif

        if (GetOperandDataType(psContext, &psInst->asOperands[0]) == SVT_UINT)
        {
            ui32Flags = TO_FLAG_UNSIGNED_INTEGER;
        }

        CallTernaryOp(psContext, "*", "+", psInst, 0, 1, 2, 3, ui32Flags);
        break;
    }
    case OPCODE_DADD:
    {
#ifdef _DEBUG
        AddIndentation(psContext);
        bcatcstr(glsl, "//DADD\n");
#endif
        CallBinaryOp(psContext, "+", psInst, 0, 1, 2, TO_FLAG_DOUBLE);
        break;
    }
    case OPCODE_IADD:
    {
        uint32_t ui32Flags = TO_FLAG_INTEGER;
#ifdef _DEBUG
        AddIndentation(psContext);
        bcatcstr(glsl, "//IADD\n");
#endif
        //Is this a signed or unsigned add?
        if (GetOperandDataType(psContext, &psInst->asOperands[0]) == SVT_UINT)
        {
            ui32Flags = TO_FLAG_UNSIGNED_INTEGER;
        }
        CallBinaryOp(psContext, "+", psInst, 0, 1, 2, ui32Flags);
        break;
    }
    case OPCODE_ADD:
    {
#ifdef _DEBUG
        AddIndentation(psContext);
        bcatcstr(glsl, "//ADD\n");
#endif
        CallBinaryOp(psContext, "+", psInst, 0, 1, 2, TO_FLAG_FLOAT);
        break;
    }
    case OPCODE_OR:
    {
        /*Todo: vector version */
#ifdef _DEBUG
        AddIndentation(psContext);
        bcatcstr(glsl, "//OR\n");
#endif
        CallBinaryOp(psContext, "|", psInst, 0, 1, 2, TO_FLAG_INTEGER);
        break;
    }
    case OPCODE_AND:
    {
#ifdef _DEBUG
        AddIndentation(psContext);
        bcatcstr(glsl, "//AND\n");
#endif
        CallBinaryOp(psContext, "&", psInst, 0, 1, 2, TO_FLAG_INTEGER);
        break;
    }
    case OPCODE_GE:
    {
        /*
            dest = vec4(greaterThanEqual(vec4(srcA), vec4(srcB));
            Caveat: The result is a boolean but HLSL asm returns 0xFFFFFFFF/0x0 instead.
         */
#ifdef _DEBUG
        AddIndentation(psContext);
        bcatcstr(glsl, "//GE\n");
#endif
        AddComparision(psContext, psInst, CMP_GE, TO_FLAG_FLOAT);
        break;
    }
    case OPCODE_MUL:
    {
#ifdef _DEBUG
        AddIndentation(psContext);
        bcatcstr(glsl, "//MUL\n");
#endif
        CallBinaryOp(psContext, "*", psInst, 0, 1, 2, TO_FLAG_FLOAT);
        break;
    }
    case OPCODE_IMUL:
    {
        uint32_t ui32Flags = TO_FLAG_INTEGER;
#ifdef _DEBUG
        AddIndentation(psContext);
        bcatcstr(glsl, "//IMUL\n");
#endif
        if (GetOperandDataType(psContext, &psInst->asOperands[1]) == SVT_UINT)
        {
            ui32Flags = TO_FLAG_UNSIGNED_INTEGER;
        }

        ASSERT(psInst->asOperands[0].eType == OPERAND_TYPE_NULL);

        CallBinaryOp(psContext, "*", psInst, 1, 2, 3, ui32Flags);
        break;
    }
    case OPCODE_UDIV:
    {
#ifdef _DEBUG
        AddIndentation(psContext);
        bcatcstr(glsl, "//UDIV\n");
#endif
        //destQuotient, destRemainder, src0, src1
        CallBinaryOp(psContext, "/", psInst, 0, 2, 3, TO_FLAG_UNSIGNED_INTEGER);
        CallBinaryOp(psContext, "%", psInst, 1, 2, 3, TO_FLAG_UNSIGNED_INTEGER);
        break;
    }
    case OPCODE_DIV:
    {
#ifdef _DEBUG
        AddIndentation(psContext);
        bcatcstr(glsl, "//DIV\n");
#endif
        CallBinaryOp(psContext, "/", psInst, 0, 1, 2, TO_FLAG_FLOAT);
        break;
    }
    case OPCODE_SINCOS:
    {
#ifdef _DEBUG
        AddIndentation(psContext);
        bcatcstr(glsl, "//SINCOS\n");
#endif
        if (psInst->asOperands[0].eType != OPERAND_TYPE_NULL)
        {
            CallHelper1(psContext, "sin", psInst, 0, 2);
        }

        if (psInst->asOperands[1].eType != OPERAND_TYPE_NULL)
        {
            CallHelper1(psContext, "cos", psInst, 1, 2);
        }
        break;
    }

    case OPCODE_DP2:
    {
#ifdef _DEBUG
        AddIndentation(psContext);
        bcatcstr(glsl, "//DP2\n");
#endif
        AddIndentation(psContext);
        BeginAssignment(psContext, &psInst->asOperands[0], TO_FLAG_FLOAT, psInst->bSaturate);
        bcatcstr(glsl, "vec4(dot((");
        TranslateOperand(psContext, &psInst->asOperands[1], TO_FLAG_FLOAT);
        bcatcstr(glsl, ").xy, (");
        TranslateOperand(psContext, &psInst->asOperands[2], TO_FLAG_FLOAT);
        bcatcstr(glsl, ").xy))");
        TranslateOperandSwizzle(psContext, &psInst->asOperands[0]);
        EndAssignment(psContext, &psInst->asOperands[0], TO_FLAG_FLOAT, psInst->bSaturate);
        bcatcstr(glsl, ";\n");
        break;
    }
    case OPCODE_DP3:
    {
#ifdef _DEBUG
        AddIndentation(psContext);
        bcatcstr(glsl, "//DP3\n");
#endif
        AddIndentation(psContext);
        BeginAssignment(psContext, &psInst->asOperands[0], TO_FLAG_FLOAT, psInst->bSaturate);
        bcatcstr(glsl, "vec4(dot((");
        TranslateOperand(psContext, &psInst->asOperands[1], TO_FLAG_FLOAT);
        bcatcstr(glsl, ").xyz, (");
        TranslateOperand(psContext, &psInst->asOperands[2], TO_FLAG_FLOAT);
        bcatcstr(glsl, ").xyz))");
        TranslateOperandSwizzle(psContext, &psInst->asOperands[0]);
        EndAssignment(psContext, &psInst->asOperands[0], TO_FLAG_FLOAT, psInst->bSaturate);
        bcatcstr(glsl, ";\n");
        break;
    }
    case OPCODE_DP4:
    {
#ifdef _DEBUG
        AddIndentation(psContext);
        bcatcstr(glsl, "//DP4\n");
#endif
        CallHelper2(psContext, "dot", psInst, 0, 1, 2);
        break;
    }
    case OPCODE_INE:
    {
#ifdef _DEBUG
        AddIndentation(psContext);
        bcatcstr(glsl, "//INE\n");
#endif
        AddComparision(psContext, psInst, CMP_NE, TO_FLAG_INTEGER);
        break;
    }
    case OPCODE_NE:
    {
#ifdef _DEBUG
        AddIndentation(psContext);
        bcatcstr(glsl, "//NE\n");
#endif
        AddComparision(psContext, psInst, CMP_NE, TO_FLAG_FLOAT);
        break;
    }
    case OPCODE_IGE:
    {
#ifdef _DEBUG
        AddIndentation(psContext);
        bcatcstr(glsl, "//IGE\n");
#endif
        AddComparision(psContext, psInst, CMP_GE, TO_FLAG_INTEGER);
        break;
    }
    case OPCODE_ILT:
    {
#ifdef _DEBUG
        AddIndentation(psContext);
        bcatcstr(glsl, "//ILT\n");
#endif
        AddComparision(psContext, psInst, CMP_LT, TO_FLAG_INTEGER);
        break;
    }
    case OPCODE_LT:
    {
#ifdef _DEBUG
        AddIndentation(psContext);
        bcatcstr(glsl, "//LT\n");
#endif
        AddComparision(psContext, psInst, CMP_LT, TO_FLAG_FLOAT);
        break;
    }
    case OPCODE_IEQ:
    {
#ifdef _DEBUG
        AddIndentation(psContext);
        bcatcstr(glsl, "//IEQ\n");
#endif
        AddComparision(psContext, psInst, CMP_EQ, TO_FLAG_INTEGER);
        break;
    }
    case OPCODE_ULT:
    {
#ifdef _DEBUG
        AddIndentation(psContext);
        bcatcstr(glsl, "//ULT\n");
#endif
        AddComparision(psContext, psInst, CMP_LT, TO_FLAG_UNSIGNED_INTEGER);
        break;
    }
    case OPCODE_UGE:
    {
#ifdef _DEBUG
        AddIndentation(psContext);
        bcatcstr(glsl, "//UGE\n");
#endif
        AddComparision(psContext, psInst, CMP_GE, TO_FLAG_UNSIGNED_INTEGER);
        break;
    }
    case OPCODE_MOVC:
    {
#ifdef _DEBUG
        AddIndentation(psContext);
        bcatcstr(glsl, "//MOVC\n");
#endif
        AddMOVCBinaryOp(psContext, &psInst->asOperands[0], 0, &psInst->asOperands[1], &psInst->asOperands[2], &psInst->asOperands[3]);
        break;
    }
    case OPCODE_SWAPC:
    {
#ifdef _DEBUG
        AddIndentation(psContext);
        bcatcstr(glsl, "//SWAPC\n");
#endif
        AddMOVCBinaryOp(psContext, &psInst->asOperands[0], 1, &psInst->asOperands[2], &psInst->asOperands[4], &psInst->asOperands[3]);
        AddMOVCBinaryOp(psContext, &psInst->asOperands[1], 0, &psInst->asOperands[2], &psInst->asOperands[3], &psInst->asOperands[4]);
        AddMOVBinaryOp(psContext, &psInst->asOperands[0], &psInst->asOperands[0], 1, 0);
        break;
    }

    case OPCODE_LOG:
    {
#ifdef _DEBUG
        AddIndentation(psContext);
        bcatcstr(glsl, "//LOG\n");
#endif
        CallHelper1(psContext, "log2", psInst, 0, 1);
        break;
    }
    case OPCODE_RSQ:
    {
#ifdef _DEBUG
        AddIndentation(psContext);
        bcatcstr(glsl, "//RSQ\n");
#endif
        CallHelper1(psContext, "inversesqrt", psInst, 0, 1);
        break;
    }
    case OPCODE_EXP:
    {
#ifdef _DEBUG
        AddIndentation(psContext);
        bcatcstr(glsl, "//EXP\n");
#endif
        CallHelper1(psContext, "exp2", psInst, 0, 1);
        break;
    }
    case OPCODE_SQRT:
    {
#ifdef _DEBUG
        AddIndentation(psContext);
        bcatcstr(glsl, "//SQRT\n");
#endif
        CallHelper1(psContext, "sqrt", psInst, 0, 1);
        break;
    }
    case OPCODE_ROUND_PI:
    {
#ifdef _DEBUG
        AddIndentation(psContext);
        bcatcstr(glsl, "//ROUND_PI\n");
#endif
        CallHelper1(psContext, "ceil", psInst, 0, 1);
        break;
    }
    case OPCODE_ROUND_NI:
    {
#ifdef _DEBUG
        AddIndentation(psContext);
        bcatcstr(glsl, "//ROUND_NI\n");
#endif
        CallHelper1(psContext, "floor", psInst, 0, 1);
        break;
    }
    case OPCODE_ROUND_Z:
    {
#ifdef _DEBUG
        AddIndentation(psContext);
        bcatcstr(glsl, "//ROUND_Z\n");
#endif
        CallHelper1(psContext, "trunc", psInst, 0, 1);
        break;
    }
    case OPCODE_ROUND_NE:
    {
#ifdef _DEBUG
        AddIndentation(psContext);
        bcatcstr(glsl, "//ROUND_NE\n");
#endif
        CallHelper1(psContext, "roundEven", psInst, 0, 1);
        break;
    }
    case OPCODE_FRC:
    {
#ifdef _DEBUG
        AddIndentation(psContext);
        bcatcstr(glsl, "//FRC\n");
#endif
        CallHelper1(psContext, "fract", psInst, 0, 1);
        break;
    }
    case OPCODE_IMAX:
    {
#ifdef _DEBUG
        AddIndentation(psContext);
        bcatcstr(glsl, "//IMAX\n");
#endif
        CallHelper2Int(psContext, "max", psInst, 0, 1, 2);
        break;
    }
    case OPCODE_UMAX:
    {
#ifdef _DEBUG
        AddIndentation(psContext);
        bcatcstr(glsl, "//UMAX\n");
#endif
        CallHelper2UInt(psContext, "max", psInst, 0, 1, 2);
        break;
    }
    case OPCODE_MAX:
    {
#ifdef _DEBUG
        AddIndentation(psContext);
        bcatcstr(glsl, "//MAX\n");
#endif
        CallHelper2(psContext, "max", psInst, 0, 1, 2);
        break;
    }
    case OPCODE_IMIN:
    {
#ifdef _DEBUG
        AddIndentation(psContext);
        bcatcstr(glsl, "//IMIN\n");
#endif
        CallHelper2Int(psContext, "min", psInst, 0, 1, 2);
        break;
    }
    case OPCODE_UMIN:
    {
#ifdef _DEBUG
        AddIndentation(psContext);
        bcatcstr(glsl, "//UMIN\n");
#endif
        CallHelper2UInt(psContext, "min", psInst, 0, 1, 2);
        break;
    }
    case OPCODE_MIN:
    {
#ifdef _DEBUG
        AddIndentation(psContext);
        bcatcstr(glsl, "//MIN\n");
#endif
        CallHelper2(psContext, "min", psInst, 0, 1, 2);
        break;
    }
    case OPCODE_GATHER4:
    {
        //dest, coords, tex, sampler
        const RESOURCE_DIMENSION eResDim = psContext->psShader->aeResourceDims[psInst->asOperands[2].ui32RegisterNumber];
        const uint32_t ui32SampleToFlags = GetResourceReturnTypeToFlags(RGROUP_TEXTURE, psInst->asOperands[2].ui32RegisterNumber, psContext);
#ifdef _DEBUG
        AddIndentation(psContext);
        bcatcstr(glsl, "//GATHER4\n");
#endif
        //gather4 r7.xyzw, r3.xyxx, t3.xyzw, s0.x
        AddIndentation(psContext);
        BeginAssignment(psContext, &psInst->asOperands[0], ui32SampleToFlags, psInst->bSaturate);
        bcatcstr(glsl, "(textureGather(");

        TextureName(*psContext->currentGLSLString, psContext->psShader, psInst->asOperands[2].ui32RegisterNumber, psInst->asOperands[3].ui32RegisterNumber, 0);
        bcatcstr(glsl, ", ");
        TranslateTexCoord(psContext, eResDim, &psInst->asOperands[1]);
        bcatcstr(glsl, ")");
        // iWriteMaskEnabled is forced off during DecodeOperand because swizzle on sampler uniforms
        // does not make sense. But need to re-enable to correctly swizzle this particular instruction.
        psInst->asOperands[2].iWriteMaskEnabled = 1;
        TranslateOperandSwizzle(psContext, &psInst->asOperands[2]);
        bcatcstr(glsl, ")");

        TranslateOperandSwizzle(psContext, &psInst->asOperands[0]);
        EndAssignment(psContext, &psInst->asOperands[0], ui32SampleToFlags, psInst->bSaturate);
        bcatcstr(glsl, ";\n");
        break;
    }
    case OPCODE_GATHER4_PO_C:
    {
        //dest, coords, offset, tex, sampler, srcReferenceValue
        const RESOURCE_DIMENSION eResDim = psContext->psShader->aeResourceDims[psInst->asOperands[3].ui32RegisterNumber];
#ifdef _DEBUG
        AddIndentation(psContext);
        bcatcstr(glsl, "//GATHER4_PO_C\n");
#endif

        AddIndentation(psContext);
        BeginAssignment(psContext, &psInst->asOperands[0], TO_FLAG_FLOAT, psInst->bSaturate);
        bcatcstr(glsl, "(textureGatherOffset(");

        TextureName(*psContext->currentGLSLString, psContext->psShader, psInst->asOperands[3].ui32RegisterNumber, psInst->asOperands[4].ui32RegisterNumber, 1);

        bcatcstr(glsl, ", ");

        TranslateTexCoord(psContext, eResDim, &psInst->asOperands[1]);

        bcatcstr(glsl, ", ");
        TranslateOperand(psContext, &psInst->asOperands[5], TO_FLAG_NONE);

        bcatcstr(glsl, ", ivec2(");
        //ivec2 offset
        psInst->asOperands[2].aui32Swizzle[2] = 0xFFFFFFFF;
        psInst->asOperands[2].aui32Swizzle[3] = 0xFFFFFFFF;
        TranslateOperand(psContext, &psInst->asOperands[2], TO_FLAG_NONE);
        bcatcstr(glsl, "))");
        // iWriteMaskEnabled is forced off during DecodeOperand because swizzle on sampler uniforms
        // does not make sense. But need to re-enable to correctly swizzle this particular instruction.
        psInst->asOperands[2].iWriteMaskEnabled = 1;
        TranslateOperandSwizzle(psContext, &psInst->asOperands[3]);
        bcatcstr(glsl, ")");

        TranslateOperandSwizzle(psContext, &psInst->asOperands[0]);
        EndAssignment(psContext, &psInst->asOperands[0], TO_FLAG_FLOAT, psInst->bSaturate);
        bcatcstr(glsl, ";\n");
        break;
    }
    case OPCODE_GATHER4_PO:
    {
        //dest, coords, offset, tex, sampler
        const uint32_t ui32SampleToFlags = GetResourceReturnTypeToFlags(RGROUP_TEXTURE, psInst->asOperands[3].ui32RegisterNumber, psContext);
#ifdef _DEBUG
        AddIndentation(psContext);
        bcatcstr(glsl, "//GATHER4_PO\n");
#endif

        AddIndentation(psContext);
        BeginAssignment(psContext, &psInst->asOperands[0], ui32SampleToFlags, psInst->bSaturate);
        bcatcstr(glsl, "(textureGatherOffset(");

        TextureName(*psContext->currentGLSLString, psContext->psShader, psInst->asOperands[3].ui32RegisterNumber, psInst->asOperands[4].ui32RegisterNumber, 0);

        bcatcstr(glsl, ", ");
        //Texture coord cannot be vec4
        //Determining if it is a vec3 for vec2 yet to be done.
        psInst->asOperands[1].aui32Swizzle[2] = 0xFFFFFFFF;
        psInst->asOperands[1].aui32Swizzle[3] = 0xFFFFFFFF;
        TranslateOperand(psContext, &psInst->asOperands[1], TO_FLAG_NONE);

        bcatcstr(glsl, ", ivec2(");
        //ivec2 offset
        psInst->asOperands[2].aui32Swizzle[2] = 0xFFFFFFFF;
        psInst->asOperands[2].aui32Swizzle[3] = 0xFFFFFFFF;
        TranslateOperand(psContext, &psInst->asOperands[2], TO_FLAG_NONE);
        bcatcstr(glsl, "))");
        // iWriteMaskEnabled is forced off during DecodeOperand because swizzle on sampler uniforms
        // does not make sense. But need to re-enable to correctly swizzle this particular instruction.
        psInst->asOperands[2].iWriteMaskEnabled = 1;
        TranslateOperandSwizzle(psContext, &psInst->asOperands[3]);
        bcatcstr(glsl, ")");

        TranslateOperandSwizzle(psContext, &psInst->asOperands[0]);
        EndAssignment(psContext, &psInst->asOperands[0], ui32SampleToFlags, psInst->bSaturate);
        bcatcstr(glsl, ";\n");
        break;
    }
    case OPCODE_GATHER4_C:
    {
        //dest, coords, tex, sampler srcReferenceValue
#ifdef _DEBUG
        AddIndentation(psContext);
        bcatcstr(glsl, "//GATHER4_C\n");
#endif

        AddIndentation(psContext);
        BeginAssignment(psContext, &psInst->asOperands[0], TO_FLAG_FLOAT, psInst->bSaturate);
        bcatcstr(glsl, "(textureGather(");

        TextureName(*psContext->currentGLSLString, psContext->psShader, psInst->asOperands[2].ui32RegisterNumber, psInst->asOperands[3].ui32RegisterNumber, 1);

        bcatcstr(glsl, ", ");
        //Texture coord cannot be vec4
        //Determining if it is a vec3 for vec2 yet to be done.
        psInst->asOperands[1].aui32Swizzle[2] = 0xFFFFFFFF;
        psInst->asOperands[1].aui32Swizzle[3] = 0xFFFFFFFF;
        TranslateOperand(psContext, &psInst->asOperands[1], TO_FLAG_NONE);

        bcatcstr(glsl, ", ");
        TranslateOperand(psContext, &psInst->asOperands[4], TO_FLAG_NONE);
        bcatcstr(glsl, ")");
        // iWriteMaskEnabled is forced off during DecodeOperand because swizzle on sampler uniforms
        // does not make sense. But need to re-enable to correctly swizzle this particular instruction.
        psInst->asOperands[2].iWriteMaskEnabled = 1;
        TranslateOperandSwizzle(psContext, &psInst->asOperands[2]);
        bcatcstr(glsl, ")");

        TranslateOperandSwizzle(psContext, &psInst->asOperands[0]);
        EndAssignment(psContext, &psInst->asOperands[0], TO_FLAG_FLOAT, psInst->bSaturate);
        bcatcstr(glsl, ";\n");
        break;
    }
    case OPCODE_SAMPLE:
    {
#ifdef _DEBUG
        AddIndentation(psContext);
        bcatcstr(glsl, "//SAMPLE\n");
#endif
        TranslateTextureSample(psContext, psInst, TEXSMP_FLAG_NONE);
        break;
    }
    case OPCODE_SAMPLE_L:
    {
#ifdef _DEBUG
        AddIndentation(psContext);
        bcatcstr(glsl, "//SAMPLE_L\n");
#endif
        TranslateTextureSample(psContext, psInst, TEXSMP_FLAG_LOD);
        break;
    }
    case OPCODE_SAMPLE_C:
    {
#ifdef _DEBUG
        AddIndentation(psContext);
        bcatcstr(glsl, "//SAMPLE_C\n");
#endif

        TranslateTextureSample(psContext, psInst, TEXSMP_FLAG_COMPARE);
        break;
    }
    case OPCODE_SAMPLE_C_LZ:
    {
#ifdef _DEBUG
        AddIndentation(psContext);
        bcatcstr(glsl, "//SAMPLE_C_LZ\n");
#endif

        TranslateTextureSample(psContext, psInst, TEXSMP_FLAG_COMPARE | TEXSMP_FLAG_FIRSTLOD);
        break;
    }
    case OPCODE_SAMPLE_D:
    {
#ifdef _DEBUG
        AddIndentation(psContext);
        bcatcstr(glsl, "//SAMPLE_D\n");
#endif

        TranslateTextureSample(psContext, psInst, TEXSMP_FLAGS_GRAD);
        break;
    }
    case OPCODE_SAMPLE_B:
    {
#ifdef _DEBUG
        AddIndentation(psContext);
        bcatcstr(glsl, "//SAMPLE_B\n");
#endif

        TranslateTextureSample(psContext, psInst, TEXSMP_FLAG_BIAS);
        break;
    }
    case OPCODE_RET:
    {
#ifdef _DEBUG
        AddIndentation(psContext);
        bcatcstr(glsl, "//RET\n");
#endif
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
        if (psContext->flags & HLSLCC_FLAG_TRACING_INSTRUMENTATION)
        {
            WriteEndTrace(psContext);
        }
        AddIndentation(psContext);
        bcatcstr(glsl, "return;\n");
        break;
    }
    case OPCODE_INTERFACE_CALL:
    {
        const char* name;
        ShaderVar* psVar;
        uint32_t varFound;

        uint32_t funcPointer;
        uint32_t funcTableIndex;
        uint32_t funcTable;
        uint32_t funcBodyIndex;
        uint32_t funcBody;
        uint32_t ui32NumBodiesPerTable;

#ifdef _DEBUG
        AddIndentation(psContext);
        bcatcstr(glsl, "//INTERFACE_CALL\n");
#endif

        ASSERT(psInst->asOperands[0].eIndexRep[0] == OPERAND_INDEX_IMMEDIATE32);

        funcPointer = psInst->asOperands[0].aui32ArraySizes[0];
        funcTableIndex = psInst->asOperands[0].aui32ArraySizes[1];
        funcBodyIndex = psInst->ui32FuncIndexWithinInterface;

        ui32NumBodiesPerTable = psContext->psShader->funcPointer[funcPointer].ui32NumBodiesPerTable;

        funcTable = psContext->psShader->funcPointer[funcPointer].aui32FuncTables[funcTableIndex];

        funcBody = psContext->psShader->funcTable[funcTable].aui32FuncBodies[funcBodyIndex];

        varFound = GetInterfaceVarFromOffset(funcPointer, &psContext->psShader->sInfo, &psVar);

        ASSERT(varFound);

        name = &psVar->sType.Name[0];

        AddIndentation(psContext);
        bcatcstr(glsl, name);
        TranslateOperandIndexMAD(psContext, &psInst->asOperands[0], 1, ui32NumBodiesPerTable, funcBodyIndex);
        //bformata(glsl, "[%d]", funcBodyIndex);
        bcatcstr(glsl, "();\n");
        break;
    }
    case OPCODE_LABEL:
    {
#ifdef _DEBUG
        AddIndentation(psContext);
        bcatcstr(glsl, "//LABEL\n");
#endif
        --psContext->indent;
        AddIndentation(psContext);
        bcatcstr(glsl, "}\n");     //Closing brace ends the previous function.
        AddIndentation(psContext);

        bcatcstr(glsl, "subroutine(SubroutineType)\n");
        bcatcstr(glsl, "void ");
        TranslateOperand(psContext, &psInst->asOperands[0], TO_FLAG_DESTINATION);
        bcatcstr(glsl, "(){\n");
        ++psContext->indent;
        break;
    }
    case OPCODE_COUNTBITS:
    {
#ifdef _DEBUG
        AddIndentation(psContext);
        bcatcstr(glsl, "//COUNTBITS\n");
#endif
        AddIndentation(psContext);
        BeginAssignment(psContext, &psInst->asOperands[0], TO_FLAG_INTEGER, psInst->bSaturate);
        bcatcstr(glsl, "bitCount(");
        TranslateOperand(psContext, &psInst->asOperands[1], TO_FLAG_INTEGER);
        bcatcstr(glsl, ")");
        EndAssignment(psContext, &psInst->asOperands[0], TO_FLAG_INTEGER, psInst->bSaturate);
        bcatcstr(glsl, ";\n");
        break;
    }
    case OPCODE_FIRSTBIT_HI:
    {
#ifdef _DEBUG
        AddIndentation(psContext);
        bcatcstr(glsl, "//FIRSTBIT_HI\n");
#endif
        AddIndentation(psContext);
        BeginAssignment(psContext, &psInst->asOperands[0], TO_FLAG_UNSIGNED_INTEGER, psInst->bSaturate);
        bcatcstr(glsl, "findMSB(");
        TranslateOperand(psContext, &psInst->asOperands[1], TO_FLAG_UNSIGNED_INTEGER);
        bcatcstr(glsl, ")");
        EndAssignment(psContext, &psInst->asOperands[0], TO_FLAG_UNSIGNED_INTEGER, psInst->bSaturate);
        bcatcstr(glsl, ";\n");
        break;
    }
    case OPCODE_FIRSTBIT_LO:
    {
#ifdef _DEBUG
        AddIndentation(psContext);
        bcatcstr(glsl, "//FIRSTBIT_LO\n");
#endif
        AddIndentation(psContext);
        BeginAssignment(psContext, &psInst->asOperands[0], TO_FLAG_UNSIGNED_INTEGER, psInst->bSaturate);
        bcatcstr(glsl, "findLSB(");
        TranslateOperand(psContext, &psInst->asOperands[1], TO_FLAG_UNSIGNED_INTEGER);
        bcatcstr(glsl, ")");
        EndAssignment(psContext, &psInst->asOperands[0], TO_FLAG_UNSIGNED_INTEGER, psInst->bSaturate);
        bcatcstr(glsl, ";\n");
        break;
    }
    case OPCODE_FIRSTBIT_SHI:     //signed high
    {
#ifdef _DEBUG
        AddIndentation(psContext);
        bcatcstr(glsl, "//FIRSTBIT_SHI\n");
#endif
        AddIndentation(psContext);
        BeginAssignment(psContext, &psInst->asOperands[0], TO_FLAG_INTEGER, psInst->bSaturate);
        bcatcstr(glsl, "findMSB(");
        TranslateOperand(psContext, &psInst->asOperands[1], TO_FLAG_INTEGER);
        bcatcstr(glsl, ")");
        EndAssignment(psContext, &psInst->asOperands[0], TO_FLAG_INTEGER, psInst->bSaturate);
        bcatcstr(glsl, ";\n");
        break;
    }
    case OPCODE_BFREV:
    {
#ifdef _DEBUG
        AddIndentation(psContext);
        bcatcstr(glsl, "//BFREV\n");
#endif
        AddIndentation(psContext);
        BeginAssignment(psContext, &psInst->asOperands[0], TO_FLAG_INTEGER, psInst->bSaturate);
        bcatcstr(glsl, "bitfieldReverse(");
        TranslateOperand(psContext, &psInst->asOperands[1], TO_FLAG_INTEGER);
        bcatcstr(glsl, ")");
        EndAssignment(psContext, &psInst->asOperands[0], TO_FLAG_INTEGER, psInst->bSaturate);
        bcatcstr(glsl, ";\n");
        break;
    }
    case OPCODE_BFI:
    {
        uint32_t numelements_width = GetNumSwizzleElements(&psInst->asOperands[1]);
        uint32_t numelements_offset = GetNumSwizzleElements(&psInst->asOperands[2]);
        uint32_t numelements_dest = GetNumSwizzleElements(&psInst->asOperands[0]);
        uint32_t numoverall_elements = min(min(numelements_width, numelements_offset), numelements_dest);
        uint32_t i, j;
        static const char* bfi_elementidx[] = { "x", "y", "z", "w" };
#ifdef _DEBUG
        AddIndentation(psContext);
        bcatcstr(glsl, "//BFI\n");
#endif

        AddIndentation(psContext);
        BeginAssignment(psContext, &psInst->asOperands[0], TO_FLAG_INTEGER, psInst->bSaturate);
        bformata(glsl, "ivec%d(", numoverall_elements);
        for (i = 0; i < numoverall_elements; ++i)
        {
            bcatcstr(glsl, "bitfieldInsert(");

            for (j = 4; j >= 1; --j)
            {
                uint32_t opSwizzleCount = GetNumSwizzleElements(&psInst->asOperands[j]);

                if (opSwizzleCount != 1)
                {
                    bcatcstr(glsl, " (");
                }
                TranslateOperand(psContext, &psInst->asOperands[j], TO_FLAG_INTEGER);
                if (opSwizzleCount != 1)
                {
                    bformata(glsl, " ).%s", bfi_elementidx[i]);
                }
                if (j != 1)
                {
                    bcatcstr(glsl, ",");
                }
            }

            bcatcstr(glsl, ") ");
            if (i + 1 != numoverall_elements)
            {
                bcatcstr(glsl, ", ");
            }
        }

        bcatcstr(glsl, ").");
        for (i = 0; i < numoverall_elements; ++i)
        {
            bformata(glsl, "%s", bfi_elementidx[i]);
        }
        EndAssignment(psContext, &psInst->asOperands[0], TO_FLAG_INTEGER, psInst->bSaturate);
        bcatcstr(glsl, ";\n");
        break;
    }
    case OPCODE_CUT:
    {
#ifdef _DEBUG
        AddIndentation(psContext);
        bcatcstr(glsl, "//CUT\n");
#endif
        AddIndentation(psContext);
        bcatcstr(glsl, "EndPrimitive();\n");
        break;
    }
    case OPCODE_EMIT:
    {
#ifdef _DEBUG
        AddIndentation(psContext);
        bcatcstr(glsl, "//EMIT\n");
#endif
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
            AddIndentation(psContext);
        }

        AddIndentation(psContext);
        bcatcstr(glsl, "EmitVertex();\n");
        break;
    }
    case OPCODE_EMITTHENCUT:
    {
#ifdef _DEBUG
        AddIndentation(psContext);
        bcatcstr(glsl, "//EMITTHENCUT\n");
#endif
        AddIndentation(psContext);
        bcatcstr(glsl, "EmitVertex();\nEndPrimitive();\n");
        break;
    }

    case OPCODE_CUT_STREAM:
    {
#ifdef _DEBUG
        AddIndentation(psContext);
        bcatcstr(glsl, "//CUT\n");
#endif
        AddIndentation(psContext);
        bcatcstr(glsl, "EndStreamPrimitive(");
        TranslateOperand(psContext, &psInst->asOperands[0], TO_FLAG_DESTINATION);
        bcatcstr(glsl, ");\n");

        break;
    }
    case OPCODE_EMIT_STREAM:
    {
#ifdef _DEBUG
        AddIndentation(psContext);
        bcatcstr(glsl, "//EMIT\n");
#endif
        AddIndentation(psContext);
        bcatcstr(glsl, "EmitStreamVertex(");
        TranslateOperand(psContext, &psInst->asOperands[0], TO_FLAG_DESTINATION);
        bcatcstr(glsl, ");\n");
        break;
    }
    case OPCODE_EMITTHENCUT_STREAM:
    {
#ifdef _DEBUG
        AddIndentation(psContext);
        bcatcstr(glsl, "//EMITTHENCUT\n");
#endif
        AddIndentation(psContext);
        bcatcstr(glsl, "EmitStreamVertex(");
        TranslateOperand(psContext, &psInst->asOperands[0], TO_FLAG_DESTINATION);
        bcatcstr(glsl, ");\n");
        bcatcstr(glsl, "EndStreamPrimitive(");
        TranslateOperand(psContext, &psInst->asOperands[0], TO_FLAG_DESTINATION);
        bcatcstr(glsl, ");\n");
        break;
    }
    case OPCODE_REP:
    {
#ifdef _DEBUG
        AddIndentation(psContext);
        bcatcstr(glsl, "//REP\n");
#endif
        //Need to handle nesting.
        //Max of 4 for rep - 'Flow Control Limitations' http://msdn.microsoft.com/en-us/library/windows/desktop/bb219848(v=vs.85).aspx

        AddIndentation(psContext);
        bcatcstr(glsl, "RepCounter = ivec4(");
        TranslateOperand(psContext, &psInst->asOperands[0], TO_FLAG_NONE);
        bcatcstr(glsl, ").x;\n");

        AddIndentation(psContext);
        bcatcstr(glsl, "while(RepCounter!=0){\n");
        ++psContext->indent;
        break;
    }
    case OPCODE_ENDREP:
    {
#ifdef _DEBUG
        AddIndentation(psContext);
        bcatcstr(glsl, "//ENDREP\n");
#endif
        AddIndentation(psContext);
        bcatcstr(glsl, "RepCounter--;\n");

        --psContext->indent;

        AddIndentation(psContext);
        bcatcstr(glsl, "}\n");
        break;
    }
    case OPCODE_LOOP:
    {
#ifdef _DEBUG
        AddIndentation(psContext);
        bcatcstr(glsl, "//LOOP\n");
#endif
        AddIndentation(psContext);

        if (psInst->ui32NumOperands == 2)
        {
            //DX9 version
            ASSERT(psInst->asOperands[0].eType == OPERAND_TYPE_SPECIAL_LOOPCOUNTER);
            bcatcstr(glsl, "for(");
            bcatcstr(glsl, "LoopCounter = ");
            TranslateOperand(psContext, &psInst->asOperands[1], TO_FLAG_NONE);
            bcatcstr(glsl, ".y, ZeroBasedCounter = 0;");
            bcatcstr(glsl, "ZeroBasedCounter < ");
            TranslateOperand(psContext, &psInst->asOperands[1], TO_FLAG_NONE);
            bcatcstr(glsl, ".x;");

            bcatcstr(glsl, "LoopCounter += ");
            TranslateOperand(psContext, &psInst->asOperands[1], TO_FLAG_NONE);
            bcatcstr(glsl, ".z, ZeroBasedCounter++){\n");
            ++psContext->indent;
        }
        else
        {
            bcatcstr(glsl, "while(true){\n");
            ++psContext->indent;
        }
        break;
    }
    case OPCODE_ENDLOOP:
    {
        --psContext->indent;
#ifdef _DEBUG
        AddIndentation(psContext);
        bcatcstr(glsl, "//ENDLOOP\n");
#endif
        AddIndentation(psContext);
        bcatcstr(glsl, "}\n");
        break;
    }
    case OPCODE_BREAK:
    {
#ifdef _DEBUG
        AddIndentation(psContext);
        bcatcstr(glsl, "//BREAK\n");
#endif
        AddIndentation(psContext);
        bcatcstr(glsl, "break;\n");
        break;
    }
    case OPCODE_BREAKC:
    {
#ifdef _DEBUG
        AddIndentation(psContext);
        bcatcstr(glsl, "//BREAKC\n");
#endif
        AddIndentation(psContext);

        TranslateConditional(psContext, psInst, glsl);
        break;
    }
    case OPCODE_CONTINUEC:
    {
#ifdef _DEBUG
        AddIndentation(psContext);
        bcatcstr(glsl, "//CONTINUEC\n");
#endif
        AddIndentation(psContext);

        TranslateConditional(psContext, psInst, glsl);
        break;
    }
    case OPCODE_IF:
    {
#ifdef _DEBUG
        AddIndentation(psContext);
        bcatcstr(glsl, "//IF\n");
#endif
        AddIndentation(psContext);

        TranslateConditional(psContext, psInst, glsl);
        ++psContext->indent;
        break;
    }
    case OPCODE_RETC:
    {
#ifdef _DEBUG
        AddIndentation(psContext);
        bcatcstr(glsl, "//RETC\n");
#endif
        AddIndentation(psContext);

        TranslateConditional(psContext, psInst, glsl);
        break;
    }
    case OPCODE_ELSE:
    {
        --psContext->indent;
#ifdef _DEBUG
        AddIndentation(psContext);
        bcatcstr(glsl, "//ELSE\n");
#endif
        AddIndentation(psContext);
        bcatcstr(glsl, "} else {\n");
        psContext->indent++;
        break;
    }
    case OPCODE_ENDSWITCH:
    case OPCODE_ENDIF:
    {
        --psContext->indent;
        AddIndentation(psContext);
        bcatcstr(glsl, "//ENDIF\n");
        AddIndentation(psContext);
        bcatcstr(glsl, "}\n");
        break;
    }
    case OPCODE_CONTINUE:
    {
        AddIndentation(psContext);
        bcatcstr(glsl, "continue;\n");
        break;
    }
    case OPCODE_DEFAULT:
    {
        --psContext->indent;
        AddIndentation(psContext);
        bcatcstr(glsl, "default:\n");
        ++psContext->indent;
        break;
    }
    case OPCODE_NOP:
    {
        break;
    }
    case OPCODE_SYNC:
    {
        const uint32_t ui32SyncFlags = psInst->ui32SyncFlags;

#ifdef _DEBUG
        AddIndentation(psContext);
        bcatcstr(glsl, "//SYNC\n");
#endif

        if (ui32SyncFlags & SYNC_THREADS_IN_GROUP)
        {
            AddIndentation(psContext);
            bcatcstr(glsl, "barrier();\n");
            AddIndentation(psContext);
            bcatcstr(glsl, "groupMemoryBarrier();\n");
        }
        if (ui32SyncFlags & SYNC_THREAD_GROUP_SHARED_MEMORY)
        {
            AddIndentation(psContext);
            bcatcstr(glsl, "memoryBarrierShared();\n");
        }
        if (ui32SyncFlags & (SYNC_UNORDERED_ACCESS_VIEW_MEMORY_GROUP | SYNC_UNORDERED_ACCESS_VIEW_MEMORY_GLOBAL))
        {
            AddIndentation(psContext);
            bcatcstr(glsl, "memoryBarrier();\n");
        }
        break;
    }
    case OPCODE_SWITCH:
    {
#ifdef _DEBUG
        AddIndentation(psContext);
        bcatcstr(glsl, "//SWITCH\n");
#endif
        AddIndentation(psContext);
        bcatcstr(glsl, "switch(int(");
        TranslateOperand(psContext, &psInst->asOperands[0], TO_FLAG_NONE);
        bcatcstr(glsl, ")){\n");

        psContext->indent += 2;
        break;
    }
    case OPCODE_CASE:
    {
        --psContext->indent;
#ifdef _DEBUG
        AddIndentation(psContext);
        bcatcstr(glsl, "//case\n");
#endif
        AddIndentation(psContext);

        bcatcstr(glsl, "case ");
        TranslateOperand(psContext, &psInst->asOperands[0], TO_FLAG_INTEGER);
        bcatcstr(glsl, ":\n");

        ++psContext->indent;
        break;
    }
    case OPCODE_EQ:
    {
#ifdef _DEBUG
        AddIndentation(psContext);
        bcatcstr(glsl, "//EQ\n");
#endif
        AddComparision(psContext, psInst, CMP_EQ, TO_FLAG_FLOAT);
        break;
    }
    case OPCODE_USHR:
    {
#ifdef _DEBUG
        AddIndentation(psContext);
        bcatcstr(glsl, "//USHR\n");
#endif
        CallBinaryOp(psContext, ">>", psInst, 0, 1, 2, TO_FLAG_UNSIGNED_INTEGER);
        break;
    }
    case OPCODE_ISHL:
    {
        uint32_t ui32Flags = TO_FLAG_INTEGER;

#ifdef _DEBUG
        AddIndentation(psContext);
        bcatcstr(glsl, "//ISHL\n");
#endif

        if (GetOperandDataType(psContext, &psInst->asOperands[0]) == SVT_UINT)
        {
            ui32Flags = TO_FLAG_UNSIGNED_INTEGER;
        }

        CallBinaryOp(psContext, "<<", psInst, 0, 1, 2, ui32Flags);
        break;
    }
    case OPCODE_ISHR:
    {
        uint32_t ui32Flags = TO_FLAG_INTEGER;
#ifdef _DEBUG
        AddIndentation(psContext);
        bcatcstr(glsl, "//ISHR\n");
#endif

        if (GetOperandDataType(psContext, &psInst->asOperands[0]) == SVT_UINT)
        {
            ui32Flags = TO_FLAG_UNSIGNED_INTEGER;
        }

        CallBinaryOp(psContext, ">>", psInst, 0, 1, 2, ui32Flags);
        break;
    }
    case OPCODE_LD:
    case OPCODE_LD_MS:
    {
        ResourceBinding* psBinding = 0;
        uint32_t ui32FetchTypeToFlags;
#ifdef _DEBUG
        AddIndentation(psContext);
        if (psInst->eOpcode == OPCODE_LD)
        {
            bcatcstr(glsl, "//LD\n");
        }
        else
        {
            bcatcstr(glsl, "//LD_MS\n");
        }
#endif

        GetResourceFromBindingPoint(RGROUP_TEXTURE, psInst->asOperands[2].ui32RegisterNumber, &psContext->psShader->sInfo, &psBinding);
        ui32FetchTypeToFlags = GetReturnTypeToFlags(psBinding->ui32ReturnType);

        const char* fetchFunctionString = psInst->bAddressOffset ? "texelFetchOffset" : "texelFetch";
        switch (psBinding->eDimension)
        {
        case REFLECT_RESOURCE_DIMENSION_TEXTURE1D:
        {
            //texelFetch(samplerBuffer, int coord, level)
            AddIndentation(psContext);
            BeginAssignment(psContext, &psInst->asOperands[0], ui32FetchTypeToFlags, psInst->bSaturate);
            bcatcstr(glsl, fetchFunctionString);
            bcatcstr(glsl, "(");

            TranslateOperand(psContext, &psInst->asOperands[2], TO_FLAG_NONE);
            bcatcstr(glsl, ", (");
            TranslateOperand(psContext, &psInst->asOperands[1], TO_FLAG_INTEGER);
            bcatcstr(glsl, ").x, int((");
            TranslateOperand(psContext, &psInst->asOperands[1], TO_FLAG_INTEGER);
            bcatcstr(glsl, ").w)");
            if (psInst->bAddressOffset)
            {
                bformata(glsl, ", %d", psInst->iUAddrOffset);
            }
            bcatcstr(glsl, ")");
            TranslateOperandSwizzle(psContext, &psInst->asOperands[2]);
            TranslateOperandSwizzle(psContext, &psInst->asOperands[0]);
            EndAssignment(psContext, &psInst->asOperands[0], ui32FetchTypeToFlags, psInst->bSaturate);
            bcatcstr(glsl, ";\n");
            break;
        }
        case REFLECT_RESOURCE_DIMENSION_TEXTURE2DARRAY:
        case REFLECT_RESOURCE_DIMENSION_TEXTURE3D:
        {
            //texelFetch(samplerBuffer, ivec3 coord, level)
            AddIndentation(psContext);
            BeginAssignment(psContext, &psInst->asOperands[0], ui32FetchTypeToFlags, psInst->bSaturate);
            bcatcstr(glsl, fetchFunctionString);
            bcatcstr(glsl, "(");

            TranslateOperand(psContext, &psInst->asOperands[2], TO_FLAG_NONE);
            bcatcstr(glsl, ", (");
            TranslateOperand(psContext, &psInst->asOperands[1], TO_FLAG_INTEGER);
            bcatcstr(glsl, ").xyz, int((");
            TranslateOperand(psContext, &psInst->asOperands[1], TO_FLAG_INTEGER);
            bcatcstr(glsl, ").w)");
            if (psInst->bAddressOffset)
            {
                if (psBinding->eDimension == REFLECT_RESOURCE_DIMENSION_TEXTURE2DARRAY)
                {
                    bformata(glsl, ", ivec2(%d, %d)",
                        psInst->iUAddrOffset,
                        psInst->iVAddrOffset);
                }
                else
                {
                    bformata(glsl, ", ivec3(%d, %d, %d)",
                        psInst->iUAddrOffset,
                        psInst->iVAddrOffset,
                        psInst->iWAddrOffset);
                }
            }
            bcatcstr(glsl, ")");
            TranslateOperandSwizzle(psContext, &psInst->asOperands[2]);
            TranslateOperandSwizzle(psContext, &psInst->asOperands[0]);
            EndAssignment(psContext, &psInst->asOperands[0], ui32FetchTypeToFlags, psInst->bSaturate);
            bcatcstr(glsl, ";\n");
            break;
        }
        case REFLECT_RESOURCE_DIMENSION_TEXTURE2D:
        case REFLECT_RESOURCE_DIMENSION_TEXTURE1DARRAY:
        {
            AddIndentation(psContext);
            BeginAssignment(psContext, &psInst->asOperands[0], ui32FetchTypeToFlags, psInst->bSaturate);

            if (IsGmemReservedSlot(FBF_ANY, psInst->asOperands[2].ui32RegisterNumber))         // FRAMEBUFFER FETCH
            {
                TranslateOperand(psContext, &psInst->asOperands[2], TO_FLAG_NONE);
            }
            else
            {
                bcatcstr(glsl, fetchFunctionString);
                bcatcstr(glsl, "(");

                TranslateOperand(psContext, &psInst->asOperands[2], TO_FLAG_NONE);
                bcatcstr(glsl, ", (");
                TranslateOperand(psContext, &psInst->asOperands[1], TO_FLAG_INTEGER);
                bcatcstr(glsl, ").xy, int((");
                TranslateOperand(psContext, &psInst->asOperands[1], TO_FLAG_INTEGER);
                bcatcstr(glsl, ").w)");
                if (psInst->bAddressOffset)
                {
                    if (psBinding->eDimension == REFLECT_RESOURCE_DIMENSION_TEXTURE1DARRAY)
                    {
                        bformata(glsl, ", int(%d)", psInst->iUAddrOffset);
                    }
                    else
                    {
                        bformata(glsl, ", ivec2(%d, %d)",
                            psInst->iUAddrOffset,
                            psInst->iVAddrOffset);
                    }
                }
                bcatcstr(glsl, ")");
                TranslateOperandSwizzle(psContext, &psInst->asOperands[2]);
            }

            TranslateOperandSwizzle(psContext, &psInst->asOperands[0]);
            EndAssignment(psContext, &psInst->asOperands[0], ui32FetchTypeToFlags, psInst->bSaturate);
            bcatcstr(glsl, ";\n");
            break;
        }
        case REFLECT_RESOURCE_DIMENSION_BUFFER:
        {
            //texelFetch(samplerBuffer, scalar integer coord)
            AddIndentation(psContext);
            BeginAssignment(psContext, &psInst->asOperands[0], ui32FetchTypeToFlags, psInst->bSaturate);
            bcatcstr(glsl, "texelFetch(");

            TranslateOperand(psContext, &psInst->asOperands[2], TO_FLAG_NONE);
            bcatcstr(glsl, ", (");
            TranslateOperand(psContext, &psInst->asOperands[1], TO_FLAG_INTEGER);
            bcatcstr(glsl, ").x)");
            TranslateOperandSwizzle(psContext, &psInst->asOperands[2]);
            TranslateOperandSwizzle(psContext, &psInst->asOperands[0]);
            EndAssignment(psContext, &psInst->asOperands[0], ui32FetchTypeToFlags, psInst->bSaturate);
            bcatcstr(glsl, ";\n");
            break;
        }
        case REFLECT_RESOURCE_DIMENSION_TEXTURE2DMS:
        {
            //texelFetch(samplerBuffer, ivec2 coord, sample)

            ASSERT(psInst->eOpcode == OPCODE_LD_MS);

            AddIndentation(psContext);
            BeginAssignment(psContext, &psInst->asOperands[0], ui32FetchTypeToFlags, psInst->bSaturate);
            bcatcstr(glsl, "texelFetch(");

            TranslateOperand(psContext, &psInst->asOperands[2], TO_FLAG_NONE);
            bcatcstr(glsl, ", (");
            TranslateOperand(psContext, &psInst->asOperands[1], TO_FLAG_INTEGER);
            bcatcstr(glsl, ").xy, int(");
            TranslateOperand(psContext, &psInst->asOperands[3], TO_FLAG_INTEGER);
            bcatcstr(glsl, "))");
            TranslateOperandSwizzle(psContext, &psInst->asOperands[2]);
            TranslateOperandSwizzle(psContext, &psInst->asOperands[0]);
            EndAssignment(psContext, &psInst->asOperands[0], ui32FetchTypeToFlags, psInst->bSaturate);
            bcatcstr(glsl, ";\n");
            break;
        }
        case REFLECT_RESOURCE_DIMENSION_TEXTURE2DMSARRAY:
        {
            //texelFetch(samplerBuffer, ivec3 coord, sample)

            ASSERT(psInst->eOpcode == OPCODE_LD_MS);

            AddIndentation(psContext);
            BeginAssignment(psContext, &psInst->asOperands[0], ui32FetchTypeToFlags, psInst->bSaturate);
            bcatcstr(glsl, "texelFetch(");

            TranslateOperand(psContext, &psInst->asOperands[2], TO_FLAG_NONE);
            bcatcstr(glsl, ", ivec3((");
            TranslateOperand(psContext, &psInst->asOperands[1], TO_FLAG_INTEGER);
            bcatcstr(glsl, ").xyz), int(");
            TranslateOperand(psContext, &psInst->asOperands[3], TO_FLAG_INTEGER);
            bcatcstr(glsl, "))");
            TranslateOperandSwizzle(psContext, &psInst->asOperands[2]);
            TranslateOperandSwizzle(psContext, &psInst->asOperands[0]);
            EndAssignment(psContext, &psInst->asOperands[0], ui32FetchTypeToFlags, psInst->bSaturate);
            bcatcstr(glsl, ";\n");
            break;
        }
        case REFLECT_RESOURCE_DIMENSION_TEXTURECUBE:
        case REFLECT_RESOURCE_DIMENSION_TEXTURECUBEARRAY:
        case REFLECT_RESOURCE_DIMENSION_BUFFEREX:
        default:
        {
            break;
        }
        }
        break;
    }
    case OPCODE_DISCARD:
    {
#ifdef _DEBUG
        AddIndentation(psContext);
        bcatcstr(glsl, "//DISCARD\n");
#endif
        AddIndentation(psContext);
        if (psContext->psShader->ui32MajorVersion <= 3)
        {
            bcatcstr(glsl, "if(any(lessThan((");
            TranslateOperand(psContext, &psInst->asOperands[0], TO_FLAG_FLOAT);

            if (psContext->psShader->ui32MajorVersion == 1)
            {
                /* SM1.X only kills based on the rgb channels */
                bcatcstr(glsl, ").xyz, vec3(0.0)))){discard;}\n");
            }
            else
            {
                bcatcstr(glsl, "), vec4(0.0)))){discard;}\n");
            }
        }
        else if (psInst->eBooleanTestType == INSTRUCTION_TEST_ZERO)
        {
            bcatcstr(glsl, "if((");
            TranslateOperand(psContext, &psInst->asOperands[0], TO_FLAG_FLOAT);
            bcatcstr(glsl, ")==0.0){discard;}\n");
        }
        else
        {
            ASSERT(psInst->eBooleanTestType == INSTRUCTION_TEST_NONZERO);
            bcatcstr(glsl, "if((");
            TranslateOperand(psContext, &psInst->asOperands[0], TO_FLAG_FLOAT);
            bcatcstr(glsl, ")!=0.0){discard;}\n");
        }
        break;
    }
    case OPCODE_LOD:
    {
        uint32_t ui32SampleTypeToFlags = GetResourceReturnTypeToFlags(RGROUP_TEXTURE, psInst->asOperands[2].ui32RegisterNumber, psContext);
#ifdef _DEBUG
        AddIndentation(psContext);
        bcatcstr(glsl, "//LOD\n");
#endif
        //LOD computes the following vector (ClampedLOD, NonClampedLOD, 0, 0)

        AddIndentation(psContext);
        BeginAssignment(psContext, &psInst->asOperands[0], ui32SampleTypeToFlags, psInst->bSaturate);

        //If the core language does not have query-lod feature,
        //then the extension is used. The name of the function
        //changed between extension and core.
        if (HaveQueryLod(psContext->psShader->eTargetLanguage))
        {
            bcatcstr(glsl, "textureQueryLod(");
        }
        else
        {
            bcatcstr(glsl, "textureQueryLOD(");
        }

        TranslateOperand(psContext, &psInst->asOperands[2], TO_FLAG_NONE);
        bcatcstr(glsl, ",");
        TranslateTexCoord(psContext,
            psContext->psShader->aeResourceDims[psInst->asOperands[2].ui32RegisterNumber],
            &psInst->asOperands[1]);
        bcatcstr(glsl, ")");

        //The swizzle on srcResource allows the returned values to be swizzled arbitrarily before they are written to the destination.

        // iWriteMaskEnabled is forced off during DecodeOperand because swizzle on sampler uniforms
        // does not make sense. But need to re-enable to correctly swizzle this particular instruction.
        psInst->asOperands[2].iWriteMaskEnabled = 1;
        TranslateOperandSwizzle(psContext, &psInst->asOperands[2]);
        EndAssignment(psContext, &psInst->asOperands[0], ui32SampleTypeToFlags, psInst->bSaturate);
        bcatcstr(glsl, ";\n");
        break;
    }
    case OPCODE_EVAL_CENTROID:
    {
#ifdef _DEBUG
        AddIndentation(psContext);
        bcatcstr(glsl, "//EVAL_CENTROID\n");
#endif
        AddIndentation(psContext);
        BeginAssignment(psContext, &psInst->asOperands[0], TO_FLAG_FLOAT, psInst->bSaturate);
        bcatcstr(glsl, "interpolateAtCentroid(");
        //interpolateAtCentroid accepts in-qualified variables.
        //As long as bytecode only writes vX registers in declarations
        //we should be able to use the declared name directly.
        TranslateOperand(psContext, &psInst->asOperands[1], TO_FLAG_DECLARATION_NAME);
        bcatcstr(glsl, ")");
        EndAssignment(psContext, &psInst->asOperands[0], TO_FLAG_FLOAT, psInst->bSaturate);
        bcatcstr(glsl, ";\n");
        break;
    }
    case OPCODE_EVAL_SAMPLE_INDEX:
    {
#ifdef _DEBUG
        AddIndentation(psContext);
        bcatcstr(glsl, "//EVAL_SAMPLE_INDEX\n");
#endif
        AddIndentation(psContext);
        BeginAssignment(psContext, &psInst->asOperands[0], TO_FLAG_FLOAT, psInst->bSaturate);
        bcatcstr(glsl, "interpolateAtSample(");
        //interpolateAtSample accepts in-qualified variables.
        //As long as bytecode only writes vX registers in declarations
        //we should be able to use the declared name directly.
        TranslateOperand(psContext, &psInst->asOperands[1], TO_FLAG_DECLARATION_NAME);
        bcatcstr(glsl, ", ");
        TranslateOperand(psContext, &psInst->asOperands[2], TO_FLAG_INTEGER);
        bcatcstr(glsl, ")");
        EndAssignment(psContext, &psInst->asOperands[0], TO_FLAG_FLOAT, psInst->bSaturate);
        bcatcstr(glsl, ";\n");
        break;
    }
    case OPCODE_EVAL_SNAPPED:
    {
#ifdef _DEBUG
        AddIndentation(psContext);
        bcatcstr(glsl, "//EVAL_SNAPPED\n");
#endif
        AddIndentation(psContext);
        BeginAssignment(psContext, &psInst->asOperands[0], TO_FLAG_FLOAT, psInst->bSaturate);
        bcatcstr(glsl, "interpolateAtOffset(");
        //interpolateAtOffset accepts in-qualified variables.
        //As long as bytecode only writes vX registers in declarations
        //we should be able to use the declared name directly.
        TranslateOperand(psContext, &psInst->asOperands[1], TO_FLAG_DECLARATION_NAME);
        bcatcstr(glsl, ", ");
        TranslateOperand(psContext, &psInst->asOperands[2], TO_FLAG_INTEGER);
        bcatcstr(glsl, ".xy)");
        EndAssignment(psContext, &psInst->asOperands[0], TO_FLAG_FLOAT, psInst->bSaturate);
        bcatcstr(glsl, ";\n");
        break;
    }
    case OPCODE_LD_STRUCTURED:
    {
#ifdef _DEBUG
        AddIndentation(psContext);
        bcatcstr(glsl, "//LD_STRUCTURED ");
#endif
        uint32_t reg_num = psInst->asOperands[3].ui32RegisterNumber;
        if (reg_num >= GMEM_PLS_RO_SLOT && reg_num <= GMEM_PLS_RW_SLOT)
        {
#ifdef _DEBUG
            bcatcstr(glsl, "-> LOAD FROM PLS\n");
#endif
            // Ensure it's not a write only PLS
            ASSERT(reg_num != GMEM_PLS_WO_SLOT);

            TranslateShaderPLSLoad(psContext, psInst);
        }
        else
        {
            bcatcstr(glsl, "\n");
            TranslateShaderStorageLoad(psContext, psInst);
        }
        break;
    }
    case OPCODE_LD_UAV_TYPED:
    {
        uint32_t ui32UAVReturnTypeToFlags = GetResourceReturnTypeToFlags(RGROUP_UAV, psInst->asOperands[2].ui32RegisterNumber, psContext);
#ifdef _DEBUG
        AddIndentation(psContext);
        bcatcstr(glsl, "//LD_UAV_TYPED\n");
#endif
        AddIndentation(psContext);
        BeginAssignment(psContext, &psInst->asOperands[0], ui32UAVReturnTypeToFlags, psInst->bSaturate);
        bcatcstr(glsl, "imageLoad(");
        TranslateOperand(psContext, &psInst->asOperands[2], TO_FLAG_NAME_ONLY);

        switch (psInst->eResDim)
        {
        case RESOURCE_DIMENSION_BUFFER:
        case RESOURCE_DIMENSION_TEXTURE1D:
            bcatcstr(glsl, ", (");
            TranslateOperand(psContext, &psInst->asOperands[1], TO_FLAG_INTEGER);
            bformata(glsl, ").x)");
            break;
        case RESOURCE_DIMENSION_TEXTURE2D:
        case RESOURCE_DIMENSION_TEXTURE1DARRAY:
        case RESOURCE_DIMENSION_TEXTURE2DMS:
            bcatcstr(glsl, ", (");
            TranslateOperand(psContext, &psInst->asOperands[1], TO_FLAG_INTEGER);
            bformata(glsl, ").xy)");
            break;
        case RESOURCE_DIMENSION_TEXTURE2DARRAY:
        case RESOURCE_DIMENSION_TEXTURE3D:
        case RESOURCE_DIMENSION_TEXTURE2DMSARRAY:
        case RESOURCE_DIMENSION_TEXTURECUBE:
            bcatcstr(glsl, ", (");
            TranslateOperand(psContext, &psInst->asOperands[1], TO_FLAG_INTEGER);
            bformata(glsl, ").xyz)");
            break;
        case RESOURCE_DIMENSION_TEXTURECUBEARRAY:
            bcatcstr(glsl, ", (");
            TranslateOperand(psContext, &psInst->asOperands[1], TO_FLAG_INTEGER);
            bformata(glsl, ").xyzw)");
            break;
        }

        TranslateOperandSwizzle(psContext, &psInst->asOperands[0]);
        EndAssignment(psContext, &psInst->asOperands[0], ui32UAVReturnTypeToFlags, psInst->bSaturate);
        bcatcstr(glsl, ";\n");
        break;
    }
    case OPCODE_STORE_RAW:
    {
#ifdef _DEBUG
        AddIndentation(psContext);
        bcatcstr(glsl, "//STORE_RAW\n");
#endif
        TranslateShaderStorageStore(psContext, psInst);
        break;
    }
    case OPCODE_STORE_STRUCTURED:
    {
#ifdef _DEBUG
        AddIndentation(psContext);
        bcatcstr(glsl, "//STORE_STRUCTURE ");
#endif
        uint32_t reg_num = psInst->asOperands[0].ui32RegisterNumber;
        if (reg_num >= GMEM_PLS_RO_SLOT && reg_num <= GMEM_PLS_RW_SLOT)
        {
#ifdef _DEBUG
            bcatcstr(glsl, "-> STORE TO PLS\n");
#endif
            // Ensure it's not a read only PLS
            ASSERT(reg_num != GMEM_PLS_RO_SLOT);

            TranslateShaderPLSStore(psContext, psInst);
        }
        else
        {
            bcatcstr(glsl, "\n");
            TranslateShaderStorageStore(psContext, psInst);
        }
        break;
    }

    case OPCODE_STORE_UAV_TYPED:
    {
        ResourceBinding* psRes;
        int foundResource;
#ifdef _DEBUG
        AddIndentation(psContext);
        bcatcstr(glsl, "//STORE_UAV_TYPED\n");
#endif
        AddIndentation(psContext);

        foundResource = GetResourceFromBindingPoint(RGROUP_UAV, psInst->asOperands[0].ui32RegisterNumber, &psContext->psShader->sInfo, &psRes);

        ASSERT(foundResource);

        bcatcstr(glsl, "imageStore(");
        TranslateOperand(psContext, &psInst->asOperands[0], TO_FLAG_NAME_ONLY);

        switch (psRes->eDimension)
        {
        case REFLECT_RESOURCE_DIMENSION_BUFFER:
        case REFLECT_RESOURCE_DIMENSION_TEXTURE1D:
            bcatcstr(glsl, ", (");
            TranslateOperand(psContext, &psInst->asOperands[1], TO_FLAG_INTEGER);
            bcatcstr(glsl, ").x");

            // HACK!!
            bcatcstr(glsl, ", ");
            TranslateOperand(psContext, &psInst->asOperands[2], TO_FLAG_INTEGER | TO_FLAG_UNSIGNED_INTEGER);
            bformata(glsl, ");\n");
            break;
        case REFLECT_RESOURCE_DIMENSION_TEXTURE2D:
        case REFLECT_RESOURCE_DIMENSION_TEXTURE1DARRAY:
        case REFLECT_RESOURCE_DIMENSION_TEXTURE2DMS:
            bcatcstr(glsl, ", (");
            TranslateOperand(psContext, &psInst->asOperands[1], TO_FLAG_INTEGER);
            bcatcstr(glsl, ".xy)");

            // HACK!!
            bcatcstr(glsl, ", ");
            TranslateOperand(psContext, &psInst->asOperands[2], TO_FLAG_FLOAT);
            bformata(glsl, ");\n");
            break;
        case REFLECT_RESOURCE_DIMENSION_TEXTURE2DARRAY:
        case REFLECT_RESOURCE_DIMENSION_TEXTURE3D:
        case REFLECT_RESOURCE_DIMENSION_TEXTURE2DMSARRAY:
        case REFLECT_RESOURCE_DIMENSION_TEXTURECUBE:
            bcatcstr(glsl, ", (");
            TranslateOperand(psContext, &psInst->asOperands[1], TO_FLAG_INTEGER);
            bcatcstr(glsl, ".xyz)");

            // HACK!!
            bcatcstr(glsl, ", ");
            TranslateOperand(psContext, &psInst->asOperands[2], TO_FLAG_FLOAT);
            bformata(glsl, ");\n");
            break;
        case REFLECT_RESOURCE_DIMENSION_TEXTURECUBEARRAY:
            bcatcstr(glsl, ", (");
            TranslateOperand(psContext, &psInst->asOperands[1], TO_FLAG_INTEGER);
            bcatcstr(glsl, ".xyzw)");

            // HACK!!
            bcatcstr(glsl, ", ");
            TranslateOperand(psContext, &psInst->asOperands[2], TO_FLAG_FLOAT);
            bformata(glsl, ");\n");
            break;
        }

        break;
    }
    case OPCODE_LD_RAW:
    {
#ifdef _DEBUG
        AddIndentation(psContext);
        bcatcstr(glsl, "//LD_RAW\n");
#endif

        TranslateShaderStorageLoad(psContext, psInst);
        break;
    }

    case OPCODE_ATOMIC_CMP_STORE:
    case OPCODE_IMM_ATOMIC_AND:
    case OPCODE_ATOMIC_AND:
    case OPCODE_IMM_ATOMIC_IADD:
    case OPCODE_ATOMIC_IADD:
    case OPCODE_ATOMIC_OR:
    case OPCODE_ATOMIC_XOR:
    case OPCODE_ATOMIC_IMIN:
    case OPCODE_ATOMIC_UMIN:
    case OPCODE_ATOMIC_IMAX:
    case OPCODE_ATOMIC_UMAX:
    case OPCODE_IMM_ATOMIC_IMAX:
    case OPCODE_IMM_ATOMIC_IMIN:
    case OPCODE_IMM_ATOMIC_UMAX:
    case OPCODE_IMM_ATOMIC_UMIN:
    case OPCODE_IMM_ATOMIC_OR:
    case OPCODE_IMM_ATOMIC_XOR:
    case OPCODE_IMM_ATOMIC_EXCH:
    case OPCODE_IMM_ATOMIC_CMP_EXCH:
    {
        TranslateAtomicMemOp(psContext, psInst);
        break;
    }
    case OPCODE_UBFE:
    case OPCODE_IBFE:
    {
        const char* swizzles = "xyzw";
        uint32_t eDataType, destElem;
        uint32_t destElemCount = GetNumSwizzleElements(&psInst->asOperands[0]);
        uint32_t s0ElemCount = GetNumSwizzleElements(&psInst->asOperands[1]);
        uint32_t s1ElemCount = GetNumSwizzleElements(&psInst->asOperands[2]);
        uint32_t s2ElemCount = GetNumSwizzleElements(&psInst->asOperands[3]);
        const char* szVecType;
        const char* szDataType;
#ifdef _DEBUG
        AddIndentation(psContext);
        if (psInst->eOpcode == OPCODE_UBFE)
        {
            bcatcstr(glsl, "//OPCODE_UBFE\n");
        }
        else
        {
            bcatcstr(glsl, "//OPCODE_IBFE\n");
        }
#endif
        if (psInst->eOpcode == OPCODE_UBFE)
        {
            eDataType = TO_FLAG_UNSIGNED_INTEGER;
            szVecType = "uvec";
            szDataType = "uint";
        }
        else
        {
            eDataType = TO_FLAG_INTEGER;
            szVecType = "ivec";
            szDataType = "int";
        }

        if (psContext->psShader->eTargetLanguage != LANG_ES_300)
        {
            AddIndentation(psContext);
            BeginAssignment(psContext, &psInst->asOperands[0], eDataType, psInst->bSaturate);

            if (destElemCount > 1)
            {
                bformata(glsl, "%s%d(", szVecType, destElemCount);
            }

            for (destElem = 0; destElem < destElemCount; ++destElem)
            {
                if (destElem > 0)
                {
                    bcatcstr(glsl, ", ");
                }

                bformata(glsl, "bitfieldExtract(");

                TranslateOperand(psContext, &psInst->asOperands[3], eDataType);
                if (s2ElemCount > 1)
                {
                    TranslateOperandSwizzle(psContext, &psInst->asOperands[3]);
                    bformata(glsl, ".%c", swizzles[destElem]);
                }

                bcatcstr(glsl, ", ");

                TranslateOperand(psContext, &psInst->asOperands[2], TO_FLAG_INTEGER);
                if (s1ElemCount > 1)
                {
                    TranslateOperandSwizzle(psContext, &psInst->asOperands[2]);
                    bformata(glsl, ".%c", swizzles[destElem]);
                }

                bcatcstr(glsl, ", ");

                TranslateOperand(psContext, &psInst->asOperands[1], TO_FLAG_INTEGER);
                if (s0ElemCount > 1)
                {
                    TranslateOperandSwizzle(psContext, &psInst->asOperands[1]);
                    bformata(glsl, ".%c", swizzles[destElem]);
                }

                bformata(glsl, ")");
            }
            if (destElemCount > 1)
            {
                bcatcstr(glsl, ")");
            }
            EndAssignment(psContext, &psInst->asOperands[0], eDataType, psInst->bSaturate);
            bcatcstr(glsl, ";\n");
        }
        else
        {
            // Following is the explicit impl' for ES3.0
            //  Here's the description of what bitfieldExtract actually does
            //  https://www.opengl.org/registry/specs/ARB/gpu_shader5.txt


            AddIndentation(psContext);
            bcatcstr(glsl, "{\n");

            //  << (32-bits-offset)
            AddIndentation(psContext);
            AddIndentation(psContext);
            bcatcstr(glsl, "int offsetLeft = (32 - ");
            TranslateOperand(psContext, &psInst->asOperands[1], TO_FLAG_INTEGER);
            bcatcstr(glsl, " - ");
            TranslateOperand(psContext, &psInst->asOperands[2], TO_FLAG_INTEGER);
            bcatcstr(glsl, ");\n");

            //  >> (32-bits)
            AddIndentation(psContext);
            AddIndentation(psContext);
            bcatcstr(glsl, "int offsetRight = (32 - ");
            TranslateOperand(psContext, &psInst->asOperands[1], TO_FLAG_INTEGER);
            bcatcstr(glsl, ");\n");

            AddIndentation(psContext);
            AddIndentation(psContext);
            bformata(glsl, "%s tmp;\n", szDataType);

            for (destElem = 0; destElem < destElemCount; ++destElem)
            {
                AddIndentation(psContext);
                AddIndentation(psContext);
                bcatcstr(glsl, "tmp = ");

                if (psInst->eOpcode == OPCODE_IBFE)
                {
                    TranslateOperand(psContext, &psInst->asOperands[1], TO_FLAG_INTEGER);
                    bcatcstr(glsl, " ? ");
                }

                TranslateOperand(psContext, &psInst->asOperands[3], eDataType);
                if (s2ElemCount > 1)
                {
                    TranslateOperandSwizzle(psContext, &psInst->asOperands[3]);
                    bformata(glsl, ".%c", swizzles[destElem]);
                }
                if (psInst->eOpcode == OPCODE_IBFE)
                {
                    TranslateOperand(psContext, &psInst->asOperands[1], TO_FLAG_INTEGER);
                    bcatcstr(glsl, " : 0 ");
                }
                bcatcstr(glsl, ";\n");

                AddIndentation(psContext);
                AddIndentation(psContext);
                bcatcstr(glsl, "tmp = ((tmp << offsetLeft) >> offsetRight);\n");

                AddIndentation(psContext);
                AddIndentation(psContext);
                BeginAssignment(psContext, &psInst->asOperands[0], 0, psInst->bSaturate);
                if (eDataType == TO_FLAG_INTEGER)
                {
                    bcatcstr(glsl, "intBitsToFloat(tmp));\n");
                }
                else
                {
                    bcatcstr(glsl, "uintBitsToFloat(tmp));\n");
                }
            }

            AddIndentation(psContext);
            bcatcstr(glsl, "}\n");
        }

        break;
    }
    case OPCODE_RCP:
    {
        const uint32_t destElemCount = GetNumSwizzleElements(&psInst->asOperands[0]);
#ifdef _DEBUG
        AddIndentation(psContext);
        bcatcstr(glsl, "//RCP\n");
#endif
        AddIndentation(psContext);
        BeginAssignment(psContext, &psInst->asOperands[0], TO_FLAG_FLOAT, psInst->bSaturate);
        bcatcstr(glsl, "(vec4(1.0) / vec4(");
        TranslateOperand(psContext, &psInst->asOperands[1], TO_FLAG_FLOAT);
        bcatcstr(glsl, "))");
        AddSwizzleUsingElementCount(psContext, destElemCount);
        EndAssignment(psContext, &psInst->asOperands[0], TO_FLAG_FLOAT, psInst->bSaturate);
        bcatcstr(glsl, ";\n");
        break;
    }
    case OPCODE_F16TOF32:
    {
        const uint32_t destElemCount = GetNumSwizzleElements(&psInst->asOperands[0]);
        const uint32_t s0ElemCount = GetNumSwizzleElements(&psInst->asOperands[1]);
        uint32_t destElem;
#ifdef _DEBUG
        AddIndentation(psContext);
        bcatcstr(glsl, "//F16TOF32\n");
#endif
        for (destElem = 0; destElem < destElemCount; ++destElem)
        {
            const char* swizzle[] = {".x", ".y", ".z", ".w"};

            //unpackHalf2x16 converts two f16s packed into uint to two f32s.

            //dest.swiz.x = unpackHalf2x16(src.swiz.x).x
            //dest.swiz.y = unpackHalf2x16(src.swiz.y).x
            //dest.swiz.z = unpackHalf2x16(src.swiz.z).x
            //dest.swiz.w = unpackHalf2x16(src.swiz.w).x

            AddIndentation(psContext);
            if (destElemCount > 1)
            {
                BeginAssignmentEx(psContext, &psInst->asOperands[0], TO_FLAG_FLOAT, psInst->bSaturate, swizzle[destElem]);
            }
            else
            {
                BeginAssignment(psContext, &psInst->asOperands[0], TO_FLAG_FLOAT, psInst->bSaturate);
            }

            bcatcstr(glsl, "unpackHalf2x16(");
            TranslateOperand(psContext, &psInst->asOperands[1], TO_FLAG_UNSIGNED_INTEGER);
            if (s0ElemCount > 1)
            {
                bcatcstr(glsl, swizzle[destElem]);
            }
            bcatcstr(glsl, ").x");
            EndAssignment(psContext, &psInst->asOperands[0], TO_FLAG_FLOAT, psInst->bSaturate);
            bcatcstr(glsl, ";\n");
        }
        break;
    }
    case OPCODE_F32TOF16:
    {
        const uint32_t destElemCount = GetNumSwizzleElements(&psInst->asOperands[0]);
        const uint32_t s0ElemCount = GetNumSwizzleElements(&psInst->asOperands[1]);
        uint32_t destElem;
#ifdef _DEBUG
        AddIndentation(psContext);
        bcatcstr(glsl, "//F32TOF16\n");
#endif
        for (destElem = 0; destElem < destElemCount; ++destElem)
        {
            const char* swizzle[] = {".x", ".y", ".z", ".w"};

            //packHalf2x16 converts two f32s to two f16s packed into a uint.

            //dest.swiz.x = packHalf2x16(vec2(src.swiz.x)) & 0xFFFF
            //dest.swiz.y = packHalf2x16(vec2(src.swiz.y)) & 0xFFFF
            //dest.swiz.z = packHalf2x16(vec2(src.swiz.z)) & 0xFFFF
            //dest.swiz.w = packHalf2x16(vec2(src.swiz.w)) & 0xFFFF

            AddIndentation(psContext);
            if (destElemCount > 1)
            {
                BeginAssignmentEx(psContext, &psInst->asOperands[0], TO_FLAG_UNSIGNED_INTEGER, psInst->bSaturate, swizzle[destElem]);
            }
            else
            {
                BeginAssignment(psContext, &psInst->asOperands[0], TO_FLAG_UNSIGNED_INTEGER, psInst->bSaturate);
            }

            bcatcstr(glsl, "packHalf2x16(vec2(");
            TranslateOperand(psContext, &psInst->asOperands[1], TO_FLAG_FLOAT);
            if (s0ElemCount > 1)
            {
                bcatcstr(glsl, swizzle[destElem]);
            }
            bcatcstr(glsl, ")) & 0xFFFFu");
            EndAssignment(psContext, &psInst->asOperands[0], TO_FLAG_UNSIGNED_INTEGER, psInst->bSaturate);
            bcatcstr(glsl, ";\n");
        }
        break;
    }
    case OPCODE_INEG:
    {
        uint32_t dstCount = GetNumSwizzleElements(&psInst->asOperands[0]);
        uint32_t srcCount = GetNumSwizzleElements(&psInst->asOperands[1]);
#ifdef _DEBUG
        AddIndentation(psContext);
        bcatcstr(glsl, "//INEG\n");
#endif
        //dest = 0 - src0
        AddIndentation(psContext);
        BeginAssignment(psContext, &psInst->asOperands[0], TO_FLAG_INTEGER, psInst->bSaturate);
        //bcatcstr(glsl, " = 0 - ");
        bcatcstr(glsl, "-(");
        TranslateOperand(psContext, &psInst->asOperands[1], TO_FLAG_NONE | TO_FLAG_INTEGER);
        if (srcCount > dstCount)
        {
            AddSwizzleUsingElementCount(psContext, dstCount);
        }
        bcatcstr(glsl, ")");
        EndAssignment(psContext, &psInst->asOperands[0], TO_FLAG_INTEGER, psInst->bSaturate);
        bcatcstr(glsl, ";\n");
        break;
    }
    case OPCODE_DERIV_RTX_COARSE:
    case OPCODE_DERIV_RTX_FINE:
    case OPCODE_DERIV_RTX:
    {
#ifdef _DEBUG
        AddIndentation(psContext);
        bcatcstr(glsl, "//DERIV_RTX\n");
#endif
        CallHelper1(psContext, "dFdx", psInst, 0, 1);
        break;
    }
    case OPCODE_DERIV_RTY_COARSE:
    case OPCODE_DERIV_RTY_FINE:
    case OPCODE_DERIV_RTY:
    {
#ifdef _DEBUG
        AddIndentation(psContext);
        bcatcstr(glsl, "//DERIV_RTY\n");
#endif
        CallHelper1(psContext, "dFdy", psInst, 0, 1);
        break;
    }
    case OPCODE_LRP:
    {
#ifdef _DEBUG
        AddIndentation(psContext);
        bcatcstr(glsl, "//LRP\n");
#endif
        CallHelper3(psContext, "mix", psInst, 0, 2, 3, 1);
        break;
    }
    case OPCODE_DP2ADD:
    {
#ifdef _DEBUG
        AddIndentation(psContext);
        bcatcstr(glsl, "//DP2ADD\n");
#endif
        AddIndentation(psContext);
        BeginAssignment(psContext, &psInst->asOperands[0], TO_FLAG_FLOAT, psInst->bSaturate);
        bcatcstr(glsl, "dot(vec2(");
        TranslateOperand(psContext, &psInst->asOperands[1], TO_FLAG_FLOAT);
        bcatcstr(glsl, "), vec2(");
        TranslateOperand(psContext, &psInst->asOperands[2], TO_FLAG_FLOAT);
        bcatcstr(glsl, ")) + ");
        TranslateOperand(psContext, &psInst->asOperands[3], TO_FLAG_FLOAT);
        EndAssignment(psContext, &psInst->asOperands[0], TO_FLAG_FLOAT, psInst->bSaturate);
        bcatcstr(glsl, ";\n");
        break;
    }
    case OPCODE_POW:
    {
#ifdef _DEBUG
        AddIndentation(psContext);
        bcatcstr(glsl, "//POW\n");
#endif
        AddIndentation(psContext);
        BeginAssignment(psContext, &psInst->asOperands[0], TO_FLAG_FLOAT, psInst->bSaturate);
        bcatcstr(glsl, "pow(abs(");
        TranslateOperand(psContext, &psInst->asOperands[1], TO_FLAG_FLOAT);
        bcatcstr(glsl, "), ");
        TranslateOperand(psContext, &psInst->asOperands[2], TO_FLAG_FLOAT);
        bcatcstr(glsl, ")");
        EndAssignment(psContext, &psInst->asOperands[0], TO_FLAG_FLOAT, psInst->bSaturate);
        bcatcstr(glsl, ";\n");
        break;
    }

    case OPCODE_IMM_ATOMIC_ALLOC:
    {
#ifdef _DEBUG
        AddIndentation(psContext);
        bcatcstr(glsl, "//IMM_ATOMIC_ALLOC\n");
#endif
        AddIndentation(psContext);
        BeginAssignment(psContext, &psInst->asOperands[0], TO_FLAG_UNSIGNED_INTEGER, psInst->bSaturate);
        bcatcstr(glsl, "atomicCounterIncrement(");
        bformata(glsl, "UAV%d_counter)", psInst->asOperands[1].ui32RegisterNumber);
        EndAssignment(psContext, &psInst->asOperands[0], TO_FLAG_UNSIGNED_INTEGER, psInst->bSaturate);
        bcatcstr(glsl, ";\n");
        break;
    }
    case OPCODE_IMM_ATOMIC_CONSUME:
    {
#ifdef _DEBUG
        AddIndentation(psContext);
        bcatcstr(glsl, "//IMM_ATOMIC_CONSUME\n");
#endif
        AddIndentation(psContext);
        BeginAssignment(psContext, &psInst->asOperands[0], TO_FLAG_UNSIGNED_INTEGER, psInst->bSaturate);
        bcatcstr(glsl, "atomicCounterDecrement(");
        bformata(glsl, "UAV%d_counter)", psInst->asOperands[1].ui32RegisterNumber);
        EndAssignment(psContext, &psInst->asOperands[0], TO_FLAG_UNSIGNED_INTEGER, psInst->bSaturate);
        bcatcstr(glsl, ";\n");
        break;
    }

    case OPCODE_NOT:
    {
#ifdef _DEBUG
        AddIndentation(psContext);
        bcatcstr(glsl, "//INOT\n");
#endif
        AddIndentation(psContext);
        BeginAssignment(psContext, &psInst->asOperands[0], TO_FLAG_INTEGER, psInst->bSaturate);
        
        uint32_t uDestElemCount = GetNumSwizzleElements(&psInst->asOperands[0]);
        uint32_t uSrcElemCount = GetNumSwizzleElements(&psInst->asOperands[1]);

        if (uDestElemCount == uSrcElemCount)
        {
            bcatcstr(glsl, "~(");
            TranslateOperand(psContext, &psInst->asOperands[1], TO_FLAG_INTEGER);
            bcatcstr(glsl, ")");            
        }
        else
        {   
            ASSERT(uSrcElemCount > uDestElemCount);
            bformata(glsl, "ivec%d(~(", uSrcElemCount);            
            TranslateOperand(psContext, &psInst->asOperands[1], TO_FLAG_INTEGER);
            bcatcstr(glsl, "))");
            TranslateOperandSwizzle(psContext, &psInst->asOperands[0]);            
        }
         
        EndAssignment(psContext, &psInst->asOperands[0], TO_FLAG_INTEGER, psInst->bSaturate);
        bcatcstr(glsl, ";\n");
        break;
    }
    case OPCODE_XOR:
    {
#ifdef _DEBUG
        AddIndentation(psContext);
        bcatcstr(glsl, "//XOR\n");
#endif

        CallBinaryOp(psContext, "^", psInst, 0, 1, 2, TO_FLAG_INTEGER);
        break;
    }
    case OPCODE_RESINFO:
    {
        const RESINFO_RETURN_TYPE eResInfoReturnType = psInst->eResInfoReturnType;
        uint32_t destElemCount = GetNumSwizzleElements(&psInst->asOperands[0]);
        uint32_t destElem;
#ifdef _DEBUG
        AddIndentation(psContext);
        bcatcstr(glsl, "//RESINFO\n");
#endif

        //ASSERT(psInst->asOperands[0].eSelMode == OPERAND_4_COMPONENT_MASK_MODE);
        //ASSERT(psInst->asOperands[0].ui32CompMask == OPERAND_4_COMPONENT_MASK_ALL);




        for (destElem = 0; destElem < destElemCount; ++destElem)
        {
            const char* swizzle[] = {"x", "y", "z", "w"};
            uint32_t ui32ResInfoReturnTypeToFlags = (eResInfoReturnType == RESINFO_INSTRUCTION_RETURN_UINT) ? TO_FLAG_INTEGER /* currently it's treated as int */ : TO_FLAG_FLOAT;

            AddIndentation(psContext);
            if (destElemCount > 1)
            {
                BeginAssignmentEx(psContext, &psInst->asOperands[0], ui32ResInfoReturnTypeToFlags, psInst->bSaturate, swizzle[destElem]);
            }
            else
            {
                BeginAssignment(psContext, &psInst->asOperands[0], ui32ResInfoReturnTypeToFlags, psInst->bSaturate);
            }

            GetResInfoData(psContext, psInst, destElem);

            EndAssignment(psContext, &psInst->asOperands[0], ui32ResInfoReturnTypeToFlags, psInst->bSaturate);

            bcatcstr(glsl, ";\n");
        }

        break;
    }


    case OPCODE_DMAX:
    case OPCODE_DMIN:
    case OPCODE_DMUL:
    case OPCODE_DEQ:
    case OPCODE_DGE:
    case OPCODE_DLT:
    case OPCODE_DNE:
    case OPCODE_DMOV:
    case OPCODE_DMOVC:
    case OPCODE_DTOF:
    case OPCODE_FTOD:
    case OPCODE_DDIV:
    case OPCODE_DFMA:
    case OPCODE_DRCP:
    case OPCODE_MSAD:
    case OPCODE_DTOI:
    case OPCODE_DTOU:
    case OPCODE_ITOD:
    case OPCODE_UTOD:
    default:
    {
        ASSERT(0);
        break;
    }
    }
}

static int IsIntegerOpcode(OPCODE_TYPE eOpcode)
{
    switch (eOpcode)
    {
    case OPCODE_IADD:
    case OPCODE_IF:
    case OPCODE_IEQ:
    case OPCODE_IGE:
    case OPCODE_ILT:
    case OPCODE_IMAD:
    case OPCODE_IMAX:
    case OPCODE_IMIN:
    case OPCODE_IMUL:
    case OPCODE_INE:
    case OPCODE_INEG:
    case OPCODE_ISHL:
    case OPCODE_ISHR:
    case OPCODE_ITOF:
    case OPCODE_AND:
    case OPCODE_OR:
    {
        return 1;
    }
    default:
    {
        return 0;
    }
    }
}

int InstructionUsesRegister(const Instruction* psInst, const Operand* psOperand)
{
    uint32_t operand;
    for (operand = 0; operand < psInst->ui32NumOperands; ++operand)
    {
        if (psInst->asOperands[operand].eType == psOperand->eType)
        {
            if (psInst->asOperands[operand].ui32RegisterNumber == psOperand->ui32RegisterNumber)
            {
                if (CompareOperandSwizzles(&psInst->asOperands[operand], psOperand))
                {
                    return 1;
                }
            }
        }
    }
    return 0;
}

void MarkIntegerImmediates(HLSLCrossCompilerContext* psContext)
{
    const uint32_t count = psContext->psShader->ui32InstCount;
    Instruction* psInst = psContext->psShader->psInst;
    uint32_t i;

    for (i = 0; i < count; )
    {
        if (psInst[i].eOpcode == OPCODE_MOV && psInst[i].asOperands[1].eType == OPERAND_TYPE_IMMEDIATE32 &&
            psInst[i].asOperands[0].eType == OPERAND_TYPE_TEMP)
        {
            uint32_t k;

            for (k = i + 1; k < count; ++k)
            {
                if (psInst[k].eOpcode == OPCODE_ILT)
                {
                    k = k;
                }
                if (InstructionUsesRegister(&psInst[k], &psInst[i].asOperands[0]))
                {
                    if (IsIntegerOpcode(psInst[k].eOpcode))
                    {
                        psInst[i].asOperands[1].iIntegerImmediate = 1;
                    }

                    goto next_iteration;
                }
            }
        }
next_iteration:
        ++i;
    }
}
