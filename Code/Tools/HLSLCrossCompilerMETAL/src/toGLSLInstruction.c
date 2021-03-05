// Modifications copyright Amazon.com, Inc. or its affiliates
// Modifications copyright Crytek GmbH

#include "internal_includes/toGLSLInstruction.h"
#include <stdlib.h>
#include "bstrlib.h"
#include "hlslcc.h"
#include "internal_includes/debug.h"
#include "internal_includes/languages.h"
#include "internal_includes/toGLSLOperand.h"
#include "stdio.h"

extern void AddIndentation(HLSLCrossCompilerContext* psContext);
static int GLSLIsIntegerImmediateOpcode(OPCODE_TYPE eOpcode);

// Calculate the bits set in mask
static int GLSLWriteMaskToComponentCount(uint32_t writeMask)
{
    uint32_t count;
    // In HLSL bytecode writemask 0 also means everything
    if (writeMask == 0)
        return 4;

    // Count bits set
    // https://graphics.stanford.edu/~seander/bithacks.html#CountBitsSet64
    count = (writeMask * 0x200040008001ULL & 0x111111111111111ULL) % 0xf;

    return (int)count;
}

static uint32_t GLSLBuildComponentMaskFromElementCount(int count)
{
    // Translate numComponents into bitmask
    // 1 -> 1, 2 -> 3, 3 -> 7 and 4 -> 15
    return (1 << count) - 1;
}

// This function prints out the destination name, possible destination writemask, assignment operator
// and any possible conversions needed based on the eSrcType+ui32SrcElementCount (type and size of data expected to be coming in)
// As an output, pNeedsParenthesis will be filled with the amount of closing parenthesis needed
// and pSrcCount will be filled with the number of components expected
// ui32CompMask can be used to only write to 1 or more components (used by MOVC)
static void GLSLAddOpAssignToDestWithMask(HLSLCrossCompilerContext* psContext,
                                          const Operand* psDest,
                                          SHADER_VARIABLE_TYPE eSrcType,
                                          uint32_t ui32SrcElementCount,
                                          const char* szAssignmentOp,
                                          int* pNeedsParenthesis,
                                          uint32_t ui32CompMask)
{
    uint32_t ui32DestElementCount = GetNumSwizzleElementsWithMask(psDest, ui32CompMask);
    bstring glsl = *psContext->currentShaderString;
    SHADER_VARIABLE_TYPE eDestDataType = GetOperandDataType(psContext, psDest);
    ASSERT(pNeedsParenthesis != NULL);

    *pNeedsParenthesis = 0;

    TranslateOperandWithMask(psContext, psDest, TO_FLAG_DESTINATION, ui32CompMask);

    // Simple path: types match.
    if (eDestDataType == eSrcType)
    {
        // Cover cases where the HLSL language expects the rest of the components to be default-filled
        // eg. MOV r0, c0.x => Temp[0] = vec4(c0.x);
        if (ui32DestElementCount > ui32SrcElementCount)
        {
            bformata(glsl, " %s %s(", szAssignmentOp, GetConstructorForType(eDestDataType, ui32DestElementCount));
            *pNeedsParenthesis = 1;
        }
        else
            bformata(glsl, " %s ", szAssignmentOp);
        return;
    }

    switch (eDestDataType)
    {
        case SVT_INT:
            if (eSrcType == SVT_FLOAT && psContext->psShader->ui32MajorVersion > 3)
            {
                bformata(glsl, " %s floatBitsToInt(", szAssignmentOp);
                // Cover cases where the HLSL language expects the rest of the components to be default-filled
                if (ui32DestElementCount > ui32SrcElementCount)
                {
                    bformata(glsl, "%s(", GetConstructorForType(eSrcType, ui32DestElementCount));
                    (*pNeedsParenthesis)++;
                }
            }
            else
                bformata(glsl, " %s %s(", szAssignmentOp, GetConstructorForType(eDestDataType, ui32DestElementCount));
            break;
        case SVT_UINT:
            if (eSrcType == SVT_FLOAT && psContext->psShader->ui32MajorVersion > 3)
            {
                bformata(glsl, " %s floatBitsToUint(", szAssignmentOp);
                // Cover cases where the HLSL language expects the rest of the components to be default-filled
                if (ui32DestElementCount > ui32SrcElementCount)
                {
                    bformata(glsl, "%s(", GetConstructorForType(eSrcType, ui32DestElementCount));
                    (*pNeedsParenthesis)++;
                }
            }
            else
                bformata(glsl, " %s %s(", szAssignmentOp, GetConstructorForType(eDestDataType, ui32DestElementCount));
            break;

        case SVT_FLOAT:
            if (psContext->psShader->ui32MajorVersion > 3)
            {
                if (eSrcType == SVT_INT)
                    bformata(glsl, " %s intBitsToFloat(", szAssignmentOp);
                else
                    bformata(glsl, " %s uintBitsToFloat(", szAssignmentOp);
                // Cover cases where the HLSL language expects the rest of the components to be default-filled
                if (ui32DestElementCount > ui32SrcElementCount)
                {
                    bformata(glsl, "%s(", GetConstructorForType(eSrcType, ui32DestElementCount));
                    (*pNeedsParenthesis)++;
                }
            }
            else
                bformata(glsl, " %s %s(", szAssignmentOp, GetConstructorForType(eDestDataType, ui32DestElementCount));
            break;
        default:
            // TODO: Handle bools?
            break;
    }
    (*pNeedsParenthesis)++;
    return;
}

static void GLSLMETALAddAssignToDest(HLSLCrossCompilerContext* psContext,
                                     const Operand* psDest,
                                     SHADER_VARIABLE_TYPE eSrcType,
                                     uint32_t ui32SrcElementCount,
                                     int* pNeedsParenthesis)
{
    GLSLAddOpAssignToDestWithMask(psContext, psDest, eSrcType, ui32SrcElementCount, "=", pNeedsParenthesis, OPERAND_4_COMPONENT_MASK_ALL);
}

static void GLSLAddAssignPrologue(HLSLCrossCompilerContext* psContext, int numParenthesis)
{
    bstring glsl = *psContext->currentShaderString;
    while (numParenthesis != 0)
    {
        bcatcstr(glsl, ")");
        numParenthesis--;
    }
    bcatcstr(glsl, ";\n");
}
static uint32_t GLSLResourceReturnTypeToFlag(const RESOURCE_RETURN_TYPE eType)
{
    if (eType == RETURN_TYPE_SINT)
    {
        return TO_FLAG_INTEGER;
    }
    else if (eType == RETURN_TYPE_UINT)
    {
        return TO_FLAG_UNSIGNED_INTEGER;
    }
    else
    {
        return TO_FLAG_NONE;
    }
}

typedef enum
{
    GLSL_CMP_EQ,
    GLSL_CMP_LT,
    GLSL_CMP_GE,
    GLSL_CMP_NE,
} GLSLComparisonType;

static void GLSLAddComparision(HLSLCrossCompilerContext* psContext, Instruction* psInst, GLSLComparisonType eType, uint32_t typeFlag, Instruction* psNextInst)
{
    // Multiple cases to consider here:
    // For shader model <=3: all comparisons are floats
    // otherwise:
    // OPCODE_LT, _GT, _NE etc: inputs are floats, outputs UINT 0xffffffff or 0. typeflag: TO_FLAG_NONE
    // OPCODE_ILT, _IGT etc: comparisons are signed ints, outputs UINT 0xffffffff or 0 typeflag TO_FLAG_INTEGER
    // _ULT, UGT etc: inputs unsigned ints, outputs UINTs typeflag TO_FLAG_UNSIGNED_INTEGER
    //
    // Additional complexity: if dest swizzle element count is 1, we can use normal comparison operators, otherwise glsl intrinsics.

    bstring glsl = *psContext->currentShaderString;
    const uint32_t destElemCount = GetNumSwizzleElements(&psInst->asOperands[0]);
    const uint32_t s0ElemCount = GetNumSwizzleElements(&psInst->asOperands[1]);
    const uint32_t s1ElemCount = GetNumSwizzleElements(&psInst->asOperands[2]);

    int floatResult = 0;
    int needsParenthesis = 0;

    ASSERT(s0ElemCount == s1ElemCount || s1ElemCount == 1 || s0ElemCount == 1);
    if (s0ElemCount != s1ElemCount)
    {
        // Set the proper auto-expand flag is either argument is scalar
        typeFlag |= (TO_AUTO_EXPAND_TO_VEC2 << (max(s0ElemCount, s1ElemCount) - 2));
    }

    if (psContext->psShader->ui32MajorVersion < 4)
    {
        floatResult = 1;
    }

    if (destElemCount > 1)
    {
        const char* glslOpcode[] = {
            "equal",
            "lessThan",
            "greaterThanEqual",
            "notEqual",
        };

        AddIndentation(psContext);
        GLSLMETALAddAssignToDest(psContext, &psInst->asOperands[0], floatResult ? SVT_FLOAT : SVT_UINT, destElemCount, &needsParenthesis);

        bcatcstr(glsl, GetConstructorForType(floatResult ? SVT_FLOAT : SVT_UINT, destElemCount));
        bformata(glsl, "(%s(", glslOpcode[eType]);
        TranslateOperand(psContext, &psInst->asOperands[1], typeFlag);
        bcatcstr(glsl, ", ");
        TranslateOperand(psContext, &psInst->asOperands[2], typeFlag);
        bcatcstr(glsl, "))");
        if (!floatResult)
        {
            bcatcstr(glsl, " * 0xFFFFFFFFu");
        }

        GLSLAddAssignPrologue(psContext, needsParenthesis);
    }
    else
    {
        const char* glslOpcode[] = {
            "==",
            "<",
            ">=",
            "!=",
        };

        // Scalar compare

        // Optimization shortcut for the IGE+BREAKC_NZ combo:
        // First print out the if(cond)->break directly, and then
        // to guarantee correctness with side-effects, re-run
        // the actual comparison. In most cases, the second run will
        // be removed by the shader compiler optimizer pass (dead code elimination)
        // This also makes it easier for some GLSL optimizers to recognize the for loop.

        if (psInst->eOpcode == OPCODE_IGE && psNextInst && psNextInst->eOpcode == OPCODE_BREAKC &&
            (psInst->asOperands[0].ui32RegisterNumber == psNextInst->asOperands[0].ui32RegisterNumber))
        {
            AddIndentation(psContext);
            bcatcstr(glsl, "// IGE+BREAKC opt\n");
            AddIndentation(psContext);

            if (psNextInst->eBooleanTestType == INSTRUCTION_TEST_NONZERO)
                bcatcstr(glsl, "if ((");
            else
                bcatcstr(glsl, "if (!(");
            TranslateOperand(psContext, &psInst->asOperands[1], typeFlag);
            bformata(glsl, "%s ", glslOpcode[eType]);
            TranslateOperand(psContext, &psInst->asOperands[2], typeFlag);
            bcatcstr(glsl, ")) { break; }\n");

            // Mark the BREAKC instruction as already handled
            psNextInst->eOpcode = OPCODE_NOP;

            // Continue as usual
        }

        AddIndentation(psContext);
        GLSLMETALAddAssignToDest(psContext, &psInst->asOperands[0], floatResult ? SVT_FLOAT : SVT_UINT, destElemCount, &needsParenthesis);

        bcatcstr(glsl, "(");
        TranslateOperand(psContext, &psInst->asOperands[1], typeFlag);
        bformata(glsl, "%s", glslOpcode[eType]);
        TranslateOperand(psContext, &psInst->asOperands[2], typeFlag);
        if (floatResult)
        {
            bcatcstr(glsl, ") ? 1.0 : 0.0");
        }
        else
        {
            bcatcstr(glsl, ") ? 0xFFFFFFFFu : 0u");
        }
        GLSLAddAssignPrologue(psContext, needsParenthesis);
    }
}

static void GLSLAddMOVBinaryOp(HLSLCrossCompilerContext* psContext, const Operand* pDest, Operand* pSrc)
{
    int numParenthesis = 0;
    int destComponents = GetMaxComponentFromComponentMask(pDest);
    int srcSwizzleCount = GetNumSwizzleElements(pSrc);
    uint32_t writeMask = GetOperandWriteMask(pDest);

    const SHADER_VARIABLE_TYPE eSrcType = GetOperandDataTypeEx(psContext, pSrc, GetOperandDataType(psContext, pDest));
    uint32_t flags = SVTTypeToFlag(eSrcType);

    GLSLMETALAddAssignToDest(psContext, pDest, eSrcType, srcSwizzleCount, &numParenthesis);
    TranslateOperandWithMask(psContext, pSrc, flags, writeMask);

    GLSLAddAssignPrologue(psContext, numParenthesis);
}

static uint32_t GLSLElemCountToAutoExpandFlag(uint32_t elemCount)
{
    return TO_AUTO_EXPAND_TO_VEC2 << (elemCount - 2);
}

static void GLSLAddMOVCBinaryOp(HLSLCrossCompilerContext* psContext, const Operand* pDest, const Operand* src0, Operand* src1, Operand* src2)
{
    bstring glsl = *psContext->currentShaderString;
    uint32_t destElemCount = GetNumSwizzleElements(pDest);
    uint32_t s0ElemCount = GetNumSwizzleElements(src0);
    uint32_t s1ElemCount = GetNumSwizzleElements(src1);
    uint32_t s2ElemCount = GetNumSwizzleElements(src2);
    uint32_t destWriteMask = GetOperandWriteMask(pDest);
    uint32_t destElem;

    const SHADER_VARIABLE_TYPE eDestType = GetOperandDataType(psContext, pDest);
    const SHADER_VARIABLE_TYPE eSrc0Type = GetOperandDataType(psContext, src0);
    /*
    for each component in dest[.mask]
    if the corresponding component in src0 (POS-swizzle)
    has any bit set
    {
    copy this component (POS-swizzle) from src1 into dest
    }
    else
    {
    copy this component (POS-swizzle) from src2 into dest
    }
    endfor
    */

    /* Single-component conditional variable (src0) */
    if (s0ElemCount == 1 || IsSwizzleReplicated(src0))
    {
        int numParenthesis = 0;
        AddIndentation(psContext);
        GLSLMETALAddAssignToDest(psContext, pDest, eDestType, destElemCount, &numParenthesis);
        bcatcstr(glsl, "(");
        TranslateOperand(psContext, src0, TO_AUTO_BITCAST_TO_INT);
        if (s0ElemCount > 1)
            bcatcstr(glsl, ".x");
        if (psContext->psShader->ui32MajorVersion < 4)
        {
            // cmp opcode uses >= 0
            bcatcstr(glsl, " >= 0) ? ");
        }
        else
        {
            bcatcstr(glsl, " != 0) ? ");
        }

        if (s1ElemCount == 1 && destElemCount > 1)
            TranslateOperand(psContext, src1, SVTTypeToFlag(eDestType) | GLSLElemCountToAutoExpandFlag(destElemCount));
        else
            TranslateOperandWithMask(psContext, src1, SVTTypeToFlag(eDestType), destWriteMask);

        bcatcstr(glsl, " : ");
        if (s2ElemCount == 1 && destElemCount > 1)
            TranslateOperand(psContext, src2, SVTTypeToFlag(eDestType) | GLSLElemCountToAutoExpandFlag(destElemCount));
        else
            TranslateOperandWithMask(psContext, src2, SVTTypeToFlag(eDestType), destWriteMask);

        GLSLAddAssignPrologue(psContext, numParenthesis);
    }
    else
    {
        // TODO: We can actually do this in one op using mix().
        int srcElem = 0;
        for (destElem = 0; destElem < 4; ++destElem)
        {
            int numParenthesis = 0;
            if (pDest->eSelMode == OPERAND_4_COMPONENT_MASK_MODE && pDest->ui32CompMask != 0 && !(pDest->ui32CompMask & (1 << destElem)))
                continue;

            AddIndentation(psContext);
            GLSLAddOpAssignToDestWithMask(psContext, pDest, eDestType, 1, "=", &numParenthesis, 1 << destElem);
            bcatcstr(glsl, "(");
            TranslateOperandWithMask(psContext, src0, TO_AUTO_BITCAST_TO_INT, 1 << srcElem);
            if (psContext->psShader->ui32MajorVersion < 4)
            {
                // cmp opcode uses >= 0
                bcatcstr(glsl, " >= 0) ? ");
            }
            else
            {
                bcatcstr(glsl, " != 0) ? ");
            }

            TranslateOperandWithMask(psContext, src1, SVTTypeToFlag(eDestType), 1 << srcElem);
            bcatcstr(glsl, " : ");
            TranslateOperandWithMask(psContext, src2, SVTTypeToFlag(eDestType), 1 << srcElem);

            GLSLAddAssignPrologue(psContext, numParenthesis);

            srcElem++;
        }
    }
}

// Returns nonzero if operands are identical, only cares about temp registers currently.
static int GLSLAreTempOperandsIdentical(const Operand* psA, const Operand* psB)
{
    if (!psA || !psB)
        return 0;

    if (psA->eType != OPERAND_TYPE_TEMP || psB->eType != OPERAND_TYPE_TEMP)
        return 0;

    if (psA->eModifier != psB->eModifier)
        return 0;

    if (psA->iNumComponents != psB->iNumComponents)
        return 0;

    if (psA->ui32RegisterNumber != psB->ui32RegisterNumber)
        return 0;

    if (psA->eSelMode != psB->eSelMode)
        return 0;

    if (psA->eSelMode == OPERAND_4_COMPONENT_MASK_MODE && psA->ui32CompMask != psB->ui32CompMask)
        return 0;

    if (psA->eSelMode != OPERAND_4_COMPONENT_MASK_MODE && psA->ui32Swizzle != psB->ui32Swizzle)
        return 0;

    return 1;
}

// Returns nonzero if the operation is commutative
static int GLSLIsOperationCommutative(OPCODE_TYPE eOpCode)
{
    switch (eOpCode)
    {
        case OPCODE_DADD:
        case OPCODE_IADD:
        case OPCODE_ADD:
        case OPCODE_MUL:
        case OPCODE_IMUL:
        case OPCODE_OR:
        case OPCODE_AND:
            return 1;
        default:
            return 0;
    };
}

