// Modifications copyright Amazon.com, Inc. or its affiliates
// Modifications copyright Crytek GmbH

#include "internal_includes/decode.h"
#include "internal_includes/debug.h"
#include "internal_includes/hlslcc_malloc.h"
#include "internal_includes/reflect.h"
#include "internal_includes/structs.h"
#include "internal_includes/tokens.h"
#include "stdio.h"
#include "stdlib.h"

enum
{
    FOURCC_DXBC = FOURCC('D', 'X', 'B', 'C')
};  // DirectX byte code
enum
{
    FOURCC_SHDR = FOURCC('S', 'H', 'D', 'R')
};  // Shader model 4 code
enum
{
    FOURCC_SHEX = FOURCC('S', 'H', 'E', 'X')
};  // Shader model 5 code
enum
{
    FOURCC_RDEF = FOURCC('R', 'D', 'E', 'F')
};  // Resource definition (e.g. constant buffers)
enum
{
    FOURCC_ISGN = FOURCC('I', 'S', 'G', 'N')
};  // Input signature
enum
{
    FOURCC_IFCE = FOURCC('I', 'F', 'C', 'E')
};  // Interface (for dynamic linking)
enum
{
    FOURCC_OSGN = FOURCC('O', 'S', 'G', 'N')
};  // Output signature

enum
{
    FOURCC_ISG1 = FOURCC('I', 'S', 'G', '1')
};  // Input signature with Stream and MinPrecision
enum
{
    FOURCC_OSG1 = FOURCC('O', 'S', 'G', '1')
};  // Output signature with Stream and MinPrecision
enum
{
    FOURCC_OSG5 = FOURCC('O', 'S', 'G', '5')
};  // Output signature with Stream

typedef struct DXBCContainerHeaderTAG
{
    unsigned fourcc;
    uint32_t unk[4];
    uint32_t one;
    uint32_t totalSize;
    uint32_t chunkCount;
} DXBCContainerHeader;

typedef struct DXBCChunkHeaderTAG
{
    unsigned fourcc;
    unsigned size;
} DXBCChunkHeader;

#ifdef _DEBUG
static uint64_t operandID = 0;
static uint64_t instructionID = 0;
#endif

#if defined(_WIN32)
#define osSprintf(dest, size, src) sprintf_s(dest, size, src)
#else
#define osSprintf(dest, size, src) sprintf(dest, src)
#endif

void DecodeNameToken(const uint32_t* pui32NameToken, Operand* psOperand)
{
    const size_t MAX_BUFFER_SIZE = sizeof(psOperand->pszSpecialName);
    psOperand->eSpecialName = DecodeOperandSpecialName(*pui32NameToken);
    switch (psOperand->eSpecialName)
    {
        case NAME_UNDEFINED:
        {
            osSprintf(psOperand->pszSpecialName, MAX_BUFFER_SIZE, "undefined");
            break;
        }
        case NAME_POSITION:
        {
            osSprintf(psOperand->pszSpecialName, MAX_BUFFER_SIZE, "position");
            break;
        }
        case NAME_CLIP_DISTANCE:
        {
            osSprintf(psOperand->pszSpecialName, MAX_BUFFER_SIZE, "clipDistance");
            break;
        }
        case NAME_CULL_DISTANCE:
        {
            osSprintf(psOperand->pszSpecialName, MAX_BUFFER_SIZE, "cullDistance");
            break;
        }
        case NAME_RENDER_TARGET_ARRAY_INDEX:
        {
            osSprintf(psOperand->pszSpecialName, MAX_BUFFER_SIZE, "renderTargetArrayIndex");
            break;
        }
        case NAME_VIEWPORT_ARRAY_INDEX:
        {
            osSprintf(psOperand->pszSpecialName, MAX_BUFFER_SIZE, "viewportArrayIndex");
            break;
        }
        case NAME_VERTEX_ID:
        {
            osSprintf(psOperand->pszSpecialName, MAX_BUFFER_SIZE, "vertexID");
            break;
        }
        case NAME_PRIMITIVE_ID:
        {
            osSprintf(psOperand->pszSpecialName, MAX_BUFFER_SIZE, "primitiveID");
            break;
        }
        case NAME_INSTANCE_ID:
        {
            osSprintf(psOperand->pszSpecialName, MAX_BUFFER_SIZE, "instanceID");
            break;
        }
        case NAME_IS_FRONT_FACE:
        {
            osSprintf(psOperand->pszSpecialName, MAX_BUFFER_SIZE, "isFrontFace");
            break;
        }
        case NAME_SAMPLE_INDEX:
        {
            osSprintf(psOperand->pszSpecialName, MAX_BUFFER_SIZE, "sampleIndex");
            break;
        }
            // For the quadrilateral domain, there are 6 factors (4 sides, 2 inner).
        case NAME_FINAL_QUAD_U_EQ_0_EDGE_TESSFACTOR:
        case NAME_FINAL_QUAD_V_EQ_0_EDGE_TESSFACTOR:
        case NAME_FINAL_QUAD_U_EQ_1_EDGE_TESSFACTOR:
        case NAME_FINAL_QUAD_V_EQ_1_EDGE_TESSFACTOR:
        case NAME_FINAL_QUAD_U_INSIDE_TESSFACTOR:
        case NAME_FINAL_QUAD_V_INSIDE_TESSFACTOR:

            // For the triangular domain, there are 4 factors (3 sides, 1 inner)
        case NAME_FINAL_TRI_U_EQ_0_EDGE_TESSFACTOR:
        case NAME_FINAL_TRI_V_EQ_0_EDGE_TESSFACTOR:
        case NAME_FINAL_TRI_W_EQ_0_EDGE_TESSFACTOR:
        case NAME_FINAL_TRI_INSIDE_TESSFACTOR:

            // For the isoline domain, there are 2 factors (detail and density).
        case NAME_FINAL_LINE_DETAIL_TESSFACTOR:
        case NAME_FINAL_LINE_DENSITY_TESSFACTOR:
        {
            osSprintf(psOperand->pszSpecialName, MAX_BUFFER_SIZE, "tessFactor");
            break;
        }
        default:
        {
            ASSERT(0);
            break;
        }
    }

    return;
}

