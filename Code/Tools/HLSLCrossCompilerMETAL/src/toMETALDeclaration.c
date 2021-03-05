// Modifications copyright Amazon.com, Inc. or its affiliates
// Modifications copyright Crytek GmbH

#include "hlslcc.h"
#include "internal_includes/toMETALDeclaration.h"
#include "internal_includes/toMETALOperand.h"
#include "internal_includes/languages.h"
#include "bstrlib.h"
#include "internal_includes/debug.h"
#include "internal_includes/hlslcc_malloc.h"
#include "internal_includes/structsMetal.h"
#include <math.h>
#include <float.h>

#if defined(__clang__)
#pragma clang diagnostic ignored "-Wpointer-sign"
#endif

#ifdef _MSC_VER
#ifndef isnan
#define isnan(x) _isnan(x)
#endif

#ifndef isinf
#define isinf(x) (!_finite(x))
#endif
#endif

#define fpcheck(x) (isnan(x) || isinf(x))

typedef enum
{
    GLVARTYPE_FLOAT,
    GLVARTYPE_INT,
    GLVARTYPE_FLOAT4,
} GLVARTYPE;

extern void AddIndentation(HLSLCrossCompilerContext* psContext);

const char* GetTypeStringMETAL(GLVARTYPE eType)
{
    switch (eType)
    {
    case GLVARTYPE_FLOAT:
    {
        return "float";
    }
    case GLVARTYPE_INT:
    {
        return "int";
    }
    case GLVARTYPE_FLOAT4:
    {
        return "float4";
    }
    default:
    {
        return "";
    }
    }
}
const uint32_t GetTypeElementCountMETAL(GLVARTYPE eType)
{
    switch (eType)
    {
    case GLVARTYPE_FLOAT:
    case GLVARTYPE_INT:
    {
        return 1;
    }
    case GLVARTYPE_FLOAT4:
    {
        return 4;
    }
    default:
    {
        return 0;
    }
    }
}

void AddToDx9ImmConstIndexableArrayMETAL(HLSLCrossCompilerContext* psContext, const Operand* psOperand)
{
    bstring* savedStringPtr = psContext->currentShaderString;

    psContext->currentShaderString = &psContext->earlyMain;
    psContext->indent++;
    AddIndentation(psContext);
    psContext->psShader->aui32Dx9ImmConstArrayRemap[psOperand->ui32RegisterNumber] = psContext->psShader->ui32NumDx9ImmConst;
    bformata(psContext->earlyMain, "ImmConstArray[%d] = ", psContext->psShader->ui32NumDx9ImmConst);
    TranslateOperandMETAL(psContext, psOperand, TO_FLAG_NONE);
    bcatcstr(psContext->earlyMain, ";\n");
    psContext->indent--;
    psContext->psShader->ui32NumDx9ImmConst++;

    psContext->currentShaderString = savedStringPtr;
}

void DeclareConstBufferShaderVariableMETAL(bstring metal, const char* Name, const struct ShaderVarType_TAG* psType, int pointerType, int const createDummyAlignment, AtomicVarList* psAtomicList)
//const SHADER_VARIABLE_CLASS eClass, const SHADER_VARIABLE_TYPE eType,
//const char* pszName)
{
    if (psType->Class == SVC_STRUCT)
    {
        bformata(metal, "%s_Type %s%s", Name, pointerType ? "*" : "", Name);
        if (psType->Elements > 1)
        {
            bformata(metal, "[%d]", psType->Elements);
        }
    }
    else if (psType->Class == SVC_MATRIX_COLUMNS || psType->Class == SVC_MATRIX_ROWS)
    {
        switch (psType->Type)
        {
        case SVT_FLOAT:
        {
            bformata(metal, "\tfloat%d %s%s[%d", psType->Columns, pointerType ? "*" : "", Name, psType->Rows);
            break;
        }
        case SVT_FLOAT16:
        {
            bformata(metal, "\thalf%d %s%s[%d", psType->Columns, pointerType ? "*" : "", Name, psType->Rows);
            break;
        }
        default:
        {
            ASSERT(0);
            break;
        }
        }
        if (psType->Elements > 1)
        {
            bformata(metal, " * %d", psType->Elements);
        }
        bformata(metal, "]");
    }
    else
    if (psType->Class == SVC_VECTOR)
    {
        switch (psType->Type)
        {
        case SVT_DOUBLE:
        case SVT_FLOAT:
        {
            bformata(metal, "\tfloat%d %s%s", psType->Columns, pointerType ? "*" : "", Name);
            break;
        }
        case SVT_FLOAT16:
        {
            bformata(metal, "\thalf%d %s%s", psType->Columns, pointerType ? "*" : "", Name);
            break;
        }
        case SVT_UINT:
        {
            bformata(metal, "\tuint%d %s%s", psType->Columns, pointerType ? "*" : "", Name);
            break;
        }
        case SVT_INT:
        case SVT_BOOL:
        {
            bformata(metal, "\tint%d %s%s", psType->Columns, pointerType ? "*" : "", Name);
            break;
        }
        default:
        {
            ASSERT(0);
            break;
        }
        }

        if (psType->Elements > 1)
        {
            bformata(metal, "[%d]", psType->Elements);
        }
    }
    else
    if (psType->Class == SVC_SCALAR)
    {
        switch (psType->Type)
        {
        case SVT_DOUBLE:
        case SVT_FLOAT:
        {
            bformata(metal, "\tfloat %s%s", pointerType ? "*" : "", Name);
            break;
        }
        case SVT_FLOAT16:
        {
            bformata(metal, "\thalf %s%s", pointerType ? "*" : "", Name);
            break;
        }
        case SVT_UINT:
        {
            if (IsAtomicVar(psType, psAtomicList))
            {
                bformata(metal, "\tvolatile atomic_uint %s%s", pointerType ? "*" : "", Name);
            }
            else
            {
                bformata(metal, "\tuint %s%s", pointerType ? "*" : "", Name);
            }
            break;
        }
        case SVT_INT:
        {
            if (IsAtomicVar(psType, psAtomicList))
            {
                bformata(metal, "\tvolatile atomic_int %s%s", pointerType ? "*" : "", Name);
            }
            else
            {
                bformata(metal, "\tint %s%s", pointerType ? "*" : "", Name);
            }
            break;
        }
        case SVT_BOOL:
        {
            //Use int instead of bool.
            //Allows implicit conversions to integer and
            //bool consumes 4-bytes in HLSL and metal anyway.
            bformata(metal, "\tint %s%s", pointerType ? "*" : "", Name);
            // Also change the definition in the type tree.
            ((ShaderVarType*)psType)->Type = SVT_INT;
            break;
        }
        default:
        {
            ASSERT(0);
            break;
        }
        }

        if (psType->Elements > 1)
        {
            bformata(metal, "[%d]", psType->Elements);
        }
    }
    if (!pointerType)
    {
        bformata(metal, ";\n");
    }

    // We need to add more dummies if float2 or less since they are not 16 bytes aligned
    // float = 4
    // float2 = 8
    // float3 = float4 = 16
    // https://developer.apple.com/library/ios/documentation/Metal/Reference/MetalShadingLanguageGuide/data-types/data-types.html
    if (createDummyAlignment)
    {
        uint16_t sizeInBytes = 16;
        if (1 == psType->Columns)
        {
            sizeInBytes = 4;
        }
        else if (2 == psType->Columns)
        {
            sizeInBytes = 8;
        }

        if (4 == sizeInBytes)
        {
            bformata(metal, "\tfloat  offsetDummy_4Bytes_%s;\n", Name);
            bformata(metal, "\tfloat2 offsetDummy_8Bytes_%s;\n", Name);
        }
        else if (8 == sizeInBytes)
        {
            bformata(metal, "\tfloat2 offsetDummy_8Bytes_%s;\n", Name);
        }
    }
}

//In metal embedded structure definitions are not supported.
void PreDeclareStructTypeMETAL(bstring metal, const char* Name, const struct ShaderVarType_TAG* psType, AtomicVarList* psAtomicList)
{
    uint32_t i;

    for (i = 0; i < psType->MemberCount; ++i)
    {
        if (psType->Members[i].Class == SVC_STRUCT)
        {
            PreDeclareStructTypeMETAL(metal, psType->Members[i].Name, &psType->Members[i], psAtomicList);
        }
    }

    if (psType->Class == SVC_STRUCT)
    {
        uint32_t unnamed_struct = strcmp(Name, "$Element") == 0 ? 1 : 0;

        //Not supported at the moment
        ASSERT(!unnamed_struct);

        bformata(metal, "struct %s_Type {\n", Name);

        for (i = 0; i < psType->MemberCount; ++i)
        {
            ASSERT(psType->Members != 0);

            DeclareConstBufferShaderVariableMETAL(metal, psType->Members[i].Name, &psType->Members[i], 0, 0, psAtomicList);
        }

        bformata(metal, "};\n");
    }
}

char* GetDeclaredInputNameMETAL(const HLSLCrossCompilerContext* psContext, const SHADER_TYPE eShaderType, const Operand* psOperand)
{
    bstring inputName;
    char* cstr;
    InOutSignature* psIn;

    if (eShaderType == PIXEL_SHADER)
    {
        inputName = bformat("VtxOutput%d", psOperand->ui32RegisterNumber);
    }
    else
    {
        ASSERT(eShaderType == VERTEX_SHADER);
        inputName = bformat("dcl_Input%d", psOperand->ui32RegisterNumber);
    }
    if ((psContext->flags & HLSLCC_FLAG_INOUT_SEMANTIC_NAMES) && GetInputSignatureFromRegister(psOperand->ui32RegisterNumber, &psContext->psShader->sInfo, &psIn))
    {
        bformata(inputName, "_%s%d", psIn->SemanticName, psIn->ui32SemanticIndex);
    }

    cstr = bstr2cstr(inputName, '\0');
    bdestroy(inputName);
    return cstr;
}

