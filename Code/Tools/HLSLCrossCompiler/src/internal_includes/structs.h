// Modifications copyright Amazon.com, Inc. or its affiliates
// Modifications copyright Crytek GmbH

#ifndef STRUCTS_H
#define STRUCTS_H

#include "hlslcc.h"
#include "bstrlib.h"

#include "internal_includes/tokens.h"
#include "internal_includes/reflect.h"

enum
{
    MAX_SUB_OPERANDS = 3
};

typedef struct Operand_TAG
{
    int iExtended;
    OPERAND_TYPE eType;
    OPERAND_MODIFIER eModifier;
    OPERAND_MIN_PRECISION eMinPrecision;
    int iIndexDims;
    int indexRepresentation[4];
    int writeMask;
    int iGSInput;
    int iWriteMaskEnabled;

    int iNumComponents;

    OPERAND_4_COMPONENT_SELECTION_MODE eSelMode;
    uint32_t ui32CompMask;
    uint32_t ui32Swizzle;
    uint32_t aui32Swizzle[4];

    uint32_t aui32ArraySizes[3];
    uint32_t ui32RegisterNumber;
    //If eType is OPERAND_TYPE_IMMEDIATE32
    float afImmediates[4];
    //If eType is OPERAND_TYPE_IMMEDIATE64
    double adImmediates[4];

    int iIntegerImmediate;

    SPECIAL_NAME eSpecialName;
    char pszSpecialName[64];

    OPERAND_INDEX_REPRESENTATION eIndexRep[3];

    struct Operand_TAG* psSubOperand[MAX_SUB_OPERANDS];

    //One type for each component.
    SHADER_VARIABLE_TYPE aeDataType[4];

#ifdef _DEBUG
    uint64_t id;
#endif
} Operand;

typedef struct Instruction_TAG
{
    OPCODE_TYPE eOpcode;
    INSTRUCTION_TEST_BOOLEAN eBooleanTestType;
    COMPARISON_DX9 eDX9TestType;
    uint32_t ui32SyncFlags;
    uint32_t ui32NumOperands;
    uint32_t ui32FirstSrc;
    Operand asOperands[6];
    uint32_t bSaturate;
    uint32_t ui32FuncIndexWithinInterface;
    RESINFO_RETURN_TYPE eResInfoReturnType;

    int bAddressOffset;
    int iUAddrOffset;
    int iVAddrOffset;
    int iWAddrOffset;
    RESOURCE_RETURN_TYPE xType, yType, zType, wType;
    RESOURCE_DIMENSION eResDim;

#ifdef _DEBUG
    uint64_t id;
#endif
} Instruction;

enum
{
    MAX_IMMEDIATE_CONST_BUFFER_VEC4_SIZE = 1024
};

typedef struct ICBVec4_TAG
{
    uint32_t a;
    uint32_t b;
    uint32_t c;
    uint32_t d;
} ICBVec4;

