// Modifications copyright Amazon.com, Inc. or its affiliates
// Modifications copyright Crytek GmbH

#include "internal_includes/toMETALInstruction.h"
#include "internal_includes/toMETALOperand.h"
#include "internal_includes/languages.h"
#include "bstrlib.h"
#include "stdio.h"
#include <stdlib.h>
#include "hlslcc.h"
#include <internal_includes/toGLSLOperand.h>
#include "internal_includes/debug.h"

extern void AddIndentation(HLSLCrossCompilerContext* psContext);
static int METALIsIntegerImmediateOpcode(OPCODE_TYPE eOpcode);

// Calculate the bits set in mask
static int METALWriteMaskToComponentCount(uint32_t writeMask)
{
    uint32_t count;
    // In HLSL bytecode writemask 0 also means everything
    if (writeMask == 0)
    {
        return 4;
    }

    // Count bits set
    // https://graphics.stanford.edu/~seander/bithacks.html#CountBitsSet64
    count = (writeMask * 0x200040008001ULL & 0x111111111111111ULL) % 0xf;

    return (int)count;
}

static uint32_t METALBuildComponentMaskFromElementCount(int count)
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
static void METALAddOpAssignToDestWithMask(HLSLCrossCompilerContext* psContext, const Operand* psDest,
    SHADER_VARIABLE_TYPE eSrcType, uint32_t ui32SrcElementCount, const char* szAssignmentOp, int* pNeedsParenthesis, uint32_t ui32CompMask)
{
    uint32_t ui32DestElementCount = GetNumSwizzleElementsWithMaskMETAL(psDest, ui32CompMask);
    bstring metal = *psContext->currentShaderString;
    SHADER_VARIABLE_TYPE eDestDataType = GetOperandDataTypeMETAL(psContext, psDest);
    ASSERT(pNeedsParenthesis != NULL);

    *pNeedsParenthesis = 0;

    uint32_t flags = TO_FLAG_DESTINATION;
    // Default is full floats. Handle half floats if the source is half precision
    if (eSrcType == SVT_FLOAT16)
    {
        flags |= TO_FLAG_FLOAT16;
    }
    TranslateOperandWithMaskMETAL(psContext, psDest, flags, ui32CompMask);
    
    //GMEM data output types can only be full floats.
    if(eDestDataType== SVT_FLOAT16 && psDest->eType== OPERAND_TYPE_OUTPUT && psContext->gmemOutputNumElements[0]>0 )
    {
        eDestDataType = SVT_FLOAT;
    }
    
    // Simple path: types match.
    if (eDestDataType == eSrcType)
    {
        // Cover cases where the HLSL language expects the rest of the components to be default-filled
        // eg. MOV r0, c0.x => Temp[0] = vec4(c0.x);
        if (ui32DestElementCount > ui32SrcElementCount)
        {
            bformata(metal, " %s %s(", szAssignmentOp, GetConstructorForTypeMETAL(eDestDataType, ui32DestElementCount));
            *pNeedsParenthesis = 1;
        }
        else
        {
            bformata(metal, " %s ", szAssignmentOp);
        }
        return;
    }

    switch (eDestDataType)
    {
    case SVT_INT:
    {
        if (1 == ui32DestElementCount)
        {
            bformata(metal, " %s as_type<int>(", szAssignmentOp);
        }
        else
        {
            bformata(metal, "%s as_type<int%d>(", szAssignmentOp, ui32DestElementCount);
        }
        break;
    }
    case SVT_UINT:
    {
        if (1 == ui32DestElementCount)
        {
            bformata(metal, " %s as_type<uint>(", szAssignmentOp);
        }
        else
        {
            bformata(metal, "%s as_type<uint%d>(", szAssignmentOp, ui32DestElementCount);
        }
        break;
    }
    case SVT_FLOAT:
    {
        const char* castType = eSrcType == SVT_FLOAT16 ? "static_cast" : "as_type";
        if (1 == ui32DestElementCount)
        {
            bformata(metal, " %s %s<float>(", szAssignmentOp, castType);
        }
        else
        {
            bformata(metal, "%s %s<float%d>(", szAssignmentOp, castType, ui32DestElementCount);
        }
        break;
    }
    case SVT_FLOAT16:
    {
        if (1 == ui32DestElementCount)
        {
            bformata(metal, " %s static_cast<half>(", szAssignmentOp);
        }
        else
        {
            bformata(metal, "%s static_cast<half%d>(", szAssignmentOp, ui32DestElementCount);
        }
        break;
    }
    default:
        // TODO: Handle bools?
        break;
    }
    
    switch (eDestDataType)
    {
        case SVT_INT:
        case SVT_UINT:
        case SVT_FLOAT:
        case SVT_FLOAT16:
        {
            // Cover cases where the HLSL language expects the rest of the components to be default-filled
            if (ui32DestElementCount > ui32SrcElementCount)
            {
                bformata(metal, "%s(", GetConstructorForTypeMETAL(eSrcType, ui32DestElementCount));
                (*pNeedsParenthesis)++;
            }
        }
    }
    (*pNeedsParenthesis)++;
    return;
}

static void METALAddAssignToDest(HLSLCrossCompilerContext* psContext, const Operand* psDest,
    SHADER_VARIABLE_TYPE eSrcType, uint32_t ui32SrcElementCount, int* pNeedsParenthesis)
{
    METALAddOpAssignToDestWithMask(psContext, psDest, eSrcType, ui32SrcElementCount, "=", pNeedsParenthesis, OPERAND_4_COMPONENT_MASK_ALL);
}

static void METALAddAssignPrologue(HLSLCrossCompilerContext* psContext, int numParenthesis)
{
    bstring glsl = *psContext->currentShaderString;
    while (numParenthesis != 0)
    {
        bcatcstr(glsl, ")");
        numParenthesis--;
    }
    bcatcstr(glsl, ";\n");
}
static uint32_t METALResourceReturnTypeToFlag(const RESOURCE_RETURN_TYPE eType)
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
    METAL_CMP_EQ,
    METAL_CMP_LT,
    METAL_CMP_GE,
    METAL_CMP_NE,
} METALComparisonType;

static void METALAddComparision(HLSLCrossCompilerContext* psContext, Instruction* psInst, METALComparisonType eType,
    uint32_t typeFlag, Instruction* psNextInst)
{
    (void)psNextInst;

    // Multiple cases to consider here:
    // For shader model <=3: all comparisons are floats
    // otherwise:
    // OPCODE_LT, _GT, _NE etc: inputs are floats, outputs UINT 0xffffffff or 0. typeflag: TO_FLAG_NONE
    // OPCODE_ILT, _IGT etc: comparisons are signed ints, outputs UINT 0xffffffff or 0 typeflag TO_FLAG_INTEGER
    // _ULT, UGT etc: inputs unsigned ints, outputs UINTs typeflag TO_FLAG_UNSIGNED_INTEGER
    //
    // Additional complexity: if dest swizzle element count is 1, we can use normal comparison operators, otherwise glsl intrinsics.

    uint32_t orig_type = typeFlag;

    bstring metal = *psContext->currentShaderString;
    const uint32_t destElemCount = GetNumSwizzleElementsMETAL(&psInst->asOperands[0]);
    const uint32_t s0ElemCount = GetNumSwizzleElementsMETAL(&psInst->asOperands[1]);
    const uint32_t s1ElemCount = GetNumSwizzleElementsMETAL(&psInst->asOperands[2]);

    uint32_t minElemCount = destElemCount < s0ElemCount ? destElemCount : s0ElemCount;

    int needsParenthesis = 0;

    ASSERT(s0ElemCount == s1ElemCount || s1ElemCount == 1 || s0ElemCount == 1);
    if (s0ElemCount != s1ElemCount)
    {
        // Set the proper auto-expand flag is either argument is scalar
        typeFlag |= (TO_AUTO_EXPAND_TO_VEC2 << (max(s0ElemCount, s1ElemCount) - 2));
    }

    const char* metalOpcode[] = {
        "==",
        "<",
        ">=",
        "!=",
    };

    //Scalar compare

    // Optimization shortcut for the IGE+BREAKC_NZ combo:
    // First print out the if(cond)->break directly, and then
    // to guarantee correctness with side-effects, re-run
    // the actual comparison. In most cases, the second run will
    // be removed by the shader compiler optimizer pass (dead code elimination)
    // This also makes it easier for some GLSL optimizers to recognize the for loop.

    //if (psInst->eOpcode == OPCODE_IGE &&
    //  psNextInst &&
    //  psNextInst->eOpcode == OPCODE_BREAKC &&
    //  (psInst->asOperands[0].ui32RegisterNumber == psNextInst->asOperands[0].ui32RegisterNumber))
    //{

    //  AddIndentation(psContext);
    //  bcatcstr(glsl, "// IGE+BREAKC opt\n");
    //  AddIndentation(psContext);

    //  if (psNextInst->eBooleanTestType == INSTRUCTION_TEST_NONZERO)
    //      bcatcstr(glsl, "if ((");
    //  else
    //      bcatcstr(glsl, "if (!(");
    //  TranslateOperand(psContext, &psInst->asOperands[1], typeFlag);
    //  bformata(glsl, "%s ", glslOpcode[eType]);
    //  TranslateOperand(psContext, &psInst->asOperands[2], typeFlag);
    //  bcatcstr(glsl, ")) { break; }\n");

    //  // Mark the BREAKC instruction as already handled
    //  psNextInst->eOpcode = OPCODE_NOP;

    //  // Continue as usual
    //}

    AddIndentation(psContext);
    METALAddAssignToDest(psContext, &psInst->asOperands[0], SVT_INT, destElemCount, &needsParenthesis);

    bcatcstr(metal, "select(");

    /* Confetti note:
       ASM returns 0XFFFFFFFF or 0
       It's important to use int.
       A sign intrinsic converts to the following:
       lt r0.x, l(0.000000), v0.z
       lt r0.y, v0.z, l(0.000000)
       iadd r0.x, -r0.x, r0.y
       itof o0.xyzw, r0.xxxx
     */

    if (destElemCount == 1)
    {
        bcatcstr(metal, "0, (int)0xFFFFFFFF, (");
    }
    else
    {
        bformata(metal, "int%d(0), int%d(0xFFFFFFFF), (", destElemCount, destElemCount);
    }

    TranslateOperandMETAL(psContext, &psInst->asOperands[1], typeFlag);
    bcatcstr(metal, ")");
    if (destElemCount > 1)
    {
        TranslateOperandSwizzleMETAL(psContext, &psInst->asOperands[0]);
    }
    else if (s0ElemCount > minElemCount)
    {
        AddSwizzleUsingElementCountMETAL(psContext, minElemCount);
    }
    bformata(metal, " %s (", metalOpcode[eType]);
    TranslateOperandMETAL(psContext, &psInst->asOperands[2], typeFlag);
    bcatcstr(metal, ")");
    if (destElemCount > 1)
    {
        TranslateOperandSwizzleMETAL(psContext, &psInst->asOperands[0]);
    }
    else if (s1ElemCount > minElemCount || orig_type != typeFlag)
    {
        AddSwizzleUsingElementCountMETAL(psContext, minElemCount);
    }
    bcatcstr(metal, ")");
    METALAddAssignPrologue(psContext, needsParenthesis);
}


static void METALAddMOVBinaryOp(HLSLCrossCompilerContext* psContext, const Operand* pDest, Operand* pSrc)
{
    int numParenthesis = 0;
    int destComponents = GetMaxComponentFromComponentMaskMETAL(pDest);
    int srcSwizzleCount = GetNumSwizzleElementsMETAL(pSrc);
    uint32_t writeMask = GetOperandWriteMaskMETAL(pDest);

    const SHADER_VARIABLE_TYPE eSrcType = GetOperandDataTypeExMETAL(psContext, pSrc, GetOperandDataTypeMETAL(psContext, pDest));
    uint32_t flags = SVTTypeToFlagMETAL(eSrcType);

    METALAddAssignToDest(psContext, pDest, eSrcType, srcSwizzleCount, &numParenthesis);
    TranslateOperandWithMaskMETAL(psContext, pSrc, flags, writeMask);

    METALAddAssignPrologue(psContext, numParenthesis);
}

static uint32_t METALElemCountToAutoExpandFlag(uint32_t elemCount)
{
    return TO_AUTO_EXPAND_TO_VEC2 << (elemCount - 2);
}

static void METALAddMOVCBinaryOp(HLSLCrossCompilerContext* psContext, const Operand* pDest, const Operand* src0, Operand* src1, Operand* src2)
{
    bstring metal = *psContext->currentShaderString;
    uint32_t destElemCount = GetNumSwizzleElementsMETAL(pDest);
    uint32_t s0ElemCount = GetNumSwizzleElementsMETAL(src0);
    uint32_t s1ElemCount = GetNumSwizzleElementsMETAL(src1);
    uint32_t s2ElemCount = GetNumSwizzleElementsMETAL(src2);
    uint32_t destWriteMask = GetOperandWriteMaskMETAL(pDest);
    uint32_t destElem;

    const SHADER_VARIABLE_TYPE eDestType = GetOperandDataTypeMETAL(psContext, pDest);
    const SHADER_VARIABLE_TYPE eSrc0Type = GetOperandDataTypeMETAL(psContext, src0);
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
    if (s0ElemCount == 1 || IsSwizzleReplicatedMETAL(src0))
    {
        int numParenthesis = 0;
        AddIndentation(psContext);

        bcatcstr(metal, "if (");
        TranslateOperandMETAL(psContext, src0, TO_AUTO_BITCAST_TO_INT);
        if (s0ElemCount > 1)
        {
            bcatcstr(metal, ".x");
        }

        bcatcstr(metal, " != 0)\n");
        AddIndentation(psContext);
        AddIndentation(psContext);

        METALAddAssignToDest(psContext, pDest, eDestType, destElemCount, &numParenthesis);

        if (s1ElemCount == 1 && destElemCount > 1)
        {
            TranslateOperandMETAL(psContext, src1, SVTTypeToFlagMETAL(eDestType) | METALElemCountToAutoExpandFlag(destElemCount));
        }
        else
        {
            TranslateOperandWithMaskMETAL(psContext, src1, SVTTypeToFlagMETAL(eDestType), destWriteMask);
        }

        bcatcstr(metal, ";\n");
        AddIndentation(psContext);
        bcatcstr(metal, "else\n");
        AddIndentation(psContext);
        AddIndentation(psContext);

        METALAddAssignToDest(psContext, pDest, eDestType, destElemCount, &numParenthesis);

        if (s2ElemCount == 1 && destElemCount > 1)
        {
            TranslateOperandMETAL(psContext, src2, SVTTypeToFlagMETAL(eDestType) | METALElemCountToAutoExpandFlag(destElemCount));
        }
        else
        {
            TranslateOperandWithMaskMETAL(psContext, src2, SVTTypeToFlagMETAL(eDestType), destWriteMask);
        }

        METALAddAssignPrologue(psContext, numParenthesis);
    }
    else
    {
        // TODO: We can actually do this in one op using mix().
        int srcElem = 0;
        for (destElem = 0; destElem < 4; ++destElem)
        {
            int numParenthesis = 0;
            if (pDest->eSelMode == OPERAND_4_COMPONENT_MASK_MODE && pDest->ui32CompMask != 0 && !(pDest->ui32CompMask & (1 << destElem)))
            {
                continue;
            }

            AddIndentation(psContext);

            bcatcstr(metal, "if (");
            TranslateOperandWithMaskMETAL(psContext, src0, TO_AUTO_BITCAST_TO_INT, 1 << destElem);
            bcatcstr(metal, " != 0)\n");

            AddIndentation(psContext);
            AddIndentation(psContext);

            METALAddOpAssignToDestWithMask(psContext, pDest, eDestType, 1, "=", &numParenthesis, 1 << destElem);

            TranslateOperandWithMaskMETAL(psContext, src1, SVTTypeToFlagMETAL(eDestType), 1 << destElem);

            bcatcstr(metal, ";\n");
            AddIndentation(psContext);
            bcatcstr(metal, "else\n");
            AddIndentation(psContext);
            AddIndentation(psContext);

            METALAddOpAssignToDestWithMask(psContext, pDest, eDestType, 1, "=", &numParenthesis, 1 << destElem);
            TranslateOperandWithMaskMETAL(psContext, src2, SVTTypeToFlagMETAL(eDestType), 1 << destElem);

            METALAddAssignPrologue(psContext, numParenthesis);

            srcElem++;
        }
    }
}

// Returns nonzero if operands are identical, only cares about temp registers currently.
static int METALAreTempOperandsIdentical(const Operand* psA, const Operand* psB)
{
    if (!psA || !psB)
    {
        return 0;
    }

    if (psA->eType != OPERAND_TYPE_TEMP || psB->eType != OPERAND_TYPE_TEMP)
    {
        return 0;
    }

    if (psA->eModifier != psB->eModifier)
    {
        return 0;
    }

    if (psA->iNumComponents != psB->iNumComponents)
    {
        return 0;
    }

    if (psA->ui32RegisterNumber != psB->ui32RegisterNumber)
    {
        return 0;
    }

    if (psA->eSelMode != psB->eSelMode)
    {
        return 0;
    }

    if (psA->eSelMode == OPERAND_4_COMPONENT_MASK_MODE && psA->ui32CompMask != psB->ui32CompMask)
    {
        return 0;
    }

    if (psA->eSelMode != OPERAND_4_COMPONENT_MASK_MODE && psA->ui32Swizzle != psB->ui32Swizzle)
    {
        return 0;
    }

    return 1;
}

// Returns nonzero if the operation is commutative
static int METALIsOperationCommutative(OPCODE_TYPE eOpCode)
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
    }
}

static void METALCallBinaryOp(HLSLCrossCompilerContext* psContext, const char* name, Instruction* psInst,
    int dest, int src0, int src1, SHADER_VARIABLE_TYPE eDataType)
{
    bstring glsl = *psContext->currentShaderString;
    uint32_t src1SwizCount = GetNumSwizzleElementsMETAL(&psInst->asOperands[src1]);
    uint32_t src0SwizCount = GetNumSwizzleElementsMETAL(&psInst->asOperands[src0]);
    uint32_t dstSwizCount = GetNumSwizzleElementsMETAL(&psInst->asOperands[dest]);
    uint32_t destMask = GetOperandWriteMaskMETAL(&psInst->asOperands[dest]);
    int needsParenthesis = 0;

    AddIndentation(psContext);

    if (src1SwizCount == src0SwizCount == dstSwizCount)
    {
        // Optimization for readability (and to make for loops in WebGL happy): detect cases where either src == dest and emit +=, -= etc. instead.
        if (METALAreTempOperandsIdentical(&psInst->asOperands[dest], &psInst->asOperands[src0]) != 0)
        {
            METALAddOpAssignToDestWithMask(psContext, &psInst->asOperands[dest], eDataType, dstSwizCount, name, &needsParenthesis, OPERAND_4_COMPONENT_MASK_ALL);
            TranslateOperandMETAL(psContext, &psInst->asOperands[src1], SVTTypeToFlagMETAL(eDataType));
            METALAddAssignPrologue(psContext, needsParenthesis);
            return;
        }
        else if (METALAreTempOperandsIdentical(&psInst->asOperands[dest], &psInst->asOperands[src1]) != 0 && (METALIsOperationCommutative(psInst->eOpcode) != 0))
        {
            METALAddOpAssignToDestWithMask(psContext, &psInst->asOperands[dest], eDataType, dstSwizCount, name, &needsParenthesis, OPERAND_4_COMPONENT_MASK_ALL);
            TranslateOperandMETAL(psContext, &psInst->asOperands[src0], SVTTypeToFlagMETAL(eDataType));
            METALAddAssignPrologue(psContext, needsParenthesis);
            return;
        }
    }

    METALAddAssignToDest(psContext, &psInst->asOperands[dest], eDataType, dstSwizCount, &needsParenthesis);

    TranslateOperandWithMaskMETAL(psContext, &psInst->asOperands[src0], SVTTypeToFlagMETAL(eDataType), destMask);
    bformata(glsl, " %s ", name);
    TranslateOperandWithMaskMETAL(psContext, &psInst->asOperands[src1], SVTTypeToFlagMETAL(eDataType), destMask);
    METALAddAssignPrologue(psContext, needsParenthesis);
}