char* GetDeclaredOutputNameMETAL(const HLSLCrossCompilerContext* psContext,
    const SHADER_TYPE eShaderType,
    const Operand* psOperand)
{
    bstring outputName = bformat("");
    char* cstr;
    InOutSignature* psOut;

    int foundOutput = GetOutputSignatureFromRegister(
            psContext->currentPhase,
            psOperand->ui32RegisterNumber,
            psOperand->ui32CompMask,
            psContext->psShader->ui32CurrentVertexOutputStream,
            &psContext->psShader->sInfo,
            &psOut);

    ASSERT(foundOutput);

    if (eShaderType == VERTEX_SHADER)
    {
        outputName = bformat("VtxOutput%d", psOperand->ui32RegisterNumber);
    }
    else if (eShaderType == PIXEL_SHADER)
    {
        outputName = bformat("PixOutput%d", psOperand->ui32RegisterNumber);
    }

    if (psContext->flags & HLSLCC_FLAG_INOUT_APPEND_SEMANTIC_NAMES)
    {
        bformata(outputName, "_%s%d", psOut->SemanticName, psOut->ui32SemanticIndex);
    }

    cstr = bstr2cstr(outputName, '\0');
    bdestroy(outputName);
    return cstr;
}

const char* GetInterpolationStringMETAL(INTERPOLATION_MODE eMode)
{
    switch (eMode)
    {
    case INTERPOLATION_CONSTANT:
    {
        return "flat";
    }
    case INTERPOLATION_LINEAR:
    {
        return "center_perspective";
    }
    case INTERPOLATION_LINEAR_CENTROID:
    {
        return "centroid_perspective";
    }
    case INTERPOLATION_LINEAR_NOPERSPECTIVE:
    {
        return "center_no_perspective";
        break;
    }
    case INTERPOLATION_LINEAR_NOPERSPECTIVE_CENTROID:
    {
        return "centroid_no_perspective";
    }
    case INTERPOLATION_LINEAR_SAMPLE:
    {
        return "sample_perspective";
    }
    case INTERPOLATION_LINEAR_NOPERSPECTIVE_SAMPLE:
    {
        return "sample_no_perspective";
    }
    default:
    {
        return "";
    }
    }
}

static void DeclareInput(
    HLSLCrossCompilerContext* psContext,
    const Declaration* psDecl, const char* StorageQualifier, OPERAND_MIN_PRECISION minPrecision, int iNumComponents, OPERAND_INDEX_DIMENSION eIndexDim, const char* InputName)
{
    ShaderData* psShader = psContext->psShader;
    psContext->currentShaderString = &psContext->parameterDeclarations;
    bstring metal = *psContext->currentShaderString;

    // This falls within the specified index ranges. The default is 0 if no input range is specified
    if (psShader->aIndexedInput[psDecl->asOperands[0].ui32RegisterNumber] == -1)
    {
        return;
    }

    if (psShader->aiInputDeclaredSize[psDecl->asOperands[0].ui32RegisterNumber] == 0)
    {
        
        InOutSignature* psSignature = NULL;
        int emptyQualifier = 0;

        const char* type = "float";
        if (minPrecision == OPERAND_MIN_PRECISION_FLOAT_16)
        {
            type = "half";
        }
        if (GetInputSignatureFromRegister(psDecl->asOperands[0].ui32RegisterNumber, &psShader->sInfo, &psSignature))
        {
            switch (psSignature->eComponentType)
            {
            case INOUT_COMPONENT_UINT32:
            {
                type = "uint";
                break;
            }
            case INOUT_COMPONENT_SINT32:
            {
                type = "int";
                break;
            }
            case INOUT_COMPONENT_FLOAT32:
            {
                break;
            }
            }
        }

        bstring qual = bfromcstralloc(256, StorageQualifier);

        if (biseqcstr(qual, "attribute"))
        {
            bformata(qual, "(%d)", psDecl->asOperands[0].ui32RegisterNumber);
            psContext->currentShaderString = &psContext->stagedInputDeclarations;
            metal = *psContext->currentShaderString;
        }
        else if (biseqcstr(qual, "user"))
        {
            bformata(qual, "(varying%d)", psDecl->asOperands[0].ui32RegisterNumber);
            psContext->currentShaderString = &psContext->stagedInputDeclarations;
            metal = *psContext->currentShaderString;
        }
        else if (biseqcstr(qual, "buffer"))
        {
            bformata(qual, "(%d)", psDecl->asOperands[0].ui32RegisterNumber);
        }

        if (metal == psContext->stagedInputDeclarations)
        {
            bformata(metal, "\t%s", type);
            if (iNumComponents > 1)
            {
                bformata(metal, "%d", iNumComponents);
            }
        }
        else
        {
            if (iNumComponents > 1)
            {
                bformata(metal, "\tdevice %s%d*", type, iNumComponents);
            }
            else
            {
                bformata(metal, "\tdevice %s*", type, iNumComponents);
            }
        }


        if (psDecl->asOperands[0].eType == OPERAND_TYPE_SPECIAL_TEXCOORD)
        {
            InputName = "TexCoord";
        }

        bformata(metal, " %s", InputName);

        switch (eIndexDim)
        {
        case INDEX_2D:
        {
            if (iNumComponents == 1)
            {
                psContext->psShader->abScalarInput[psDecl->asOperands[0].ui32RegisterNumber] = -1;
            }

            const uint32_t regNum = psDecl->asOperands[0].ui32RegisterNumber;
            const uint32_t arraySize = psDecl->asOperands[0].aui32ArraySizes[0];

            bformata(metal, " [%d]", arraySize);

            psShader->aiInputDeclaredSize[psDecl->asOperands[0].ui32RegisterNumber] = arraySize;
            break;
        }
        default:
        {
            if (iNumComponents == 1)
            {
                psContext->psShader->abScalarInput[psDecl->asOperands[0].ui32RegisterNumber] = 1;
            }
            else
            {
                if (psShader->aIndexedInput[psDecl->asOperands[0].ui32RegisterNumber] > 0)
                {
                    bformata(metal, "[%d]", type, iNumComponents, InputName,
                        psShader->aIndexedInput[psDecl->asOperands[0].ui32RegisterNumber]);

                    psShader->aiInputDeclaredSize[psDecl->asOperands[0].ui32RegisterNumber] = psShader->aIndexedInput[psDecl->asOperands[0].ui32RegisterNumber];
                }
                else
                {
                    psShader->aiInputDeclaredSize[psDecl->asOperands[0].ui32RegisterNumber] = -1;
                }
            }
            break;
        }
        }

        if (blength(qual) > 0)
        {
            bformata(metal, " [[ %s ]]", bdata(qual));
        }
        bdestroy(qual);

        bformata(metal, "%c\n", (metal == psContext->stagedInputDeclarations) ? ';' : ',');

        if (psShader->abInputReferencedByInstruction[psDecl->asOperands[0].ui32RegisterNumber])
        {
            const char* stageInString = (metal == psContext->stagedInputDeclarations) ? "stageIn." : "";
            const char* bufferAccessString = (metal == psContext->stagedInputDeclarations) ? "" : "[vId]";

            psContext->currentShaderString = &psContext->earlyMain;
            metal = *psContext->currentShaderString;
            psContext->indent++;

            if (psShader->aiInputDeclaredSize[psDecl->asOperands[0].ui32RegisterNumber] == -1) //Not an array
            {
                AddIndentation(psContext);
                bformata(metal, "%s%d Input%d = %s%s%s;\n", type, iNumComponents,
                    psDecl->asOperands[0].ui32RegisterNumber, stageInString, InputName, bufferAccessString);
            }
            else
            {
                int arrayIndex = psShader->aiInputDeclaredSize[psDecl->asOperands[0].ui32RegisterNumber];
                bformata(metal, "%s%d Input%d[%d];\n", type, iNumComponents, psDecl->asOperands[0].ui32RegisterNumber,
                    psShader->aIndexedInput[psDecl->asOperands[0].ui32RegisterNumber]);

                while (arrayIndex)
                {
                    AddIndentation(psContext);
                    bformata(metal, "Input%d[%d] = %s%s%s[%d];\n", psDecl->asOperands[0].ui32RegisterNumber, arrayIndex - 1,
                        stageInString, InputName, bufferAccessString, arrayIndex - 1);

                    arrayIndex--;
                }
            }
            psContext->indent--;
        }
    }
    psContext->currentShaderString = &psContext->mainShader;
}

static void AddBuiltinInputMETAL(HLSLCrossCompilerContext* psContext, const Declaration* psDecl, const char* builtinName, const char* type)
{
    psContext->currentShaderString = &psContext->stagedInputDeclarations;
    bstring metal = *psContext->currentShaderString;
    ShaderData* psShader = psContext->psShader;
    char* InputName = GetDeclaredInputNameMETAL(psContext, PIXEL_SHADER, &psDecl->asOperands[0]);

    if (psShader->aiInputDeclaredSize[psDecl->asOperands[0].ui32RegisterNumber] == 0)
    {
        // CONFETTI NOTE: DAVID SROUR
        // vertex_id and instance_id must be part of the function's params -- not part of stage_in!
        if (psDecl->asOperands[0].eSpecialName == NAME_INSTANCE_ID || psDecl->asOperands[0].eSpecialName == NAME_VERTEX_ID)
        {
            bformata(psContext->parameterDeclarations, "\t%s %s [[ %s ]],\n", type, &psDecl->asOperands[0].pszSpecialName, builtinName);
        }
        else
        {
            bformata(metal, "\t%s %s [[ %s ]];\n", type, InputName, builtinName);
        }

        psShader->aiInputDeclaredSize[psDecl->asOperands[0].ui32RegisterNumber] = 1;
    }

    if (psShader->abInputReferencedByInstruction[psDecl->asOperands[0].ui32RegisterNumber])
    {
        psContext->currentShaderString = &psContext->earlyMain;
        metal = *psContext->currentShaderString;
        psContext->indent++;
        AddIndentation(psContext);

        if (psDecl->asOperands[0].eSpecialName == NAME_INSTANCE_ID || psDecl->asOperands[0].eSpecialName == NAME_VERTEX_ID)
        {
            bformata(metal, "uint4 ");
            bformata(metal, "Input%d; Input%d.x = %s;\n",
                psDecl->asOperands[0].ui32RegisterNumber, psDecl->asOperands[0].ui32RegisterNumber, &psDecl->asOperands[0].pszSpecialName);
        }
        else if (!strcmp(type, "bool"))
        {
            bformata(metal, "int4 ");
            bformata(metal, "Input%d; Input%d.x = stageIn.%s;\n",
                psDecl->asOperands[0].ui32RegisterNumber, psDecl->asOperands[0].ui32RegisterNumber, InputName);
        }
        else if (!strcmp(type, "float"))
        {
            bformata(metal, "float4 ");
            bformata(metal, "Input%d; Input%d.x = stageIn.%s;\n",
                psDecl->asOperands[0].ui32RegisterNumber, psDecl->asOperands[0].ui32RegisterNumber, InputName);
        }
        else if (!strcmp(type, "int"))
        {
            bformata(metal, "int4 ");
            bformata(metal, "Input%d; Input%d.x = stageIn.%s;\n",
                psDecl->asOperands[0].ui32RegisterNumber, psDecl->asOperands[0].ui32RegisterNumber, InputName);
        }
        else if (!strcmp(type, "uint"))
        {
            bformata(metal, "uint4 ");
            bformata(metal, "Input%d; Input%d.x = stageIn.%s;\n",
                psDecl->asOperands[0].ui32RegisterNumber, psDecl->asOperands[0].ui32RegisterNumber, InputName);
        }
        else
        {
            bformata(metal, "%s Input%d = stageIn.%s;\n", type,
                psDecl->asOperands[0].ui32RegisterNumber, InputName);
        }

        if (psDecl->asOperands[0].eSpecialName == NAME_POSITION)
        {
            if (psContext->psShader->eShaderType == PIXEL_SHADER)
            {
                if (psDecl->asOperands[0].eSelMode == OPERAND_4_COMPONENT_MASK_MODE &&
                    psDecl->asOperands[0].eType == OPERAND_TYPE_INPUT)
                {
                    if (psDecl->asOperands[0].ui32CompMask & OPERAND_4_COMPONENT_MASK_W)
                    {
                        bformata(metal, "Input%d.w = 1.0 / Input%d.w;", psDecl->asOperands[0].ui32RegisterNumber, psDecl->asOperands[0].ui32RegisterNumber);
                    }
                }
            }
        }

        psContext->indent--;
    }
    bcstrfree(InputName);

    psContext->currentShaderString = &psContext->mainShader;
}

