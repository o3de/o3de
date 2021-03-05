// Modifications copyright Amazon.com, Inc. or its affiliates
// Modifications copyright Crytek GmbH

#include "hlslcc.h"
#include "internal_includes/toGLSLDeclaration.h"
#include "internal_includes/toGLSLOperand.h"
#include "internal_includes/languages.h"
#include "internal_includes/hlslccToolkit.h"
#include "bstrlib.h"
#include "internal_includes/debug.h"
#include <math.h>
#include <float.h>
#include <stdbool.h>

#if !defined(isnan)
#ifdef _MSC_VER
#define isnan(x) _isnan(x)
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
extern uint32_t AddImport(HLSLCrossCompilerContext* psContext, SYMBOL_TYPE eType, uint32_t ui32ID, uint32_t ui32Default);
extern uint32_t AddExport(HLSLCrossCompilerContext* psContext, SYMBOL_TYPE eType, uint32_t ui32ID, uint32_t ui32Value);

const char* GetTypeString(GLVARTYPE eType)
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
        return "vec4";
    }
    default:
    {
        return "";
    }
    }
}
const uint32_t GetTypeElementCount(GLVARTYPE eType)
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

void GetSTD140Layout(ShaderVarType* pType, uint32_t* puAlignment, uint32_t* puSize)
{
    *puSize = 0;
    *puAlignment = 1;
    switch (pType->Type)
    {
    case SVT_BOOL:
    case SVT_UINT:
    case SVT_UINT8:
    case SVT_UINT16:
    case SVT_INT:
    case SVT_INT12:
    case SVT_INT16:
    case SVT_FLOAT:
    case SVT_FLOAT10:
    case SVT_FLOAT16:
        *puSize = 4;
        *puAlignment = 4;
        break;
    case SVT_DOUBLE:
        *puSize = 8;
        *puAlignment = 4;
        break;
    case SVT_VOID:
        break;
    default:
        ASSERT(0);
        break;
    }
    switch (pType->Class)
    {
    case SVC_SCALAR:
        break;
    case SVC_MATRIX_ROWS:
    case SVC_MATRIX_COLUMNS:
        // Matrices are translated to arrays of vectors
        *puSize *= pType->Rows;
    case SVC_VECTOR:
        switch (pType->Columns)
        {
        case 2:
            *puSize *= 2;
            *puAlignment *= 2;
            break;
        case 3:
        case 4:
            *puSize *= 4;
            *puAlignment *= 4;
            break;
        }
        break;
    case SVC_STRUCT:
    {
        uint32_t uMember;
        for (uMember = 0; uMember < pType->MemberCount; ++uMember)
        {
            uint32_t uMemberAlignment, uMemberSize;
            *puSize += pType->Members[uMember].Offset;
            GetSTD140Layout(pType->Members + uMember, &uMemberAlignment, &uMemberSize);
            *puSize += uMemberAlignment - 1;
            *puSize -= *puSize % uMemberAlignment;
            *puAlignment = *puAlignment > uMemberAlignment ? *puAlignment : uMemberAlignment;
        }
    }
    break;
    default:
        ASSERT(0);
        break;
    }

    if (pType->Elements > 1)
    {
        *puSize *= pType->Elements;
    }

    if (pType->Elements > 1 || pType->Class == SVC_MATRIX_ROWS || pType->Class == SVC_MATRIX_COLUMNS)
    {
        *puAlignment = (*puAlignment + 0x0000000F) & 0xFFFFFFF0;
    }
}

void AddToDx9ImmConstIndexableArray(HLSLCrossCompilerContext* psContext, const Operand* psOperand)
{
    bstring* savedStringPtr = psContext->currentGLSLString;

    psContext->currentGLSLString = &psContext->earlyMain;
    psContext->indent++;
    AddIndentation(psContext);
    psContext->psShader->aui32Dx9ImmConstArrayRemap[psOperand->ui32RegisterNumber] = psContext->psShader->ui32NumDx9ImmConst;
    bformata(psContext->earlyMain, "ImmConstArray[%d] = ", psContext->psShader->ui32NumDx9ImmConst);
    TranslateOperand(psContext, psOperand, TO_FLAG_NONE);
    bcatcstr(psContext->earlyMain, ";\n");
    psContext->indent--;
    psContext->psShader->ui32NumDx9ImmConst++;

    psContext->currentGLSLString = savedStringPtr;
}