static void METALCallTernaryOp(HLSLCrossCompilerContext* psContext, const char* op1, const char* op2, Instruction* psInst,
    int dest, int src0, int src1, int src2, uint32_t dataType)
{
    bstring glsl = *psContext->currentShaderString;
    uint32_t src2SwizCount = GetNumSwizzleElementsMETAL(&psInst->asOperands[src2]);
    uint32_t src1SwizCount = GetNumSwizzleElementsMETAL(&psInst->asOperands[src1]);
    uint32_t src0SwizCount = GetNumSwizzleElementsMETAL(&psInst->asOperands[src0]);
    uint32_t dstSwizCount = GetNumSwizzleElementsMETAL(&psInst->asOperands[dest]);
    uint32_t destMask = GetOperandWriteMaskMETAL(&psInst->asOperands[dest]);

    const SHADER_VARIABLE_TYPE eDestType = GetOperandDataTypeMETAL(psContext, &psInst->asOperands[dest]);
    uint32_t ui32Flags = dataType | SVTTypeToFlagMETAL(eDestType);
    int numParenthesis = 0;

    AddIndentation(psContext);

    METALAddAssignToDest(psContext, &psInst->asOperands[dest], TypeFlagsToSVTTypeMETAL(dataType), dstSwizCount, &numParenthesis);

    TranslateOperandWithMaskMETAL(psContext, &psInst->asOperands[src0], ui32Flags, destMask);
    bformata(glsl, " %s ", op1);
    TranslateOperandWithMaskMETAL(psContext, &psInst->asOperands[src1], ui32Flags, destMask);
    bformata(glsl, " %s ", op2);
    TranslateOperandWithMaskMETAL(psContext, &psInst->asOperands[src2], ui32Flags, destMask);
    METALAddAssignPrologue(psContext, numParenthesis);
}

static void METALCallHelper3(HLSLCrossCompilerContext* psContext, const char* name, Instruction* psInst,
    int dest, int src0, int src1, int src2, int paramsShouldFollowWriteMask)
{
    const SHADER_VARIABLE_TYPE eDestType = GetOperandDataTypeMETAL(psContext, &psInst->asOperands[dest]);
    uint32_t ui32Flags = TO_AUTO_BITCAST_TO_FLOAT | SVTTypeToFlagMETAL(eDestType);
    
    bstring glsl = *psContext->currentShaderString;
    uint32_t destMask = paramsShouldFollowWriteMask ? GetOperandWriteMaskMETAL(&psInst->asOperands[dest]) : OPERAND_4_COMPONENT_MASK_ALL;
    uint32_t src2SwizCount = GetNumSwizzleElementsMETAL(&psInst->asOperands[src2]);
    uint32_t src1SwizCount = GetNumSwizzleElementsMETAL(&psInst->asOperands[src1]);
    uint32_t src0SwizCount = GetNumSwizzleElementsMETAL(&psInst->asOperands[src0]);
    uint32_t dstSwizCount = GetNumSwizzleElementsMETAL(&psInst->asOperands[dest]);
    int numParenthesis = 0;


    AddIndentation(psContext);

    METALAddAssignToDest(psContext, &psInst->asOperands[dest], SVT_FLOAT, dstSwizCount, &numParenthesis);

    bformata(glsl, "%s(", name);
    numParenthesis++;
    TranslateOperandWithMaskMETAL(psContext, &psInst->asOperands[src0], ui32Flags, destMask);
    bcatcstr(glsl, ", ");
    TranslateOperandWithMaskMETAL(psContext, &psInst->asOperands[src1], ui32Flags, destMask);
    bcatcstr(glsl, ", ");
    TranslateOperandWithMaskMETAL(psContext, &psInst->asOperands[src2], ui32Flags, destMask);
    METALAddAssignPrologue(psContext, numParenthesis);
}

static void METALCallHelper2(HLSLCrossCompilerContext* psContext, const char* name, Instruction* psInst,
    int dest, int src0, int src1, int paramsShouldFollowWriteMask)
{
    const SHADER_VARIABLE_TYPE eDestType = GetOperandDataTypeMETAL(psContext, &psInst->asOperands[dest]);
    uint32_t ui32Flags = TO_AUTO_BITCAST_TO_FLOAT | SVTTypeToFlagMETAL(eDestType);
    
    bstring glsl = *psContext->currentShaderString;
    uint32_t destMask = paramsShouldFollowWriteMask ? GetOperandWriteMaskMETAL(&psInst->asOperands[dest]) : OPERAND_4_COMPONENT_MASK_ALL;
    uint32_t src1SwizCount = GetNumSwizzleElementsMETAL(&psInst->asOperands[src1]);
    uint32_t src0SwizCount = GetNumSwizzleElementsMETAL(&psInst->asOperands[src0]);
    uint32_t dstSwizCount = GetNumSwizzleElementsMETAL(&psInst->asOperands[dest]);

    int isDotProduct = (strncmp(name, "dot", 3) == 0) ? 1 : 0;
    int numParenthesis = 0;

    AddIndentation(psContext);
    METALAddAssignToDest(psContext, &psInst->asOperands[dest], SVT_FLOAT, isDotProduct ? 1 : dstSwizCount, &numParenthesis);

    bformata(glsl, "%s(", name);
    numParenthesis++;

    TranslateOperandWithMaskMETAL(psContext, &psInst->asOperands[src0], ui32Flags, destMask);
    bcatcstr(glsl, ", ");
    TranslateOperandWithMaskMETAL(psContext, &psInst->asOperands[src1], ui32Flags, destMask);

    METALAddAssignPrologue(psContext, numParenthesis);
}

static void METALCallHelper2Int(HLSLCrossCompilerContext* psContext, const char* name, Instruction* psInst,
    int dest, int src0, int src1, int paramsShouldFollowWriteMask)
{
    uint32_t ui32Flags = TO_AUTO_BITCAST_TO_INT;
    bstring glsl = *psContext->currentShaderString;
    uint32_t src1SwizCount = GetNumSwizzleElementsMETAL(&psInst->asOperands[src1]);
    uint32_t src0SwizCount = GetNumSwizzleElementsMETAL(&psInst->asOperands[src0]);
    uint32_t dstSwizCount = GetNumSwizzleElementsMETAL(&psInst->asOperands[dest]);
    uint32_t destMask = paramsShouldFollowWriteMask ? GetOperandWriteMaskMETAL(&psInst->asOperands[dest]) : OPERAND_4_COMPONENT_MASK_ALL;
    int numParenthesis = 0;

    AddIndentation(psContext);

    METALAddAssignToDest(psContext, &psInst->asOperands[dest], SVT_INT, dstSwizCount, &numParenthesis);

    bformata(glsl, "%s(", name);
    numParenthesis++;
    TranslateOperandWithMaskMETAL(psContext, &psInst->asOperands[src0], ui32Flags, destMask);
    bcatcstr(glsl, ", ");
    TranslateOperandWithMaskMETAL(psContext, &psInst->asOperands[src1], ui32Flags, destMask);
    METALAddAssignPrologue(psContext, numParenthesis);
}

static void METALCallHelper2UInt(HLSLCrossCompilerContext* psContext, const char* name, Instruction* psInst,
    int dest, int src0, int src1, int paramsShouldFollowWriteMask)
{
    uint32_t ui32Flags = TO_AUTO_BITCAST_TO_UINT;
    bstring glsl = *psContext->currentShaderString;
    uint32_t src1SwizCount = GetNumSwizzleElementsMETAL(&psInst->asOperands[src1]);
    uint32_t src0SwizCount = GetNumSwizzleElementsMETAL(&psInst->asOperands[src0]);
    uint32_t dstSwizCount = GetNumSwizzleElementsMETAL(&psInst->asOperands[dest]);
    uint32_t destMask = paramsShouldFollowWriteMask ? GetOperandWriteMaskMETAL(&psInst->asOperands[dest]) : OPERAND_4_COMPONENT_MASK_ALL;
    int numParenthesis = 0;

    AddIndentation(psContext);

    METALAddAssignToDest(psContext, &psInst->asOperands[dest], SVT_UINT, dstSwizCount, &numParenthesis);

    bformata(glsl, "%s(", name);
    numParenthesis++;
    TranslateOperandWithMaskMETAL(psContext, &psInst->asOperands[src0], ui32Flags, destMask);
    bcatcstr(glsl, ", ");
    TranslateOperandWithMaskMETAL(psContext, &psInst->asOperands[src1], ui32Flags, destMask);
    METALAddAssignPrologue(psContext, numParenthesis);
}

static void METALCallHelper1(HLSLCrossCompilerContext* psContext, const char* name, Instruction* psInst,
    int dest, int src0, int paramsShouldFollowWriteMask)
{
    uint32_t ui32Flags = TO_AUTO_BITCAST_TO_FLOAT;
    bstring glsl = *psContext->currentShaderString;
    uint32_t src0SwizCount = GetNumSwizzleElementsMETAL(&psInst->asOperands[src0]);
    uint32_t dstSwizCount = GetNumSwizzleElementsMETAL(&psInst->asOperands[dest]);
    uint32_t destMask = paramsShouldFollowWriteMask ? GetOperandWriteMaskMETAL(&psInst->asOperands[dest]) : OPERAND_4_COMPONENT_MASK_ALL;
    int numParenthesis = 0;

    AddIndentation(psContext);

    METALAddAssignToDest(psContext, &psInst->asOperands[dest], SVT_FLOAT, dstSwizCount, &numParenthesis);

    bformata(glsl, "%s(", name);
    numParenthesis++;
    TranslateOperandWithMaskMETAL(psContext, &psInst->asOperands[src0], ui32Flags, destMask);
    METALAddAssignPrologue(psContext, numParenthesis);
}

////Result is an int.
//static void METALCallHelper1Int(HLSLCrossCompilerContext* psContext,
//  const char* name,
//  Instruction* psInst,
//  const int dest,
//  const int src0,
//  int paramsShouldFollowWriteMask)
//{
//  uint32_t ui32Flags = TO_AUTO_BITCAST_TO_INT;
//  bstring glsl = *psContext->currentShaderString;
//  uint32_t src0SwizCount = GetNumSwizzleElements(&psInst->asOperands[src0]);
//  uint32_t dstSwizCount = GetNumSwizzleElements(&psInst->asOperands[dest]);
//  uint32_t destMask = paramsShouldFollowWriteMask ? GetOperandWriteMask(&psInst->asOperands[dest]) : OPERAND_4_COMPONENT_MASK_ALL;
//  int numParenthesis = 0;
//
//  AddIndentation(psContext);
//
//  METALAddAssignToDest(psContext, &psInst->asOperands[dest], SVT_INT, dstSwizCount, &numParenthesis);
//
//  bformata(glsl, "%s(", name);
//  numParenthesis++;
//  TranslateOperandWithMask(psContext, &psInst->asOperands[src0], ui32Flags, destMask);
//  METALAddAssignPrologue(psContext, numParenthesis);
//}