int OutputNeedsDeclaringMETAL(HLSLCrossCompilerContext* psContext, const Operand* psOperand, const int count)
{
    ShaderData* psShader = psContext->psShader;
    
    // Depth Output operands are a special case and won't have a ui32RegisterNumber,
    // so first we have to check if the output operand is depth.
    if (psShader->eShaderType == PIXEL_SHADER)
    {
        if (psOperand->eType == OPERAND_TYPE_OUTPUT_DEPTH_GREATER_EQUAL ||
            psOperand->eType == OPERAND_TYPE_OUTPUT_DEPTH_LESS_EQUAL ||
            psOperand->eType == OPERAND_TYPE_OUTPUT_DEPTH)
        {
            return 1;
        }
    }
    
    const uint32_t declared = ((psContext->currentPhase + 1) << 3) | psShader->ui32CurrentVertexOutputStream;
    ASSERT(psOperand->ui32RegisterNumber >= 0);
    ASSERT(psOperand->ui32RegisterNumber < MAX_SHADER_VEC4_OUTPUT);
    if (psShader->aiOutputDeclared[psOperand->ui32RegisterNumber] != declared)
    {
        int offset;

        for (offset = 0; offset < count; offset++)
        {
            psShader->aiOutputDeclared[psOperand->ui32RegisterNumber + offset] = declared;
        }
        return 1;
    }

    return 0;
}

void AddBuiltinOutputMETAL(HLSLCrossCompilerContext* psContext, const Declaration* psDecl, const GLVARTYPE type, int arrayElements, const char* builtinName)
{
    (void)type;

    bstring metal = *psContext->currentShaderString;
    ShaderData* psShader = psContext->psShader;

    psContext->havePostShaderCode[psContext->currentPhase] = 1;

    if (OutputNeedsDeclaringMETAL(psContext, &psDecl->asOperands[0], arrayElements ? arrayElements : 1))
    {
        char* OutputName = GetDeclaredOutputNameMETAL(psContext, VERTEX_SHADER, &psDecl->asOperands[0]);
        psContext->currentShaderString = &psContext->declaredOutputs;
        metal = *psContext->currentShaderString;
        InOutSignature* psSignature = NULL;

        int regNum = psDecl->asOperands[0].ui32RegisterNumber;

        GetOutputSignatureFromRegister(psContext->currentPhase, regNum,
            psDecl->asOperands[0].ui32CompMask,
            0,
            &psShader->sInfo, &psSignature);

        if (psDecl->asOperands[0].eSpecialName == NAME_CLIP_DISTANCE)
        {
            int max = GetMaxComponentFromComponentMaskMETAL(&psDecl->asOperands[0]);
            bformata(metal, "\tfloat %s [%d] [[ %s ]];\n", builtinName, max, builtinName);
        }
        else
        {
            bformata(metal, "\tfloat4 %s [[ %s ]];\n", builtinName, builtinName);
        }
        bformata(metal, "#define Output%d output.%s\n", regNum, builtinName);

        psContext->currentShaderString = &psContext->mainShader;
    }
}

void AddUserOutputMETAL(HLSLCrossCompilerContext* psContext, const Declaration* psDecl)
{
    psContext->currentShaderString = &psContext->declaredOutputs;
    bstring metal = *psContext->currentShaderString;
    ShaderData* psShader = psContext->psShader;

    if (OutputNeedsDeclaringMETAL(psContext, &psDecl->asOperands[0], 1))
    {
        const Operand* psOperand = &psDecl->asOperands[0];
        const char* type = "\tfloat";
        const SHADER_VARIABLE_TYPE eOutType = GetOperandDataTypeMETAL(psContext, &psDecl->asOperands[0]);

        switch (eOutType)
        {
        case SVT_UINT:
        {
            type = "\tuint";
            break;
        }
        case SVT_INT:
        {
            type = "\tint";
            break;
        }
        case SVT_FLOAT16:
        {
            type = "\thalf";
            break;
        }
        case SVT_FLOAT:
        {
            break;
        }
        }

        switch (psShader->eShaderType)
        {
        case PIXEL_SHADER:
        {
            switch (psDecl->asOperands[0].eType)
            {
            case OPERAND_TYPE_OUTPUT_COVERAGE_MASK:
            {
                break;
            }
            case OPERAND_TYPE_OUTPUT_DEPTH:
            {
                bformata(metal, "%s PixOutDepthAny [[ depth(any) ]];\n", type);
                bformata(metal, "#define DepthAny output.PixOutDepthAny\n");
                break;
            }
            case OPERAND_TYPE_OUTPUT_DEPTH_GREATER_EQUAL:
            {
                bformata(metal, "%s PixOutDepthGreater [[ depth(greater) ]];\n", type);
                bformata(metal, "#define DepthGreater output.PixOutDepthGreater\n");
                break;
            }
            case OPERAND_TYPE_OUTPUT_DEPTH_LESS_EQUAL:
            {
                bformata(metal, "%s PixOutDepthLess [[ depth(less) ]];\n", type);
                bformata(metal, "#define DepthLess output.PixOutDepthLess\n");
                break;
            }
            default:
            {
                uint32_t renderTarget = psDecl->asOperands[0].ui32RegisterNumber;

                if (!psContext->gmemOutputNumElements[psDecl->asOperands[0].ui32RegisterNumber])
                {
                    bformata(metal, "%s4 PixOutColor%d [[ color(%d) ]];\n", type, renderTarget, renderTarget);
                }
                else // GMEM output type must match the input!
                {
                    bformata(metal, "float%d PixOutColor%d [[ color(%d) ]];\n", psContext->gmemOutputNumElements[psDecl->asOperands[0].ui32RegisterNumber], renderTarget, renderTarget);
                }
                bformata(metal, "#define Output%d output.PixOutColor%d\n", psDecl->asOperands[0].ui32RegisterNumber, renderTarget);

                break;
            }
            }
            break;
        }
        case VERTEX_SHADER:
        {
            int iNumComponents = 4;//GetMaxComponentFromComponentMaskMETAL(&psDecl->asOperands[0]);
            const char* Interpolation = "";
            int stream = 0;
            char* OutputName = GetDeclaredOutputNameMETAL(psContext, VERTEX_SHADER, psOperand);

            bformata(metal, "%s%d %s [[ user(varying%d) ]];\n", type, iNumComponents, OutputName, psDecl->asOperands[0].ui32RegisterNumber);
            bformata(metal, "#define Output%d output.%s\n", psDecl->asOperands[0].ui32RegisterNumber, OutputName);
            bcstrfree(OutputName);

            break;
        }
        }
    }

    psContext->currentShaderString = &psContext->mainShader;
}

void DeclareBufferVariableMETAL(HLSLCrossCompilerContext* psContext, const uint32_t ui32BindingPoint,
    ConstantBuffer* psCBuf, const Operand* psOperand,
    const ResourceType eResourceType,
    bstring metal, AtomicVarList* psAtomicList)
{
    (void)ui32BindingPoint;

    bstring StructName;
    uint32_t unnamed_struct = strcmp(psCBuf->asVars[0].Name, "$Element") == 0 ? 1 : 0;

    ASSERT(psCBuf->ui32NumVars == 1);
    ASSERT(unnamed_struct);

    StructName = bfromcstr("");

    //TranslateOperandMETAL(psContext, psOperand, TO_FLAG_NAME_ONLY);
    if (psOperand->eType == OPERAND_TYPE_RESOURCE && eResourceType == RTYPE_STRUCTURED)
    {
        ResourceNameMETAL(StructName, psContext, RGROUP_TEXTURE, psOperand->ui32RegisterNumber, 0);
    }
    else if (psOperand->eType == OPERAND_TYPE_RESOURCE && eResourceType == RTYPE_UAV_RWBYTEADDRESS)
    {
        bformata(StructName, "RawRes%d", psOperand->ui32RegisterNumber);
    }
    else
    {
        ResourceNameMETAL(StructName, psContext, RGROUP_UAV, psOperand->ui32RegisterNumber, 0);
    }

    PreDeclareStructTypeMETAL(metal,
        bstr2cstr(StructName, '\0'),
        &psCBuf->asVars[0].sType, psAtomicList);


    bcatcstr(psContext->parameterDeclarations, "\t");
    if (eResourceType == RTYPE_STRUCTURED)
    {
        bcatcstr(psContext->parameterDeclarations, "constant ");
    }
    else
    {
        bcatcstr(psContext->parameterDeclarations, "device ");
    }


    DeclareConstBufferShaderVariableMETAL(psContext->parameterDeclarations,
        bstr2cstr(StructName, '\0'),
        &psCBuf->asVars[0].sType,
        1, 0, psAtomicList);
    if (eResourceType == RTYPE_UAV_RWSTRUCTURED)
    {
        //If it is UAV raw structured, let Metal compiler assign it with the first available location index
        bformata(psContext->parameterDeclarations, " [[ buffer(%d) ]],\n", psOperand->ui32RegisterNumber + UAV_BUFFER_START_SLOT);
        //modify the reflection data to match the binding index
        int count = 0;
        for (uint32_t index = 0; index < psContext->psShader->sInfo.ui32NumResourceBindings; index++)
        {
            if (strcmp(psContext->psShader->sInfo.psResourceBindings[index].Name, (const char*)StructName->data) == 0)
            {
                count++;
                //psContext->psShader->sInfo.psResourceBindings[index].ui32BindPoint += UAV_BUFFER_START_SLOT;
                psContext->psShader->sInfo.psResourceBindings[index].eBindArea = UAVAREA_CBUFFER;
            }
        }
        //If count >2, the logic here is wrong and need to be modified.
        ASSERT(count < 2);
    }
    else
    {
        bformata(psContext->parameterDeclarations, " [[ buffer(%d) ]],\n", psOperand->ui32RegisterNumber);
    }

    bdestroy(StructName);
}