uint32_t DecodeOperand(const uint32_t* pui32Tokens, Operand* psOperand)
{
    int i;
    uint32_t ui32NumTokens = 1;
    OPERAND_NUM_COMPONENTS eNumComponents;

#ifdef _DEBUG
    psOperand->id = operandID++;
#endif

    // Some defaults
    psOperand->iWriteMaskEnabled = 1;
    psOperand->iGSInput = 0;
    psOperand->aeDataType[0] = SVT_FLOAT;
    psOperand->aeDataType[1] = SVT_FLOAT;
    psOperand->aeDataType[2] = SVT_FLOAT;
    psOperand->aeDataType[3] = SVT_FLOAT;

    psOperand->iExtended = DecodeIsOperandExtended(*pui32Tokens);

    psOperand->eModifier = OPERAND_MODIFIER_NONE;
    psOperand->psSubOperand[0] = 0;
    psOperand->psSubOperand[1] = 0;
    psOperand->psSubOperand[2] = 0;

    psOperand->eMinPrecision = OPERAND_MIN_PRECISION_DEFAULT;

    /* Check if this instruction is extended.  If it is,
     * we need to print the information first */
    if (psOperand->iExtended)
    {
        /* OperandToken1 is the second token */
        ui32NumTokens++;

        if (DecodeExtendedOperandType(pui32Tokens[1]) == EXTENDED_OPERAND_MODIFIER)
        {
            psOperand->eModifier = DecodeExtendedOperandModifier(pui32Tokens[1]);
            psOperand->eMinPrecision = DecodeOperandMinPrecision(pui32Tokens[1]);
        }
    }

    psOperand->iIndexDims = DecodeOperandIndexDimension(*pui32Tokens);
    psOperand->eType = DecodeOperandType(*pui32Tokens);

    psOperand->ui32RegisterNumber = 0;

    eNumComponents = DecodeOperandNumComponents(*pui32Tokens);

    switch (eNumComponents)
    {
        case OPERAND_1_COMPONENT:
        {
            psOperand->iNumComponents = 1;
            break;
        }
        case OPERAND_4_COMPONENT:
        {
            psOperand->iNumComponents = 4;
            break;
        }
        default:
        {
            psOperand->iNumComponents = 0;
            break;
        }
    }

    if (psOperand->iWriteMaskEnabled && psOperand->iNumComponents == 4)
    {
        psOperand->eSelMode = DecodeOperand4CompSelMode(*pui32Tokens);

        if (psOperand->eSelMode == OPERAND_4_COMPONENT_MASK_MODE)
        {
            psOperand->ui32CompMask = DecodeOperand4CompMask(*pui32Tokens);
        }
        else if (psOperand->eSelMode == OPERAND_4_COMPONENT_SWIZZLE_MODE)
        {
            psOperand->ui32Swizzle = DecodeOperand4CompSwizzle(*pui32Tokens);

            if (psOperand->ui32Swizzle != NO_SWIZZLE)
            {
                psOperand->aui32Swizzle[0] = DecodeOperand4CompSwizzleSource(*pui32Tokens, 0);
                psOperand->aui32Swizzle[1] = DecodeOperand4CompSwizzleSource(*pui32Tokens, 1);
                psOperand->aui32Swizzle[2] = DecodeOperand4CompSwizzleSource(*pui32Tokens, 2);
                psOperand->aui32Swizzle[3] = DecodeOperand4CompSwizzleSource(*pui32Tokens, 3);
            }
            else
            {
                psOperand->aui32Swizzle[0] = OPERAND_4_COMPONENT_X;
                psOperand->aui32Swizzle[1] = OPERAND_4_COMPONENT_Y;
                psOperand->aui32Swizzle[2] = OPERAND_4_COMPONENT_Z;
                psOperand->aui32Swizzle[3] = OPERAND_4_COMPONENT_W;
            }
        }
        else if (psOperand->eSelMode == OPERAND_4_COMPONENT_SELECT_1_MODE)
        {
            psOperand->aui32Swizzle[0] = DecodeOperand4CompSel1(*pui32Tokens);
        }
    }

    // Set externally to this function based on the instruction opcode.
    psOperand->iIntegerImmediate = 0;

    if (psOperand->eType == OPERAND_TYPE_IMMEDIATE32)
    {
        for (i = 0; i < psOperand->iNumComponents; ++i)
        {
            psOperand->afImmediates[i] = *((float*)(&pui32Tokens[ui32NumTokens]));
            ui32NumTokens++;
        }
    }
    else if (psOperand->eType == OPERAND_TYPE_IMMEDIATE64)
    {
        for (i = 0; i < psOperand->iNumComponents; ++i)
        {
            psOperand->adImmediates[i] = *((double*)(&pui32Tokens[ui32NumTokens]));
            ui32NumTokens += 2;
        }
    }

    if (psOperand->eType == OPERAND_TYPE_OUTPUT_DEPTH_GREATER_EQUAL || psOperand->eType == OPERAND_TYPE_OUTPUT_DEPTH_LESS_EQUAL ||
        psOperand->eType == OPERAND_TYPE_OUTPUT_DEPTH)
    {
        psOperand->ui32RegisterNumber = -1;
        psOperand->ui32CompMask = -1;
    }

    for (i = 0; i < psOperand->iIndexDims; ++i)
    {
        OPERAND_INDEX_REPRESENTATION eRep = DecodeOperandIndexRepresentation(i, *pui32Tokens);

        psOperand->eIndexRep[i] = eRep;

        psOperand->aui32ArraySizes[i] = 0;
        psOperand->ui32RegisterNumber = 0;

        switch (eRep)
        {
            case OPERAND_INDEX_IMMEDIATE32:
            {
                psOperand->ui32RegisterNumber = *(pui32Tokens + ui32NumTokens);
                psOperand->aui32ArraySizes[i] = psOperand->ui32RegisterNumber;
                break;
            }
            case OPERAND_INDEX_RELATIVE:
            {
                psOperand->psSubOperand[i] = hlslcc_malloc(sizeof(Operand));
                DecodeOperand(pui32Tokens + ui32NumTokens, psOperand->psSubOperand[i]);

                ui32NumTokens++;
                break;
            }
            case OPERAND_INDEX_IMMEDIATE32_PLUS_RELATIVE:
            {
                psOperand->ui32RegisterNumber = *(pui32Tokens + ui32NumTokens);
                psOperand->aui32ArraySizes[i] = psOperand->ui32RegisterNumber;

                ui32NumTokens++;

                psOperand->psSubOperand[i] = hlslcc_malloc(sizeof(Operand));
                DecodeOperand(pui32Tokens + ui32NumTokens, psOperand->psSubOperand[i]);

                ui32NumTokens++;
                break;
            }
            default:
            {
                ASSERT(0);
                break;
            }
        }

        ui32NumTokens++;
    }

    psOperand->pszSpecialName[0] = '\0';

    return ui32NumTokens;
}