static void METALTranslateTexelFetch(HLSLCrossCompilerContext* psContext,
    Instruction* psInst,
    ResourceBinding* psBinding,
    bstring metal)
{
    int numParenthesis = 0;
    uint32_t destCount = GetNumSwizzleElementsMETAL(&psInst->asOperands[0]);
    AddIndentation(psContext);
    METALAddAssignToDest(psContext, &psInst->asOperands[0], TypeFlagsToSVTTypeMETAL(METALResourceReturnTypeToFlag(psBinding->ui32ReturnType)), 4, &numParenthesis);

    switch (psBinding->eDimension)
    {
    case REFLECT_RESOURCE_DIMENSION_TEXTURE1D:
    {
        bcatcstr(metal, "(");
        TranslateOperandMETAL(psContext, &psInst->asOperands[2], TO_FLAG_NONE);
        bcatcstr(metal, ".read(");
        bcatcstr(metal, "(");
        TranslateOperandMETAL(psContext, &psInst->asOperands[1], TO_FLAG_UNSIGNED_INTEGER);
        bcatcstr(metal, ").x)");
        TranslateOperandSwizzleMETAL(psContext, &psInst->asOperands[2]);
        bcatcstr(metal, ")");

        TranslateOperandSwizzleMETAL(psContext, &psInst->asOperands[0]);

        break;
    }
    case REFLECT_RESOURCE_DIMENSION_TEXTURE1DARRAY:
    {
        bcatcstr(metal, "(");
        TranslateOperandMETAL(psContext, &psInst->asOperands[2], TO_FLAG_NONE);
        bcatcstr(metal, ".read(");
        bcatcstr(metal, "(");
        TranslateOperandMETAL(psContext, &psInst->asOperands[1], TO_FLAG_UNSIGNED_INTEGER);
        bcatcstr(metal, ").x, (");
        TranslateOperandMETAL(psContext, &psInst->asOperands[1], TO_FLAG_UNSIGNED_INTEGER);
        bcatcstr(metal, ").y)");
        TranslateOperandSwizzleMETAL(psContext, &psInst->asOperands[2]);
        bcatcstr(metal, ")");

        TranslateOperandSwizzleMETAL(psContext, &psInst->asOperands[0]);

        break;
    }
    case REFLECT_RESOURCE_DIMENSION_TEXTURE2D:
    {
        // METAL PIXEL SHADER RT FETCH
        if (psInst->asOperands[2].ui32RegisterNumber >= GMEM_FLOAT_START_SLOT)
        {
            bformata(metal, "(GMEM_Input%d", GetGmemInputResourceSlotMETAL(psInst->asOperands[2].ui32RegisterNumber));

            int gmemNumElements = GetGmemInputResourceNumElementsMETAL(psInst->asOperands[2].ui32RegisterNumber);

            int destNumElements = 0;

            if (psInst->asOperands[0].iNumComponents != 1)
            {
                //Component Mask
                uint32_t mask = psInst->asOperands[0].ui32CompMask;

                if (mask == OPERAND_4_COMPONENT_MASK_ALL)
                {
                    destNumElements = 4;
                }
                else if (mask != 0)
                {
                    if (mask & OPERAND_4_COMPONENT_MASK_X)
                    {
                        destNumElements++;
                    }
                    if (mask & OPERAND_4_COMPONENT_MASK_Y)
                    {
                        destNumElements++;
                    }
                    if (mask & OPERAND_4_COMPONENT_MASK_Z)
                    {
                        destNumElements++;
                    }
                    if (mask & OPERAND_4_COMPONENT_MASK_W)
                    {
                        destNumElements++;
                    }
                }
            }
            else
            {
                destNumElements = 4;
            }

            TranslateGmemOperandSwizzleWithMaskMETAL(psContext, &psInst->asOperands[2], OPERAND_4_COMPONENT_MASK_ALL, gmemNumElements);
            bcatcstr(metal, ")");

            TranslateOperandSwizzleMETAL(psContext, &psInst->asOperands[0]);
        }
        else
        {
            bcatcstr(metal, "(");
            TranslateOperandMETAL(psContext, &psInst->asOperands[2], TO_FLAG_NONE);
            bcatcstr(metal, ".read(");
            bcatcstr(metal, "(");
            TranslateOperandMETAL(psContext, &psInst->asOperands[1], TO_FLAG_UNSIGNED_INTEGER);
            bcatcstr(metal, ").xy, (");
            TranslateOperandMETAL(psContext, &psInst->asOperands[1], TO_FLAG_UNSIGNED_INTEGER);
            bcatcstr(metal, ").w)");
            TranslateOperandSwizzleMETAL(psContext, &psInst->asOperands[2]);
            bcatcstr(metal, ")");
            TranslateOperandSwizzleMETAL(psContext, &psInst->asOperands[0]);
        }

        break;
    }
    case REFLECT_RESOURCE_DIMENSION_TEXTURE2DARRAY:
    {
        bcatcstr(metal, "(");
        TranslateOperandMETAL(psContext, &psInst->asOperands[2], TO_FLAG_NONE);
        bcatcstr(metal, ".read(");
        bcatcstr(metal, "(");
        TranslateOperandMETAL(psContext, &psInst->asOperands[1], TO_FLAG_UNSIGNED_INTEGER);
        bcatcstr(metal, ").xy, (");
        TranslateOperandMETAL(psContext, &psInst->asOperands[1], TO_FLAG_UNSIGNED_INTEGER);
        bcatcstr(metal, ").z, (");
        TranslateOperandMETAL(psContext, &psInst->asOperands[1], TO_FLAG_UNSIGNED_INTEGER);
        bcatcstr(metal, ").w)");
        TranslateOperandSwizzleMETAL(psContext, &psInst->asOperands[2]);
        bcatcstr(metal, ")");
        TranslateOperandSwizzleMETAL(psContext, &psInst->asOperands[0]);

        break;
    }
    case REFLECT_RESOURCE_DIMENSION_TEXTURE3D:
    {
        bcatcstr(metal, "(");
        TranslateOperandMETAL(psContext, &psInst->asOperands[2], TO_FLAG_NONE);
        bcatcstr(metal, ".read(");
        bcatcstr(metal, "(");
        TranslateOperandMETAL(psContext, &psInst->asOperands[1], TO_FLAG_UNSIGNED_INTEGER);
        bcatcstr(metal, ").xyz, (");
        TranslateOperandMETAL(psContext, &psInst->asOperands[1], TO_FLAG_UNSIGNED_INTEGER);
        bcatcstr(metal, ").w)");
        TranslateOperandSwizzleMETAL(psContext, &psInst->asOperands[2]);
        bcatcstr(metal, ")");

        TranslateOperandSwizzleMETAL(psContext, &psInst->asOperands[0]);

        break;
    }
    case REFLECT_RESOURCE_DIMENSION_TEXTURE2DMS:
    {
        ASSERT(psInst->eOpcode == OPCODE_LD_MS);

        TranslateOperandMETAL(psContext, &psInst->asOperands[2], TO_FLAG_NONE);
        bcatcstr(metal, ".read(");

        bcatcstr(metal, "(");
        TranslateOperandMETAL(psContext, &psInst->asOperands[1], TO_FLAG_UNSIGNED_INTEGER);
        bcatcstr(metal, ").xy, ");
        TranslateOperandMETAL(psContext, &psInst->asOperands[3], TO_FLAG_UNSIGNED_INTEGER);
        bcatcstr(metal, ")");
        TranslateOperandSwizzleMETAL(psContext, &psInst->asOperands[2]);
        TranslateOperandSwizzleMETAL(psContext, &psInst->asOperands[0]);

        break;
    }
    case REFLECT_RESOURCE_DIMENSION_BUFFER:
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

    METALAddAssignPrologue(psContext, numParenthesis);
}

//static void METALTranslateTexelFetchOffset(HLSLCrossCompilerContext* psContext,
//  Instruction* psInst,
//  ResourceBinding* psBinding,
//  bstring metal)
//{
//  int numParenthesis = 0;
//  uint32_t destCount = GetNumSwizzleElements(&psInst->asOperands[0]);
//  AddIndentation(psContext);
//  METALAddAssignToDest(psContext, &psInst->asOperands[0], TypeFlagsToSVTType(METALResourceReturnTypeToFlag(psBinding->ui32ReturnType)), 4, &numParenthesis);
//
//  bcatcstr(metal, "texelFetchOffset(");
//
//  switch (psBinding->eDimension)
//  {
//  case REFLECT_RESOURCE_DIMENSION_TEXTURE1D:
//  {
//      TranslateOperand(psContext, &psInst->asOperands[2], TO_FLAG_NONE);
//      bcatcstr(metal, ", ");
//      TranslateOperandWithMask(psContext, &psInst->asOperands[1], TO_FLAG_INTEGER, OPERAND_4_COMPONENT_MASK_X);
//      bformata(metal, ", 0, %d)", psInst->iUAddrOffset);
//      break;
//  }
//  case REFLECT_RESOURCE_DIMENSION_TEXTURE2DARRAY:
//  {
//      TranslateOperand(psContext, &psInst->asOperands[2], TO_FLAG_NONE);
//      bcatcstr(metal, ", ");
//      TranslateOperandWithMask(psContext, &psInst->asOperands[1], TO_FLAG_INTEGER | TO_AUTO_EXPAND_TO_VEC3, 7 /* .xyz */);
//      bformata(metal, ", 0, int2(%d, %d))",
//          psInst->iUAddrOffset,
//          psInst->iVAddrOffset);
//      break;
//  }
//  case REFLECT_RESOURCE_DIMENSION_TEXTURE3D:
//  {
//      TranslateOperand(psContext, &psInst->asOperands[2], TO_FLAG_NONE);
//      bcatcstr(metal, ", ");
//      TranslateOperandWithMask(psContext, &psInst->asOperands[1], TO_FLAG_INTEGER | TO_AUTO_EXPAND_TO_VEC3, 7 /* .xyz */);
//      bformata(metal, ", 0, int3(%d, %d, %d))",
//          psInst->iUAddrOffset,
//          psInst->iVAddrOffset,
//          psInst->iWAddrOffset);
//      break;
//  }
//  case REFLECT_RESOURCE_DIMENSION_TEXTURE2D:
//  {
//      TranslateOperand(psContext, &psInst->asOperands[2], TO_FLAG_NONE);
//      bcatcstr(metal, ", ");
//      TranslateOperandWithMask(psContext, &psInst->asOperands[1], TO_FLAG_INTEGER | TO_AUTO_EXPAND_TO_VEC2, 3 /* .xy */);
//      bformata(metal, ", 0, int2(%d, %d))", psInst->iUAddrOffset, psInst->iVAddrOffset);
//      break;
//  }
//  case REFLECT_RESOURCE_DIMENSION_TEXTURE1DARRAY:
//  {
//      TranslateOperand(psContext, &psInst->asOperands[2], TO_FLAG_NONE);
//      bcatcstr(metal, ", ");
//      TranslateOperandWithMask(psContext, &psInst->asOperands[1], TO_FLAG_INTEGER | TO_AUTO_EXPAND_TO_VEC2, 3 /* .xy */);
//      bformata(metal, ", 0, int(%d))", psInst->iUAddrOffset);
//      break;
//  }
//  case REFLECT_RESOURCE_DIMENSION_BUFFER:
//  case REFLECT_RESOURCE_DIMENSION_TEXTURE2DMS:
//  case REFLECT_RESOURCE_DIMENSION_TEXTURE2DMSARRAY:
//  case REFLECT_RESOURCE_DIMENSION_TEXTURECUBE:
//  case REFLECT_RESOURCE_DIMENSION_TEXTURECUBEARRAY:
//  case REFLECT_RESOURCE_DIMENSION_BUFFEREX:
//  default:
//  {
//      ASSERT(0);
//      break;
//  }
//  }
//
//  AddSwizzleUsingElementCount(psContext, destCount);
//  METALAddAssignPrologue(psContext, numParenthesis);
//}


//Makes sure the texture coordinate swizzle is appropriate for the texture type.
//i.e. vecX for X-dimension texture.
//Currently supports floating point coord only, so not used for texelFetch.
static void METALTranslateTexCoord(HLSLCrossCompilerContext* psContext,
    const RESOURCE_DIMENSION eResDim,
    Operand* psTexCoordOperand)
{
    int numParenthesis = 0;
    uint32_t flags = TO_AUTO_BITCAST_TO_FLOAT;
    bstring glsl = *psContext->currentShaderString;
    uint32_t opMask = OPERAND_4_COMPONENT_MASK_ALL;
    int isArray = 0;
    switch (eResDim)
    {
    case RESOURCE_DIMENSION_TEXTURE1D:
    {
        //Vec1 texcoord. Mask out the other components.
        opMask = OPERAND_4_COMPONENT_MASK_X;
        break;
    }
    case RESOURCE_DIMENSION_TEXTURE2D:
    case RESOURCE_DIMENSION_TEXTURE1DARRAY:
    {
        //Vec2 texcoord. Mask out the other components.
        opMask = OPERAND_4_COMPONENT_MASK_X | OPERAND_4_COMPONENT_MASK_Y;
        flags |= TO_AUTO_EXPAND_TO_VEC2;
        break;
    }
    case RESOURCE_DIMENSION_TEXTURECUBE:
    case RESOURCE_DIMENSION_TEXTURE3D:
    {
        //Vec3 texcoord. Mask out the other components.
        opMask = OPERAND_4_COMPONENT_MASK_X | OPERAND_4_COMPONENT_MASK_Y | OPERAND_4_COMPONENT_MASK_Z;
        flags |= TO_AUTO_EXPAND_TO_VEC3;
        break;
    }
    case RESOURCE_DIMENSION_TEXTURE2DARRAY:
    {
        //Vec3 texcoord. Mask out the other components.
        opMask = OPERAND_4_COMPONENT_MASK_X | OPERAND_4_COMPONENT_MASK_Y;
        flags |= TO_AUTO_EXPAND_TO_VEC2;
        isArray = 1;
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

    //FIXME detect when integer coords are needed.
    TranslateOperandWithMaskMETAL(psContext, psTexCoordOperand, flags, opMask);
    if (isArray)
    {
        bformata(glsl, ",");
        TranslateOperandWithMaskMETAL(psContext, psTexCoordOperand, 0, OPERAND_4_COMPONENT_MASK_Z);
    }
}

static int METALGetNumTextureDimensions(HLSLCrossCompilerContext* psContext,
    const RESOURCE_DIMENSION eResDim)
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

void GetResInfoDataMETAL(HLSLCrossCompilerContext* psContext, Instruction* psInst, int index, int destElem)
{
    bstring metal = *psContext->currentShaderString;
    int numParenthesis = 0;
    const RESINFO_RETURN_TYPE eResInfoReturnType = psInst->eResInfoReturnType;
    const RESOURCE_DIMENSION eResDim = psContext->psShader->aeResourceDims[psInst->asOperands[2].ui32RegisterNumber];

    AddIndentation(psContext);
    METALAddOpAssignToDestWithMask(psContext, &psInst->asOperands[0], eResInfoReturnType == RESINFO_INSTRUCTION_RETURN_UINT ? SVT_UINT : SVT_FLOAT, 1, "=", &numParenthesis, 1 << destElem);

    //[width, height, depth or array size, total-mip-count]
    if (index < 3)
    {
        int dim = METALGetNumTextureDimensions(psContext, eResDim);
        bcatcstr(metal, "(");
        if (dim < (index + 1))
        {
            bcatcstr(metal, eResInfoReturnType == RESINFO_INSTRUCTION_RETURN_UINT ? "0u" : "0.0");
        }
        else
        {
            if (eResInfoReturnType == RESINFO_INSTRUCTION_RETURN_UINT)
            {
                bformata(metal, "uint%d(textureSize(", dim);
            }
            else if (eResInfoReturnType == RESINFO_INSTRUCTION_RETURN_RCPFLOAT)
            {
                bformata(metal, "float%d(1.0) / float%d(textureSize(", dim, dim);
            }
            else
            {
                bformata(metal, "float%d(textureSize(", dim);
            }
            TranslateOperandMETAL(psContext, &psInst->asOperands[2], TO_FLAG_NONE);
            bcatcstr(metal, ", ");
            TranslateOperandMETAL(psContext, &psInst->asOperands[1], TO_FLAG_INTEGER);
            bcatcstr(metal, "))");

            switch (index)
            {
            case 0:
                bcatcstr(metal, ".x");
                break;
            case 1:
                bcatcstr(metal, ".y");
                break;
            case 2:
                bcatcstr(metal, ".z");
                break;
            }
        }

        bcatcstr(metal, ")");
    }
    else
    {
        if (eResInfoReturnType == RESINFO_INSTRUCTION_RETURN_UINT)
        {
            bcatcstr(metal, "uint(");
        }
        else
        {
            bcatcstr(metal, "float(");
        }
        bcatcstr(metal, "textureQueryLevels(");
        TranslateOperandMETAL(psContext, &psInst->asOperands[2], TO_FLAG_NONE);
        bcatcstr(metal, "))");
    }
    METALAddAssignPrologue(psContext, numParenthesis);
}

#define TEXSMP_FLAG_NONE 0x0
#define TEXSMP_FLAG_LOD 0x1 //LOD comes from operand
#define TEXSMP_FLAG_DEPTHCOMPARE 0x2
#define TEXSMP_FLAG_FIRSTLOD 0x4 //LOD is 0
#define TEXSMP_FLAG_BIAS 0x8
#define TEXSMP_FLAGS_GRAD 0x10

// TODO FIXME: non-float samplers!
static void METALTranslateTextureSample(HLSLCrossCompilerContext* psContext, Instruction* psInst,
    uint32_t ui32Flags)
{
    bstring metal = *psContext->currentShaderString;
    int numParenthesis = 0;

    const char* funcName = "sample";
    const char* offset = "";
    const char* depthCmpCoordType = "";
    const char* gradSwizzle = "";

    uint32_t ui32NumOffsets = 0;

    const RESOURCE_DIMENSION eResDim = psContext->psShader->aeResourceDims[psInst->asOperands[2].ui32RegisterNumber];

    ASSERT(psInst->asOperands[2].ui32RegisterNumber < MAX_TEXTURES);
    switch (eResDim)
    {
    case RESOURCE_DIMENSION_TEXTURE1D:
    {
        gradSwizzle = ".x";
        ui32NumOffsets = 1;
        break;
    }
    case RESOURCE_DIMENSION_TEXTURE2D:
    {
        depthCmpCoordType = "float2";
        gradSwizzle = ".xy";
        ui32NumOffsets = 2;
        break;
    }
    case RESOURCE_DIMENSION_TEXTURECUBE:
    {
        depthCmpCoordType = "float3";
        gradSwizzle = ".xyz";
        ui32NumOffsets = 3;
        break;
    }
    case RESOURCE_DIMENSION_TEXTURE3D:
    {
        gradSwizzle = ".xyz";
        ui32NumOffsets = 3;
        break;
    }
    case RESOURCE_DIMENSION_TEXTURE1DARRAY:
    {
        gradSwizzle = ".x";
        ui32NumOffsets = 1;
        break;
    }
    case RESOURCE_DIMENSION_TEXTURE2DARRAY:
    {
        depthCmpCoordType = "float2";
        gradSwizzle = ".xy";
        ui32NumOffsets = 2;
        break;
    }
    case RESOURCE_DIMENSION_TEXTURECUBEARRAY:
    {
        //bformata(metal, "TODO:Sample from texture cube array LOD\n");
        gradSwizzle = ".xyz";
        ui32NumOffsets = 3;
        //ASSERT(0);
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
        //For non-cubeMap Arrays the reference value comes from the
        //texture coord vector in GLSL. For cubmap arrays there is a
        //separate parameter.
        //It is always separate paramter in HLSL.
        SHADER_VARIABLE_TYPE dataType = SVT_FLOAT; // TODO!!
        AddIndentation(psContext);
        METALAddAssignToDest(psContext, &psInst->asOperands[0], dataType, GetNumSwizzleElementsMETAL(&psInst->asOperands[2]), &numParenthesis);

        bcatcstr(metal, "(float4(");
        ResourceNameMETAL(metal, psContext, RGROUP_TEXTURE, psInst->asOperands[2].ui32RegisterNumber, 0);
        bformata(metal, ".%s_compare(", funcName);
        bconcat(metal, TextureSamplerNameMETAL(&psContext->psShader->sInfo, psInst->asOperands[2].ui32RegisterNumber, psInst->asOperands[3].ui32RegisterNumber, 1));
        bformata(metal, ", %s(", depthCmpCoordType);
        METALTranslateTexCoord(psContext, eResDim, &psInst->asOperands[1]);
        bcatcstr(metal, "), ");
        //.z = reference.
        TranslateOperandMETAL(psContext, &psInst->asOperands[4], TO_AUTO_BITCAST_TO_FLOAT);

        if (ui32Flags & TEXSMP_FLAG_FIRSTLOD)
        {
            bcatcstr(metal, ", level(0)");
        }

        if (psInst->bAddressOffset)
        {
            if (ui32NumOffsets == 2)
            {
                bformata(metal, ", int2(%d, %d)",
                    psInst->iUAddrOffset,
                    psInst->iVAddrOffset);
            }
            else
            if (ui32NumOffsets == 3)
            {
                bformata(metal, ", int3(%d, %d, %d)",
                    psInst->iUAddrOffset,
                    psInst->iVAddrOffset,
                    psInst->iWAddrOffset);
            }
        }
        bcatcstr(metal, ")))");

        psInst->asOperands[2].iWriteMaskEnabled = 1;
        TranslateOperandSwizzleWithMaskMETAL(psContext, &psInst->asOperands[2], GetOperandWriteMaskMETAL(&psInst->asOperands[0]));
    }
    else
    {
        SHADER_VARIABLE_TYPE dataType = SVT_FLOAT; // TODO!!
        AddIndentation(psContext);
        METALAddAssignToDest(psContext, &psInst->asOperands[0], dataType, GetNumSwizzleElementsMETAL(&psInst->asOperands[2]), &numParenthesis);

        bcatcstr(metal, "(");
        ResourceNameMETAL(metal, psContext, RGROUP_TEXTURE, psInst->asOperands[2].ui32RegisterNumber, 0);
        bformata(metal, ".%s(", funcName);
        bconcat(metal, TextureSamplerNameMETAL(&psContext->psShader->sInfo, psInst->asOperands[2].ui32RegisterNumber, psInst->asOperands[3].ui32RegisterNumber, 0));
        bformata(metal, ", ");
        METALTranslateTexCoord(psContext, eResDim, &psInst->asOperands[1]);

        if (ui32NumOffsets > 1)
        {
            if (ui32Flags & (TEXSMP_FLAG_LOD))
            {
                bcatcstr(metal, ", level(");
                TranslateOperandMETAL(psContext, &psInst->asOperands[4], TO_AUTO_BITCAST_TO_FLOAT);
                bcatcstr(metal, ")");
            }
            else
            if (ui32Flags & TEXSMP_FLAG_FIRSTLOD)
            {
                bcatcstr(metal, ", level(0)");
            }
            else
            if (ui32Flags & (TEXSMP_FLAG_BIAS))
            {
                bcatcstr(metal, ", bias(");
                TranslateOperandMETAL(psContext, &psInst->asOperands[4], TO_AUTO_BITCAST_TO_FLOAT);
                bcatcstr(metal, ")");
            }
            else
            if (ui32Flags & TEXSMP_FLAGS_GRAD)
            {
                if (eResDim == RESOURCE_DIMENSION_TEXTURECUBE)
                {
                    bcatcstr(metal, ", gradientcube(float4(");
                }
                else
                {
                    bformata(metal, ", gradient%dd(float4(", ui32NumOffsets);
                }

                TranslateOperandMETAL(psContext, &psInst->asOperands[4], TO_AUTO_BITCAST_TO_FLOAT);            //dx
                bcatcstr(metal, ")");
                bcatcstr(metal, gradSwizzle);
                bcatcstr(metal, ", float4(");
                TranslateOperandMETAL(psContext, &psInst->asOperands[5], TO_AUTO_BITCAST_TO_FLOAT);            //dy
                bcatcstr(metal, ")");
                bcatcstr(metal, gradSwizzle);
                bcatcstr(metal, ")");
            }
        }

        if (psInst->bAddressOffset)
        {
            if (ui32NumOffsets == 1)
            {
                bformata(metal, ", %d",
                    psInst->iUAddrOffset);
            }
            else
            if (ui32NumOffsets == 2)
            {
                bformata(metal, ", int2(%d, %d)",
                    psInst->iUAddrOffset,
                    psInst->iVAddrOffset);
            }
            else
            if (ui32NumOffsets == 3)
            {
                bformata(metal, ", int3(%d, %d, %d)",
                    psInst->iUAddrOffset,
                    psInst->iVAddrOffset,
                    psInst->iWAddrOffset);
            }
        }

        bcatcstr(metal, "))");
    }

    if (!(ui32Flags & TEXSMP_FLAG_DEPTHCOMPARE))
    {
        // iWriteMaskEnabled is forced off during DecodeOperand because swizzle on sampler uniforms
        // does not make sense. But need to re-enable to correctly swizzle this particular instruction.
        psInst->asOperands[2].iWriteMaskEnabled = 1;
        TranslateOperandSwizzleWithMaskMETAL(psContext, &psInst->asOperands[2], GetOperandWriteMaskMETAL(&psInst->asOperands[0]));
    }
    METALAddAssignPrologue(psContext, numParenthesis);
}

static ShaderVarType* METALLookupStructuredVar(HLSLCrossCompilerContext* psContext,
    Operand* psResource,
    Operand* psByteOffset,
    uint32_t ui32Component)
{
    ConstantBuffer* psCBuf = NULL;
    ShaderVarType* psVarType = NULL;
    uint32_t aui32Swizzle[4] = { OPERAND_4_COMPONENT_X };
    int byteOffset = ((int*)psByteOffset->afImmediates)[0] + 4 * ui32Component;
    int vec4Offset = 0;
    int32_t index = -1;
    int32_t rebase = -1;
    int found;

    ASSERT(psByteOffset->eType == OPERAND_TYPE_IMMEDIATE32);
    //TODO: multi-component stores and vector writes need testing.

    //aui32Swizzle[0] = psInst->asOperands[0].aui32Swizzle[component];
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
        return &psContext->psShader->sGroupSharedVarType[psResource->ui32RegisterNumber];
    }
    default:
        ASSERT(0);
        break;
    }

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
    vec4Offset = byteOffset / 16;

    found = GetShaderVarFromOffset(vec4Offset, aui32Swizzle, psCBuf, &psVarType, &index, &rebase);
    ASSERT(found);

    return psVarType;
}

static ShaderVarType* METALLookupStructuredVarAtomic(HLSLCrossCompilerContext* psContext,
    Operand* psResource,
    Operand* psByteOffset,
    uint32_t ui32Component)
{
    ConstantBuffer* psCBuf = NULL;
    ShaderVarType* psVarType = NULL;
    uint32_t aui32Swizzle[4] = { OPERAND_4_COMPONENT_X };
    int byteOffset = ((int*)psByteOffset->afImmediates)[0] + 4 * ui32Component;
    int vec4Offset = 0;
    int32_t index = -1;
    int32_t rebase = -1;
    int found;

    ASSERT(psByteOffset->eType == OPERAND_TYPE_IMMEDIATE32);
    //TODO: multi-component stores and vector writes need testing.

    //aui32Swizzle[0] = psInst->asOperands[0].aui32Swizzle[component];
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
        return &psContext->psShader->sGroupSharedVarType[psResource->ui32RegisterNumber];
    }
    default:
        ASSERT(0);
        break;
    }

    if (psCBuf->asVars->sType.Class == SVC_STRUCT)
    {
        //recalculate offset based on address.y;
        int offset = *((int*)(&psByteOffset->afImmediates[1]));
        if (offset > 0)
        {
            byteOffset = offset + 4 * ui32Component;
        }
    }

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
    vec4Offset = byteOffset / 16;

    found = GetShaderVarFromOffset(vec4Offset, aui32Swizzle, psCBuf, &psVarType, &index, &rebase);
    ASSERT(found);

    return psVarType;
}