static uint32_t ComputeVariableTypeSize(const ShaderVarType* psType)
{
    if (psType->Class == SVC_STRUCT)
    {
        uint32_t i;
        uint32_t size = 0;
        for (i = 0; i < psType->MemberCount; ++i)
        {
            size += ComputeVariableTypeSize(&psType->Members[i]);
        }

        if (psType->Elements > 1)
        {
            return size * psType->Elements;
        }
        else
        {
            return size;
        }
    }
    else if (psType->Class == SVC_MATRIX_COLUMNS || psType->Class == SVC_MATRIX_ROWS)
    {
        if (psType->Elements > 1)
        {
            return psType->Rows * psType->Elements;
        }
        else
        {
            return psType->Rows;
        }
    }
    else
    if (psType->Class == SVC_VECTOR)
    {
        if (psType->Elements > 1)
        {
            return psType->Elements;
        }
        else
        {
            return 1;
        }
    }

    return 1;
}


void DeclareStructConstantsMETAL(HLSLCrossCompilerContext* psContext, const uint32_t ui32BindingPoint,
    ConstantBuffer* psCBuf, const Operand* psOperand,
    bstring metal, AtomicVarList* psAtomicList)
{
    (void)psOperand;

    uint32_t i;
    const char* StageName = "VS";
    uint32_t nextBufferRegister = 0;
    uint32_t numDummyBuffers = 0;

    for (i = 0; i < psCBuf->ui32NumVars; ++i)
    {
        PreDeclareStructTypeMETAL(metal,
            psCBuf->asVars[i].sType.Name,
            &psCBuf->asVars[i].sType, psAtomicList);
    }

    switch (psContext->psShader->eShaderType)
    {
    case PIXEL_SHADER:
    {
        StageName = "PS";
        break;
    }
    case COMPUTE_SHADER:
    {
        StageName = "CS";
        break;
    }
    default:
    {
        break;
    }
    }

    bformata(metal, "struct %s%s_Type {\n", psCBuf->Name, StageName);

    for (i = 0; i < psCBuf->ui32NumVars; ++i)
    {
        uint32_t ui32RegNum = psCBuf->asVars[i].ui32StartOffset / 16;
        if (ui32RegNum > nextBufferRegister)
        {
            bformata(metal, "\tfloat4 offsetDummy_%d[%d];\n", numDummyBuffers++, ui32RegNum - nextBufferRegister);
        }

        DeclareConstBufferShaderVariableMETAL(metal,
            psCBuf->asVars[i].sType.Name,
            &psCBuf->asVars[i].sType, 0, i < psCBuf->ui32NumVars - 1, psAtomicList);

        uint32_t varSize = ComputeVariableTypeSize(&psCBuf->asVars[i].sType);
        nextBufferRegister = ui32RegNum + varSize;
    }

    bcatcstr(metal, "};\n");

    bcatcstr(psContext->parameterDeclarations, "\tconstant ");
    bformata(psContext->parameterDeclarations, "%s%s_Type ", psCBuf->Name, StageName);
    bcatcstr(psContext->parameterDeclarations, "& ");

    bformata(psContext->parameterDeclarations, "%s%s_In", psCBuf->Name, StageName);
    bformata(psContext->parameterDeclarations, " [[ buffer(%d) ]],\n", ui32BindingPoint);

    for (i = 0; i < psCBuf->ui32NumVars; ++i)
    {
        const struct ShaderVarType_TAG* psType = &psCBuf->asVars[i].sType;
        const char* Name = psCBuf->asVars[i].sType.Name;
        const char* addressSpace = "constant";

        if (psType->Class == SVC_STRUCT)
        {
            bformata(psContext->earlyMain, "\t%s %s_Type%s const &%s", addressSpace, Name, psType->Elements > 1 ? "*" : "", Name);
        }
        else if (psType->Class == SVC_MATRIX_COLUMNS || psType->Class == SVC_MATRIX_ROWS)
        {
            switch (psType->Type)
            {
            case SVT_FLOAT:
            {
                bformata(psContext->earlyMain, "\t%s float%d%s const &%s", addressSpace, psType->Columns, "*", Name, psType->Rows);
                break;
            }
            case SVT_FLOAT16:
            {
                bformata(psContext->earlyMain, "\t%s half%d%s const &%s", addressSpace, psType->Columns, "*", Name, psType->Rows);
                break;
            }
            default:
            {
                ASSERT(0);
                break;
            }
            }
        }
        else
        if (psType->Class == SVC_VECTOR)
        {
            switch (psType->Type)
            {
            case SVT_FLOAT:
            case SVT_DOUBLE:     // double is not supported in metal
            {
                bformata(psContext->earlyMain, "\t%s float%d%s const &%s", addressSpace, psType->Columns, psType->Elements > 1 ? "*" : "", Name);
                break;
            }
            case SVT_FLOAT16:    
            {
                bformata(psContext->earlyMain, "\t%s half%d%s const &%s", addressSpace, psType->Columns, psType->Elements > 1 ? "*" : "", Name);
                break;
            }
            case SVT_UINT:
            {
                bformata(psContext->earlyMain, "\t%s uint%d%s const &%s", addressSpace, psType->Columns, psType->Elements > 1 ? "*" : "", Name);
                break;
            }
            case SVT_INT:
            {
                bformata(psContext->earlyMain, "\t%s int%d%s const &%s", addressSpace, psType->Columns, psType->Elements > 1 ? "*" : "", Name);
                break;
            }
            default:
            {
                ASSERT(0);
                break;
            }
            }
        }
        else
        if (psType->Class == SVC_SCALAR)
        {
            switch (psType->Type)
            {
            case SVT_FLOAT:
            case SVT_DOUBLE:         // double is not supported in metal
            {
                bformata(psContext->earlyMain, "\t%s float%s const &%s", addressSpace, psType->Elements > 1 ? "*" : "", Name);
                break;
            }
            case SVT_FLOAT16:            
            {
                bformata(psContext->earlyMain, "\t%s half%s const &%s", addressSpace, psType->Elements > 1 ? "*" : "", Name);
                break;
            }
            case SVT_UINT:
            {
                bformata(psContext->earlyMain, "\t%s uint%s const &%s", addressSpace, psType->Elements > 1 ? "*" : "", Name);
                break;
            }
            case SVT_INT:
            {
                bformata(psContext->earlyMain, "\t%s int%s const &%s", addressSpace, psType->Elements > 1 ? "*" : "", Name);
                break;
            }
            case SVT_BOOL:
            {
                //Use int instead of bool.
                //Allows implicit conversions to integer
                bformata(psContext->earlyMain, "\t%s int%s const &%s", addressSpace, psType->Elements > 1 ? "*" : "", Name);
                break;
            }
            default:
            {
                ASSERT(0);
                break;
            }
            }
        }

        bformata(psContext->earlyMain, " = %s%s_In.%s;\n", psCBuf->Name, StageName, psCBuf->asVars[i].sType.Name);
    }
}