const uint32_t* DecodeDeclaration(Shader* psShader, const uint32_t* pui32Token, Declaration* psDecl)
{
    uint32_t ui32TokenLength = DecodeInstructionLength(*pui32Token);
    const uint32_t bExtended = DecodeIsOpcodeExtended(*pui32Token);
    const OPCODE_TYPE eOpcode = DecodeOpcodeType(*pui32Token);
    uint32_t ui32OperandOffset = 1;

    if (eOpcode < NUM_OPCODES && eOpcode >= 0)
    {
        psShader->aiOpcodeUsed[eOpcode] = 1;
    }

    psDecl->eOpcode = eOpcode;

    psDecl->ui32TexReturnType = SVT_FLOAT;

    if (bExtended)
    {
        ui32OperandOffset = 2;
    }

    switch (eOpcode)
    {
        case OPCODE_DCL_RESOURCE:  // DCL* opcodes have
        {
            ResourceBinding* psBinding = 0;
            psDecl->value.eResourceDimension = DecodeResourceDimension(*pui32Token);
            psDecl->ui32NumOperands = 1;
            DecodeOperand(pui32Token + ui32OperandOffset, &psDecl->asOperands[0]);

            if (psDecl->asOperands[0].eType == OPERAND_TYPE_RESOURCE &&
                GetResourceFromBindingPoint(RGROUP_TEXTURE, psDecl->asOperands[0].ui32RegisterNumber, &psShader->sInfo, &psBinding))
            {
                psDecl->ui32TexReturnType = psBinding->ui32ReturnType;
            }
            break;
        }
        case OPCODE_DCL_CONSTANT_BUFFER:  // custom operand formats.
        {
            psDecl->value.eCBAccessPattern = DecodeConstantBufferAccessPattern(*pui32Token);
            psDecl->ui32NumOperands = 1;
            DecodeOperand(pui32Token + ui32OperandOffset, &psDecl->asOperands[0]);
            break;
        }
        case OPCODE_DCL_SAMPLER:
        {
            break;
        }
        case OPCODE_DCL_INDEX_RANGE:
        {
            psDecl->ui32NumOperands = 1;
            ui32OperandOffset += DecodeOperand(pui32Token + ui32OperandOffset, &psDecl->asOperands[0]);
            psDecl->value.ui32IndexRange = pui32Token[ui32OperandOffset];

            if (psDecl->asOperands[0].eType == OPERAND_TYPE_INPUT)
            {
                uint32_t i;
                const uint32_t indexRange = psDecl->value.ui32IndexRange;
                const uint32_t reg = psDecl->asOperands[0].ui32RegisterNumber;

                psShader->aIndexedInput[reg] = indexRange;
                psShader->aIndexedInputParents[reg] = reg;

                //-1 means don't declare this input because it falls in
                // the range of an already declared array.
                for (i = reg + 1; i < reg + indexRange; ++i)
                {
                    psShader->aIndexedInput[i] = -1;
                    psShader->aIndexedInputParents[i] = reg;
                }
            }

            if (psDecl->asOperands[0].eType == OPERAND_TYPE_OUTPUT)
            {
                psShader->aIndexedOutput[psDecl->asOperands[0].ui32RegisterNumber] = psDecl->value.ui32IndexRange;
            }
            break;
        }
        case OPCODE_DCL_GS_OUTPUT_PRIMITIVE_TOPOLOGY:
        {
            psDecl->value.eOutputPrimitiveTopology = DecodeGSOutputPrimitiveTopology(*pui32Token);
            break;
        }
        case OPCODE_DCL_GS_INPUT_PRIMITIVE:
        {
            psDecl->value.eInputPrimitive = DecodeGSInputPrimitive(*pui32Token);
            break;
        }
        case OPCODE_DCL_MAX_OUTPUT_VERTEX_COUNT:
        {
            psDecl->value.ui32MaxOutputVertexCount = pui32Token[1];
            break;
        }
        case OPCODE_DCL_TESS_PARTITIONING:
        {
            psDecl->value.eTessPartitioning = DecodeTessPartitioning(*pui32Token);
            break;
        }
        case OPCODE_DCL_TESS_DOMAIN:
        {
            psDecl->value.eTessDomain = DecodeTessDomain(*pui32Token);
            break;
        }
        case OPCODE_DCL_TESS_OUTPUT_PRIMITIVE:
        {
            psDecl->value.eTessOutPrim = DecodeTessOutPrim(*pui32Token);
            break;
        }
        case OPCODE_DCL_THREAD_GROUP:
        {
            psDecl->value.aui32WorkGroupSize[0] = pui32Token[1];
            psDecl->value.aui32WorkGroupSize[1] = pui32Token[2];
            psDecl->value.aui32WorkGroupSize[2] = pui32Token[3];
            break;
        }
        case OPCODE_DCL_INPUT:
        {
            psDecl->ui32NumOperands = 1;
            DecodeOperand(pui32Token + ui32OperandOffset, &psDecl->asOperands[0]);
            break;
        }
        case OPCODE_DCL_INPUT_SIV:
        {
            psDecl->ui32NumOperands = 1;
            DecodeOperand(pui32Token + ui32OperandOffset, &psDecl->asOperands[0]);
            if (psShader->eShaderType == PIXEL_SHADER)
            {
                psDecl->value.eInterpolation = DecodeInterpolationMode(*pui32Token);
            }
            break;
        }
        case OPCODE_DCL_INPUT_PS:
        {
            psDecl->ui32NumOperands = 1;
            psDecl->value.eInterpolation = DecodeInterpolationMode(*pui32Token);
            DecodeOperand(pui32Token + ui32OperandOffset, &psDecl->asOperands[0]);
            break;
        }
        case OPCODE_DCL_INPUT_SGV:
        case OPCODE_DCL_INPUT_PS_SGV:
        {
            psDecl->ui32NumOperands = 1;
            DecodeOperand(pui32Token + ui32OperandOffset, &psDecl->asOperands[0]);
            DecodeNameToken(pui32Token + 3, &psDecl->asOperands[0]);
            break;
        }
        case OPCODE_DCL_INPUT_PS_SIV:
        {
            psDecl->ui32NumOperands = 1;
            psDecl->value.eInterpolation = DecodeInterpolationMode(*pui32Token);
            DecodeOperand(pui32Token + ui32OperandOffset, &psDecl->asOperands[0]);
            DecodeNameToken(pui32Token + 3, &psDecl->asOperands[0]);
            break;
        }
        case OPCODE_DCL_OUTPUT:
        {
            psDecl->ui32NumOperands = 1;
            DecodeOperand(pui32Token + ui32OperandOffset, &psDecl->asOperands[0]);
            break;
        }
        case OPCODE_DCL_OUTPUT_SGV:
        {
            break;
        }
        case OPCODE_DCL_OUTPUT_SIV:
        {
            psDecl->ui32NumOperands = 1;
            DecodeOperand(pui32Token + ui32OperandOffset, &psDecl->asOperands[0]);
            DecodeNameToken(pui32Token + 3, &psDecl->asOperands[0]);
            break;
        }
        case OPCODE_DCL_TEMPS:
        {
            psDecl->value.ui32NumTemps = *(pui32Token + ui32OperandOffset);
            break;
        }
        case OPCODE_DCL_INDEXABLE_TEMP:
        {
            psDecl->sIdxTemp.ui32RegIndex = *(pui32Token + ui32OperandOffset);
            psDecl->sIdxTemp.ui32RegCount = *(pui32Token + ui32OperandOffset + 1);
            psDecl->sIdxTemp.ui32RegComponentSize = *(pui32Token + ui32OperandOffset + 2);
            break;
        }
        case OPCODE_DCL_GLOBAL_FLAGS:
        {
            psDecl->value.ui32GlobalFlags = DecodeGlobalFlags(*pui32Token);
            break;
        }
        case OPCODE_DCL_INTERFACE:
        {
            uint32_t func = 0, numClassesImplementingThisInterface, arrayLen, interfaceID;
            interfaceID = pui32Token[ui32OperandOffset];
            ui32OperandOffset++;
            psDecl->ui32TableLength = pui32Token[ui32OperandOffset];
            ui32OperandOffset++;

            numClassesImplementingThisInterface = DecodeInterfaceTableLength(*(pui32Token + ui32OperandOffset));
            arrayLen = DecodeInterfaceArrayLength(*(pui32Token + ui32OperandOffset));

            ui32OperandOffset++;

            psDecl->value.interface.ui32InterfaceID = interfaceID;
            psDecl->value.interface.ui32NumFuncTables = numClassesImplementingThisInterface;
            psDecl->value.interface.ui32ArraySize = arrayLen;

            psShader->funcPointer[interfaceID].ui32NumBodiesPerTable = psDecl->ui32TableLength;

            for (; func < numClassesImplementingThisInterface; ++func)
            {
                uint32_t ui32FuncTable = *(pui32Token + ui32OperandOffset);
                psShader->aui32FuncTableToFuncPointer[ui32FuncTable] = interfaceID;

                psShader->funcPointer[interfaceID].aui32FuncTables[func] = ui32FuncTable;
                ui32OperandOffset++;
            }

            break;
        }
        case OPCODE_DCL_FUNCTION_BODY:
        {
            psDecl->ui32NumOperands = 1;
            DecodeOperand(pui32Token + ui32OperandOffset, &psDecl->asOperands[0]);
            break;
        }
        case OPCODE_DCL_FUNCTION_TABLE:
        {
            uint32_t ui32Func;
            const uint32_t ui32FuncTableID = pui32Token[ui32OperandOffset++];
            const uint32_t ui32NumFuncsInTable = pui32Token[ui32OperandOffset++];

            for (ui32Func = 0; ui32Func < ui32NumFuncsInTable; ++ui32Func)
            {
                const uint32_t ui32FuncBodyID = pui32Token[ui32OperandOffset++];

                psShader->aui32FuncBodyToFuncTable[ui32FuncBodyID] = ui32FuncTableID;

                psShader->funcTable[ui32FuncTableID].aui32FuncBodies[ui32Func] = ui32FuncBodyID;
            }

            // OpcodeToken0 is followed by a DWORD that represents the function table
            // identifier and another DWORD (TableLength) that gives the number of
            // functions in the table.
            //
            // This is followed by TableLength DWORDs which are function body indices.
            //

            break;
        }
        case OPCODE_DCL_INPUT_CONTROL_POINT_COUNT:
        {
            break;
        }
        case OPCODE_HS_DECLS:
        {
            break;
        }
        case OPCODE_DCL_OUTPUT_CONTROL_POINT_COUNT:
        {
            psDecl->value.ui32MaxOutputVertexCount = DecodeOutputControlPointCount(*pui32Token);
            break;
        }
        case OPCODE_HS_JOIN_PHASE:
        case OPCODE_HS_FORK_PHASE:
        case OPCODE_HS_CONTROL_POINT_PHASE:
        {
            break;
        }
        case OPCODE_DCL_HS_FORK_PHASE_INSTANCE_COUNT:
        {
            ASSERT(psShader->ui32ForkPhaseCount != 0);  // Check for wrapping when we decrement.
            psDecl->value.aui32HullPhaseInstanceInfo[0] = psShader->ui32ForkPhaseCount - 1;
            psDecl->value.aui32HullPhaseInstanceInfo[1] = pui32Token[1];
            break;
        }
        case OPCODE_CUSTOMDATA:
        {
            ui32TokenLength = pui32Token[1];
            {
                const uint32_t ui32NumVec4 = (ui32TokenLength - 2) / 4;
                uint32_t uIdx = 0;

                ICBVec4 const* pVec4Array = (void*)(pui32Token + 2);

                // The buffer will contain at least one value, but not more than 4096 scalars/1024 vec4's.
                ASSERT(ui32NumVec4 < MAX_IMMEDIATE_CONST_BUFFER_VEC4_SIZE);

                /* must be a multiple of 4 */
                ASSERT(((ui32TokenLength - 2) % 4) == 0);

                for (uIdx = 0; uIdx < ui32NumVec4; uIdx++)
                {
                    psDecl->asImmediateConstBuffer[uIdx] = pVec4Array[uIdx];
                }

                psDecl->ui32NumOperands = ui32NumVec4;
            }
            break;
        }
        case OPCODE_DCL_HS_MAX_TESSFACTOR:
        {
            psDecl->value.fMaxTessFactor = *((float*)&pui32Token[1]);
            break;
        }
        case OPCODE_DCL_UNORDERED_ACCESS_VIEW_TYPED:
        {
            psDecl->ui32NumOperands = 2;
            psDecl->value.eResourceDimension = DecodeResourceDimension(*pui32Token);
            psDecl->sUAV.ui32GloballyCoherentAccess = DecodeAccessCoherencyFlags(*pui32Token);
            psDecl->sUAV.bCounter = 0;
            psDecl->sUAV.ui32BufferSize = 0;
            ui32OperandOffset += DecodeOperand(pui32Token + ui32OperandOffset, &psDecl->asOperands[0]);
            psDecl->sUAV.Type = DecodeResourceReturnType(0, pui32Token[ui32OperandOffset]);
            break;
        }
        case OPCODE_DCL_UNORDERED_ACCESS_VIEW_RAW:
        {

            psDecl->ui32NumOperands = 1;
            psDecl->sUAV.ui32GloballyCoherentAccess = DecodeAccessCoherencyFlags(*pui32Token);
            psDecl->sUAV.bCounter = 0;
            psDecl->sUAV.ui32BufferSize = 0;
            DecodeOperand(pui32Token + ui32OperandOffset, &psDecl->asOperands[0]);
            // This should be a RTYPE_UAV_RWBYTEADDRESS buffer. It is memory backed by
            // a shader storage buffer whose is unknown at compile time.
            psDecl->sUAV.ui32BufferSize = 0;
            break;
        }
        case OPCODE_DCL_UNORDERED_ACCESS_VIEW_STRUCTURED:
        {
            ResourceBinding* psBinding = NULL;
            ConstantBuffer* psBuffer = NULL;

            psDecl->ui32NumOperands = 1;
            psDecl->sUAV.ui32GloballyCoherentAccess = DecodeAccessCoherencyFlags(*pui32Token);
            psDecl->sUAV.bCounter = 0;
            psDecl->sUAV.ui32BufferSize = 0;
            DecodeOperand(pui32Token + ui32OperandOffset, &psDecl->asOperands[0]);

            GetResourceFromBindingPoint(RGROUP_UAV, psDecl->asOperands[0].ui32RegisterNumber, &psShader->sInfo, &psBinding);

            GetConstantBufferFromBindingPoint(RGROUP_UAV, psBinding->ui32BindPoint, &psShader->sInfo, &psBuffer);
            psDecl->sUAV.ui32BufferSize = psBuffer->ui32TotalSizeInBytes;
            switch (psBinding->eType)
            {
                case RTYPE_UAV_RWSTRUCTURED_WITH_COUNTER:
                case RTYPE_UAV_APPEND_STRUCTURED:
                case RTYPE_UAV_CONSUME_STRUCTURED:
                    psDecl->sUAV.bCounter = 1;
                    break;
                default:
                    break;
            }
            break;
        }
        case OPCODE_DCL_RESOURCE_STRUCTURED:
        {
            psDecl->ui32NumOperands = 1;
            DecodeOperand(pui32Token + ui32OperandOffset, &psDecl->asOperands[0]);
            break;
        }
        case OPCODE_DCL_RESOURCE_RAW:
        {
            psDecl->ui32NumOperands = 1;
            DecodeOperand(pui32Token + ui32OperandOffset, &psDecl->asOperands[0]);
            break;
        }
        case OPCODE_DCL_THREAD_GROUP_SHARED_MEMORY_STRUCTURED:
        {

            psDecl->ui32NumOperands = 1;
            psDecl->sUAV.ui32GloballyCoherentAccess = 0;

            ui32OperandOffset += DecodeOperand(pui32Token + ui32OperandOffset, &psDecl->asOperands[0]);

            psDecl->sTGSM.ui32Stride = pui32Token[ui32OperandOffset++];
            psDecl->sTGSM.ui32Count = pui32Token[ui32OperandOffset++];
            break;
        }
        case OPCODE_DCL_THREAD_GROUP_SHARED_MEMORY_RAW:
        {

            psDecl->ui32NumOperands = 1;
            psDecl->sUAV.ui32GloballyCoherentAccess = 0;

            ui32OperandOffset += DecodeOperand(pui32Token + ui32OperandOffset, &psDecl->asOperands[0]);

            psDecl->sTGSM.ui32Stride = 4;
            psDecl->sTGSM.ui32Count = pui32Token[ui32OperandOffset++] / 4;
            break;
        }
        case OPCODE_DCL_STREAM:
        {
            psDecl->ui32NumOperands = 1;
            DecodeOperand(pui32Token + ui32OperandOffset, &psDecl->asOperands[0]);
            break;
        }
        case OPCODE_DCL_GS_INSTANCE_COUNT:
        {
            psDecl->ui32NumOperands = 0;
            psDecl->value.ui32GSInstanceCount = pui32Token[1];
            break;
        }
        default:
        {
            // Reached end of declarations
            return 0;
        }
    }

    UpdateDeclarationReferences(psShader, psDecl);

    return pui32Token + ui32TokenLength;
}