static void
GLSLCallBinaryOp(HLSLCrossCompilerContext* psContext, const char* name, Instruction* psInst, int dest, int src0, int src1, SHADER_VARIABLE_TYPE eDataType)
{
    bstring glsl = *psContext->currentShaderString;
    uint32_t src1SwizCount = GetNumSwizzleElements(&psInst->asOperands[src1]);
    uint32_t src0SwizCount = GetNumSwizzleElements(&psInst->asOperands[src0]);
    uint32_t dstSwizCount = GetNumSwizzleElements(&psInst->asOperands[dest]);
    uint32_t destMask = GetOperandWriteMask(&psInst->asOperands[dest]);
    int needsParenthesis = 0;

    AddIndentation(psContext);

    if (src1SwizCount == src0SwizCount == dstSwizCount)
    {
        // Optimization for readability (and to make for loops in WebGL happy): detect cases where either src == dest and emit +=, -= etc. instead.
        if (GLSLAreTempOperandsIdentical(&psInst->asOperands[dest], &psInst->asOperands[src0]) != 0)
        {
            GLSLAddOpAssignToDestWithMask(psContext, &psInst->asOperands[dest], eDataType, dstSwizCount, name, &needsParenthesis, OPERAND_4_COMPONENT_MASK_ALL);
            TranslateOperand(psContext, &psInst->asOperands[src1], SVTTypeToFlag(eDataType));
            GLSLAddAssignPrologue(psContext, needsParenthesis);
            return;
        }
        else if (GLSLAreTempOperandsIdentical(&psInst->asOperands[dest], &psInst->asOperands[src1]) != 0 && (GLSLIsOperationCommutative(psInst->eOpcode) != 0))
        {
            GLSLAddOpAssignToDestWithMask(psContext, &psInst->asOperands[dest], eDataType, dstSwizCount, name, &needsParenthesis, OPERAND_4_COMPONENT_MASK_ALL);
            TranslateOperand(psContext, &psInst->asOperands[src0], SVTTypeToFlag(eDataType));
            GLSLAddAssignPrologue(psContext, needsParenthesis);
            return;
        }
    }

    GLSLMETALAddAssignToDest(psContext, &psInst->asOperands[dest], eDataType, dstSwizCount, &needsParenthesis);

    TranslateOperandWithMask(psContext, &psInst->asOperands[src0], SVTTypeToFlag(eDataType), destMask);
    bformata(glsl, " %s ", name);
    TranslateOperandWithMask(psContext, &psInst->asOperands[src1], SVTTypeToFlag(eDataType), destMask);
    GLSLAddAssignPrologue(psContext, needsParenthesis);
}

static void GLSLCallTernaryOp(HLSLCrossCompilerContext* psContext,
                              const char* op1,
                              const char* op2,
                              Instruction* psInst,
                              int dest,
                              int src0,
                              int src1,
                              int src2,
                              uint32_t dataType)
{
    bstring glsl = *psContext->currentShaderString;
    uint32_t src2SwizCount = GetNumSwizzleElements(&psInst->asOperands[src2]);
    uint32_t src1SwizCount = GetNumSwizzleElements(&psInst->asOperands[src1]);
    uint32_t src0SwizCount = GetNumSwizzleElements(&psInst->asOperands[src0]);
    uint32_t dstSwizCount = GetNumSwizzleElements(&psInst->asOperands[dest]);
    uint32_t destMask = GetOperandWriteMask(&psInst->asOperands[dest]);

    uint32_t ui32Flags = dataType;
    int numParenthesis = 0;

    AddIndentation(psContext);

    GLSLMETALAddAssignToDest(psContext, &psInst->asOperands[dest], TypeFlagsToSVTType(dataType), dstSwizCount, &numParenthesis);

    TranslateOperandWithMask(psContext, &psInst->asOperands[src0], ui32Flags, destMask);
    bformata(glsl, " %s ", op1);
    TranslateOperandWithMask(psContext, &psInst->asOperands[src1], ui32Flags, destMask);
    bformata(glsl, " %s ", op2);
    TranslateOperandWithMask(psContext, &psInst->asOperands[src2], ui32Flags, destMask);
    GLSLAddAssignPrologue(psContext, numParenthesis);
}

static void GLSLCallHelper3(HLSLCrossCompilerContext* psContext,
                            const char* name,
                            Instruction* psInst,
                            int dest,
                            int src0,
                            int src1,
                            int src2,
                            int paramsShouldFollowWriteMask)
{
    uint32_t ui32Flags = TO_AUTO_BITCAST_TO_FLOAT;
    bstring glsl = *psContext->currentShaderString;
    uint32_t destMask = paramsShouldFollowWriteMask ? GetOperandWriteMask(&psInst->asOperands[dest]) : OPERAND_4_COMPONENT_MASK_ALL;
    uint32_t src2SwizCount = GetNumSwizzleElements(&psInst->asOperands[src2]);
    uint32_t src1SwizCount = GetNumSwizzleElements(&psInst->asOperands[src1]);
    uint32_t src0SwizCount = GetNumSwizzleElements(&psInst->asOperands[src0]);
    uint32_t dstSwizCount = GetNumSwizzleElements(&psInst->asOperands[dest]);
    int numParenthesis = 0;

    AddIndentation(psContext);

    GLSLMETALAddAssignToDest(psContext, &psInst->asOperands[dest], SVT_FLOAT, dstSwizCount, &numParenthesis);

    bformata(glsl, "%s(", name);
    numParenthesis++;
    TranslateOperandWithMask(psContext, &psInst->asOperands[src0], ui32Flags, destMask);
    bcatcstr(glsl, ", ");
    TranslateOperandWithMask(psContext, &psInst->asOperands[src1], ui32Flags, destMask);
    bcatcstr(glsl, ", ");
    TranslateOperandWithMask(psContext, &psInst->asOperands[src2], ui32Flags, destMask);
    GLSLAddAssignPrologue(psContext, numParenthesis);
}

static void
GLSLCallHelper2(HLSLCrossCompilerContext* psContext, const char* name, Instruction* psInst, int dest, int src0, int src1, int paramsShouldFollowWriteMask)
{
    uint32_t ui32Flags = TO_AUTO_BITCAST_TO_FLOAT;
    bstring glsl = *psContext->currentShaderString;
    uint32_t destMask = paramsShouldFollowWriteMask ? GetOperandWriteMask(&psInst->asOperands[dest]) : OPERAND_4_COMPONENT_MASK_ALL;
    uint32_t src1SwizCount = GetNumSwizzleElements(&psInst->asOperands[src1]);
    uint32_t src0SwizCount = GetNumSwizzleElements(&psInst->asOperands[src0]);
    uint32_t dstSwizCount = GetNumSwizzleElements(&psInst->asOperands[dest]);

    int isDotProduct = (strncmp(name, "dot", 3) == 0) ? 1 : 0;
    int numParenthesis = 0;

    AddIndentation(psContext);
    GLSLMETALAddAssignToDest(psContext, &psInst->asOperands[dest], SVT_FLOAT, isDotProduct ? 1 : dstSwizCount, &numParenthesis);

    bformata(glsl, "%s(", name);
    numParenthesis++;

    TranslateOperandWithMask(psContext, &psInst->asOperands[src0], ui32Flags, destMask);
    bcatcstr(glsl, ", ");
    TranslateOperandWithMask(psContext, &psInst->asOperands[src1], ui32Flags, destMask);

    GLSLAddAssignPrologue(psContext, numParenthesis);
}

static void
GLSLCallHelper2Int(HLSLCrossCompilerContext* psContext, const char* name, Instruction* psInst, int dest, int src0, int src1, int paramsShouldFollowWriteMask)
{
    uint32_t ui32Flags = TO_AUTO_BITCAST_TO_INT;
    bstring glsl = *psContext->currentShaderString;
    uint32_t src1SwizCount = GetNumSwizzleElements(&psInst->asOperands[src1]);
    uint32_t src0SwizCount = GetNumSwizzleElements(&psInst->asOperands[src0]);
    uint32_t dstSwizCount = GetNumSwizzleElements(&psInst->asOperands[dest]);
    uint32_t destMask = paramsShouldFollowWriteMask ? GetOperandWriteMask(&psInst->asOperands[dest]) : OPERAND_4_COMPONENT_MASK_ALL;
    int numParenthesis = 0;

    AddIndentation(psContext);

    GLSLMETALAddAssignToDest(psContext, &psInst->asOperands[dest], SVT_INT, dstSwizCount, &numParenthesis);

    bformata(glsl, "%s(", name);
    numParenthesis++;
    TranslateOperandWithMask(psContext, &psInst->asOperands[src0], ui32Flags, destMask);
    bcatcstr(glsl, ", ");
    TranslateOperandWithMask(psContext, &psInst->asOperands[src1], ui32Flags, destMask);
    GLSLAddAssignPrologue(psContext, numParenthesis);
}

static void
GLSLCallHelper2UInt(HLSLCrossCompilerContext* psContext, const char* name, Instruction* psInst, int dest, int src0, int src1, int paramsShouldFollowWriteMask)
{
    uint32_t ui32Flags = TO_AUTO_BITCAST_TO_UINT;
    bstring glsl = *psContext->currentShaderString;
    uint32_t src1SwizCount = GetNumSwizzleElements(&psInst->asOperands[src1]);
    uint32_t src0SwizCount = GetNumSwizzleElements(&psInst->asOperands[src0]);
    uint32_t dstSwizCount = GetNumSwizzleElements(&psInst->asOperands[dest]);
    uint32_t destMask = paramsShouldFollowWriteMask ? GetOperandWriteMask(&psInst->asOperands[dest]) : OPERAND_4_COMPONENT_MASK_ALL;
    int numParenthesis = 0;

    AddIndentation(psContext);

    GLSLMETALAddAssignToDest(psContext, &psInst->asOperands[dest], SVT_UINT, dstSwizCount, &numParenthesis);

    bformata(glsl, "%s(", name);
    numParenthesis++;
    TranslateOperandWithMask(psContext, &psInst->asOperands[src0], ui32Flags, destMask);
    bcatcstr(glsl, ", ");
    TranslateOperandWithMask(psContext, &psInst->asOperands[src1], ui32Flags, destMask);
    GLSLAddAssignPrologue(psContext, numParenthesis);
}

static void GLSLCallHelper1(HLSLCrossCompilerContext* psContext, const char* name, Instruction* psInst, int dest, int src0, int paramsShouldFollowWriteMask)
{
    uint32_t ui32Flags = TO_AUTO_BITCAST_TO_FLOAT;
    bstring glsl = *psContext->currentShaderString;
    uint32_t src0SwizCount = GetNumSwizzleElements(&psInst->asOperands[src0]);
    uint32_t dstSwizCount = GetNumSwizzleElements(&psInst->asOperands[dest]);
    uint32_t destMask = paramsShouldFollowWriteMask ? GetOperandWriteMask(&psInst->asOperands[dest]) : OPERAND_4_COMPONENT_MASK_ALL;
    int numParenthesis = 0;

    AddIndentation(psContext);

    GLSLMETALAddAssignToDest(psContext, &psInst->asOperands[dest], SVT_FLOAT, dstSwizCount, &numParenthesis);

    bformata(glsl, "%s(", name);
    numParenthesis++;
    TranslateOperandWithMask(psContext, &psInst->asOperands[src0], ui32Flags, destMask);
    GLSLAddAssignPrologue(psContext, numParenthesis);
}

// Result is an int.
static void GLSLCallHelper1Int(HLSLCrossCompilerContext* psContext,
                               const char* name,
                               Instruction* psInst,
                               const int dest,
                               const int src0,
                               int paramsShouldFollowWriteMask)
{
    uint32_t ui32Flags = TO_AUTO_BITCAST_TO_INT;
    bstring glsl = *psContext->currentShaderString;
    uint32_t src0SwizCount = GetNumSwizzleElements(&psInst->asOperands[src0]);
    uint32_t dstSwizCount = GetNumSwizzleElements(&psInst->asOperands[dest]);
    uint32_t destMask = paramsShouldFollowWriteMask ? GetOperandWriteMask(&psInst->asOperands[dest]) : OPERAND_4_COMPONENT_MASK_ALL;
    int numParenthesis = 0;

    AddIndentation(psContext);

    GLSLMETALAddAssignToDest(psContext, &psInst->asOperands[dest], SVT_INT, dstSwizCount, &numParenthesis);

    bformata(glsl, "%s(", name);
    numParenthesis++;
    TranslateOperandWithMask(psContext, &psInst->asOperands[src0], ui32Flags, destMask);
    GLSLAddAssignPrologue(psContext, numParenthesis);
}

static void GLSLTranslateTexelFetch(HLSLCrossCompilerContext* psContext, Instruction* psInst, ResourceBinding* psBinding, bstring glsl)
{
    int numParenthesis = 0;
    uint32_t destCount = GetNumSwizzleElements(&psInst->asOperands[0]);
    AddIndentation(psContext);
    GLSLMETALAddAssignToDest(psContext, &psInst->asOperands[0], TypeFlagsToSVTType(GLSLResourceReturnTypeToFlag(psBinding->ui32ReturnType)), 4,
                             &numParenthesis);
    bcatcstr(glsl, "texelFetch(");

    switch (psBinding->eDimension)
    {
        case REFLECT_RESOURCE_DIMENSION_TEXTURE1D:
        case REFLECT_RESOURCE_DIMENSION_BUFFER:
        {
            TranslateOperand(psContext, &psInst->asOperands[2], TO_FLAG_NONE);
            bcatcstr(glsl, ", ");
            TranslateOperandWithMask(psContext, &psInst->asOperands[1], TO_FLAG_INTEGER, OPERAND_4_COMPONENT_MASK_X);
            if (psBinding->eDimension != REFLECT_RESOURCE_DIMENSION_BUFFER)
                bcatcstr(glsl, ", 0");  // Buffers don't have LOD
            bcatcstr(glsl, ")");
            break;
        }
        case REFLECT_RESOURCE_DIMENSION_TEXTURE2DARRAY:
        case REFLECT_RESOURCE_DIMENSION_TEXTURE3D:
        {
            TranslateOperand(psContext, &psInst->asOperands[2], TO_FLAG_NONE);
            bcatcstr(glsl, ", ");
            TranslateOperandWithMask(psContext, &psInst->asOperands[1], TO_FLAG_INTEGER | TO_AUTO_EXPAND_TO_VEC3, 7 /* .xyz */);
            bcatcstr(glsl, ", 0)");
            break;
        }
        case REFLECT_RESOURCE_DIMENSION_TEXTURE2D:
        case REFLECT_RESOURCE_DIMENSION_TEXTURE1DARRAY:
        {
            TranslateOperand(psContext, &psInst->asOperands[2], TO_FLAG_NONE);
            bcatcstr(glsl, ", ");
            TranslateOperandWithMask(psContext, &psInst->asOperands[1], TO_FLAG_INTEGER | TO_AUTO_EXPAND_TO_VEC2, 3 /* .xy */);
            bcatcstr(glsl, ", 0)");
            break;
        }
        case REFLECT_RESOURCE_DIMENSION_TEXTURE2DMS:  // TODO does this make any sense at all?
        {
            ASSERT(psInst->eOpcode == OPCODE_LD_MS);
            TranslateOperand(psContext, &psInst->asOperands[2], TO_FLAG_NONE);
            bcatcstr(glsl, ", ");
            TranslateOperandWithMask(psContext, &psInst->asOperands[1], TO_FLAG_INTEGER | TO_AUTO_EXPAND_TO_VEC2, 3 /* .xy */);
            bcatcstr(glsl, ", ");
            TranslateOperandWithMask(psContext, &psInst->asOperands[3], TO_FLAG_INTEGER, OPERAND_4_COMPONENT_MASK_X);
            bcatcstr(glsl, ")");
            break;
        }
        case REFLECT_RESOURCE_DIMENSION_TEXTURE2DMSARRAY:
        {
            ASSERT(psInst->eOpcode == OPCODE_LD_MS);
            TranslateOperand(psContext, &psInst->asOperands[2], TO_FLAG_NONE);
            bcatcstr(glsl, ", ");
            TranslateOperandWithMask(psContext, &psInst->asOperands[1], TO_FLAG_INTEGER | TO_AUTO_EXPAND_TO_VEC3, 7 /* .xyz */);
            bcatcstr(glsl, ", ");
            TranslateOperandWithMask(psContext, &psInst->asOperands[3], TO_FLAG_INTEGER, OPERAND_4_COMPONENT_MASK_X);
            bcatcstr(glsl, ")");
            break;
        }
        case REFLECT_RESOURCE_DIMENSION_TEXTURECUBE:
        case REFLECT_RESOURCE_DIMENSION_TEXTURECUBEARRAY:
        case REFLECT_RESOURCE_DIMENSION_BUFFEREX:
        default:
        {
            ASSERT(0);
            break;
        }
    }

    AddSwizzleUsingElementCount(psContext, destCount);
    GLSLAddAssignPrologue(psContext, numParenthesis);
}