typedef struct Declaration_TAG
{
    OPCODE_TYPE eOpcode;

    uint32_t ui32NumOperands;

    Operand asOperands[2];

    ICBVec4 asImmediateConstBuffer[MAX_IMMEDIATE_CONST_BUFFER_VEC4_SIZE];
    //The declaration can set one of these
    //values depending on the opcode.
    union
    {
        uint32_t ui32GlobalFlags;
        uint32_t ui32NumTemps;
        RESOURCE_DIMENSION eResourceDimension;
        CONSTANT_BUFFER_ACCESS_PATTERN eCBAccessPattern;
        INTERPOLATION_MODE eInterpolation;
        PRIMITIVE_TOPOLOGY eOutputPrimitiveTopology;
        PRIMITIVE eInputPrimitive;
        uint32_t ui32MaxOutputVertexCount;
        TESSELLATOR_DOMAIN eTessDomain;
        TESSELLATOR_PARTITIONING eTessPartitioning;
        TESSELLATOR_OUTPUT_PRIMITIVE eTessOutPrim;
        uint32_t aui32WorkGroupSize[3];
        //Fork phase index followed by the instance count.
        uint32_t aui32HullPhaseInstanceInfo[2];
        float fMaxTessFactor;
        uint32_t ui32IndexRange;
        uint32_t ui32GSInstanceCount;

        struct Interface_TAG
        {
            uint32_t ui32InterfaceID;
            uint32_t ui32NumFuncTables;
            uint32_t ui32ArraySize;
        } interface;
    } value;

    struct UAV_TAG
    {
        uint32_t ui32GloballyCoherentAccess;
        uint32_t ui32BufferSize;
        uint8_t bCounter;
        RESOURCE_RETURN_TYPE Type;
    } sUAV;

    struct TGSM_TAG
    {
        uint32_t ui32Stride;
        uint32_t ui32Count;
    } sTGSM;

    struct IndexableTemp_TAG
    {
        uint32_t ui32RegIndex;
        uint32_t ui32RegCount;
        uint32_t ui32RegComponentSize;
    } sIdxTemp;

    uint32_t ui32TableLength;

    uint32_t ui32TexReturnType;
} Declaration;

enum
{
    MAX_TEMP_VEC4 = 512
};

enum
{
    MAX_GROUPSHARED = 8
};

enum
{
    MAX_DX9_IMMCONST = 256
};

typedef struct Shader_TAG
{
    uint32_t ui32MajorVersion;
    uint32_t ui32MinorVersion;
    SHADER_TYPE eShaderType;

    GLLang eTargetLanguage;
    const struct GlExtensions *extensions;

    int fp64;

    //DWORDs in program code, including version and length tokens.
    uint32_t ui32ShaderLength;

    uint32_t ui32DeclCount;
    Declaration* psDecl;

    //Instruction* functions;//non-main subroutines

    uint32_t aui32FuncTableToFuncPointer[MAX_FUNCTION_TABLES];//FIXME dynamic alloc
    uint32_t aui32FuncBodyToFuncTable[MAX_FUNCTION_BODIES];

    struct
    {
        uint32_t aui32FuncBodies[MAX_FUNCTION_BODIES];
    }funcTable[MAX_FUNCTION_TABLES];

    struct
    {
        uint32_t aui32FuncTables[MAX_FUNCTION_TABLES];
        uint32_t ui32NumBodiesPerTable;
    }funcPointer[MAX_FUNCTION_POINTERS];

    uint32_t ui32NextClassFuncName[MAX_CLASS_TYPES];

    uint32_t ui32InstCount;
    Instruction* psInst;

    const uint32_t* pui32FirstToken;//Reference for calculating current position in token stream.

    //Hull shader declarations and instructions.
    //psDecl, psInst are null for hull shaders.
    uint32_t ui32HSDeclCount;
    Declaration* psHSDecl;

    uint32_t ui32HSControlPointDeclCount;
    Declaration* psHSControlPointPhaseDecl;

    uint32_t ui32HSControlPointInstrCount;
    Instruction* psHSControlPointPhaseInstr;

    uint32_t ui32ForkPhaseCount;

    uint32_t aui32HSForkDeclCount[MAX_FORK_PHASES];
    Declaration* apsHSForkPhaseDecl[MAX_FORK_PHASES];

    uint32_t aui32HSForkInstrCount[MAX_FORK_PHASES];
    Instruction* apsHSForkPhaseInstr[MAX_FORK_PHASES];

    uint32_t ui32HSJoinDeclCount;
    Declaration* psHSJoinPhaseDecl;

    uint32_t ui32HSJoinInstrCount;
    Instruction* psHSJoinPhaseInstr;

    ShaderInfo sInfo;

    int abScalarInput[MAX_SHADER_VEC4_INPUT];

    int aIndexedOutput[MAX_SHADER_VEC4_OUTPUT];

    int aIndexedInput[MAX_SHADER_VEC4_INPUT];
    int aIndexedInputParents[MAX_SHADER_VEC4_INPUT];

    RESOURCE_DIMENSION aeResourceDims[MAX_TEXTURES];

    int aiInputDeclaredSize[MAX_SHADER_VEC4_INPUT];

    int aiOutputDeclared[MAX_SHADER_VEC4_OUTPUT];

    //Does not track built-in inputs.
    int abInputReferencedByInstruction[MAX_SHADER_VEC4_INPUT];

    int aiOpcodeUsed[NUM_OPCODES];

    uint32_t ui32CurrentVertexOutputStream;

    uint32_t ui32NumDx9ImmConst;
    uint32_t aui32Dx9ImmConstArrayRemap[MAX_DX9_IMMCONST];

    ShaderVarType sGroupSharedVarType[MAX_GROUPSHARED];

    SHADER_VARIABLE_TYPE aeCommonTempVecType[MAX_TEMP_VEC4];
    uint32_t bUseTempCopy;
    FRAMEBUFFER_FETCH_TYPE eGmemType;
} Shader;