const uint32_t* DecodeInstruction(const uint32_t* pui32Token, Instruction* psInst, Shader* psShader)
{
    uint32_t ui32TokenLength = DecodeInstructionLength(*pui32Token);
    const uint32_t bExtended = DecodeIsOpcodeExtended(*pui32Token);
    const OPCODE_TYPE eOpcode = DecodeOpcodeType(*pui32Token);
    uint32_t ui32OperandOffset = 1;

#ifdef _DEBUG
    psInst->id = instructionID++;
#endif

    psInst->eOpcode = eOpcode;

    psInst->bSaturate = DecodeInstructionSaturate(*pui32Token);

    psInst->bAddressOffset = 0;

    psInst->ui32FirstSrc = 1;

    if (bExtended)
    {
        do
        {
            const uint32_t ui32ExtOpcodeToken = pui32Token[ui32OperandOffset];
            const EXTENDED_OPCODE_TYPE eExtType = DecodeExtendedOpcodeType(ui32ExtOpcodeToken);

            if (eExtType == EXTENDED_OPCODE_SAMPLE_CONTROLS)
            {
                psInst->bAddressOffset = 1;

                psInst->iUAddrOffset = DecodeImmediateAddressOffset(IMMEDIATE_ADDRESS_OFFSET_U, ui32ExtOpcodeToken);
                psInst->iVAddrOffset = DecodeImmediateAddressOffset(IMMEDIATE_ADDRESS_OFFSET_V, ui32ExtOpcodeToken);
                psInst->iWAddrOffset = DecodeImmediateAddressOffset(IMMEDIATE_ADDRESS_OFFSET_W, ui32ExtOpcodeToken);
            }
            else if (eExtType == EXTENDED_OPCODE_RESOURCE_RETURN_TYPE)
            {
                psInst->xType = DecodeExtendedResourceReturnType(0, ui32ExtOpcodeToken);
                psInst->yType = DecodeExtendedResourceReturnType(1, ui32ExtOpcodeToken);
                psInst->zType = DecodeExtendedResourceReturnType(2, ui32ExtOpcodeToken);
                psInst->wType = DecodeExtendedResourceReturnType(3, ui32ExtOpcodeToken);
            }
            else if (eExtType == EXTENDED_OPCODE_RESOURCE_DIM)
            {
                psInst->eResDim = DecodeExtendedResourceDimension(ui32ExtOpcodeToken);
            }

            ui32OperandOffset++;
        } while (DecodeIsOpcodeExtended(pui32Token[ui32OperandOffset - 1]));
    }

    if (eOpcode < NUM_OPCODES && eOpcode >= 0)
    {
        psShader->aiOpcodeUsed[eOpcode] = 1;
    }

    switch (eOpcode)
    {
            // no operands
        case OPCODE_CUT:
        case OPCODE_EMIT:
        case OPCODE_EMITTHENCUT:
        case OPCODE_RET:
        case OPCODE_LOOP:
        case OPCODE_ENDLOOP:
        case OPCODE_BREAK:
        case OPCODE_ELSE:
        case OPCODE_ENDIF:
        case OPCODE_CONTINUE:
        case OPCODE_DEFAULT:
        case OPCODE_ENDSWITCH:
        case OPCODE_NOP:
        case OPCODE_HS_CONTROL_POINT_PHASE:
        case OPCODE_HS_FORK_PHASE:
        case OPCODE_HS_JOIN_PHASE:
        {
            psInst->ui32NumOperands = 0;
            psInst->ui32FirstSrc = 0;
            break;
        }
        case OPCODE_DCL_HS_FORK_PHASE_INSTANCE_COUNT:
        {
            psInst->ui32NumOperands = 0;
            psInst->ui32FirstSrc = 0;
            break;
        }
        case OPCODE_SYNC:
        {
            psInst->ui32NumOperands = 0;
            psInst->ui32FirstSrc = 0;
            psInst->ui32SyncFlags = DecodeSyncFlags(*pui32Token);
            break;
        }

            // 1 operand
        case OPCODE_EMIT_STREAM:
        case OPCODE_CUT_STREAM:
        case OPCODE_EMITTHENCUT_STREAM:
        case OPCODE_CASE:
        case OPCODE_SWITCH:
        case OPCODE_LABEL:
        {
            psInst->ui32NumOperands = 1;
            psInst->ui32FirstSrc = 0;
            ui32OperandOffset += DecodeOperand(pui32Token + ui32OperandOffset, &psInst->asOperands[0]);

            //            if(eOpcode == OPCODE_CASE)
            //            {
            //                psInst->asOperands[0].iIntegerImmediate = 1;
            //            }
            break;
        }

        case OPCODE_INTERFACE_CALL:
        {
            psInst->ui32NumOperands = 1;
            psInst->ui32FirstSrc = 0;
            psInst->ui32FuncIndexWithinInterface = pui32Token[ui32OperandOffset];
            ui32OperandOffset++;
            ui32OperandOffset += DecodeOperand(pui32Token + ui32OperandOffset, &psInst->asOperands[0]);

            break;
        }

            /* Floating point instruction decodes */

            // Instructions with two operands go here
        case OPCODE_MOV:
        {
            psInst->ui32NumOperands = 2;
            ui32OperandOffset += DecodeOperand(pui32Token + ui32OperandOffset, &psInst->asOperands[0]);
            ui32OperandOffset += DecodeOperand(pui32Token + ui32OperandOffset, &psInst->asOperands[1]);

            // Mov with an integer dest. If src is an immediate then it must be encoded as an integer.
            if (psInst->asOperands[0].eMinPrecision == OPERAND_MIN_PRECISION_SINT_16 || psInst->asOperands[0].eMinPrecision == OPERAND_MIN_PRECISION_UINT_16)
            {
                psInst->asOperands[1].iIntegerImmediate = 1;
            }
            break;
        }
        case OPCODE_LOG:
        case OPCODE_RSQ:
        case OPCODE_EXP:
        case OPCODE_SQRT:
        case OPCODE_ROUND_PI:
        case OPCODE_ROUND_NI:
        case OPCODE_ROUND_Z:
        case OPCODE_ROUND_NE:
        case OPCODE_FRC:
        case OPCODE_FTOU:
        case OPCODE_FTOI:
        case OPCODE_UTOF:
        case OPCODE_ITOF:
        case OPCODE_INEG:
        case OPCODE_IMM_ATOMIC_ALLOC:
        case OPCODE_IMM_ATOMIC_CONSUME:
        case OPCODE_DMOV:
        case OPCODE_DTOF:
        case OPCODE_FTOD:
        case OPCODE_DRCP:
        case OPCODE_COUNTBITS:
        case OPCODE_FIRSTBIT_HI:
        case OPCODE_FIRSTBIT_LO:
        case OPCODE_FIRSTBIT_SHI:
        case OPCODE_BFREV:
        case OPCODE_F32TOF16:
        case OPCODE_F16TOF32:
        case OPCODE_RCP:
        case OPCODE_DERIV_RTX:
        case OPCODE_DERIV_RTY:
        case OPCODE_DERIV_RTX_COARSE:
        case OPCODE_DERIV_RTX_FINE:
        case OPCODE_DERIV_RTY_COARSE:
        case OPCODE_DERIV_RTY_FINE:
        case OPCODE_NOT:
        {
            psInst->ui32NumOperands = 2;
            ui32OperandOffset += DecodeOperand(pui32Token + ui32OperandOffset, &psInst->asOperands[0]);
            ui32OperandOffset += DecodeOperand(pui32Token + ui32OperandOffset, &psInst->asOperands[1]);
            break;
        }

            // Instructions with three operands go here
        case OPCODE_SINCOS:
        {
            psInst->ui32FirstSrc = 2;
            // Intentional fall-through
        }
        case OPCODE_IMIN:
        case OPCODE_UMIN:
        case OPCODE_MIN:
        case OPCODE_IMAX:
        case OPCODE_UMAX:
        case OPCODE_MAX:
        case OPCODE_MUL:
        case OPCODE_DIV:
        case OPCODE_ADD:
        case OPCODE_DP2:
        case OPCODE_DP3:
        case OPCODE_DP4:
        case OPCODE_NE:
        case OPCODE_OR:
        case OPCODE_XOR:
        case OPCODE_LT:
        case OPCODE_IEQ:
        case OPCODE_IADD:
        case OPCODE_AND:
        case OPCODE_GE:
        case OPCODE_IGE:
        case OPCODE_EQ:
        case OPCODE_ISHL:
        case OPCODE_ISHR:
        case OPCODE_LD:
        case OPCODE_ILT:
        case OPCODE_INE:
        case OPCODE_ATOMIC_AND:
        case OPCODE_ATOMIC_IADD:
        case OPCODE_ATOMIC_OR:
        case OPCODE_ATOMIC_XOR:
        case OPCODE_ATOMIC_IMAX:
        case OPCODE_ATOMIC_IMIN:
        case OPCODE_DADD:
        case OPCODE_DMAX:
        case OPCODE_DMIN:
        case OPCODE_DMUL:
        case OPCODE_DEQ:
        case OPCODE_DGE:
        case OPCODE_DLT:
        case OPCODE_DNE:
        case OPCODE_DDIV:
        {
            psInst->ui32NumOperands = 3;
            ui32OperandOffset += DecodeOperand(pui32Token + ui32OperandOffset, &psInst->asOperands[0]);
            ui32OperandOffset += DecodeOperand(pui32Token + ui32OperandOffset, &psInst->asOperands[1]);
            ui32OperandOffset += DecodeOperand(pui32Token + ui32OperandOffset, &psInst->asOperands[2]);
            break;
        }
        case OPCODE_UGE:
        case OPCODE_ULT:
        case OPCODE_USHR:
        case OPCODE_ATOMIC_UMAX:
        case OPCODE_ATOMIC_UMIN:
        {
            psInst->ui32NumOperands = 3;
            ui32OperandOffset += DecodeOperand(pui32Token + ui32OperandOffset, &psInst->asOperands[0]);
            ui32OperandOffset += DecodeOperand(pui32Token + ui32OperandOffset, &psInst->asOperands[1]);
            ui32OperandOffset += DecodeOperand(pui32Token + ui32OperandOffset, &psInst->asOperands[2]);
            break;
        }
            // Instructions with four operands go here
        case OPCODE_MAD:
        case OPCODE_MOVC:
        case OPCODE_IMAD:
        case OPCODE_UDIV:
        case OPCODE_LOD:
        case OPCODE_SAMPLE:
        case OPCODE_GATHER4:
        case OPCODE_LD_MS:
        case OPCODE_UBFE:
        case OPCODE_IBFE:
        case OPCODE_ATOMIC_CMP_STORE:
        case OPCODE_IMM_ATOMIC_IADD:
        case OPCODE_IMM_ATOMIC_AND:
        case OPCODE_IMM_ATOMIC_OR:
        case OPCODE_IMM_ATOMIC_XOR:
        case OPCODE_IMM_ATOMIC_EXCH:
        case OPCODE_IMM_ATOMIC_IMAX:
        case OPCODE_IMM_ATOMIC_IMIN:
        case OPCODE_DMOVC:
        case OPCODE_DFMA:
        case OPCODE_IMUL:
        {
            psInst->ui32NumOperands = 4;

            if (eOpcode == OPCODE_IMUL || eOpcode == OPCODE_UDIV)
            {
                psInst->ui32FirstSrc = 2;
            }

            ui32OperandOffset += DecodeOperand(pui32Token + ui32OperandOffset, &psInst->asOperands[0]);
            ui32OperandOffset += DecodeOperand(pui32Token + ui32OperandOffset, &psInst->asOperands[1]);
            ui32OperandOffset += DecodeOperand(pui32Token + ui32OperandOffset, &psInst->asOperands[2]);
            ui32OperandOffset += DecodeOperand(pui32Token + ui32OperandOffset, &psInst->asOperands[3]);

            break;
        }
        case OPCODE_UADDC:
        case OPCODE_USUBB:
        case OPCODE_IMM_ATOMIC_UMAX:
        case OPCODE_IMM_ATOMIC_UMIN:
        {
            psInst->ui32NumOperands = 4;

            if (eOpcode == OPCODE_IMUL)
            {
                psInst->ui32FirstSrc = 2;
            }

            ui32OperandOffset += DecodeOperand(pui32Token + ui32OperandOffset, &psInst->asOperands[0]);
            ui32OperandOffset += DecodeOperand(pui32Token + ui32OperandOffset, &psInst->asOperands[1]);
            ui32OperandOffset += DecodeOperand(pui32Token + ui32OperandOffset, &psInst->asOperands[2]);
            ui32OperandOffset += DecodeOperand(pui32Token + ui32OperandOffset, &psInst->asOperands[3]);

            break;
        }
        case OPCODE_GATHER4_PO:
        case OPCODE_SAMPLE_L:
        case OPCODE_BFI:
        case OPCODE_SWAPC:
        case OPCODE_IMM_ATOMIC_CMP_EXCH:
        {
            psInst->ui32NumOperands = 5;
            ui32OperandOffset += DecodeOperand(pui32Token + ui32OperandOffset, &psInst->asOperands[0]);
            ui32OperandOffset += DecodeOperand(pui32Token + ui32OperandOffset, &psInst->asOperands[1]);
            ui32OperandOffset += DecodeOperand(pui32Token + ui32OperandOffset, &psInst->asOperands[2]);
            ui32OperandOffset += DecodeOperand(pui32Token + ui32OperandOffset, &psInst->asOperands[3]);
            ui32OperandOffset += DecodeOperand(pui32Token + ui32OperandOffset, &psInst->asOperands[4]);

            break;
        }
        case OPCODE_GATHER4_C:
        case OPCODE_SAMPLE_C:
        case OPCODE_SAMPLE_C_LZ:
        case OPCODE_SAMPLE_B:
        {
            psInst->ui32NumOperands = 5;
            ui32OperandOffset += DecodeOperand(pui32Token + ui32OperandOffset, &psInst->asOperands[0]);
            ui32OperandOffset += DecodeOperand(pui32Token + ui32OperandOffset, &psInst->asOperands[1]);
            ui32OperandOffset += DecodeOperand(pui32Token + ui32OperandOffset, &psInst->asOperands[2]);
            ui32OperandOffset += DecodeOperand(pui32Token + ui32OperandOffset, &psInst->asOperands[3]);
            ui32OperandOffset += DecodeOperand(pui32Token + ui32OperandOffset, &psInst->asOperands[4]);
            break;
        }
        case OPCODE_GATHER4_PO_C:
        case OPCODE_SAMPLE_D:
        {
            psInst->ui32NumOperands = 6;
            ui32OperandOffset += DecodeOperand(pui32Token + ui32OperandOffset, &psInst->asOperands[0]);
            ui32OperandOffset += DecodeOperand(pui32Token + ui32OperandOffset, &psInst->asOperands[1]);
            ui32OperandOffset += DecodeOperand(pui32Token + ui32OperandOffset, &psInst->asOperands[2]);
            ui32OperandOffset += DecodeOperand(pui32Token + ui32OperandOffset, &psInst->asOperands[3]);
            ui32OperandOffset += DecodeOperand(pui32Token + ui32OperandOffset, &psInst->asOperands[4]);
            ui32OperandOffset += DecodeOperand(pui32Token + ui32OperandOffset, &psInst->asOperands[5]);
            break;
        }
        case OPCODE_IF:
        case OPCODE_BREAKC:
        case OPCODE_CONTINUEC:
        case OPCODE_RETC:
        case OPCODE_DISCARD:
        {
            psInst->eBooleanTestType = DecodeInstrTestBool(*pui32Token);
            psInst->ui32NumOperands = 1;
            psInst->ui32FirstSrc = 0;
            ui32OperandOffset += DecodeOperand(pui32Token + ui32OperandOffset, &psInst->asOperands[0]);
            break;
        }
        case OPCODE_CALLC:
        {
            psInst->eBooleanTestType = DecodeInstrTestBool(*pui32Token);
            psInst->ui32NumOperands = 2;
            psInst->ui32FirstSrc = 0;
            ui32OperandOffset += DecodeOperand(pui32Token + ui32OperandOffset, &psInst->asOperands[0]);
            ui32OperandOffset += DecodeOperand(pui32Token + ui32OperandOffset, &psInst->asOperands[1]);
            break;
        }
        case OPCODE_CUSTOMDATA:
        {
            psInst->ui32NumOperands = 0;
            ui32TokenLength = pui32Token[1];
            break;
        }
        case OPCODE_EVAL_CENTROID:
        {
            psInst->ui32NumOperands = 2;
            ui32OperandOffset += DecodeOperand(pui32Token + ui32OperandOffset, &psInst->asOperands[0]);
            ui32OperandOffset += DecodeOperand(pui32Token + ui32OperandOffset, &psInst->asOperands[1]);
            break;
        }
        case OPCODE_EVAL_SAMPLE_INDEX:
        case OPCODE_EVAL_SNAPPED:
        case OPCODE_STORE_UAV_TYPED:
        case OPCODE_LD_UAV_TYPED:
        case OPCODE_LD_RAW:
        case OPCODE_STORE_RAW:
        {
            psInst->ui32NumOperands = 3;
            ui32OperandOffset += DecodeOperand(pui32Token + ui32OperandOffset, &psInst->asOperands[0]);
            ui32OperandOffset += DecodeOperand(pui32Token + ui32OperandOffset, &psInst->asOperands[1]);
            ui32OperandOffset += DecodeOperand(pui32Token + ui32OperandOffset, &psInst->asOperands[2]);
            break;
        }
        case OPCODE_STORE_STRUCTURED:
        case OPCODE_LD_STRUCTURED:
        {
            psInst->ui32NumOperands = 4;
            ui32OperandOffset += DecodeOperand(pui32Token + ui32OperandOffset, &psInst->asOperands[0]);
            ui32OperandOffset += DecodeOperand(pui32Token + ui32OperandOffset, &psInst->asOperands[1]);
            ui32OperandOffset += DecodeOperand(pui32Token + ui32OperandOffset, &psInst->asOperands[2]);
            ui32OperandOffset += DecodeOperand(pui32Token + ui32OperandOffset, &psInst->asOperands[3]);
            break;
        }
        case OPCODE_RESINFO:
        {
            psInst->ui32NumOperands = 3;

            psInst->eResInfoReturnType = DecodeResInfoReturnType(pui32Token[0]);

            ui32OperandOffset += DecodeOperand(pui32Token + ui32OperandOffset, &psInst->asOperands[0]);
            ui32OperandOffset += DecodeOperand(pui32Token + ui32OperandOffset, &psInst->asOperands[1]);
            ui32OperandOffset += DecodeOperand(pui32Token + ui32OperandOffset, &psInst->asOperands[2]);
            break;
        }
        case OPCODE_MSAD:
        default:
        {
            ASSERT(0);
            break;
        }
    }

    UpdateInstructionReferences(psShader, psInst);

    return pui32Token + ui32TokenLength;
}