static void GLSLTranslateTexelFetchOffset(HLSLCrossCompilerContext* psContext, Instruction* psInst, ResourceBinding* psBinding, bstring glsl)
{
    int numParenthesis = 0;
    uint32_t destCount = GetNumSwizzleElements(&psInst->asOperands[0]);
    AddIndentation(psContext);
    GLSLMETALAddAssignToDest(psContext, &psInst->asOperands[0], TypeFlagsToSVTType(GLSLResourceReturnTypeToFlag(psBinding->ui32ReturnType)), 4,
                             &numParenthesis);

    bcatcstr(glsl, "texelFetchOffset(");

    switch (psBinding->eDimension)
    {
        case REFLECT_RESOURCE_DIMENSION_TEXTURE1D:
        {
            TranslateOperand(psContext, &psInst->asOperands[2], TO_FLAG_NONE);
            bcatcstr(glsl, ", ");
            TranslateOperandWithMask(psContext, &psInst->asOperands[1], TO_FLAG_INTEGER, OPERAND_4_COMPONENT_MASK_X);
            bformata(glsl, ", 0, %d)", psInst->iUAddrOffset);
            break;
        }
        case REFLECT_RESOURCE_DIMENSION_TEXTURE2DARRAY:
        {
            TranslateOperand(psContext, &psInst->asOperands[2], TO_FLAG_NONE);
            bcatcstr(glsl, ", ");
            TranslateOperandWithMask(psContext, &psInst->asOperands[1], TO_FLAG_INTEGER | TO_AUTO_EXPAND_TO_VEC3, 7 /* .xyz */);
            bformata(glsl, ", 0, ivec2(%d, %d))", psInst->iUAddrOffset, psInst->iVAddrOffset);
            break;
        }
        case REFLECT_RESOURCE_DIMENSION_TEXTURE3D:
        {
            TranslateOperand(psContext, &psInst->asOperands[2], TO_FLAG_NONE);
            bcatcstr(glsl, ", ");
            TranslateOperandWithMask(psContext, &psInst->asOperands[1], TO_FLAG_INTEGER | TO_AUTO_EXPAND_TO_VEC3, 7 /* .xyz */);
            bformata(glsl, ", 0, ivec3(%d, %d, %d))", psInst->iUAddrOffset, psInst->iVAddrOffset, psInst->iWAddrOffset);
            break;
        }
        case REFLECT_RESOURCE_DIMENSION_TEXTURE2D:
        {
            TranslateOperand(psContext, &psInst->asOperands[2], TO_FLAG_NONE);
            bcatcstr(glsl, ", ");
            TranslateOperandWithMask(psContext, &psInst->asOperands[1], TO_FLAG_INTEGER | TO_AUTO_EXPAND_TO_VEC2, 3 /* .xy */);
            bformata(glsl, ", 0, ivec2(%d, %d))", psInst->iUAddrOffset, psInst->iVAddrOffset);
            break;
        }
        case REFLECT_RESOURCE_DIMENSION_TEXTURE1DARRAY:
        {
            TranslateOperand(psContext, &psInst->asOperands[2], TO_FLAG_NONE);
            bcatcstr(glsl, ", ");
            TranslateOperandWithMask(psContext, &psInst->asOperands[1], TO_FLAG_INTEGER | TO_AUTO_EXPAND_TO_VEC2, 3 /* .xy */);
            bformata(glsl, ", 0, int(%d))", psInst->iUAddrOffset);
            break;
        }
        case REFLECT_RESOURCE_DIMENSION_BUFFER:
        case REFLECT_RESOURCE_DIMENSION_TEXTURE2DMS:
        case REFLECT_RESOURCE_DIMENSION_TEXTURE2DMSARRAY:
        case REFLECT_RESOURCE_DIMENSION_TEXTURECUBE:
        case REFLECT_RESOURCE_DIMENSION_TEXTURECUBEARRAY:
        case REFLECT_RESOURCE_DIMENSION_BUFFEREX:
        default:
        {
            ASSERT(0);
            break;
        }
    }

    AddSwizzleUsingElementCount(psContext, destCount);
    GLSLAddAssignPrologue(psContext, numParenthesis);
}

// Makes sure the texture coordinate swizzle is appropriate for the texture type.
// i.e. vecX for X-dimension texture.
// Currently supports floating point coord only, so not used for texelFetch.
static void GLSLTranslateTexCoord(HLSLCrossCompilerContext* psContext, const RESOURCE_DIMENSION eResDim, Operand* psTexCoordOperand)
{
    int numParenthesis = 0;
    uint32_t flags = TO_AUTO_BITCAST_TO_FLOAT;
    bstring glsl = *psContext->currentShaderString;
    uint32_t opMask = OPERAND_4_COMPONENT_MASK_ALL;

    switch (eResDim)
    {
        case RESOURCE_DIMENSION_TEXTURE1D:
        {
            // Vec1 texcoord. Mask out the other components.
            opMask = OPERAND_4_COMPONENT_MASK_X;
            break;
        }
        case RESOURCE_DIMENSION_TEXTURE2D:
        case RESOURCE_DIMENSION_TEXTURE1DARRAY:
        {
            // Vec2 texcoord. Mask out the other components.
            opMask = OPERAND_4_COMPONENT_MASK_X | OPERAND_4_COMPONENT_MASK_Y;
            flags |= TO_AUTO_EXPAND_TO_VEC2;
            break;
        }
        case RESOURCE_DIMENSION_TEXTURECUBE:
        case RESOURCE_DIMENSION_TEXTURE3D:
        case RESOURCE_DIMENSION_TEXTURE2DARRAY:
        {
            // Vec3 texcoord. Mask out the other components.
            opMask = OPERAND_4_COMPONENT_MASK_X | OPERAND_4_COMPONENT_MASK_Y | OPERAND_4_COMPONENT_MASK_Z;
            flags |= TO_AUTO_EXPAND_TO_VEC3;
            break;
        }
        case RESOURCE_DIMENSION_TEXTURECUBEARRAY:
        {
            flags |= TO_AUTO_EXPAND_TO_VEC4;
            break;
        }
        default:
        {
            ASSERT(0);
            break;
        }
    }

    // FIXME detect when integer coords are needed.
    TranslateOperandWithMask(psContext, psTexCoordOperand, flags, opMask);
}

