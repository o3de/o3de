// Modifications copyright Amazon.com, Inc. or its affiliates
// Modifications copyright Crytek GmbH

#include "hlslcc.h"
#include "internal_includes/toGLSLDeclaration.h"
#include "internal_includes/toGLSLOperand.h"
#include "internal_includes/languages.h"
#include "bstrlib.h"
#include "internal_includes/debug.h"
#include "internal_includes/hlslcc_malloc.h"
#include <math.h>
#include <float.h>

#ifdef _MSC_VER
    #ifndef isnan
    #define isnan(x) _isnan(x)
    #endif

    #ifndef isinf
    #define isinf(x) (!_finite(x))
    #endif
#endif

#define fpcheck(x) (isnan(x) || isinf(x))

typedef enum {
    GLVARTYPE_FLOAT,
    GLVARTYPE_INT,
    GLVARTYPE_FLOAT4,
} GLVARTYPE;

extern void AddIndentation(HLSLCrossCompilerContext* psContext);

const char* GetTypeString(GLVARTYPE eType)
{
    switch(eType)
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
    switch(eType)
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

void AddToDx9ImmConstIndexableArray(HLSLCrossCompilerContext* psContext, const Operand* psOperand)
{
    bstring* savedStringPtr = psContext->currentShaderString;

    psContext->currentShaderString = &psContext->earlyMain;
    psContext->indent++;
    AddIndentation(psContext);
    psContext->psShader->aui32Dx9ImmConstArrayRemap[psOperand->ui32RegisterNumber] = psContext->psShader->ui32NumDx9ImmConst;
    bformata(psContext->earlyMain, "ImmConstArray[%d] = ", psContext->psShader->ui32NumDx9ImmConst);
    TranslateOperand(psContext, psOperand, TO_FLAG_NONE);
    bcatcstr(psContext->earlyMain, ";\n");
    psContext->indent--;
    psContext->psShader->ui32NumDx9ImmConst++;

    psContext->currentShaderString = savedStringPtr;
}

void DeclareConstBufferShaderVariable(bstring glsl, const char* Name, const struct ShaderVarType_TAG* psType, int unsizedArray)
    //const SHADER_VARIABLE_CLASS eClass, const SHADER_VARIABLE_TYPE eType,
    //const char* pszName)
{
    if(psType->Class == SVC_STRUCT)
    {
        bformata(glsl, "\t%s_Type %s", Name, Name);
    }
    else if(psType->Class == SVC_MATRIX_COLUMNS || psType->Class == SVC_MATRIX_ROWS)
    {
        switch(psType->Type)
        {
            case SVT_FLOAT:
            {
                bformata(glsl, "\tmat4 %s", Name);
                break;
            }
            default:
            {
                ASSERT(0);
                break;
            }
        }
        if(psType->Elements > 1)
        {
            bformata(glsl, "[%d]", psType->Elements);
        }
    }
    else
    if(psType->Class == SVC_VECTOR)
    {
        switch(psType->Type)
        {
            case SVT_FLOAT:
            {
                bformata(glsl, "\tvec%d %s", psType->Columns, Name);
                break;
            }
            case SVT_UINT:
            {
                bformata(glsl, "\tuvec%d %s", psType->Columns, Name);
                break;
            }
            case SVT_INT:
            {
                bformata(glsl, "\tivec%d %s", psType->Columns, Name);
                break;
            }
            case SVT_DOUBLE:
            {
                bformata(glsl, "\tdvec%d %s", psType->Columns, Name);
                break;
            }
            case SVT_BOOL:
            {
                bformata(glsl, "\tbvec%d %s", psType->Columns, Name);
                break;
            }
            default:
            {
                ASSERT(0);
                break;
            }
        }

        if(psType->Elements > 1)
        {
            bformata(glsl, "[%d]", psType->Elements);
        }
    }
    else
    if(psType->Class == SVC_SCALAR)
    {
        switch(psType->Type)
        {
            case SVT_FLOAT:
            {
                bformata(glsl, "\tfloat %s", Name);
                break;
            }
            case SVT_UINT:
            {
                bformata(glsl, "\tuint %s", Name);
                break;
            }
            case SVT_INT:
            {
                bformata(glsl, "\tint %s", Name);
                break;
            }
            case SVT_DOUBLE:
            {
                bformata(glsl, "\tdouble %s", Name);
                break;
            }
            case SVT_BOOL:
            {
                //Use int instead of bool.
                //Allows implicit conversions to integer and
                //bool consumes 4-bytes in HLSL and GLSL anyway.
                bformata(glsl, "\tint %s", Name);
                // Also change the definition in the type tree.
                ((ShaderVarType *)psType)->Type = SVT_INT;
                break;
            }
            default:
            {
                ASSERT(0);
                break;
            }
        }

        if(psType->Elements > 1)
        {
            bformata(glsl, "[%d]", psType->Elements);
        }
    }
    if(unsizedArray)
        bformata(glsl, "[]");
    bformata(glsl, ";\n");
}

//In GLSL embedded structure definitions are not supported.
void PreDeclareStructType(bstring glsl, const char* Name, const struct ShaderVarType_TAG* psType)
{
    uint32_t i;

    for(i=0; i<psType->MemberCount; ++i)
    {
        if(psType->Members[i].Class == SVC_STRUCT)
        {
            PreDeclareStructType(glsl, psType->Members[i].Name, &psType->Members[i]);
        }
    }

    if(psType->Class == SVC_STRUCT)
    {
        
        uint32_t unnamed_struct = strcmp(Name, "$Element") == 0 ? 1 : 0;

        //Not supported at the moment
        ASSERT(!unnamed_struct);

        bformata(glsl, "struct %s_Type {\n", Name);

        for(i=0; i<psType->MemberCount; ++i)
        {
            ASSERT(psType->Members != 0);

            DeclareConstBufferShaderVariable(glsl, psType->Members[i].Name, &psType->Members[i], 0);
        }

        bformata(glsl, "};\n");
    }
}

const char* GetDeclaredInputName(const HLSLCrossCompilerContext* psContext, const SHADER_TYPE eShaderType, const Operand* psOperand)
{
    bstring inputName;
    char* cstr;
    InOutSignature* psIn;
    int found = GetInputSignatureFromRegister(psOperand->ui32RegisterNumber, &psContext->psShader->sInfo, &psIn);

    if((psContext->flags & HLSLCC_FLAG_INOUT_SEMANTIC_NAMES) && found)
    {
        if (eShaderType == VERTEX_SHADER) /* We cannot have input and output names conflict, but vs output must match ps input. Prefix vs input. */
            inputName = bformat("in_%s%d", psIn->SemanticName, psIn->ui32SemanticIndex);
        else
        inputName = bformat("%s%d", psIn->SemanticName, psIn->ui32SemanticIndex);
    }
    else if(eShaderType == GEOMETRY_SHADER)
    {
        inputName = bformat("VtxOutput%d", psOperand->ui32RegisterNumber);
    }
    else if(eShaderType == HULL_SHADER)
    {
        inputName = bformat("VtxGeoOutput%d", psOperand->ui32RegisterNumber);
    }
    else if(eShaderType == DOMAIN_SHADER)
    {
        inputName = bformat("HullOutput%d", psOperand->ui32RegisterNumber);
    }
    else if(eShaderType == PIXEL_SHADER)
    {
        if(psContext->flags & HLSLCC_FLAG_TESS_ENABLED)
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
    if((psContext->flags & HLSLCC_FLAG_INOUT_APPEND_SEMANTIC_NAMES) && found)
    {
        bformata(inputName,"_%s%d", psIn->SemanticName, psIn->ui32SemanticIndex);
    }

    cstr = bstr2cstr(inputName, '\0');
    bdestroy(inputName);
    return cstr;
}

const char* GetDeclaredOutputName(const HLSLCrossCompilerContext* psContext,
                                  const SHADER_TYPE eShaderType,
                                  const Operand* psOperand,
                                  int* piStream)
{
    bstring outputName;
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

    if(psContext->flags & HLSLCC_FLAG_INOUT_SEMANTIC_NAMES)
    {
        outputName = bformat("%s%d", psOut->SemanticName, psOut->ui32SemanticIndex);
    }
    else if(eShaderType == GEOMETRY_SHADER)
    {
        if(psOut->ui32Stream != 0)
        {
            outputName = bformat("VtxGeoOutput%d_S%d", psOperand->ui32RegisterNumber, psOut->ui32Stream);
            piStream[0] = psOut->ui32Stream;
        }
        else
        {
            outputName = bformat("VtxGeoOutput%d", psOperand->ui32RegisterNumber);
        }

    }
    else if(eShaderType == DOMAIN_SHADER)
    {
        outputName = bformat("DomOutput%d", psOperand->ui32RegisterNumber);
    }
    else if(eShaderType == VERTEX_SHADER)
    {
        if(psContext->flags & HLSLCC_FLAG_GS_ENABLED)
        {
            outputName = bformat("VtxOutput%d", psOperand->ui32RegisterNumber);
        }
        else
        {
            outputName = bformat("VtxGeoOutput%d", psOperand->ui32RegisterNumber);
        }
    }
    else if(eShaderType == PIXEL_SHADER)
    {
        outputName = bformat("PixOutput%d", psOperand->ui32RegisterNumber);
    }
    else
    {
        ASSERT(eShaderType == HULL_SHADER);
        outputName = bformat("HullOutput%d", psOperand->ui32RegisterNumber);
    }
    if(psContext->flags & HLSLCC_FLAG_INOUT_APPEND_SEMANTIC_NAMES)
    {
        bformata(outputName, "_%s%d", psOut->SemanticName, psOut->ui32SemanticIndex);
    }

    cstr = bstr2cstr(outputName, '\0');
    bdestroy(outputName);
    return cstr;
}

const char* GetInterpolationString(INTERPOLATION_MODE eMode)
{
    switch(eMode)
    {
        case INTERPOLATION_CONSTANT:
        {
            return "flat";
        }
        case INTERPOLATION_LINEAR:
        {
            return "";
        }
        case INTERPOLATION_LINEAR_CENTROID:
        {
            return "centroid";
        }
        case INTERPOLATION_LINEAR_NOPERSPECTIVE:
        {
            return "noperspective";
            break;
        }
        case INTERPOLATION_LINEAR_NOPERSPECTIVE_CENTROID:
        {
            return "noperspective centroid";
        }
        case INTERPOLATION_LINEAR_SAMPLE:
        {
            return "sample";
        }
        case INTERPOLATION_LINEAR_NOPERSPECTIVE_SAMPLE:
        {
            return "noperspective sample";
        }
        default:
        {
            return "";
        }
    }
}

static void DeclareInput(
    HLSLCrossCompilerContext* psContext,
    const Declaration* psDecl,
    const char* Interpolation, const char* StorageQualifier, const char* Precision, int iNumComponents, OPERAND_INDEX_DIMENSION eIndexDim, const char* InputName)
{
    ShaderData* psShader = psContext->psShader;
    bstring glsl = *psContext->currentShaderString;

    // This falls within the specified index ranges. The default is 0 if no input range is specified
    if(psShader->aIndexedInput[psDecl->asOperands[0].ui32RegisterNumber] == -1)
        return;

    if(psShader->aiInputDeclaredSize[psDecl->asOperands[0].ui32RegisterNumber] == 0)
    {
        const char* vecType = "vec";
        const char* scalarType = "float";
        InOutSignature* psSignature = NULL;

        if( GetInputSignatureFromRegister(psDecl->asOperands[0].ui32RegisterNumber, &psShader->sInfo, &psSignature) )
        {
            switch(psSignature->eComponentType)
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
              
        if (HaveInOutLocationQualifier(psContext->psShader->eTargetLanguage, psContext->psShader->extensions, psContext->flags) ||
            (psShader->eShaderType == VERTEX_SHADER && HaveLimitedInOutLocationQualifier(psContext->psShader->eTargetLanguage, psContext->flags)))
        {
            // Skip location if requested by the flags.
            if (!(psContext->flags & HLSLCC_FLAG_DISABLE_EXPLICIT_LOCATIONS))
            bformata(glsl, "layout(location = %d) ", psDecl->asOperands[0].ui32RegisterNumber);
        }

        switch(eIndexDim)
        {
            case INDEX_2D:
            {
                if(iNumComponents == 1)
                {
                    const uint32_t regNum =  psDecl->asOperands[0].ui32RegisterNumber;
                    const uint32_t arraySize = psDecl->asOperands[0].aui32ArraySizes[0];

                    psContext->psShader->abScalarInput[psDecl->asOperands[0].ui32RegisterNumber] = -1;

                    bformata(glsl, "%s %s %s %s [%d];\n", StorageQualifier, Precision, scalarType, InputName,
                        arraySize);

                    bformata(glsl, "%s1 Input%d;\n", vecType, psDecl->asOperands[0].ui32RegisterNumber);

                    psShader->aiInputDeclaredSize[psDecl->asOperands[0].ui32RegisterNumber] = arraySize;
                }
                else
                {
                    bformata(glsl, "%s %s %s%d %s [%d];\n", StorageQualifier, Precision, vecType, iNumComponents, InputName,
                        psDecl->asOperands[0].aui32ArraySizes[0]);

                    bformata(glsl, "%s%d Input%d[%d];\n", vecType, iNumComponents, psDecl->asOperands[0].ui32RegisterNumber,
                        psDecl->asOperands[0].aui32ArraySizes[0]);

                    psShader->aiInputDeclaredSize[psDecl->asOperands[0].ui32RegisterNumber] = psDecl->asOperands[0].aui32ArraySizes[0];
                }
                break;
            }
            default:
            {

                if(psDecl->asOperands[0].eType == OPERAND_TYPE_SPECIAL_TEXCOORD)
                {
                    InputName = "TexCoord";
                }
                
                if(iNumComponents == 1)
                {
                    psContext->psShader->abScalarInput[psDecl->asOperands[0].ui32RegisterNumber] = 1;

                    bformata(glsl, "%s %s %s %s %s;\n", Interpolation, StorageQualifier, Precision, scalarType, InputName);
                    bformata(glsl, "%s1 Input%d;\n", vecType, psDecl->asOperands[0].ui32RegisterNumber);

                    psShader->aiInputDeclaredSize[psDecl->asOperands[0].ui32RegisterNumber] = -1;
                }
                else
                {
                    if(psShader->aIndexedInput[psDecl->asOperands[0].ui32RegisterNumber] > 0)
                    {
                        bformata(glsl, "%s %s %s %s%d %s", Interpolation, StorageQualifier, Precision, vecType, iNumComponents, InputName);
                        bformata(glsl, "[%d];\n", psShader->aIndexedInput[psDecl->asOperands[0].ui32RegisterNumber]);

                        bformata(glsl, "%s%d Input%d[%d];\n", vecType, iNumComponents, psDecl->asOperands[0].ui32RegisterNumber,
                            psShader->aIndexedInput[psDecl->asOperands[0].ui32RegisterNumber]);


                        psShader->aiInputDeclaredSize[psDecl->asOperands[0].ui32RegisterNumber] = psShader->aIndexedInput[psDecl->asOperands[0].ui32RegisterNumber];
                    }
                    else
                    {
                        bformata(glsl, "%s %s %s %s%d %s;\n", Interpolation, StorageQualifier, Precision, vecType, iNumComponents, InputName);
                        bformata(glsl, "%s%d Input%d;\n", vecType, iNumComponents, psDecl->asOperands[0].ui32RegisterNumber);

                        psShader->aiInputDeclaredSize[psDecl->asOperands[0].ui32RegisterNumber] = -1;
                    }
                }
                break;
            }
        }
    }

    if(psShader->abInputReferencedByInstruction[psDecl->asOperands[0].ui32RegisterNumber])
    {
        psContext->currentShaderString = &psContext->earlyMain;
        psContext->indent++;

        if(psShader->aiInputDeclaredSize[psDecl->asOperands[0].ui32RegisterNumber] == -1) //Not an array
        {
            AddIndentation(psContext);
            bformata(psContext->earlyMain, "Input%d = %s;\n", psDecl->asOperands[0].ui32RegisterNumber, InputName);
        }
        else
        {
            int arrayIndex = psShader->aiInputDeclaredSize[psDecl->asOperands[0].ui32RegisterNumber];

            while(arrayIndex)
            {
                AddIndentation(psContext);
                bformata(psContext->earlyMain, "Input%d[%d] = %s[%d];\n", psDecl->asOperands[0].ui32RegisterNumber, arrayIndex-1,
                    InputName, arrayIndex-1);

                arrayIndex--;
            }
        }
        psContext->indent--;
        psContext->currentShaderString = &psContext->mainShader;
    }
}

void AddBuiltinInput(HLSLCrossCompilerContext* psContext, const Declaration* psDecl, const char* builtinName)
{
    bstring glsl = *psContext->currentShaderString;
    ShaderData* psShader = psContext->psShader;

    if(psShader->aiInputDeclaredSize[psDecl->asOperands[0].ui32RegisterNumber] == 0)
    {
        SHADER_VARIABLE_TYPE eType = GetOperandDataType(psContext, &psDecl->asOperands[0]);
        switch(eType)
        {
            case SVT_INT:
                bformata(glsl, "ivec4 ");
                break;
            case SVT_UINT:
                bformata(glsl, "uvec4 ");
                break;
            case SVT_BOOL:
                bformata(glsl, "bvec4 ");
                break;
            default:
                bformata(glsl, "vec4 ");
                break;
        }
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

    psContext->currentShaderString = &psContext->earlyMain;
    psContext->indent++;
    AddIndentation(psContext);
    TranslateOperand(psContext, &psDecl->asOperands[0], TO_FLAG_DESTINATION);

    bformata(psContext->earlyMain, " = %s", builtinName);

    switch(psDecl->asOperands[0].eSpecialName)
    {
        case NAME_POSITION:
            TranslateOperandSwizzle(psContext, &psDecl->asOperands[0]);
            break;
        default:
            //Scalar built-in. Don't apply swizzle.
            break;
    }
    bcatcstr(psContext->earlyMain, ";\n");

    psContext->indent--;
    psContext->currentShaderString = &psContext->mainShader;
}

int OutputNeedsDeclaring(HLSLCrossCompilerContext* psContext, const Operand* psOperand, const int count)
{
    ShaderData* psShader = psContext->psShader;
    const uint32_t declared = ((psContext->currentPhase + 1) << 3) | psShader->ui32CurrentVertexOutputStream;
    if(psShader->aiOutputDeclared[psOperand->ui32RegisterNumber] != declared)
    {
        int offset;

        for(offset = 0; offset < count; offset++)
        {
            psShader->aiOutputDeclared[psOperand->ui32RegisterNumber+offset] = declared;
        }
        return 1;
    }

    if(psShader->eShaderType == PIXEL_SHADER)
    {
        if(psOperand->eType == OPERAND_TYPE_OUTPUT_DEPTH_GREATER_EQUAL ||
           psOperand->eType == OPERAND_TYPE_OUTPUT_DEPTH_LESS_EQUAL)
        {
            return 1;
        }
    }

    return 0;
}

void AddBuiltinOutput(HLSLCrossCompilerContext* psContext, const Declaration* psDecl, const GLVARTYPE type, int arrayElements, const char* builtinName)
{
    bstring glsl = *psContext->currentShaderString;
    ShaderData* psShader = psContext->psShader;

    psContext->havePostShaderCode[psContext->currentPhase] = 1;

    if(OutputNeedsDeclaring(psContext, &psDecl->asOperands[0], arrayElements ? arrayElements : 1))
    {
        InOutSignature* psSignature = NULL;

        GetOutputSignatureFromRegister(
            psContext->currentPhase,
            psDecl->asOperands[0].ui32RegisterNumber,
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

        switch (type)
        {
        case GLVARTYPE_INT:
            bcatcstr(glsl, "ivec4 ");
            break;
        default:
            bcatcstr(glsl, "vec4 ");
        }
        
        bformata(glsl, "phase%d_", psContext->currentPhase);
        TranslateOperand(psContext, &psDecl->asOperands[0], TO_FLAG_NAME_ONLY);
        if(arrayElements)
            bformata(glsl, "[%d];\n", arrayElements);
        else
            bcatcstr(glsl, ";\n");

        psContext->currentShaderString = &psContext->postShaderCode[psContext->currentPhase];
        glsl = *psContext->currentShaderString;
        psContext->indent++;
        if(arrayElements)
        {
            int elem;
            for(elem = 0; elem < arrayElements; elem++)
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

            if(psDecl->asOperands[0].eSpecialName == NAME_CLIP_DISTANCE)
            {
                int max = GetMaxComponentFromComponentMask(&psDecl->asOperands[0]);

                int applySiwzzle = GetNumSwizzleElements(&psDecl->asOperands[0]) > 1 ? 1 : 0;
                int index;
                int i;
                int multiplier = 1;
                char* swizzle[] = {".x", ".y", ".z", ".w"};

                ASSERT(psSignature!=NULL);

                index = psSignature->ui32SemanticIndex;

                //Clip distance can be spread across 1 or 2 outputs (each no more than a vec4).
                //Some examples:
                //float4 clip[2] : SV_ClipDistance; //8 clip distances
                //float3 clip[2] : SV_ClipDistance; //6 clip distances
                //float4 clip : SV_ClipDistance; //4 clip distances
                //float clip : SV_ClipDistance; //1 clip distance.

                //In GLSL the clip distance built-in is an array of up to 8 floats.
                //So vector to array conversion needs to be done here.
                if(index == 1)
                {
                    InOutSignature* psFirstClipSignature;
                    if(GetOutputSignatureFromSystemValue(NAME_CLIP_DISTANCE, 1, &psShader->sInfo, &psFirstClipSignature))
                    {
                        if(psFirstClipSignature->ui32Mask & (1 << 3))
                        {
                            multiplier = 4;
                        }
                        else
                        if(psFirstClipSignature->ui32Mask & (1 << 2))
                        {
                            multiplier = 3;
                        }
                        else
                        if(psFirstClipSignature->ui32Mask & (1 << 1))
                        {
                            multiplier = 2;
                        }
                    }
                }

                for(i=0; i<max; ++i)
                {
                    AddIndentation(psContext);
                    bformata(glsl, "%s[%d] = (phase%d_", builtinName, i + multiplier*index, psContext->currentPhase);
                    TranslateOperand(psContext, &psDecl->asOperands[0], TO_FLAG_NONE);
                    if(applySiwzzle)
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

                if(elements != GetTypeElementCount(type))
                {
                    //This is to handle float3 position seen in control point phases
                    //struct HS_OUTPUT
                    //{
                    //    float3 vPosition : POSITION;
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
                    TranslateOperand(psContext, &psDecl->asOperands[0], type == GLVARTYPE_INT ? TO_FLAG_INTEGER : TO_FLAG_NONE);
                    bformata(glsl, ");\n");
                }
            }
        }
        psContext->indent--;
        psContext->currentShaderString = &psContext->mainShader;
    }
}

void AddUserOutput(HLSLCrossCompilerContext* psContext, const Declaration* psDecl)
{
    bstring glsl = *psContext->currentShaderString;
    ShaderData* psShader = psContext->psShader;

    if(OutputNeedsDeclaring(psContext, &psDecl->asOperands[0], 1))
    {
        const Operand* psOperand = &psDecl->asOperands[0];
        const char* Precision = "";
        const char* type = "vec";

        InOutSignature* psSignature = NULL;

        GetOutputSignatureFromRegister(
            psContext->currentPhase,
            psDecl->asOperands[0].ui32RegisterNumber,
            psDecl->asOperands[0].ui32CompMask,
            psShader->ui32CurrentVertexOutputStream,
            &psShader->sInfo,
            &psSignature);

        switch(psSignature->eComponentType)
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

        if(HavePrecisionQualifers(psShader->eTargetLanguage))
        {
            switch(psOperand->eMinPrecision)
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

        switch(psShader->eShaderType)
        {
            case PIXEL_SHADER:
            {
                switch(psDecl->asOperands[0].eType)
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
                        if(WriteToFragData(psContext->psShader->eTargetLanguage))
                        {
                            bformata(glsl, "#define Output%d gl_FragData[%d]\n", psDecl->asOperands[0].ui32RegisterNumber, psDecl->asOperands[0].ui32RegisterNumber);
                        }
                        else
                        {
                            int stream = 0;
                            const char* OutputName = GetDeclaredOutputName(psContext, PIXEL_SHADER, psOperand, &stream);

                            if (HaveInOutLocationQualifier(psContext->psShader->eTargetLanguage, psContext->psShader->extensions, psContext->flags) || HaveLimitedInOutLocationQualifier(psContext->psShader->eTargetLanguage, psContext->flags))
                            {
                                uint32_t index = 0;
                                uint32_t renderTarget = psDecl->asOperands[0].ui32RegisterNumber;

                                if((psContext->flags & HLSLCC_FLAG_DUAL_SOURCE_BLENDING) && DualSourceBlendSupported(psContext->psShader->eTargetLanguage))
                                {
                                    if(renderTarget > 0)
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
                            if(stream)
                            {
                                bformata(glsl, "#define Output%d_S%d %s\n", psDecl->asOperands[0].ui32RegisterNumber, stream, OutputName);
                            }
                            else
                            {
                                bformata(glsl, "#define Output%d %s\n", psDecl->asOperands[0].ui32RegisterNumber, OutputName);
                            }
                        }
                        break;
                    }
                }
                break;
            }
            case VERTEX_SHADER:
            {
                int iNumComponents = 4;//GetMaxComponentFromComponentMask(&psDecl->asOperands[0]);
                const char* Interpolation = "";
                int stream = 0;
                const char* OutputName = GetDeclaredOutputName(psContext, VERTEX_SHADER, psOperand, &stream);

                if (HaveInOutLocationQualifier(psContext->psShader->eTargetLanguage, psContext->psShader->extensions, psContext->flags))
                {
                    if (!(psContext->flags & HLSLCC_FLAG_DISABLE_EXPLICIT_LOCATIONS))
                    bformata(glsl, "layout(location = %d) ", psDecl->asOperands[0].ui32RegisterNumber);
                }

                if(InOutSupported(psContext->psShader->eTargetLanguage))
                {
                    bformata(glsl, "%s out %s %s%d %s;\n", Interpolation, Precision, type, iNumComponents, OutputName);
                }
                else
                {
                    bformata(glsl, "%s varying %s %s%d %s;\n", Interpolation, Precision, type, iNumComponents, OutputName);
                }
                bformata(glsl, "#define Output%d %s\n", psDecl->asOperands[0].ui32RegisterNumber, OutputName);

                break;
            }
            case GEOMETRY_SHADER:
            {
                int stream = 0;
                const char* OutputName = GetDeclaredOutputName(psContext, GEOMETRY_SHADER, psOperand, &stream);

                if (HaveInOutLocationQualifier(psContext->psShader->eTargetLanguage, psContext->psShader->extensions, psContext->flags))
                {
                    bformata(glsl, "layout(location = %d) ", psDecl->asOperands[0].ui32RegisterNumber);
                }

                bformata(glsl, "out %s4 %s;\n", type, OutputName);
                if(stream)
                {
                    bformata(glsl, "#define Output%d_S%d %s\n", psDecl->asOperands[0].ui32RegisterNumber, stream, OutputName);
                }
                else
                {
                    bformata(glsl, "#define Output%d %s\n", psDecl->asOperands[0].ui32RegisterNumber, OutputName);
                }
                break;
            }
            case HULL_SHADER:
            {
                int stream = 0;
                const char* OutputName = GetDeclaredOutputName(psContext, HULL_SHADER, psOperand, &stream);

                ASSERT(psDecl->asOperands[0].ui32RegisterNumber!=0);//Reg 0 should be gl_out[gl_InvocationID].gl_Position.

                if(psContext->currentPhase == HS_JOIN_PHASE)
                {
                    bformata(glsl, "out patch %s4 %s[];\n", type, OutputName);
                }
                else
                {
                    if (HaveInOutLocationQualifier(psContext->psShader->eTargetLanguage, psContext->psShader->extensions, psContext->flags))
                    {
                        bformata(glsl, "layout(location = %d) ", psDecl->asOperands[0].ui32RegisterNumber);
                    }

                    bformata(glsl, "out %s4 %s[];\n", type, OutputName);
                }
                bformata(glsl, "#define Output%d %s[gl_InvocationID]\n", psDecl->asOperands[0].ui32RegisterNumber, OutputName);
                break;
            }
            case DOMAIN_SHADER:
            {
                int stream = 0;
                const char* OutputName = GetDeclaredOutputName(psContext, DOMAIN_SHADER, psOperand, &stream);
                if (HaveInOutLocationQualifier(psContext->psShader->eTargetLanguage, psContext->psShader->extensions, psContext->flags))
                {
                    bformata(glsl, "layout(location = %d) ", psDecl->asOperands[0].ui32RegisterNumber);
                }
                bformata(glsl, "out %s4 %s;\n", type, OutputName);
                bformata(glsl, "#define Output%d %s\n", psDecl->asOperands[0].ui32RegisterNumber, OutputName);
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

            This unpacking of outputs is only done when using HLSLCC_FLAG_INOUT_SEMANTIC_NAMES/HLSLCC_FLAG_INOUT_APPEND_SEMANTIC_NAMES.
            When not set the application will be using HLSL reflection information to discover
            what the input and outputs mean if need be.
        */

        //

        if((psContext->flags & (HLSLCC_FLAG_INOUT_SEMANTIC_NAMES|HLSLCC_FLAG_INOUT_APPEND_SEMANTIC_NAMES)) && (psDecl->asOperands[0].eType == OPERAND_TYPE_OUTPUT))
        {
            const Operand* psOperand = &psDecl->asOperands[0];
            InOutSignature* psSignature = NULL;
            const char* type = "vec";
            int stream = 0;
            const char* OutputName = GetDeclaredOutputName(psContext, psShader->eShaderType, psOperand, &stream);

            GetOutputSignatureFromRegister(
                psContext->currentPhase,
                psOperand->ui32RegisterNumber,
                psOperand->ui32CompMask,
                0,
                &psShader->sInfo,
                &psSignature);

            if (HaveInOutLocationQualifier(psContext->psShader->eTargetLanguage, psContext->psShader->extensions, psContext->flags))
            {
                if (!((psShader->eShaderType == VERTEX_SHADER) && (psContext->flags & HLSLCC_FLAG_DISABLE_EXPLICIT_LOCATIONS)))
                bformata(glsl, "layout(location = %d) ", psDecl->asOperands[0].ui32RegisterNumber);
            }

            switch(psSignature->eComponentType)
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

            psContext->currentShaderString = &psContext->postShaderCode[psContext->currentPhase];
            glsl = *psContext->currentShaderString;

            bcatcstr(glsl, OutputName);
            AddSwizzleUsingElementCount(psContext, GetNumSwizzleElements(psOperand));
            bformata(glsl, " = Output%d", psOperand->ui32RegisterNumber);
            TranslateOperandSwizzle(psContext, psOperand);
            bcatcstr(glsl, ";\n");

            psContext->currentShaderString = &psContext->mainShader;
            glsl = *psContext->currentShaderString;
        }
    }
}

void DeclareUBOConstants(HLSLCrossCompilerContext* psContext, const uint32_t ui32BindingPoint,
                            ConstantBuffer* psCBuf,
                            bstring glsl)
{
    uint32_t i;
    const char* Name = psCBuf->Name;
    if(psCBuf->Name[0] == '$') //For $Globals
    {
        Name++;
    }

    for(i=0; i < psCBuf->ui32NumVars; ++i)
    {
        PreDeclareStructType(glsl,
            psCBuf->asVars[i].Name,
            &psCBuf->asVars[i].sType);
    }

    /* [layout (location = X)] uniform vec4 HLSLConstantBufferName[numConsts]; */
    if (HaveUniformBindingsAndLocations(psContext->psShader->eTargetLanguage, psContext->psShader->extensions, psContext->flags))
        bformata(glsl, "layout(binding = %d) ", ui32BindingPoint);

    bformata(glsl, "uniform %s {\n ", Name);

    for(i=0; i < psCBuf->ui32NumVars; ++i)
    {
        DeclareConstBufferShaderVariable(glsl,
            psCBuf->asVars[i].Name,
            &psCBuf->asVars[i].sType, 0);
    }
                
    bcatcstr(glsl, "};\n");
}

void DeclareBufferVariable(HLSLCrossCompilerContext* psContext, const uint32_t ui32BindingPoint,
                            ConstantBuffer* psCBuf, const Operand* psOperand,
                            const uint32_t ui32GloballyCoherentAccess,
                            const ResourceType eResourceType,
                            bstring glsl)
{
    bstring StructName;
    uint32_t unnamed_struct = strcmp(psCBuf->asVars[0].Name, "$Element") == 0 ? 1 : 0;

    ASSERT(psCBuf->ui32NumVars == 1);
    ASSERT(unnamed_struct);

    StructName = bfromcstr("");
    
    //TranslateOperand(psContext, psOperand, TO_FLAG_NAME_ONLY);
    if(psOperand->eType == OPERAND_TYPE_RESOURCE && eResourceType == RTYPE_STRUCTURED)
    {
        bformata(StructName, "StructuredRes%d", psOperand->ui32RegisterNumber);
    }
    else if(psOperand->eType == OPERAND_TYPE_RESOURCE && eResourceType == RTYPE_UAV_RWBYTEADDRESS)
    {
        bformata(StructName, "RawRes%d", psOperand->ui32RegisterNumber);
    }
    else
    {
        ResourceName(StructName, psContext, RGROUP_UAV, psOperand->ui32RegisterNumber, 0);
    }

    PreDeclareStructType(glsl,
        bstr2cstr(StructName, '\0'),
        &psCBuf->asVars[0].sType);

    /* [layout (location = X)] uniform vec4 HLSLConstantBufferName[numConsts]; */
    if (HaveUniformBindingsAndLocations(psContext->psShader->eTargetLanguage, psContext->psShader->extensions, psContext->flags))
        bformata(glsl, "layout(binding = %d) ", ui32BindingPoint);

    if(ui32GloballyCoherentAccess & GLOBALLY_COHERENT_ACCESS)
    {
        bcatcstr(glsl, "coherent ");
    }

    if(eResourceType == RTYPE_STRUCTURED)
    {
        bcatcstr(glsl, "readonly ");
    }

    bformata(glsl, "buffer Block%d {\n", psOperand->ui32RegisterNumber);

    DeclareConstBufferShaderVariable(glsl,
    bstr2cstr(StructName, '\0'),
    &psCBuf->asVars[0].sType,
    1);

    bcatcstr(glsl, "};\n");

    bdestroy(StructName);
}


void DeclareStructConstants(HLSLCrossCompilerContext* psContext, const uint32_t ui32BindingPoint,
                            ConstantBuffer* psCBuf, const Operand* psOperand,
                            bstring glsl)
{
    uint32_t i;
    int useGlobalsStruct = 1;

    if(psContext->flags & HLSLCC_FLAG_DISABLE_GLOBALS_STRUCT && psCBuf->Name[0] == '$')
        useGlobalsStruct = 0;

    if(useGlobalsStruct)
    {
    for(i=0; i < psCBuf->ui32NumVars; ++i)
    {
        PreDeclareStructType(glsl,
            psCBuf->asVars[i].Name,
            &psCBuf->asVars[i].sType);
    }
    }

    /* [layout (location = X)] uniform vec4 HLSLConstantBufferName[numConsts]; */
    if (HaveUniformBindingsAndLocations(psContext->psShader->eTargetLanguage, psContext->psShader->extensions, psContext->flags))
        bformata(glsl, "layout(location = %d) ", ui32BindingPoint);
    if(useGlobalsStruct)
    {
    bcatcstr(glsl, "uniform struct ");
    TranslateOperand(psContext, psOperand, TO_FLAG_DECLARATION_NAME);

    bcatcstr(glsl, "_Type {\n");
    }

    for(i=0; i < psCBuf->ui32NumVars; ++i)
    {
        if(!useGlobalsStruct)
            bcatcstr(glsl, "uniform ");

        DeclareConstBufferShaderVariable(glsl,
            psCBuf->asVars[i].Name,
            &psCBuf->asVars[i].sType, 0);
    }
                
    if(useGlobalsStruct)
    {
    bcatcstr(glsl, "} ");

    TranslateOperand(psContext, psOperand, TO_FLAG_DECLARATION_NAME);

    bcatcstr(glsl, ";\n");
}
}

char* GetSamplerType(HLSLCrossCompilerContext* psContext,
                     const RESOURCE_DIMENSION eDimension,
                     const uint32_t ui32RegisterNumber)
{
    ResourceBinding* psBinding = 0;
    RESOURCE_RETURN_TYPE eType = RETURN_TYPE_UNORM;
    int found;
    found = GetResourceFromBindingPoint(RGROUP_TEXTURE, ui32RegisterNumber, &psContext->psShader->sInfo, &psBinding);
    if(found)
    {
        eType = (RESOURCE_RETURN_TYPE)psBinding->ui32ReturnType;
    }
    switch(eDimension)
    {
        case RESOURCE_DIMENSION_BUFFER:
        {
            switch(eType)
            {
                case RETURN_TYPE_SINT:
                    return "isamplerBuffer";
                case RETURN_TYPE_UINT:
                    return "usamplerBuffer";
                default:
                    return "samplerBuffer";
            }
            break;
        }

        case RESOURCE_DIMENSION_TEXTURE1D:
        {
            switch(eType)
            {
                case RETURN_TYPE_SINT:
                    return "isampler1D";
                case RETURN_TYPE_UINT:
                    return "usampler1D";
                default:
                    return "sampler1D";
            }
            break;
        }

        case RESOURCE_DIMENSION_TEXTURE2D:
        {
            switch(eType)
            {
                case RETURN_TYPE_SINT:
                    return "isampler2D";
                case RETURN_TYPE_UINT:
                    return "usampler2D";
                default:
                    return "sampler2D";
            }
            break;
        }

        case RESOURCE_DIMENSION_TEXTURE2DMS:
        {
            switch(eType)
            {
                case RETURN_TYPE_SINT:
                    return "isampler2DMS";
                case RETURN_TYPE_UINT:
                    return "usampler2DMS";
                default:
                    return "sampler2DMS";
            }
            break;
        }

        case RESOURCE_DIMENSION_TEXTURE3D:
        {
            switch(eType)
            {
                case RETURN_TYPE_SINT:
                    return "isampler3D";
                case RETURN_TYPE_UINT:
                    return "usampler3D";
                default:
                    return "sampler3D";
            }
            break;
        }

        case RESOURCE_DIMENSION_TEXTURECUBE:
        {
            switch(eType)
            {
                case RETURN_TYPE_SINT:
                    return "isamplerCube";
                case RETURN_TYPE_UINT:
                    return "usamplerCube";
                default:
                    return "samplerCube";
            }
            break;
        }

        case RESOURCE_DIMENSION_TEXTURE1DARRAY:
        {
            switch(eType)
            {
                case RETURN_TYPE_SINT:
                    return "isampler1DArray";
                case RETURN_TYPE_UINT:
                    return "usampler1DArray";
                default:
                    return "sampler1DArray";
            }
            break;
        }

        case RESOURCE_DIMENSION_TEXTURE2DARRAY:
        {
            switch(eType)
            {
                case RETURN_TYPE_SINT:
                    return "isampler2DArray";
                case RETURN_TYPE_UINT:
                    return "usampler2DArray";
                default:
                    return "sampler2DArray";
            }
            break;
        }

        case RESOURCE_DIMENSION_TEXTURE2DMSARRAY:
        {
            switch(eType)
            {
                case RETURN_TYPE_SINT:
                    return "isampler2DMSArray";
                case RETURN_TYPE_UINT:
                    return "usampler2DMSArray";
                default:
                    return "sampler2DMSArray";
            }
            break;
        }

        case RESOURCE_DIMENSION_TEXTURECUBEARRAY:
        {
            switch(eType)
            {
                case RETURN_TYPE_SINT:
                    return "isamplerCubeArray";
                case RETURN_TYPE_UINT:
                    return "usamplerCubeArray";
                default:
                    return "samplerCubeArray";
            }
            break;
        }
    }

    return "sampler2D";
}

static void TranslateResourceTexture(HLSLCrossCompilerContext* psContext, const Declaration* psDecl, uint32_t samplerCanDoShadowCmp)
{
    bstring glsl = *psContext->currentShaderString;
    ShaderData* psShader = psContext->psShader;
    uint32_t i;

    const char* samplerTypeName = GetSamplerType(psContext,
        psDecl->value.eResourceDimension,
        psDecl->asOperands[0].ui32RegisterNumber);
    
    if (psContext->flags & HLSLCC_FLAG_COMBINE_TEXTURE_SAMPLERS)
    {
        if(samplerCanDoShadowCmp && psDecl->ui32IsShadowTex)
        {
            for (i = 0; i < psDecl->ui32SamplerUsedCount; i++)
            {
                bcatcstr(glsl, "uniform ");
                bcatcstr(glsl, samplerTypeName);
                bcatcstr(glsl, "Shadow ");
                ConcatTextureSamplerName(glsl, &psShader->sInfo, psDecl->asOperands[0].ui32RegisterNumber, psDecl->ui32SamplerUsed[i], 1);
                bcatcstr(glsl, ";\n");
            }
        }
        for (i = 0; i < psDecl->ui32SamplerUsedCount; i++)
        {
            bcatcstr(glsl, "uniform ");
            bcatcstr(glsl, samplerTypeName);
            bcatcstr(glsl, " ");
            ConcatTextureSamplerName(glsl, &psShader->sInfo, psDecl->asOperands[0].ui32RegisterNumber, psDecl->ui32SamplerUsed[i], 0);
            bcatcstr(glsl, ";\n");
        }
    }

    if(samplerCanDoShadowCmp && psDecl->ui32IsShadowTex)
    {
        //Create shadow and non-shadow sampler.
        //HLSL does not have separate types for depth compare, just different functions.

        bcatcstr(glsl, "uniform ");
        bcatcstr(glsl, samplerTypeName);
        bcatcstr(glsl, "Shadow ");
        ResourceName(glsl, psContext, RGROUP_TEXTURE, psDecl->asOperands[0].ui32RegisterNumber, 1);
        bcatcstr(glsl, ";\n");
    }
                        
    bcatcstr(glsl, "uniform ");
    bcatcstr(glsl, samplerTypeName);
    bcatcstr(glsl, " ");
    ResourceName(glsl, psContext, RGROUP_TEXTURE, psDecl->asOperands[0].ui32RegisterNumber, 0);
    bcatcstr(glsl, ";\n");
}

void TranslateDeclaration(HLSLCrossCompilerContext* psContext, const Declaration* psDecl)
{
    bstring glsl = *psContext->currentShaderString;
    ShaderData* psShader = psContext->psShader;

    switch(psDecl->eOpcode)
    {
        case OPCODE_DCL_INPUT_SGV:
        case OPCODE_DCL_INPUT_PS_SGV:
        {
            const SPECIAL_NAME eSpecialName = psDecl->asOperands[0].eSpecialName;
            switch(eSpecialName)
            {
                case NAME_POSITION:
                {
                    AddBuiltinInput(psContext, psDecl, "gl_Position");
                    break;
                }
                case NAME_RENDER_TARGET_ARRAY_INDEX:
                {
                    AddBuiltinInput(psContext, psDecl, "gl_Layer");
                    break;
                }
                case NAME_CLIP_DISTANCE:
                {
                    AddBuiltinInput(psContext, psDecl, "gl_ClipDistance");
                    break;
                }
                case NAME_VIEWPORT_ARRAY_INDEX:
                {
                    AddBuiltinInput(psContext, psDecl, "gl_ViewportIndex");
                    break;
                }
                case NAME_INSTANCE_ID:
                {
                    AddBuiltinInput(psContext, psDecl, "gl_InstanceID");
                    break;
                }
                case NAME_IS_FRONT_FACE:
                {
                    /*
                        Cast to int used because
                        if(gl_FrontFacing != 0) failed to compiled on Intel HD 4000.
                        Suggests no implicit conversion for bool<->int.
                    */

                    AddBuiltinInput(psContext, psDecl, "int(gl_FrontFacing)");
                    break;
                }
                case NAME_SAMPLE_INDEX:
                {
                    AddBuiltinInput(psContext, psDecl, "gl_SampleID");
                    break;
                }
                case NAME_VERTEX_ID:
                {
                    AddBuiltinInput(psContext, psDecl, "gl_VertexID");
                    break;
                }
                case NAME_PRIMITIVE_ID:
                {
                    AddBuiltinInput(psContext, psDecl, "gl_PrimitiveID");
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
            switch(psDecl->asOperands[0].eSpecialName)
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
                    ASSERT(0); //VertexID is not an output
                    break;
                }
                case NAME_PRIMITIVE_ID:
                {
                    AddBuiltinOutput(psContext, psDecl, GLVARTYPE_INT, 0, "gl_PrimitiveID");
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
                case NAME_FINAL_QUAD_U_EQ_0_EDGE_TESSFACTOR:
                {
                    if(psContext->psShader->aIndexedOutput[psDecl->asOperands[0].ui32RegisterNumber])
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
                    if(psContext->psShader->aIndexedOutput[psDecl->asOperands[0].ui32RegisterNumber])
                    {
                        AddBuiltinOutput(psContext, psDecl, GLVARTYPE_FLOAT, 3,"gl_TessLevelOuter");
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
                    if(psContext->psShader->aIndexedOutput[psDecl->asOperands[0].ui32RegisterNumber])
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
                    if(psContext->psShader->aIndexedOutput[psDecl->asOperands[0].ui32RegisterNumber])
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
            int iNumComponents = 4;//GetMaxComponentFromComponentMask(psOperand);
            const char* StorageQualifier = "attribute";
            const char* InputName;
            const char* Precision = "";

            if((psOperand->eType == OPERAND_TYPE_INPUT_DOMAIN_POINT)||
            (psOperand->eType == OPERAND_TYPE_OUTPUT_CONTROL_POINT_ID)||
            (psOperand->eType == OPERAND_TYPE_INPUT_COVERAGE_MASK)||
            (psOperand->eType == OPERAND_TYPE_INPUT_THREAD_ID)||
            (psOperand->eType == OPERAND_TYPE_INPUT_THREAD_GROUP_ID)||
            (psOperand->eType == OPERAND_TYPE_INPUT_THREAD_ID_IN_GROUP)||
            (psOperand->eType == OPERAND_TYPE_INPUT_THREAD_ID_IN_GROUP_FLATTENED) ||
            (psOperand->eType == OPERAND_TYPE_INPUT_FORK_INSTANCE_ID))
            {
                break;
            }

            //Already declared as part of an array.
            if(psShader->aIndexedInput[psDecl->asOperands[0].ui32RegisterNumber] == -1)
            {
                break;
            }

            InputName = GetDeclaredInputName(psContext, psShader->eShaderType, psOperand);

            if(InOutSupported(psContext->psShader->eTargetLanguage))
            {
                StorageQualifier = "in";
            }

            if(HavePrecisionQualifers(psShader->eTargetLanguage))
            {
                switch(psOperand->eMinPrecision)
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

            break;
        }
        case OPCODE_DCL_INPUT_PS_SIV:
        {
            switch(psDecl->asOperands[0].eSpecialName)
            {
                case NAME_POSITION:
                {
                    AddBuiltinInput(psContext, psDecl, "gl_FragCoord");
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
            const char* StorageQualifier = "varying";
            const char* Precision = "";
            const char* InputName = GetDeclaredInputName(psContext, PIXEL_SHADER, psOperand);
            const char* Interpolation = "";

            if(InOutSupported(psContext->psShader->eTargetLanguage))
            {
                StorageQualifier = "in";
            }

            switch(psDecl->value.eInterpolation)
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

            if(HavePrecisionQualifers(psShader->eTargetLanguage))
            {
                switch(psOperand->eMinPrecision)
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
            
            break;
        }
        case OPCODE_DCL_TEMPS:
        {
            uint32_t i = 0; 
            const uint32_t ui32NumTemps = psDecl->value.ui32NumTemps;

            if(ui32NumTemps > 0)
            {
                bformata(glsl, "vec4 Temp[%d];\n", ui32NumTemps);

                bformata(glsl, "ivec4 Temp_int[%d];\n", ui32NumTemps);
                if(HaveUVec(psShader->eTargetLanguage))
                {
                    bformata(glsl, "uvec4 Temp_uint[%d];\n", ui32NumTemps);
                }
                if(psShader->fp64)
                {
                    bformata(glsl, "dvec4 Temp_double[%d];\n", ui32NumTemps);
                }
            }

            break;
        }
        case OPCODE_SPECIAL_DCL_IMMCONST:
        {
            const Operand* psDest = &psDecl->asOperands[0];
            const Operand* psSrc = &psDecl->asOperands[1];

            ASSERT(psSrc->eType == OPERAND_TYPE_IMMEDIATE32);
            if(psDest->eType == OPERAND_TYPE_SPECIAL_IMMCONSTINT)
            {
                bformata(glsl, "const ivec4 IntImmConst%d = ", psDest->ui32RegisterNumber);
            }
            else
            {
                bformata(glsl, "const vec4 ImmConst%d = ", psDest->ui32RegisterNumber);
                AddToDx9ImmConstIndexableArray(psContext, psDest);
            }
            TranslateOperand(psContext, psSrc, psDest->eType == OPERAND_TYPE_SPECIAL_IMMCONSTINT ? TO_FLAG_INTEGER : TO_AUTO_BITCAST_TO_FLOAT);
            bcatcstr(glsl, ";\n");
            
            break;
        }
        case OPCODE_DCL_CONSTANT_BUFFER:
        {
            const Operand* psOperand = &psDecl->asOperands[0];
            const uint32_t ui32BindingPoint = psOperand->aui32ArraySizes[0];

            const char* StageName = "VS";

            switch(psContext->psShader->eShaderType)
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
            if(!psCBuf)
            {
                if (HaveUniformBindingsAndLocations(psContext->psShader->eTargetLanguage, psContext->psShader->extensions, psContext->flags))
                    bformata(glsl, "layout(location = %d) ",ui32BindingPoint);
                    
                bformata(glsl, "layout(std140) uniform ConstantBuffer%d {\n\tvec4 data[%d];\n} cb%d;\n", ui32BindingPoint,psOperand->aui32ArraySizes[1],ui32BindingPoint);
                break;
            }
            else if (psCBuf->blob)
            {
                bformata(glsl, "layout(std140) uniform %s%s {\n\tvec4 %s%s_data[%d];\n};\n", psCBuf->Name, StageName, psCBuf->Name, StageName, psOperand->aui32ArraySizes[1]);
                break;
            }

            if(psContext->flags & HLSLCC_FLAG_UNIFORM_BUFFER_OBJECT)
            {
                if(psContext->flags & HLSLCC_FLAG_GLOBAL_CONSTS_NEVER_IN_UBO && psCBuf->Name[0] == '$')
                {
                    DeclareStructConstants(psContext, ui32BindingPoint, psCBuf, psOperand, glsl);
                }
                else
                {
                    DeclareUBOConstants(psContext, ui32BindingPoint, psCBuf, glsl);
                }
            }
            else
            {
                DeclareStructConstants(psContext, ui32BindingPoint, psCBuf, psOperand, glsl);
            }
            break;
        }
        case OPCODE_DCL_RESOURCE:
        {
            if (HaveUniformBindingsAndLocations(psContext->psShader->eTargetLanguage, psContext->psShader->extensions, psContext->flags))
            {
                // Explicit layout bindings are not currently compatible with combined texture samplers. The layout below assumes there is exactly one GLSL sampler
                // for each HLSL texture declaration, but when combining textures+samplers, there can be multiple OGL samplers for each HLSL texture declaration.
                if((psContext->flags & HLSLCC_FLAG_COMBINE_TEXTURE_SAMPLERS) != HLSLCC_FLAG_COMBINE_TEXTURE_SAMPLERS)
                {      
                    //Constant buffer locations start at 0. Resource locations start at ui32NumConstantBuffers.
                    bformata(glsl, "layout(location = %d) ", 
                        psContext->psShader->sInfo.ui32NumConstantBuffers + psDecl->asOperands[0].ui32RegisterNumber);
                }
            }

            switch(psDecl->value.eResourceDimension)
            {
                case RESOURCE_DIMENSION_BUFFER:
                {
                    bformata(glsl, "uniform %s ", GetSamplerType(psContext,
                        RESOURCE_DIMENSION_BUFFER,
                        psDecl->asOperands[0].ui32RegisterNumber));
                    TranslateOperand(psContext, &psDecl->asOperands[0], TO_FLAG_NONE);
                    bcatcstr(glsl, ";\n");
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
                    TranslateResourceTexture(psContext, psDecl, 0);
                    break;
                }
                case RESOURCE_DIMENSION_TEXTURECUBEARRAY:
                {
                    TranslateResourceTexture(psContext, psDecl, 1);
                    break;
                }
            }
            ASSERT(psDecl->asOperands[0].ui32RegisterNumber < MAX_TEXTURES);
            psShader->aeResourceDims[psDecl->asOperands[0].ui32RegisterNumber] = psDecl->value.eResourceDimension;
            break;
        }
        case OPCODE_DCL_OUTPUT:
        {
            if(psShader->eShaderType == HULL_SHADER && psDecl->asOperands[0].ui32RegisterNumber==0)
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
            
            if(ui32Flags & GLOBAL_FLAG_FORCE_EARLY_DEPTH_STENCIL)
            {
                bcatcstr(glsl, "layout(early_fragment_tests) in;\n");
            }
            if(!(ui32Flags & GLOBAL_FLAG_REFACTORING_ALLOWED))
            {
                //TODO add precise
                //HLSL precise - http://msdn.microsoft.com/en-us/library/windows/desktop/hh447204(v=vs.85).aspx
            }
            if(ui32Flags & GLOBAL_FLAG_ENABLE_DOUBLE_PRECISION_FLOAT_OPS)
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
            if(psContext->psShader->eShaderType == HULL_SHADER)
            {
                psContext->psShader->sInfo.eTessOutPrim = psDecl->value.eTessOutPrim;
            }
            break;
        }
        case OPCODE_DCL_TESS_DOMAIN:
        {
            if(psContext->psShader->eShaderType == DOMAIN_SHADER)
            {
                switch(psDecl->value.eTessDomain)
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
            if(psContext->psShader->eShaderType == HULL_SHADER)
            {
                psContext->psShader->sInfo.eTessPartitioning = psDecl->value.eTessPartitioning;
            }
            break;
        }
        case OPCODE_DCL_GS_OUTPUT_PRIMITIVE_TOPOLOGY:
        {
            switch(psDecl->value.eOutputPrimitiveTopology)
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
            switch(psDecl->value.eInputPrimitive)
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
            uniformName = &psVar->Name[0];

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
            const uint32_t ui32NumVec4Minus1 = (ui32NumVec4-1);
            uint32_t ui32ConstIndex = 0;
            float x, y, z, w;

            //If ShaderBitEncodingSupported then 1 integer buffer, use intBitsToFloat to get float values. - More instructions.
            //else 2 buffers - one integer and one float. - More data

            if(ShaderBitEncodingSupported(psShader->eTargetLanguage) == 0)
            {
                bcatcstr(glsl, "#define immediateConstBufferI(idx) immediateConstBufferInt[idx]\n");
                bcatcstr(glsl, "#define immediateConstBufferF(idx) immediateConstBuffer[idx]\n");

                bformata(glsl, "vec4 immediateConstBuffer[%d] = vec4[%d] (\n", ui32NumVec4, ui32NumVec4);
                for(;ui32ConstIndex < ui32NumVec4Minus1; ui32ConstIndex++)
                {
                    float loopLocalX, loopLocalY, loopLocalZ, loopLocalW;
                    loopLocalX = *(float*)&psDecl->asImmediateConstBuffer[ui32ConstIndex].a;
                    loopLocalY = *(float*)&psDecl->asImmediateConstBuffer[ui32ConstIndex].b;
                    loopLocalZ = *(float*)&psDecl->asImmediateConstBuffer[ui32ConstIndex].c;
                    loopLocalW = *(float*)&psDecl->asImmediateConstBuffer[ui32ConstIndex].d;

                    //A single vec4 can mix integer and float types.
                    //Forced NAN and INF to zero inside the immediate constant buffer. This will allow the shader to compile.
                    if(fpcheck(loopLocalX))
                    {
                        loopLocalX = 0;
                    }
                    if(fpcheck(loopLocalY))
                    {
                        loopLocalY = 0;
                    }
                    if(fpcheck(loopLocalZ))
                    {
                        loopLocalZ = 0;
                    }
                    if(fpcheck(loopLocalW))
                    {
                        loopLocalW = 0;
                    }

                    bformata(glsl, "\tvec4(%f, %f, %f, %f), \n", loopLocalX, loopLocalY, loopLocalZ, loopLocalW);
                }
                //No trailing comma on this one
                x = *(float*)&psDecl->asImmediateConstBuffer[ui32ConstIndex].a;
                y = *(float*)&psDecl->asImmediateConstBuffer[ui32ConstIndex].b;
                z = *(float*)&psDecl->asImmediateConstBuffer[ui32ConstIndex].c;
                w = *(float*)&psDecl->asImmediateConstBuffer[ui32ConstIndex].d;
                if(fpcheck(x))
                {
                    x = 0;
                }
                if(fpcheck(y))
                {
                    y = 0;
                }
                if(fpcheck(z))
                {
                    z = 0;
                }
                if(fpcheck(w))
                {
                    w = 0;
                }
                bformata(glsl, "\tvec4(%f, %f, %f, %f)\n", x, y, z, w);
                bcatcstr(glsl, ");\n");
            }
            else
            {
                bcatcstr(glsl, "#define immediateConstBufferI(idx) immediateConstBufferInt[idx]\n");
                bcatcstr(glsl, "#define immediateConstBufferF(idx) intBitsToFloat(immediateConstBufferInt[idx])\n");
            }

            {
                uint32_t ui32ConstIndex2 = 0;
                int x2, y2, z2, w2;

                bformata(glsl, "ivec4 immediateConstBufferInt[%d] = ivec4[%d] (\n", ui32NumVec4, ui32NumVec4);
                for (; ui32ConstIndex2 < ui32NumVec4Minus1; ui32ConstIndex2++)
                {
                    int loopLocalX, loopLocalY, loopLocalZ, loopLocalW;
                    loopLocalX = *(int*)&psDecl->asImmediateConstBuffer[ui32ConstIndex2].a;
                    loopLocalY = *(int*)&psDecl->asImmediateConstBuffer[ui32ConstIndex2].b;
                    loopLocalZ = *(int*)&psDecl->asImmediateConstBuffer[ui32ConstIndex2].c;
                    loopLocalW = *(int*)&psDecl->asImmediateConstBuffer[ui32ConstIndex2].d;

                    bformata(glsl, "\tivec4(%d, %d, %d, %d), \n", loopLocalX, loopLocalY, loopLocalZ, loopLocalW);
                }
                //No trailing comma on this one
                x2 = *(int*)&psDecl->asImmediateConstBuffer[ui32ConstIndex2].a;
                y2 = *(int*)&psDecl->asImmediateConstBuffer[ui32ConstIndex2].b;
                z2 = *(int*)&psDecl->asImmediateConstBuffer[ui32ConstIndex2].c;
                w2 = *(int*)&psDecl->asImmediateConstBuffer[ui32ConstIndex2].d;

                bformata(glsl, "\tivec4(%d, %d, %d, %d)\n", x2, y2, z2, w2);
                bcatcstr(glsl, ");\n");
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
            if(HaveUVec(psShader->eTargetLanguage))
            {
                bformata(glsl, "uvec%d TempArray%d_uint[%d];\n", ui32RegComponentSize, ui32RegIndex, ui32RegCount);
            }
            if(psShader->fp64)
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
            if(psContext->psShader->eShaderType == HULL_SHADER)
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
            // non-float images need either 'i' or 'u' prefix.
            char imageTypePrefix[2] = { 0, 0 };
            if(psDecl->sUAV.ui32GloballyCoherentAccess & GLOBALLY_COHERENT_ACCESS)
            {
                bcatcstr(glsl, "coherent ");
            }

            if(psShader->aiOpcodeUsed[OPCODE_LD_UAV_TYPED] == 0)
            {
                bcatcstr(glsl, "writeonly ");
            }
            else
            {
                if(psShader->aiOpcodeUsed[OPCODE_STORE_UAV_TYPED] == 0)
                {
                    bcatcstr(glsl, "readonly ");
                }

                switch(psDecl->sUAV.Type)
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
                    imageTypePrefix[0] = 'u';
                    break;
                case RETURN_TYPE_SINT:
                    bcatcstr(glsl, "layout(rgba32i) ");
                    imageTypePrefix[0] = 'i';
                    break;
                default:
                    ASSERT(0);
                }
            }

            switch(psDecl->value.eResourceDimension)
            {
            case RESOURCE_DIMENSION_BUFFER:
            {
                bformata(glsl, "uniform %simageBuffer ", imageTypePrefix);
                break;
            }
            case RESOURCE_DIMENSION_TEXTURE1D:
            {
                bformata(glsl, "uniform %simage1D ", imageTypePrefix);
                break;
            }
            case RESOURCE_DIMENSION_TEXTURE2D:
            {
                bformata(glsl, "uniform %simage2D ", imageTypePrefix);
                break;
            }
            case RESOURCE_DIMENSION_TEXTURE2DMS:
            {
                bformata(glsl, "uniform %simage2DMS ", imageTypePrefix);
                break;
            }
            case RESOURCE_DIMENSION_TEXTURE3D:
            {
                bformata(glsl, "uniform %simage3D ", imageTypePrefix);
                break;
            }
            case RESOURCE_DIMENSION_TEXTURECUBE:
            {
                bformata(glsl, "uniform %simageCube ", imageTypePrefix);
                break;
            }
            case RESOURCE_DIMENSION_TEXTURE1DARRAY:
            {
                bformata(glsl, "uniform %simage1DArray ", imageTypePrefix);
                break;
            }
            case RESOURCE_DIMENSION_TEXTURE2DARRAY:
            {
                bformata(glsl, "uniform %simage2DArray ", imageTypePrefix);
                break;
            }
            case RESOURCE_DIMENSION_TEXTURE2DMSARRAY:
            {
                bformata(glsl, "uniform %simage3DArray ", imageTypePrefix);
                break;
            }
            case RESOURCE_DIMENSION_TEXTURECUBEARRAY:
            {
                bformata(glsl, "uniform %simageCubeArray ", imageTypePrefix);
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

            if(psDecl->sUAV.bCounter)
            {
                bformata(glsl, "layout (binding = 1) uniform atomic_uint ");
                ResourceName(glsl, psContext, RGROUP_UAV, psDecl->asOperands[0].ui32RegisterNumber, 0);
                bformata(glsl, "_counter; \n");
            }

            GetConstantBufferFromBindingPoint(RGROUP_UAV, ui32BindingPoint, &psContext->psShader->sInfo, &psCBuf);

            DeclareBufferVariable(psContext, ui32BindingPoint, psCBuf, &psDecl->asOperands[0], 
                psDecl->sUAV.ui32GloballyCoherentAccess, RTYPE_UAV_RWSTRUCTURED, glsl);
            break;
        }
        case OPCODE_DCL_UNORDERED_ACCESS_VIEW_RAW:
        {
            if(psDecl->sUAV.bCounter)
            {
                bformata(glsl, "layout (binding = 1) uniform atomic_uint ");
                ResourceName(glsl, psContext, RGROUP_UAV, psDecl->asOperands[0].ui32RegisterNumber, 0);
                bformata(glsl, "_counter; \n");
            }

            bformata(glsl, "buffer Block%d {\n\tuint ", psDecl->asOperands[0].ui32RegisterNumber);
            ResourceName(glsl, psContext, RGROUP_UAV, psDecl->asOperands[0].ui32RegisterNumber, 0);
            bcatcstr(glsl, "[];\n};\n");

            break;
        }
        case OPCODE_DCL_RESOURCE_STRUCTURED:
        {
            ConstantBuffer* psCBuf = NULL;

            GetConstantBufferFromBindingPoint(RGROUP_TEXTURE, psDecl->asOperands[0].ui32RegisterNumber, &psContext->psShader->sInfo, &psCBuf);

            DeclareBufferVariable(psContext, psDecl->asOperands[0].ui32RegisterNumber, psCBuf, &psDecl->asOperands[0], 
                0, RTYPE_STRUCTURED, glsl);
            break;
        }
        case OPCODE_DCL_RESOURCE_RAW:
        {
            bformata(glsl, "buffer Block%d {\n\tuint RawRes%d[];\n};\n", psDecl->asOperands[0].ui32RegisterNumber, psDecl->asOperands[0].ui32RegisterNumber);
            break;
        }
        case OPCODE_DCL_THREAD_GROUP_SHARED_MEMORY_STRUCTURED:
        {
            ShaderVarType* psVarType = &psShader->sGroupSharedVarType[psDecl->asOperands[0].ui32RegisterNumber];

            ASSERT(psDecl->asOperands[0].ui32RegisterNumber < MAX_GROUPSHARED);

            bcatcstr(glsl, "shared struct {\n");
                bformata(glsl, "uint value[%d];\n", psDecl->sTGSM.ui32Stride/4);
            bcatcstr(glsl, "} ");
            TranslateOperand(psContext, &psDecl->asOperands[0], TO_FLAG_NONE);
            bformata(glsl, "[%d];\n",
                psDecl->sTGSM.ui32Count);

            memset(psVarType, 0, sizeof(ShaderVarType));
            strcpy(psVarType->Name, "$Element");

            psVarType->Columns = psDecl->sTGSM.ui32Stride/4;
            psVarType->Elements = psDecl->sTGSM.ui32Count;
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
void ConsolidateHullTempVars(ShaderData* psShader)
{
    uint32_t i, k;
    uint32_t ui32Phase, ui32Instance;
    const uint32_t ui32NumDeclLists = psShader->asPhase[HS_FORK_PHASE].ui32InstanceCount +
        psShader->asPhase[HS_CTRL_POINT_PHASE].ui32InstanceCount +
        psShader->asPhase[HS_JOIN_PHASE].ui32InstanceCount +
        psShader->asPhase[HS_GLOBAL_DECL].ui32InstanceCount;

    Declaration** pasDeclArray = hlslcc_malloc(sizeof(Declaration*) * ui32NumDeclLists);

    uint32_t* pui32DeclCounts = hlslcc_malloc(sizeof(uint32_t) * ui32NumDeclLists);
    uint32_t ui32NumTemps = 0;

    i=0;
    for(ui32Phase = HS_GLOBAL_DECL; ui32Phase < NUM_PHASES; ui32Phase++)
    {
        for(ui32Instance = 0; ui32Instance < psShader->asPhase[ui32Phase].ui32InstanceCount; ++ui32Instance)
        {
            pasDeclArray[i] = psShader->asPhase[ui32Phase].ppsDecl[ui32Instance];
            pui32DeclCounts[i++] = psShader->asPhase[ui32Phase].pui32DeclCount[ui32Instance];
        }
    }

    for(k = 0; k < ui32NumDeclLists; ++k)
    {
        for(i=0; i < pui32DeclCounts[k]; ++i)
        {
            Declaration* psDecl = pasDeclArray[k]+i;
 
            if(psDecl->eOpcode == OPCODE_DCL_TEMPS)
            {
                if(ui32NumTemps < psDecl->value.ui32NumTemps)
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
    for(k = 0; k < ui32NumDeclLists; ++k)
    {
        for(i=0; i < pui32DeclCounts[k]; ++i)
        {
            Declaration* psDecl = pasDeclArray[k]+i;
 
            if(psDecl->eOpcode == OPCODE_DCL_TEMPS)
            {
                psDecl->value.ui32NumTemps = ui32NumTemps;
                return;
            }
        }
    }

    hlslcc_free(pasDeclArray);
    hlslcc_free(pui32DeclCounts);
}