/* CONFETTI NOTE: DAVID SROUR
 * The following is super sketchy, but at the moment,
 * there is no way to figure out the type of a resource
 * since HLSL has only register sets for the following:
 * bool, int4, float4, sampler.
 * THIS CODE IS DUPLICATED FROM HLSLcc METAL.
 * IF ANYTHING CHANGES, BOTH TRANSLATORS SHOULD HAVE THE CHANGE.
 * TODO: CONSOLIDATE THE 2 HLSLcc PROJECTS.
 */
enum
{
    GMEM_FLOAT4_START_SLOT = 120
};
enum
{
    GMEM_FLOAT3_START_SLOT = 112
};
enum
{
    GMEM_FLOAT2_START_SLOT = 104
};
enum
{
    GMEM_FLOAT_START_SLOT = 96
};

enum
{
    GMEM_ARM_COLOR_SLOT = 93,
    GMEM_ARM_DEPTH_SLOT = 94,
    GMEM_ARM_STENCIL_SLOT = 95
};

/* CONFETTI NOTE: DAVID SROUR
 * Following is the reserved slot for PLS extension (https://www.khronos.org/registry/gles/extensions/EXT/EXT_shader_pixel_local_storage.txt).
 * It will get picked up when a RWStructuredBuffer resource is defined at the following reserved slot.
 * Note that only one PLS struct can be present at a time otherwise the behavior is undefined.
 *
 * Types in the struct and their output conversion (each output variable will always be 4 bytes):
 * float2   -> rg16f
 * float3   -> r11f_g11f_b10f
 * float4   -> rgba8
 * uint     -> r32ui
 * int2     -> rg16i
 * int4     -> rgba8i
 */
enum
{
    GMEM_PLS_RO_SLOT = 60
};                              // READ-ONLY
enum
{
    GMEM_PLS_WO_SLOT = 61
};                              // WRITE-ONLY
enum
{
    GMEM_PLS_RW_SLOT = 62
};                              // READ/WRITE

static const uint32_t MAIN_PHASE = 0;
static const uint32_t HS_FORK_PHASE = 1;
static const uint32_t HS_CTRL_POINT_PHASE = 2;
static const uint32_t HS_JOIN_PHASE = 3;
enum
{
    NUM_PHASES = 4
};

enum
{
    MAX_COLOR_MRT = 8
};

enum
{
    INPUT_RENDERTARGET = 1 << 0,
    OUTPUT_RENDERTARGET = 1 << 1
};

typedef struct HLSLCrossCompilerContext_TAG
{
    bstring glsl;
    bstring earlyMain;//Code to be inserted at the start of main()
    bstring postShaderCode[NUM_PHASES];//End of main or before emit()
    bstring debugHeader;

    bstring* currentGLSLString;//either glsl or earlyMain

    int havePostShaderCode[NUM_PHASES];
    uint32_t currentPhase;

    uint32_t rendertargetUse[MAX_COLOR_MRT];

    int indent;
    unsigned int flags;
    Shader* psShader;
} HLSLCrossCompilerContext;

#endif