char* GetSamplerTypeMETAL(HLSLCrossCompilerContext* psContext,
    const RESOURCE_DIMENSION eDimension,
    const uint32_t ui32RegisterNumber, const uint32_t isShadow)
{
    ResourceBinding* psBinding = 0;
    RESOURCE_RETURN_TYPE eType = RETURN_TYPE_UNORM;
    int found;
    found = GetResourceFromBindingPoint(RGROUP_TEXTURE, ui32RegisterNumber, &psContext->psShader->sInfo, &psBinding);
    if (found)
    {
        eType = (RESOURCE_RETURN_TYPE)psBinding->ui32ReturnType;
    }
    switch (eDimension)
    {
    case RESOURCE_DIMENSION_BUFFER:
    {
        switch (eType)
        {
        case RETURN_TYPE_SINT:
            return "";
        case RETURN_TYPE_UINT:
            return "";
        default:
            return "";
        }
        break;
    }

    case RESOURCE_DIMENSION_TEXTURE1D:
    {
        switch (eType)
        {
        case RETURN_TYPE_SINT:
            return "\ttexture1d<int>";
        case RETURN_TYPE_UINT:
            return "\ttexture1d<uint>";
        default:
            return "\ttexture1d<float>";
        }
        break;
    }

    case RESOURCE_DIMENSION_TEXTURE2D:
    {
        if (isShadow)
        {
            return "\tdepth2d<float>";
        }

        switch (eType)
        {
        case RETURN_TYPE_SINT:
            return "\ttexture2d<int>";
        case RETURN_TYPE_UINT:
            return "\ttexture2d<uint>";
        default:
            return "\ttexture2d<float>";
        }
        break;
    }

    case RESOURCE_DIMENSION_TEXTURE2DMS:
    {
        if (isShadow)
        {
            return "\tdepth2d_ms<float>";
        }

        switch (eType)
        {
        case RETURN_TYPE_SINT:
            return "\ttexture2d_ms<int>";
        case RETURN_TYPE_UINT:
            return "\ttexture2d_ms<uint>";
        default:
            return "\ttexture2d_ms<float>";
        }
        break;
    }

    case RESOURCE_DIMENSION_TEXTURE3D:
    {
        switch (eType)
        {
        case RETURN_TYPE_SINT:
            return "\ttexture3d<int>";
        case RETURN_TYPE_UINT:
            return "\ttexture3d<uint>";
        default:
            return "\ttexture3d<float>";
        }
        break;
    }

    case RESOURCE_DIMENSION_TEXTURECUBE:
    {
        if (isShadow)
        {
            return "\tdepthcube<float>";
        }

        switch (eType)
        {
        case RETURN_TYPE_SINT:
            return "\ttexturecube<int>";
        case RETURN_TYPE_UINT:
            return "\ttexturecube<uint>";
        default:
            return "\ttexturecube<float>";
        }
        break;
    }

    case RESOURCE_DIMENSION_TEXTURE1DARRAY:
    {
        switch (eType)
        {
        case RETURN_TYPE_SINT:
            return "\ttexture1d_array<int>";
        case RETURN_TYPE_UINT:
            return "\ttexture1d_array<uint>";
        default:
            return "\ttexture1d_array<float>";
        }
        break;
    }

    case RESOURCE_DIMENSION_TEXTURE2DARRAY:
    {
        if (isShadow)
        {
            return "\tdepth2d_array<float>";
        }

        switch (eType)
        {
        case RETURN_TYPE_SINT:
            return "\ttexture2d_array<int>";
        case RETURN_TYPE_UINT:
            return "\ttexture2d_array<uint>";
        default:
            return "\ttexture2d_array<float>";
        }
        break;
    }

    case RESOURCE_DIMENSION_TEXTURE2DMSARRAY:
    {
        //Metal does not support this type of resource
        ASSERT(0);
        switch (eType)
        {
        case RETURN_TYPE_SINT:
            return "";
        case RETURN_TYPE_UINT:
            return "";
        default:
            return "";
        }
        break;
    }

    case RESOURCE_DIMENSION_TEXTURECUBEARRAY:
    {
        switch (eType)
        {
        case RETURN_TYPE_SINT:
            return "\ttexturecube_array<int>";
        case RETURN_TYPE_UINT:
            return "\ttexturecube_array<uint>";
        default:
            return "\ttexturecube_array<float>";
        }
        break;
    }
    }

    return "sampler2D";
}

static void TranslateResourceTexture(HLSLCrossCompilerContext* psContext, const Declaration* psDecl, uint32_t samplerCanDoShadowCmp)
{
    bstring metal = *psContext->currentShaderString;
    ShaderData* psShader = psContext->psShader;

    const char* samplerTypeName = GetSamplerTypeMETAL(psContext,
            psDecl->value.eResourceDimension,
            psDecl->asOperands[0].ui32RegisterNumber, samplerCanDoShadowCmp && psDecl->ui32IsShadowTex);

    if (samplerCanDoShadowCmp && psDecl->ui32IsShadowTex)
    {
        //Create shadow and non-shadow sampler.
        //HLSL does not have separate types for depth compare, just different functions.
        bcatcstr(metal, samplerTypeName);
        bcatcstr(metal, " ");
        ResourceNameMETAL(metal, psContext, RGROUP_TEXTURE, psDecl->asOperands[0].ui32RegisterNumber, 1);
    }
    else
    {
        bcatcstr(metal, samplerTypeName);
        bcatcstr(metal, " ");
        ResourceNameMETAL(metal, psContext, RGROUP_TEXTURE, psDecl->asOperands[0].ui32RegisterNumber, 0);
    }
}