void BindTextureToSampler(Shader* psShader, uint32_t ui32TextureRegister, uint32_t ui32SamplerRegister, uint32_t bCompare)
{
    uint32_t ui32Sampler, ui32TextureUnit, bLoad;
    ASSERT(ui32TextureRegister < (1 << 10));
    ASSERT(ui32SamplerRegister < (1 << 10));

    if (psShader->sInfo.ui32NumSamplers >= MAX_RESOURCE_BINDINGS)
    {
        ASSERT(0);
        return;
    }

    ui32TextureUnit = ui32TextureRegister;
    for (ui32Sampler = 0; ui32Sampler < psShader->sInfo.ui32NumSamplers; ++ui32Sampler)
    {
        if (psShader->sInfo.asSamplers[ui32Sampler].sMask.ui10TextureBindPoint == ui32TextureRegister)
        {
            if (psShader->sInfo.asSamplers[ui32Sampler].sMask.ui10SamplerBindPoint == ui32SamplerRegister)
                break;
            ui32TextureUnit = MAX_RESOURCE_BINDINGS;  // Texture is used by two or more samplers - assign to an available texture unit later
        }
    }

    // MAX_RESOURCE_BINDINGS means no sampler object (used for texture load)
    bLoad = ui32SamplerRegister == MAX_RESOURCE_BINDINGS;

    if (bCompare)
        psShader->sInfo.asSamplers[ui32Sampler].sMask.bCompareSample = 1;
    else if (!bLoad)
        psShader->sInfo.asSamplers[ui32Sampler].sMask.bNormalSample = 1;
    else
    {
        psShader->sInfo.asSamplers[ui32Sampler].sMask.bNormalSample = 0;
        psShader->sInfo.asSamplers[ui32Sampler].sMask.bCompareSample = 0;
    }

    if (ui32Sampler == psShader->sInfo.ui32NumSamplers)
    {
        psShader->sInfo.asSamplers[ui32Sampler].sMask.ui10TextureBindPoint = ui32TextureRegister;
        psShader->sInfo.asSamplers[ui32Sampler].sMask.ui10SamplerBindPoint = ui32SamplerRegister;
        psShader->sInfo.asSamplers[ui32Sampler].sMask.ui10TextureUnit = ui32TextureUnit;
        ++psShader->sInfo.ui32NumSamplers;
    }
}