static void METALTranslateShaderStorageStore(HLSLCrossCompilerContext* psContext, Instruction* psInst)
{
    bstring metal = *psContext->currentShaderString;
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
        const char* swizzleString[] = { ".x", ".y", ".z", ".w" };
        ASSERT(psInst->asOperands[0].eSelMode == OPERAND_4_COMPONENT_MASK_MODE);
        if (psInst->asOperands[0].ui32CompMask & (1 << component))
        {
            SHADER_VARIABLE_TYPE eSrcDataType = GetOperandDataTypeMETAL(psContext, psSrc);

            if (structured && psDest->eType != OPERAND_TYPE_THREAD_GROUP_SHARED_MEMORY)
            {
                psVarType = METALLookupStructuredVar(psContext, psDest, psDestByteOff, component);
            }

            AddIndentation(psContext);

            if (!structured && (psDest->eType == OPERAND_TYPE_THREAD_GROUP_SHARED_MEMORY))
            {
                bformata(metal, "atomic_store_explicit( &");
                TranslateOperandMETAL(psContext, psDest, TO_FLAG_DESTINATION | TO_FLAG_NAME_ONLY);
                bformata(metal, "[");
                if (structured) //Dest address and dest byte offset
                {
                    if (psDest->eType == OPERAND_TYPE_THREAD_GROUP_SHARED_MEMORY)
                    {
                        TranslateOperandMETAL(psContext, psDestAddr, TO_FLAG_INTEGER | TO_FLAG_UNSIGNED_INTEGER);
                        bformata(metal, "].value[");
                        TranslateOperandMETAL(psContext, psDestByteOff, TO_FLAG_INTEGER | TO_FLAG_UNSIGNED_INTEGER);
                        bformata(metal, "/4u ");//bytes to floats
                    }
                    else
                    {
                        TranslateOperandMETAL(psContext, psDestAddr, TO_FLAG_INTEGER | TO_FLAG_UNSIGNED_INTEGER);
                    }
                }
                else
                {
                    TranslateOperandMETAL(psContext, psDestByteOff, TO_FLAG_INTEGER | TO_FLAG_UNSIGNED_INTEGER);
                }
                //RAW: change component using index offset
                if (!structured || (psDest->eType == OPERAND_TYPE_THREAD_GROUP_SHARED_MEMORY))
                {
                    bformata(metal, " + %d", component);
                }
                bformata(metal, "],");

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
                        else if (psVarType->Type == SVT_FLOAT16)
                        {
                            flags = TO_FLAG_FLOAT16;
                        } 
                        else
                        {
                            ASSERT(0);
                        }
                    }
                    //TGSM always uint
                    bformata(metal, " (");
                    if (GetNumSwizzleElementsMETAL(psSrc) > 1)
                    {
                        TranslateOperandWithMaskMETAL(psContext, psSrc, flags, 1 << (srcComponent++));
                    }
                    else
                    {
                        TranslateOperandWithMaskMETAL(psContext, psSrc, flags, OPERAND_4_COMPONENT_MASK_X);
                    }
                }
                else
                {
                    //Dest type is currently always a uint array.
                    bformata(metal, " (");
                    if (GetNumSwizzleElementsMETAL(psSrc) > 1)
                    {
                        TranslateOperandWithMaskMETAL(psContext, psSrc, TO_FLAG_UNSIGNED_INTEGER, 1 << (srcComponent++));
                    }
                    else
                    {
                        TranslateOperandWithMaskMETAL(psContext, psSrc, TO_FLAG_UNSIGNED_INTEGER, OPERAND_4_COMPONENT_MASK_X);
                    }
                }

                //Double takes an extra slot.
                if (psVarType && psVarType->Type == SVT_DOUBLE)
                {
                    if (structured && psDest->eType == OPERAND_TYPE_THREAD_GROUP_SHARED_MEMORY)
                    {
                        bcatcstr(metal, ")");
                    }
                    component++;
                }

                bformata(metal, "),");
                bformata(metal, "memory_order_relaxed");
                bformata(metal, ");\n");
                return;
            }

            if (structured && psDest->eType == OPERAND_TYPE_RESOURCE)
            {
                ResourceNameMETAL(metal, psContext, RGROUP_TEXTURE, psDest->ui32RegisterNumber, 0);
            }
            else
            {
                TranslateOperandMETAL(psContext, psDest, TO_FLAG_DESTINATION | TO_FLAG_NAME_ONLY);
            }
            bformata(metal, "[");
            if (structured) //Dest address and dest byte offset
            {
                if (psDest->eType == OPERAND_TYPE_THREAD_GROUP_SHARED_MEMORY)
                {
                    TranslateOperandMETAL(psContext, psDestAddr, TO_FLAG_INTEGER | TO_FLAG_UNSIGNED_INTEGER);
                    bformata(metal, "].value[");
                    TranslateOperandMETAL(psContext, psDestByteOff, TO_FLAG_INTEGER | TO_FLAG_UNSIGNED_INTEGER);
                    bformata(metal, "/4u ");//bytes to floats
                }
                else
                {
                    TranslateOperandMETAL(psContext, psDestAddr, TO_FLAG_INTEGER | TO_FLAG_UNSIGNED_INTEGER);
                }
            }
            else
            {
                TranslateOperandMETAL(psContext, psDestByteOff, TO_FLAG_INTEGER | TO_FLAG_UNSIGNED_INTEGER);
            }

            //RAW: change component using index offset
            if (!structured || (psDest->eType == OPERAND_TYPE_THREAD_GROUP_SHARED_MEMORY))
            {
                bformata(metal, " + %d", component);
            }

            bformata(metal, "]");

            if (structured && psDest->eType != OPERAND_TYPE_THREAD_GROUP_SHARED_MEMORY)
            {
                if (strcmp(psVarType->Name, "$Element") != 0)
                {
                    bformata(metal, ".%s", psVarType->Name);
                }
                if (psVarType->Columns > 1 || psVarType->Rows > 1)
                {
                    bformata(metal, "%s", swizzleString[((((int*)psDestByteOff->afImmediates)[0] + 4 * component - psVarType->Offset) % 16 / 4)]);
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
                    else if (psVarType->Type == SVT_FLOAT16)
                    {
                        flags = TO_FLAG_FLOAT16;
                    }
                    else
                    {
                        ASSERT(0);
                    }
                }
                //TGSM always uint
                bformata(metal, " = (");
                if (GetNumSwizzleElementsMETAL(psSrc) > 1)
                {
                    TranslateOperandWithMaskMETAL(psContext, psSrc, flags, 1 << (srcComponent++));
                }
                else
                {
                    TranslateOperandWithMaskMETAL(psContext, psSrc, flags, OPERAND_4_COMPONENT_MASK_X);
                }
            }
            else
            {
                //Dest type is currently always a uint array.
                bformata(metal, " = (");
                if (GetNumSwizzleElementsMETAL(psSrc) > 1)
                {
                    TranslateOperandWithMaskMETAL(psContext, psSrc, TO_FLAG_UNSIGNED_INTEGER, 1 << (srcComponent++));
                }
                else
                {
                    TranslateOperandWithMaskMETAL(psContext, psSrc, TO_FLAG_UNSIGNED_INTEGER, OPERAND_4_COMPONENT_MASK_X);
                }
            }

            //Double takes an extra slot.
            if (psVarType && psVarType->Type == SVT_DOUBLE)
            {
                if (structured && psDest->eType == OPERAND_TYPE_THREAD_GROUP_SHARED_MEMORY)
                {
                    bcatcstr(metal, ")");
                }
                component++;
            }

            bformata(metal, ");\n");
        }
    }
}

static void METALTranslateShaderStorageLoad(HLSLCrossCompilerContext* psContext, Instruction* psInst)
{
    bstring metal = *psContext->currentShaderString;
    ShaderVarType* psVarType = NULL;
    uint32_t aui32Swizzle[4] = { OPERAND_4_COMPONENT_X };
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
        uint32_t destCount = GetNumSwizzleElementsMETAL(psDest);
        uint32_t destMask = GetOperandWriteMaskMETAL(psDest);
        AddIndentation(psContext);
        METALAddAssignToDest(psContext, psDest, SVT_UINT, destCount, &numParenthesis);
        if (destCount > 1)
        {
            bformata(metal, "%s(", GetConstructorForTypeMETAL(SVT_UINT, destCount));
            numParenthesis++;
        }
        for (component = 0; component < 4; component++)
        {
            if (!(destMask & (1 << component)))
            {
                continue;
            }

            if (firstItemAdded)
            {
                bcatcstr(metal, ", ");
            }
            else
            {
                firstItemAdded = 1;
            }

            if (psSrc->eType == OPERAND_TYPE_THREAD_GROUP_SHARED_MEMORY)
            {
                //ld from threadgroup shared memory
                bformata(metal, "atomic_load_explicit( &");
                bformata(metal, "TGSM%d[((", psSrc->ui32RegisterNumber);
                TranslateOperandMETAL(psContext, psSrcByteOff, TO_FLAG_INTEGER);
                bcatcstr(metal, ") >> 2)");
                if (psSrc->eSelMode == OPERAND_4_COMPONENT_SWIZZLE_MODE && psSrc->aui32Swizzle[component] != 0)
                {
                    bformata(metal, " + %d", psSrc->aui32Swizzle[component]);
                }
                bcatcstr(metal, "]");
                bcatcstr(metal, " , ");
                bcatcstr(metal, "memory_order::memory_order_relaxed");
                bformata(metal, ")");

                /*
                bformata(metal, "TGSM%d[((", psSrc->ui32RegisterNumber);
                TranslateOperandMETAL(psContext, psSrcByteOff, TO_FLAG_INTEGER);
                bcatcstr(metal, ") >> 2)");
                if (psSrc->eSelMode == OPERAND_4_COMPONENT_SWIZZLE_MODE && psSrc->aui32Swizzle[component] != 0)
                {
                    bformata(metal, " + %d", psSrc->aui32Swizzle[component]);
                }
                bcatcstr(metal, "]");
                */
            }
            else
            {
                //ld from raw buffer
                bformata(metal, "RawRes%d[((", psSrc->ui32RegisterNumber);
                TranslateOperandMETAL(psContext, psSrcByteOff, TO_FLAG_INTEGER);
                bcatcstr(metal, ") >> 2)");
                if (psSrc->eSelMode == OPERAND_4_COMPONENT_SWIZZLE_MODE && psSrc->aui32Swizzle[component] != 0)
                {
                    bformata(metal, " + %d", psSrc->aui32Swizzle[component]);
                }
                bcatcstr(metal, "]");
            }
        }
        METALAddAssignPrologue(psContext, numParenthesis);
    }
    else
    {
        int numParenthesis = 0;
        int firstItemAdded = 0;
        uint32_t destCount = GetNumSwizzleElementsMETAL(psDest);
        uint32_t destMask = GetOperandWriteMaskMETAL(psDest);
        ASSERT(psInst->eOpcode == OPCODE_LD_STRUCTURED);
        AddIndentation(psContext);
        METALAddAssignToDest(psContext, psDest, SVT_UINT, destCount, &numParenthesis);
        if (destCount > 1)
        {
            bformata(metal, "%s(", GetConstructorForTypeMETAL(SVT_UINT, destCount));
            numParenthesis++;
        }
        for (component = 0; component < 4; component++)
        {
            ShaderVarType* psVar = NULL;
            int addedBitcast = 0;
            if (!(destMask & (1 << component)))
            {
                continue;
            }

            if (firstItemAdded)
            {
                bcatcstr(metal, ", ");
            }
            else
            {
                firstItemAdded = 1;
            }

            if (psSrc->eType == OPERAND_TYPE_THREAD_GROUP_SHARED_MEMORY)
            {
                // input already in uints
                TranslateOperandMETAL(psContext, psSrc, TO_FLAG_NAME_ONLY);
                bcatcstr(metal, "[");
                TranslateOperandMETAL(psContext, psSrcAddr, TO_FLAG_INTEGER);
                bcatcstr(metal, "].value[(");
                TranslateOperandMETAL(psContext, psSrcByteOff, TO_FLAG_UNSIGNED_INTEGER);
                bformata(metal, " >> 2u) + %d]", psSrc->eSelMode == OPERAND_4_COMPONENT_SWIZZLE_MODE ? psSrc->aui32Swizzle[component] : component);
            }
            else
            {
                ConstantBuffer* psCBuf = NULL;
                psVar = METALLookupStructuredVar(psContext, psSrc, psSrcByteOff, psSrc->eSelMode == OPERAND_4_COMPONENT_SWIZZLE_MODE ? psSrc->aui32Swizzle[component] : component);
                GetConstantBufferFromBindingPoint(RGROUP_UAV, psSrc->ui32RegisterNumber, &psContext->psShader->sInfo, &psCBuf);

                if (psVar->Type == SVT_FLOAT)
                {
                    bcatcstr(metal, "as_type<uint>(");
                    bcatcstr(metal, "(");
                    addedBitcast = 1;
                }
                else if (psVar->Type == SVT_DOUBLE)
                {
                    bcatcstr(metal, "as_type<uint>(");
                    bcatcstr(metal, "(");
                    addedBitcast = 1;
                }
                if (psSrc->eType == OPERAND_TYPE_UNORDERED_ACCESS_VIEW)
                {
                    bformata(metal, "%s[", psCBuf->Name);
                    TranslateOperandMETAL(psContext, psSrcAddr, TO_FLAG_INTEGER);
                    bcatcstr(metal, "]");
                    if (strcmp(psVar->Name, "$Element") != 0)
                    {
                        bcatcstr(metal, ".");
                        bcatcstr(metal, psVar->Name);
                    }

                    int swizcomponent = psSrc->eSelMode == OPERAND_4_COMPONENT_SWIZZLE_MODE ? psSrc->aui32Swizzle[component] : component;
                    int byteOffset = ((int*)psSrcByteOff->afImmediates)[0] + 4 * swizcomponent;
                    int bytes = byteOffset - psVar->Offset;
                    if (psVar->Class != SVC_SCALAR)
                    {
                        static const char* const m_swizzlers[] = { "x", "y", "z", "w" };
                        if (bytes > 16)
                        {
                            int i = 0;
                        }

                        int offset = (bytes % 16) / 4;
                        if (offset == 0)
                        {
                            bcatcstr(metal, ".x");
                        }
                        if (offset == 1)
                        {
                            bcatcstr(metal, ".y");
                        }
                        if (offset == 2)
                        {
                            bcatcstr(metal, ".z");
                        }
                        if (offset == 3)
                        {
                            bcatcstr(metal, ".w");
                        }
                    }
                }
                else
                {
                    ResourceNameMETAL(metal, psContext, RGROUP_TEXTURE, psSrc->ui32RegisterNumber, 0);
                    bcatcstr(metal, "[");
                    TranslateOperandMETAL(psContext, psSrcAddr, TO_FLAG_INTEGER);
                    bcatcstr(metal, "]");
                    if (strcmp(psVar->Name, "$Element") != 0)
                    {
                        bcatcstr(metal, ".");
                        bcatcstr(metal, psVar->Name);
                        int swizcomponent = psSrc->eSelMode == OPERAND_4_COMPONENT_SWIZZLE_MODE ? psSrc->aui32Swizzle[component] : component;
                        int byteOffset = ((int*)psSrcByteOff->afImmediates)[0] + 4 * swizcomponent;
                        int bytes = byteOffset - psVar->Offset;
                        if (psVar->Class == SVC_MATRIX_ROWS)
                        {
                            int offset = bytes / 16;
                            bcatcstr(metal, "[");
                            bformata(metal, "%i", offset);
                            bcatcstr(metal, "]");
                        }
                        if (psVar->Class != SVC_SCALAR)
                        {
                            static const char* const m_swizzlers[] = { "x", "y", "z", "w" };

                            int offset = (bytes % 16) / 4;
                            if (offset == 0)
                            {
                                bcatcstr(metal, ".x");
                            }
                            if (offset == 1)
                            {
                                bcatcstr(metal, ".y");
                            }
                            if (offset == 2)
                            {
                                bcatcstr(metal, ".z");
                            }
                            if (offset == 3)
                            {
                                bcatcstr(metal, ".w");
                            }
                        }
                    }
                    else if (psVar->Columns > 1)
                    {
                        int swizcomponent = psSrc->eSelMode == OPERAND_4_COMPONENT_SWIZZLE_MODE ? psSrc->aui32Swizzle[component] : component;
                        int byteOffset = ((int*)psSrcByteOff->afImmediates)[0] + 4 * swizcomponent;
                        int bytes = byteOffset - psVar->Offset;

                        static const char* const m_swizzlers[] = { "x", "y", "z", "w" };

                        int offset = (bytes % 16) / 4;
                        if (offset == 0)
                        {
                            bcatcstr(metal, ".x");
                        }
                        if (offset == 1)
                        {
                            bcatcstr(metal, ".y");
                        }
                        if (offset == 2)
                        {
                            bcatcstr(metal, ".z");
                        }
                        if (offset == 3)
                        {
                            bcatcstr(metal, ".w");
                        }
                    }
                }

                if (addedBitcast)
                {
                    bcatcstr(metal, "))");
                }

                if (psVar->Columns > 1)
                {
                    int multiplier = 1;

                    if (psVar->Type == SVT_DOUBLE)
                    {
                        multiplier++; // doubles take up 2 slots
                    }
                    //component += psVar->Columns * multiplier;
                }
            }
        }
        METALAddAssignPrologue(psContext, numParenthesis);

        return;
    }
}

void TranslateAtomicMemOpMETAL(HLSLCrossCompilerContext* psContext, Instruction* psInst)
{
    bstring metal = *psContext->currentShaderString;
    int numParenthesis = 0;
    ShaderVarType* psVarType = NULL;
    uint32_t ui32DataTypeFlag = TO_FLAG_UNSIGNED_INTEGER;
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
        bcatcstr(metal, "//IMM_ATOMIC_IADD\n");
#endif
        func = "atomic_fetch_add_explicit";
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
        bcatcstr(metal, "//ATOMIC_IADD\n");
#endif
        func = "atomic_fetch_add_explicit";
        dest = &psInst->asOperands[0];
        destAddr = &psInst->asOperands[1];
        src = &psInst->asOperands[2];
        break;
    }
    case OPCODE_IMM_ATOMIC_AND:
    {
#ifdef _DEBUG
        AddIndentation(psContext);
        bcatcstr(metal, "//IMM_ATOMIC_AND\n");
#endif
        func = "atomic_fetch_and_explicit";
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
        bcatcstr(metal, "//ATOMIC_AND\n");
#endif
        func = "atomic_fetch_and_explicit";
        dest = &psInst->asOperands[0];
        destAddr = &psInst->asOperands[1];
        src = &psInst->asOperands[2];
        break;
    }
    case OPCODE_IMM_ATOMIC_OR:
    {
#ifdef _DEBUG
        AddIndentation(psContext);
        bcatcstr(metal, "//IMM_ATOMIC_OR\n");
#endif
        func = "atomic_fetch_or_explicit";
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
        bcatcstr(metal, "//ATOMIC_OR\n");
#endif
        func = "atomic_fetch_or_explicit";
        dest = &psInst->asOperands[0];
        destAddr = &psInst->asOperands[1];
        src = &psInst->asOperands[2];
        break;
    }
    case OPCODE_IMM_ATOMIC_XOR:
    {
#ifdef _DEBUG
        AddIndentation(psContext);
        bcatcstr(metal, "//IMM_ATOMIC_XOR\n");
#endif
        func = "atomic_fetch_xor_explicit";
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
        bcatcstr(metal, "//ATOMIC_XOR\n");
#endif
        func = "atomic_fetch_xor_explicit";
        dest = &psInst->asOperands[0];
        destAddr = &psInst->asOperands[1];
        src = &psInst->asOperands[2];
        break;
    }

    case OPCODE_IMM_ATOMIC_EXCH:
    {
#ifdef _DEBUG
        AddIndentation(psContext);
        bcatcstr(metal, "//IMM_ATOMIC_EXCH\n");
#endif
        func = "atomic_exchange_explicit";
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
        bcatcstr(metal, "//IMM_ATOMIC_CMP_EXC\n");
#endif
        func = "atomic_compare_exchange_weak_explicit";
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
        bcatcstr(metal, "//ATOMIC_CMP_STORE\n");
#endif
        func = "atomic_compare_exchange_weak_explicit";
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
        bcatcstr(metal, "//IMM_ATOMIC_UMIN\n");
#endif
        func = "atomic_fetch_min_explicit";
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
        bcatcstr(metal, "//ATOMIC_UMIN\n");
#endif
        func = "atomic_fetch_min_explicit";
        dest = &psInst->asOperands[0];
        destAddr = &psInst->asOperands[1];
        src = &psInst->asOperands[2];
        break;
    }
    case OPCODE_IMM_ATOMIC_IMIN:
    {
#ifdef _DEBUG
        AddIndentation(psContext);
        bcatcstr(metal, "//IMM_ATOMIC_IMIN\n");
#endif
        func = "atomic_fetch_min_explicit";
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
        bcatcstr(metal, "//ATOMIC_IMIN\n");
#endif
        func = "atomic_fetch_min_explicit";
        dest = &psInst->asOperands[0];
        destAddr = &psInst->asOperands[1];
        src = &psInst->asOperands[2];
        break;
    }
    case OPCODE_IMM_ATOMIC_UMAX:
    {
#ifdef _DEBUG
        AddIndentation(psContext);
        bcatcstr(metal, "//IMM_ATOMIC_UMAX\n");
#endif
        func = "atomic_fetch_max_explicit";
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
        bcatcstr(metal, "//ATOMIC_UMAX\n");
#endif
        func = "atomic_fetch_max_explicit";
        dest = &psInst->asOperands[0];
        destAddr = &psInst->asOperands[1];
        src = &psInst->asOperands[2];
        break;
    }
    case OPCODE_IMM_ATOMIC_IMAX:
    {
#ifdef _DEBUG
        AddIndentation(psContext);
        bcatcstr(metal, "//IMM_ATOMIC_IMAX\n");
#endif
        func = "atomic_fetch_max_explicit";
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
        bcatcstr(metal, "//ATOMIC_IMAX\n");
#endif
        func = "atomic_fetch_max_explicit";
        dest = &psInst->asOperands[0];
        destAddr = &psInst->asOperands[1];
        src = &psInst->asOperands[2];
        break;
    }
    }

    AddIndentation(psContext);

    if (previousValue)
    {
        //all atomic operation returns uint or int
        METALAddAssignToDest(psContext, previousValue, SVT_UINT, 1, &numParenthesis);
    }

    bcatcstr(metal, func);
    bformata(metal, "( &");
    TranslateOperandMETAL(psContext, dest, TO_FLAG_DESTINATION | TO_FLAG_NAME_ONLY);

    if (dest->eType == OPERAND_TYPE_THREAD_GROUP_SHARED_MEMORY)
    {
        //threadgroup shared mem
        bformata(metal, "[");
        TranslateOperandMETAL(psContext, destAddr, TO_FLAG_INTEGER | TO_FLAG_UNSIGNED_INTEGER);
        bformata(metal, "]");
    }
    else
    {
        ResourceBinding* psRes;
        int foundResource = GetResourceFromBindingPoint(RGROUP_UAV,
                dest->ui32RegisterNumber,
                &psContext->psShader->sInfo,
                &psRes);

        ASSERT(foundResource);

        if (psRes->eBindArea == UAVAREA_CBUFFER)
        {
            //rwbuffer
            if (psRes->eType == RTYPE_UAV_RWTYPED)
            {
                bformata(metal, "[");
                TranslateOperandMETAL(psContext, destAddr, TO_FLAG_INTEGER | TO_FLAG_UNSIGNED_INTEGER);
                bformata(metal, "]");
            }
            //rwstructured buffer
            else if (psRes->eType == RTYPE_UAV_RWSTRUCTURED)
            {
                if (destAddr->eType == OPERAND_TYPE_IMMEDIATE32)
                {
                    psVarType = METALLookupStructuredVarAtomic(psContext, dest, destAddr, 0);
                }
                if (psVarType->Type == SVT_UINT)
                {
                    ui32DataTypeFlag = TO_FLAG_UNSIGNED_INTEGER | TO_AUTO_BITCAST_TO_UINT;
                }
                else
                {
                    ui32DataTypeFlag = TO_FLAG_INTEGER | TO_AUTO_BITCAST_TO_INT;
                }
                bformata(metal, "[");
                bformata(metal, "%i", *((int*)(&destAddr->afImmediates[0])));
                bformata(metal, "]");
                if (strcmp(psVarType->Name, "$Element") != 0)
                {
                    bformata(metal, ".%s", psVarType->Name);
                }
            }
        }
        else if (psRes->eBindArea == UAVAREA_TEXTURE)
        {
            //Atomic operation on texture uav not supported
            ASSERT(0);
        }
        else
        {
            //UAV is not exist in either [[buffer]] or [[texture]]
            ASSERT(0);
        }
    }
    //ResourceNameMETAL(metal, psContext, RGROUP_UAV, dest->ui32RegisterNumber, 0);

    bcatcstr(metal, ", ");

    if (compare)
    {
        bcatcstr(metal, "& ");
        TranslateOperandMETAL(psContext, compare, ui32DataTypeFlag);
        bcatcstr(metal, ", ");
    }

    TranslateOperandMETAL(psContext, src, ui32DataTypeFlag);
    bcatcstr(metal, ", ");
    if (compare)
    {
        bcatcstr(metal, "memory_order_relaxed ");
        bcatcstr(metal, ",");
    }
    bcatcstr(metal, "memory_order_relaxed ");
    bcatcstr(metal, ")");
    if (previousValue)
    {
        METALAddAssignPrologue(psContext, numParenthesis);
    }
    else
    {
        bcatcstr(metal, ";\n");
    }
}