void DeclareConstBufferShaderVariable(HLSLCrossCompilerContext* psContext, const char* Name, const struct ShaderVarType_TAG* psType, int unsizedArray)
{
    bstring glsl = *psContext->currentGLSLString;

    if (psType->Class == SVC_STRUCT)
    {
        bcatcstr(glsl, "\t");
        ShaderVarName(glsl, psContext->psShader, Name);
        bcatcstr(glsl, "_Type ");
        ShaderVarName(glsl, psContext->psShader, Name);
        if (psType->Elements > 1)
        {
            bformata(glsl, "[%d]", psType->Elements);
        }
    }
    else if (psType->Class == SVC_MATRIX_COLUMNS || psType->Class == SVC_MATRIX_ROWS)
    {
        switch (psType->Type)
        {
        case SVT_FLOAT:
        {
            bformata(glsl, "\tvec%d ", psType->Columns);
            ShaderVarName(glsl, psContext->psShader, Name);
            bformata(glsl, "[%d", psType->Rows);
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
            bformata(glsl, " * %d", psType->Elements);
        }
        bformata(glsl, "]");
    }
    else
    if (psType->Class == SVC_VECTOR)
    {
        switch (psType->Type)
        {
        default:
            ASSERT(0);
        case SVT_FLOAT:
        case SVT_FLOAT10:
        case SVT_FLOAT16:
        case SVT_UINT:
        case SVT_UINT8:
        case SVT_UINT16:
        case SVT_INT:
        case SVT_INT12:
        case SVT_INT16:
            bformata(glsl, "\t%s ", GetConstructorForTypeGLSL(psContext, psType->Type, psType->Columns, true));
            break;
        case SVT_DOUBLE:
            bformata(glsl, "\tdvec%d ", psType->Columns);
            break;
        }

        ShaderVarName(glsl, psContext->psShader, Name);

        if (psType->Elements > 1)
        {
            bformata(glsl, "[%d]", psType->Elements);
        }
    }
    else
    if (psType->Class == SVC_SCALAR)
    {
        switch (psType->Type)
        {
        default:
            ASSERT(0);
        case SVT_FLOAT:
        case SVT_FLOAT10:
        case SVT_FLOAT16:
        case SVT_UINT:
        case SVT_UINT8:
        case SVT_UINT16:
        case SVT_INT:
        case SVT_INT12:
        case SVT_INT16:
            bformata(glsl, "\t%s ", GetConstructorForTypeGLSL(psContext, psType->Type, 1, true));
            break;
        case SVT_DOUBLE:
            bformata(glsl, "\tdouble ");
            break;
        case SVT_BOOL:
            //Use int instead of bool.
            //Allows implicit conversions to integer and
            //bool consumes 4-bytes in HLSL and GLSL anyway.
            bformata(glsl, "\tint ");
            break;
        }

        ShaderVarName(glsl, psContext->psShader, Name);

        if (psType->Elements > 1)
        {
            bformata(glsl, "[%d]", psType->Elements);
        }
    }
    if (unsizedArray)
    {
        bformata(glsl, "[]");
    }
    bformata(glsl, ";\n");
}

//In GLSL embedded structure definitions are not supported.
void PreDeclareStructType(HLSLCrossCompilerContext* psContext, const char* Name, const struct ShaderVarType_TAG* psType)
{
    uint32_t i;
    bstring glsl = *psContext->currentGLSLString;

    for (i = 0; i < psType->MemberCount; ++i)
    {
        if (psType->Members[i].Class == SVC_STRUCT)
        {
            PreDeclareStructType(psContext, psType->Members[i].Name, &psType->Members[i]);
        }
    }

    if (psType->Class == SVC_STRUCT)
    {
        uint32_t unnamed_struct = strcmp(Name, "$Element") == 0 ? 1 : 0;

        //Not supported at the moment
        ASSERT(!unnamed_struct);

        bcatcstr(glsl, "struct ");
        ShaderVarName(glsl, psContext->psShader, Name);
        bcatcstr(glsl, "_Type {\n");

        for (i = 0; i < psType->MemberCount; ++i)
        {
            ASSERT(psType->Members != 0);

            DeclareConstBufferShaderVariable(psContext, psType->Members[i].Name, &psType->Members[i], 0);
        }

        bformata(glsl, "};\n");
    }
}

void DeclarePLSStructVars(HLSLCrossCompilerContext* psContext, const char* Name, const struct ShaderVarType_TAG* psType)
{
    (void)Name;

    uint32_t i;
    bstring glsl = *psContext->currentGLSLString;

    ASSERT(psType->Members != 0);

    for (i = 0; i < psType->MemberCount; ++i)
    {
        if (psType->Members[i].Class == SVC_STRUCT)
        {
            ASSERT(0); // PLS can't have nested structs
        }
    }

    if (psType->Class == SVC_STRUCT)
    {
        for (i = 0; i < psType->MemberCount; ++i)
        {
            ShaderVarType cur_member = psType->Members[i];

            if (cur_member.Class == SVC_VECTOR)
            {
                switch (cur_member.Type)
                {
                case SVT_FLOAT:
                {
                    // float2 -> rg16f
                    if (2 == cur_member.Columns)
                    {
                        bcatcstr(glsl, "\tlayout(rg16f) highp vec2 ");
                    }
                    // float3 -> r11f_g11f_b10f
                    else if (3 == cur_member.Columns)
                    {
                        bcatcstr(glsl, "\tlayout(r11f_g11f_b10f) highp vec3 ");
                    }
                    // float4 -> rgba8
                    else if (4 == cur_member.Columns)
                    {
                        bcatcstr(glsl, "\tlayout(rgba8) highp vec4 ");
                    }
                    else
                    {
                        ASSERT(0); // not supported
                    }
                    break;
                }
                case SVT_INT:
                {
                    // int2 -> rg16i
                    if (2 == cur_member.Columns)
                    {
                        bcatcstr(glsl, "\tlayout(rg16i) highp ivec2 ");
                    }
                    // int4 -> rgba8i
                    else if (4 == cur_member.Columns)
                    {
                        bcatcstr(glsl, "\tlayout(rgba8i) highp ivec4 ");
                    }
                    else
                    {
                        ASSERT(0); // not supported
                    }
                    break;
                }
                case SVT_UINT:
                case SVT_DOUBLE:
                default:
                    ASSERT(0);
                }

                if (cur_member.Elements > 1)
                {
                    ASSERT(0); // PLS can't have arrays
                }
            }
            else if (cur_member.Class == SVC_SCALAR)
            {
                switch (cur_member.Type)
                {
                case SVT_UINT:
                    bcatcstr(glsl, "\tlayout(r32ui) highp uint ");
                    break;
                case SVT_FLOAT:
                case SVT_INT:
                case SVT_DOUBLE:
                case SVT_BOOL:
                default:
                    ASSERT(0);
                }
            }

            ShaderVarName(glsl, psContext->psShader, cur_member.Name);
            bcatcstr(glsl, ";\n");
        }
    }
    else
    {
        ASSERT(0);
    }
}

char* GetDeclaredInputName(const HLSLCrossCompilerContext* psContext, const SHADER_TYPE eShaderType, const Operand* psOperand)
{
    bstring inputName;
    char* cstr;
    InOutSignature* psIn;

    if (eShaderType == GEOMETRY_SHADER)
    {
        inputName = bformat("VtxOutput%d", psOperand->ui32RegisterNumber);
    }
    else if (eShaderType == HULL_SHADER)
    {
        inputName = bformat("VtxGeoOutput%d", psOperand->ui32RegisterNumber);
    }
    else if (eShaderType == DOMAIN_SHADER)
    {
        inputName = bformat("HullOutput%d", psOperand->ui32RegisterNumber);
    }
    else if (eShaderType == PIXEL_SHADER)
    {
        if (psContext->flags & HLSLCC_FLAG_TESS_ENABLED)
        {
            inputName = bformat("DomOutput%d", psOperand->ui32RegisterNumber);
        }
        else
        {
            inputName = bformat("VtxGeoOutput%d", psOperand->ui32RegisterNumber);
        }
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

char* GetDeclaredOutputName(const HLSLCrossCompilerContext* psContext,
    const SHADER_TYPE eShaderType,
    const Operand* psOperand,
    int* piStream)
{
    bstring outputName;
    char* cstr;
    InOutSignature* psOut;

    int foundOutput = GetOutputSignatureFromRegister(psOperand->ui32RegisterNumber,
            psOperand->ui32CompMask,
            psContext->psShader->ui32CurrentVertexOutputStream,
            &psContext->psShader->sInfo,
            &psOut);

    ASSERT(foundOutput);

    if (eShaderType == GEOMETRY_SHADER)
    {
        if (psOut->ui32Stream != 0)
        {
            outputName = bformat("VtxGeoOutput%d_S%d", psOperand->ui32RegisterNumber, psOut->ui32Stream);
            piStream[0] = psOut->ui32Stream;
        }
        else
        {
            outputName = bformat("VtxGeoOutput%d", psOperand->ui32RegisterNumber);
        }
    }
    else if (eShaderType == DOMAIN_SHADER)
    {
        outputName = bformat("DomOutput%d", psOperand->ui32RegisterNumber);
    }
    else if (eShaderType == VERTEX_SHADER)
    {
        if (psContext->flags & HLSLCC_FLAG_GS_ENABLED)
        {
            outputName = bformat("VtxOutput%d", psOperand->ui32RegisterNumber);
        }
        else
        {
            outputName = bformat("VtxGeoOutput%d", psOperand->ui32RegisterNumber);
        }
    }
    else if (eShaderType == PIXEL_SHADER)
    {
        outputName = bformat("PixOutput%d", psOperand->ui32RegisterNumber);
    }
    else
    {
        ASSERT(eShaderType == HULL_SHADER);
        outputName = bformat("HullOutput%d", psOperand->ui32RegisterNumber);
    }
    if (psContext->flags & HLSLCC_FLAG_INOUT_SEMANTIC_NAMES)
    {
        bformata(outputName, "_%s%d", psOut->SemanticName, psOut->ui32SemanticIndex);
    }

    cstr = bstr2cstr(outputName, '\0');
    bdestroy(outputName);
    return cstr;
}
static void DeclareInput(
    HLSLCrossCompilerContext* psContext,
    const Declaration* psDecl,
    const char* Interpolation, const char* StorageQualifier, const char* Precision, int iNumComponents, OPERAND_INDEX_DIMENSION eIndexDim, const char* InputName)
{
    Shader* psShader = psContext->psShader;
    bstring glsl = *psContext->currentGLSLString;

    // This falls within the specified index ranges. The default is 0 if no input range is specified
    if (psShader->aIndexedInput[psDecl->asOperands[0].ui32RegisterNumber] == -1)
    {
        return;
    }

    if (psShader->aiInputDeclaredSize[psDecl->asOperands[0].ui32RegisterNumber] == 0)
    {
        const char* vecType = "vec";
        const char* scalarType = "float";
        InOutSignature* psSignature = NULL;

        if (GetInputSignatureFromRegister(psDecl->asOperands[0].ui32RegisterNumber, &psShader->sInfo, &psSignature))
        {
            switch (psSignature->eComponentType)
            {
            case INOUT_COMPONENT_UINT32:
            {
                vecType = "uvec";
                scalarType = "uint";
                break;
            }
            case INOUT_COMPONENT_SINT32:
            {
                vecType = "ivec";
                scalarType = "int";
                break;
            }
            case INOUT_COMPONENT_FLOAT32:
            {
                break;
            }
            }
        }

        if (psShader->eShaderType == PIXEL_SHADER)
        {
            psShader->sInfo.aePixelInputInterpolation[psDecl->asOperands[0].ui32RegisterNumber] = psDecl->value.eInterpolation;
        }

        if (HaveInOutLocationQualifier(psContext->psShader->eTargetLanguage, psContext->psShader->extensions) ||
            (psShader->eShaderType == VERTEX_SHADER && HaveLimitedInOutLocationQualifier(psContext->psShader->eTargetLanguage)))
        {
            bformata(glsl, "layout(location = %d) ", psDecl->asOperands[0].ui32RegisterNumber);
        }

        switch (eIndexDim)
        {
        case INDEX_2D:
        {
            if (iNumComponents == 1)
            {
                const uint32_t regNum =  psDecl->asOperands[0].ui32RegisterNumber;
                const uint32_t arraySize = psDecl->asOperands[0].aui32ArraySizes[0];

                psContext->psShader->abScalarInput[psDecl->asOperands[0].ui32RegisterNumber] = -1;

                bformata(glsl, "%s %s %s %s [%d];\n", StorageQualifier, Precision, scalarType, InputName, arraySize);

                bformata(glsl, "%s1 Input%d;\n", vecType, psDecl->asOperands[0].ui32RegisterNumber);

                psShader->aiInputDeclaredSize[psDecl->asOperands[0].ui32RegisterNumber] = arraySize;
            }
            else
            {
                bformata(glsl, "%s %s %s%d %s [%d];\n", StorageQualifier, Precision, vecType, iNumComponents, InputName, psDecl->asOperands[0].aui32ArraySizes[0]);

                bformata(glsl, "%s %s%d Input%d[%d];\n", Precision, vecType, iNumComponents, psDecl->asOperands[0].ui32RegisterNumber, psDecl->asOperands[0].aui32ArraySizes[0]);

                psShader->aiInputDeclaredSize[psDecl->asOperands[0].ui32RegisterNumber] = psDecl->asOperands[0].aui32ArraySizes[0];
            }
            break;
        }
        default:
        {
            if (psDecl->asOperands[0].eType == OPERAND_TYPE_SPECIAL_TEXCOORD)
            {
                InputName = "TexCoord";
            }

            if (iNumComponents == 1)
            {
                psContext->psShader->abScalarInput[psDecl->asOperands[0].ui32RegisterNumber] = 1;

                bformata(glsl, "%s %s %s %s %s;\n", Interpolation, StorageQualifier, Precision, scalarType, InputName);
                bformata(glsl, "%s1 Input%d;\n", vecType, psDecl->asOperands[0].ui32RegisterNumber);

                psShader->aiInputDeclaredSize[psDecl->asOperands[0].ui32RegisterNumber] = -1;
            }
            else
            {
                if (psShader->aIndexedInput[psDecl->asOperands[0].ui32RegisterNumber] > 0)
                {
                    bformata(glsl, "%s %s %s %s%d %s", Interpolation, StorageQualifier, Precision, vecType, iNumComponents, InputName);
                    bformata(glsl, "[%d];\n", psShader->aIndexedInput[psDecl->asOperands[0].ui32RegisterNumber]);

                    bformata(glsl, "%s %s%d Input%d[%d];\n", Precision, vecType, iNumComponents, psDecl->asOperands[0].ui32RegisterNumber, psShader->aIndexedInput[psDecl->asOperands[0].ui32RegisterNumber]);

                    psShader->aiInputDeclaredSize[psDecl->asOperands[0].ui32RegisterNumber] = psShader->aIndexedInput[psDecl->asOperands[0].ui32RegisterNumber];
                }
                else
                {
                    bformata(glsl, "%s %s %s %s%d %s;\n", Interpolation, StorageQualifier, Precision, vecType, iNumComponents, InputName);
                    bformata(glsl, "%s %s%d Input%d;\n", Precision, vecType, iNumComponents, psDecl->asOperands[0].ui32RegisterNumber);

                    psShader->aiInputDeclaredSize[psDecl->asOperands[0].ui32RegisterNumber] = -1;
                }
            }
            break;
        }
        }
    }

    if (psShader->abInputReferencedByInstruction[psDecl->asOperands[0].ui32RegisterNumber])
    {
        psContext->currentGLSLString = &psContext->earlyMain;
        psContext->indent++;

        if (psShader->aiInputDeclaredSize[psDecl->asOperands[0].ui32RegisterNumber] == -1) //Not an array
        {
            AddIndentation(psContext);
            bformata(psContext->earlyMain, "Input%d = %s;\n", psDecl->asOperands[0].ui32RegisterNumber, InputName);
        }
        else
        {
            int arrayIndex = psShader->aiInputDeclaredSize[psDecl->asOperands[0].ui32RegisterNumber];

            while (arrayIndex)
            {
                AddIndentation(psContext);
                bformata(psContext->earlyMain, "Input%d[%d] = %s[%d];\n", psDecl->asOperands[0].ui32RegisterNumber, arrayIndex - 1, InputName, arrayIndex - 1);

                arrayIndex--;
            }
        }
        psContext->indent--;
        psContext->currentGLSLString = &psContext->glsl;
    }
}

void AddBuiltinInput(HLSLCrossCompilerContext* psContext, const Declaration* psDecl, const char* builtinName, uint32_t uNumComponents)
{
    (void)uNumComponents;

    bstring glsl = *psContext->currentGLSLString;
    Shader* psShader = psContext->psShader;

    if (psShader->aiInputDeclaredSize[psDecl->asOperands[0].ui32RegisterNumber] == 0)
    {
        SHADER_VARIABLE_TYPE eType = GetOperandDataType(psContext, &psDecl->asOperands[0]);
        bformata(glsl, "%s ", GetConstructorForTypeGLSL(psContext, eType, 4, false));
        TranslateOperand(psContext, &psDecl->asOperands[0], TO_FLAG_NAME_ONLY);
        bformata(glsl, ";\n");

        psShader->aiInputDeclaredSize[psDecl->asOperands[0].ui32RegisterNumber] = 1;
    }
    else
    {
        //This register has already been declared. The HLSL bytecode likely looks
        //something like this then:
        //  dcl_input_ps constant v3.x
        //  dcl_input_ps_sgv v3.y, primitive_id

        //GLSL does not allow assignment to a varying!
    }

    psContext->currentGLSLString = &psContext->earlyMain;
    psContext->indent++;
    AddIndentation(psContext);
    TranslateOperand(psContext, &psDecl->asOperands[0], TO_FLAG_NONE);

    bformata(psContext->earlyMain, " = %s", builtinName);

    switch (psDecl->asOperands[0].eSpecialName)
    {
    case NAME_POSITION:
        TranslateOperandSwizzle(psContext, &psDecl->asOperands[0]);
        // Invert w coordinate if necessary to be the same as SV_Position
        if (psContext->psShader->eShaderType == PIXEL_SHADER)
        {
            if (psDecl->asOperands[0].eSelMode == OPERAND_4_COMPONENT_MASK_MODE &&
                psDecl->asOperands[0].eType == OPERAND_TYPE_INPUT)
            {
                if (psDecl->asOperands[0].ui32CompMask & OPERAND_4_COMPONENT_MASK_Z)
                {
                    uint32_t ui32IgnoreSwizzle;
                    bcatcstr(psContext->earlyMain, ";\n#ifdef EMULATE_DEPTH_CLAMP\n");
                    AddIndentation(psContext);
                    TranslateVariableName(psContext, &psDecl->asOperands[0], TO_FLAG_NONE, &ui32IgnoreSwizzle);
                    bcatcstr(psContext->earlyMain, ".z = unclampedDepth;\n");
                    bcatcstr(psContext->earlyMain, "#endif\n");
                }
                if (psDecl->asOperands[0].ui32CompMask & OPERAND_4_COMPONENT_MASK_W)
                {
                    uint32_t ui32IgnoreSwizzle;
                    bcatcstr(psContext->earlyMain, ";\n");
                    AddIndentation(psContext);
                    TranslateVariableName(psContext, &psDecl->asOperands[0], TO_FLAG_NONE, &ui32IgnoreSwizzle);
                    bcatcstr(psContext->earlyMain, ".w = 1.0 / ");
                    TranslateVariableName(psContext, &psDecl->asOperands[0], TO_FLAG_NONE, &ui32IgnoreSwizzle);
                    bcatcstr(psContext->earlyMain, ".w;\n");
                }
            }
            else
            {
                ASSERT(0);
            }
        }

        break;
    default:
        //Scalar built-in. Don't apply swizzle.
        break;
    }
    bcatcstr(psContext->earlyMain, ";\n");

    psContext->indent--;
    psContext->currentGLSLString = &psContext->glsl;
}

int OutputNeedsDeclaring(HLSLCrossCompilerContext* psContext, const Operand* psOperand, const int count)
{
    Shader* psShader = psContext->psShader;
    
    // Depth Output operands are a special case and won't have a ui32RegisterNumber,
    // so first we have to check if the output operand is depth.
    if (psShader->eShaderType == PIXEL_SHADER)
    {
        if (psOperand->eType == OPERAND_TYPE_OUTPUT_DEPTH_GREATER_EQUAL ||
            psOperand->eType == OPERAND_TYPE_OUTPUT_DEPTH_LESS_EQUAL)
        {
            return 1;
        }
        else if (psOperand->eType == OPERAND_TYPE_OUTPUT_DEPTH)
        {
            return 0; // OpenGL doesn't need to declare depth output variable (gl_FragDepth)
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

void AddBuiltinOutput(HLSLCrossCompilerContext* psContext, const Declaration* psDecl, const GLVARTYPE type, int arrayElements, const char* builtinName)
{
    bstring glsl = *psContext->currentGLSLString;
    Shader* psShader = psContext->psShader;

    psContext->havePostShaderCode[psContext->currentPhase] = 1;

    if (OutputNeedsDeclaring(psContext, &psDecl->asOperands[0], arrayElements ? arrayElements : 1))
    {
        InOutSignature* psSignature = NULL;

        GetOutputSignatureFromRegister(psDecl->asOperands[0].ui32RegisterNumber,
            psDecl->asOperands[0].ui32CompMask,
            0,
            &psShader->sInfo, &psSignature);

        bcatcstr(glsl, "#undef ");
        TranslateOperand(psContext, &psDecl->asOperands[0], TO_FLAG_NAME_ONLY);
        bcatcstr(glsl, "\n");

        bcatcstr(glsl, "#define ");
        TranslateOperand(psContext, &psDecl->asOperands[0], TO_FLAG_NAME_ONLY);
        bformata(glsl, " phase%d_", psContext->currentPhase);
        TranslateOperand(psContext, &psDecl->asOperands[0], TO_FLAG_NAME_ONLY);
        bcatcstr(glsl, "\n");

        bcatcstr(glsl, "vec4 ");
        bformata(glsl, "phase%d_", psContext->currentPhase);
        TranslateOperand(psContext, &psDecl->asOperands[0], TO_FLAG_NAME_ONLY);
        if (arrayElements)
        {
            bformata(glsl, "[%d];\n", arrayElements);
        }
        else
        {
            bcatcstr(glsl, ";\n");
        }

        psContext->currentGLSLString = &psContext->postShaderCode[psContext->currentPhase];
        glsl = *psContext->currentGLSLString;
        psContext->indent++;
        if (arrayElements)
        {
            int elem;
            for (elem = 0; elem < arrayElements; elem++)
            {
                AddIndentation(psContext);
                bformata(glsl, "%s[%d] = %s(phase%d_", builtinName, elem, GetTypeString(type), psContext->currentPhase);
                TranslateOperand(psContext, &psDecl->asOperands[0], TO_FLAG_NAME_ONLY);
                bformata(glsl, "[%d]", elem);
                TranslateOperandSwizzle(psContext, &psDecl->asOperands[0]);
                bformata(glsl, ");\n");
            }
        }
        else
        {
            if (psDecl->asOperands[0].eSpecialName == NAME_CLIP_DISTANCE)
            {
                int max = GetMaxComponentFromComponentMask(&psDecl->asOperands[0]);

                int applySiwzzle = GetNumSwizzleElements(&psDecl->asOperands[0]) > 1 ? 1 : 0;
                int index;
                int i;
                int multiplier = 1;
                char* swizzle[] = {".x", ".y", ".z", ".w"};

                ASSERT(psSignature != NULL);

                index = psSignature->ui32SemanticIndex;

                //Clip distance can be spread across 1 or 2 outputs (each no more than a vec4).
                //Some examples:
                //float4 clip[2] : SV_ClipDistance; //8 clip distances
                //float3 clip[2] : SV_ClipDistance; //6 clip distances
                //float4 clip : SV_ClipDistance; //4 clip distances
                //float clip : SV_ClipDistance; //1 clip distance.

                //In GLSL the clip distance built-in is an array of up to 8 floats.
                //So vector to array conversion needs to be done here.
                if (index == 1)
                {
                    InOutSignature* psFirstClipSignature;
                    if (GetOutputSignatureFromSystemValue(NAME_CLIP_DISTANCE, 1, &psShader->sInfo, &psFirstClipSignature))
                    {
                        if (psFirstClipSignature->ui32Mask & (1 << 3))
                        {
                            multiplier = 4;
                        }
                        else
                        if (psFirstClipSignature->ui32Mask & (1 << 2))
                        {
                            multiplier = 3;
                        }
                        else
                        if (psFirstClipSignature->ui32Mask & (1 << 1))
                        {
                            multiplier = 2;
                        }
                    }
                }

                for (i = 0; i < max; ++i)
                {
                    AddIndentation(psContext);
                    bformata(glsl, "%s[%d] = (phase%d_", builtinName, i + multiplier * index, psContext->currentPhase);
                    TranslateOperand(psContext, &psDecl->asOperands[0], TO_FLAG_NONE);
                    if (applySiwzzle)
                    {
                        bformata(glsl, ")%s;\n", swizzle[i]);
                    }
                    else
                    {
                        bformata(glsl, ");\n");
                    }
                }
            }
            else
            {
                uint32_t elements = GetNumSwizzleElements(&psDecl->asOperands[0]);

                if (elements != GetTypeElementCount(type))
                {
                    //This is to handle float3 position seen in control point phases
                    //struct HS_OUTPUT
                    //{
                    //  float3 vPosition : POSITION;
                    //}; -> dcl_output o0.xyz
                    //gl_Position is vec4.
                    AddIndentation(psContext);
                    bformata(glsl, "%s = %s(phase%d_", builtinName, GetTypeString(type), psContext->currentPhase);
                    TranslateOperand(psContext, &psDecl->asOperands[0], TO_FLAG_NONE);
                    bformata(glsl, ", 1);\n");
                }
                else
                {
                    AddIndentation(psContext);
                    bformata(glsl, "%s = %s(phase%d_", builtinName, GetTypeString(type), psContext->currentPhase);
                    TranslateOperand(psContext, &psDecl->asOperands[0], TO_FLAG_NONE);
                    bformata(glsl, ");\n");
                }
            }

            if (psShader->eShaderType == VERTEX_SHADER && psDecl->asOperands[0].eSpecialName == NAME_POSITION)
            {
                if (psContext->flags & HLSLCC_FLAG_INVERT_CLIP_SPACE_Y)
                {
                    AddIndentation(psContext);
                    bformata(glsl, "gl_Position.y = -gl_Position.y;\n");
                }

                if (EmulateDepthClamp(psContext->psShader->eTargetLanguage))
                {
                    bcatcstr(glsl, "#ifdef EMULATE_DEPTH_CLAMP\n");
                    bcatcstr(glsl, "#if EMULATE_DEPTH_CLAMP == 1\n");
                    AddIndentation(psContext);
                    bcatcstr(glsl, "unclampedDepth = gl_DepthRange.near + gl_DepthRange.diff * gl_Position.z / gl_Position.w;\n");
                    bcatcstr(glsl, "#elif EMULATE_DEPTH_CLAMP == 2\n");
                    AddIndentation(psContext);
                    bcatcstr(glsl, "unclampedZ = gl_DepthRange.diff * gl_Position.z;\n");
                    bcatcstr(glsl, "#endif\n");
                    AddIndentation(psContext);
                    bcatcstr(glsl, "gl_Position.z = 0.0;\n");
                }

                if (psContext->flags & HLSLCC_FLAG_CONVERT_CLIP_SPACE_Z)
                {
                    if (EmulateDepthClamp(psContext->psShader->eTargetLanguage))
                    {
                        bcatcstr(glsl, "#else\n");
                    }

                    AddIndentation(psContext);
                    bcatcstr(glsl, "gl_Position.z = gl_Position.z * 2.0 - gl_Position.w;\n");
                }

                if (EmulateDepthClamp(psContext->psShader->eTargetLanguage))
                {
                    bcatcstr(glsl, "#endif\n");
                }
            }
        }
        psContext->indent--;
        psContext->currentGLSLString = &psContext->glsl;
    }
}

void AddUserOutput(HLSLCrossCompilerContext* psContext, const Declaration* psDecl)
{
    bstring glsl = *psContext->currentGLSLString;
    Shader* psShader = psContext->psShader;

    if (OutputNeedsDeclaring(psContext, &psDecl->asOperands[0], 1))
    {
        const Operand* psOperand = &psDecl->asOperands[0];
        const char* Precision = "";
        const char* type = "vec";

        InOutSignature* psSignature = NULL;

        GetOutputSignatureFromRegister(psDecl->asOperands[0].ui32RegisterNumber,
            psDecl->asOperands[0].ui32CompMask,
            psShader->ui32CurrentVertexOutputStream,
            &psShader->sInfo,
            &psSignature);

        switch (psSignature->eComponentType)
        {
        case INOUT_COMPONENT_UINT32:
        {
            type = "uvec";
            break;
        }
        case INOUT_COMPONENT_SINT32:
        {
            type = "ivec";
            break;
        }
        case INOUT_COMPONENT_FLOAT32:
        {
            break;
        }
        }

        if (HavePrecisionQualifers(psShader->eTargetLanguage))
        {
            switch (psOperand->eMinPrecision)
            {
            case OPERAND_MIN_PRECISION_DEFAULT:
            {
                Precision = "highp";
                break;
            }
            case OPERAND_MIN_PRECISION_FLOAT_16:
            {
                Precision = "mediump";
                break;
            }
            case OPERAND_MIN_PRECISION_FLOAT_2_8:
            {
                Precision = "lowp";
                break;
            }
            case OPERAND_MIN_PRECISION_SINT_16:
            {
                Precision = "mediump";
                //type = "ivec";
                break;
            }
            case OPERAND_MIN_PRECISION_UINT_16:
            {
                Precision = "mediump";
                //type = "uvec";
                break;
            }
            }
        }

        switch (psShader->eShaderType)
        {
        case PIXEL_SHADER:
        {
            switch (psDecl->asOperands[0].eType)
            {
            case OPERAND_TYPE_OUTPUT_COVERAGE_MASK:
            case OPERAND_TYPE_OUTPUT_DEPTH:
            {
                break;
            }
            case OPERAND_TYPE_OUTPUT_DEPTH_GREATER_EQUAL:
            {
                bcatcstr(glsl, "#ifdef GL_ARB_conservative_depth\n");
                bcatcstr(glsl, "#extension GL_ARB_conservative_depth : enable\n");
                bcatcstr(glsl, "layout (depth_greater) out float gl_FragDepth;\n");
                bcatcstr(glsl, "#endif\n");
                break;
            }
            case OPERAND_TYPE_OUTPUT_DEPTH_LESS_EQUAL:
            {
                bcatcstr(glsl, "#ifdef GL_ARB_conservative_depth\n");
                bcatcstr(glsl, "#extension GL_ARB_conservative_depth : enable\n");
                bcatcstr(glsl, "layout (depth_less) out float gl_FragDepth;\n");
                bcatcstr(glsl, "#endif\n");
                break;
            }
            default:
            {
                if (WriteToFragData(psContext->psShader->eTargetLanguage))
                {
                    bformata(glsl, "#define Output%d gl_FragData[%d]\n", psDecl->asOperands[0].ui32RegisterNumber, psDecl->asOperands[0].ui32RegisterNumber);
                }
                else
                {
                    int stream = 0;
                    char* OutputName = GetDeclaredOutputName(psContext, PIXEL_SHADER, psOperand, &stream);

                    uint32_t renderTarget = psDecl->asOperands[0].ui32RegisterNumber;

                    // Check if we already defined this as a "inout"
                    if ((psContext->rendertargetUse[renderTarget] & INPUT_RENDERTARGET) == 0)
                    {
                        if (HaveInOutLocationQualifier(psContext->psShader->eTargetLanguage, psContext->psShader->extensions) || HaveLimitedInOutLocationQualifier(psContext->psShader->eTargetLanguage))
                        {
                            uint32_t index = 0;

                            if ((psContext->flags & HLSLCC_FLAG_DUAL_SOURCE_BLENDING) && DualSourceBlendSupported(psContext->psShader->eTargetLanguage))
                            {
                                if (renderTarget > 0)
                                {
                                    renderTarget = 0;
                                    index = 1;
                                }
                                bformata(glsl, "layout(location = %d, index = %d) ", renderTarget, index);
                            }
                            else
                            {
                                bformata(glsl, "layout(location = %d) ", renderTarget);
                            }
                        }

                        bformata(glsl, "out %s %s4 %s;\n", Precision, type, OutputName);
                    }

                    if (stream)
                    {
                        bformata(glsl, "#define Output%d_S%d %s\n", psDecl->asOperands[0].ui32RegisterNumber, stream, OutputName);
                    }
                    else
                    {
                        bformata(glsl, "#define Output%d %s\n", psDecl->asOperands[0].ui32RegisterNumber, OutputName);
                    }
                    bcstrfree(OutputName);
                }
                break;
            }
            }
            break;
        }
        case VERTEX_SHADER:
        {
            int iNumComponents = 4;    //GetMaxComponentFromComponentMask(&psDecl->asOperands[0]);
            const char* Interpolation = "";
            int stream = 0;
            char* OutputName = GetDeclaredOutputName(psContext, VERTEX_SHADER, psOperand, &stream);

            if (psShader->eShaderType == VERTEX_SHADER)
            {
                uint32_t ui32InterpImp = AddImport(psContext, SYMBOL_INPUT_INTERPOLATION_MODE, psDecl->asOperands[0].ui32RegisterNumber, (uint32_t)INTERPOLATION_LINEAR);
                bformata(glsl, "#if IMPORT_%d == %d\n", ui32InterpImp, INTERPOLATION_CONSTANT);
                bformata(glsl, "#define Output%dInterpolation flat\n", psDecl->asOperands[0].ui32RegisterNumber);
                bformata(glsl, "#elif IMPORT_%d == %d\n", ui32InterpImp, INTERPOLATION_LINEAR_CENTROID);
                bformata(glsl, "#define Output%dInterpolation centroid\n", psDecl->asOperands[0].ui32RegisterNumber);
                bformata(glsl, "#elif IMPORT_%d == %d\n", ui32InterpImp, INTERPOLATION_LINEAR_NOPERSPECTIVE);
                bformata(glsl, "#define Output%dInterpolation noperspective\n", psDecl->asOperands[0].ui32RegisterNumber);
                bformata(glsl, "#elif IMPORT_%d == %d\n", ui32InterpImp, INTERPOLATION_LINEAR_NOPERSPECTIVE_CENTROID);
                bformata(glsl, "#define Output%dInterpolation noperspective centroid\n", psDecl->asOperands[0].ui32RegisterNumber);
                bformata(glsl, "#elif IMPORT_%d == %d\n", ui32InterpImp, INTERPOLATION_LINEAR_SAMPLE);
                bformata(glsl, "#define Output%dInterpolation sample\n", psDecl->asOperands[0].ui32RegisterNumber);
                bformata(glsl, "#elif IMPORT_%d == %d\n", ui32InterpImp, INTERPOLATION_LINEAR_NOPERSPECTIVE_SAMPLE);
                bformata(glsl, "#define Output%dInterpolation noperspective sample\n", psDecl->asOperands[0].ui32RegisterNumber);
                bcatcstr(glsl, "#else\n");
                bformata(glsl, "#define Output%dInterpolation \n", psDecl->asOperands[0].ui32RegisterNumber);
                bcatcstr(glsl, "#endif\n");
            }

            if (HaveInOutLocationQualifier(psContext->psShader->eTargetLanguage, psContext->psShader->extensions))
            {
                bformata(glsl, "layout(location = %d) ", psDecl->asOperands[0].ui32RegisterNumber);
            }

            if (psShader->eShaderType == VERTEX_SHADER)
            {
                bformata(glsl, "Output%dInterpolation ", psDecl->asOperands[0].ui32RegisterNumber);
            }

            if (InOutSupported(psContext->psShader->eTargetLanguage))
            {
                bformata(glsl, "out %s %s%d %s;\n", Precision, type, iNumComponents, OutputName);
            }
            else
            {
                bformata(glsl, "varying %s %s%d %s;\n", Precision, type, iNumComponents, OutputName);
            }
            bformata(glsl, "#define Output%d %s\n", psDecl->asOperands[0].ui32RegisterNumber, OutputName);
            bcstrfree(OutputName);

            break;
        }
        case GEOMETRY_SHADER:
        {
            int stream = 0;
            char* OutputName = GetDeclaredOutputName(psContext, GEOMETRY_SHADER, psOperand, &stream);

            if (HaveInOutLocationQualifier(psContext->psShader->eTargetLanguage, psContext->psShader->extensions))
            {
                bformata(glsl, "layout(location = %d) ", psDecl->asOperands[0].ui32RegisterNumber);
            }

            bformata(glsl, "out %s4 %s;\n", type, OutputName);
            if (stream)
            {
                bformata(glsl, "#define Output%d_S%d %s\n", psDecl->asOperands[0].ui32RegisterNumber, stream, OutputName);
            }
            else
            {
                bformata(glsl, "#define Output%d %s\n", psDecl->asOperands[0].ui32RegisterNumber, OutputName);
            }
            bcstrfree(OutputName);
            break;
        }
        case HULL_SHADER:
        {
            int stream = 0;
            char* OutputName = GetDeclaredOutputName(psContext, HULL_SHADER, psOperand, &stream);

            ASSERT(psDecl->asOperands[0].ui32RegisterNumber != 0);  //Reg 0 should be gl_out[gl_InvocationID].gl_Position.

            if (HaveInOutLocationQualifier(psContext->psShader->eTargetLanguage, psContext->psShader->extensions))
            {
                bformata(glsl, "layout(location = %d) ", psDecl->asOperands[0].ui32RegisterNumber);
            }
            bformata(glsl, "out %s4 %s[];\n", type, OutputName);
            bformata(glsl, "#define Output%d %s[gl_InvocationID]\n", psDecl->asOperands[0].ui32RegisterNumber, OutputName);
            bcstrfree(OutputName);
            break;
        }
        case DOMAIN_SHADER:
        {
            int stream = 0;
            char* OutputName = GetDeclaredOutputName(psContext, DOMAIN_SHADER, psOperand, &stream);
            if (HaveInOutLocationQualifier(psContext->psShader->eTargetLanguage, psContext->psShader->extensions))
            {
                bformata(glsl, "layout(location = %d) ", psDecl->asOperands[0].ui32RegisterNumber);
            }
            bformata(glsl, "out %s4 %s;\n", type, OutputName);
            bformata(glsl, "#define Output%d %s\n", psDecl->asOperands[0].ui32RegisterNumber, OutputName);
            bcstrfree(OutputName);
            break;
        }
        }
    }
    else
    {
        /*
        Multiple outputs can be packed into one register. e.g.
        // Name                 Index   Mask Register SysValue  Format   Used
        // -------------------- ----- ------ -------- -------- ------- ------
        // FACTOR                   0   x           3     NONE     int   x
        // MAX                      0    y          3     NONE     int    y

        We want unique outputs to make it easier to use transform feedback.

        out  ivec4 FACTOR0;
        #define Output3 FACTOR0
        out  ivec4 MAX0;

        MAIN SHADER CODE. Writes factor and max to Output3 which aliases FACTOR0.

        MAX0.x = FACTOR0.y;

        This unpacking of outputs is only done when using HLSLCC_FLAG_INOUT_SEMANTIC_NAMES.
        When not set the application will be using HLSL reflection information to discover
        what the input and outputs mean if need be.
        */

        //

        if ((psContext->flags & HLSLCC_FLAG_INOUT_SEMANTIC_NAMES) && (psDecl->asOperands[0].eType == OPERAND_TYPE_OUTPUT))
        {
            const Operand* psOperand = &psDecl->asOperands[0];
            InOutSignature* psSignature = NULL;
            const char* type = "vec";
            int stream = 0;
            char* OutputName = GetDeclaredOutputName(psContext, psShader->eShaderType, psOperand, &stream);

            GetOutputSignatureFromRegister(psOperand->ui32RegisterNumber,
                psOperand->ui32CompMask,
                0,
                &psShader->sInfo,
                &psSignature);

            if (HaveInOutLocationQualifier(psContext->psShader->eTargetLanguage, psContext->psShader->extensions))
            {
                bformata(glsl, "layout(location = %d) ", psDecl->asOperands[0].ui32RegisterNumber);
            }

            switch (psSignature->eComponentType)
            {
            case INOUT_COMPONENT_UINT32:
            {
                type = "uvec";
                break;
            }
            case INOUT_COMPONENT_SINT32:
            {
                type = "ivec";
                break;
            }
            case INOUT_COMPONENT_FLOAT32:
            {
                break;
            }
            }
            bformata(glsl, "out %s4 %s;\n", type, OutputName);

            psContext->havePostShaderCode[psContext->currentPhase] = 1;

            psContext->currentGLSLString = &psContext->postShaderCode[psContext->currentPhase];
            glsl = *psContext->currentGLSLString;

            bcatcstr(glsl, OutputName);
            bcstrfree(OutputName);
            AddSwizzleUsingElementCount(psContext, GetNumSwizzleElements(psOperand));
            bformata(glsl, " = Output%d", psOperand->ui32RegisterNumber);
            TranslateOperandSwizzle(psContext, psOperand);
            bcatcstr(glsl, ";\n");

            psContext->currentGLSLString = &psContext->glsl;
            glsl = *psContext->currentGLSLString;
        }
    }
}

void DeclareUBOConstants(HLSLCrossCompilerContext* psContext, const uint32_t ui32BindingPoint, ConstantBuffer* psCBuf)
{
    bstring glsl = *psContext->currentGLSLString;

    uint32_t i, implicitOffset;
    const char* Name = psCBuf->Name;
    uint32_t auiSortedVars[MAX_SHADER_VARS];
    if (psCBuf->Name[0] == '$') //For $Globals
    {
        Name++;
    }

    for (i = 0; i < psCBuf->ui32NumVars; ++i)
    {
        PreDeclareStructType(psContext, psCBuf->asVars[i].sType.Name, &psCBuf->asVars[i].sType);
    }

    /* [layout (location = X)] uniform vec4 HLSLConstantBufferName[numConsts]; */
    if (HaveUniformBindingsAndLocations(psContext->psShader->eTargetLanguage, psContext->psShader->extensions) && (psContext->flags & HLSLCC_FLAG_AVOID_RESOURCE_BINDINGS_AND_LOCATIONS) == 0)
    {
        bformata(glsl, "layout(binding = %d) ", ui32BindingPoint);
    }

    bformata(glsl, "uniform ");
    ConvertToUniformBufferName(glsl, psContext->psShader, psCBuf->Name);
    bformata(glsl, " {\n ");

    if (psCBuf->ui32NumVars > 0)
    {
        uint32_t bSorted = 1;
        auiSortedVars[0] = 0;
        for (i = 1; i < psCBuf->ui32NumVars; ++i)
        {
            auiSortedVars[i] = i;
            bSorted = bSorted && psCBuf->asVars[i - 1].ui32StartOffset <= psCBuf->asVars[i].ui32StartOffset;
        }
        while (!bSorted)
        {
            bSorted = 1;
            for (i = 1; i < psCBuf->ui32NumVars; ++i)
            {
                if (psCBuf->asVars[auiSortedVars[i - 1]].ui32StartOffset > psCBuf->asVars[auiSortedVars[i]].ui32StartOffset)
                {
                    uint32_t uiTemp = auiSortedVars[i];
                    auiSortedVars[i] = auiSortedVars[i - 1];
                    auiSortedVars[i - 1] = uiTemp;
                    bSorted = 0;
                }
            }
        }
    }

    implicitOffset = 0;
    for (i = 0; i < psCBuf->ui32NumVars; ++i)
    {
        uint32_t uVarAlignment, uVarSize;
        ShaderVar* psVar = psCBuf->asVars + auiSortedVars[i];
        GetSTD140Layout(&psVar->sType, &uVarAlignment, &uVarSize);

        if ((implicitOffset + 16 - 1) / 16 < psVar->ui32StartOffset / 16)
        {
            uint32_t uNumPaddingUvecs = psVar->ui32StartOffset / 16 - (implicitOffset + 16 - 1) / 16;
            bcatcstr(glsl, "\tuvec4 padding_");
            ConvertToUniformBufferName(glsl, psContext->psShader, psCBuf->Name);
            bformata(glsl, "_%d[%d];\n", implicitOffset, uNumPaddingUvecs);
            implicitOffset = psVar->ui32StartOffset - psVar->ui32StartOffset % 16;
        }

        if ((implicitOffset + 4 - 1) / 4 < psVar->ui32StartOffset / 4)
        {
            uint32_t uNumPaddingUints = psVar->ui32StartOffset / 4 - (implicitOffset + 4 - 1) / 4;
            uint32_t uPaddingUint;
            for (uPaddingUint = 0; uPaddingUint < uNumPaddingUints; ++uPaddingUint)
            {
                bcatcstr(glsl, "\tuint padding_");
                ConvertToUniformBufferName(glsl, psContext->psShader, psCBuf->Name);
                bformata(glsl, "_%d_%d;\n", psVar->ui32StartOffset, uPaddingUint);
            }
            implicitOffset = psVar->ui32StartOffset - psVar->ui32StartOffset % 4;
        }

        implicitOffset += uVarAlignment - 1;
        implicitOffset -= implicitOffset % uVarAlignment;

        ASSERT(implicitOffset == psVar->ui32StartOffset);

        DeclareConstBufferShaderVariable(psContext, psVar->sType.Name, &psVar->sType, 0);
        implicitOffset += uVarSize;
    }

    bcatcstr(glsl, "};\n");
}

void DeclareBufferVariable(HLSLCrossCompilerContext* psContext, const uint32_t ui32BindingPoint, ConstantBuffer* psCBuf, const Operand* psOperand, const uint32_t ui32GloballyCoherentAccess, const ResourceType eResourceType)
{
    const char* Name = psCBuf->Name;
    bstring StructName;
    uint32_t unnamed_struct = strcmp(psCBuf->asVars[0].Name, "$Element") == 0 ? 1 : 0;
    bstring glsl = *psContext->currentGLSLString;

    ASSERT(psCBuf->ui32NumVars == 1);
    ASSERT(unnamed_struct);

    StructName = bfromcstr("");

    //TranslateOperand(psContext, psOperand, TO_FLAG_NAME_ONLY);
    if (psOperand->eType == OPERAND_TYPE_RESOURCE && eResourceType == RTYPE_STRUCTURED)
    {
        bformata(StructName, "StructuredRes%d", psOperand->ui32RegisterNumber);
    }
    else if (psOperand->eType == OPERAND_TYPE_RESOURCE && eResourceType == RTYPE_UAV_RWBYTEADDRESS)
    {
        bformata(StructName, "RawRes%d", psOperand->ui32RegisterNumber);
    }
    else
    {
        bformata(StructName, "UAV%d", psOperand->ui32RegisterNumber);
    }

    PreDeclareStructType(psContext, bstr2cstr(StructName, '\0'), &psCBuf->asVars[0].sType);

    // Add 'std430' layout for storage buffers.
    // We don't use a global setting for all buffers because Mali drivers don't like that.
    bcatcstr(glsl, "layout(std430");

    /* [layout (location = X)] uniform vec4 HLSLConstantBufferName[numConsts]; */
    // If storage blocking binding is not supported, then we must set the binding location in the shader. If we don't do it,
    // all the storage buffers of the program get assigned the same value (0).
    // Unfortunately this could cause binding collisions between different render stages for a storage buffer.
    if (HaveUniformBindingsAndLocations(psContext->psShader->eTargetLanguage, psContext->psShader->extensions) &&
        (!StorageBlockBindingSupported(psContext->psShader->eTargetLanguage) || (psContext->flags & HLSLCC_FLAG_AVOID_RESOURCE_BINDINGS_AND_LOCATIONS) == 0))
    {
        bformata(glsl, ", binding = %d", ui32BindingPoint);
    }

    // Close 'layout'
    bcatcstr(glsl, ")");

    if (ui32GloballyCoherentAccess & GLOBALLY_COHERENT_ACCESS)
    {
        bcatcstr(glsl, "coherent ");
    }

    if (eResourceType == RTYPE_STRUCTURED)
    {
        bcatcstr(glsl, "readonly ");
    }

    bcatcstr(glsl, "buffer ");
    if (eResourceType == RTYPE_STRUCTURED)
    {
        ConvertToTextureName(glsl, psContext->psShader, Name, NULL, 0);
    }
    else
    {
        ConvertToUAVName(glsl, psContext->psShader, Name);
    }
    bcatcstr(glsl, " {\n ");

    DeclareConstBufferShaderVariable(psContext, bstr2cstr(StructName, '\0'), &psCBuf->asVars[0].sType, 1);

    bcatcstr(glsl, "};\n");

    bdestroy(StructName);
}

void DeclarePLSVariable(HLSLCrossCompilerContext* psContext, const uint32_t ui32BindingPoint, ConstantBuffer* plsVar, const Operand* psOperand, const uint32_t ui32GloballyCoherentAccess, const ResourceType eResourceType)
{
    (void)psOperand;
    (void)ui32GloballyCoherentAccess;
    (void)eResourceType;

    const char* Name = plsVar->Name;
    uint32_t unnamed_struct = strcmp(plsVar->asVars[0].Name, "$Element") == 0 ? 1 : 0;
    bstring glsl = *psContext->currentGLSLString;

    ASSERT(plsVar->ui32NumVars == 1);
    ASSERT(unnamed_struct);

    // Define extension
    // TODO: if we need more than one PLS var... we can't redefine the extension every time
    // Extensions need to be declared before any non-preprocessor symbols. So we put it all the way at the beginning.
    bstring ext = bfromcstralloc(1024, "#extension GL_EXT_shader_pixel_local_storage : require\n");
    bconcat(ext, glsl);
    bassign(glsl, ext);
    bdestroy(ext);

    switch (ui32BindingPoint)
    {
    case GMEM_PLS_RO_SLOT:
        bcatcstr(glsl, "__pixel_local_inEXT PLS_STRUCT_READ_ONLY");
        break;
    case GMEM_PLS_WO_SLOT:
        bcatcstr(glsl, "__pixel_local_outEXT PLS_STRUCT_WRITE_ONLY");
        break;
    case GMEM_PLS_RW_SLOT:
        bcatcstr(glsl, "__pixel_localEXT PLS_STRUCT_READ_WRITE");
        break;
    default:
        ASSERT(0);
    }

    bcatcstr(glsl, "\n{\n");

    ASSERT(plsVar->ui32NumVars == 1);
    ASSERT(plsVar->asVars[0].sType.Members != 0);
    DeclarePLSStructVars(psContext, plsVar->asVars[0].sType.Name, &plsVar->asVars[0].sType);

    bcatcstr(glsl, "\n} ");
    ConvertToUAVName(glsl, psContext->psShader, Name);
    bcatcstr(glsl, ";\n\n");
}

void DeclareStructConstants(HLSLCrossCompilerContext* psContext, const uint32_t ui32BindingPoint, ConstantBuffer* psCBuf, const Operand* psOperand)
{
    bstring glsl = *psContext->currentGLSLString;

    uint32_t i;

    for (i = 0; i < psCBuf->ui32NumVars; ++i)
    {
        PreDeclareStructType(psContext, psCBuf->asVars[i].sType.Name, &psCBuf->asVars[i].sType);
    }

    /* [layout (location = X)] uniform vec4 HLSLConstantBufferName[numConsts]; */
    if (HaveUniformBindingsAndLocations(psContext->psShader->eTargetLanguage, psContext->psShader->extensions) && (psContext->flags & HLSLCC_FLAG_AVOID_RESOURCE_BINDINGS_AND_LOCATIONS) == 0)
    {
        bformata(glsl, "layout(location = %d) ", ui32BindingPoint);
    }
    bcatcstr(glsl, "uniform struct ");
    TranslateOperand(psContext, psOperand, TO_FLAG_DECLARATION_NAME);

    bcatcstr(glsl, "_Type {\n");

    for (i = 0; i < psCBuf->ui32NumVars; ++i)
    {
        DeclareConstBufferShaderVariable(psContext, psCBuf->asVars[i].sType.Name, &psCBuf->asVars[i].sType, 0);
    }

    bcatcstr(glsl, "} ");

    TranslateOperand(psContext, psOperand, TO_FLAG_DECLARATION_NAME);

    bcatcstr(glsl, ";\n");
}

void TranslateDeclaration(HLSLCrossCompilerContext* psContext, const Declaration* psDecl)
{
    bstring glsl = *psContext->currentGLSLString;
    Shader* psShader = psContext->psShader;

    switch (psDecl->eOpcode)
    {
    case OPCODE_DCL_INPUT_SGV:
    case OPCODE_DCL_INPUT_PS_SGV:
    case OPCODE_DCL_INPUT_PS_SIV:
    {
        const SPECIAL_NAME eSpecialName = psDecl->asOperands[0].eSpecialName;
        switch (eSpecialName)
        {
        case NAME_POSITION:
        {
            if (psShader->eShaderType == PIXEL_SHADER)
            {
                AddBuiltinInput(psContext, psDecl, "gl_FragCoord", 4);
            }
            else
            {
                AddBuiltinInput(psContext, psDecl, "gl_Position", 4);
            }
            break;
        }
        case NAME_RENDER_TARGET_ARRAY_INDEX:
        {
            AddBuiltinInput(psContext, psDecl, "gl_Layer", 1);
            break;
        }
        case NAME_CLIP_DISTANCE:
        {
            AddBuiltinInput(psContext, psDecl, "gl_ClipDistance", 4);
            break;
        }
        case NAME_VIEWPORT_ARRAY_INDEX:
        {
            AddBuiltinInput(psContext, psDecl, "gl_ViewportIndex", 1);
            break;
        }
        case NAME_INSTANCE_ID:
        {
            AddBuiltinInput(psContext, psDecl, "uint(gl_InstanceID)", 1);
            break;
        }
        case NAME_IS_FRONT_FACE:
        {
            /*
            Cast to uint used because
            if(gl_FrontFacing != 0) failed to compiled on Intel HD 4000.
            Suggests no implicit conversion for bool<->uint.
            */

            AddBuiltinInput(psContext, psDecl, "uint(gl_FrontFacing)", 1);
            break;
        }
        case NAME_SAMPLE_INDEX:
        {
            AddBuiltinInput(psContext, psDecl, "gl_SampleID", 1);
            break;
        }
        case NAME_VERTEX_ID:
        {
            AddBuiltinInput(psContext, psDecl, "uint(gl_VertexID)", 1);
            break;
        }
        case NAME_PRIMITIVE_ID:
        {
            AddBuiltinInput(psContext, psDecl, "gl_PrimitiveID", 1);
            break;
        }
        default:
        {
            bformata(glsl, "in vec4 %s;\n", psDecl->asOperands[0].pszSpecialName);

            bcatcstr(glsl, "#define ");
            TranslateOperand(psContext, &psDecl->asOperands[0], TO_FLAG_NONE);
            bformata(glsl, " %s\n", psDecl->asOperands[0].pszSpecialName);
            break;
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
            AddBuiltinOutput(psContext, psDecl, GLVARTYPE_FLOAT4, 0, "gl_Position");
            break;
        }
        case NAME_RENDER_TARGET_ARRAY_INDEX:
        {
            AddBuiltinOutput(psContext, psDecl, GLVARTYPE_INT, 0, "gl_Layer");
            break;
        }
        case NAME_CLIP_DISTANCE:
        {
            AddBuiltinOutput(psContext, psDecl, GLVARTYPE_FLOAT, 0, "gl_ClipDistance");
            break;
        }
        case NAME_VIEWPORT_ARRAY_INDEX:
        {
            AddBuiltinOutput(psContext, psDecl, GLVARTYPE_INT, 0, "gl_ViewportIndex");
            break;
        }
        case NAME_VERTEX_ID:
        {
            ASSERT(0);         //VertexID is not an output
            break;
        }
        case NAME_PRIMITIVE_ID:
        {
            AddBuiltinOutput(psContext, psDecl, GLVARTYPE_INT, 0, "gl_PrimitiveID");
            break;
        }
        case NAME_INSTANCE_ID:
        {
            ASSERT(0);         //InstanceID is not an output
            break;
        }
        case NAME_IS_FRONT_FACE:
        {
            ASSERT(0);         //FrontFacing is not an output
            break;
        }
        case NAME_FINAL_QUAD_U_EQ_0_EDGE_TESSFACTOR:
        {
            if (psContext->psShader->aIndexedOutput[psDecl->asOperands[0].ui32RegisterNumber])
            {
                AddBuiltinOutput(psContext, psDecl, GLVARTYPE_FLOAT, 4, "gl_TessLevelOuter");
            }
            else
            {
                AddBuiltinOutput(psContext, psDecl, GLVARTYPE_FLOAT, 0, "gl_TessLevelOuter[0]");
            }
            break;
        }
        case NAME_FINAL_QUAD_V_EQ_0_EDGE_TESSFACTOR:
        {
            AddBuiltinOutput(psContext, psDecl, GLVARTYPE_FLOAT, 0, "gl_TessLevelOuter[1]");
            break;
        }
        case NAME_FINAL_QUAD_U_EQ_1_EDGE_TESSFACTOR:
        {
            AddBuiltinOutput(psContext, psDecl, GLVARTYPE_FLOAT, 0, "gl_TessLevelOuter[2]");
            break;
        }
        case NAME_FINAL_QUAD_V_EQ_1_EDGE_TESSFACTOR:
        {
            AddBuiltinOutput(psContext, psDecl, GLVARTYPE_FLOAT, 0, "gl_TessLevelOuter[3]");
            break;
        }
        case NAME_FINAL_TRI_U_EQ_0_EDGE_TESSFACTOR:
        {
            if (psContext->psShader->aIndexedOutput[psDecl->asOperands[0].ui32RegisterNumber])
            {
                AddBuiltinOutput(psContext, psDecl, GLVARTYPE_FLOAT, 3, "gl_TessLevelOuter");
            }
            else
            {
                AddBuiltinOutput(psContext, psDecl, GLVARTYPE_FLOAT, 0, "gl_TessLevelOuter[0]");
            }
            break;
        }
        case NAME_FINAL_TRI_V_EQ_0_EDGE_TESSFACTOR:
        {
            AddBuiltinOutput(psContext, psDecl, GLVARTYPE_FLOAT, 0, "gl_TessLevelOuter[1]");
            break;
        }
        case NAME_FINAL_TRI_W_EQ_0_EDGE_TESSFACTOR:
        {
            AddBuiltinOutput(psContext, psDecl, GLVARTYPE_FLOAT, 0, "gl_TessLevelOuter[2]");
            break;
        }
        case NAME_FINAL_LINE_DENSITY_TESSFACTOR:
        {
            if (psContext->psShader->aIndexedOutput[psDecl->asOperands[0].ui32RegisterNumber])
            {
                AddBuiltinOutput(psContext, psDecl, GLVARTYPE_FLOAT, 2, "gl_TessLevelOuter");
            }
            else
            {
                AddBuiltinOutput(psContext, psDecl, GLVARTYPE_FLOAT, 0, "gl_TessLevelOuter[0]");
            }
            break;
        }
        case NAME_FINAL_LINE_DETAIL_TESSFACTOR:
        {
            AddBuiltinOutput(psContext, psDecl, GLVARTYPE_FLOAT, 0, "gl_TessLevelOuter[1]");
            break;
        }
        case NAME_FINAL_TRI_INSIDE_TESSFACTOR:
        case NAME_FINAL_QUAD_U_INSIDE_TESSFACTOR:
        {
            if (psContext->psShader->aIndexedOutput[psDecl->asOperands[0].ui32RegisterNumber])
            {
                AddBuiltinOutput(psContext, psDecl, GLVARTYPE_FLOAT, 2, "gl_TessLevelInner");
            }
            else
            {
                AddBuiltinOutput(psContext, psDecl, GLVARTYPE_FLOAT, 0, "gl_TessLevelInner[0]");
            }
            break;
        }
        case NAME_FINAL_QUAD_V_INSIDE_TESSFACTOR:
        {
            AddBuiltinOutput(psContext, psDecl, GLVARTYPE_FLOAT, 0, "gl_TessLevelInner[1]");
            break;
        }
        default:
        {
            bformata(glsl, "out vec4 %s;\n", psDecl->asOperands[0].pszSpecialName);

            bcatcstr(glsl, "#define ");
            TranslateOperand(psContext, &psDecl->asOperands[0], TO_FLAG_NONE);
            bformata(glsl, " %s\n", psDecl->asOperands[0].pszSpecialName);
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
        int iNumComponents = 4;    //GetMaxComponentFromComponentMask(psOperand);
        const char* StorageQualifier = "attribute";
        char* InputName;
        const char* Precision = "";

        if ((psOperand->eType == OPERAND_TYPE_INPUT_DOMAIN_POINT) ||
            (psOperand->eType == OPERAND_TYPE_OUTPUT_CONTROL_POINT_ID) ||
            (psOperand->eType == OPERAND_TYPE_INPUT_COVERAGE_MASK) ||
            (psOperand->eType == OPERAND_TYPE_INPUT_THREAD_ID) ||
            (psOperand->eType == OPERAND_TYPE_INPUT_THREAD_GROUP_ID) ||
            (psOperand->eType == OPERAND_TYPE_INPUT_THREAD_ID_IN_GROUP) ||
            (psOperand->eType == OPERAND_TYPE_INPUT_THREAD_ID_IN_GROUP_FLATTENED))
        {
            break;
        }

        //Already declared as part of an array.
        if (psShader->aIndexedInput[psDecl->asOperands[0].ui32RegisterNumber] == -1)
        {
            break;
        }

        InputName = GetDeclaredInputName(psContext, psShader->eShaderType, psOperand);

        if (InOutSupported(psContext->psShader->eTargetLanguage))
        {
            StorageQualifier = "in";
        }

        if (HavePrecisionQualifers(psShader->eTargetLanguage))
        {
            switch (psOperand->eMinPrecision)
            {
            case OPERAND_MIN_PRECISION_DEFAULT:
            {
                Precision = "highp";
                break;
            }
            case OPERAND_MIN_PRECISION_FLOAT_16:
            {
                Precision = "mediump";
                break;
            }
            case OPERAND_MIN_PRECISION_FLOAT_2_8:
            {
                Precision = "lowp";
                break;
            }
            case OPERAND_MIN_PRECISION_SINT_16:
            {
                Precision = "mediump";
                break;
            }
            case OPERAND_MIN_PRECISION_UINT_16:
            {
                Precision = "mediump";
                break;
            }
            }
        }

        DeclareInput(psContext, psDecl,
            "", StorageQualifier, Precision, iNumComponents, (OPERAND_INDEX_DIMENSION)psOperand->iIndexDims, InputName);
        bcstrfree(InputName);

        break;
    }
    case OPCODE_DCL_INPUT_SIV:
    {
        if (psShader->eShaderType == PIXEL_SHADER)
        {
            psShader->sInfo.aePixelInputInterpolation[psDecl->asOperands[0].ui32RegisterNumber] = psDecl->value.eInterpolation;
        }
        break;
    }
    case OPCODE_DCL_INPUT_PS:
    {
        const Operand* psOperand = &psDecl->asOperands[0];
        int iNumComponents = 4;    //GetMaxComponentFromComponentMask(psOperand);
        const char* StorageQualifier = "varying";
        const char* Precision = "";
        char* InputName = GetDeclaredInputName(psContext, PIXEL_SHADER, psOperand);
        const char* Interpolation = "";

        //Already declared as part of an array.
        if (psShader->aIndexedInput[psDecl->asOperands[0].ui32RegisterNumber] == -1)
        {
            break;
        }

        if (InOutSupported(psContext->psShader->eTargetLanguage))
        {
            StorageQualifier = "in";
        }

        switch (psDecl->value.eInterpolation)
        {
        case INTERPOLATION_CONSTANT:
        {
            Interpolation = "flat";
            break;
        }
        case INTERPOLATION_LINEAR:
        {
            break;
        }
        case INTERPOLATION_LINEAR_CENTROID:
        {
            Interpolation = "centroid";
            break;
        }
        case INTERPOLATION_LINEAR_NOPERSPECTIVE:
        {
            Interpolation = "noperspective";
            break;
        }
        case INTERPOLATION_LINEAR_NOPERSPECTIVE_CENTROID:
        {
            Interpolation = "noperspective centroid";
            break;
        }
        case INTERPOLATION_LINEAR_SAMPLE:
        {
            Interpolation = "sample";
            break;
        }
        case INTERPOLATION_LINEAR_NOPERSPECTIVE_SAMPLE:
        {
            Interpolation = "noperspective sample";
            break;
        }
        }

        if (HavePrecisionQualifers(psShader->eTargetLanguage))
        {
            switch (psOperand->eMinPrecision)
            {
            case OPERAND_MIN_PRECISION_DEFAULT:
            {
                Precision = "highp";
                break;
            }
            case OPERAND_MIN_PRECISION_FLOAT_16:
            {
                Precision = "mediump";
                break;
            }
            case OPERAND_MIN_PRECISION_FLOAT_2_8:
            {
                Precision = "lowp";
                break;
            }
            case OPERAND_MIN_PRECISION_SINT_16:
            {
                Precision = "mediump";
                break;
            }
            case OPERAND_MIN_PRECISION_UINT_16:
            {
                Precision = "mediump";
                break;
            }
            }
        }

        DeclareInput(psContext, psDecl,
            Interpolation, StorageQualifier, Precision, iNumComponents, INDEX_1D, InputName);
        bcstrfree(InputName);

        break;
    }
    case OPCODE_DCL_TEMPS:
    {
        uint32_t i = 0;
        const uint32_t ui32NumTemps = psDecl->value.ui32NumTemps;

        if (psContext->flags & HLSLCC_FLAG_AVOID_TEMP_REGISTER_ALIASING && psContext->psShader->eShaderType != HULL_SHADER)
        {
            break;
        }

        if (ui32NumTemps > 0)
        {
            bformata(glsl, "vec4 Temp[%d];\n", ui32NumTemps);
            if (psContext->psShader->bUseTempCopy)
            {
                bcatcstr(glsl, "vec4 TempCopy;\n");
            }

            bformata(glsl, "ivec4 Temp_int[%d];\n", ui32NumTemps);
            if (psContext->psShader->bUseTempCopy)
            {
                bcatcstr(glsl, "vec4 TempCopy_int;\n");
            }
            if (HaveUVec(psShader->eTargetLanguage))
            {
                bformata(glsl, "uvec4 Temp_uint[%d];\n", ui32NumTemps);
                if (psContext->psShader->bUseTempCopy)
                {
                    bcatcstr(glsl, "uvec4 TempCopy_uint;\n");
                }
            }
            if (psShader->fp64)
            {
                bformata(glsl, "dvec4 Temp_double[%d];\n", ui32NumTemps);
                if (psContext->psShader->bUseTempCopy)
                {
                    bcatcstr(glsl, "dvec4 TempCopy_double;\n");
                }
            }
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
            bformata(glsl, "const ivec4 IntImmConst%d = ", psDest->ui32RegisterNumber);
        }
        else
        {
            bformata(glsl, "const vec4 ImmConst%d = ", psDest->ui32RegisterNumber);
            AddToDx9ImmConstIndexableArray(psContext, psDest);
        }
        TranslateOperand(psContext, psSrc, TO_FLAG_NONE);
        bcatcstr(glsl, ";\n");

        break;
    }
    case OPCODE_DCL_CONSTANT_BUFFER:
    {
        const Operand* psOperand = &psDecl->asOperands[0];
        const uint32_t ui32BindingPoint = psOperand->aui32ArraySizes[0];

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
            if (HaveUniformBindingsAndLocations(psContext->psShader->eTargetLanguage, psContext->psShader->extensions) && (psContext->flags & HLSLCC_FLAG_AVOID_RESOURCE_BINDINGS_AND_LOCATIONS) == 0)
            {
                bformata(glsl, "layout(location = %d) ", ui32BindingPoint);
            }

            bformata(glsl, "layout(std140) uniform ConstantBuffer%d {\n\tvec4 data[%d];\n} cb%d;\n", ui32BindingPoint, psOperand->aui32ArraySizes[1], ui32BindingPoint);
            break;
        }
        else if (psCBuf->blob)
        {
            if (HaveUniformBindingsAndLocations(psContext->psShader->eTargetLanguage, psContext->psShader->extensions) && (psContext->flags & HLSLCC_FLAG_AVOID_RESOURCE_BINDINGS_AND_LOCATIONS) == 0)
            {
                bformata(glsl, "layout(location = %d) ", ui32BindingPoint);
            }

            bcatcstr(glsl, "layout(std140) uniform ");
            ConvertToUniformBufferName(glsl, psShader, psCBuf->Name);
            bcatcstr(glsl, " {\n\tvec4 ");
            ConvertToUniformBufferName(glsl, psShader, psCBuf->Name);
            bformata(glsl, "_data[%d];\n};\n", psOperand->aui32ArraySizes[1]);
            break;
        }

        if (psContext->flags & HLSLCC_FLAG_UNIFORM_BUFFER_OBJECT)
        {
            if (psContext->flags & HLSLCC_FLAG_GLOBAL_CONSTS_NEVER_IN_UBO && psCBuf->Name[0] == '$')
            {
                DeclareStructConstants(psContext, ui32BindingPoint, psCBuf, psOperand);
            }
            else
            {
                DeclareUBOConstants(psContext, ui32BindingPoint, psCBuf);
            }
        }
        else
        {
            DeclareStructConstants(psContext, ui32BindingPoint, psCBuf, psOperand);
        }
        break;
    }
    case OPCODE_DCL_RESOURCE:
    {
        bool isGmemResource = false;
        const int initialMemSize = 64;
        bstring earlyMain = bfromcstralloc(initialMemSize, "");
        if (IsGmemReservedSlot(FBF_EXT_COLOR, psDecl->asOperands[0].ui32RegisterNumber))
        {
            // A GMEM reserve slot was used.
            // This is not a resource but an inout RT of the pixel shader            
            int regNum = GetGmemInputResourceSlot(psDecl->asOperands[0].ui32RegisterNumber);
            // FXC thinks this is a texture so we can't trust the number of elements. We get that from the "register number".
            int numElements = GetGmemInputResourceNumElements(psDecl->asOperands[0].ui32RegisterNumber);
            ASSERT(numElements);

            const char* Precision = "highp";
            const char* outputName = "PixOutput";
            const char* qualifier = psContext->rendertargetUse[regNum] & OUTPUT_RENDERTARGET ? "inout" : "in";

            bformata(glsl, "layout(location = %d) ", regNum);
            bformata(glsl, "inout %s vec%d %s%d;\n", Precision, numElements, outputName, regNum);

            const char* mask[] = { "x", "y", "z", "w" };
            // Since we are using Textures as GMEM inputs FXC will threat them as vec4 values. The rendertarget may not be a vec4 (numElements != 4)
            // so we create a new variable (GMEM_InputXX) at the beginning of the shader that wraps the rendertarget value.
            bformata(earlyMain, "%s vec4 GMEM_Input%d = %s vec4(%s%d.", Precision, regNum, Precision, outputName, regNum);
            for (int i = 0; i < 4; ++i)
            {
                bformata(earlyMain, "%s", i < numElements ? mask[i] : mask[numElements - 1]);
            }
            bcatcstr(earlyMain, ");\n");
            isGmemResource = true;
        }
        else if (IsGmemReservedSlot(FBF_ARM_COLOR, psDecl->asOperands[0].ui32RegisterNumber))
        {
            bcatcstr(earlyMain, "vec4 GMEM_Input0 = vec4(gl_LastFragColorARM);\n");
            isGmemResource = true;
        }
        else if (IsGmemReservedSlot(FBF_ARM_DEPTH, psDecl->asOperands[0].ui32RegisterNumber))
        {
            bcatcstr(earlyMain, "vec4 GMEM_Depth = vec4(gl_LastFragDepthARM);\n");
            isGmemResource = true;
        }
        else if (IsGmemReservedSlot(FBF_ARM_STENCIL, psDecl->asOperands[0].ui32RegisterNumber))
        {
            bcatcstr(earlyMain, "ivec4 GMEM_Stencil = ivec4(gl_LastFragStencilARM);\n");
            isGmemResource = true;
        }

        if (isGmemResource)
        {
            if (earlyMain->slen)
            {
                bstring* savedStringPtr = psContext->currentGLSLString;
                psContext->currentGLSLString = &psContext->earlyMain;
                psContext->indent++;
                AddIndentation(psContext);
                bconcat(*psContext->currentGLSLString, earlyMain);
                psContext->indent--;
                psContext->currentGLSLString = savedStringPtr;
            }
            break;
        }

        char* szResourceTypeName = "";
        uint32_t bCanBeCompare;
        uint32_t i;
        SamplerMask sMask;

        if (HaveUniformBindingsAndLocations(psContext->psShader->eTargetLanguage, psContext->psShader->extensions) && (psContext->flags & HLSLCC_FLAG_AVOID_RESOURCE_BINDINGS_AND_LOCATIONS) == 0)
        {
            //Constant buffer locations start at 0. Resource locations start at ui32NumConstantBuffers.
            bformata(glsl, "layout(location = %d) ", psContext->psShader->sInfo.ui32NumConstantBuffers + psDecl->asOperands[0].ui32RegisterNumber);
        }

        switch (psDecl->value.eResourceDimension)
        {
        case RESOURCE_DIMENSION_BUFFER:
            szResourceTypeName = "Buffer";
            bCanBeCompare = 0;
            break;
        case RESOURCE_DIMENSION_TEXTURE1D:
            szResourceTypeName = "1D";
            bCanBeCompare = 1;
            break;
        case RESOURCE_DIMENSION_TEXTURE2D:
            szResourceTypeName = "2D";
            bCanBeCompare = 1;
            break;
        case RESOURCE_DIMENSION_TEXTURE2DMS:
            szResourceTypeName = "2DMS";
            bCanBeCompare = 0;
            break;
        case RESOURCE_DIMENSION_TEXTURE3D:
            szResourceTypeName = "3D";
            bCanBeCompare = 0;
            break;
        case RESOURCE_DIMENSION_TEXTURECUBE:
            szResourceTypeName = "Cube";
            bCanBeCompare = 1;
            break;
        case RESOURCE_DIMENSION_TEXTURE1DARRAY:
            szResourceTypeName = "1DArray";
            bCanBeCompare = 1;
            break;
        case RESOURCE_DIMENSION_TEXTURE2DARRAY:
            szResourceTypeName = "2DArray";
            bCanBeCompare = 1;
            break;
        case RESOURCE_DIMENSION_TEXTURE2DMSARRAY:
            szResourceTypeName = "2DMSArray";
            bCanBeCompare = 0;
            break;
        case RESOURCE_DIMENSION_TEXTURECUBEARRAY:
            szResourceTypeName = "CubeArray";
            bCanBeCompare = 1;
            break;
        }

        for (i = 0; i < psShader->sInfo.ui32NumSamplers; ++i)
        {
            if (psShader->sInfo.asSamplers[i].sMask.ui10TextureBindPoint == psDecl->asOperands[0].ui32RegisterNumber)
            {
                sMask = psShader->sInfo.asSamplers[i].sMask;

                if (bCanBeCompare && sMask.bCompareSample)     // Sampled with depth comparison
                {
                    bformata(glsl, "uniform sampler%sShadow ", szResourceTypeName);
                    TextureName(*psContext->currentGLSLString, psContext->psShader, psDecl->asOperands[0].ui32RegisterNumber, sMask.ui10SamplerBindPoint, 1);
                    bcatcstr(glsl, ";\n");
                }
                if (sMask.bNormalSample || !sMask.bCompareSample)     // Either sampled normally or with texelFetch
                {
                    if (psDecl->ui32TexReturnType == RETURN_TYPE_SINT)
                    {
                        bformata(glsl, "uniform isampler%s ", szResourceTypeName);
                    }
                    else if (psDecl->ui32TexReturnType == RETURN_TYPE_UINT)
                    {
                        bformata(glsl, "uniform usampler%s ", szResourceTypeName);
                    }
                    else
                    {
                        bformata(glsl, "uniform sampler%s ", szResourceTypeName);
                    }
                    TextureName(*psContext->currentGLSLString, psContext->psShader, psDecl->asOperands[0].ui32RegisterNumber, sMask.ui10SamplerBindPoint, 0);
                    bcatcstr(glsl, ";\n");
                }
            }
        }

        ASSERT(psDecl->asOperands[0].ui32RegisterNumber < MAX_TEXTURES);
        psShader->aeResourceDims[psDecl->asOperands[0].ui32RegisterNumber] = psDecl->value.eResourceDimension;
        break;
    }
    case OPCODE_DCL_OUTPUT:
    {
        if (psShader->eShaderType == HULL_SHADER && psDecl->asOperands[0].ui32RegisterNumber == 0)
        {
            AddBuiltinOutput(psContext, psDecl, GLVARTYPE_FLOAT4, 0, "gl_out[gl_InvocationID].gl_Position");
        }
        else
        {
            AddUserOutput(psContext, psDecl);
        }
        break;
    }
    case OPCODE_DCL_GLOBAL_FLAGS:
    {
        uint32_t ui32Flags = psDecl->value.ui32GlobalFlags;

        // OpenGL versions lower than 4.1 don't support the
        // layout(early_fragment_tests) directive and will fail to compile
        // the shader
        if (ui32Flags & GLOBAL_FLAG_FORCE_EARLY_DEPTH_STENCIL && EarlyDepthTestSupported(psShader->eTargetLanguage) && 
            !(psShader->eGmemType & (FBF_ARM_DEPTH | FBF_ARM_STENCIL))) // Early fragment test is not allowed when fetching from the depth/stencil buffer. 
        {
            bcatcstr(glsl, "layout(early_fragment_tests) in;\n");
        }
        if (!(ui32Flags & GLOBAL_FLAG_REFACTORING_ALLOWED))
        {
            //TODO add precise
            //HLSL precise - http://msdn.microsoft.com/en-us/library/windows/desktop/hh447204(v=vs.85).aspx
        }
        if (ui32Flags & GLOBAL_FLAG_ENABLE_DOUBLE_PRECISION_FLOAT_OPS)
        {
            bcatcstr(glsl, "#extension GL_ARB_gpu_shader_fp64 : enable\n");
            psShader->fp64 = 1;
        }
        break;
    }

    case OPCODE_DCL_THREAD_GROUP:
    {
        bformata(glsl, "layout(local_size_x = %d, local_size_y = %d, local_size_z = %d) in;\n",
            psDecl->value.aui32WorkGroupSize[0],
            psDecl->value.aui32WorkGroupSize[1],
            psDecl->value.aui32WorkGroupSize[2]);
        break;
    }
    case OPCODE_DCL_TESS_OUTPUT_PRIMITIVE:
    {
        if (psContext->psShader->eShaderType == HULL_SHADER)
        {
            psContext->psShader->sInfo.eTessOutPrim = psDecl->value.eTessOutPrim;
        }
        break;
    }
    case OPCODE_DCL_TESS_DOMAIN:
    {
        if (psContext->psShader->eShaderType == DOMAIN_SHADER)
        {
            switch (psDecl->value.eTessDomain)
            {
            case TESSELLATOR_DOMAIN_ISOLINE:
            {
                bcatcstr(glsl, "layout(isolines) in;\n");
                break;
            }
            case TESSELLATOR_DOMAIN_TRI:
            {
                bcatcstr(glsl, "layout(triangles) in;\n");
                break;
            }
            case TESSELLATOR_DOMAIN_QUAD:
            {
                bcatcstr(glsl, "layout(quads) in;\n");
                break;
            }
            default:
            {
                break;
            }
            }
        }
        break;
    }
    case OPCODE_DCL_TESS_PARTITIONING:
    {
        if (psContext->psShader->eShaderType == HULL_SHADER)
        {
            psContext->psShader->sInfo.eTessPartitioning = psDecl->value.eTessPartitioning;
        }
        break;
    }
    case OPCODE_DCL_GS_OUTPUT_PRIMITIVE_TOPOLOGY:
    {
        switch (psDecl->value.eOutputPrimitiveTopology)
        {
        case PRIMITIVE_TOPOLOGY_POINTLIST:
        {
            bcatcstr(glsl, "layout(points) out;\n");
            break;
        }
        case PRIMITIVE_TOPOLOGY_LINELIST_ADJ:
        case PRIMITIVE_TOPOLOGY_LINESTRIP_ADJ:
        case PRIMITIVE_TOPOLOGY_LINELIST:
        case PRIMITIVE_TOPOLOGY_LINESTRIP:
        {
            bcatcstr(glsl, "layout(line_strip) out;\n");
            break;
        }

        case PRIMITIVE_TOPOLOGY_TRIANGLELIST_ADJ:
        case PRIMITIVE_TOPOLOGY_TRIANGLESTRIP_ADJ:
        case PRIMITIVE_TOPOLOGY_TRIANGLESTRIP:
        case PRIMITIVE_TOPOLOGY_TRIANGLELIST:
        {
            bcatcstr(glsl, "layout(triangle_strip) out;\n");
            break;
        }
        default:
        {
            break;
        }
        }
        break;
    }
    case OPCODE_DCL_MAX_OUTPUT_VERTEX_COUNT:
    {
        bformata(glsl, "layout(max_vertices = %d) out;\n", psDecl->value.ui32MaxOutputVertexCount);
        break;
    }
    case OPCODE_DCL_GS_INPUT_PRIMITIVE:
    {
        switch (psDecl->value.eInputPrimitive)
        {
        case PRIMITIVE_POINT:
        {
            bcatcstr(glsl, "layout(points) in;\n");
            break;
        }
        case PRIMITIVE_LINE:
        {
            bcatcstr(glsl, "layout(lines) in;\n");
            break;
        }
        case PRIMITIVE_LINE_ADJ:
        {
            bcatcstr(glsl, "layout(lines_adjacency) in;\n");
            break;
        }
        case PRIMITIVE_TRIANGLE:
        {
            bcatcstr(glsl, "layout(triangles) in;\n");
            break;
        }
        case PRIMITIVE_TRIANGLE_ADJ:
        {
            bcatcstr(glsl, "layout(triangles_adjacency) in;\n");
            break;
        }
        default:
        {
            break;
        }
        }
        break;
    }
    case OPCODE_DCL_INTERFACE:
    {
        const uint32_t interfaceID = psDecl->value.interface.ui32InterfaceID;
        const uint32_t numUniforms = psDecl->value.interface.ui32ArraySize;
        const uint32_t ui32NumBodiesPerTable = psContext->psShader->funcPointer[interfaceID].ui32NumBodiesPerTable;
        ShaderVar* psVar;
        uint32_t varFound;

        const char* uniformName;

        varFound = GetInterfaceVarFromOffset(interfaceID, &psContext->psShader->sInfo, &psVar);
        ASSERT(varFound);
        uniformName = &psVar->sType.Name[0];

        bformata(glsl, "subroutine uniform SubroutineType %s[%d*%d];\n", uniformName, numUniforms, ui32NumBodiesPerTable);
        break;
    }
    case OPCODE_DCL_FUNCTION_BODY:
    {
        //bformata(glsl, "void Func%d();//%d\n", psDecl->asOperands[0].ui32RegisterNumber, psDecl->asOperands[0].eType);
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
        int integerCoords[4];
        bool qualcommWorkaround = (psContext->flags & HLSLCC_FLAG_QUALCOMM_GLES30_DRIVER_WORKAROUND) != 0;

        if (qualcommWorkaround)
        {
            bformata(glsl, "const ");
        }

        bformata(glsl, "ivec4 immediateConstBufferInt[%d] = ivec4[%d] (\n", ui32NumVec4, ui32NumVec4);
        for (ui32ConstIndex = 0; ui32ConstIndex < ui32NumVec4Minus1; ui32ConstIndex++)
        {
            integerCoords[0] = *(int*)&psDecl->asImmediateConstBuffer[ui32ConstIndex].a;
            integerCoords[1] = *(int*)&psDecl->asImmediateConstBuffer[ui32ConstIndex].b;
            integerCoords[2] = *(int*)&psDecl->asImmediateConstBuffer[ui32ConstIndex].c;
            integerCoords[3] = *(int*)&psDecl->asImmediateConstBuffer[ui32ConstIndex].d;

            bformata(glsl, "\tivec4(%d, %d, %d, %d), \n", integerCoords[0], integerCoords[1], integerCoords[2], integerCoords[3]);
        }
        //No trailing comma on this one
        integerCoords[0] = *(int*)&psDecl->asImmediateConstBuffer[ui32ConstIndex].a;
        integerCoords[1] = *(int*)&psDecl->asImmediateConstBuffer[ui32ConstIndex].b;
        integerCoords[2] = *(int*)&psDecl->asImmediateConstBuffer[ui32ConstIndex].c;
        integerCoords[3] = *(int*)&psDecl->asImmediateConstBuffer[ui32ConstIndex].d;

        bformata(glsl, "\tivec4(%d, %d, %d, %d)\n", integerCoords[0], integerCoords[1], integerCoords[2], integerCoords[3]);
        bcatcstr(glsl, ");\n");

        //If ShaderBitEncodingSupported then 1 integer buffer, use intBitsToFloat to get float values. - More instructions.
        //else 2 buffers - one integer and one float. - More data

        if (ShaderBitEncodingSupported(psShader->eTargetLanguage) == 0)
        {
            float floatCoords[4];
            bcatcstr(glsl, "#define immediateConstBufferI(idx) immediateConstBufferInt[idx]\n");
            bcatcstr(glsl, "#define immediateConstBufferF(idx) immediateConstBuffer[idx]\n");

            bformata(glsl, "vec4 immediateConstBuffer[%d] = vec4[%d] (\n", ui32NumVec4, ui32NumVec4);
            for (ui32ConstIndex = 0; ui32ConstIndex < ui32NumVec4Minus1; ui32ConstIndex++)
            {
                floatCoords[0] = *(float*)&psDecl->asImmediateConstBuffer[ui32ConstIndex].a;
                floatCoords[1] = *(float*)&psDecl->asImmediateConstBuffer[ui32ConstIndex].b;
                floatCoords[2] = *(float*)&psDecl->asImmediateConstBuffer[ui32ConstIndex].c;
                floatCoords[3] = *(float*)&psDecl->asImmediateConstBuffer[ui32ConstIndex].d;

                //A single vec4 can mix integer and float types.
                //Forced NAN and INF to zero inside the immediate constant buffer. This will allow the shader to compile.
                if (fpcheck(floatCoords[0]))
                {
                    floatCoords[0] = 0;
                }
                if (fpcheck(floatCoords[1]))
                {
                    floatCoords[1] = 0;
                }
                if (fpcheck(floatCoords[2]))
                {
                    floatCoords[2] = 0;
                }
                if (fpcheck(floatCoords[3]))
                {
                    floatCoords[3] = 0;
                }

                bformata(glsl, "\tvec4(%e, %e, %e, %e), \n", floatCoords[0], floatCoords[1], floatCoords[2], floatCoords[3]);
            }
            //No trailing comma on this one
            floatCoords[0] = *(float*)&psDecl->asImmediateConstBuffer[ui32ConstIndex].a;
            floatCoords[1] = *(float*)&psDecl->asImmediateConstBuffer[ui32ConstIndex].b;
            floatCoords[2] = *(float*)&psDecl->asImmediateConstBuffer[ui32ConstIndex].c;
            floatCoords[3] = *(float*)&psDecl->asImmediateConstBuffer[ui32ConstIndex].d;
            if (fpcheck(floatCoords[0]))
            {
                floatCoords[0] = 0;
            }
            if (fpcheck(floatCoords[1]))
            {
                floatCoords[1] = 0;
            }
            if (fpcheck(floatCoords[2]))
            {
                floatCoords[2] = 0;
            }
            if (fpcheck(floatCoords[3]))
            {
                floatCoords[3] = 0;
            }
            bformata(glsl, "\tvec4(%e, %e, %e, %e)\n", floatCoords[0], floatCoords[1], floatCoords[2], floatCoords[3]);
            bcatcstr(glsl, ");\n");
        }
        else
        {
            if (qualcommWorkaround)
            {
                bcatcstr(glsl, "ivec4 immediateConstBufferI(int idx) { return immediateConstBufferInt[idx]; }\n");
                bcatcstr(glsl, "vec4 immediateConstBufferF(int idx) { return intBitsToFloat(immediateConstBufferInt[idx]); }\n");
            }
            else
            {
                bcatcstr(glsl, "#define immediateConstBufferI(idx) immediateConstBufferInt[idx]\n");
                bcatcstr(glsl, "#define immediateConstBufferF(idx) intBitsToFloat(immediateConstBufferInt[idx])\n");
            }
        }

        break;
    }
    case OPCODE_DCL_HS_FORK_PHASE_INSTANCE_COUNT:
    {
        const uint32_t forkPhaseNum = psDecl->value.aui32HullPhaseInstanceInfo[0];
        const uint32_t instanceCount = psDecl->value.aui32HullPhaseInstanceInfo[1];
        bformata(glsl, "const int HullPhase%dInstanceCount = %d;\n", forkPhaseNum, instanceCount);
        break;
    }
    case OPCODE_DCL_INDEXABLE_TEMP:
    {
        const uint32_t ui32RegIndex = psDecl->sIdxTemp.ui32RegIndex;
        const uint32_t ui32RegCount = psDecl->sIdxTemp.ui32RegCount;
        const uint32_t ui32RegComponentSize = psDecl->sIdxTemp.ui32RegComponentSize;
        bformata(glsl, "vec%d TempArray%d[%d];\n", ui32RegComponentSize, ui32RegIndex, ui32RegCount);
        bformata(glsl, "ivec%d TempArray%d_int[%d];\n", ui32RegComponentSize, ui32RegIndex, ui32RegCount);
        if (HaveUVec(psShader->eTargetLanguage))
        {
            bformata(glsl, "uvec%d TempArray%d_uint[%d];\n", ui32RegComponentSize, ui32RegIndex, ui32RegCount);
        }
        if (psShader->fp64)
        {
            bformata(glsl, "dvec%d TempArray%d_double[%d];\n", ui32RegComponentSize, ui32RegIndex, ui32RegCount);
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
        if (psContext->psShader->eShaderType == HULL_SHADER)
        {
            bformata(glsl, "layout(vertices=%d) out;\n", psDecl->value.ui32MaxOutputVertexCount);
        }
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
    case OPCODE_DCL_SAMPLER:
    {
        break;
    }
    case OPCODE_DCL_HS_MAX_TESSFACTOR:
    {
        //For GLSL the max tessellation factor is fixed to the value of gl_MaxTessGenLevel.
        break;
    }
    case OPCODE_DCL_UNORDERED_ACCESS_VIEW_TYPED:
    {
        if (psDecl->sUAV.ui32GloballyCoherentAccess & GLOBALLY_COHERENT_ACCESS)
        {
            bcatcstr(glsl, "coherent ");
        }

        if (psShader->aiOpcodeUsed[OPCODE_LD_UAV_TYPED] == 0)
        {
            bcatcstr(glsl, "writeonly ");
        }
        else
        {
            if (psShader->aiOpcodeUsed[OPCODE_STORE_UAV_TYPED] == 0)
            {
                bcatcstr(glsl, "readonly ");
            }

            switch (psDecl->sUAV.Type)
            {
            case RETURN_TYPE_FLOAT:
                bcatcstr(glsl, "layout(rgba32f) ");
                break;
            case RETURN_TYPE_UNORM:
                bcatcstr(glsl, "layout(rgba8) ");
                break;
            case RETURN_TYPE_SNORM:
                bcatcstr(glsl, "layout(rgba8_snorm) ");
                break;
            case RETURN_TYPE_UINT:
                bcatcstr(glsl, "layout(rgba32ui) ");
                break;
            case RETURN_TYPE_SINT:
                bcatcstr(glsl, "layout(rgba32i) ");
                break;
            default:
                ASSERT(0);
            }
        }

        {
            char* prefix = "";
            switch (psDecl->sUAV.Type)
            {
            case RETURN_TYPE_UINT:
                prefix = "u";
                break;
            case RETURN_TYPE_SINT:
                prefix = "i";
                break;
            default:
                break;
            }

            switch (psDecl->value.eResourceDimension)
            {
            case RESOURCE_DIMENSION_BUFFER:
                bformata(glsl, "uniform %simageBuffer ", prefix);
                break;
            case RESOURCE_DIMENSION_TEXTURE1D:
                bformata(glsl, "uniform %simage1D ", prefix);
                break;
            case RESOURCE_DIMENSION_TEXTURE2D:
                bformata(glsl, "uniform %simage2D ", prefix);
                break;
            case RESOURCE_DIMENSION_TEXTURE2DMS:
                bformata(glsl, "uniform %simage2DMS ", prefix);
                break;
            case RESOURCE_DIMENSION_TEXTURE3D:
                bformata(glsl, "uniform %simage3D ", prefix);
                break;
            case RESOURCE_DIMENSION_TEXTURECUBE:
                bformata(glsl, "uniform %simageCube ", prefix);
                break;
            case RESOURCE_DIMENSION_TEXTURE1DARRAY:
                bformata(glsl, "uniform %simage1DArray ", prefix);
                break;
            case RESOURCE_DIMENSION_TEXTURE2DARRAY:
                bformata(glsl, "uniform %simage2DArray ", prefix);
                break;
            case RESOURCE_DIMENSION_TEXTURE2DMSARRAY:
                bformata(glsl, "uniform %simage3DArray ", prefix);
                break;
            case RESOURCE_DIMENSION_TEXTURECUBEARRAY:
                bformata(glsl, "uniform %simageCubeArray ", prefix);
                break;
            }
        }
        TranslateOperand(psContext, &psDecl->asOperands[0], TO_FLAG_NONE);
        bcatcstr(glsl, ";\n");
        break;
    }
    case OPCODE_DCL_UNORDERED_ACCESS_VIEW_STRUCTURED:
    {
        const uint32_t ui32BindingPoint = psDecl->asOperands[0].aui32ArraySizes[0];
        ConstantBuffer* psCBuf = NULL;

        if (psDecl->sUAV.bCounter)
        {
            bformata(glsl, "layout (binding = 1) uniform atomic_uint UAV%d_counter;\n", psDecl->asOperands[0].ui32RegisterNumber);
        }

        GetConstantBufferFromBindingPoint(RGROUP_UAV, ui32BindingPoint, &psContext->psShader->sInfo, &psCBuf);

        if (ui32BindingPoint >= GMEM_PLS_RO_SLOT && ui32BindingPoint <= GMEM_PLS_RW_SLOT)
        {
            DeclarePLSVariable(psContext, ui32BindingPoint, psCBuf, &psDecl->asOperands[0], psDecl->sUAV.ui32GloballyCoherentAccess, RTYPE_UAV_RWSTRUCTURED);
        }
        else
        {
            DeclareBufferVariable(psContext, ui32BindingPoint, psCBuf, &psDecl->asOperands[0], psDecl->sUAV.ui32GloballyCoherentAccess, RTYPE_UAV_RWSTRUCTURED);
        }
        break;
    }
    case OPCODE_DCL_UNORDERED_ACCESS_VIEW_RAW:
    {
        bstring varName;
        if (psDecl->sUAV.bCounter)
        {
            bformata(glsl, "layout (binding = 1) uniform atomic_uint UAV%d_counter;\n", psDecl->asOperands[0].ui32RegisterNumber);
        }

        varName = bfromcstralloc(16, "");
        bformata(varName, "UAV%d", psDecl->asOperands[0].ui32RegisterNumber);

        bformata(glsl, "buffer Block%d {\n\tuint ", psDecl->asOperands[0].ui32RegisterNumber);
        ShaderVarName(glsl, psShader, bstr2cstr(varName, '\0'));
        bcatcstr(glsl, "[];\n};\n");

        bdestroy(varName);
        break;
    }
    case OPCODE_DCL_RESOURCE_STRUCTURED:
    {
        ConstantBuffer* psCBuf = NULL;

        GetConstantBufferFromBindingPoint(RGROUP_TEXTURE, psDecl->asOperands[0].ui32RegisterNumber, &psContext->psShader->sInfo, &psCBuf);

        DeclareBufferVariable(psContext, psDecl->asOperands[0].ui32RegisterNumber, psCBuf, &psDecl->asOperands[0], 0, RTYPE_STRUCTURED);
        break;
    }
    case OPCODE_DCL_RESOURCE_RAW:
    {
        bstring varName = bfromcstralloc(16, "");
        bformata(varName, "RawRes%d", psDecl->asOperands[0].ui32RegisterNumber);

        bformata(glsl, "buffer Block%d {\n\tuint ", psDecl->asOperands[0].ui32RegisterNumber);
        ShaderVarName(glsl, psContext->psShader, bstr2cstr(varName, '\0'));
        bcatcstr(glsl, "[];\n};\n");

        bdestroy(varName);
        break;
    }
    case OPCODE_DCL_THREAD_GROUP_SHARED_MEMORY_RAW:
    {
        ShaderVarType* psVarType = &psShader->sGroupSharedVarType[psDecl->asOperands[0].ui32RegisterNumber];

        ASSERT(psDecl->asOperands[0].ui32RegisterNumber < MAX_GROUPSHARED);
        ASSERT(psDecl->sTGSM.ui32Count == 1);

        bcatcstr(glsl, "shared uint ");

        TranslateOperand(psContext, &psDecl->asOperands[0], TO_FLAG_NONE);
        bformata(glsl, "[%d];\n", psDecl->sTGSM.ui32Count);

        memset(psVarType, 0, sizeof(ShaderVarType));
        strcpy(psVarType->Name, "$Element");

        psVarType->Columns = psDecl->sTGSM.ui32Stride / 4;
        psVarType->Elements = psDecl->sTGSM.ui32Count;
        psVarType->Type = SVT_UINT;
        break;
    }
    case OPCODE_DCL_THREAD_GROUP_SHARED_MEMORY_STRUCTURED:
    {
        ShaderVarType* psVarType = &psShader->sGroupSharedVarType[psDecl->asOperands[0].ui32RegisterNumber];

        ASSERT(psDecl->asOperands[0].ui32RegisterNumber < MAX_GROUPSHARED);

        bcatcstr(glsl, "shared struct {\n");
        bformata(glsl, "uint value[%d];\n", psDecl->sTGSM.ui32Stride / 4);
        bcatcstr(glsl, "} ");
        TranslateOperand(psContext, &psDecl->asOperands[0], TO_FLAG_NONE);
        bformata(glsl, "[%d];\n",
            psDecl->sTGSM.ui32Count);

        memset(psVarType, 0, sizeof(ShaderVarType));
        strcpy(psVarType->Name, "$Element");

        psVarType->Columns = psDecl->sTGSM.ui32Stride / 4;
        psVarType->Elements = psDecl->sTGSM.ui32Count;
        psVarType->Type = SVT_UINT;
        break;
    }
    case OPCODE_DCL_STREAM:
    {
        ASSERT(psDecl->asOperands[0].eType == OPERAND_TYPE_STREAM);

        psShader->ui32CurrentVertexOutputStream = psDecl->asOperands[0].ui32RegisterNumber;

        bformata(glsl, "layout(stream = %d) out;\n", psShader->ui32CurrentVertexOutputStream);

        break;
    }
    case OPCODE_DCL_GS_INSTANCE_COUNT:
    {
        bformata(glsl, "layout(invocations = %d) in;\n", psDecl->value.ui32GSInstanceCount);
        break;
    }
    default:
    {
        ASSERT(0);
        break;
    }
    }
}

//Convert from per-phase temps to global temps for GLSL.
void ConsolidateHullTempVars(Shader* psShader)
{
    uint32_t i, k;
    const uint32_t ui32NumDeclLists = 3 + psShader->ui32ForkPhaseCount;
    Declaration* pasDeclArray[3 + MAX_FORK_PHASES];
    uint32_t aui32DeclCounts[3 + MAX_FORK_PHASES];
    uint32_t ui32NumTemps = 0;

    i = 0;

    pasDeclArray[i] = psShader->psHSDecl;
    aui32DeclCounts[i++] = psShader->ui32HSDeclCount;

    pasDeclArray[i] = psShader->psHSControlPointPhaseDecl;
    aui32DeclCounts[i++] = psShader->ui32HSControlPointDeclCount;
    for (k = 0; k < psShader->ui32ForkPhaseCount; ++k)
    {
        pasDeclArray[i] = psShader->apsHSForkPhaseDecl[k];
        aui32DeclCounts[i++] = psShader->aui32HSForkDeclCount[k];
    }
    pasDeclArray[i] = psShader->psHSJoinPhaseDecl;
    aui32DeclCounts[i++] = psShader->ui32HSJoinDeclCount;

    for (k = 0; k < ui32NumDeclLists; ++k)
    {
        for (i = 0; i < aui32DeclCounts[k]; ++i)
        {
            Declaration* psDecl = pasDeclArray[k] + i;

            if (psDecl->eOpcode == OPCODE_DCL_TEMPS)
            {
                if (ui32NumTemps < psDecl->value.ui32NumTemps)
                {
                    //Find the total max number of temps needed by the entire
                    //shader.
                    ui32NumTemps = psDecl->value.ui32NumTemps;
                }
                //Only want one global temp declaration.
                psDecl->value.ui32NumTemps = 0;
            }
        }
    }

    //Find the first temp declaration and make it
    //declare the max needed amount of temps.
    for (k = 0; k < ui32NumDeclLists; ++k)
    {
        for (i = 0; i < aui32DeclCounts[k]; ++i)
        {
            Declaration* psDecl = pasDeclArray[k] + i;

            if (psDecl->eOpcode == OPCODE_DCL_TEMPS)
            {
                psDecl->value.ui32NumTemps = ui32NumTemps;
                return;
            }
        }
    }
}

const char* GetMangleSuffix(const SHADER_TYPE eShaderType)
{
    switch (eShaderType)
    {
    case VERTEX_SHADER:
        return "VS";
    case PIXEL_SHADER:
        return "PS";
    case GEOMETRY_SHADER:
        return "GS";
    case HULL_SHADER:
        return "HS";
    case DOMAIN_SHADER:
        return "DS";
    case COMPUTE_SHADER:
        return "CS";
    }
    ASSERT(0);
    return "";
}