void RegisterUniformBuffer(Shader* psShader, ResourceGroup eGroup, uint32_t ui32BindPoint)
{
    uint32_t ui32UniformBuffer = psShader->sInfo.ui32NumUniformBuffers;
    psShader->sInfo.asUniformBuffers[ui32UniformBuffer].ui32BindPoint = ui32BindPoint;
    psShader->sInfo.asUniformBuffers[ui32UniformBuffer].eGroup = eGroup;
    ++psShader->sInfo.ui32NumUniformBuffers;
}

void RegisterStorageBuffer(Shader* psShader, ResourceGroup eGroup, uint32_t ui32BindPoint)
{
    uint32_t ui32StorageBuffer = psShader->sInfo.ui32NumStorageBuffers;
    psShader->sInfo.asStorageBuffers[ui32StorageBuffer].ui32BindPoint = ui32BindPoint;
    psShader->sInfo.asStorageBuffers[ui32StorageBuffer].eGroup = eGroup;
    ++psShader->sInfo.ui32NumStorageBuffers;
}

void RegisterImage(Shader* psShader, ResourceGroup eGroup, uint32_t ui32BindPoint)
{
    uint32_t ui32Image = psShader->sInfo.ui32NumImages;
    psShader->sInfo.asImages[ui32Image].ui32BindPoint = ui32BindPoint;
    psShader->sInfo.asImages[ui32Image].eGroup = eGroup;
    ++psShader->sInfo.ui32NumImages;
}

void AssignRemainingSamplers(Shader* psShader)
{
    uint32_t ui32Sampler;
    uint32_t aui32TextureUnitsUsed[(MAX_RESOURCE_BINDINGS + 31) / 32];
    uint32_t ui32MinAvailUnit;

    memset((void*)aui32TextureUnitsUsed, 0, sizeof(aui32TextureUnitsUsed));
    for (ui32Sampler = 0; ui32Sampler < psShader->sInfo.ui32NumSamplers; ++ui32Sampler)
    {
        uint32_t ui32Unit = psShader->sInfo.asSamplers[ui32Sampler].sMask.ui10TextureUnit;
        if (ui32Unit < MAX_RESOURCE_BINDINGS)
            aui32TextureUnitsUsed[ui32Unit / 32] |= 1 << (ui32Unit % 32);
    }

    ui32MinAvailUnit = 0;
    for (ui32Sampler = 0; ui32Sampler < psShader->sInfo.ui32NumSamplers; ++ui32Sampler)
    {
        uint32_t ui32Unit = psShader->sInfo.asSamplers[ui32Sampler].sMask.ui10TextureUnit;
        if (ui32Unit == MAX_RESOURCE_BINDINGS)
        {
            uint32_t ui32Mask, ui32AvailUnit;
            uint32_t ui32WordIndex = ui32MinAvailUnit / 32;
            uint32_t ui32BitIndex = ui32MinAvailUnit % 32;

            while (ui32WordIndex < sizeof(aui32TextureUnitsUsed))
            {
                if (aui32TextureUnitsUsed[ui32WordIndex] != ~0L)
                    break;
                ++ui32WordIndex;
                ui32BitIndex = 0;
            }
            if (ui32WordIndex == sizeof(aui32TextureUnitsUsed))
            {
                ASSERT(0);  // Not enough resource bindings
                break;
            }

            ui32Mask = aui32TextureUnitsUsed[ui32WordIndex];
            while (ui32BitIndex < 32)
            {
                if ((ui32Mask & (1 << ui32BitIndex)) == 0)
                    break;
                ++ui32BitIndex;
            }
            if (ui32BitIndex == 32)
            {
                ASSERT(0);
                break;
            }

            ui32AvailUnit = 32 * ui32WordIndex + ui32BitIndex;
            aui32TextureUnitsUsed[ui32WordIndex] |= (1 << ui32BitIndex);

            psShader->sInfo.asSamplers[ui32Sampler].sMask.ui10TextureUnit = ui32AvailUnit;
            ui32MinAvailUnit = ui32AvailUnit + 1;

            ASSERT(psShader->sInfo.asSamplers[ui32Sampler].sMask.ui10TextureUnit < MAX_RESOURCE_BINDINGS);
        }
    }
}