static void METALTranslateConditional(HLSLCrossCompilerContext* psContext,
    Instruction* psInst,
    bstring glsl)
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

    if (psInst->eBooleanTestType == INSTRUCTION_TEST_ZERO)
    {
        bcatcstr(glsl, "if((");
        TranslateOperandMETAL(psContext, &psInst->asOperands[0], TO_FLAG_UNSIGNED_INTEGER);

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
        TranslateOperandMETAL(psContext, &psInst->asOperands[0], TO_FLAG_UNSIGNED_INTEGER);

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

// Returns the "more important" type of a and b, currently int < uint < float
static SHADER_VARIABLE_TYPE METALSelectHigherType(SHADER_VARIABLE_TYPE a, SHADER_VARIABLE_TYPE b)
{
    if (a == SVT_FLOAT || b == SVT_FLOAT)
    {
        return SVT_FLOAT;
    }

    if (a == SVT_FLOAT16 || b == SVT_FLOAT16)
    {
        return SVT_FLOAT16;
    }
    // Apart from floats, the enum values are fairly well-ordered, use that directly.
    return a > b ? a : b;
}

// Helper function to set the vector type of 1 or more components in a vector
// If the existing values (that we're writing to) are all SVT_VOID, just upgrade the value and we're done
// Otherwise, set all the components in the vector that currently are set to that same value OR are now being written to
// to the "highest" type value (ordering int->uint->float)
static void METALSetVectorType(SHADER_VARIABLE_TYPE* aeTempVecType, uint32_t regBaseIndex, uint32_t componentMask, SHADER_VARIABLE_TYPE eType)
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
                eType = METALSelectHigherType(eType, aeTempVecType[regBaseIndex + i]);
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

static void METALMarkOperandAs(Operand* psOperand, SHADER_VARIABLE_TYPE eType, SHADER_VARIABLE_TYPE* aeTempVecType)
{
    if (psOperand->eType == OPERAND_TYPE_INDEXABLE_TEMP || psOperand->eType == OPERAND_TYPE_TEMP)
    {
        const uint32_t ui32RegIndex = psOperand->ui32RegisterNumber * 4;

        if (psOperand->eSelMode == OPERAND_4_COMPONENT_SELECT_1_MODE)
        {
            METALSetVectorType(aeTempVecType, ui32RegIndex, 1 << psOperand->aui32Swizzle[0], eType);
        }
        else if (psOperand->eSelMode == OPERAND_4_COMPONENT_SWIZZLE_MODE)
        {
            // 0xf == all components, swizzle order doesn't matter.
            METALSetVectorType(aeTempVecType, ui32RegIndex, 0xf, eType);
        }
        else if (psOperand->eSelMode == OPERAND_4_COMPONENT_MASK_MODE)
        {
            uint32_t ui32CompMask = psOperand->ui32CompMask;
            if (!psOperand->ui32CompMask)
            {
                ui32CompMask = OPERAND_4_COMPONENT_MASK_ALL;
            }

            METALSetVectorType(aeTempVecType, ui32RegIndex, ui32CompMask, eType);
        }
    }
}

static void METALMarkAllOperandsAs(Instruction* psInst, SHADER_VARIABLE_TYPE eType, SHADER_VARIABLE_TYPE* aeTempVecType)
{
    uint32_t i = 0;
    for (i = 0; i < psInst->ui32NumOperands; i++)
    {
        METALMarkOperandAs(&psInst->asOperands[i], eType, aeTempVecType);
    }
}

static void METALWriteOperandTypes(Operand* psOperand, const SHADER_VARIABLE_TYPE* aeTempVecType)
{
    const uint32_t ui32RegIndex = psOperand->ui32RegisterNumber * 4;

    if (psOperand->eType != OPERAND_TYPE_TEMP)
    {
        return;
    }

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
static void METALSetCBOperandComponents(HLSLCrossCompilerContext* psContext, Operand* psOperand)
{
    ConstantBuffer* psCBuf = NULL;
    ShaderVarType* psVarType = NULL;
    int32_t index = -1;
    int rebase = 0;

    if (psOperand->eType != OPERAND_TYPE_CONSTANT_BUFFER)
    {
        return;
    }

    GetConstantBufferFromBindingPoint(RGROUP_CBUFFER, psOperand->aui32ArraySizes[0], &psContext->psShader->sInfo, &psCBuf);
    GetShaderVarFromOffset(psOperand->aui32ArraySizes[1], psOperand->aui32Swizzle, psCBuf, &psVarType, &index, &rebase);

    if (psVarType->Class == SVC_SCALAR)
    {
        psOperand->iNumComponents = 1;
    }
}


void SetDataTypesMETAL(HLSLCrossCompilerContext* psContext, Instruction* psInst, const int32_t i32InstCount)
{
    int32_t i;
    Instruction* psFirstInst = psInst;

    SHADER_VARIABLE_TYPE aeTempVecType[MAX_TEMP_VEC4 * 4];

    // Start with void, then move up the chain void->int->uint->float
    for (i = 0; i < MAX_TEMP_VEC4 * 4; ++i)
    {
        aeTempVecType[i] = SVT_VOID;
    }

    {
        // First pass, do analysis: deduce the data type based on opcodes, fill out aeTempVecType table
        // Only ever to int->float promotion (or int->uint), never the other way around
        for (i = 0; i < i32InstCount; ++i, psInst++)
        {
            int k = 0;
            if (psInst->ui32NumOperands == 0)
            {
                continue;
            }

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

                METALMarkAllOperandsAs(psInst, SVT_FLOAT, aeTempVecType);
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
                METALMarkAllOperandsAs(psInst, SVT_INT, aeTempVecType);
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
                METALMarkAllOperandsAs(psInst, SVT_UINT, aeTempVecType);
                break;

            // Need special handling
            case OPCODE_FTOI:
            case OPCODE_FTOU:
                METALMarkOperandAs(&psInst->asOperands[0], psInst->eOpcode == OPCODE_FTOI ? SVT_INT : SVT_UINT, aeTempVecType);
                METALMarkOperandAs(&psInst->asOperands[1], SVT_FLOAT, aeTempVecType);
                break;

            case OPCODE_GE:
            case OPCODE_LT:
                METALMarkOperandAs(&psInst->asOperands[0], SVT_UINT, aeTempVecType);
                METALMarkOperandAs(&psInst->asOperands[1], SVT_FLOAT, aeTempVecType);
                METALMarkOperandAs(&psInst->asOperands[2], SVT_FLOAT, aeTempVecType);
                break;

            case OPCODE_ITOF:
            case OPCODE_UTOF:
                METALMarkOperandAs(&psInst->asOperands[0], SVT_FLOAT, aeTempVecType);
                METALMarkOperandAs(&psInst->asOperands[1], psInst->eOpcode == OPCODE_ITOF ? SVT_INT : SVT_UINT, aeTempVecType);
                break;

            case OPCODE_LD:
            case OPCODE_LD_MS:
                // TODO: Would need to know the sampler return type
                METALMarkOperandAs(&psInst->asOperands[0], SVT_FLOAT, aeTempVecType);
                break;


            case OPCODE_RESINFO:
            {
                if (psInst->eResInfoReturnType != RESINFO_INSTRUCTION_RETURN_UINT)
                {
                    METALMarkAllOperandsAs(psInst, SVT_FLOAT, aeTempVecType);
                }
                break;
            }

            case OPCODE_SAMPLE_INFO:
                // TODO decode the _uint flag
                METALMarkOperandAs(&psInst->asOperands[0], SVT_FLOAT, aeTempVecType);
                break;

            case OPCODE_SAMPLE_POS:
                METALMarkOperandAs(&psInst->asOperands[0], SVT_FLOAT, aeTempVecType);
                break;


            case OPCODE_LD_UAV_TYPED:
            case OPCODE_STORE_UAV_TYPED:
            case OPCODE_LD_RAW:
            case OPCODE_STORE_RAW:
            case OPCODE_LD_STRUCTURED:
            case OPCODE_STORE_STRUCTURED:
            {
                METALMarkOperandAs(&psInst->asOperands[0], SVT_INT, aeTempVecType);
                break;
            }
            case OPCODE_F32TOF16:
            case OPCODE_F16TOF32:
                // TODO
                break;



            // No-operands, should never get here anyway
            /*              case OPCODE_BREAK:
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
        {
            aeTempVecType[i] = SVT_INT;
        }
    }

    // Now the aeTempVecType table has been filled with (mostly) valid data, write it back to all operands
    psInst = psFirstInst;
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
                METALWriteOperandTypes(psSubOperand, aeTempVecType);
            }
        }
        if (psInst->asOperands[0].eType == OPERAND_TYPE_CONSTANT_BUFFER)
        {
            METALSetCBOperandComponents(psContext, &psInst->asOperands[0]);
        }

        //Preserve the current type on sources.
        for (k = psInst->ui32NumOperands - 1; k >= (int)psInst->ui32FirstSrc; --k)
        {
            int32_t subOperand;
            Operand* psOperand = &psInst->asOperands[k];

            METALWriteOperandTypes(psOperand, aeTempVecType);
            if (psOperand->eType == OPERAND_TYPE_CONSTANT_BUFFER)
            {
                METALSetCBOperandComponents(psContext, psOperand);
            }

            for (subOperand = 0; subOperand < MAX_SUB_OPERANDS; subOperand++)
            {
                if (psOperand->psSubOperand[subOperand] != 0)
                {
                    Operand* psSubOperand = psOperand->psSubOperand[subOperand];
                    METALWriteOperandTypes(psSubOperand, aeTempVecType);
                    if (psSubOperand->eType == OPERAND_TYPE_CONSTANT_BUFFER)
                    {
                        METALSetCBOperandComponents(psContext, psSubOperand);
                    }
                }
            }

            //Set immediates
            if (METALIsIntegerImmediateOpcode(psInst->eOpcode))
            {
                if (psOperand->eType == OPERAND_TYPE_IMMEDIATE32)
                {
                    psOperand->iIntegerImmediate = 1;
                }
            }
        }

        //Process the destination last in order to handle instructions
        //where the destination register is also used as a source.
        for (k = 0; k < (int)psInst->ui32FirstSrc; ++k)
        {
            Operand* psOperand = &psInst->asOperands[k];
            METALWriteOperandTypes(psOperand, aeTempVecType);
        }
    }
}

void DetectAtomicInstructionMETAL(HLSLCrossCompilerContext* psContext, Instruction* psInst, Instruction* psNextInst, AtomicVarList* psAtomicList)
{
    (void)psNextInst;

    Operand* dest = 0;
    Operand* destAddr = 0;

    switch (psInst->eOpcode)
    {
    case OPCODE_ATOMIC_CMP_STORE:
    case OPCODE_ATOMIC_AND:
    case OPCODE_ATOMIC_IADD:
    case OPCODE_ATOMIC_OR:
    case OPCODE_ATOMIC_XOR:
    case OPCODE_ATOMIC_IMIN:
    case OPCODE_ATOMIC_UMIN:
    case OPCODE_ATOMIC_UMAX:
    case OPCODE_ATOMIC_IMAX:
        dest = &psInst->asOperands[0];
        destAddr = &psInst->asOperands[1];
        break;
    case OPCODE_IMM_ATOMIC_IADD:
    case OPCODE_IMM_ATOMIC_IMAX:
    case OPCODE_IMM_ATOMIC_IMIN:
    case OPCODE_IMM_ATOMIC_UMAX:
    case OPCODE_IMM_ATOMIC_UMIN:
    case OPCODE_IMM_ATOMIC_OR:
    case OPCODE_IMM_ATOMIC_XOR:
    case OPCODE_IMM_ATOMIC_EXCH:
    case OPCODE_IMM_ATOMIC_CMP_EXCH:
    case OPCODE_IMM_ATOMIC_AND:
        dest = &psInst->asOperands[1];
        destAddr = &psInst->asOperands[2];
        break;
    default:
        return;
    }

    ShaderVarType* psVarType = NULL;
    if (dest->eType == OPERAND_TYPE_THREAD_GROUP_SHARED_MEMORY)
    {
    }
    else
    {
        ResourceBinding* psRes;
        int foundResource = GetResourceFromBindingPoint(RGROUP_UAV,
                dest->ui32RegisterNumber,
                &psContext->psShader->sInfo,
                &psRes);

        ASSERT(foundResource);

        {
            //rwbuffer
            if (psRes->eType == RTYPE_UAV_RWTYPED)
            {
            }
            //rwstructured buffer
            else if (psRes->eType == RTYPE_UAV_RWSTRUCTURED)
            {
                if (destAddr->eType == OPERAND_TYPE_IMMEDIATE32)
                {
                    psAtomicList->AtomicVars[psAtomicList->Filled] = METALLookupStructuredVarAtomic(psContext, dest, destAddr, 0);
                    psAtomicList->Filled++;
                }
            }
        }
    }
}

void TranslateInstructionMETAL(HLSLCrossCompilerContext* psContext, Instruction* psInst, Instruction* psNextInst)
{
    bstring metal = *psContext->currentShaderString;
    int numParenthesis = 0;

#ifdef _DEBUG
    AddIndentation(psContext);
    bformata(metal, "//Instruction %d\n", psInst->id);
#if 0
    if (psInst->id == 73)
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
        uint32_t dstCount = GetNumSwizzleElementsMETAL(&psInst->asOperands[0]);
        uint32_t srcCount = GetNumSwizzleElementsMETAL(&psInst->asOperands[1]);
        uint32_t ui32DstFlags = TO_FLAG_DESTINATION;
        const SHADER_VARIABLE_TYPE eSrcType = GetOperandDataTypeMETAL(psContext, &psInst->asOperands[1]);
        const SHADER_VARIABLE_TYPE eDestType = GetOperandDataTypeMETAL(psContext, &psInst->asOperands[0]);

#ifdef _DEBUG
        AddIndentation(psContext);
        if (psInst->eOpcode == OPCODE_FTOU)
        {
            bcatcstr(metal, "//FTOU\n");
        }
        else
        {
            bcatcstr(metal, "//FTOI\n");
        }
#endif

        AddIndentation(psContext);

        METALAddAssignToDest(psContext, &psInst->asOperands[0], psInst->eOpcode == OPCODE_FTOU ? SVT_UINT : SVT_INT, srcCount, &numParenthesis);
        bcatcstr(metal, GetConstructorForTypeMETAL(psInst->eOpcode == OPCODE_FTOU ? SVT_UINT : SVT_INT, srcCount == dstCount ? dstCount : 4));
        bcatcstr(metal, "("); // 1
        TranslateOperandMETAL(psContext, &psInst->asOperands[1], TO_AUTO_BITCAST_TO_FLOAT);
        bcatcstr(metal, ")"); // 1
        // Add destination writemask if the component counts do not match
        if (srcCount != dstCount)
        {
            AddSwizzleUsingElementCountMETAL(psContext, dstCount);
        }
        METALAddAssignPrologue(psContext, numParenthesis);
        break;
    }

    case OPCODE_MOV:
    {
#ifdef _DEBUG
        AddIndentation(psContext);
        bcatcstr(metal, "//MOV\n");
#endif
        AddIndentation(psContext);
        METALAddMOVBinaryOp(psContext, &psInst->asOperands[0], &psInst->asOperands[1]);
        break;
    }
    case OPCODE_ITOF://signed to float
    case OPCODE_UTOF://unsigned to float
    {
        const SHADER_VARIABLE_TYPE eDestType = GetOperandDataTypeMETAL(psContext, &psInst->asOperands[0]);
        const SHADER_VARIABLE_TYPE eSrcType = GetOperandDataTypeMETAL(psContext, &psInst->asOperands[1]);
        uint32_t dstCount = GetNumSwizzleElementsMETAL(&psInst->asOperands[0]);
        uint32_t srcCount = GetNumSwizzleElementsMETAL(&psInst->asOperands[1]);
        uint32_t destMask = GetOperandWriteMaskMETAL(&psInst->asOperands[0]);

#ifdef _DEBUG
        AddIndentation(psContext);
        if (psInst->eOpcode == OPCODE_ITOF)
        {
            bcatcstr(metal, "//ITOF\n");
        }
        else
        {
            bcatcstr(metal, "//UTOF\n");
        }
#endif
        AddIndentation(psContext);
        METALAddAssignToDest(psContext, &psInst->asOperands[0], SVT_FLOAT, srcCount, &numParenthesis);
        bcatcstr(metal, GetConstructorForTypeMETAL(SVT_FLOAT, dstCount));
        bcatcstr(metal, "("); // 1
        TranslateOperandWithMaskMETAL(psContext, &psInst->asOperands[1], psInst->eOpcode == OPCODE_UTOF ? TO_AUTO_BITCAST_TO_UINT : TO_AUTO_BITCAST_TO_INT, destMask);
        bcatcstr(metal, ")"); // 1
        // Add destination writemask if the component counts do not match
        if (srcCount != dstCount)
        {
            AddSwizzleUsingElementCountMETAL(psContext, dstCount);
        }
        METALAddAssignPrologue(psContext, numParenthesis);
        break;
    }
    case OPCODE_MAD:
    {
#ifdef _DEBUG
        AddIndentation(psContext);
        bcatcstr(metal, "//MAD\n");
#endif
        METALCallTernaryOp(psContext, "*", "+", psInst, 0, 1, 2, 3, TO_FLAG_NONE);
        break;
    }
    case OPCODE_IMAD:
    {
        uint32_t ui32Flags = TO_FLAG_INTEGER;
#ifdef _DEBUG
        AddIndentation(psContext);
        bcatcstr(metal, "//IMAD\n");
#endif

        if (GetOperandDataTypeMETAL(psContext, &psInst->asOperands[0]) == SVT_UINT)
        {
            ui32Flags = TO_FLAG_UNSIGNED_INTEGER;
        }

        METALCallTernaryOp(psContext, "*", "+", psInst, 0, 1, 2, 3, ui32Flags);
        break;
    }
    case OPCODE_DADD:
    {
#ifdef _DEBUG
        AddIndentation(psContext);
        bcatcstr(metal, "//DADD\n");
#endif
        METALCallBinaryOp(psContext, "+", psInst, 0, 1, 2, SVT_DOUBLE);
        break;
    }
    case OPCODE_IADD:
    {
        SHADER_VARIABLE_TYPE eType = SVT_INT;
#ifdef _DEBUG
        AddIndentation(psContext);
        bcatcstr(metal, "//IADD\n");
#endif
        //Is this a signed or unsigned add?
        if (GetOperandDataTypeMETAL(psContext, &psInst->asOperands[0]) == SVT_UINT)
        {
            eType = SVT_UINT;
        }
        METALCallBinaryOp(psContext, "+", psInst, 0, 1, 2, eType);
        break;
    }
    case OPCODE_ADD:
    {
#ifdef _DEBUG
        AddIndentation(psContext);
        bcatcstr(metal, "//ADD\n");
#endif
        METALCallBinaryOp(psContext, "+", psInst, 0, 1, 2, SVT_FLOAT);
        break;
    }
    case OPCODE_OR:
    {
        /*Todo: vector version */
#ifdef _DEBUG
        AddIndentation(psContext);
        bcatcstr(metal, "//OR\n");
#endif
        METALCallBinaryOp(psContext, "|", psInst, 0, 1, 2, SVT_UINT);
        break;
    }
    case OPCODE_AND:
    {
#ifdef _DEBUG
        AddIndentation(psContext);
        bcatcstr(metal, "//AND\n");
#endif
        METALCallBinaryOp(psContext, "&", psInst, 0, 1, 2, SVT_UINT);
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
        bcatcstr(metal, "//GE\n");
#endif
        METALAddComparision(psContext, psInst, METAL_CMP_GE, TO_FLAG_NONE, NULL);
        break;
    }
    case OPCODE_MUL:
    {
#ifdef _DEBUG
        AddIndentation(psContext);
        bcatcstr(metal, "//MUL\n");
#endif
        METALCallBinaryOp(psContext, "*", psInst, 0, 1, 2, SVT_FLOAT);
        break;
    }
    case OPCODE_IMUL:
    {
        SHADER_VARIABLE_TYPE eType = SVT_INT;
#ifdef _DEBUG
        AddIndentation(psContext);
        bcatcstr(metal, "//IMUL\n");
#endif
        if (GetOperandDataTypeMETAL(psContext, &psInst->asOperands[1]) == SVT_UINT)
        {
            eType = SVT_UINT;
        }

        ASSERT(psInst->asOperands[0].eType == OPERAND_TYPE_NULL);

        METALCallBinaryOp(psContext, "*", psInst, 1, 2, 3, eType);
        break;
    }
    case OPCODE_UDIV:
    {
#ifdef _DEBUG
        AddIndentation(psContext);
        bcatcstr(metal, "//UDIV\n");
#endif
        //destQuotient, destRemainder, src0, src1
        METALCallBinaryOp(psContext, "/", psInst, 0, 2, 3, SVT_UINT);
        METALCallBinaryOp(psContext, "%", psInst, 1, 2, 3, SVT_UINT);
        break;
    }
    case OPCODE_DIV:
    {
#ifdef _DEBUG
        AddIndentation(psContext);
        bcatcstr(metal, "//DIV\n");
#endif
        METALCallBinaryOp(psContext, "/", psInst, 0, 1, 2, SVT_FLOAT);
        break;
    }
    case OPCODE_SINCOS:
    {
#ifdef _DEBUG
        AddIndentation(psContext);
        bcatcstr(metal, "//SINCOS\n");
#endif
        // Need careful ordering if src == dest[0], as then the cos() will be reading from wrong value
        if (psInst->asOperands[0].eType == psInst->asOperands[2].eType &&
            psInst->asOperands[0].ui32RegisterNumber == psInst->asOperands[2].ui32RegisterNumber)
        {
            // sin() result overwrites source, do cos() first.
            // The case where both write the src shouldn't really happen anyway.
            if (psInst->asOperands[1].eType != OPERAND_TYPE_NULL)
            {
                METALCallHelper1(psContext, "cos", psInst, 1, 2, 1);
            }

            if (psInst->asOperands[0].eType != OPERAND_TYPE_NULL)
            {
                METALCallHelper1(psContext, "sin", psInst, 0, 2, 1);
            }
        }
        else
        {
            if (psInst->asOperands[0].eType != OPERAND_TYPE_NULL)
            {
                METALCallHelper1(psContext, "sin", psInst, 0, 2, 1);
            }

            if (psInst->asOperands[1].eType != OPERAND_TYPE_NULL)
            {
                METALCallHelper1(psContext, "cos", psInst, 1, 2, 1);
            }
        }
        break;
    }

    case OPCODE_DP2:
    {
        SHADER_VARIABLE_TYPE eDestDataType = GetOperandDataTypeMETAL(psContext, &psInst->asOperands[0]);
        int numParenthesis2 = 0;
#ifdef _DEBUG
        AddIndentation(psContext);
        bcatcstr(metal, "//DP2\n");
#endif
        AddIndentation(psContext);
        METALAddAssignToDest(psContext, &psInst->asOperands[0], SVT_FLOAT, 1, &numParenthesis2);
        bcatcstr(metal, "dot(");
        TranslateOperandWithMaskMETAL(psContext, &psInst->asOperands[1], TO_AUTO_BITCAST_TO_FLOAT | SVTTypeToFlagMETAL(eDestDataType), 3 /* .xy */);
        bcatcstr(metal, ", ");
        TranslateOperandWithMaskMETAL(psContext, &psInst->asOperands[2], TO_AUTO_BITCAST_TO_FLOAT | SVTTypeToFlagMETAL(eDestDataType), 3 /* .xy */);
        bcatcstr(metal, ")");
        METALAddAssignPrologue(psContext, numParenthesis2);
        break;
    }
    case OPCODE_DP3:
    {
        SHADER_VARIABLE_TYPE eDestDataType = GetOperandDataTypeMETAL(psContext, &psInst->asOperands[0]);
        int numParenthesis2 = 0;
#ifdef _DEBUG
        AddIndentation(psContext);
        bcatcstr(metal, "//DP3\n");
#endif
        AddIndentation(psContext);
        METALAddAssignToDest(psContext, &psInst->asOperands[0], SVT_FLOAT, 1, &numParenthesis2);
        bcatcstr(metal, "dot(");
        TranslateOperandWithMaskMETAL(psContext, &psInst->asOperands[1], TO_AUTO_BITCAST_TO_FLOAT | SVTTypeToFlagMETAL(eDestDataType), 7 /* .xyz */);
        bcatcstr(metal, ", ");
        TranslateOperandWithMaskMETAL(psContext, &psInst->asOperands[2], TO_AUTO_BITCAST_TO_FLOAT | SVTTypeToFlagMETAL(eDestDataType), 7 /* .xyz */);
        bcatcstr(metal, ")");
        METALAddAssignPrologue(psContext, numParenthesis2);
        break;
    }
    case OPCODE_DP4:
    {
#ifdef _DEBUG
        AddIndentation(psContext);
        bcatcstr(metal, "//DP4\n");
#endif
        METALCallHelper2(psContext, "dot", psInst, 0, 1, 2, 0);
        break;
    }
    case OPCODE_INE:
    {
#ifdef _DEBUG
        AddIndentation(psContext);
        bcatcstr(metal, "//INE\n");
#endif
        METALAddComparision(psContext, psInst, METAL_CMP_NE, TO_FLAG_INTEGER, NULL);
        break;
    }
    case OPCODE_NE:
    {
#ifdef _DEBUG
        AddIndentation(psContext);
        bcatcstr(metal, "//NE\n");
#endif
        METALAddComparision(psContext, psInst, METAL_CMP_NE, TO_FLAG_NONE, NULL);
        break;
    }
    case OPCODE_IGE:
    {
#ifdef _DEBUG
        AddIndentation(psContext);
        bcatcstr(metal, "//IGE\n");
#endif
        METALAddComparision(psContext, psInst, METAL_CMP_GE, TO_FLAG_INTEGER, psNextInst);
        break;
    }
    case OPCODE_ILT:
    {
#ifdef _DEBUG
        AddIndentation(psContext);
        bcatcstr(metal, "//ILT\n");
#endif
        METALAddComparision(psContext, psInst, METAL_CMP_LT, TO_FLAG_INTEGER, NULL);
        break;
    }
    case OPCODE_LT:
    {
#ifdef _DEBUG
        AddIndentation(psContext);
        bcatcstr(metal, "//LT\n");
#endif
        METALAddComparision(psContext, psInst, METAL_CMP_LT, TO_FLAG_NONE, NULL);
        break;
    }
    case OPCODE_IEQ:
    {
#ifdef _DEBUG
        AddIndentation(psContext);
        bcatcstr(metal, "//IEQ\n");
#endif
        METALAddComparision(psContext, psInst, METAL_CMP_EQ, TO_FLAG_INTEGER, NULL);
        break;
    }
    case OPCODE_ULT:
    {
#ifdef _DEBUG
        AddIndentation(psContext);
        bcatcstr(metal, "//ULT\n");
#endif
        METALAddComparision(psContext, psInst, METAL_CMP_LT, TO_FLAG_UNSIGNED_INTEGER, NULL);
        break;
    }
    case OPCODE_UGE:
    {
#ifdef _DEBUG
        AddIndentation(psContext);
        bcatcstr(metal, "//UGE\n");
#endif
        METALAddComparision(psContext, psInst, METAL_CMP_GE, TO_FLAG_UNSIGNED_INTEGER, NULL);
        break;
    }
    case OPCODE_MOVC:
    {
#ifdef _DEBUG
        AddIndentation(psContext);
        bcatcstr(metal, "//MOVC\n");
#endif
        METALAddMOVCBinaryOp(psContext, &psInst->asOperands[0], &psInst->asOperands[1], &psInst->asOperands[2], &psInst->asOperands[3]);
        break;
    }
    case OPCODE_SWAPC:
    {
#ifdef _DEBUG
        AddIndentation(psContext);
        bcatcstr(metal, "//SWAPC\n");
#endif
        // TODO needs temps!!
        METALAddMOVCBinaryOp(psContext, &psInst->asOperands[0], &psInst->asOperands[2], &psInst->asOperands[4], &psInst->asOperands[3]);
        METALAddMOVCBinaryOp(psContext, &psInst->asOperands[1], &psInst->asOperands[2], &psInst->asOperands[3], &psInst->asOperands[4]);
        break;
    }

    case OPCODE_LOG:
    {
#ifdef _DEBUG
        AddIndentation(psContext);
        bcatcstr(metal, "//LOG\n");
#endif
        METALCallHelper1(psContext, "log2", psInst, 0, 1, 1);
        break;
    }
    case OPCODE_RSQ:
    {
#ifdef _DEBUG
        AddIndentation(psContext);
        bcatcstr(metal, "//RSQ\n");
#endif
        METALCallHelper1(psContext, "rsqrt", psInst, 0, 1, 1);
        break;
    }
    case OPCODE_EXP:
    {
#ifdef _DEBUG
        AddIndentation(psContext);
        bcatcstr(metal, "//EXP\n");
#endif
        METALCallHelper1(psContext, "exp2", psInst, 0, 1, 1);
        break;
    }
    case OPCODE_SQRT:
    {
#ifdef _DEBUG
        AddIndentation(psContext);
        bcatcstr(metal, "//SQRT\n");
#endif
        METALCallHelper1(psContext, "sqrt", psInst, 0, 1, 1);
        break;
    }
    case OPCODE_ROUND_PI:
    {
#ifdef _DEBUG
        AddIndentation(psContext);
        bcatcstr(metal, "//ROUND_PI\n");
#endif
        METALCallHelper1(psContext, "ceil", psInst, 0, 1, 1);
        break;
    }
    case OPCODE_ROUND_NI:
    {
#ifdef _DEBUG
        AddIndentation(psContext);
        bcatcstr(metal, "//ROUND_NI\n");
#endif
        METALCallHelper1(psContext, "floor", psInst, 0, 1, 1);
        break;
    }
    case OPCODE_ROUND_Z:
    {
#ifdef _DEBUG
        AddIndentation(psContext);
        bcatcstr(metal, "//ROUND_Z\n");
#endif
        METALCallHelper1(psContext, "trunc", psInst, 0, 1, 1);
        break;
    }
    case OPCODE_ROUND_NE:
    {
#ifdef _DEBUG
        AddIndentation(psContext);
        bcatcstr(metal, "//ROUND_NE\n");
#endif
        METALCallHelper1(psContext, "rint", psInst, 0, 1, 1);
        break;
    }
    case OPCODE_FRC:
    {
#ifdef _DEBUG
        AddIndentation(psContext);
        bcatcstr(metal, "//FRC\n");
#endif
        METALCallHelper1(psContext, "fract", psInst, 0, 1, 1);
        break;
    }
    case OPCODE_IMAX:
    {
#ifdef _DEBUG
        AddIndentation(psContext);
        bcatcstr(metal, "//IMAX\n");
#endif
        METALCallHelper2Int(psContext, "max", psInst, 0, 1, 2, 1);
        break;
    }
    case OPCODE_MAX:
    case OPCODE_UMAX:
    {
#ifdef _DEBUG
        AddIndentation(psContext);
        bcatcstr(metal, "//MAX\n");
#endif
        METALCallHelper2(psContext, "max", psInst, 0, 1, 2, 1);
        break;
    }
    case OPCODE_IMIN:
    {
#ifdef _DEBUG
        AddIndentation(psContext);
        bcatcstr(metal, "//IMIN\n");
#endif
        METALCallHelper2Int(psContext, "min", psInst, 0, 1, 2, 1);
        break;
    }
    case OPCODE_MIN:
    case OPCODE_UMIN:
    {
#ifdef _DEBUG
        AddIndentation(psContext);
        bcatcstr(metal, "//MIN\n");
#endif
        METALCallHelper2(psContext, "min", psInst, 0, 1, 2, 1);
        break;
    }
    case OPCODE_GATHER4:
    case OPCODE_GATHER4_C:
    {
        //dest, coords, tex, sampler
        const RESOURCE_DIMENSION eResDim = psContext->psShader->aeResourceDims[psInst->asOperands[2].ui32RegisterNumber];

#ifdef _DEBUG
        AddIndentation(psContext);
        if (psInst->eOpcode == OPCODE_GATHER4_C)
        {
            bcatcstr(metal, "//GATHER4_C\n");
        }
        else
        {
            bcatcstr(metal, "//GATHER4\n");
        }
#endif
        //gather4 r7.xyzw, r3.xyxx, t3.xyzw, s0.x
        AddIndentation(psContext); // TODO FIXME integer samplers
        METALAddAssignToDest(psContext, &psInst->asOperands[0], SVT_FLOAT, GetNumSwizzleElementsMETAL(&psInst->asOperands[2]), &numParenthesis);
        bcatcstr(metal, "(");

        ResourceNameMETAL(metal, psContext, RGROUP_TEXTURE, psInst->asOperands[2].ui32RegisterNumber, 0);

        bcatcstr(metal, ".gather(");
        bconcat(metal, TextureSamplerNameMETAL(&psContext->psShader->sInfo, psInst->asOperands[2].ui32RegisterNumber, psInst->asOperands[3].ui32RegisterNumber, psInst->eOpcode == OPCODE_GATHER4_PO_C));
        bcatcstr(metal, ", ");
        METALTranslateTexCoord(psContext, eResDim, &psInst->asOperands[1]);

        if (psInst->eOpcode == OPCODE_GATHER4_C)
        {
            bcatcstr(metal, ", ");
            TranslateOperandMETAL(psContext, &psInst->asOperands[4], TO_FLAG_NONE);
        }
        bcatcstr(metal, ")");

        // iWriteMaskEnabled is forced off during DecodeOperand because swizzle on sampler uniforms
        // does not make sense. But need to re-enable to correctly swizzle this particular instruction.
        psInst->asOperands[2].iWriteMaskEnabled = 1;
        TranslateOperandSwizzleMETAL(psContext, &psInst->asOperands[2]);
        bcatcstr(metal, ")");

        AddSwizzleUsingElementCountMETAL(psContext, GetNumSwizzleElementsMETAL(&psInst->asOperands[0]));
        METALAddAssignPrologue(psContext, numParenthesis);
        break;
    }
    case OPCODE_GATHER4_PO:
    case OPCODE_GATHER4_PO_C:
    {
        //dest, coords, offset, tex, sampler, srcReferenceValue
        const RESOURCE_DIMENSION eResDim = psContext->psShader->aeResourceDims[psInst->asOperands[3].ui32RegisterNumber];

#ifdef _DEBUG
        AddIndentation(psContext);
        if (psInst->eOpcode == OPCODE_GATHER4_PO_C)
        {
            bcatcstr(metal, "//GATHER4_PO_C\n");
        }
        else
        {
            bcatcstr(metal, "//GATHER4_PO\n");
        }
#endif

        AddIndentation(psContext); // TODO FIXME integer samplers
        METALAddAssignToDest(psContext, &psInst->asOperands[0], SVT_FLOAT, GetNumSwizzleElementsMETAL(&psInst->asOperands[2]), &numParenthesis);
        bcatcstr(metal, "(");

        ResourceNameMETAL(metal, psContext, RGROUP_TEXTURE, psInst->asOperands[3].ui32RegisterNumber, 0);

        bcatcstr(metal, ".gather(");
        bconcat(metal, TextureSamplerNameMETAL(&psContext->psShader->sInfo, psInst->asOperands[3].ui32RegisterNumber, psInst->asOperands[4].ui32RegisterNumber, psInst->eOpcode == OPCODE_GATHER4_PO_C));

        bcatcstr(metal, ", ");
        //Texture coord cannot be vec4
        //Determining if it is a vec3 for vec2 yet to be done.
        psInst->asOperands[1].aui32Swizzle[2] = 0xFFFFFFFF;
        psInst->asOperands[1].aui32Swizzle[3] = 0xFFFFFFFF;
        TranslateOperandMETAL(psContext, &psInst->asOperands[1], TO_FLAG_NONE);

        if (psInst->eOpcode == OPCODE_GATHER4_PO_C)
        {
            bcatcstr(metal, ", ");
            TranslateOperandMETAL(psContext, &psInst->asOperands[5], TO_FLAG_NONE);
        }

        bcatcstr(metal, ", as_type<int2>(");
        //ivec2 offset
        psInst->asOperands[2].aui32Swizzle[2] = 0xFFFFFFFF;
        psInst->asOperands[2].aui32Swizzle[3] = 0xFFFFFFFF;
        TranslateOperandMETAL(psContext, &psInst->asOperands[2], TO_FLAG_NONE);
        bcatcstr(metal, "))");
        // iWriteMaskEnabled is forced off during DecodeOperand because swizzle on sampler uniforms
        // does not make sense. But need to re-enable to correctly swizzle this particular instruction.
        psInst->asOperands[2].iWriteMaskEnabled = 1;
        TranslateOperandSwizzleMETAL(psContext, &psInst->asOperands[3]);
        bcatcstr(metal, ")");

        AddSwizzleUsingElementCountMETAL(psContext, GetNumSwizzleElementsMETAL(&psInst->asOperands[0]));
        METALAddAssignPrologue(psContext, numParenthesis);
        break;
    }
    case OPCODE_SAMPLE:
    {
#ifdef _DEBUG
        AddIndentation(psContext);
        bcatcstr(metal, "//SAMPLE\n");
#endif
        METALTranslateTextureSample(psContext, psInst, TEXSMP_FLAG_NONE);
        break;
    }
    case OPCODE_SAMPLE_L:
    {
#ifdef _DEBUG
        AddIndentation(psContext);
        bcatcstr(metal, "//SAMPLE_L\n");
#endif
        METALTranslateTextureSample(psContext, psInst, TEXSMP_FLAG_LOD);
        break;
    }
    case OPCODE_SAMPLE_C:
    {
#ifdef _DEBUG
        AddIndentation(psContext);
        bcatcstr(metal, "//SAMPLE_C\n");
#endif

        METALTranslateTextureSample(psContext, psInst, TEXSMP_FLAG_DEPTHCOMPARE);
        break;
    }
    case OPCODE_SAMPLE_C_LZ:
    {
#ifdef _DEBUG
        AddIndentation(psContext);
        bcatcstr(metal, "//SAMPLE_C_LZ\n");
#endif

        METALTranslateTextureSample(psContext, psInst, TEXSMP_FLAG_DEPTHCOMPARE | TEXSMP_FLAG_FIRSTLOD);
        break;
    }
    case OPCODE_SAMPLE_D:
    {
#ifdef _DEBUG
        AddIndentation(psContext);
        bcatcstr(metal, "//SAMPLE_D\n");
#endif

        METALTranslateTextureSample(psContext, psInst, TEXSMP_FLAGS_GRAD);
        break;
    }
    case OPCODE_SAMPLE_B:
    {
#ifdef _DEBUG
        AddIndentation(psContext);
        bcatcstr(metal, "//SAMPLE_B\n");
#endif

        METALTranslateTextureSample(psContext, psInst, TEXSMP_FLAG_BIAS);
        break;
    }
    case OPCODE_RET:
    {
#ifdef _DEBUG
        AddIndentation(psContext);
        bcatcstr(metal, "//RET\n");
#endif
        if (psContext->havePostShaderCode[psContext->currentPhase])
        {
#ifdef _DEBUG
            AddIndentation(psContext);
            bcatcstr(metal, "//--- Post shader code ---\n");
#endif
            bconcat(metal, psContext->postShaderCode[psContext->currentPhase]);
#ifdef _DEBUG
            AddIndentation(psContext);
            bcatcstr(metal, "//--- End post shader code ---\n");
#endif
        }
        AddIndentation(psContext);
        if (blength(psContext->declaredOutputs) > 0)
        {
            //has output
            bcatcstr(metal, "return output;\n");
        }
        else
        {
            //no output declared
            bcatcstr(metal, "return;\n");
        }
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
        bcatcstr(metal, "//INTERFACE_CALL\n");
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
        bcatcstr(metal, name);
        TranslateOperandIndexMADMETAL(psContext, &psInst->asOperands[0], 1, ui32NumBodiesPerTable, funcBodyIndex);
        //bformata(glsl, "[%d]", funcBodyIndex);
        bcatcstr(metal, "();\n");
        break;
    }
    case OPCODE_LABEL:
    {
#ifdef _DEBUG
        AddIndentation(psContext);
        bcatcstr(metal, "//LABEL\n");
#endif
        --psContext->indent;
        AddIndentation(psContext);
        bcatcstr(metal, "}\n"); //Closing brace ends the previous function.
        AddIndentation(psContext);

        bcatcstr(metal, "subroutine(SubroutineType)\n");
        bcatcstr(metal, "void ");
        TranslateOperandMETAL(psContext, &psInst->asOperands[0], TO_FLAG_DESTINATION);
        bcatcstr(metal, "(){\n");
        ++psContext->indent;
        break;
    }
    case OPCODE_COUNTBITS:
    {
#ifdef _DEBUG
        AddIndentation(psContext);
        bcatcstr(metal, "//COUNTBITS\n");
#endif
        AddIndentation(psContext);
        TranslateOperandMETAL(psContext, &psInst->asOperands[0], TO_FLAG_INTEGER | TO_FLAG_DESTINATION);
        bcatcstr(metal, " = popcount(");
        TranslateOperandMETAL(psContext, &psInst->asOperands[1], TO_FLAG_INTEGER);
        bcatcstr(metal, ");\n");
        break;
    }
    case OPCODE_FIRSTBIT_HI:
    {
#ifdef _DEBUG
        AddIndentation(psContext);
        bcatcstr(metal, "//FIRSTBIT_HI\n");
#endif
        AddIndentation(psContext);
        TranslateOperandMETAL(psContext, &psInst->asOperands[0], TO_FLAG_UNSIGNED_INTEGER | TO_FLAG_DESTINATION);
        bcatcstr(metal, " = (32 - clz(");
        TranslateOperandMETAL(psContext, &psInst->asOperands[1], TO_FLAG_UNSIGNED_INTEGER);
        bcatcstr(metal, "));\n");
        break;
    }
    case OPCODE_FIRSTBIT_LO:
    {
#ifdef _DEBUG
        AddIndentation(psContext);
        bcatcstr(metal, "//FIRSTBIT_LO\n");
#endif
        AddIndentation(psContext);
        TranslateOperandMETAL(psContext, &psInst->asOperands[0], TO_FLAG_UNSIGNED_INTEGER | TO_FLAG_DESTINATION);
        bcatcstr(metal, " = (1 + ctz(");
        TranslateOperandMETAL(psContext, &psInst->asOperands[1], TO_FLAG_UNSIGNED_INTEGER);
        bcatcstr(metal, ")));\n");
        break;
    }
    case OPCODE_FIRSTBIT_SHI: //signed high
    {
#ifdef _DEBUG
        AddIndentation(psContext);
        bcatcstr(metal, "//FIRSTBIT_SHI\n");
#endif
        AddIndentation(psContext);
        TranslateOperandMETAL(psContext, &psInst->asOperands[0], TO_FLAG_INTEGER | TO_FLAG_DESTINATION);
        bcatcstr(metal, " = (32 - clz(");
        TranslateOperandMETAL(psContext, &psInst->asOperands[1], TO_FLAG_INTEGER);
        bcatcstr(metal, " > 0 ? ");
        TranslateOperandMETAL(psContext, &psInst->asOperands[1], TO_FLAG_INTEGER);
        bcatcstr(metal, " : 0xFFFFFFFF ^ ");
        TranslateOperandMETAL(psContext, &psInst->asOperands[1], TO_FLAG_INTEGER);
        bcatcstr(metal, ")));\n");
        break;
    }
    case OPCODE_BFI:
    {
#ifdef _DEBUG
        AddIndentation(psContext);
        bcatcstr(metal, "//BFI\n");
#endif
        // This instruction is not available in Metal shading language.
        // Need to expend it out (http://http.developer.nvidia.com/Cg/bitfieldInsert.html)

        int numComponents = psInst->asOperands[0].iNumComponents;

        AddIndentation(psContext);
        TranslateOperandMETAL(psContext, &psInst->asOperands[0], TO_FLAG_DESTINATION);
        bcatcstr(metal, " = 0;\n");

        AddIndentation(psContext);
        bcatcstr(metal, "{\n");

        AddIndentation(psContext);
        bformata(metal, "  %s mask = ~(%s(0xffffffff) << ", GetConstructorForTypeMETAL(SVT_UINT, numComponents), GetConstructorForTypeMETAL(SVT_UINT, numComponents));
        TranslateOperandMETAL(psContext, &psInst->asOperands[1], TO_FLAG_UNSIGNED_INTEGER);
        bcatcstr(metal, ") << ");
        TranslateOperandMETAL(psContext, &psInst->asOperands[2], TO_FLAG_UNSIGNED_INTEGER);
        bcatcstr(metal, ";\n");

        AddIndentation(psContext);
        bcatcstr(metal, "  mask = ~mask;\n");

        AddIndentation(psContext);
        bcatcstr(metal, "  ");
        TranslateOperandMETAL(psContext, &psInst->asOperands[0], TO_FLAG_DESTINATION);
        bformata(metal, " = ( as_type<%s>( (", GetConstructorForTypeMETAL(psInst->asOperands[0].aeDataType[0], numComponents));
        TranslateOperandMETAL(psContext, &psInst->asOperands[4], TO_FLAG_UNSIGNED_INTEGER);
        bcatcstr(metal, " & mask) | (");
        TranslateOperandMETAL(psContext, &psInst->asOperands[3], TO_FLAG_UNSIGNED_INTEGER);
        bcatcstr(metal, " << ");
        TranslateOperandMETAL(psContext, &psInst->asOperands[2], TO_FLAG_UNSIGNED_INTEGER);
        bcatcstr(metal, ")) )");
        TranslateOperandSwizzleWithMaskMETAL(psContext, &psInst->asOperands[0], GetOperandWriteMaskMETAL(&psInst->asOperands[0]));
        bcatcstr(metal, ";\n");

        AddIndentation(psContext);
        bcatcstr(metal, "}\n");



        break;
    }
    case OPCODE_BFREV:
    case OPCODE_CUT:
    case OPCODE_EMIT:
    case OPCODE_EMITTHENCUT:
    case OPCODE_CUT_STREAM:
    case OPCODE_EMIT_STREAM:
    case OPCODE_EMITTHENCUT_STREAM:
    {
        // not implemented in metal
        ASSERT(0);
        break;
    }
    case OPCODE_REP:
    {
#ifdef _DEBUG
        AddIndentation(psContext);
        bcatcstr(metal, "//REP\n");
#endif
        //Need to handle nesting.
        //Max of 4 for rep - 'Flow Control Limitations' http://msdn.microsoft.com/en-us/library/windows/desktop/bb219848(v=vs.85).aspx

        AddIndentation(psContext);
        bcatcstr(metal, "RepCounter = as_type<int4>(");
        TranslateOperandWithMaskMETAL(psContext, &psInst->asOperands[0], TO_FLAG_INTEGER, OPERAND_4_COMPONENT_MASK_X);
        bcatcstr(metal, ").x;\n");

        AddIndentation(psContext);
        bcatcstr(metal, "while(RepCounter!=0){\n");
        ++psContext->indent;
        break;
    }
    case OPCODE_ENDREP:
    {
#ifdef _DEBUG
        AddIndentation(psContext);
        bcatcstr(metal, "//ENDREP\n");
#endif
        AddIndentation(psContext);
        bcatcstr(metal, "RepCounter--;\n");

        --psContext->indent;

        AddIndentation(psContext);
        bcatcstr(metal, "}\n");
        break;
    }
    case OPCODE_LOOP:
    {
#ifdef _DEBUG
        AddIndentation(psContext);
        bcatcstr(metal, "//LOOP\n");
#endif
        AddIndentation(psContext);

        if (psInst->ui32NumOperands == 2)
        {
            //DX9 version
            ASSERT(psInst->asOperands[0].eType == OPERAND_TYPE_SPECIAL_LOOPCOUNTER);
            bcatcstr(metal, "for(");
            bcatcstr(metal, "LoopCounter = ");
            TranslateOperandMETAL(psContext, &psInst->asOperands[1], TO_FLAG_NONE);
            bcatcstr(metal, ".y, ZeroBasedCounter = 0;");
            bcatcstr(metal, "ZeroBasedCounter < ");
            TranslateOperandMETAL(psContext, &psInst->asOperands[1], TO_FLAG_NONE);
            bcatcstr(metal, ".x;");

            bcatcstr(metal, "LoopCounter += ");
            TranslateOperandMETAL(psContext, &psInst->asOperands[1], TO_FLAG_NONE);
            bcatcstr(metal, ".z, ZeroBasedCounter++){\n");
            ++psContext->indent;
        }
        else
        {
            bcatcstr(metal, "while(true){\n");
            ++psContext->indent;
        }
        break;
    }
    case OPCODE_ENDLOOP:
    {
        --psContext->indent;
#ifdef _DEBUG
        AddIndentation(psContext);
        bcatcstr(metal, "//ENDLOOP\n");
#endif
        AddIndentation(psContext);
        bcatcstr(metal, "}\n");
        break;
    }
    case OPCODE_BREAK:
    {
#ifdef _DEBUG
        AddIndentation(psContext);
        bcatcstr(metal, "//BREAK\n");
#endif
        AddIndentation(psContext);
        bcatcstr(metal, "break;\n");
        break;
    }
    case OPCODE_BREAKC:
    {
#ifdef _DEBUG
        AddIndentation(psContext);
        bcatcstr(metal, "//BREAKC\n");
#endif
        AddIndentation(psContext);

        METALTranslateConditional(psContext, psInst, metal);
        break;
    }
    case OPCODE_CONTINUEC:
    {
#ifdef _DEBUG
        AddIndentation(psContext);
        bcatcstr(metal, "//CONTINUEC\n");
#endif
        AddIndentation(psContext);

        METALTranslateConditional(psContext, psInst, metal);
        break;
    }
    case OPCODE_IF:
    {
#ifdef _DEBUG
        AddIndentation(psContext);
        bcatcstr(metal, "//IF\n");
#endif
        AddIndentation(psContext);

        METALTranslateConditional(psContext, psInst, metal);
        ++psContext->indent;
        break;
    }
    case OPCODE_RETC:
    {
#ifdef _DEBUG
        AddIndentation(psContext);
        bcatcstr(metal, "//RETC\n");
#endif
        AddIndentation(psContext);

        METALTranslateConditional(psContext, psInst, metal);
        break;
    }
    case OPCODE_ELSE:
    {
        --psContext->indent;
#ifdef _DEBUG
        AddIndentation(psContext);
        bcatcstr(metal, "//ELSE\n");
#endif
        AddIndentation(psContext);
        bcatcstr(metal, "} else {\n");
        psContext->indent++;
        break;
    }
    case OPCODE_ENDSWITCH:
    case OPCODE_ENDIF:
    {
        --psContext->indent;
        AddIndentation(psContext);
        bcatcstr(metal, "//ENDIF\n");
        AddIndentation(psContext);
        bcatcstr(metal, "}\n");
        break;
    }
    case OPCODE_CONTINUE:
    {
        AddIndentation(psContext);
        bcatcstr(metal, "continue;\n");
        break;
    }
    case OPCODE_DEFAULT:
    {
        --psContext->indent;
        AddIndentation(psContext);
        bcatcstr(metal, "default:\n");
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
        bcatcstr(metal, "//SYNC\n");
#endif
        //  warning. Although Metal documentation claims the flag can be combined
        //  this is not true in terms of binary operations. One can't simply OR flags
        //  but rather have to use pre-defined literals.
        char* aszBarrierType[] = {
            "mem_flags::mem_none",
            "mem_flags::mem_threadgroup",
            "mem_flags::mem_device",
            "mem_flags::mem_device_and_threadgroup"
        };
        typedef enum
        {
            BT_None,
            BT_MemThreadGroup,
            BT_MemDevice,
            BT_MemDeviceAndMemThreadGroup
        } BT;
        BT  barrierType = BT_None;

        if (ui32SyncFlags & SYNC_THREADS_IN_GROUP)
        {
            AddIndentation(psContext);
            bcatcstr(metal, "threadgroup_barrier(");
        }
        else
        {
            AddIndentation(psContext);
            //  simdgroup_barrier is faster than threadgroup_barrier. It is supported on iOS 10+ on all hardware.
            bcatcstr(metal, "threadgroup_barrier(");
        }

        if (ui32SyncFlags & SYNC_THREAD_GROUP_SHARED_MEMORY)
        {
            barrierType = (BT)(barrierType | BT_MemThreadGroup);
        }
        if (ui32SyncFlags & (SYNC_UNORDERED_ACCESS_VIEW_MEMORY_GROUP | SYNC_UNORDERED_ACCESS_VIEW_MEMORY_GLOBAL))
        {
            barrierType = (BT)(barrierType | BT_MemDevice);
        }

        bcatcstr(metal, aszBarrierType[barrierType]);
        bcatcstr(metal, ");\n");

        break;
    }
    case OPCODE_SWITCH:
    {
#ifdef _DEBUG
        AddIndentation(psContext);
        bcatcstr(metal, "//SWITCH\n");
#endif
        AddIndentation(psContext);
        bcatcstr(metal, "switch(int(");
        TranslateOperandMETAL(psContext, &psInst->asOperands[0], TO_FLAG_INTEGER);
        bcatcstr(metal, ")){\n");

        psContext->indent += 2;
        break;
    }
    case OPCODE_CASE:
    {
        --psContext->indent;
#ifdef _DEBUG
        AddIndentation(psContext);
        bcatcstr(metal, "//case\n");
#endif
        AddIndentation(psContext);

        bcatcstr(metal, "case ");
        TranslateOperandMETAL(psContext, &psInst->asOperands[0], TO_FLAG_INTEGER);
        bcatcstr(metal, ":\n");

        ++psContext->indent;
        break;
    }
    case OPCODE_EQ:
    {
#ifdef _DEBUG
        AddIndentation(psContext);
        bcatcstr(metal, "//EQ\n");
#endif
        METALAddComparision(psContext, psInst, METAL_CMP_EQ, TO_FLAG_NONE, NULL);
        break;
    }
    case OPCODE_USHR:
    {
#ifdef _DEBUG
        AddIndentation(psContext);
        bcatcstr(metal, "//USHR\n");
#endif
        METALCallBinaryOp(psContext, ">>", psInst, 0, 1, 2, SVT_UINT);
        break;
    }
    case OPCODE_ISHL:
    {
        SHADER_VARIABLE_TYPE eType = SVT_INT;

#ifdef _DEBUG
        AddIndentation(psContext);
        bcatcstr(metal, "//ISHL\n");
#endif

        if (GetOperandDataTypeMETAL(psContext, &psInst->asOperands[0]) == SVT_UINT)
        {
            eType = SVT_UINT;
        }

        METALCallBinaryOp(psContext, "<<", psInst, 0, 1, 2, eType);
        break;
    }
    case OPCODE_ISHR:
    {
        SHADER_VARIABLE_TYPE eType = SVT_INT;
#ifdef _DEBUG
        AddIndentation(psContext);
        bcatcstr(metal, "//ISHR\n");
#endif

        if (GetOperandDataTypeMETAL(psContext, &psInst->asOperands[0]) == SVT_UINT)
        {
            eType = SVT_UINT;
        }

        METALCallBinaryOp(psContext, ">>", psInst, 0, 1, 2, eType);
        break;
    }
    case OPCODE_LD:
    case OPCODE_LD_MS:
    {
        ResourceBinding* psBinding = 0;
        uint32_t dstSwizCount = GetNumSwizzleElementsMETAL(&psInst->asOperands[0]);
#ifdef _DEBUG
        AddIndentation(psContext);
        if (psInst->eOpcode == OPCODE_LD)
        {
            bcatcstr(metal, "//LD\n");
        }
        else
        {
            bcatcstr(metal, "//LD_MS\n");
        }
#endif

        GetResourceFromBindingPoint(RGROUP_TEXTURE, psInst->asOperands[2].ui32RegisterNumber, &psContext->psShader->sInfo, &psBinding);

        //if (psInst->bAddressOffset)
        //{
        //  METALTranslateTexelFetchOffset(psContext, psInst, psBinding, metal);
        //}
        //else
        //{
        METALTranslateTexelFetch(psContext, psInst, psBinding, metal);
        //}
        break;
    }
    case OPCODE_DISCARD:
    {
#ifdef _DEBUG
        AddIndentation(psContext);
        bcatcstr(metal, "//DISCARD\n");
#endif
        AddIndentation(psContext);

        if (psInst->eBooleanTestType == INSTRUCTION_TEST_ZERO)
        {
            bcatcstr(metal, "if(all(");
            TranslateOperandMETAL(psContext, &psInst->asOperands[0], TO_FLAG_INTEGER);
            bcatcstr(metal, "==0)){discard_fragment();}\n");
        }
        else
        {
            ASSERT(psInst->eBooleanTestType == INSTRUCTION_TEST_NONZERO);
            bcatcstr(metal, "if(any(");
            TranslateOperandMETAL(psContext, &psInst->asOperands[0], TO_FLAG_INTEGER);
            bcatcstr(metal, "!=0)){discard_fragment();}\n");
        }
        break;
    }
    case OPCODE_LOD:
    {
#ifdef _DEBUG
        AddIndentation(psContext);
        bcatcstr(metal, "//LOD\n");
#endif
        //LOD computes the following vector (ClampedLOD, NonClampedLOD, 0, 0)

        AddIndentation(psContext);
        METALAddAssignToDest(psContext, &psInst->asOperands[0], SVT_FLOAT, 4, &numParenthesis);

        //If the core language does not have query-lod feature,
        //then the extension is used. The name of the function
        //changed between extension and core.
        if (HaveQueryLod(psContext->psShader->eTargetLanguage))
        {
            bcatcstr(metal, "textureQueryLod(");
        }
        else
        {
            bcatcstr(metal, "textureQueryLOD(");
        }

        TranslateOperandMETAL(psContext, &psInst->asOperands[2], TO_FLAG_NONE);
        bcatcstr(metal, ",");
        METALTranslateTexCoord(psContext,
            psContext->psShader->aeResourceDims[psInst->asOperands[2].ui32RegisterNumber],
            &psInst->asOperands[1]);
        bcatcstr(metal, ")");

        //The swizzle on srcResource allows the returned values to be swizzled arbitrarily before they are written to the destination.

        // iWriteMaskEnabled is forced off during DecodeOperand because swizzle on sampler uniforms
        // does not make sense. But need to re-enable to correctly swizzle this particular instruction.
        psInst->asOperands[2].iWriteMaskEnabled = 1;
        TranslateOperandSwizzleWithMaskMETAL(psContext, &psInst->asOperands[2], GetOperandWriteMaskMETAL(&psInst->asOperands[0]));
        METALAddAssignPrologue(psContext, numParenthesis);
        break;
    }
    case OPCODE_EVAL_CENTROID:
    case OPCODE_EVAL_SAMPLE_INDEX:
    case OPCODE_EVAL_SNAPPED:
    {
        // ERROR: evaluation functions are not implemented in metal
        ASSERT(0);
        break;
    }
    case OPCODE_LD_STRUCTURED:
    {
#ifdef _DEBUG
        AddIndentation(psContext);
        bcatcstr(metal, "//LD_STRUCTURED\n");
#endif
        METALTranslateShaderStorageLoad(psContext, psInst);
        break;
    }
    case OPCODE_LD_UAV_TYPED:
    {
        // not implemented in metal
        ASSERT(0);
        break;
    }
    case OPCODE_STORE_RAW:
    {
#ifdef _DEBUG
        AddIndentation(psContext);
        bcatcstr(metal, "//STORE_RAW\n");
#endif
        METALTranslateShaderStorageStore(psContext, psInst);
        break;
    }
    case OPCODE_STORE_STRUCTURED:
    {
#ifdef _DEBUG
        AddIndentation(psContext);
        bcatcstr(metal, "//STORE_STRUCTURED\n");
#endif
        METALTranslateShaderStorageStore(psContext, psInst);
        break;
    }

    case OPCODE_STORE_UAV_TYPED:
    {
        ResourceBinding* psRes;
        int foundResource;

#ifdef _DEBUG
        AddIndentation(psContext);
        bcatcstr(metal, "//STORE_UAV_TYPED\n");
#endif
        AddIndentation(psContext);

        foundResource = GetResourceFromBindingPoint(RGROUP_UAV,
                psInst->asOperands[0].ui32RegisterNumber,
                &psContext->psShader->sInfo,
                &psRes);

        ASSERT(foundResource);

        if (psRes->eBindArea == UAVAREA_CBUFFER)
        {
            TranslateOperandMETAL(psContext, &psInst->asOperands[0], TO_FLAG_NAME_ONLY);
            bcatcstr(metal, "[");
            TranslateOperandWithMaskMETAL(psContext, &psInst->asOperands[1], TO_FLAG_INTEGER, OPERAND_4_COMPONENT_MASK_X);
            bcatcstr(metal, "]=");
            TranslateOperandWithMaskMETAL(psContext, &psInst->asOperands[2], METALResourceReturnTypeToFlag(psRes->ui32ReturnType), OPERAND_4_COMPONENT_MASK_X);
            bcatcstr(metal, ";\n");
        }
        else if (psRes->eBindArea == UAVAREA_TEXTURE)
        {
            TranslateOperandMETAL(psContext, &psInst->asOperands[0], TO_FLAG_NAME_ONLY);
            bcatcstr(metal, ".write(");
            TranslateOperandWithMaskMETAL(psContext, &psInst->asOperands[2], METALResourceReturnTypeToFlag(psRes->ui32ReturnType), OPERAND_4_COMPONENT_MASK_ALL);
            switch (psRes->eDimension)
            {
            case REFLECT_RESOURCE_DIMENSION_TEXTURE1D:
            {
                bcatcstr(metal, ",as_type<uint>(");
                TranslateOperandMETAL(psContext, &psInst->asOperands[1], TO_FLAG_NAME_ONLY);
                bcatcstr(metal, ") ");
                break;
            }
            case REFLECT_RESOURCE_DIMENSION_TEXTURE2D:
            {
                bcatcstr(metal, ",as_type<uint2>(");
                TranslateOperandMETAL(psContext, &psInst->asOperands[1], TO_FLAG_NAME_ONLY);
                bcatcstr(metal, ".xy) ");
                break;
            }
            case REFLECT_RESOURCE_DIMENSION_TEXTURE1DARRAY:
            {
                bcatcstr(metal, ",as_type<uint>(");
                TranslateOperandMETAL(psContext, &psInst->asOperands[1], TO_FLAG_NAME_ONLY);
                bcatcstr(metal, ".x) ");
                bcatcstr(metal, ",as_type<uint>(");
                TranslateOperandMETAL(psContext, &psInst->asOperands[1], TO_FLAG_NAME_ONLY);
                bcatcstr(metal, ".y) ");
                break;
            }
            case REFLECT_RESOURCE_DIMENSION_TEXTURE2DARRAY:
            {
                bcatcstr(metal, ",as_type<uint2>(");
                TranslateOperandMETAL(psContext, &psInst->asOperands[1], TO_FLAG_NAME_ONLY);
                bcatcstr(metal, ".xy) ");
                bcatcstr(metal, ",as_type<uint>(");
                TranslateOperandMETAL(psContext, &psInst->asOperands[1], TO_FLAG_NAME_ONLY);
                bcatcstr(metal, ".z) ");
                break;
            }
            case REFLECT_RESOURCE_DIMENSION_TEXTURE3D:
            {
                bcatcstr(metal, ", as_type<uint3>(");
                TranslateOperandMETAL(psContext, &psInst->asOperands[1], TO_FLAG_NAME_ONLY);
                bcatcstr(metal, ".xyz) ");
                break;
            }
            case REFLECT_RESOURCE_DIMENSION_TEXTURECUBE:
            {
                bcatcstr(metal, ",as_type<uint2>(");
                TranslateOperandMETAL(psContext, &psInst->asOperands[1], TO_FLAG_NAME_ONLY);
                bcatcstr(metal, ".xy) ");
                bcatcstr(metal, ",as_type<uint>(");
                TranslateOperandMETAL(psContext, &psInst->asOperands[1], TO_FLAG_NAME_ONLY);
                bcatcstr(metal, ".z) ");
                break;
            }
            case REFLECT_RESOURCE_DIMENSION_TEXTURECUBEARRAY:
            {
                bcatcstr(metal, ",as_type<uint2>(");
                TranslateOperandMETAL(psContext, &psInst->asOperands[1], TO_FLAG_NAME_ONLY);
                bcatcstr(metal, ".xy) ");
                bcatcstr(metal, ",as_type<uint>(");
                TranslateOperandMETAL(psContext, &psInst->asOperands[1], TO_FLAG_NAME_ONLY);
                bcatcstr(metal, ".z) ");
                bcatcstr(metal, ",as_type<uint>(");
                TranslateOperandMETAL(psContext, &psInst->asOperands[1], TO_FLAG_NAME_ONLY);
                bcatcstr(metal, ".w) ");
                break;
            }
            case REFLECT_RESOURCE_DIMENSION_TEXTURE2DMS:
            case REFLECT_RESOURCE_DIMENSION_TEXTURE2DMSARRAY:
                //not supported in mnetal
                ASSERT(0);
                break;
            }
            ;
            bcatcstr(metal, ");\n");
        }
        else
        {
            //UAV is not exist in either [[buffer]] or [[texture]]
            ASSERT(0);
        }
        break;
    }
    case OPCODE_LD_RAW:
    {
#ifdef _DEBUG
        AddIndentation(psContext);
        bcatcstr(metal, "//LD_RAW\n");
#endif

        METALTranslateShaderStorageLoad(psContext, psInst);
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
    case OPCODE_ATOMIC_UMAX:
    case OPCODE_ATOMIC_IMAX:
    case OPCODE_IMM_ATOMIC_IMAX:
    case OPCODE_IMM_ATOMIC_IMIN:
    case OPCODE_IMM_ATOMIC_UMAX:
    case OPCODE_IMM_ATOMIC_UMIN:
    case OPCODE_IMM_ATOMIC_OR:
    case OPCODE_IMM_ATOMIC_XOR:
    case OPCODE_IMM_ATOMIC_EXCH:
    case OPCODE_IMM_ATOMIC_CMP_EXCH:
    {
        TranslateAtomicMemOpMETAL(psContext, psInst);
        break;
    }
    case OPCODE_UBFE:
    case OPCODE_IBFE:
    {
#ifdef _DEBUG
        AddIndentation(psContext);
        if (psInst->eOpcode == OPCODE_UBFE)
        {
            bcatcstr(metal, "//OPCODE_UBFE\n");
        }
        else
        {
            bcatcstr(metal, "//OPCODE_IBFE\n");
        }
#endif
        // These instructions are not available in Metal shading language.
        // Need to expend it out (http://http.developer.nvidia.com/Cg/bitfieldExtract.html)
        // NOTE: we assume bitoffset is always > 0 as to avoid dynamic branching.
        // NOTE: We have taken out the -1 as this was breaking the GPU particles bitfields.

        int numComponents = psInst->asOperands[0].iNumComponents;

        AddIndentation(psContext);
        TranslateOperandMETAL(psContext, &psInst->asOperands[0], TO_FLAG_DESTINATION);
        bcatcstr(metal, " = 0;\n");

        AddIndentation(psContext);
        bcatcstr(metal, "{\n");

        AddIndentation(psContext);
        bformata(metal, "  %s mask = ~(%s(0xffffffff) << ", GetConstructorForTypeMETAL(SVT_UINT, numComponents), GetConstructorForTypeMETAL(SVT_UINT, numComponents));
        TranslateOperandMETAL(psContext, &psInst->asOperands[1], TO_FLAG_UNSIGNED_INTEGER);
        bcatcstr(metal, ");\n");

        AddIndentation(psContext);
        bcatcstr(metal, "  ");
        TranslateOperandMETAL(psContext, &psInst->asOperands[0], TO_FLAG_DESTINATION);
        bformata(metal, " = ( as_type<%s>((", GetConstructorForTypeMETAL(psInst->asOperands[0].aeDataType[0], numComponents));
        TranslateOperandMETAL(psContext, &psInst->asOperands[3], TO_FLAG_UNSIGNED_INTEGER);
        bcatcstr(metal, " >> ( ");
        TranslateOperandMETAL(psContext, &psInst->asOperands[2], TO_FLAG_UNSIGNED_INTEGER);
        bcatcstr(metal, ")) & mask) )");
        TranslateOperandSwizzleWithMaskMETAL(psContext, &psInst->asOperands[0], GetOperandWriteMaskMETAL(&psInst->asOperands[0]));
        bcatcstr(metal, ";\n");

        AddIndentation(psContext);
        bcatcstr(metal, "}\n");

        break;
    }
    case OPCODE_RCP:
    {
        const uint32_t destElemCount = GetNumSwizzleElementsMETAL(&psInst->asOperands[0]);
#ifdef _DEBUG
        AddIndentation(psContext);
        bcatcstr(metal, "//RCP\n");
#endif
        AddIndentation(psContext);
        TranslateOperandMETAL(psContext, &psInst->asOperands[0], TO_FLAG_DESTINATION);
        bcatcstr(metal, " = (float4(1.0) / float4(");
        TranslateOperandMETAL(psContext, &psInst->asOperands[1], TO_FLAG_NONE);
        bcatcstr(metal, "))");
        AddSwizzleUsingElementCountMETAL(psContext, destElemCount);
        bcatcstr(metal, ";\n");
        break;
    }
    case OPCODE_F32TOF16:
    {
        const uint32_t destElemCount = GetNumSwizzleElementsMETAL(&psInst->asOperands[0]);
        const uint32_t s0ElemCount = GetNumSwizzleElementsMETAL(&psInst->asOperands[1]);
        uint32_t destElem;
#ifdef _DEBUG
        AddIndentation(psContext);
        bcatcstr(metal, "//F32TOF16\n");
#endif
        for (destElem = 0; destElem < destElemCount; ++destElem)
        {
            const char* swizzle[] = { ".x", ".y", ".z", ".w" };

            AddIndentation(psContext);
            TranslateOperandMETAL(psContext, &psInst->asOperands[0], TO_FLAG_DESTINATION);
            if (destElemCount > 1)
            {
                bcatcstr(metal, swizzle[destElem]);
            }

            bcatcstr(metal, " = ");

            SHADER_VARIABLE_TYPE eDestDataType = GetOperandDataTypeMETAL(psContext, &psInst->asOperands[0]);
            if (SVT_FLOAT == eDestDataType)
            {
                bcatcstr(metal, "as_type<float>");
            }
            bcatcstr(metal, "( (uint( as_type<unsigned short>( (half)");
            TranslateOperandMETAL(psContext, &psInst->asOperands[1], TO_FLAG_NONE);
            if (s0ElemCount > 1)
            {
                bcatcstr(metal, swizzle[destElem]);
            }
            bcatcstr(metal, " ) ) ) );\n");
        }
        break;
    }
    case OPCODE_F16TOF32:
    {
        const uint32_t destElemCount = GetNumSwizzleElementsMETAL(&psInst->asOperands[0]);
        const uint32_t s0ElemCount = GetNumSwizzleElementsMETAL(&psInst->asOperands[1]);
        uint32_t destElem;
#ifdef _DEBUG
        AddIndentation(psContext);
        bcatcstr(metal, "//F16TOF32\n");
#endif
        for (destElem = 0; destElem < destElemCount; ++destElem)
        {
            const char* swizzle[] = { ".x", ".y", ".z", ".w" };

            AddIndentation(psContext);
            TranslateOperandMETAL(psContext, &psInst->asOperands[0], TO_FLAG_DESTINATION | TO_FLAG_UNSIGNED_INTEGER);
            if (destElemCount > 1)
            {
                bcatcstr(metal, swizzle[destElem]);
            }

            bcatcstr(metal, " = as_type<half> ((unsigned short)");
            TranslateOperandMETAL(psContext, &psInst->asOperands[1], TO_FLAG_UNSIGNED_INTEGER);
            if (s0ElemCount > 1)
            {
                bcatcstr(metal, swizzle[destElem]);
            }
            bcatcstr(metal, ");\n");
        }
        break;
    }
    case OPCODE_INEG:
    {
#ifdef _DEBUG
        AddIndentation(psContext);
        bcatcstr(metal, "//INEG\n");
#endif
        uint32_t dstCount = GetNumSwizzleElementsMETAL(&psInst->asOperands[0]);
        uint32_t srcCount = GetNumSwizzleElementsMETAL(&psInst->asOperands[1]);

        //dest = 0 - src0
        bcatcstr(metal, "-(");
        TranslateOperandMETAL(psContext, &psInst->asOperands[1], TO_FLAG_NONE | TO_FLAG_INTEGER);
        if (srcCount > dstCount)
        {
            AddSwizzleUsingElementCountMETAL(psContext, dstCount);
        }
        bcatcstr(metal, ")");
        bcatcstr(metal, ";\n");
        break;
    }
    case OPCODE_DERIV_RTX_COARSE:
    case OPCODE_DERIV_RTX_FINE:
    case OPCODE_DERIV_RTX:
    {
#ifdef _DEBUG
        AddIndentation(psContext);
        bcatcstr(metal, "//DERIV_RTX\n");
#endif
        METALCallHelper1(psContext, "dfdx", psInst, 0, 1, 1);
        break;
    }
    case OPCODE_DERIV_RTY_COARSE:
    case OPCODE_DERIV_RTY_FINE:
    case OPCODE_DERIV_RTY:
    {
#ifdef _DEBUG
        AddIndentation(psContext);
        bcatcstr(metal, "//DERIV_RTY\n");
#endif
        METALCallHelper1(psContext, "dfdy", psInst, 0, 1, 1);
        break;
    }
    case OPCODE_LRP:
    {
#ifdef _DEBUG
        AddIndentation(psContext);
        bcatcstr(metal, "//LRP\n");
#endif
        METALCallHelper3(psContext, "mix", psInst, 0, 2, 3, 1, 1);
        break;
    }
    case OPCODE_DP2ADD:
    {
#ifdef _DEBUG
        AddIndentation(psContext);
        bcatcstr(metal, "//DP2ADD\n");
#endif
        AddIndentation(psContext);
        TranslateOperandMETAL(psContext, &psInst->asOperands[0], TO_FLAG_DESTINATION);
        bcatcstr(metal, " = dot(float2(");
        TranslateOperandMETAL(psContext, &psInst->asOperands[1], TO_FLAG_NONE);
        bcatcstr(metal, "), float2(");
        TranslateOperandMETAL(psContext, &psInst->asOperands[2], TO_FLAG_NONE);
        bcatcstr(metal, ")) + ");
        TranslateOperandMETAL(psContext, &psInst->asOperands[3], TO_FLAG_NONE);
        bcatcstr(metal, ";\n");
        break;
    }
    case OPCODE_POW:
    {
#ifdef _DEBUG
        AddIndentation(psContext);
        bcatcstr(metal, "//POW\n");
#endif
        AddIndentation(psContext);
        TranslateOperandMETAL(psContext, &psInst->asOperands[0], TO_FLAG_DESTINATION);
        bcatcstr(metal, " = pow(abs(");
        TranslateOperandMETAL(psContext, &psInst->asOperands[1], TO_FLAG_NONE);
        bcatcstr(metal, "), ");
        TranslateOperandMETAL(psContext, &psInst->asOperands[2], TO_FLAG_NONE);
        bcatcstr(metal, ");\n");
        break;
    }

    case OPCODE_IMM_ATOMIC_ALLOC:
    case OPCODE_IMM_ATOMIC_CONSUME:
    {
        // not implemented in metal
        ASSERT(0);
        break;
    }

    case OPCODE_NOT:
    {
#ifdef _DEBUG
        AddIndentation(psContext);
        bcatcstr(metal, "//INOT\n");
#endif
        AddIndentation(psContext);
        METALAddAssignToDest(psContext, &psInst->asOperands[0], SVT_INT, GetNumSwizzleElementsMETAL(&psInst->asOperands[1]), &numParenthesis);

        bcatcstr(metal, "~");
        TranslateOperandWithMaskMETAL(psContext, &psInst->asOperands[1], TO_FLAG_INTEGER, GetOperandWriteMaskMETAL(&psInst->asOperands[0]));
        METALAddAssignPrologue(psContext, numParenthesis);
        break;
    }
    case OPCODE_XOR:
    {
#ifdef _DEBUG
        AddIndentation(psContext);
        bcatcstr(metal, "//XOR\n");
#endif

        METALCallBinaryOp(psContext, "^", psInst, 0, 1, 2, SVT_UINT);
        break;
    }
    case OPCODE_RESINFO:
    {
        const RESOURCE_DIMENSION eResDim = psContext->psShader->aeResourceDims[psInst->asOperands[2].ui32RegisterNumber];
        const RESINFO_RETURN_TYPE eResInfoReturnType = psInst->eResInfoReturnType;
        uint32_t destElemCount = GetNumSwizzleElementsMETAL(&psInst->asOperands[0]);
        uint32_t destElem;
#ifdef _DEBUG
        AddIndentation(psContext);
        bcatcstr(metal, "//RESINFO\n");
#endif

        for (destElem = 0; destElem < destElemCount; ++destElem)
        {
            const char* swizzle[] = { ".x", ".y", ".z", ".w" };

            GetResInfoDataMETAL(psContext, psInst, psInst->asOperands[2].aui32Swizzle[destElem], destElem);
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

    if (psInst->bSaturate) //Saturate is only for floating point data (float opcodes or MOV)
    {
        int dstCount = GetNumSwizzleElementsMETAL(&psInst->asOperands[0]);
        AddIndentation(psContext);
        METALAddAssignToDest(psContext, &psInst->asOperands[0], SVT_FLOAT, dstCount, &numParenthesis);
        bcatcstr(metal, "clamp(");

        TranslateOperandMETAL(psContext, &psInst->asOperands[0], TO_AUTO_BITCAST_TO_FLOAT);
        bcatcstr(metal, ", 0.0, 1.0)");
        METALAddAssignPrologue(psContext, numParenthesis);
    }
}

static int METALIsIntegerImmediateOpcode(OPCODE_TYPE eOpcode)
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
    //MOV is typeless.
    //Treat immediates as int, bitcast to float if necessary
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

int InstructionUsesRegisterMETAL(const Instruction* psInst, const Operand* psOperand)
{
    uint32_t operand;
    for (operand = 0; operand < psInst->ui32NumOperands; ++operand)
    {
        if (psInst->asOperands[operand].eType == psOperand->eType)
        {
            if (psInst->asOperands[operand].ui32RegisterNumber == psOperand->ui32RegisterNumber)
            {
                if (CompareOperandSwizzlesMETAL(&psInst->asOperands[operand], psOperand))
                {
                    return 1;
                }
            }
        }
    }
    return 0;
}

void MarkIntegerImmediatesMETAL(HLSLCrossCompilerContext* psContext)
{
    const uint32_t count = psContext->psShader->asPhase[MAIN_PHASE].pui32InstCount[0];
    Instruction* psInst = psContext->psShader->asPhase[MAIN_PHASE].ppsInst[0];
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
                if (InstructionUsesRegisterMETAL(&psInst[k], &psInst[i].asOperands[0]))
                {
                    if (METALIsIntegerImmediateOpcode(psInst[k].eOpcode))
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