static int GLSLGetNumTextureDimensions(HLSLCrossCompilerContext* psContext, const RESOURCE_DIMENSION eResDim)
{
    int constructor = 0;
    bstring glsl = *psContext->currentShaderString;

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

void GetResInfoData(HLSLCrossCompilerContext* psContext, Instruction* psInst, int index, int destElem)
{
    bstring glsl = *psContext->currentShaderString;
    int numParenthesis = 0;
    const RESINFO_RETURN_TYPE eResInfoReturnType = psInst->eResInfoReturnType;
    const RESOURCE_DIMENSION eResDim = psContext->psShader->aeResourceDims[psInst->asOperands[2].ui32RegisterNumber];

    AddIndentation(psContext);
    GLSLAddOpAssignToDestWithMask(psContext, &psInst->asOperands[0], eResInfoReturnType == RESINFO_INSTRUCTION_RETURN_UINT ? SVT_UINT : SVT_FLOAT, 1, "=",
                                  &numParenthesis, 1 << destElem);

    //[width, height, depth or array size, total-mip-count]
    if (index < 3)
    {
        int dim = GLSLGetNumTextureDimensions(psContext, eResDim);
        bcatcstr(glsl, "(");
        if (dim < (index + 1))
        {
            bcatcstr(glsl, eResInfoReturnType == RESINFO_INSTRUCTION_RETURN_UINT ? "0u" : "0.0");
        }
        else
        {
            if (eResInfoReturnType == RESINFO_INSTRUCTION_RETURN_UINT)
            {
                bformata(glsl, "uvec%d(textureSize(", dim);
            }
            else if (eResInfoReturnType == RESINFO_INSTRUCTION_RETURN_RCPFLOAT)
            {
                bformata(glsl, "vec%d(1.0) / vec%d(textureSize(", dim, dim);
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

        bcatcstr(glsl, ")");
    }
    else
    {
        if (eResInfoReturnType == RESINFO_INSTRUCTION_RETURN_UINT)
            bcatcstr(glsl, "uint(");
        else
            bcatcstr(glsl, "float(");
        bcatcstr(glsl, "textureQueryLevels(");
        TranslateOperand(psContext, &psInst->asOperands[2], TO_FLAG_NONE);
        bcatcstr(glsl, "))");
    }
    GLSLAddAssignPrologue(psContext, numParenthesis);
}

#define TEXSMP_FLAG_NONE 0x0
#define TEXSMP_FLAG_LOD 0x1  // LOD comes from operand
#define TEXSMP_FLAG_DEPTHCOMPARE 0x2
#define TEXSMP_FLAG_FIRSTLOD 0x4  // LOD is 0
#define TEXSMP_FLAG_BIAS 0x8
#define TEXSMP_FLAGS_GRAD 0x10

// TODO FIXME: non-float samplers!
static void GLSLTranslateTextureSample(HLSLCrossCompilerContext* psContext, Instruction* psInst, uint32_t ui32Flags)
{
    bstring glsl = *psContext->currentShaderString;
    int numParenthesis = 0;

    const char* funcName = "texture";
    const char* offset = "";
    const char* depthCmpCoordType = "";
    const char* gradSwizzle = "";

    uint32_t ui32NumOffsets = 0;

    const RESOURCE_DIMENSION eResDim = psContext->psShader->aeResourceDims[psInst->asOperands[2].ui32RegisterNumber];

    const int iHaveOverloadedTexFuncs = HaveOverloadedTextureFuncs(psContext->psShader->eTargetLanguage);

    const int useCombinedTextureSamplers = (psContext->flags & HLSLCC_FLAG_COMBINE_TEXTURE_SAMPLERS) ? 1 : 0;

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
                if (ui32Flags & TEXSMP_FLAG_DEPTHCOMPARE)
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
                if (ui32Flags & TEXSMP_FLAG_DEPTHCOMPARE)
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
            if (ui32Flags & TEXSMP_FLAG_DEPTHCOMPARE)
            {
                SHADER_VARIABLE_TYPE dataType = SVT_FLOAT;  // TODO!!
                // Special. Reference is a separate argument.
                AddIndentation(psContext);

                GLSLMETALAddAssignToDest(psContext, &psInst->asOperands[0], dataType, 1, &numParenthesis);
                if (ui32Flags & (TEXSMP_FLAG_LOD | TEXSMP_FLAG_FIRSTLOD))
                {
                    bcatcstr(glsl, "textureLod(");
                }
                else
                {
                    bcatcstr(glsl, "texture(");
                }
                if (!useCombinedTextureSamplers)
                    ResourceName(glsl, psContext, RGROUP_TEXTURE, psInst->asOperands[2].ui32RegisterNumber, (ui32Flags & TEXSMP_FLAG_DEPTHCOMPARE) ? 1 : 0);
                else
                    bconcat(glsl, TextureSamplerName(&psContext->psShader->sInfo, psInst->asOperands[2].ui32RegisterNumber,
                                                     psInst->asOperands[3].ui32RegisterNumber, (ui32Flags & TEXSMP_FLAG_DEPTHCOMPARE) ? 1 : 0));
                bcatcstr(glsl, ",");
                GLSLTranslateTexCoord(psContext, eResDim, &psInst->asOperands[1]);
                bcatcstr(glsl, ",");
                //.z = reference.
                TranslateOperand(psContext, &psInst->asOperands[4], TO_AUTO_BITCAST_TO_FLOAT);

                if (ui32Flags & TEXSMP_FLAG_FIRSTLOD)
                {
                    bcatcstr(glsl, ", 0.0");
                }

                bcatcstr(glsl, ")");
                // Doesn't make any sense to do swizzles here, depth comparison returns a scalar.
                GLSLAddAssignPrologue(psContext, numParenthesis);
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

    if (ui32Flags & TEXSMP_FLAG_DEPTHCOMPARE)
    {
        // For non-cubeMap Arrays the reference value comes from the
        // texture coord vector in GLSL. For cubmap arrays there is a
        // separate parameter.
        // It is always separate paramter in HLSL.
        SHADER_VARIABLE_TYPE dataType = SVT_FLOAT;  // TODO!!
        AddIndentation(psContext);
        GLSLMETALAddAssignToDest(psContext, &psInst->asOperands[0], dataType, GetNumSwizzleElements(&psInst->asOperands[2]), &numParenthesis);
        if (ui32Flags & (TEXSMP_FLAG_LOD | TEXSMP_FLAG_FIRSTLOD))
        {
            bformata(glsl, "%sLod%s(", funcName, offset);
        }
        else
        {
            bformata(glsl, "%s%s(", funcName, offset);
        }
        if (!useCombinedTextureSamplers)
            ResourceName(glsl, psContext, RGROUP_TEXTURE, psInst->asOperands[2].ui32RegisterNumber, 1);
        else
            bconcat(glsl,
                    TextureSamplerName(&psContext->psShader->sInfo, psInst->asOperands[2].ui32RegisterNumber, psInst->asOperands[3].ui32RegisterNumber, 1));
        bformata(glsl, ", %s(", depthCmpCoordType);
        GLSLTranslateTexCoord(psContext, eResDim, &psInst->asOperands[1]);
        bcatcstr(glsl, ",");
        //.z = reference.
        TranslateOperand(psContext, &psInst->asOperands[4], TO_AUTO_BITCAST_TO_FLOAT);
        bcatcstr(glsl, ")");

        if (ui32Flags & TEXSMP_FLAG_FIRSTLOD)
        {
            bcatcstr(glsl, ", 0.0");
        }

        bcatcstr(glsl, ")");
    }
    else
    {
        SHADER_VARIABLE_TYPE dataType = SVT_FLOAT;  // TODO!!
        AddIndentation(psContext);
        GLSLMETALAddAssignToDest(psContext, &psInst->asOperands[0], dataType, GetNumSwizzleElements(&psInst->asOperands[2]), &numParenthesis);

        if (ui32Flags & (TEXSMP_FLAG_LOD | TEXSMP_FLAG_FIRSTLOD))
        {
            bformata(glsl, "%sLod%s(", funcName, offset);
        }
        else if (ui32Flags & TEXSMP_FLAGS_GRAD)
        {
            bformata(glsl, "%sGrad%s(", funcName, offset);
        }
        else
        {
            bformata(glsl, "%s%s(", funcName, offset);
        }
        if (!useCombinedTextureSamplers)
            TranslateOperand(psContext, &psInst->asOperands[2], TO_FLAG_NONE);  // resource
        else
            bconcat(glsl,
                    TextureSamplerName(&psContext->psShader->sInfo, psInst->asOperands[2].ui32RegisterNumber, psInst->asOperands[3].ui32RegisterNumber, 0));
        bcatcstr(glsl, ", ");
        GLSLTranslateTexCoord(psContext, eResDim, &psInst->asOperands[1]);

        if (ui32Flags & (TEXSMP_FLAG_LOD))
        {
            bcatcstr(glsl, ", ");
            TranslateOperand(psContext, &psInst->asOperands[4], TO_AUTO_BITCAST_TO_FLOAT);
            if (psContext->psShader->ui32MajorVersion < 4)
            {
                bcatcstr(glsl, ".w");
            }
        }
        else if (ui32Flags & TEXSMP_FLAG_FIRSTLOD)
        {
            bcatcstr(glsl, ", 0.0");
        }
        else if (ui32Flags & TEXSMP_FLAGS_GRAD)
        {
            bcatcstr(glsl, ", vec4(");
            TranslateOperand(psContext, &psInst->asOperands[4], TO_AUTO_BITCAST_TO_FLOAT);  // dx
            bcatcstr(glsl, ")");
            bcatcstr(glsl, gradSwizzle);
            bcatcstr(glsl, ", vec4(");
            TranslateOperand(psContext, &psInst->asOperands[5], TO_AUTO_BITCAST_TO_FLOAT);  // dy
            bcatcstr(glsl, ")");
            bcatcstr(glsl, gradSwizzle);
        }

        if (psInst->bAddressOffset)
        {
            if (ui32NumOffsets == 1)
            {
                bformata(glsl, ", %d", psInst->iUAddrOffset);
            }
            else if (ui32NumOffsets == 2)
            {
                bformata(glsl, ", ivec2(%d, %d)", psInst->iUAddrOffset, psInst->iVAddrOffset);
            }
            else if (ui32NumOffsets == 3)
            {
                bformata(glsl, ", ivec3(%d, %d, %d)", psInst->iUAddrOffset, psInst->iVAddrOffset, psInst->iWAddrOffset);
            }
        }

        if (ui32Flags & (TEXSMP_FLAG_BIAS))
        {
            bcatcstr(glsl, ", ");
            TranslateOperand(psContext, &psInst->asOperands[4], TO_AUTO_BITCAST_TO_FLOAT);
        }

        bcatcstr(glsl, ")");
    }

    if (!(ui32Flags & TEXSMP_FLAG_DEPTHCOMPARE))
    {
        // iWriteMaskEnabled is forced off during DecodeOperand because swizzle on sampler uniforms
        // does not make sense. But need to re-enable to correctly swizzle this particular instruction.
        psInst->asOperands[2].iWriteMaskEnabled = 1;
        TranslateOperandSwizzleWithMask(psContext, &psInst->asOperands[2], GetOperandWriteMask(&psInst->asOperands[0]));
    }
    GLSLAddAssignPrologue(psContext, numParenthesis);
}

static ShaderVarType* GLSLLookupStructuredVar(HLSLCrossCompilerContext* psContext, Operand* psResource, Operand* psByteOffset, uint32_t ui32Component)
{
    ConstantBuffer* psCBuf = NULL;
    ShaderVarType* psVarType = NULL;
    uint32_t aui32Swizzle[4] = {OPERAND_4_COMPONENT_X};
    int byteOffset = ((int*)psByteOffset->afImmediates)[0] + 4 * ui32Component;
    int vec4Offset = 0;
    int32_t index = -1;
    int32_t rebase = -1;
    int found;

    ASSERT(psByteOffset->eType == OPERAND_TYPE_IMMEDIATE32);
    // TODO: multi-component stores and vector writes need testing.

    // aui32Swizzle[0] = psInst->asOperands[0].aui32Swizzle[component];

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
            // dcl_tgsm_structured defines the amount of memory and a stride.
            ASSERT(psResource->ui32RegisterNumber < MAX_GROUPSHARED);
            return &psContext->psShader->sGroupSharedVarType[psResource->ui32RegisterNumber];
        }
        default:
            ASSERT(0);
            break;
    }

    found = GetShaderVarFromOffset(vec4Offset, aui32Swizzle, psCBuf, &psVarType, &index, &rebase);
    ASSERT(found);

    return psVarType;
}

static void GLSLTranslateShaderStorageStore(HLSLCrossCompilerContext* psContext, Instruction* psInst)
{
    bstring glsl = *psContext->currentShaderString;
    ShaderVarType* psVarType = NULL;
    uint32_t ui32DataTypeFlag = TO_FLAG_INTEGER;
    int component;
    int srcComponent = 0;

    Operand* psDest = 0;
    Operand* psDestAddr = 0;
    Operand* psDestByteOff = 0;
    Operand* psSrc = 0;
    int structured = 0;
    int groupshared = 0;

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
        const char* swizzleString[] = {".x", ".y", ".z", ".w"};
        ASSERT(psInst->asOperands[0].eSelMode == OPERAND_4_COMPONENT_MASK_MODE);
        if (psInst->asOperands[0].ui32CompMask & (1 << component))
        {
            SHADER_VARIABLE_TYPE eSrcDataType = GetOperandDataType(psContext, psSrc);

            if (structured && psDest->eType != OPERAND_TYPE_THREAD_GROUP_SHARED_MEMORY)
            {
                psVarType = GLSLLookupStructuredVar(psContext, psDest, psDestByteOff, component);
            }

            AddIndentation(psContext);

            if (structured && psDest->eType == OPERAND_TYPE_RESOURCE)
            {
                bformata(glsl, "StructuredRes%d", psDest->ui32RegisterNumber);
            }
            else
            {
                TranslateOperand(psContext, psDest, TO_FLAG_DESTINATION | TO_FLAG_NAME_ONLY);
            }
            bformata(glsl, "[");
            if (structured)  // Dest address and dest byte offset
            {
                if (psDest->eType == OPERAND_TYPE_THREAD_GROUP_SHARED_MEMORY)
                {
                    TranslateOperand(psContext, psDestAddr, TO_FLAG_INTEGER | TO_FLAG_UNSIGNED_INTEGER);
                    bformata(glsl, "].value[");
                    TranslateOperand(psContext, psDestByteOff, TO_FLAG_INTEGER | TO_FLAG_UNSIGNED_INTEGER);
                    bformata(glsl, "/4u ");  // bytes to floats
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

            // RAW: change component using index offset
            if (!structured || (psDest->eType == OPERAND_TYPE_THREAD_GROUP_SHARED_MEMORY))
            {
                bformata(glsl, " + %d", component);
            }

            bformata(glsl, "]");

            if (structured && psDest->eType != OPERAND_TYPE_THREAD_GROUP_SHARED_MEMORY)
            {
                if (strcmp(psVarType->Name, "$Element") != 0)
                {
                    bformata(glsl, ".%s", psVarType->Name);
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
                // TGSM always uint
                bformata(glsl, " = (");
                if (GetNumSwizzleElements(psSrc) > 1)
                    TranslateOperandWithMask(psContext, psSrc, flags, 1 << (srcComponent++));
                else
                    TranslateOperandWithMask(psContext, psSrc, flags, OPERAND_4_COMPONENT_MASK_X);
            }
            else
            {
                // Dest type is currently always a uint array.
                bformata(glsl, " = (");
                if (GetNumSwizzleElements(psSrc) > 1)
                    TranslateOperandWithMask(psContext, psSrc, TO_FLAG_UNSIGNED_INTEGER, 1 << (srcComponent++));
                else
                    TranslateOperandWithMask(psContext, psSrc, TO_FLAG_UNSIGNED_INTEGER, OPERAND_4_COMPONENT_MASK_X);
            }

            // Double takes an extra slot.
            if (psVarType && psVarType->Type == SVT_DOUBLE)
            {
                if (structured && psDest->eType == OPERAND_TYPE_THREAD_GROUP_SHARED_MEMORY)
                    bcatcstr(glsl, ")");
                component++;
            }

            bformata(glsl, ");\n");
        }
    }
}
static void GLSLTranslateShaderStorageLoad(HLSLCrossCompilerContext* psContext, Instruction* psInst)
{
    bstring glsl = *psContext->currentShaderString;
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
        int numParenthesis = 0;
        int firstItemAdded = 0;
        uint32_t destCount = GetNumSwizzleElements(psDest);
        uint32_t destMask = GetOperandWriteMask(psDest);
        AddIndentation(psContext);
        GLSLMETALAddAssignToDest(psContext, psDest, SVT_UINT, destCount, &numParenthesis);
        if (destCount > 1)
        {
            bformata(glsl, "%s(", GetConstructorForType(SVT_UINT, destCount));
            numParenthesis++;
        }
        for (component = 0; component < 4; component++)
        {
            if (!(destMask & (1 << component)))
                continue;

            if (firstItemAdded)
                bcatcstr(glsl, ", ");
            else
                firstItemAdded = 1;

            bformata(glsl, "RawRes%d[((", psSrc->ui32RegisterNumber);
            TranslateOperand(psContext, psSrcByteOff, TO_FLAG_INTEGER);
            bcatcstr(glsl, ") >> 2)");
            if (psSrc->eSelMode == OPERAND_4_COMPONENT_SWIZZLE_MODE && psSrc->aui32Swizzle[component] != 0)
            {
                bformata(glsl, " + %d", psSrc->aui32Swizzle[component]);
            }
            bcatcstr(glsl, "]");
        }
        GLSLAddAssignPrologue(psContext, numParenthesis);
    }
    else
    {
        int numParenthesis = 0;
        int firstItemAdded = 0;
        uint32_t destCount = GetNumSwizzleElements(psDest);
        uint32_t destMask = GetOperandWriteMask(psDest);
        ASSERT(psInst->eOpcode == OPCODE_LD_STRUCTURED);
        AddIndentation(psContext);
        GLSLMETALAddAssignToDest(psContext, psDest, SVT_UINT, destCount, &numParenthesis);
        if (destCount > 1)
        {
            bformata(glsl, "%s(", GetConstructorForType(SVT_UINT, destCount));
            numParenthesis++;
        }
        for (component = 0; component < 4; component++)
        {
            ShaderVarType* psVar = NULL;
            int addedBitcast = 0;
            if (!(destMask & (1 << component)))
                continue;

            if (firstItemAdded)
                bcatcstr(glsl, ", ");
            else
                firstItemAdded = 1;

            if (psSrc->eType == OPERAND_TYPE_THREAD_GROUP_SHARED_MEMORY)
            {
                // input already in uints
                TranslateOperand(psContext, psSrc, TO_FLAG_NAME_ONLY);
                bcatcstr(glsl, "[");
                TranslateOperand(psContext, psSrcAddr, TO_FLAG_INTEGER);
                bcatcstr(glsl, "].value[(");
                TranslateOperand(psContext, psSrcByteOff, TO_FLAG_UNSIGNED_INTEGER);
                bformata(glsl, " >> 2u) + %d]", psSrc->eSelMode == OPERAND_4_COMPONENT_SWIZZLE_MODE ? psSrc->aui32Swizzle[component] : component);
            }
            else
            {
                ConstantBuffer* psCBuf = NULL;
                psVar = GLSLLookupStructuredVar(psContext, psSrc, psSrcByteOff,
                                                psSrc->eSelMode == OPERAND_4_COMPONENT_SWIZZLE_MODE ? psSrc->aui32Swizzle[component] : component);
                GetConstantBufferFromBindingPoint(RGROUP_UAV, psSrc->ui32RegisterNumber, &psContext->psShader->sInfo, &psCBuf);

                if (psVar->Type == SVT_FLOAT)
                {
                    bcatcstr(glsl, "floatBitsToUint(");
                    addedBitcast = 1;
                }
                else if (psVar->Type == SVT_DOUBLE)
                {
                    bcatcstr(glsl, "unpackDouble2x32(");
                    addedBitcast = 1;
                }
                if (psSrc->eType == OPERAND_TYPE_UNORDERED_ACCESS_VIEW)
                {
                    bformata(glsl, "%s[", psCBuf->Name);
                    TranslateOperand(psContext, psSrcAddr, TO_FLAG_INTEGER);
                    bcatcstr(glsl, "]");
                    if (strcmp(psVar->Name, "$Element") != 0)
                    {
                        bcatcstr(glsl, ".");
                        bcatcstr(glsl, psVar->Name);
                    }
                }
                else
                {
                    bformata(glsl, "StructuredRes%d[", psSrc->ui32RegisterNumber);
                    TranslateOperand(psContext, psSrcAddr, TO_FLAG_INTEGER);
                    bcatcstr(glsl, "].");

                    bcatcstr(glsl, psVar->Name);
                }

                if (addedBitcast)
                    bcatcstr(glsl, ")");
                if (psVar->Type == SVT_DOUBLE)
                    component++;  // doubles take up 2 slots
            }
        }
        GLSLAddAssignPrologue(psContext, numParenthesis);

        return;
    }

#if 0

    //(int)GetNumSwizzleElements(&psInst->asOperands[0])
    for (component = 0; component < 4; component++)
    {
        const char* swizzleString[] = { ".x", ".y", ".z", ".w" };
        ASSERT(psDest->eSelMode == OPERAND_4_COMPONENT_MASK_MODE);
        if (psDest->ui32CompMask & (1 << component))
        {
            if (structured && psSrc->eType != OPERAND_TYPE_THREAD_GROUP_SHARED_MEMORY)
            {
                psVarType = GLSLLookupStructuredVar(psContext, psSrc, psSrcByteOff, psSrc->aui32Swizzle[component]);
            }

            AddIndentation(psContext);

            aui32Swizzle[0] = psSrc->aui32Swizzle[component];

            TranslateOperand(psContext, psDest, TO_FLAG_DESTINATION);
            if (GetNumSwizzleElements(psDest) > 1)
                bformata(glsl, swizzleString[destComponent++]);

            if (psVarType)
            {
                // TODO completely broken now after GLSLMETALAddAssignToDest refactorings.
                GLSLMETALAddAssignToDest(psContext, psDest, SVTTypeToFlag(psVarType->Type), GetNumSwizzleElements(psDest), &numParenthesis);
            }
            else
            {
                GLSLMETALAddAssignToDest(psContext, psDest, TO_FLAG_NONE, GetNumSwizzleElements(psDest), &numParenthesis);
            }

            if (psSrc->eType == OPERAND_TYPE_RESOURCE)
            {
                if (structured)
                    bformata(glsl, "(StructuredRes%d[", psSrc->ui32RegisterNumber);
                else
                    bformata(glsl, "(RawRes%d[", psSrc->ui32RegisterNumber);
            }
            else
            {
                bformata(glsl, "(");
                TranslateOperand(psContext, psSrc, TO_FLAG_NAME_ONLY);
                bformata(glsl, "[");
                Translate
            }

            if (structured) //src address and src byte offset
            {
                if (psSrc->eType == OPERAND_TYPE_THREAD_GROUP_SHARED_MEMORY)
                {
                    TranslateOperand(psContext, psSrcAddr, TO_FLAG_INTEGER | TO_FLAG_UNSIGNED_INTEGER);
                    bformata(glsl, "].value[");
                    TranslateOperand(psContext, psSrcByteOff, TO_FLAG_INTEGER | TO_FLAG_UNSIGNED_INTEGER);
                    bformata(glsl, "/4u ");//bytes to floats
                }
                else
                {
                    TranslateOperand(psContext, psSrcAddr, TO_FLAG_INTEGER | TO_FLAG_UNSIGNED_INTEGER);
                }
            }
            else
            {
                TranslateOperand(psContext, psSrcByteOff, TO_FLAG_INTEGER | TO_FLAG_UNSIGNED_INTEGER);
            }

            //RAW: change component using index offset
            if (!structured || (psSrc->eType == OPERAND_TYPE_THREAD_GROUP_SHARED_MEMORY))
            {
                bformata(glsl, " + %d", psSrc->aui32Swizzle[component]);
            }

            bformata(glsl, "]");
            if (structured && psSrc->eType != OPERAND_TYPE_THREAD_GROUP_SHARED_MEMORY)
            {
                if (strcmp(psVarType->Name, "$Element") != 0)
                {
                    bformata(glsl, ".%s", psVarType->Name);
                }

                if (psVarType->Type == SVT_DOUBLE)
                {
                    //Double takes an extra slot.
                    component++;
                }
            }

            bformata(glsl, ");\n");
        }
    }
#endif
}

void TranslateAtomicMemOp(HLSLCrossCompilerContext* psContext, Instruction* psInst)
{
    bstring glsl = *psContext->currentShaderString;
    int numParenthesis = 0;
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

    psVarType = GLSLLookupStructuredVar(psContext, dest, destAddr, 0);
    if (psVarType->Type == SVT_UINT)
    {
        ui32DataTypeFlag = TO_FLAG_UNSIGNED_INTEGER | TO_AUTO_BITCAST_TO_UINT;
    }
    else
    {
        ui32DataTypeFlag = TO_FLAG_INTEGER | TO_AUTO_BITCAST_TO_INT;
    }

    if (previousValue)
    {
        GLSLMETALAddAssignToDest(psContext, previousValue, psVarType->Type, 1, &numParenthesis);
    }
    bcatcstr(glsl, func);
    bformata(glsl, "(");
    ResourceName(glsl, psContext, RGROUP_UAV, dest->ui32RegisterNumber, 0);
    bformata(glsl, "[0]");
    if (strcmp(psVarType->Name, "$Element") != 0)
    {
        bformata(glsl, ".%s", psVarType->Name);
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
        GLSLAddAssignPrologue(psContext, numParenthesis);
    }
    else
        bcatcstr(glsl, ";\n");
}

static void GLSLTranslateConditional(HLSLCrossCompilerContext* psContext, Instruction* psInst, bstring glsl)
{
    const char* statement = "";
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
    }

    if (psContext->psShader->ui32MajorVersion < 4)
    {
        bcatcstr(glsl, "if(");

        TranslateOperand(psContext, &psInst->asOperands[0], SVTTypeToFlag(GetOperandDataType(psContext, &psInst->asOperands[0])));
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

        if (psInst->eOpcode != OPCODE_IF)
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
            TranslateOperand(psContext, &psInst->asOperands[0], TO_FLAG_UNSIGNED_INTEGER);

            if (psInst->eOpcode != OPCODE_IF)
            {
                bformata(glsl, ")==0u){%s;}\n", statement);
            }
            else
            {
                bcatcstr(glsl, ")==0u){\n");
            }
        }
        else
        {
            ASSERT(psInst->eBooleanTestType == INSTRUCTION_TEST_NONZERO);
            bcatcstr(glsl, "if((");
            TranslateOperand(psContext, &psInst->asOperands[0], TO_FLAG_UNSIGNED_INTEGER);

            if (psInst->eOpcode != OPCODE_IF)
            {
                bformata(glsl, ")!=0u){%s;}\n", statement);
            }
            else
            {
                bcatcstr(glsl, ")!=0u){\n");
            }
        }
    }
}

// Returns the "more important" type of a and b, currently int < uint < float
static SHADER_VARIABLE_TYPE GLSLSelectHigherType(SHADER_VARIABLE_TYPE a, SHADER_VARIABLE_TYPE b)
{
    if (a == SVT_FLOAT || b == SVT_FLOAT)
        return SVT_FLOAT;
    // Apart from floats, the enum values are fairly well-ordered, use that directly.
    return a > b ? a : b;
}

// Helper function to set the vector type of 1 or more components in a vector
// If the existing values (that we're writing to) are all SVT_VOID, just upgrade the value and we're done
// Otherwise, set all the components in the vector that currently are set to that same value OR are now being written to
// to the "highest" type value (ordering int->uint->float)
static void GLSLSetVectorType(SHADER_VARIABLE_TYPE* aeTempVecType, uint32_t regBaseIndex, uint32_t componentMask, SHADER_VARIABLE_TYPE eType)
{
    int existingTypesFound = 0;
    int i = 0;
    for (i = 0; i < 4; i++)
    {
        if (componentMask & (1 << i))
        {
            if (aeTempVecType[regBaseIndex + i] != SVT_VOID)
            {
                existingTypesFound = 1;
                break;
            }
        }
    }

    if (existingTypesFound != 0)
    {
        // Expand the mask to include all components that are used, also upgrade type
        for (i = 0; i < 4; i++)
        {
            if (aeTempVecType[regBaseIndex + i] != SVT_VOID)
            {
                componentMask |= (1 << i);
                eType = GLSLSelectHigherType(eType, aeTempVecType[regBaseIndex + i]);
            }
        }
    }

    // Now componentMask contains the components we actually need to update and eType may have been changed to something else.
    // Write the results
    for (i = 0; i < 4; i++)
    {
        if (componentMask & (1 << i))
        {
            aeTempVecType[regBaseIndex + i] = eType;
        }
    }
}

static void GLSLMarkOperandAs(Operand* psOperand, SHADER_VARIABLE_TYPE eType, SHADER_VARIABLE_TYPE* aeTempVecType)
{
    if (psOperand->eType == OPERAND_TYPE_INDEXABLE_TEMP || psOperand->eType == OPERAND_TYPE_TEMP)
    {
        const uint32_t ui32RegIndex = psOperand->ui32RegisterNumber * 4;

        if (psOperand->eSelMode == OPERAND_4_COMPONENT_SELECT_1_MODE)
        {
            GLSLSetVectorType(aeTempVecType, ui32RegIndex, 1 << psOperand->aui32Swizzle[0], eType);
        }
        else if (psOperand->eSelMode == OPERAND_4_COMPONENT_SWIZZLE_MODE)
        {
            // 0xf == all components, swizzle order doesn't matter.
            GLSLSetVectorType(aeTempVecType, ui32RegIndex, 0xf, eType);
        }
        else if (psOperand->eSelMode == OPERAND_4_COMPONENT_MASK_MODE)
        {
            uint32_t ui32CompMask = psOperand->ui32CompMask;
            if (!psOperand->ui32CompMask)
            {
                ui32CompMask = OPERAND_4_COMPONENT_MASK_ALL;
            }

            GLSLSetVectorType(aeTempVecType, ui32RegIndex, ui32CompMask, eType);
        }
    }
}

static void GLSLMarkAllOperandsAs(Instruction* psInst, SHADER_VARIABLE_TYPE eType, SHADER_VARIABLE_TYPE* aeTempVecType)
{
    uint32_t i = 0;
    for (i = 0; i < psInst->ui32NumOperands; i++)
    {
        GLSLMarkOperandAs(&psInst->asOperands[i], eType, aeTempVecType);
    }
}

static void GLSLWriteOperandTypes(Operand* psOperand, const SHADER_VARIABLE_TYPE* aeTempVecType)
{
    const uint32_t ui32RegIndex = psOperand->ui32RegisterNumber * 4;

    if (psOperand->eType != OPERAND_TYPE_TEMP)
        return;

    if (psOperand->eSelMode == OPERAND_4_COMPONENT_SELECT_1_MODE)
    {
        psOperand->aeDataType[psOperand->aui32Swizzle[0]] = aeTempVecType[ui32RegIndex + psOperand->aui32Swizzle[0]];
    }
    else if (psOperand->eSelMode == OPERAND_4_COMPONENT_SWIZZLE_MODE)
    {
        if (psOperand->ui32Swizzle == (NO_SWIZZLE))
        {
            psOperand->aeDataType[0] = aeTempVecType[ui32RegIndex];
            psOperand->aeDataType[1] = aeTempVecType[ui32RegIndex + 1];
            psOperand->aeDataType[2] = aeTempVecType[ui32RegIndex + 2];
            psOperand->aeDataType[3] = aeTempVecType[ui32RegIndex + 3];
        }
        else
        {
            psOperand->aeDataType[psOperand->aui32Swizzle[0]] = aeTempVecType[ui32RegIndex + psOperand->aui32Swizzle[0]];
            psOperand->aeDataType[psOperand->aui32Swizzle[1]] = aeTempVecType[ui32RegIndex + psOperand->aui32Swizzle[1]];
            psOperand->aeDataType[psOperand->aui32Swizzle[2]] = aeTempVecType[ui32RegIndex + psOperand->aui32Swizzle[2]];
            psOperand->aeDataType[psOperand->aui32Swizzle[3]] = aeTempVecType[ui32RegIndex + psOperand->aui32Swizzle[3]];
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

// Mark scalars from CBs. TODO: Do we need to do the same for vec2/3's as well? There may be swizzles involved which make it vec4 or something else again.
static void GLSLSetCBOperandComponents(HLSLCrossCompilerContext* psContext, Operand* psOperand)
{
    ConstantBuffer* psCBuf = NULL;
    ShaderVarType* psVarType = NULL;
    int32_t index = -1;
    int rebase = 0;

    if (psOperand->eType != OPERAND_TYPE_CONSTANT_BUFFER)
        return;

    GetConstantBufferFromBindingPoint(RGROUP_CBUFFER, psOperand->aui32ArraySizes[0], &psContext->psShader->sInfo, &psCBuf);
    GetShaderVarFromOffset(psOperand->aui32ArraySizes[1], psOperand->aui32Swizzle, psCBuf, &psVarType, &index, &rebase);

    if (psVarType->Class == SVC_SCALAR)
        psOperand->iNumComponents = 1;
}

void SetDataTypes(HLSLCrossCompilerContext* psContext, Instruction* psInst, const int32_t i32InstCount)
{
    int32_t i;
    Instruction* psFirstInst = psInst;

    SHADER_VARIABLE_TYPE aeTempVecType[MAX_TEMP_VEC4 * 4];

    if (psContext->psShader->ui32MajorVersion <= 3)
    {
        for (i = 0; i < MAX_TEMP_VEC4 * 4; ++i)
        {
            aeTempVecType[i] = SVT_FLOAT;
        }
    }
    else
    {
        // Start with void, then move up the chain void->int->uint->float
        for (i = 0; i < MAX_TEMP_VEC4 * 4; ++i)
        {
            aeTempVecType[i] = SVT_VOID;
        }
    }

    //    if (psContext->psShader->ui32MajorVersion <= 3)
    {
        // First pass, do analysis: deduce the data type based on opcodes, fill out aeTempVecType table
        // Only ever to int->float promotion (or int->uint), never the other way around
        for (i = 0; i < i32InstCount; ++i, psInst++)
        {
            int k = 0;
            if (psInst->ui32NumOperands == 0)
                continue;

            switch (psInst->eOpcode)
            {
                    // All float-only ops
                case OPCODE_ADD:
                case OPCODE_DERIV_RTX:
                case OPCODE_DERIV_RTY:
                case OPCODE_DIV:
                case OPCODE_DP2:
                case OPCODE_DP3:
                case OPCODE_DP4:
                case OPCODE_EQ:
                case OPCODE_EXP:
                case OPCODE_FRC:
                case OPCODE_LOG:
                case OPCODE_MAD:
                case OPCODE_MIN:
                case OPCODE_MAX:
                case OPCODE_MUL:
                case OPCODE_NE:
                case OPCODE_ROUND_NE:
                case OPCODE_ROUND_NI:
                case OPCODE_ROUND_PI:
                case OPCODE_ROUND_Z:
                case OPCODE_RSQ:
                case OPCODE_SAMPLE:
                case OPCODE_SAMPLE_C:
                case OPCODE_SAMPLE_C_LZ:
                case OPCODE_SAMPLE_L:
                case OPCODE_SAMPLE_D:
                case OPCODE_SAMPLE_B:
                case OPCODE_SQRT:
                case OPCODE_SINCOS:
                case OPCODE_LOD:
                case OPCODE_GATHER4:

                case OPCODE_DERIV_RTX_COARSE:
                case OPCODE_DERIV_RTX_FINE:
                case OPCODE_DERIV_RTY_COARSE:
                case OPCODE_DERIV_RTY_FINE:
                case OPCODE_GATHER4_C:
                case OPCODE_GATHER4_PO:
                case OPCODE_GATHER4_PO_C:
                case OPCODE_RCP:

                    GLSLMarkAllOperandsAs(psInst, SVT_FLOAT, aeTempVecType);
                    break;

                    // Int-only ops, no need to do anything
                case OPCODE_AND:
                case OPCODE_BREAKC:
                case OPCODE_CALLC:
                case OPCODE_CONTINUEC:
                case OPCODE_IADD:
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
                case OPCODE_IF:
                case OPCODE_NOT:
                case OPCODE_OR:
                case OPCODE_RETC:
                case OPCODE_XOR:
                case OPCODE_BUFINFO:
                case OPCODE_COUNTBITS:
                case OPCODE_FIRSTBIT_HI:
                case OPCODE_FIRSTBIT_LO:
                case OPCODE_FIRSTBIT_SHI:
                case OPCODE_UBFE:
                case OPCODE_IBFE:
                case OPCODE_BFI:
                case OPCODE_BFREV:
                case OPCODE_ATOMIC_AND:
                case OPCODE_ATOMIC_OR:
                case OPCODE_ATOMIC_XOR:
                case OPCODE_ATOMIC_CMP_STORE:
                case OPCODE_ATOMIC_IADD:
                case OPCODE_ATOMIC_IMAX:
                case OPCODE_ATOMIC_IMIN:
                case OPCODE_ATOMIC_UMAX:
                case OPCODE_ATOMIC_UMIN:
                case OPCODE_IMM_ATOMIC_ALLOC:
                case OPCODE_IMM_ATOMIC_CONSUME:
                case OPCODE_IMM_ATOMIC_IADD:
                case OPCODE_IMM_ATOMIC_AND:
                case OPCODE_IMM_ATOMIC_OR:
                case OPCODE_IMM_ATOMIC_XOR:
                case OPCODE_IMM_ATOMIC_EXCH:
                case OPCODE_IMM_ATOMIC_CMP_EXCH:
                case OPCODE_IMM_ATOMIC_IMAX:
                case OPCODE_IMM_ATOMIC_IMIN:
                case OPCODE_IMM_ATOMIC_UMAX:
                case OPCODE_IMM_ATOMIC_UMIN:
                case OPCODE_MOV:
                case OPCODE_MOVC:
                case OPCODE_SWAPC:
                    GLSLMarkAllOperandsAs(psInst, SVT_INT, aeTempVecType);
                    break;
                    // uint ops
                case OPCODE_UDIV:
                case OPCODE_ULT:
                case OPCODE_UGE:
                case OPCODE_UMUL:
                case OPCODE_UMAD:
                case OPCODE_UMAX:
                case OPCODE_UMIN:
                case OPCODE_USHR:
                case OPCODE_UADDC:
                case OPCODE_USUBB:
                    GLSLMarkAllOperandsAs(psInst, SVT_UINT, aeTempVecType);
                    break;

                    // Need special handling
                case OPCODE_FTOI:
                case OPCODE_FTOU:
                    GLSLMarkOperandAs(&psInst->asOperands[0], psInst->eOpcode == OPCODE_FTOI ? SVT_INT : SVT_UINT, aeTempVecType);
                    GLSLMarkOperandAs(&psInst->asOperands[1], SVT_FLOAT, aeTempVecType);
                    break;

                case OPCODE_GE:
                case OPCODE_LT:
                    GLSLMarkOperandAs(&psInst->asOperands[0], SVT_UINT, aeTempVecType);
                    GLSLMarkOperandAs(&psInst->asOperands[1], SVT_FLOAT, aeTempVecType);
                    GLSLMarkOperandAs(&psInst->asOperands[2], SVT_FLOAT, aeTempVecType);
                    break;

                case OPCODE_ITOF:
                case OPCODE_UTOF:
                    GLSLMarkOperandAs(&psInst->asOperands[0], SVT_FLOAT, aeTempVecType);
                    GLSLMarkOperandAs(&psInst->asOperands[1], psInst->eOpcode == OPCODE_ITOF ? SVT_INT : SVT_UINT, aeTempVecType);
                    break;

                case OPCODE_LD:
                case OPCODE_LD_MS:
                    // TODO: Would need to know the sampler return type
                    GLSLMarkOperandAs(&psInst->asOperands[0], SVT_FLOAT, aeTempVecType);
                    break;

                case OPCODE_RESINFO:
                {
                    if (psInst->eResInfoReturnType != RESINFO_INSTRUCTION_RETURN_UINT)
                        GLSLMarkAllOperandsAs(psInst, SVT_FLOAT, aeTempVecType);
                    break;
                }

                case OPCODE_SAMPLE_INFO:
                    // TODO decode the _uint flag
                    GLSLMarkOperandAs(&psInst->asOperands[0], SVT_FLOAT, aeTempVecType);
                    break;

                case OPCODE_SAMPLE_POS:
                    GLSLMarkOperandAs(&psInst->asOperands[0], SVT_FLOAT, aeTempVecType);
                    break;

                case OPCODE_LD_UAV_TYPED:
                case OPCODE_STORE_UAV_TYPED:
                case OPCODE_LD_RAW:
                case OPCODE_STORE_RAW:
                case OPCODE_LD_STRUCTURED:
                case OPCODE_STORE_STRUCTURED:
                    GLSLMarkOperandAs(&psInst->asOperands[0], SVT_INT, aeTempVecType);
                    break;

                case OPCODE_F32TOF16:
                case OPCODE_F16TOF32:
                    // TODO
                    break;

                    // No-operands, should never get here anyway
                    /*                case OPCODE_BREAK:
                                    case OPCODE_CALL:
                                    case OPCODE_CASE:
                                    case OPCODE_CONTINUE:
                                    case OPCODE_CUT:
                                    case OPCODE_DEFAULT:
                                    case OPCODE_DISCARD:
                                    case OPCODE_ELSE:
                                    case OPCODE_EMIT:
                                    case OPCODE_EMITTHENCUT:
                                    case OPCODE_ENDIF:
                                    case OPCODE_ENDLOOP:
                                    case OPCODE_ENDSWITCH:

                                    case OPCODE_LABEL:
                                    case OPCODE_LOOP:
                                    case OPCODE_CUSTOMDATA:
                                    case OPCODE_NOP:
                                    case OPCODE_RET:
                                    case OPCODE_SWITCH:
                                    case OPCODE_DCL_RESOURCE: // DCL* opcodes have
                                    case OPCODE_DCL_CONSTANT_BUFFER: // custom operand formats.
                                    case OPCODE_DCL_SAMPLER:
                                    case OPCODE_DCL_INDEX_RANGE:
                                    case OPCODE_DCL_GS_OUTPUT_PRIMITIVE_TOPOLOGY:
                                    case OPCODE_DCL_GS_INPUT_PRIMITIVE:
                                    case OPCODE_DCL_MAX_OUTPUT_VERTEX_COUNT:
                                    case OPCODE_DCL_INPUT:
                                    case OPCODE_DCL_INPUT_SGV:
                                    case OPCODE_DCL_INPUT_SIV:
                                    case OPCODE_DCL_INPUT_PS:
                                    case OPCODE_DCL_INPUT_PS_SGV:
                                    case OPCODE_DCL_INPUT_PS_SIV:
                                    case OPCODE_DCL_OUTPUT:
                                    case OPCODE_DCL_OUTPUT_SGV:
                                    case OPCODE_DCL_OUTPUT_SIV:
                                    case OPCODE_DCL_TEMPS:
                                    case OPCODE_DCL_INDEXABLE_TEMP:
                                    case OPCODE_DCL_GLOBAL_FLAGS:


                                    case OPCODE_HS_DECLS: // token marks beginning of HS sub-shader
                                    case OPCODE_HS_CONTROL_POINT_PHASE: // token marks beginning of HS sub-shader
                                    case OPCODE_HS_FORK_PHASE: // token marks beginning of HS sub-shader
                                    case OPCODE_HS_JOIN_PHASE: // token marks beginning of HS sub-shader

                                    case OPCODE_EMIT_STREAM:
                                    case OPCODE_CUT_STREAM:
                                    case OPCODE_EMITTHENCUT_STREAM:
                                    case OPCODE_INTERFACE_CALL:


                                    case OPCODE_DCL_STREAM:
                                    case OPCODE_DCL_FUNCTION_BODY:
                                    case OPCODE_DCL_FUNCTION_TABLE:
                                    case OPCODE_DCL_INTERFACE:

                                    case OPCODE_DCL_INPUT_CONTROL_POINT_COUNT:
                                    case OPCODE_DCL_OUTPUT_CONTROL_POINT_COUNT:
                                    case OPCODE_DCL_TESS_DOMAIN:
                                    case OPCODE_DCL_TESS_PARTITIONING:
                                    case OPCODE_DCL_TESS_OUTPUT_PRIMITIVE:
                                    case OPCODE_DCL_HS_MAX_TESSFACTOR:
                                    case OPCODE_DCL_HS_FORK_PHASE_INSTANCE_COUNT:
                                    case OPCODE_DCL_HS_JOIN_PHASE_INSTANCE_COUNT:

                                    case OPCODE_DCL_THREAD_GROUP:
                                    case OPCODE_DCL_UNORDERED_ACCESS_VIEW_TYPED:
                                    case OPCODE_DCL_UNORDERED_ACCESS_VIEW_RAW:
                                    case OPCODE_DCL_UNORDERED_ACCESS_VIEW_STRUCTURED:
                                    case OPCODE_DCL_THREAD_GROUP_SHARED_MEMORY_RAW:
                                    case OPCODE_DCL_THREAD_GROUP_SHARED_MEMORY_STRUCTURED:
                                    case OPCODE_DCL_RESOURCE_RAW:
                                    case OPCODE_DCL_RESOURCE_STRUCTURED:
                                    case OPCODE_SYNC:

                                    // TODO
                                    case OPCODE_DADD:
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

                                    case OPCODE_EVAL_SNAPPED:
                                    case OPCODE_EVAL_SAMPLE_INDEX:
                                    case OPCODE_EVAL_CENTROID:

                                    case OPCODE_DCL_GS_INSTANCE_COUNT:

                                    case OPCODE_ABORT:
                                    case OPCODE_DEBUG_BREAK:*/

                default:
                    break;
            }
        }
    }

    // Fill the rest of aeTempVecType, just in case.
    for (i = 0; i < MAX_TEMP_VEC4 * 4; i++)
    {
        if (aeTempVecType[i] == SVT_VOID)
            aeTempVecType[i] = SVT_INT;
    }

    // Now the aeTempVecType table has been filled with (mostly) valid data, write it back to all operands
    psInst = psFirstInst;
    for (i = 0; i < i32InstCount; ++i, psInst++)
    {
        int k = 0;

        if (psInst->ui32NumOperands == 0)
            continue;

        // Preserve the current type on dest array index
        if (psInst->asOperands[0].eType == OPERAND_TYPE_INDEXABLE_TEMP)
        {
            Operand* psSubOperand = psInst->asOperands[0].psSubOperand[1];
            if (psSubOperand != 0)
            {
                GLSLWriteOperandTypes(psSubOperand, aeTempVecType);
            }
        }
        if (psInst->asOperands[0].eType == OPERAND_TYPE_CONSTANT_BUFFER)
            GLSLSetCBOperandComponents(psContext, &psInst->asOperands[0]);

        // Preserve the current type on sources.
        for (k = psInst->ui32NumOperands - 1; k >= (int)psInst->ui32FirstSrc; --k)
        {
            int32_t subOperand;
            Operand* psOperand = &psInst->asOperands[k];

            GLSLWriteOperandTypes(psOperand, aeTempVecType);
            if (psOperand->eType == OPERAND_TYPE_CONSTANT_BUFFER)
                GLSLSetCBOperandComponents(psContext, psOperand);

            for (subOperand = 0; subOperand < MAX_SUB_OPERANDS; subOperand++)
            {
                if (psOperand->psSubOperand[subOperand] != 0)
                {
                    Operand* psSubOperand = psOperand->psSubOperand[subOperand];
                    GLSLWriteOperandTypes(psSubOperand, aeTempVecType);
                    if (psSubOperand->eType == OPERAND_TYPE_CONSTANT_BUFFER)
                        GLSLSetCBOperandComponents(psContext, psSubOperand);
                }
            }

            // Set immediates
            if (GLSLIsIntegerImmediateOpcode(psInst->eOpcode))
            {
                if (psOperand->eType == OPERAND_TYPE_IMMEDIATE32)
                {
                    psOperand->iIntegerImmediate = 1;
                }
            }
        }

        // Process the destination last in order to handle instructions
        // where the destination register is also used as a source.
        for (k = 0; k < (int)psInst->ui32FirstSrc; ++k)
        {
            Operand* psOperand = &psInst->asOperands[k];
            GLSLWriteOperandTypes(psOperand, aeTempVecType);
        }
    }
}

void TranslateInstruction(HLSLCrossCompilerContext* psContext, Instruction* psInst, Instruction* psNextInst)
{
    bstring glsl = *psContext->currentShaderString;
    int numParenthesis = 0;

#ifdef _DEBUG
    AddIndentation(psContext);
    bformata(glsl, "//Instruction %d\n", psInst->id);
#if 0
    if(psInst->id == 73)
    {
        ASSERT(1); //Set breakpoint here to debug an instruction from its ID.
    }
#endif
#endif

    switch (psInst->eOpcode)
    {
        case OPCODE_FTOI:
        case OPCODE_FTOU:
        {
            uint32_t dstCount = GetNumSwizzleElements(&psInst->asOperands[0]);
            uint32_t srcCount = GetNumSwizzleElements(&psInst->asOperands[1]);
            uint32_t ui32DstFlags = TO_FLAG_DESTINATION;
            const SHADER_VARIABLE_TYPE eSrcType = GetOperandDataType(psContext, &psInst->asOperands[1]);
            const SHADER_VARIABLE_TYPE eDestType = GetOperandDataType(psContext, &psInst->asOperands[0]);

#ifdef _DEBUG
            AddIndentation(psContext);
            if (psInst->eOpcode == OPCODE_FTOU)
                bcatcstr(glsl, "//FTOU\n");
            else
                bcatcstr(glsl, "//FTOI\n");
#endif

            AddIndentation(psContext);

            GLSLMETALAddAssignToDest(psContext, &psInst->asOperands[0], psInst->eOpcode == OPCODE_FTOU ? SVT_UINT : SVT_INT, srcCount, &numParenthesis);
            bcatcstr(glsl, GetConstructorForType(psInst->eOpcode == OPCODE_FTOU ? SVT_UINT : SVT_INT, srcCount == dstCount ? dstCount : 4));
            bcatcstr(glsl, "(");  // 1
            TranslateOperand(psContext, &psInst->asOperands[1], TO_AUTO_BITCAST_TO_FLOAT);
            bcatcstr(glsl, ")");  // 1
            // Add destination writemask if the component counts do not match
            if (srcCount != dstCount)
                AddSwizzleUsingElementCount(psContext, dstCount);
            GLSLAddAssignPrologue(psContext, numParenthesis);
            break;
        }

        case OPCODE_MOV:
        {
#ifdef _DEBUG
            AddIndentation(psContext);
            bcatcstr(glsl, "//MOV\n");
#endif
            AddIndentation(psContext);
            GLSLAddMOVBinaryOp(psContext, &psInst->asOperands[0], &psInst->asOperands[1]);
            break;
        }
        case OPCODE_ITOF:  // signed to float
        case OPCODE_UTOF:  // unsigned to float
        {
            const SHADER_VARIABLE_TYPE eDestType = GetOperandDataType(psContext, &psInst->asOperands[0]);
            const SHADER_VARIABLE_TYPE eSrcType = GetOperandDataType(psContext, &psInst->asOperands[1]);
            uint32_t dstCount = GetNumSwizzleElements(&psInst->asOperands[0]);
            uint32_t srcCount = GetNumSwizzleElements(&psInst->asOperands[1]);

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
            GLSLMETALAddAssignToDest(psContext, &psInst->asOperands[0], SVT_FLOAT, srcCount, &numParenthesis);
            bcatcstr(glsl, GetConstructorForType(SVT_FLOAT, srcCount == dstCount ? dstCount : 4));
            bcatcstr(glsl, "(");  // 1
            TranslateOperand(psContext, &psInst->asOperands[1], psInst->eOpcode == OPCODE_UTOF ? TO_AUTO_BITCAST_TO_UINT : TO_AUTO_BITCAST_TO_INT);
            bcatcstr(glsl, ")");  // 1
            // Add destination writemask if the component counts do not match
            if (srcCount != dstCount)
                AddSwizzleUsingElementCount(psContext, dstCount);
            GLSLAddAssignPrologue(psContext, numParenthesis);
            break;
        }
        case OPCODE_MAD:
        {
#ifdef _DEBUG
            AddIndentation(psContext);
            bcatcstr(glsl, "//MAD\n");
#endif
            GLSLCallTernaryOp(psContext, "*", "+", psInst, 0, 1, 2, 3, TO_FLAG_NONE);
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

            GLSLCallTernaryOp(psContext, "*", "+", psInst, 0, 1, 2, 3, ui32Flags);
            break;
        }
        case OPCODE_DADD:
        {
#ifdef _DEBUG
            AddIndentation(psContext);
            bcatcstr(glsl, "//DADD\n");
#endif
            GLSLCallBinaryOp(psContext, "+", psInst, 0, 1, 2, SVT_DOUBLE);
            break;
        }
        case OPCODE_IADD:
        {
            SHADER_VARIABLE_TYPE eType = SVT_INT;
#ifdef _DEBUG
            AddIndentation(psContext);
            bcatcstr(glsl, "//IADD\n");
#endif
            // Is this a signed or unsigned add?
            if (GetOperandDataType(psContext, &psInst->asOperands[0]) == SVT_UINT)
            {
                eType = SVT_UINT;
            }
            GLSLCallBinaryOp(psContext, "+", psInst, 0, 1, 2, eType);
            break;
        }
        case OPCODE_ADD:
        {
#ifdef _DEBUG
            AddIndentation(psContext);
            bcatcstr(glsl, "//ADD\n");
#endif
            GLSLCallBinaryOp(psContext, "+", psInst, 0, 1, 2, SVT_FLOAT);
            break;
        }
        case OPCODE_OR:
        {
            /*Todo: vector version */
#ifdef _DEBUG
            AddIndentation(psContext);
            bcatcstr(glsl, "//OR\n");
#endif
            GLSLCallBinaryOp(psContext, "|", psInst, 0, 1, 2, SVT_UINT);
            break;
        }
        case OPCODE_AND:
        {
#ifdef _DEBUG
            AddIndentation(psContext);
            bcatcstr(glsl, "//AND\n");
#endif
            GLSLCallBinaryOp(psContext, "&", psInst, 0, 1, 2, SVT_UINT);
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
            GLSLAddComparision(psContext, psInst, GLSL_CMP_GE, TO_FLAG_NONE, NULL);
            break;
        }
        case OPCODE_MUL:
        {
#ifdef _DEBUG
            AddIndentation(psContext);
            bcatcstr(glsl, "//MUL\n");
#endif
            GLSLCallBinaryOp(psContext, "*", psInst, 0, 1, 2, SVT_FLOAT);
            break;
        }
        case OPCODE_IMUL:
        {
            SHADER_VARIABLE_TYPE eType = SVT_INT;
#ifdef _DEBUG
            AddIndentation(psContext);
            bcatcstr(glsl, "//IMUL\n");
#endif
            if (GetOperandDataType(psContext, &psInst->asOperands[1]) == SVT_UINT)
            {
                eType = SVT_UINT;
            }

            ASSERT(psInst->asOperands[0].eType == OPERAND_TYPE_NULL);

            GLSLCallBinaryOp(psContext, "*", psInst, 1, 2, 3, eType);
            break;
        }
        case OPCODE_UDIV:
        {
#ifdef _DEBUG
            AddIndentation(psContext);
            bcatcstr(glsl, "//UDIV\n");
#endif
            // destQuotient, destRemainder, src0, src1
            GLSLCallBinaryOp(psContext, "/", psInst, 0, 2, 3, SVT_UINT);
            GLSLCallBinaryOp(psContext, "%", psInst, 1, 2, 3, SVT_UINT);
            break;
        }
        case OPCODE_DIV:
        {
#ifdef _DEBUG
            AddIndentation(psContext);
            bcatcstr(glsl, "//DIV\n");
#endif
            GLSLCallBinaryOp(psContext, "/", psInst, 0, 1, 2, SVT_FLOAT);
            break;
        }
        case OPCODE_SINCOS:
        {
#ifdef _DEBUG
            AddIndentation(psContext);
            bcatcstr(glsl, "//SINCOS\n");
#endif
            // Need careful ordering if src == dest[0], as then the cos() will be reading from wrong value
            if (psInst->asOperands[0].eType == psInst->asOperands[2].eType &&
                psInst->asOperands[0].ui32RegisterNumber == psInst->asOperands[2].ui32RegisterNumber)
            {
                // sin() result overwrites source, do cos() first.
                // The case where both write the src shouldn't really happen anyway.
                if (psInst->asOperands[1].eType != OPERAND_TYPE_NULL)
                {
                    GLSLCallHelper1(psContext, "cos", psInst, 1, 2, 1);
                }

                if (psInst->asOperands[0].eType != OPERAND_TYPE_NULL)
                {
                    GLSLCallHelper1(psContext, "sin", psInst, 0, 2, 1);
                }
            }
            else
            {
                if (psInst->asOperands[0].eType != OPERAND_TYPE_NULL)
                {
                    GLSLCallHelper1(psContext, "sin", psInst, 0, 2, 1);
                }

                if (psInst->asOperands[1].eType != OPERAND_TYPE_NULL)
                {
                    GLSLCallHelper1(psContext, "cos", psInst, 1, 2, 1);
                }
            }
            break;
        }

        case OPCODE_DP2:
        {
            SHADER_VARIABLE_TYPE eDestDataType = GetOperandDataType(psContext, &psInst->asOperands[0]);
            int numParenthesis2 = 0;
#ifdef _DEBUG
            AddIndentation(psContext);
            bcatcstr(glsl, "//DP2\n");
#endif
            AddIndentation(psContext);
            GLSLMETALAddAssignToDest(psContext, &psInst->asOperands[0], SVT_FLOAT, 1, &numParenthesis2);
            bcatcstr(glsl, "dot(");
            TranslateOperandWithMask(psContext, &psInst->asOperands[1], TO_AUTO_BITCAST_TO_FLOAT, 3 /* .xy */);
            bcatcstr(glsl, ", ");
            TranslateOperandWithMask(psContext, &psInst->asOperands[2], TO_AUTO_BITCAST_TO_FLOAT, 3 /* .xy */);
            bcatcstr(glsl, ")");
            GLSLAddAssignPrologue(psContext, numParenthesis2);
            break;
        }
        case OPCODE_DP3:
        {
            int numParenthesis2 = 0;
#ifdef _DEBUG
            AddIndentation(psContext);
            bcatcstr(glsl, "//DP3\n");
#endif
            AddIndentation(psContext);
            GLSLMETALAddAssignToDest(psContext, &psInst->asOperands[0], SVT_FLOAT, 1, &numParenthesis2);
            bcatcstr(glsl, "dot(");
            TranslateOperandWithMask(psContext, &psInst->asOperands[1], TO_AUTO_BITCAST_TO_FLOAT, 7 /* .xyz */);
            bcatcstr(glsl, ", ");
            TranslateOperandWithMask(psContext, &psInst->asOperands[2], TO_AUTO_BITCAST_TO_FLOAT, 7 /* .xyz */);
            bcatcstr(glsl, ")");
            GLSLAddAssignPrologue(psContext, numParenthesis2);
            break;
        }
        case OPCODE_DP4:
        {
#ifdef _DEBUG
            AddIndentation(psContext);
            bcatcstr(glsl, "//DP4\n");
#endif
            GLSLCallHelper2(psContext, "dot", psInst, 0, 1, 2, 0);
            break;
        }
        case OPCODE_INE:
        {
#ifdef _DEBUG
            AddIndentation(psContext);
            bcatcstr(glsl, "//INE\n");
#endif
            GLSLAddComparision(psContext, psInst, GLSL_CMP_NE, TO_FLAG_INTEGER, NULL);
            break;
        }
        case OPCODE_NE:
        {
#ifdef _DEBUG
            AddIndentation(psContext);
            bcatcstr(glsl, "//NE\n");
#endif
            GLSLAddComparision(psContext, psInst, GLSL_CMP_NE, TO_FLAG_NONE, NULL);
            break;
        }
        case OPCODE_IGE:
        {
#ifdef _DEBUG
            AddIndentation(psContext);
            bcatcstr(glsl, "//IGE\n");
#endif
            GLSLAddComparision(psContext, psInst, GLSL_CMP_GE, TO_FLAG_INTEGER, psNextInst);
            break;
        }
        case OPCODE_ILT:
        {
#ifdef _DEBUG
            AddIndentation(psContext);
            bcatcstr(glsl, "//ILT\n");
#endif
            GLSLAddComparision(psContext, psInst, GLSL_CMP_LT, TO_FLAG_INTEGER, NULL);
            break;
        }
        case OPCODE_LT:
        {
#ifdef _DEBUG
            AddIndentation(psContext);
            bcatcstr(glsl, "//LT\n");
#endif
            GLSLAddComparision(psContext, psInst, GLSL_CMP_LT, TO_FLAG_NONE, NULL);
            break;
        }
        case OPCODE_IEQ:
        {
#ifdef _DEBUG
            AddIndentation(psContext);
            bcatcstr(glsl, "//IEQ\n");
#endif
            GLSLAddComparision(psContext, psInst, GLSL_CMP_EQ, TO_FLAG_INTEGER, NULL);
            break;
        }
        case OPCODE_ULT:
        {
#ifdef _DEBUG
            AddIndentation(psContext);
            bcatcstr(glsl, "//ULT\n");
#endif
            GLSLAddComparision(psContext, psInst, GLSL_CMP_LT, TO_FLAG_UNSIGNED_INTEGER, NULL);
            break;
        }
        case OPCODE_UGE:
        {
#ifdef _DEBUG
            AddIndentation(psContext);
            bcatcstr(glsl, "//UGE\n");
#endif
            GLSLAddComparision(psContext, psInst, GLSL_CMP_GE, TO_FLAG_UNSIGNED_INTEGER, NULL);
            break;
        }
        case OPCODE_MOVC:
        {
#ifdef _DEBUG
            AddIndentation(psContext);
            bcatcstr(glsl, "//MOVC\n");
#endif
            GLSLAddMOVCBinaryOp(psContext, &psInst->asOperands[0], &psInst->asOperands[1], &psInst->asOperands[2], &psInst->asOperands[3]);
            break;
        }
        case OPCODE_SWAPC:
        {
#ifdef _DEBUG
            AddIndentation(psContext);
            bcatcstr(glsl, "//SWAPC\n");
#endif
            // TODO needs temps!!
            GLSLAddMOVCBinaryOp(psContext, &psInst->asOperands[0], &psInst->asOperands[2], &psInst->asOperands[4], &psInst->asOperands[3]);
            GLSLAddMOVCBinaryOp(psContext, &psInst->asOperands[1], &psInst->asOperands[2], &psInst->asOperands[3], &psInst->asOperands[4]);
            break;
        }

        case OPCODE_LOG:
        {
#ifdef _DEBUG
            AddIndentation(psContext);
            bcatcstr(glsl, "//LOG\n");
#endif
            GLSLCallHelper1(psContext, "log2", psInst, 0, 1, 1);
            break;
        }
        case OPCODE_RSQ:
        {
#ifdef _DEBUG
            AddIndentation(psContext);
            bcatcstr(glsl, "//RSQ\n");
#endif
            GLSLCallHelper1(psContext, "inversesqrt", psInst, 0, 1, 1);
            break;
        }
        case OPCODE_EXP:
        {
#ifdef _DEBUG
            AddIndentation(psContext);
            bcatcstr(glsl, "//EXP\n");
#endif
            GLSLCallHelper1(psContext, "exp2", psInst, 0, 1, 1);
            break;
        }
        case OPCODE_SQRT:
        {
#ifdef _DEBUG
            AddIndentation(psContext);
            bcatcstr(glsl, "//SQRT\n");
#endif
            GLSLCallHelper1(psContext, "sqrt", psInst, 0, 1, 1);
            break;
        }
        case OPCODE_ROUND_PI:
        {
#ifdef _DEBUG
            AddIndentation(psContext);
            bcatcstr(glsl, "//ROUND_PI\n");
#endif
            GLSLCallHelper1(psContext, "ceil", psInst, 0, 1, 1);
            break;
        }
        case OPCODE_ROUND_NI:
        {
#ifdef _DEBUG
            AddIndentation(psContext);
            bcatcstr(glsl, "//ROUND_NI\n");
#endif
            GLSLCallHelper1(psContext, "floor", psInst, 0, 1, 1);
            break;
        }
        case OPCODE_ROUND_Z:
        {
#ifdef _DEBUG
            AddIndentation(psContext);
            bcatcstr(glsl, "//ROUND_Z\n");
#endif
            GLSLCallHelper1(psContext, "trunc", psInst, 0, 1, 1);
            break;
        }
        case OPCODE_ROUND_NE:
        {
#ifdef _DEBUG
            AddIndentation(psContext);
            bcatcstr(glsl, "//ROUND_NE\n");
#endif
            GLSLCallHelper1(psContext, "roundEven", psInst, 0, 1, 1);
            break;
        }
        case OPCODE_FRC:
        {
#ifdef _DEBUG
            AddIndentation(psContext);
            bcatcstr(glsl, "//FRC\n");
#endif
            GLSLCallHelper1(psContext, "fract", psInst, 0, 1, 1);
            break;
        }
        case OPCODE_IMAX:
        {
#ifdef _DEBUG
            AddIndentation(psContext);
            bcatcstr(glsl, "//IMAX\n");
#endif
            GLSLCallHelper2Int(psContext, "max", psInst, 0, 1, 2, 1);
            break;
        }
        case OPCODE_MAX:
        {
#ifdef _DEBUG
            AddIndentation(psContext);
            bcatcstr(glsl, "//MAX\n");
#endif
            GLSLCallHelper2(psContext, "max", psInst, 0, 1, 2, 1);
            break;
        }
        case OPCODE_IMIN:
        {
#ifdef _DEBUG
            AddIndentation(psContext);
            bcatcstr(glsl, "//IMIN\n");
#endif
            GLSLCallHelper2Int(psContext, "min", psInst, 0, 1, 2, 1);
            break;
        }
        case OPCODE_MIN:
        {
#ifdef _DEBUG
            AddIndentation(psContext);
            bcatcstr(glsl, "//MIN\n");
#endif
            GLSLCallHelper2(psContext, "min", psInst, 0, 1, 2, 1);
            break;
        }
        case OPCODE_GATHER4:
        {
            // dest, coords, tex, sampler
            const RESOURCE_DIMENSION eResDim = psContext->psShader->aeResourceDims[psInst->asOperands[2].ui32RegisterNumber];
            const int useCombinedTextureSamplers = (psContext->flags & HLSLCC_FLAG_COMBINE_TEXTURE_SAMPLERS) ? 1 : 0;
#ifdef _DEBUG
            AddIndentation(psContext);
            bcatcstr(glsl, "//GATHER4\n");
#endif
            // gather4 r7.xyzw, r3.xyxx, t3.xyzw, s0.x
            AddIndentation(psContext);  // TODO FIXME integer samplers
            GLSLMETALAddAssignToDest(psContext, &psInst->asOperands[0], SVT_FLOAT, GetNumSwizzleElements(&psInst->asOperands[2]), &numParenthesis);
            bcatcstr(glsl, "textureGather(");

            if (!useCombinedTextureSamplers)
                ResourceName(glsl, psContext, RGROUP_TEXTURE, psInst->asOperands[2].ui32RegisterNumber, 0);
            else
                bconcat(glsl,
                        TextureSamplerName(&psContext->psShader->sInfo, psInst->asOperands[2].ui32RegisterNumber, psInst->asOperands[3].ui32RegisterNumber, 0));

            bcatcstr(glsl, ", ");
            GLSLTranslateTexCoord(psContext, eResDim, &psInst->asOperands[1]);
            bcatcstr(glsl, ")");
            // iWriteMaskEnabled is forced off during DecodeOperand because swizzle on sampler uniforms
            // does not make sense. But need to re-enable to correctly swizzle this particular instruction.
            psInst->asOperands[2].iWriteMaskEnabled = 1;
            TranslateOperandSwizzle(psContext, &psInst->asOperands[2]);

            AddSwizzleUsingElementCount(psContext, GetNumSwizzleElements(&psInst->asOperands[0]));
            GLSLAddAssignPrologue(psContext, numParenthesis);
            break;
        }
        case OPCODE_GATHER4_PO_C:
        {
            // dest, coords, offset, tex, sampler, srcReferenceValue
            const RESOURCE_DIMENSION eResDim = psContext->psShader->aeResourceDims[psInst->asOperands[3].ui32RegisterNumber];
            const int useCombinedTextureSamplers = (psContext->flags & HLSLCC_FLAG_COMBINE_TEXTURE_SAMPLERS) ? 1 : 0;
#ifdef _DEBUG
            AddIndentation(psContext);
            bcatcstr(glsl, "//GATHER4_PO_C\n");
#endif

            AddIndentation(psContext);  // TODO FIXME integer samplers
            GLSLMETALAddAssignToDest(psContext, &psInst->asOperands[0], SVT_FLOAT, GetNumSwizzleElements(&psInst->asOperands[2]), &numParenthesis);
            bcatcstr(glsl, "textureGatherOffset(");

            if (!useCombinedTextureSamplers)
                ResourceName(glsl, psContext, RGROUP_TEXTURE, psInst->asOperands[3].ui32RegisterNumber, 1);
            else
                bconcat(glsl,
                        TextureSamplerName(&psContext->psShader->sInfo, psInst->asOperands[3].ui32RegisterNumber, psInst->asOperands[3].ui32RegisterNumber, 1));

            bcatcstr(glsl, ", ");

            GLSLTranslateTexCoord(psContext, eResDim, &psInst->asOperands[1]);

            bcatcstr(glsl, ", ");
            TranslateOperand(psContext, &psInst->asOperands[5], TO_FLAG_NONE);

            bcatcstr(glsl, ", ivec2(");
            // ivec2 offset
            psInst->asOperands[2].aui32Swizzle[2] = 0xFFFFFFFF;
            psInst->asOperands[2].aui32Swizzle[3] = 0xFFFFFFFF;
            TranslateOperand(psContext, &psInst->asOperands[2], TO_FLAG_NONE);
            bcatcstr(glsl, "))");
            // iWriteMaskEnabled is forced off during DecodeOperand because swizzle on sampler uniforms
            // does not make sense. But need to re-enable to correctly swizzle this particular instruction.
            psInst->asOperands[2].iWriteMaskEnabled = 1;
            TranslateOperandSwizzle(psContext, &psInst->asOperands[3]);
            AddSwizzleUsingElementCount(psContext, GetNumSwizzleElements(&psInst->asOperands[0]));
            GLSLAddAssignPrologue(psContext, numParenthesis);
            break;
        }
        case OPCODE_GATHER4_PO:
        {
            // dest, coords, offset, tex, sampler
            const int useCombinedTextureSamplers = (psContext->flags & HLSLCC_FLAG_COMBINE_TEXTURE_SAMPLERS) ? 1 : 0;
#ifdef _DEBUG
            AddIndentation(psContext);
            bcatcstr(glsl, "//GATHER4_PO\n");
#endif

            AddIndentation(psContext);  // TODO FIXME integer samplers
            GLSLMETALAddAssignToDest(psContext, &psInst->asOperands[0], SVT_FLOAT, GetNumSwizzleElements(&psInst->asOperands[2]), &numParenthesis);
            bcatcstr(glsl, "textureGatherOffset(");

            if (!useCombinedTextureSamplers)
                ResourceName(glsl, psContext, RGROUP_TEXTURE, psInst->asOperands[3].ui32RegisterNumber, 0);
            else
                bconcat(glsl,
                        TextureSamplerName(&psContext->psShader->sInfo, psInst->asOperands[3].ui32RegisterNumber, psInst->asOperands[4].ui32RegisterNumber, 0));

            bcatcstr(glsl, ", ");
            // Texture coord cannot be vec4
            // Determining if it is a vec3 for vec2 yet to be done.
            psInst->asOperands[1].aui32Swizzle[2] = 0xFFFFFFFF;
            psInst->asOperands[1].aui32Swizzle[3] = 0xFFFFFFFF;
            TranslateOperand(psContext, &psInst->asOperands[1], TO_FLAG_NONE);

            bcatcstr(glsl, ", ivec2(");
            // ivec2 offset
            psInst->asOperands[2].aui32Swizzle[2] = 0xFFFFFFFF;
            psInst->asOperands[2].aui32Swizzle[3] = 0xFFFFFFFF;
            TranslateOperand(psContext, &psInst->asOperands[2], TO_FLAG_NONE);
            bcatcstr(glsl, "))");
            // iWriteMaskEnabled is forced off during DecodeOperand because swizzle on sampler uniforms
            // does not make sense. But need to re-enable to correctly swizzle this particular instruction.
            psInst->asOperands[2].iWriteMaskEnabled = 1;
            TranslateOperandSwizzle(psContext, &psInst->asOperands[3]);
            AddSwizzleUsingElementCount(psContext, GetNumSwizzleElements(&psInst->asOperands[0]));
            GLSLAddAssignPrologue(psContext, numParenthesis);
            break;
        }
        case OPCODE_GATHER4_C:
        {
            // dest, coords, tex, sampler srcReferenceValue
            const int useCombinedTextureSamplers = (psContext->flags & HLSLCC_FLAG_COMBINE_TEXTURE_SAMPLERS) ? 1 : 0;
#ifdef _DEBUG
            AddIndentation(psContext);
            bcatcstr(glsl, "//GATHER4_C\n");
#endif

            AddIndentation(psContext);  // TODO FIXME integer samplers
            GLSLMETALAddAssignToDest(psContext, &psInst->asOperands[0], SVT_FLOAT, GetNumSwizzleElements(&psInst->asOperands[2]), &numParenthesis);
            bcatcstr(glsl, "textureGather(");

            if (!useCombinedTextureSamplers)
                ResourceName(glsl, psContext, RGROUP_TEXTURE, psInst->asOperands[2].ui32RegisterNumber, 1);
            else
                bconcat(glsl,
                        TextureSamplerName(&psContext->psShader->sInfo, psInst->asOperands[2].ui32RegisterNumber, psInst->asOperands[3].ui32RegisterNumber, 1));

            bcatcstr(glsl, ", ");
            // Texture coord cannot be vec4
            // Determining if it is a vec3 for vec2 yet to be done.
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
            AddSwizzleUsingElementCount(psContext, GetNumSwizzleElements(&psInst->asOperands[0]));
            GLSLAddAssignPrologue(psContext, numParenthesis);
            break;
        }
        case OPCODE_SAMPLE:
        {
#ifdef _DEBUG
            AddIndentation(psContext);
            bcatcstr(glsl, "//SAMPLE\n");
#endif
            GLSLTranslateTextureSample(psContext, psInst, TEXSMP_FLAG_NONE);
            break;
        }
        case OPCODE_SAMPLE_L:
        {
#ifdef _DEBUG
            AddIndentation(psContext);
            bcatcstr(glsl, "//SAMPLE_L\n");
#endif
            GLSLTranslateTextureSample(psContext, psInst, TEXSMP_FLAG_LOD);
            break;
        }
        case OPCODE_SAMPLE_C:
        {
#ifdef _DEBUG
            AddIndentation(psContext);
            bcatcstr(glsl, "//SAMPLE_C\n");
#endif

            GLSLTranslateTextureSample(psContext, psInst, TEXSMP_FLAG_DEPTHCOMPARE);
            break;
        }
        case OPCODE_SAMPLE_C_LZ:
        {
#ifdef _DEBUG
            AddIndentation(psContext);
            bcatcstr(glsl, "//SAMPLE_C_LZ\n");
#endif

            GLSLTranslateTextureSample(psContext, psInst, TEXSMP_FLAG_DEPTHCOMPARE | TEXSMP_FLAG_FIRSTLOD);
            break;
        }
        case OPCODE_SAMPLE_D:
        {
#ifdef _DEBUG
            AddIndentation(psContext);
            bcatcstr(glsl, "//SAMPLE_D\n");
#endif

            GLSLTranslateTextureSample(psContext, psInst, TEXSMP_FLAGS_GRAD);
            break;
        }
        case OPCODE_SAMPLE_B:
        {
#ifdef _DEBUG
            AddIndentation(psContext);
            bcatcstr(glsl, "//SAMPLE_B\n");
#endif

            GLSLTranslateTextureSample(psContext, psInst, TEXSMP_FLAG_BIAS);
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

            name = &psVar->Name[0];

            AddIndentation(psContext);
            bcatcstr(glsl, name);
            TranslateOperandIndexMAD(psContext, &psInst->asOperands[0], 1, ui32NumBodiesPerTable, funcBodyIndex);
            // bformata(glsl, "[%d]", funcBodyIndex);
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
            bcatcstr(glsl, "}\n");  // Closing brace ends the previous function.
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
            TranslateOperand(psContext, &psInst->asOperands[0], TO_FLAG_INTEGER | TO_FLAG_DESTINATION);
            bcatcstr(glsl, " = bitCount(");
            TranslateOperand(psContext, &psInst->asOperands[1], TO_FLAG_INTEGER);
            bcatcstr(glsl, ");\n");
            break;
        }
        case OPCODE_FIRSTBIT_HI:
        {
#ifdef _DEBUG
            AddIndentation(psContext);
            bcatcstr(glsl, "//FIRSTBIT_HI\n");
#endif
            AddIndentation(psContext);
            TranslateOperand(psContext, &psInst->asOperands[0], TO_FLAG_UNSIGNED_INTEGER | TO_FLAG_DESTINATION);
            bcatcstr(glsl, " = findMSB(");
            TranslateOperand(psContext, &psInst->asOperands[1], TO_FLAG_UNSIGNED_INTEGER);
            bcatcstr(glsl, ");\n");
            break;
        }
        case OPCODE_FIRSTBIT_LO:
        {
#ifdef _DEBUG
            AddIndentation(psContext);
            bcatcstr(glsl, "//FIRSTBIT_LO\n");
#endif
            AddIndentation(psContext);
            TranslateOperand(psContext, &psInst->asOperands[0], TO_FLAG_UNSIGNED_INTEGER | TO_FLAG_DESTINATION);
            bcatcstr(glsl, " = findLSB(");
            TranslateOperand(psContext, &psInst->asOperands[1], TO_FLAG_UNSIGNED_INTEGER);
            bcatcstr(glsl, ");\n");
            break;
        }
        case OPCODE_FIRSTBIT_SHI:  // signed high
        {
#ifdef _DEBUG
            AddIndentation(psContext);
            bcatcstr(glsl, "//FIRSTBIT_SHI\n");
#endif
            AddIndentation(psContext);
            TranslateOperand(psContext, &psInst->asOperands[0], TO_FLAG_INTEGER | TO_FLAG_DESTINATION);
            bcatcstr(glsl, " = findMSB(");
            TranslateOperand(psContext, &psInst->asOperands[1], TO_FLAG_INTEGER);
            bcatcstr(glsl, ");\n");
            break;
        }
        case OPCODE_BFREV:
        {
#ifdef _DEBUG
            AddIndentation(psContext);
            bcatcstr(glsl, "//BFREV\n");
#endif
            AddIndentation(psContext);
            TranslateOperand(psContext, &psInst->asOperands[0], TO_FLAG_INTEGER | TO_FLAG_DESTINATION);
            bcatcstr(glsl, " = bitfieldReverse(");
            TranslateOperand(psContext, &psInst->asOperands[1], TO_FLAG_INTEGER);
            bcatcstr(glsl, ");\n");
            break;
        }
        case OPCODE_BFI:
        {
            uint32_t numelements_width = GetNumSwizzleElements(&psInst->asOperands[1]);
            uint32_t numelements_offset = GetNumSwizzleElements(&psInst->asOperands[2]);
            uint32_t numelements_dest = GetNumSwizzleElements(&psInst->asOperands[0]);
            uint32_t numoverall_elements = min(min(numelements_width, numelements_offset), numelements_dest);
            uint32_t i, j;
            static const char* bfi_elementidx[] = {"x", "y", "z", "w"};
#ifdef _DEBUG
            AddIndentation(psContext);
            bcatcstr(glsl, "//BFI\n");
#endif

            AddIndentation(psContext);
            TranslateOperand(psContext, &psInst->asOperands[0], TO_FLAG_INTEGER | TO_FLAG_DESTINATION);
            bformata(glsl, " = ivec%d(", numoverall_elements);
            for (i = 0; i < numoverall_elements; ++i)
            {
                bcatcstr(glsl, "bitfieldInsert(");

                for (j = 4; j >= 1; --j)
                {
                    uint32_t opSwizzleCount = GetNumSwizzleElements(&psInst->asOperands[j]);

                    if (opSwizzleCount != 1)
                        bcatcstr(glsl, " (");
                    TranslateOperand(psContext, &psInst->asOperands[j], TO_FLAG_INTEGER);
                    if (opSwizzleCount != 1)
                        bformata(glsl, " ).%s", bfi_elementidx[i]);
                    if (j != 1)
                        bcatcstr(glsl, ",");
                }

                bcatcstr(glsl, ") ");
                if (i + 1 != numoverall_elements)
                    bcatcstr(glsl, ", ");
            }

            bcatcstr(glsl, ").");
            for (i = 0; i < numoverall_elements; ++i)
                bformata(glsl, "%s", bfi_elementidx[i]);
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
            bcatcstr(glsl, "//EMIT_STREAM\n");
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
            // Need to handle nesting.
            // Max of 4 for rep - 'Flow Control Limitations' http://msdn.microsoft.com/en-us/library/windows/desktop/bb219848(v=vs.85).aspx

            AddIndentation(psContext);
            bcatcstr(glsl, "RepCounter = ");
            TranslateOperandWithMask(psContext, &psInst->asOperands[0], TO_FLAG_INTEGER, OPERAND_4_COMPONENT_MASK_X);
            bcatcstr(glsl, ";\n");

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
                // DX9 version
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

            GLSLTranslateConditional(psContext, psInst, glsl);
            break;
        }
        case OPCODE_CONTINUEC:
        {
#ifdef _DEBUG
            AddIndentation(psContext);
            bcatcstr(glsl, "//CONTINUEC\n");
#endif
            AddIndentation(psContext);

            GLSLTranslateConditional(psContext, psInst, glsl);
            break;
        }
        case OPCODE_IF:
        {
#ifdef _DEBUG
            AddIndentation(psContext);
            bcatcstr(glsl, "//IF\n");
#endif
            AddIndentation(psContext);

            GLSLTranslateConditional(psContext, psInst, glsl);
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

            GLSLTranslateConditional(psContext, psInst, glsl);
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
            TranslateOperand(psContext, &psInst->asOperands[0], TO_FLAG_INTEGER);
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
            GLSLAddComparision(psContext, psInst, GLSL_CMP_EQ, TO_FLAG_NONE, NULL);
            break;
        }
        case OPCODE_USHR:
        {
#ifdef _DEBUG
            AddIndentation(psContext);
            bcatcstr(glsl, "//USHR\n");
#endif
            GLSLCallBinaryOp(psContext, ">>", psInst, 0, 1, 2, SVT_UINT);
            break;
        }
        case OPCODE_ISHL:
        {
            SHADER_VARIABLE_TYPE eType = SVT_INT;

#ifdef _DEBUG
            AddIndentation(psContext);
            bcatcstr(glsl, "//ISHL\n");
#endif

            if (GetOperandDataType(psContext, &psInst->asOperands[0]) == SVT_UINT)
            {
                eType = SVT_UINT;
            }

            GLSLCallBinaryOp(psContext, "<<", psInst, 0, 1, 2, eType);
            break;
        }
        case OPCODE_ISHR:
        {
            SHADER_VARIABLE_TYPE eType = SVT_INT;
#ifdef _DEBUG
            AddIndentation(psContext);
            bcatcstr(glsl, "//ISHR\n");
#endif

            if (GetOperandDataType(psContext, &psInst->asOperands[0]) == SVT_UINT)
            {
                eType = SVT_UINT;
            }

            GLSLCallBinaryOp(psContext, ">>", psInst, 0, 1, 2, eType);
            break;
        }
        case OPCODE_LD:
        case OPCODE_LD_MS:
        {
            ResourceBinding* psBinding = 0;
            uint32_t dstSwizCount = GetNumSwizzleElements(&psInst->asOperands[0]);
#ifdef _DEBUG
            AddIndentation(psContext);
            if (psInst->eOpcode == OPCODE_LD)
                bcatcstr(glsl, "//LD\n");
            else
                bcatcstr(glsl, "//LD_MS\n");
#endif

            GetResourceFromBindingPoint(RGROUP_TEXTURE, psInst->asOperands[2].ui32RegisterNumber, &psContext->psShader->sInfo, &psBinding);

            if (psInst->bAddressOffset)
            {
                GLSLTranslateTexelFetchOffset(psContext, psInst, psBinding, glsl);
            }
            else
            {
                GLSLTranslateTexelFetch(psContext, psInst, psBinding, glsl);
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
                TranslateOperand(psContext, &psInst->asOperands[0], TO_FLAG_NONE);

                if (psContext->psShader->ui32MajorVersion == 1)
                {
                    /* SM1.X only kills based on the rgb channels */
                    bcatcstr(glsl, ").xyz, vec3(0)))){discard;}\n");
                }
                else
                {
                    bcatcstr(glsl, "), vec4(0)))){discard;}\n");
                }
            }
            else if (psInst->eBooleanTestType == INSTRUCTION_TEST_ZERO)
            {
                bcatcstr(glsl, "if((");
                TranslateOperand(psContext, &psInst->asOperands[0], TO_FLAG_INTEGER);
                bcatcstr(glsl, ")==0){discard;}\n");
            }
            else
            {
                ASSERT(psInst->eBooleanTestType == INSTRUCTION_TEST_NONZERO);
                bcatcstr(glsl, "if((");
                TranslateOperand(psContext, &psInst->asOperands[0], TO_FLAG_INTEGER);
                bcatcstr(glsl, ")!=0){discard;}\n");
            }
            break;
        }
        case OPCODE_LOD:
        {
#ifdef _DEBUG
            AddIndentation(psContext);
            bcatcstr(glsl, "//LOD\n");
#endif
            // LOD computes the following vector (ClampedLOD, NonClampedLOD, 0, 0)

            AddIndentation(psContext);
            GLSLMETALAddAssignToDest(psContext, &psInst->asOperands[0], SVT_FLOAT, 4, &numParenthesis);

            // If the core language does not have query-lod feature,
            // then the extension is used. The name of the function
            // changed between extension and core.
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
            GLSLTranslateTexCoord(psContext, psContext->psShader->aeResourceDims[psInst->asOperands[2].ui32RegisterNumber], &psInst->asOperands[1]);
            bcatcstr(glsl, ")");

            // The swizzle on srcResource allows the returned values to be swizzled arbitrarily before they are written to the destination.

            // iWriteMaskEnabled is forced off during DecodeOperand because swizzle on sampler uniforms
            // does not make sense. But need to re-enable to correctly swizzle this particular instruction.
            psInst->asOperands[2].iWriteMaskEnabled = 1;
            TranslateOperandSwizzleWithMask(psContext, &psInst->asOperands[2], GetOperandWriteMask(&psInst->asOperands[0]));
            GLSLAddAssignPrologue(psContext, numParenthesis);
            break;
        }
        case OPCODE_EVAL_CENTROID:
        {
#ifdef _DEBUG
            AddIndentation(psContext);
            bcatcstr(glsl, "//EVAL_CENTROID\n");
#endif
            AddIndentation(psContext);
            TranslateOperand(psContext, &psInst->asOperands[0], TO_FLAG_DESTINATION);
            bcatcstr(glsl, " = interpolateAtCentroid(");
            // interpolateAtCentroid accepts in-qualified variables.
            // As long as bytecode only writes vX registers in declarations
            // we should be able to use the declared name directly.
            TranslateOperand(psContext, &psInst->asOperands[1], TO_FLAG_DECLARATION_NAME);
            bcatcstr(glsl, ");\n");
            break;
        }
        case OPCODE_EVAL_SAMPLE_INDEX:
        {
#ifdef _DEBUG
            AddIndentation(psContext);
            bcatcstr(glsl, "//EVAL_SAMPLE_INDEX\n");
#endif
            AddIndentation(psContext);
            TranslateOperand(psContext, &psInst->asOperands[0], TO_FLAG_DESTINATION);
            bcatcstr(glsl, " = interpolateAtSample(");
            // interpolateAtSample accepts in-qualified variables.
            // As long as bytecode only writes vX registers in declarations
            // we should be able to use the declared name directly.
            TranslateOperand(psContext, &psInst->asOperands[1], TO_FLAG_DECLARATION_NAME);
            bcatcstr(glsl, ", ");
            TranslateOperand(psContext, &psInst->asOperands[2], TO_FLAG_INTEGER);
            bcatcstr(glsl, ");\n");
            break;
        }
        case OPCODE_EVAL_SNAPPED:
        {
#ifdef _DEBUG
            AddIndentation(psContext);
            bcatcstr(glsl, "//EVAL_SNAPPED\n");
#endif
            AddIndentation(psContext);
            TranslateOperand(psContext, &psInst->asOperands[0], TO_FLAG_DESTINATION);
            bcatcstr(glsl, " = interpolateAtOffset(");
            // interpolateAtOffset accepts in-qualified variables.
            // As long as bytecode only writes vX registers in declarations
            // we should be able to use the declared name directly.
            TranslateOperand(psContext, &psInst->asOperands[1], TO_FLAG_DECLARATION_NAME);
            bcatcstr(glsl, ", ");
            TranslateOperand(psContext, &psInst->asOperands[2], TO_FLAG_INTEGER);
            bcatcstr(glsl, ".xy);\n");
            break;
        }
        case OPCODE_LD_STRUCTURED:
        {
#ifdef _DEBUG
            AddIndentation(psContext);
            bcatcstr(glsl, "//LD_STRUCTURED\n");
#endif
            GLSLTranslateShaderStorageLoad(psContext, psInst);
            break;
        }
        case OPCODE_LD_UAV_TYPED:
        {
#ifdef _DEBUG
            AddIndentation(psContext);
            bcatcstr(glsl, "//LD_UAV_TYPED\n");
#endif
            switch (psInst->eResDim)
            {
                case RESOURCE_DIMENSION_TEXTURE1D:
                    AddIndentation(psContext);
                    TranslateOperand(psContext, &psInst->asOperands[0], TO_FLAG_DESTINATION);
                    bcatcstr(glsl, " = imageLoad(");
                    TranslateOperand(psContext, &psInst->asOperands[2], TO_FLAG_NAME_ONLY);
                    bcatcstr(glsl, ", (");
                    TranslateOperand(psContext, &psInst->asOperands[1], TO_FLAG_INTEGER);
                    bformata(glsl, ").x)");
                    TranslateOperandSwizzle(psContext, &psInst->asOperands[0]);
                    bcatcstr(glsl, ";\n");
                    break;
                case RESOURCE_DIMENSION_TEXTURECUBE:
                case RESOURCE_DIMENSION_TEXTURE1DARRAY:
                case RESOURCE_DIMENSION_TEXTURE2D:
                case RESOURCE_DIMENSION_TEXTURE2DMS:
                    AddIndentation(psContext);
                    TranslateOperand(psContext, &psInst->asOperands[0], TO_FLAG_DESTINATION);
                    bcatcstr(glsl, " = imageLoad(");
                    TranslateOperand(psContext, &psInst->asOperands[2], TO_FLAG_NAME_ONLY);
                    bcatcstr(glsl, ", (");
                    TranslateOperand(psContext, &psInst->asOperands[1], TO_FLAG_INTEGER);
                    bformata(glsl, ").xy)");
                    TranslateOperandSwizzle(psContext, &psInst->asOperands[0]);
                    bcatcstr(glsl, ";\n");
                    break;
                case RESOURCE_DIMENSION_TEXTURE3D:
                case RESOURCE_DIMENSION_TEXTURE2DARRAY:
                case RESOURCE_DIMENSION_TEXTURE2DMSARRAY:
                case RESOURCE_DIMENSION_TEXTURECUBEARRAY:
                    AddIndentation(psContext);
                    TranslateOperand(psContext, &psInst->asOperands[0], TO_FLAG_DESTINATION);
                    bcatcstr(glsl, " = imageLoad(");
                    TranslateOperand(psContext, &psInst->asOperands[2], TO_FLAG_NAME_ONLY);
                    bcatcstr(glsl, ", (");
                    TranslateOperand(psContext, &psInst->asOperands[1], TO_FLAG_INTEGER);
                    bformata(glsl, ").xyz)");
                    TranslateOperandSwizzle(psContext, &psInst->asOperands[0]);
                    bcatcstr(glsl, ";\n");
                    break;
            }
            break;
        }
        case OPCODE_STORE_RAW:
        {
#ifdef _DEBUG
            AddIndentation(psContext);
            bcatcstr(glsl, "//STORE_RAW\n");
#endif
            GLSLTranslateShaderStorageStore(psContext, psInst);
            break;
        }
        case OPCODE_STORE_STRUCTURED:
        {
#ifdef _DEBUG
            AddIndentation(psContext);
            bcatcstr(glsl, "//STORE_STRUCTURED\n");
#endif
            GLSLTranslateShaderStorageStore(psContext, psInst);
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
                case REFLECT_RESOURCE_DIMENSION_TEXTURE1D:
                    bcatcstr(glsl, ", int(");
                    TranslateOperand(psContext, &psInst->asOperands[1], TO_FLAG_NAME_ONLY);
                    bcatcstr(glsl, "), ");
                    break;
                case REFLECT_RESOURCE_DIMENSION_TEXTURE2D:
                case REFLECT_RESOURCE_DIMENSION_TEXTURE1DARRAY:
                case REFLECT_RESOURCE_DIMENSION_TEXTURE2DMS:
                    bcatcstr(glsl, ", ivec2(");
                    TranslateOperand(psContext, &psInst->asOperands[1], TO_FLAG_NAME_ONLY);
                    bcatcstr(glsl, ".xy), ");
                    break;
                case REFLECT_RESOURCE_DIMENSION_TEXTURE2DARRAY:
                case REFLECT_RESOURCE_DIMENSION_TEXTURE3D:
                case REFLECT_RESOURCE_DIMENSION_TEXTURE2DMSARRAY:
                case REFLECT_RESOURCE_DIMENSION_TEXTURECUBE:
                    bcatcstr(glsl, ", ivec3(");
                    TranslateOperand(psContext, &psInst->asOperands[1], TO_FLAG_NAME_ONLY);
                    bcatcstr(glsl, ".xyz), ");
                    break;
                case REFLECT_RESOURCE_DIMENSION_TEXTURECUBEARRAY:
                    bcatcstr(glsl, ", ivec4(");
                    TranslateOperand(psContext, &psInst->asOperands[1], TO_FLAG_NAME_ONLY);
                    bcatcstr(glsl, ".xyzw) ");
                    break;
            };

            TranslateOperand(psContext, &psInst->asOperands[2], GLSLResourceReturnTypeToFlag(psRes->ui32ReturnType));
            bformata(glsl, ");\n");

            break;
        }
        case OPCODE_LD_RAW:
        {
#ifdef _DEBUG
            AddIndentation(psContext);
            bcatcstr(glsl, "//LD_RAW\n");
#endif

            GLSLTranslateShaderStorageLoad(psContext, psInst);
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
#ifdef _DEBUG
            AddIndentation(psContext);
            if (psInst->eOpcode == OPCODE_UBFE)
                bcatcstr(glsl, "//OPCODE_UBFE\n");
            else
                bcatcstr(glsl, "//OPCODE_IBFE\n");
#endif
            AddIndentation(psContext);
            TranslateOperand(psContext, &psInst->asOperands[0], TO_FLAG_DESTINATION);
            bcatcstr(glsl, " = bitfieldExtract(");
            TranslateOperand(psContext, &psInst->asOperands[3], TO_FLAG_NONE);
            bcatcstr(glsl, ", ");
            TranslateOperand(psContext, &psInst->asOperands[2], TO_FLAG_NONE);
            bcatcstr(glsl, ", ");
            TranslateOperand(psContext, &psInst->asOperands[1], TO_FLAG_NONE);
            bcatcstr(glsl, ");\n");
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
            TranslateOperand(psContext, &psInst->asOperands[0], TO_FLAG_DESTINATION);
            bcatcstr(glsl, " = (vec4(1.0) / vec4(");
            TranslateOperand(psContext, &psInst->asOperands[1], TO_FLAG_NONE);
            bcatcstr(glsl, "))");
            AddSwizzleUsingElementCount(psContext, destElemCount);
            bcatcstr(glsl, ";\n");
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

                // unpackHalf2x16 converts two f16s packed into uint to two f32s.

                // dest.swiz.x = unpackHalf2x16(src.swiz.x).x
                // dest.swiz.y = unpackHalf2x16(src.swiz.y).x
                // dest.swiz.z = unpackHalf2x16(src.swiz.z).x
                // dest.swiz.w = unpackHalf2x16(src.swiz.w).x

                AddIndentation(psContext);
                TranslateOperand(psContext, &psInst->asOperands[0], TO_FLAG_DESTINATION);
                if (destElemCount > 1)
                    bcatcstr(glsl, swizzle[destElem]);

                bcatcstr(glsl, " = unpackHalf2x16(");
                TranslateOperand(psContext, &psInst->asOperands[1], TO_FLAG_UNSIGNED_INTEGER);
                if (s0ElemCount > 1)
                    bcatcstr(glsl, swizzle[destElem]);
                bcatcstr(glsl, ").x;\n");
            }
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

                // packHalf2x16 converts two f32s to two f16s packed into a uint.

                // dest.swiz.x = packHalf2x16(vec2(src.swiz.x)) & 0xFFFF
                // dest.swiz.y = packHalf2x16(vec2(src.swiz.y)) & 0xFFFF
                // dest.swiz.z = packHalf2x16(vec2(src.swiz.z)) & 0xFFFF
                // dest.swiz.w = packHalf2x16(vec2(src.swiz.w)) & 0xFFFF

                AddIndentation(psContext);
                TranslateOperand(psContext, &psInst->asOperands[0], TO_FLAG_DESTINATION | TO_FLAG_UNSIGNED_INTEGER);
                if (destElemCount > 1)
                    bcatcstr(glsl, swizzle[destElem]);

                bcatcstr(glsl, " = packHalf2x16(vec2(");
                TranslateOperand(psContext, &psInst->asOperands[1], TO_FLAG_NONE);
                if (s0ElemCount > 1)
                    bcatcstr(glsl, swizzle[destElem]);
                bcatcstr(glsl, ")) & 0xFFFF;\n");
            }
            break;
        }
        case OPCODE_INEG:
        {
#ifdef _DEBUG
            AddIndentation(psContext);
            bcatcstr(glsl, "//INEG\n");
#endif
            // dest = 0 - src0
            AddIndentation(psContext);
            TranslateOperand(psContext, &psInst->asOperands[0], TO_FLAG_DESTINATION | TO_FLAG_INTEGER);
            bcatcstr(glsl, " = 0 - ");
            TranslateOperand(psContext, &psInst->asOperands[1], TO_FLAG_NONE | TO_FLAG_INTEGER);
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
            GLSLCallHelper1(psContext, "dFdx", psInst, 0, 1, 1);
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
            GLSLCallHelper1(psContext, "dFdy", psInst, 0, 1, 1);
            break;
        }
        case OPCODE_LRP:
        {
#ifdef _DEBUG
            AddIndentation(psContext);
            bcatcstr(glsl, "//LRP\n");
#endif
            GLSLCallHelper3(psContext, "mix", psInst, 0, 2, 3, 1, 1);
            break;
        }
        case OPCODE_DP2ADD:
        {
#ifdef _DEBUG
            AddIndentation(psContext);
            bcatcstr(glsl, "//DP2ADD\n");
#endif
            AddIndentation(psContext);
            TranslateOperand(psContext, &psInst->asOperands[0], TO_FLAG_DESTINATION);
            bcatcstr(glsl, " = dot(vec2(");
            TranslateOperand(psContext, &psInst->asOperands[1], TO_FLAG_NONE);
            bcatcstr(glsl, "), vec2(");
            TranslateOperand(psContext, &psInst->asOperands[2], TO_FLAG_NONE);
            bcatcstr(glsl, ")) + ");
            TranslateOperand(psContext, &psInst->asOperands[3], TO_FLAG_NONE);
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
            TranslateOperand(psContext, &psInst->asOperands[0], TO_FLAG_DESTINATION);
            bcatcstr(glsl, " = pow(abs(");
            TranslateOperand(psContext, &psInst->asOperands[1], TO_FLAG_NONE);
            bcatcstr(glsl, "), ");
            TranslateOperand(psContext, &psInst->asOperands[2], TO_FLAG_NONE);
            bcatcstr(glsl, ");\n");
            break;
        }

        case OPCODE_IMM_ATOMIC_ALLOC:
        {
#ifdef _DEBUG
            AddIndentation(psContext);
            bcatcstr(glsl, "//IMM_ATOMIC_ALLOC\n");
#endif
            AddIndentation(psContext);
            TranslateOperand(psContext, &psInst->asOperands[0], TO_FLAG_DESTINATION);
            bcatcstr(glsl, " = int(atomicCounterIncrement(");
            ResourceName(glsl, psContext, RGROUP_UAV, psInst->asOperands[1].ui32RegisterNumber, 0);
            bformata(glsl, "_counter");
            bcatcstr(glsl, "));\n");
            break;
        }
        case OPCODE_IMM_ATOMIC_CONSUME:
        {
#ifdef _DEBUG
            AddIndentation(psContext);
            bcatcstr(glsl, "//IMM_ATOMIC_CONSUME\n");
#endif
            AddIndentation(psContext);
            TranslateOperand(psContext, &psInst->asOperands[0], TO_FLAG_DESTINATION);
            // Temps are always signed and atomci counters are always unsigned
            // at the moment.
            bcatcstr(glsl, " = int(atomicCounterDecrement(");
            ResourceName(glsl, psContext, RGROUP_UAV, psInst->asOperands[1].ui32RegisterNumber, 0);
            bformata(glsl, "_counter");
            bcatcstr(glsl, "));\n");
            break;
        }

        case OPCODE_NOT:
        {
#ifdef _DEBUG
            AddIndentation(psContext);
            bcatcstr(glsl, "//INOT\n");
#endif
            AddIndentation(psContext);
            GLSLMETALAddAssignToDest(psContext, &psInst->asOperands[0], SVT_INT, GetNumSwizzleElements(&psInst->asOperands[1]), &numParenthesis);

            bcatcstr(glsl, "~");
            TranslateOperandWithMask(psContext, &psInst->asOperands[1], TO_FLAG_INTEGER, GetOperandWriteMask(&psInst->asOperands[0]));
            GLSLAddAssignPrologue(psContext, numParenthesis);
            break;
        }
        case OPCODE_XOR:
        {
#ifdef _DEBUG
            AddIndentation(psContext);
            bcatcstr(glsl, "//XOR\n");
#endif

            GLSLCallBinaryOp(psContext, "^", psInst, 0, 1, 2, SVT_UINT);
            break;
        }
        case OPCODE_RESINFO:
        {
            const RESOURCE_DIMENSION eResDim = psContext->psShader->aeResourceDims[psInst->asOperands[2].ui32RegisterNumber];
            const RESINFO_RETURN_TYPE eResInfoReturnType = psInst->eResInfoReturnType;
            uint32_t destElemCount = GetNumSwizzleElements(&psInst->asOperands[0]);
            uint32_t destElem;
#ifdef _DEBUG
            AddIndentation(psContext);
            bcatcstr(glsl, "//RESINFO\n");
#endif

            for (destElem = 0; destElem < destElemCount; ++destElem)
            {
                const char* swizzle[] = {".x", ".y", ".z", ".w"};

                GetResInfoData(psContext, psInst, psInst->asOperands[2].aui32Swizzle[destElem], destElem);
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

    if (psInst->bSaturate)  // Saturate is only for floating point data (float opcodes or MOV)
    {
        int dstCount = GetNumSwizzleElements(&psInst->asOperands[0]);
        AddIndentation(psContext);
        GLSLMETALAddAssignToDest(psContext, &psInst->asOperands[0], SVT_FLOAT, dstCount, &numParenthesis);
        bcatcstr(glsl, "clamp(");

        TranslateOperand(psContext, &psInst->asOperands[0], TO_AUTO_BITCAST_TO_FLOAT);
        bcatcstr(glsl, ", 0.0, 1.0)");
        GLSLAddAssignPrologue(psContext, numParenthesis);
    }
}

static int GLSLIsIntegerImmediateOpcode(OPCODE_TYPE eOpcode)
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
        case OPCODE_USHR:
        case OPCODE_AND:
        case OPCODE_OR:
        case OPCODE_XOR:
        case OPCODE_BREAKC:
        case OPCODE_CONTINUEC:
        case OPCODE_RETC:
        case OPCODE_DISCARD:
            // MOV is typeless.
            // Treat immediates as int, bitcast to float if necessary
        case OPCODE_MOV:
        case OPCODE_MOVC:
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
    const uint32_t count = psContext->psShader->asPhase[MAIN_PHASE].pui32InstCount[0];
    Instruction* psInst = psContext->psShader->asPhase[MAIN_PHASE].ppsInst[0];
    uint32_t i;

    for (i = 0; i < count;)
    {
        if (psInst[i].eOpcode == OPCODE_MOV && psInst[i].asOperands[1].eType == OPERAND_TYPE_IMMEDIATE32 && psInst[i].asOperands[0].eType == OPERAND_TYPE_TEMP)
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
                    if (GLSLIsIntegerImmediateOpcode(psInst[k].eOpcode))
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