void UpdateDeclarationReferences(Shader* psShader, Declaration* psDecl)
{
    switch (psDecl->eOpcode)
    {
        case OPCODE_DCL_CONSTANT_BUFFER:
            RegisterUniformBuffer(psShader, RGROUP_CBUFFER, psDecl->asOperands[0].aui32ArraySizes[0]);
            break;
        case OPCODE_DCL_UNORDERED_ACCESS_VIEW_TYPED:
            RegisterImage(psShader, RGROUP_UAV, psDecl->asOperands[0].ui32RegisterNumber);
            break;
        case OPCODE_DCL_UNORDERED_ACCESS_VIEW_RAW:
            RegisterStorageBuffer(psShader, RGROUP_UAV, psDecl->asOperands[0].ui32RegisterNumber);
            break;
        case OPCODE_DCL_UNORDERED_ACCESS_VIEW_STRUCTURED:
            RegisterStorageBuffer(psShader, RGROUP_UAV, psDecl->asOperands[0].aui32ArraySizes[0]);
            break;
        case OPCODE_DCL_RESOURCE_RAW:
            RegisterStorageBuffer(psShader, RGROUP_TEXTURE, psDecl->asOperands[0].ui32RegisterNumber);
            break;
        case OPCODE_DCL_RESOURCE_STRUCTURED:
            RegisterStorageBuffer(psShader, RGROUP_TEXTURE, psDecl->asOperands[0].ui32RegisterNumber);
            break;
    }
}

void UpdateInstructionReferences(Shader* psShader, Instruction* psInst)
{
    uint32_t ui32Operand;
    const uint32_t ui32NumOperands = psInst->ui32NumOperands;
    for (ui32Operand = 0; ui32Operand < ui32NumOperands; ++ui32Operand)
    {
        Operand* psOperand = &psInst->asOperands[ui32Operand];
        if (psOperand->eType == OPERAND_TYPE_INPUT || psOperand->eType == OPERAND_TYPE_INPUT_CONTROL_POINT)
        {
            if (psOperand->iIndexDims == INDEX_2D)
            {
                if (psOperand->aui32ArraySizes[1] != 0)  // gl_in[].gl_Position
                {
                    psShader->abInputReferencedByInstruction[psOperand->ui32RegisterNumber] = 1;
                }
            }
            else
            {
                psShader->abInputReferencedByInstruction[psOperand->ui32RegisterNumber] = 1;
            }
        }
    }

    switch (psInst->eOpcode)
    {
        case OPCODE_SWAPC:
            psShader->bUseTempCopy = 1;
            break;
        case OPCODE_SAMPLE:
        case OPCODE_SAMPLE_L:
        case OPCODE_SAMPLE_D:
        case OPCODE_SAMPLE_B:
        case OPCODE_GATHER4:
            BindTextureToSampler(psShader, psInst->asOperands[2].ui32RegisterNumber, psInst->asOperands[3].ui32RegisterNumber, 0);
            break;
        case OPCODE_SAMPLE_C_LZ:
        case OPCODE_SAMPLE_C:
        case OPCODE_GATHER4_C:
            BindTextureToSampler(psShader, psInst->asOperands[2].ui32RegisterNumber, psInst->asOperands[3].ui32RegisterNumber, 1);
            break;
        case OPCODE_GATHER4_PO:
            BindTextureToSampler(psShader, psInst->asOperands[3].ui32RegisterNumber, psInst->asOperands[4].ui32RegisterNumber, 0);
            break;
        case OPCODE_GATHER4_PO_C:
            BindTextureToSampler(psShader, psInst->asOperands[3].ui32RegisterNumber, psInst->asOperands[4].ui32RegisterNumber, 1);
            break;
        case OPCODE_LD:
        case OPCODE_LD_MS:
            // MAX_RESOURCE_BINDINGS means no sampler object
            BindTextureToSampler(psShader, psInst->asOperands[2].ui32RegisterNumber, MAX_RESOURCE_BINDINGS, 0);
            break;
    }
}

const uint32_t* DecodeHullShaderJoinPhase(const uint32_t* pui32Tokens, Shader* psShader)
{
    const uint32_t* pui32CurrentToken = pui32Tokens;
    const uint32_t ui32ShaderLength = psShader->ui32ShaderLength;

    Instruction* psInst;

    // Declarations
    Declaration* psDecl;
    psDecl = hlslcc_malloc(sizeof(Declaration) * ui32ShaderLength);
    psShader->psHSJoinPhaseDecl = psDecl;
    psShader->ui32HSJoinDeclCount = 0;

    while (1)  // Keep going until we reach the first non-declaration token, or the end of the shader.
    {
        const uint32_t* pui32Result = DecodeDeclaration(psShader, pui32CurrentToken, psDecl);

        if (pui32Result)
        {
            pui32CurrentToken = pui32Result;
            psShader->ui32HSJoinDeclCount++;
            psDecl++;

            if (pui32CurrentToken >= (psShader->pui32FirstToken + ui32ShaderLength))
            {
                break;
            }
        }
        else
        {
            break;
        }
    }

    // Instructions
    psInst = hlslcc_malloc(sizeof(Instruction) * ui32ShaderLength);
    psShader->psHSJoinPhaseInstr = psInst;
    psShader->ui32HSJoinInstrCount = 0;

    while (pui32CurrentToken < (psShader->pui32FirstToken + ui32ShaderLength))
    {
        const uint32_t* nextInstr = DecodeInstruction(pui32CurrentToken, psInst, psShader);

#ifdef _DEBUG
        if (nextInstr == pui32CurrentToken)
        {
            ASSERT(0);
            break;
        }
#endif

        pui32CurrentToken = nextInstr;
        psShader->ui32HSJoinInstrCount++;

        psInst++;
    }

    return pui32CurrentToken;
}

const uint32_t* DecodeHullShaderForkPhase(const uint32_t* pui32Tokens, Shader* psShader)
{
    const uint32_t* pui32CurrentToken = pui32Tokens;
    const uint32_t ui32ShaderLength = psShader->ui32ShaderLength;
    const uint32_t ui32ForkPhaseIndex = psShader->ui32ForkPhaseCount;

    Instruction* psInst;

    // Declarations
    Declaration* psDecl;
    psDecl = hlslcc_malloc(sizeof(Declaration) * ui32ShaderLength);

    ASSERT(ui32ForkPhaseIndex < MAX_FORK_PHASES);

    psShader->ui32ForkPhaseCount++;

    psShader->apsHSForkPhaseDecl[ui32ForkPhaseIndex] = psDecl;
    psShader->aui32HSForkDeclCount[ui32ForkPhaseIndex] = 0;

    while (1)  // Keep going until we reach the first non-declaration token, or the end of the shader.
    {
        const uint32_t* pui32Result = DecodeDeclaration(psShader, pui32CurrentToken, psDecl);

        if (pui32Result)
        {
            pui32CurrentToken = pui32Result;
            psShader->aui32HSForkDeclCount[ui32ForkPhaseIndex]++;
            psDecl++;

            if (pui32CurrentToken >= (psShader->pui32FirstToken + ui32ShaderLength))
            {
                break;
            }
        }
        else
        {
            break;
        }
    }

    // Instructions
    psInst = hlslcc_malloc(sizeof(Instruction) * ui32ShaderLength);
    psShader->apsHSForkPhaseInstr[ui32ForkPhaseIndex] = psInst;
    psShader->aui32HSForkInstrCount[ui32ForkPhaseIndex] = 0;

    while (pui32CurrentToken < (psShader->pui32FirstToken + ui32ShaderLength))
    {
        const uint32_t* nextInstr = DecodeInstruction(pui32CurrentToken, psInst, psShader);

#ifdef _DEBUG
        if (nextInstr == pui32CurrentToken)
        {
            ASSERT(0);
            break;
        }
#endif

        pui32CurrentToken = nextInstr;

        if (psInst->eOpcode == OPCODE_HS_FORK_PHASE)
        {
            pui32CurrentToken = DecodeHullShaderForkPhase(pui32CurrentToken, psShader);
            return pui32CurrentToken;
        }

        psShader->aui32HSForkInstrCount[ui32ForkPhaseIndex]++;
        psInst++;
    }

    return pui32CurrentToken;
}