void TranslateDeclarationMETAL(HLSLCrossCompilerContext* psContext, const Declaration* psDecl, AtomicVarList* psAtomicList)
{
    bstring metal = *psContext->currentShaderString;
    ShaderData* psShader = psContext->psShader;

    switch (psDecl->eOpcode)
    {
    case OPCODE_DCL_INPUT_SGV:
    case OPCODE_DCL_INPUT_PS_SGV:
    {
        const SPECIAL_NAME eSpecialName = psDecl->asOperands[0].eSpecialName;

        if (psShader->eShaderType == PIXEL_SHADER)
        {
            switch (eSpecialName)
            {
            case NAME_POSITION:
            {
                AddBuiltinInputMETAL(psContext, psDecl, "position", "float4");
                break;
            }
            case NAME_CLIP_DISTANCE:
            {
                AddBuiltinInputMETAL(psContext, psDecl, "clip_distance", "float");
                break;
            }
            case NAME_INSTANCE_ID:
            {
                AddBuiltinInputMETAL(psContext, psDecl, "instance_id", "uint");
                break;
            }
            case NAME_IS_FRONT_FACE:
            {
                /*
                Cast to int used because
                if(gl_FrontFacing != 0) failed to compiled on Intel HD 4000.
                Suggests no implicit conversion for bool<->int.
                */

                AddBuiltinInputMETAL(psContext, psDecl, "front_facing", "bool");
                break;
            }
            case NAME_SAMPLE_INDEX:
            {
                AddBuiltinInputMETAL(psContext, psDecl, "sample_id", "uint");
                break;
            }
            default:
            {
                DeclareInput(psContext, psDecl,
                    "user", OPERAND_MIN_PRECISION_DEFAULT, 4, INDEX_1D, psDecl->asOperands[0].pszSpecialName);
            }
            }
        }
        else if (psShader->eShaderType == VERTEX_SHADER)
        {
            switch (eSpecialName)
            {
            case NAME_VERTEX_ID:
            {
                AddBuiltinInputMETAL(psContext, psDecl, "vertex_id", "uint");
                break;
            }
            case NAME_INSTANCE_ID:
            {
                AddBuiltinInputMETAL(psContext, psDecl, "instance_id", "uint");
                break;
            }
            default:
            {
                DeclareInput(psContext, psDecl,
                    "attribute", OPERAND_MIN_PRECISION_DEFAULT, 4, INDEX_1D, psDecl->asOperands[0].pszSpecialName);
            }
            }
        }
        break;
    }

    case OPCODE_DCL_OUTPUT_SIV:
    {
        switch (psDecl->asOperands[0].eSpecialName)
        {
        case NAME_POSITION:
        {
            AddBuiltinOutputMETAL(psContext, psDecl, GLVARTYPE_FLOAT4, 0, "position");
            break;
        }
        case NAME_CLIP_DISTANCE:
        {
            AddBuiltinOutputMETAL(psContext, psDecl, GLVARTYPE_FLOAT, 0, "clip_distance");
            break;
        }
        case NAME_VERTEX_ID:
        {
            ASSERT(0); //VertexID is not an output
            break;
        }
        case NAME_INSTANCE_ID:
        {
            ASSERT(0); //InstanceID is not an output
            break;
        }
        case NAME_IS_FRONT_FACE:
        {
            ASSERT(0); //FrontFacing is not an output
            break;
        }
        default:
        {
            bformata(metal, "float4 %s;\n", psDecl->asOperands[0].pszSpecialName);

            bcatcstr(metal, "#define ");
            TranslateOperandMETAL(psContext, &psDecl->asOperands[0], TO_FLAG_NONE);
            bformata(metal, " %s\n", psDecl->asOperands[0].pszSpecialName);
            break;
        }
        }
        break;
    }
    case OPCODE_DCL_INPUT:
    {
        const Operand* psOperand = &psDecl->asOperands[0];
        //Force the number of components to be 4.
        /*dcl_output o3.xy
        dcl_output o3.z

        Would generate a vec2 and a vec3. We discard the second one making .z invalid!

        */
        int iNumComponents = 4;//GetMaxComponentFromComponentMask(psOperand);
        const char* InputName;

        if ((psOperand->eType == OPERAND_TYPE_INPUT_DOMAIN_POINT) ||
            (psOperand->eType == OPERAND_TYPE_OUTPUT_CONTROL_POINT_ID) ||
            (psOperand->eType == OPERAND_TYPE_INPUT_COVERAGE_MASK) ||
            (psOperand->eType == OPERAND_TYPE_INPUT_FORK_INSTANCE_ID))
        {
            break;
        }
        if (psOperand->eType == OPERAND_TYPE_INPUT_THREAD_ID)
        {
            bformata(psContext->parameterDeclarations, "\tuint3 vThreadID [[ thread_position_in_grid ]],\n");
            break;
        }
        if (psOperand->eType == OPERAND_TYPE_INPUT_THREAD_ID_IN_GROUP)
        {
            bformata(psContext->parameterDeclarations, "\tuint3 vThreadIDInGroup [[ thread_position_in_threadgroup ]],\n");
            break;
        }
        if (psOperand->eType == OPERAND_TYPE_INPUT_THREAD_GROUP_ID)
        {
            bformata(psContext->parameterDeclarations, "\tuint3 vThreadGroupID [[ threadgroup_position_in_grid  ]],\n");
            break;
        }
        if (psOperand->eType == OPERAND_TYPE_INPUT_THREAD_ID_IN_GROUP_FLATTENED)
        {
            bformata(psContext->parameterDeclarations, "\tuint vThreadIDInGroupFlattened [[ thread_index_in_threadgroup ]],\n");
            break;
        }
        //Already declared as part of an array.
        if (psShader->aIndexedInput[psDecl->asOperands[0].ui32RegisterNumber] == -1)
        {
            break;
        }

        InputName = GetDeclaredInputNameMETAL(psContext, psShader->eShaderType, psOperand);

        DeclareInput(psContext, psDecl,
            "attribute", (OPERAND_MIN_PRECISION)psOperand->eMinPrecision, iNumComponents, (OPERAND_INDEX_DIMENSION)psOperand->iIndexDims, InputName);

        break;
    }
    case OPCODE_DCL_INPUT_PS_SIV:
    {
        switch (psDecl->asOperands[0].eSpecialName)
        {
        case NAME_POSITION:
        {
            AddBuiltinInputMETAL(psContext, psDecl, "position", "float4");
            break;
        }
        }
        break;
    }
    case OPCODE_DCL_INPUT_SIV:
    {
        break;
    }
    case OPCODE_DCL_INPUT_PS:
    {
        const Operand* psOperand = &psDecl->asOperands[0];
        int iNumComponents = 4;//GetMaxComponentFromComponentMask(psOperand);
        const char* StorageQualifier = "";
        const char* InputName = GetDeclaredInputNameMETAL(psContext, PIXEL_SHADER, psOperand);
        const char* Interpolation = "";

        DeclareInput(psContext, psDecl,
            "user", (OPERAND_MIN_PRECISION)psOperand->eMinPrecision, iNumComponents, INDEX_1D, InputName);

        break;
    }
    case OPCODE_DCL_TEMPS:
    {
        uint32_t i = 0;
        const uint32_t ui32NumTemps = psDecl->value.ui32NumTemps;

        if (ui32NumTemps > 0)
        {
            bformata(psContext->earlyMain, "\tfloat4 Temp[%d];\n", ui32NumTemps);

            bformata(psContext->earlyMain, "\tint4 Temp_int[%d];\n", ui32NumTemps);
            bformata(psContext->earlyMain, "\tuint4 Temp_uint[%d];\n", ui32NumTemps);
            bformata(psContext->earlyMain, "\thalf4 Temp_half[%d];\n", ui32NumTemps);
        }

        break;
    }
    case OPCODE_SPECIAL_DCL_IMMCONST:
    {
        const Operand* psDest = &psDecl->asOperands[0];
        const Operand* psSrc = &psDecl->asOperands[1];

        ASSERT(psSrc->eType == OPERAND_TYPE_IMMEDIATE32);
        if (psDest->eType == OPERAND_TYPE_SPECIAL_IMMCONSTINT)
        {
            bformata(metal, "const int4 IntImmConst%d = ", psDest->ui32RegisterNumber);
        }
        else
        {
            bformata(metal, "const float4 ImmConst%d = ", psDest->ui32RegisterNumber);
            AddToDx9ImmConstIndexableArrayMETAL(psContext, psDest);
        }
        TranslateOperandMETAL(psContext, psSrc, psDest->eType == OPERAND_TYPE_SPECIAL_IMMCONSTINT ? TO_FLAG_INTEGER : TO_AUTO_BITCAST_TO_FLOAT);
        bcatcstr(metal, ";\n");

        break;
    }
    case OPCODE_DCL_CONSTANT_BUFFER:
    {
        const Operand* psOperand = &psDecl->asOperands[0];
        const uint32_t ui32BindingPoint = psOperand->aui32ArraySizes[0];

        const char* StageName = "VS";

        switch (psContext->psShader->eShaderType)
        {
        case PIXEL_SHADER:
        {
            StageName = "PS";
            break;
        }
        case HULL_SHADER:
        {
            StageName = "HS";
            break;
        }
        case DOMAIN_SHADER:
        {
            StageName = "DS";
            break;
        }
        case GEOMETRY_SHADER:
        {
            StageName = "GS";
            break;
        }
        case COMPUTE_SHADER:
        {
            StageName = "CS";
            break;
        }
        default:
        {
            break;
        }
        }

        ConstantBuffer* psCBuf = NULL;
        GetConstantBufferFromBindingPoint(RGROUP_CBUFFER, ui32BindingPoint, &psContext->psShader->sInfo, &psCBuf);

        if (psCBuf)
        {
            // Constant buffers declared as "dynamicIndexed" are declared as raw vec4 arrays, as there is no general way to retrieve the member corresponding to a dynamic index.
            // Simple cases can probably be handled easily, but for example when arrays (possibly nested with structs) are contained in the constant buffer and the shader reads
            // from a dynamic index we would need to "undo" the operations done in order to compute the variable offset, and such a feature is not available at the moment.
            psCBuf->blob = psDecl->value.eCBAccessPattern == CONSTANT_BUFFER_ACCESS_PATTERN_DYNAMICINDEXED;
        }

        // We don't have a original resource name, maybe generate one???
        if (!psCBuf)
        {
            bformata(metal, "struct ConstantBuffer%d {\n\tfloat4 data[%d];\n};\n", ui32BindingPoint, psOperand->aui32ArraySizes[1], ui32BindingPoint);
            // For vertex shaders HLSLcc generates code that expectes the
            // constant buffer to be a pointer. For other shaders it generates
            // code that expects a reference instead...
            if (psContext->psShader->eShaderType == VERTEX_SHADER)
            {
                bformata(psContext->parameterDeclarations, "\tconstant ConstantBuffer%d* cb%d [[ buffer(%d) ]],\n", ui32BindingPoint, ui32BindingPoint, ui32BindingPoint);
            }
            else
            {
                bformata(psContext->parameterDeclarations, "\tconstant ConstantBuffer%d& cb%d [[ buffer(%d) ]],\n", ui32BindingPoint, ui32BindingPoint, ui32BindingPoint);
            }
            break;
        }
        else if (psCBuf->blob)
        {
            // For vertex shaders HLSLcc generates code that expectes the
            // constant buffer to be a pointer. For other shaders it generates
            // code that expects a reference instead...
            bformata(metal, "struct ConstantBuffer%d {\n\tfloat4 %s[%d];\n};\n", ui32BindingPoint, psCBuf->asVars->Name, psOperand->aui32ArraySizes[1], ui32BindingPoint);
            if (psContext->psShader->eShaderType == VERTEX_SHADER)
            {
                bformata(psContext->parameterDeclarations, "\tconstant ConstantBuffer%d* %s%s_data [[ buffer(%d) ]],\n", ui32BindingPoint, psCBuf->Name, StageName, ui32BindingPoint);
            }
            else
            {
                bformata(psContext->parameterDeclarations, "\tconstant ConstantBuffer%d& %s%s_data [[ buffer(%d) ]],\n", ui32BindingPoint, psCBuf->Name, StageName, ui32BindingPoint);
            }
            break;
        }

        DeclareStructConstantsMETAL(psContext, ui32BindingPoint, psCBuf, psOperand, metal, psAtomicList);

        break;
    }
    case OPCODE_DCL_SAMPLER:
    {
        if (psDecl->bIsComparisonSampler)
        {
            psContext->currentShaderString = &psContext->mainShader;
            metal = *psContext->currentShaderString;

            bcatcstr(metal, "constexpr sampler ");
            ResourceNameMETAL(metal, psContext, RGROUP_SAMPLER, psDecl->asOperands[0].ui32RegisterNumber, 1);
            bformata(metal, "(compare_func::less);\n", psDecl->asOperands[0].ui32RegisterNumber);
        }

        /* CONFETTI NOTE (DAVID SROUR):
         * The following declaration still needs to occur for comparison samplers.
         * The Metal layer of the engine will still try to bind a sampler in the appropriate slot.
         * This parameter of the shader's entrance function acts as a dummy comparison sampler for the engine.
         * Note that 0 is always passed for the "bZCompare" argument of ResourceNameMETAL(...) as to give the dummy
         * sampler a different name as the constexpr one.
         */
        {
            psContext->currentShaderString = &psContext->parameterDeclarations;
            metal = *psContext->currentShaderString;

            bcatcstr(metal, "\tsampler ");
            ResourceNameMETAL(metal, psContext, RGROUP_SAMPLER, psDecl->asOperands[0].ui32RegisterNumber, 0);
            bformata(metal, "[[ sampler(%d) ]],\n", psDecl->asOperands[0].ui32RegisterNumber);
        }
        break;
    }
    case OPCODE_DCL_RESOURCE:
    {
        // CONFETTI BEGIN: David Srour
        // METAL PIXEL SHADER RT FETCH
        if (psDecl->asOperands[0].ui32RegisterNumber >= GMEM_FLOAT_START_SLOT)
        {
            int regNum = GetGmemInputResourceSlotMETAL(psDecl->asOperands[0].ui32RegisterNumber);
            int numElements = GetGmemInputResourceNumElementsMETAL(psDecl->asOperands[0].ui32RegisterNumber);

            switch (numElements)
            {
            case 1:
                bformata(psContext->parameterDeclarations, "\tfloat");
                break;
            case 2:
                bformata(psContext->parameterDeclarations, "\tfloat2");
                break;
            case 3:
                bformata(psContext->parameterDeclarations, "\tfloat3");
                break;
            case 4:
                bformata(psContext->parameterDeclarations, "\tfloat4");
                break;
            default:
                bformata(psContext->parameterDeclarations, "\tfloat4");
                break;
            }

            psContext->gmemOutputNumElements[regNum] = numElements;

            // Function input framebuffer
            bformata(psContext->parameterDeclarations, " GMEM_Input%d [[ color(%d) ]],\n", regNum, regNum);

            break;
        }
        // CONFETTI END

        psContext->currentShaderString = &psContext->parameterDeclarations;
        metal = *psContext->currentShaderString;

        switch (psDecl->value.eResourceDimension)
        {
        case RESOURCE_DIMENSION_BUFFER:
        {
            break;
        }
        case RESOURCE_DIMENSION_TEXTURE1D:
        {
            TranslateResourceTexture(psContext, psDecl, 1);
            break;
        }
        case RESOURCE_DIMENSION_TEXTURE2D:
        {
            TranslateResourceTexture(psContext, psDecl, 1);
            break;
        }
        case RESOURCE_DIMENSION_TEXTURE2DMS:
        {
            TranslateResourceTexture(psContext, psDecl, 0);
            break;
        }
        case RESOURCE_DIMENSION_TEXTURE3D:
        {
            TranslateResourceTexture(psContext, psDecl, 0);
            break;
        }
        case RESOURCE_DIMENSION_TEXTURECUBE:
        {
            TranslateResourceTexture(psContext, psDecl, 1);
            break;
        }
        case RESOURCE_DIMENSION_TEXTURE1DARRAY:
        {
            TranslateResourceTexture(psContext, psDecl, 1);
            break;
        }
        case RESOURCE_DIMENSION_TEXTURE2DARRAY:
        {
            TranslateResourceTexture(psContext, psDecl, 1);
            break;
        }
        case RESOURCE_DIMENSION_TEXTURE2DMSARRAY:
        {
            TranslateResourceTexture(psContext, psDecl, 1);
            break;
        }
        case RESOURCE_DIMENSION_TEXTURECUBEARRAY:
        {
            TranslateResourceTexture(psContext, psDecl, 1);
            break;
        }
        }

        bformata(metal, "[[ texture(%d) ]],\n", psDecl->asOperands[0].ui32RegisterNumber);
        psContext->currentShaderString = &psContext->mainShader;
        metal = *psContext->currentShaderString;

        ASSERT(psDecl->asOperands[0].ui32RegisterNumber < MAX_TEXTURES);
        psShader->aeResourceDims[psDecl->asOperands[0].ui32RegisterNumber] = psDecl->value.eResourceDimension;
        break;
    }
    case OPCODE_DCL_OUTPUT:
    {
        AddUserOutputMETAL(psContext, psDecl);
        break;
    }
    case OPCODE_DCL_GLOBAL_FLAGS:
    {
        uint32_t ui32Flags = psDecl->value.ui32GlobalFlags;

        if (ui32Flags & GLOBAL_FLAG_FORCE_EARLY_DEPTH_STENCIL)
        {
            psContext->needsFragmentTestHint = 1;
        }
        if (!(ui32Flags & GLOBAL_FLAG_REFACTORING_ALLOWED))
        {
            //TODO add precise
            //HLSL precise - http://msdn.microsoft.com/en-us/library/windows/desktop/hh447204(v=vs.85).aspx
        }
        if (ui32Flags & GLOBAL_FLAG_ENABLE_DOUBLE_PRECISION_FLOAT_OPS)
        {
            // TODO
            // Is there something for this in METAL?
        }

        break;
    }

    case OPCODE_DCL_THREAD_GROUP:
    {
        /* CONFETTI NOTE:
        The thread group information need to be passed to engine side. Add the information
        into reflection data.
        */
        psContext->psShader->sInfo.ui32Thread_x = psDecl->value.aui32WorkGroupSize[0];
        psContext->psShader->sInfo.ui32Thread_y = psDecl->value.aui32WorkGroupSize[1];
        psContext->psShader->sInfo.ui32Thread_z = psDecl->value.aui32WorkGroupSize[2];
        break;
    }
    case OPCODE_DCL_TESS_OUTPUT_PRIMITIVE:
    {
        break;
    }
    case OPCODE_DCL_TESS_DOMAIN:
    {
        break;
    }
    case OPCODE_DCL_TESS_PARTITIONING:
    {
        break;
    }
    case OPCODE_DCL_GS_OUTPUT_PRIMITIVE_TOPOLOGY:
    {
        break;
    }
    case OPCODE_DCL_MAX_OUTPUT_VERTEX_COUNT:
    {
        break;
    }
    case OPCODE_DCL_GS_INPUT_PRIMITIVE:
    {
        break;
    }
    case OPCODE_DCL_INTERFACE:
    {
        break;
    }
    case OPCODE_DCL_FUNCTION_BODY:
    {
        //bformata(metal, "void Func%d();//%d\n", psDecl->asOperands[0].ui32RegisterNumber, psDecl->asOperands[0].eType);
        break;
    }
    case OPCODE_DCL_FUNCTION_TABLE:
    {
        break;
    }
    case OPCODE_CUSTOMDATA:
    {
        const uint32_t ui32NumVec4 = psDecl->ui32NumOperands;
        const uint32_t ui32NumVec4Minus1 = (ui32NumVec4 - 1);
        uint32_t ui32ConstIndex = 0;
        float x, y, z, w;

        //If ShaderBitEncodingSupported then 1 integer buffer, use intBitsToFloat to get float values. - More instructions.
        //else 2 buffers - one integer and one float. - More data

        if (ShaderBitEncodingSupported(psShader->eTargetLanguage) == 0)
        {
            bcatcstr(metal, "#define immediateConstBufferI(idx) immediateConstBufferInt[idx]\n");
            bcatcstr(metal, "#define immediateConstBufferF(idx) immediateConstBuffer[idx]\n");

            bformata(metal, "static constant float4 immediateConstBuffer[%d] = {\n", ui32NumVec4, ui32NumVec4);
            for (; ui32ConstIndex < ui32NumVec4Minus1; ui32ConstIndex++)
            {
                float loopLocalX, loopLocalY, loopLocalZ, loopLocalW;
                loopLocalX = *(float*)&psDecl->asImmediateConstBuffer[ui32ConstIndex].a;
                loopLocalY = *(float*)&psDecl->asImmediateConstBuffer[ui32ConstIndex].b;
                loopLocalZ = *(float*)&psDecl->asImmediateConstBuffer[ui32ConstIndex].c;
                loopLocalW = *(float*)&psDecl->asImmediateConstBuffer[ui32ConstIndex].d;

                //A single vec4 can mix integer and float types.
                //Forced NAN and INF to zero inside the immediate constant buffer. This will allow the shader to compile.
                if (fpcheck(loopLocalX))
                {
                    loopLocalX = 0;
                }
                if (fpcheck(loopLocalY))
                {
                    loopLocalY = 0;
                }
                if (fpcheck(loopLocalZ))
                {
                    loopLocalZ = 0;
                }
                if (fpcheck(loopLocalW))
                {
                    loopLocalW = 0;
                }

                bformata(metal, "\tfloat4(%f, %f, %f, %f), \n", loopLocalX, loopLocalY, loopLocalZ, loopLocalW);
            }
            //No trailing comma on this one
            x = *(float*)&psDecl->asImmediateConstBuffer[ui32ConstIndex].a;
            y = *(float*)&psDecl->asImmediateConstBuffer[ui32ConstIndex].b;
            z = *(float*)&psDecl->asImmediateConstBuffer[ui32ConstIndex].c;
            w = *(float*)&psDecl->asImmediateConstBuffer[ui32ConstIndex].d;
            if (fpcheck(x))
            {
                x = 0;
            }
            if (fpcheck(y))
            {
                y = 0;
            }
            if (fpcheck(z))
            {
                z = 0;
            }
            if (fpcheck(w))
            {
                w = 0;
            }
            bformata(metal, "\tfloat4(%f, %f, %f, %f)\n", x, y, z, w);
            bcatcstr(metal, "};\n");
        }
        else
        {
            bcatcstr(metal, "#define immediateConstBufferI(idx) immediateConstBufferInt[idx]\n");
            bcatcstr(metal, "#define immediateConstBufferF(idx) as_type<float4>(immediateConstBufferInt[idx])\n");
        }

        {
            uint32_t ui32ConstIndex2 = 0;
            int x2, y2, z2, w2;

            bformata(metal, "static constant int4 immediateConstBufferInt[%d] = {\n", ui32NumVec4, ui32NumVec4);
            for (; ui32ConstIndex2 < ui32NumVec4Minus1; ui32ConstIndex2++)
            {
                int loopLocalX, loopLocalY, loopLocalZ, loopLocalW;
                loopLocalX = *(int*)&psDecl->asImmediateConstBuffer[ui32ConstIndex2].a;
                loopLocalY = *(int*)&psDecl->asImmediateConstBuffer[ui32ConstIndex2].b;
                loopLocalZ = *(int*)&psDecl->asImmediateConstBuffer[ui32ConstIndex2].c;
                loopLocalW = *(int*)&psDecl->asImmediateConstBuffer[ui32ConstIndex2].d;

                bformata(metal, "\tint4(%d, %d, %d, %d), \n", loopLocalX, loopLocalY, loopLocalZ, loopLocalW);
            }
            //No trailing comma on this one
            x2 = *(int*)&psDecl->asImmediateConstBuffer[ui32ConstIndex2].a;
            y2 = *(int*)&psDecl->asImmediateConstBuffer[ui32ConstIndex2].b;
            z2 = *(int*)&psDecl->asImmediateConstBuffer[ui32ConstIndex2].c;
            w2 = *(int*)&psDecl->asImmediateConstBuffer[ui32ConstIndex2].d;

            bformata(metal, "\tint4(%d, %d, %d, %d)\n", x2, y2, z2, w2);
            bcatcstr(metal, "};\n");
        }

        break;
    }
    case OPCODE_DCL_HS_FORK_PHASE_INSTANCE_COUNT:
    {
        break;
    }
    case OPCODE_DCL_INDEXABLE_TEMP:
    {
        const uint32_t ui32RegIndex = psDecl->sIdxTemp.ui32RegIndex;
        const uint32_t ui32RegCount = psDecl->sIdxTemp.ui32RegCount;
        const uint32_t ui32RegComponentSize = psDecl->sIdxTemp.ui32RegComponentSize;
        bformata(psContext->earlyMain, "float%d TempArray%d[%d];\n", ui32RegComponentSize, ui32RegIndex, ui32RegCount);
        bformata(psContext->earlyMain, "int%d TempArray%d_int[%d];\n", ui32RegComponentSize, ui32RegIndex, ui32RegCount);
        if (HaveUVec(psShader->eTargetLanguage))
        {
            bformata(psContext->earlyMain, "uint%d TempArray%d_uint[%d];\n", ui32RegComponentSize, ui32RegIndex, ui32RegCount);
        }
        break;
    }
    case OPCODE_DCL_INDEX_RANGE:
    {
        break;
    }
    case OPCODE_HS_DECLS:
    {
        break;
    }
    case OPCODE_DCL_INPUT_CONTROL_POINT_COUNT:
    {
        break;
    }
    case OPCODE_DCL_OUTPUT_CONTROL_POINT_COUNT:
    {
        break;
    }
    case OPCODE_HS_FORK_PHASE:
    {
        break;
    }
    case OPCODE_HS_JOIN_PHASE:
    {
        break;
    }
    case OPCODE_DCL_HS_MAX_TESSFACTOR:
    {
        //For metal the max tessellation factor is fixed to the value of gl_MaxTessGenLevel.
        break;
    }
    case OPCODE_DCL_UNORDERED_ACCESS_VIEW_TYPED:
    {
        psContext->currentShaderString = &psContext->parameterDeclarations;
        metal = *psContext->currentShaderString;

        if (psDecl->value.eResourceDimension == RESOURCE_DIMENSION_BUFFER)
        {
            {
                //give write access
                bcatcstr(metal, "\tdevice ");
            }
            switch (psDecl->sUAV.Type)
            {
            case RETURN_TYPE_FLOAT:
                bcatcstr(metal, "float ");
                break;
            case RETURN_TYPE_UNORM:
                bcatcstr(metal, "TODO: OPCODE_DCL_UNORDERED_ACCESS_VIEW_TYPED->RETURN_TYPE_UNORM ");
                break;
            case RETURN_TYPE_SNORM:
                bcatcstr(metal, "TODO: OPCODE_DCL_UNORDERED_ACCESS_VIEW_TYPED->RETURN_TYPE_SNORM ");
                break;
            case RETURN_TYPE_UINT:
                bcatcstr(metal, "uint ");
                break;
            case RETURN_TYPE_SINT:
                bcatcstr(metal, "int ");
                break;
            default:
                ASSERT(0);
            }
            bstring StructName;
            StructName = bfromcstr("");
            ResourceNameMETAL(StructName, psContext, RGROUP_UAV, psDecl->asOperands[0].ui32RegisterNumber, 0);
            bformata(metal, " * ");
            TranslateOperandMETAL(psContext, &psDecl->asOperands[0], TO_FLAG_NONE);
            bformata(metal, " [[buffer(%d)]], \n", psDecl->asOperands[0].ui32RegisterNumber + UAV_BUFFER_START_SLOT);
            int count = 0;
            for (uint32_t index = 0; index < psContext->psShader->sInfo.ui32NumResourceBindings; index++)
            {
                if (strcmp(psContext->psShader->sInfo.psResourceBindings[index].Name, (const char*)StructName->data) == 0)
                {
                    count++;
                    //psContext->psShader->sInfo.psResourceBindings[index].ui32BindPoint += UAV_BUFFER_START_SLOT;
                    psContext->psShader->sInfo.psResourceBindings[index].eBindArea = UAVAREA_CBUFFER;
                }
            }
            //If count >2, the logic here is wrong and need to be modified.
            ASSERT(count < 2);
        }
        else
        {
            switch (psDecl->value.eResourceDimension)
            {
            case RESOURCE_DIMENSION_TEXTURE1D:
            {
                bformata(metal, "\ttexture1d<");
                break;
            }
            case RESOURCE_DIMENSION_TEXTURE2D:
            {
                bformata(metal, "\ttexture2d<");
                break;
            }
            case RESOURCE_DIMENSION_TEXTURE2DMS:
            {
                //metal does not support this
                ASSERT(0);
                break;
            }
            case RESOURCE_DIMENSION_TEXTURE3D:
            {
                bformata(metal, "\ttexture3d<");
                break;
            }
            case RESOURCE_DIMENSION_TEXTURECUBE:
            {
                bformata(metal, "\ttexturecube<");
                break;
            }
            case RESOURCE_DIMENSION_TEXTURE1DARRAY:
            {
                bformata(metal, "\ttexture1d_array<");
                break;
            }
            case RESOURCE_DIMENSION_TEXTURE2DARRAY:
            {
                bformata(metal, "\ttexture2d_array<");
                break;
            }
            case RESOURCE_DIMENSION_TEXTURE2DMSARRAY:
            {
                //metal does not suuport this.
                ASSERT(0);
                break;
            }
            case RESOURCE_DIMENSION_TEXTURECUBEARRAY:
            {
                bformata(metal, "\ttexturecube_array<");
                break;
            }
            }
            switch (psDecl->sUAV.Type)
            {
            case RETURN_TYPE_FLOAT:
                bcatcstr(metal, "float ");
                break;
            case RETURN_TYPE_UNORM:
                bcatcstr(metal, "TODO: OPCODE_DCL_UNORDERED_ACCESS_VIEW_TYPED->RETURN_TYPE_UNORM ");
                break;
            case RETURN_TYPE_SNORM:
                bcatcstr(metal, "TODO: OPCODE_DCL_UNORDERED_ACCESS_VIEW_TYPED->RETURN_TYPE_SNORM ");
                break;
            case RETURN_TYPE_UINT:
                bcatcstr(metal, "uint ");
                break;
            case RETURN_TYPE_SINT:
                bcatcstr(metal, "int ");
                break;
            default:
                ASSERT(0);
            }
            if (psShader->aiOpcodeUsed[OPCODE_STORE_UAV_TYPED] == 0)
            {
                bcatcstr(metal, "> ");
            }
            else
            {
                //give write access
                bcatcstr(metal, ", access::write> ");
            }
            bstring StructName;
            StructName = bfromcstr("");
            ResourceNameMETAL(StructName, psContext, RGROUP_UAV, psDecl->asOperands[0].ui32RegisterNumber, 0);
            TranslateOperandMETAL(psContext, &psDecl->asOperands[0], TO_FLAG_NONE);
            bformata(metal, " [[texture(%d)]], \n", psDecl->asOperands[0].ui32RegisterNumber + UAV_BUFFER_START_SLOT);
            int count = 0;
            for (uint32_t index = 0; index < psContext->psShader->sInfo.ui32NumResourceBindings; index++)
            {
                if (strcmp(psContext->psShader->sInfo.psResourceBindings[index].Name, (const char*)StructName->data) == 0)
                {
                    count++;
                    //psContext->psShader->sInfo.psResourceBindings[index].ui32BindPoint += UAV_BUFFER_START_SLOT;
                    psContext->psShader->sInfo.psResourceBindings[index].eBindArea = UAVAREA_TEXTURE;
                }
            }
            //If count >2, the logic here is wrong and need to be modified.
            ASSERT(count < 2);
            //TranslateOperand(psContext, &psDecl->asOperands[0], TO_FLAG_NONE);
        }
        psContext->currentShaderString = &psContext->mainShader;
        metal = *psContext->currentShaderString;
        break;
    }
    case OPCODE_DCL_UNORDERED_ACCESS_VIEW_STRUCTURED:
    {
        const uint32_t ui32BindingPoint = psDecl->asOperands[0].aui32ArraySizes[0];
        ConstantBuffer* psCBuf = NULL;

        if (psDecl->sUAV.bCounter)
        {
            bformata(metal, "atomic_uint ");
            ResourceNameMETAL(metal, psContext, RGROUP_UAV, psDecl->asOperands[0].ui32RegisterNumber, 0);
            bformata(metal, "_counter; \n");
        }

        GetConstantBufferFromBindingPoint(RGROUP_UAV, ui32BindingPoint, &psContext->psShader->sInfo, &psCBuf);

        DeclareBufferVariableMETAL(psContext, ui32BindingPoint, psCBuf, &psDecl->asOperands[0], RTYPE_UAV_RWSTRUCTURED, metal, psAtomicList);
        break;
    }
    case OPCODE_DCL_UNORDERED_ACCESS_VIEW_RAW:
    {
        if (psDecl->sUAV.bCounter)
        {
            bformata(metal, "atomic_uint ");
            ResourceNameMETAL(metal, psContext, RGROUP_UAV, psDecl->asOperands[0].ui32RegisterNumber, 0);
            bformata(metal, "_counter; \n");
        }

        bformata(metal, "buffer Block%d {\n\tuint ", psDecl->asOperands[0].ui32RegisterNumber);
        ResourceNameMETAL(metal, psContext, RGROUP_UAV, psDecl->asOperands[0].ui32RegisterNumber, 0);
        bcatcstr(metal, "[];\n};\n");

        break;
    }
    case OPCODE_DCL_RESOURCE_STRUCTURED:
    {
        ConstantBuffer* psCBuf = NULL;

        GetConstantBufferFromBindingPoint(RGROUP_TEXTURE, psDecl->asOperands[0].ui32RegisterNumber, &psContext->psShader->sInfo, &psCBuf);

        DeclareBufferVariableMETAL(psContext, psDecl->asOperands[0].ui32RegisterNumber, psCBuf, &psDecl->asOperands[0],
            RTYPE_STRUCTURED, psContext->mainShader, psAtomicList);
        break;
    }
    case OPCODE_DCL_RESOURCE_RAW:
    {
        bformata(metal, "buffer Block%d {\n\tuint RawRes%d[];\n};\n", psDecl->asOperands[0].ui32RegisterNumber, psDecl->asOperands[0].ui32RegisterNumber);
        break;
    }
    case OPCODE_DCL_THREAD_GROUP_SHARED_MEMORY_STRUCTURED:
    {
        psContext->currentShaderString = &psContext->earlyMain;
        metal = *psContext->currentShaderString;

        ShaderVarType* psVarType = &psShader->sGroupSharedVarType[psDecl->asOperands[0].ui32RegisterNumber];

        ASSERT(psDecl->asOperands[0].ui32RegisterNumber < MAX_GROUPSHARED);

        bcatcstr(metal, "\tthreadgroup struct {\n");
        bformata(metal, "\t\tuint value[%d];\n", psDecl->sTGSM.ui32Stride / 4);
        bcatcstr(metal, "\t} ");
        TranslateOperandMETAL(psContext, &psDecl->asOperands[0], TO_FLAG_NONE);
        bformata(metal, "[%d];\n",
            psDecl->sTGSM.ui32Count);

        memset(psVarType, 0, sizeof(ShaderVarType));
        strcpy(psVarType->Name, "$Element");

        psVarType->Columns = psDecl->sTGSM.ui32Stride / 4;
        psVarType->Elements = psDecl->sTGSM.ui32Count;

        psContext->currentShaderString = &psContext->mainShader;
        metal = *psContext->currentShaderString;
        break;
    }
    case OPCODE_DCL_THREAD_GROUP_SHARED_MEMORY_RAW:
    {
#ifdef _DEBUG
        //AddIndentation(psContext);
        //bcatcstr(metal, "//TODO: OPCODE_DCL_THREAD_GROUP_SHARED_MEMORY_RAW\n");
#endif
        psContext->currentShaderString = &psContext->earlyMain;
        metal = *psContext->currentShaderString;
        bcatcstr(metal, "\tthreadgroup ");
        bformata(metal, "atomic_uint ");
        //psDecl->asOperands
        TranslateOperandMETAL(psContext, &psDecl->asOperands[0], TO_FLAG_NONE);
        bformata(metal, "[%d]; \n", psDecl->sTGSM.ui32Stride / 4);

        psContext->currentShaderString = &psContext->mainShader;
        metal = *psContext->currentShaderString;
        break;
    }
    case OPCODE_DCL_STREAM:
    {
        break;
    }
    case OPCODE_DCL_GS_INSTANCE_COUNT:
    {
        break;
    }
    default:
    {
        ASSERT(0);
        break;
    }
    }
}