const uint32_t* DecodeHullShaderControlPointPhase(const uint32_t* pui32Tokens, Shader* psShader)
{
    const uint32_t* pui32CurrentToken = pui32Tokens;
    const uint32_t ui32ShaderLength = psShader->ui32ShaderLength;

    Instruction* psInst;

    // TODO one block of memory for instructions and declarions to reduce memory usage and number of allocs.
    // hlscc_malloc max(sizeof(declaration), sizeof(instruction) * shader length; or sizeof(DeclInst) - unifying both structs.

    // Declarations
    Declaration* psDecl;
    psDecl = hlslcc_malloc(sizeof(Declaration) * ui32ShaderLength);
    psShader->psHSControlPointPhaseDecl = psDecl;
    psShader->ui32HSControlPointDeclCount = 0;

    while (1)  // Keep going until we reach the first non-declaration token, or the end of the shader.
    {
        const uint32_t* pui32Result = DecodeDeclaration(psShader, pui32CurrentToken, psDecl);

        if (pui32Result)
        {
            pui32CurrentToken = pui32Result;
            psShader->ui32HSControlPointDeclCount++;
            psDecl++;

            if (pui32CurrentToken >= (psShader->pui32FirstToken + ui32ShaderLength))
            {
                break;
            }
        }
        else
        {
            break;
        }
    }

    // Instructions
    psInst = hlslcc_malloc(sizeof(Instruction) * ui32ShaderLength);
    psShader->psHSControlPointPhaseInstr = psInst;
    psShader->ui32HSControlPointInstrCount = 0;

    while (pui32CurrentToken < (psShader->pui32FirstToken + ui32ShaderLength))
    {
        const uint32_t* nextInstr = DecodeInstruction(pui32CurrentToken, psInst, psShader);

#ifdef _DEBUG
        if (nextInstr == pui32CurrentToken)
        {
            ASSERT(0);
            break;
        }
#endif

        pui32CurrentToken = nextInstr;

        if (psInst->eOpcode == OPCODE_HS_FORK_PHASE)
        {
            pui32CurrentToken = DecodeHullShaderForkPhase(pui32CurrentToken, psShader);
            return pui32CurrentToken;
        }
        if (psInst->eOpcode == OPCODE_HS_JOIN_PHASE)
        {
            pui32CurrentToken = DecodeHullShaderJoinPhase(pui32CurrentToken, psShader);
            return pui32CurrentToken;
        }
        psInst++;
        psShader->ui32HSControlPointInstrCount++;
    }

    return pui32CurrentToken;
}

const uint32_t* DecodeHullShader(const uint32_t* pui32Tokens, Shader* psShader)
{
    const uint32_t* pui32CurrentToken = pui32Tokens;
    const uint32_t ui32ShaderLength = psShader->ui32ShaderLength;
    Declaration* psDecl;
    psDecl = hlslcc_malloc(sizeof(Declaration) * ui32ShaderLength);
    psShader->psHSDecl = psDecl;
    psShader->ui32HSDeclCount = 0;

    while (1)  // Keep going until we reach the first non-declaration token, or the end of the shader.
    {
        const uint32_t* pui32Result = DecodeDeclaration(psShader, pui32CurrentToken, psDecl);

        if (pui32Result)
        {
            pui32CurrentToken = pui32Result;

            if (psDecl->eOpcode == OPCODE_HS_CONTROL_POINT_PHASE)
            {
                pui32CurrentToken = DecodeHullShaderControlPointPhase(pui32CurrentToken, psShader);
                return pui32CurrentToken;
            }
            if (psDecl->eOpcode == OPCODE_HS_FORK_PHASE)
            {
                pui32CurrentToken = DecodeHullShaderForkPhase(pui32CurrentToken, psShader);
                return pui32CurrentToken;
            }
            if (psDecl->eOpcode == OPCODE_HS_JOIN_PHASE)
            {
                pui32CurrentToken = DecodeHullShaderJoinPhase(pui32CurrentToken, psShader);
                return pui32CurrentToken;
            }

            psDecl++;
            psShader->ui32HSDeclCount++;

            if (pui32CurrentToken >= (psShader->pui32FirstToken + ui32ShaderLength))
            {
                break;
            }
        }
        else
        {
            break;
        }
    }

    return pui32CurrentToken;
}

void Decode(const uint32_t* pui32Tokens, Shader* psShader)
{
    const uint32_t* pui32CurrentToken = pui32Tokens;
    const uint32_t ui32ShaderLength = pui32Tokens[1];
    Instruction* psInst;
    Declaration* psDecl;

    psShader->ui32MajorVersion = DecodeProgramMajorVersion(*pui32CurrentToken);
    psShader->ui32MinorVersion = DecodeProgramMinorVersion(*pui32CurrentToken);
    psShader->eShaderType = DecodeShaderType(*pui32CurrentToken);

    pui32CurrentToken++;  // Move to shader length
    psShader->ui32ShaderLength = ui32ShaderLength;
    pui32CurrentToken++;  // Move to after shader length (usually a declaration)

    psShader->pui32FirstToken = pui32Tokens;

#ifdef _DEBUG
    operandID = 0;
    instructionID = 0;
#endif

    if (psShader->eShaderType == HULL_SHADER)
    {
        pui32CurrentToken = DecodeHullShader(pui32CurrentToken, psShader);
        return;
    }

    // Using ui32ShaderLength as the instruction count
    // will allocate more than enough memory. Avoids having to
    // traverse the entire shader just to get the real instruction count.
    psInst = hlslcc_malloc(sizeof(Instruction) * ui32ShaderLength);
    psShader->psInst = psInst;
    psShader->ui32InstCount = 0;

    psDecl = hlslcc_malloc(sizeof(Declaration) * ui32ShaderLength);
    psShader->psDecl = psDecl;
    psShader->ui32DeclCount = 0;

    while (1)  // Keep going until we reach the first non-declaration token, or the end of the shader.
    {
        const uint32_t* pui32Result = DecodeDeclaration(psShader, pui32CurrentToken, psDecl);

        if (pui32Result)
        {
            pui32CurrentToken = pui32Result;
            psShader->ui32DeclCount++;
            psDecl++;

            if (pui32CurrentToken >= (psShader->pui32FirstToken + ui32ShaderLength))
            {
                break;
            }
        }
        else
        {
            break;
        }
    }

    while (pui32CurrentToken < (psShader->pui32FirstToken + ui32ShaderLength))
    {
        const uint32_t* nextInstr = DecodeInstruction(pui32CurrentToken, psInst, psShader);

#ifdef _DEBUG
        if (nextInstr == pui32CurrentToken)
        {
            ASSERT(0);
            break;
        }
#endif

        pui32CurrentToken = nextInstr;
        psShader->ui32InstCount++;
        psInst++;
    }

    AssignRemainingSamplers(psShader);
}

Shader* DecodeDXBC(uint32_t* data)
{
    Shader* psShader;
    DXBCContainerHeader* header = (DXBCContainerHeader*)data;
    uint32_t i;
    uint32_t chunkCount;
    uint32_t* chunkOffsets;
    ReflectionChunks refChunks;
    uint32_t* shaderChunk = 0;

    if (header->fourcc != FOURCC_DXBC)
    {
        // Could be SM1/2/3. If the shader type token
        // looks valid then we continue
        uint32_t type = DecodeShaderTypeDX9(data[0]);

        if (type != INVALID_SHADER)
        {
            return DecodeDX9BC(data);
        }
        return 0;
    }

    refChunks.pui32Inputs = NULL;
    refChunks.pui32Interfaces = NULL;
    refChunks.pui32Outputs = NULL;
    refChunks.pui32Resources = NULL;
    refChunks.pui32Inputs11 = NULL;
    refChunks.pui32Outputs11 = NULL;
    refChunks.pui32OutputsWithStreams = NULL;

    chunkOffsets = (uint32_t*)(header + 1);

    chunkCount = header->chunkCount;

    for (i = 0; i < chunkCount; ++i)
    {
        uint32_t offset = chunkOffsets[i];

        DXBCChunkHeader* chunk = (DXBCChunkHeader*)((char*)data + offset);

        switch (chunk->fourcc)
        {
            case FOURCC_ISGN:
            {
                refChunks.pui32Inputs = (uint32_t*)(chunk + 1);
                break;
            }
            case FOURCC_ISG1:
            {
                refChunks.pui32Inputs11 = (uint32_t*)(chunk + 1);
                break;
            }
            case FOURCC_RDEF:
            {
                refChunks.pui32Resources = (uint32_t*)(chunk + 1);
                break;
            }
            case FOURCC_IFCE:
            {
                refChunks.pui32Interfaces = (uint32_t*)(chunk + 1);
                break;
            }
            case FOURCC_OSGN:
            {
                refChunks.pui32Outputs = (uint32_t*)(chunk + 1);
                break;
            }
            case FOURCC_OSG1:
            {
                refChunks.pui32Outputs11 = (uint32_t*)(chunk + 1);
                break;
            }
            case FOURCC_OSG5:
            {
                refChunks.pui32OutputsWithStreams = (uint32_t*)(chunk + 1);
                break;
            }
            case FOURCC_SHDR:
            case FOURCC_SHEX:
            {
                shaderChunk = (uint32_t*)(chunk + 1);
                break;
            }
            default:
            {
                break;
            }
        }
    }

    if (shaderChunk)
    {
        uint32_t ui32MajorVersion;
        uint32_t ui32MinorVersion;

        psShader = hlslcc_calloc(1, sizeof(Shader));

        ui32MajorVersion = DecodeProgramMajorVersion(*shaderChunk);
        ui32MinorVersion = DecodeProgramMinorVersion(*shaderChunk);

        LoadShaderInfo(ui32MajorVersion, ui32MinorVersion, &refChunks, &psShader->sInfo);

        Decode(shaderChunk, psShader);

        return psShader;
    }

    return 0;
}
