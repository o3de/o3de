// Modifications copyright Amazon.com, Inc. or its affiliates
// Modifications copyright Crytek GmbH

#include "internal_includes/toGLSLOperand.h"
#include "internal_includes/toGLSLDeclaration.h"
#include "internal_includes/hlslccToolkit.h"
#include "internal_includes/languages.h"
#include "bstrlib.h"
#include "hlslcc.h"
#include "internal_includes/debug.h"

#include <float.h>
#include <math.h>
#include <stdbool.h>

#if !defined(isnan)
#ifdef _MSC_VER
#define isnan(x) _isnan(x)
#define isinf(x) (!_finite(x))
#endif
#endif

#define fpcheck(x) (isnan(x) || isinf(x))

extern void AddIndentation(HLSLCrossCompilerContext* psContext);

// Returns true if types are just different precisions of the same underlying type
static bool AreTypesCompatible(SHADER_VARIABLE_TYPE a, uint32_t ui32TOFlag)
{
    SHADER_VARIABLE_TYPE b = TypeFlagsToSVTType(ui32TOFlag);

    if (a == b)
        return true;

    // Special case for array indices: both uint and int are fine
    if ((ui32TOFlag & TO_FLAG_INTEGER) && (ui32TOFlag & TO_FLAG_UNSIGNED_INTEGER) &&
        (a == SVT_INT || a == SVT_INT16 || a == SVT_UINT || a == SVT_UINT16))
        return true;

    if ((a == SVT_FLOAT || a == SVT_FLOAT16 || a == SVT_FLOAT10) &&
        (b == SVT_FLOAT || b == SVT_FLOAT16 || b == SVT_FLOAT10))
        return true;

    if ((a == SVT_INT || a == SVT_INT16 || a == SVT_INT12) &&
        (b == SVT_INT || b == SVT_INT16 || a == SVT_INT12))
        return true;

    if ((a == SVT_UINT || a == SVT_UINT16) &&
        (b == SVT_UINT || b == SVT_UINT16))
        return true;

    return false;
}

int GetMaxComponentFromComponentMask(const Operand* psOperand)
{
    if (psOperand->iWriteMaskEnabled &&
        psOperand->iNumComponents == 4)
    {
        //Comonent Mask
        if (psOperand->eSelMode == OPERAND_4_COMPONENT_MASK_MODE)
        {
            if (psOperand->ui32CompMask != 0 && psOperand->ui32CompMask != (OPERAND_4_COMPONENT_MASK_X | OPERAND_4_COMPONENT_MASK_Y | OPERAND_4_COMPONENT_MASK_Z | OPERAND_4_COMPONENT_MASK_W))
            {
                if (psOperand->ui32CompMask & OPERAND_4_COMPONENT_MASK_W)
                {
                    return 4;
                }
                if (psOperand->ui32CompMask & OPERAND_4_COMPONENT_MASK_Z)
                {
                    return 3;
                }
                if (psOperand->ui32CompMask & OPERAND_4_COMPONENT_MASK_Y)
                {
                    return 2;
                }
                if (psOperand->ui32CompMask & OPERAND_4_COMPONENT_MASK_X)
                {
                    return 1;
                }
            }
        }
        else
        //Component Swizzle
        if (psOperand->eSelMode == OPERAND_4_COMPONENT_SWIZZLE_MODE)
        {
            return 4;
        }
        else
        if (psOperand->eSelMode == OPERAND_4_COMPONENT_SELECT_1_MODE)
        {
            return 1;
        }
    }

    return 4;
}

//Single component repeated
//e..g .wwww
uint32_t IsSwizzleReplacated(const Operand* psOperand)
{
    if (psOperand->iWriteMaskEnabled &&
        psOperand->iNumComponents == 4)
    {
        if (psOperand->eSelMode == OPERAND_4_COMPONENT_SWIZZLE_MODE)
        {
            if (psOperand->ui32Swizzle == WWWW_SWIZZLE ||
                psOperand->ui32Swizzle == ZZZZ_SWIZZLE ||
                psOperand->ui32Swizzle == YYYY_SWIZZLE ||
                psOperand->ui32Swizzle == XXXX_SWIZZLE)
            {
                return 1;
            }
        }
    }
    return 0;
}

//e.g.
//.z = 1
//.x = 1
//.yw = 2
uint32_t GetNumSwizzleElements(const Operand* psOperand)
{
    uint32_t count = 0;

    switch (psOperand->eType)
    {
    case OPERAND_TYPE_IMMEDIATE32:
    case OPERAND_TYPE_IMMEDIATE64:
    case OPERAND_TYPE_OUTPUT_DEPTH_GREATER_EQUAL:
    case OPERAND_TYPE_OUTPUT_DEPTH_LESS_EQUAL:
    case OPERAND_TYPE_OUTPUT_DEPTH:
    {
        return psOperand->iNumComponents;
    }
    default:
    {
        break;
    }
    }

    if (psOperand->iWriteMaskEnabled &&
        psOperand->iNumComponents == 4)
    {
        //Comonent Mask
        if (psOperand->eSelMode == OPERAND_4_COMPONENT_MASK_MODE)
        {
            if (psOperand->ui32CompMask != 0 && psOperand->ui32CompMask != (OPERAND_4_COMPONENT_MASK_X | OPERAND_4_COMPONENT_MASK_Y | OPERAND_4_COMPONENT_MASK_Z | OPERAND_4_COMPONENT_MASK_W))
            {
                if (psOperand->ui32CompMask & OPERAND_4_COMPONENT_MASK_X)
                {
                    count++;
                }
                if (psOperand->ui32CompMask & OPERAND_4_COMPONENT_MASK_Y)
                {
                    count++;
                }
                if (psOperand->ui32CompMask & OPERAND_4_COMPONENT_MASK_Z)
                {
                    count++;
                }
                if (psOperand->ui32CompMask & OPERAND_4_COMPONENT_MASK_W)
                {
                    count++;
                }
            }
        }
        else
        //Component Swizzle
        if (psOperand->eSelMode == OPERAND_4_COMPONENT_SWIZZLE_MODE)
        {
            if (psOperand->ui32Swizzle != (NO_SWIZZLE))
            {
                uint32_t i;

                for (i = 0; i < 4; ++i)
                {
                    if (psOperand->aui32Swizzle[i] == OPERAND_4_COMPONENT_X)
                    {
                        count++;
                    }
                    else
                    if (psOperand->aui32Swizzle[i] == OPERAND_4_COMPONENT_Y)
                    {
                        count++;
                    }
                    else
                    if (psOperand->aui32Swizzle[i] == OPERAND_4_COMPONENT_Z)
                    {
                        count++;
                    }
                    else
                    if (psOperand->aui32Swizzle[i] == OPERAND_4_COMPONENT_W)
                    {
                        count++;
                    }
                }
            }
        }
        else
        if (psOperand->eSelMode == OPERAND_4_COMPONENT_SELECT_1_MODE)
        {
            if (psOperand->aui32Swizzle[0] == OPERAND_4_COMPONENT_X)
            {
                count++;
            }
            else
            if (psOperand->aui32Swizzle[0] == OPERAND_4_COMPONENT_Y)
            {
                count++;
            }
            else
            if (psOperand->aui32Swizzle[0] == OPERAND_4_COMPONENT_Z)
            {
                count++;
            }
            else
            if (psOperand->aui32Swizzle[0] == OPERAND_4_COMPONENT_W)
            {
                count++;
            }
        }

        //Component Select 1
    }

    if (!count)
    {
        return psOperand->iNumComponents;
    }

    return count;
}

void AddSwizzleUsingElementCount(HLSLCrossCompilerContext* psContext, uint32_t count)
{
    bstring glsl = *psContext->currentGLSLString;
    if (count)
    {
        bcatcstr(glsl, ".");
        bcatcstr(glsl, "x");
        count--;
    }
    if (count)
    {
        bcatcstr(glsl, "y");
        count--;
    }
    if (count)
    {
        bcatcstr(glsl, "z");
        count--;
    }
    if (count)
    {
        bcatcstr(glsl, "w");
        count--;
    }
}

uint32_t ConvertOperandSwizzleToComponentMask(const Operand* psOperand)
{
    uint32_t mask = 0;

    if (psOperand->iWriteMaskEnabled &&
        psOperand->iNumComponents == 4)
    {
        //Comonent Mask
        if (psOperand->eSelMode == OPERAND_4_COMPONENT_MASK_MODE)
        {
            mask = psOperand->ui32CompMask;
        }
        else
        //Component Swizzle
        if (psOperand->eSelMode == OPERAND_4_COMPONENT_SWIZZLE_MODE)
        {
            if (psOperand->ui32Swizzle != (NO_SWIZZLE))
            {
                uint32_t i;

                for (i = 0; i < 4; ++i)
                {
                    if (psOperand->aui32Swizzle[i] == OPERAND_4_COMPONENT_X)
                    {
                        mask |= OPERAND_4_COMPONENT_MASK_X;
                    }
                    else
                    if (psOperand->aui32Swizzle[i] == OPERAND_4_COMPONENT_Y)
                    {
                        mask |= OPERAND_4_COMPONENT_MASK_Y;
                    }
                    else
                    if (psOperand->aui32Swizzle[i] == OPERAND_4_COMPONENT_Z)
                    {
                        mask |= OPERAND_4_COMPONENT_MASK_Z;
                    }
                    else
                    if (psOperand->aui32Swizzle[i] == OPERAND_4_COMPONENT_W)
                    {
                        mask |= OPERAND_4_COMPONENT_MASK_W;
                    }
                }
            }
        }
        else
        if (psOperand->eSelMode == OPERAND_4_COMPONENT_SELECT_1_MODE)
        {
            if (psOperand->aui32Swizzle[0] == OPERAND_4_COMPONENT_X)
            {
                mask |= OPERAND_4_COMPONENT_MASK_X;
            }
            else
            if (psOperand->aui32Swizzle[0] == OPERAND_4_COMPONENT_Y)
            {
                mask |= OPERAND_4_COMPONENT_MASK_Y;
            }
            else
            if (psOperand->aui32Swizzle[0] == OPERAND_4_COMPONENT_Z)
            {
                mask |= OPERAND_4_COMPONENT_MASK_Z;
            }
            else
            if (psOperand->aui32Swizzle[0] == OPERAND_4_COMPONENT_W)
            {
                mask |= OPERAND_4_COMPONENT_MASK_W;
            }
        }

        //Component Select 1
    }

    return mask;
}

//Non-zero means the components overlap
int CompareOperandSwizzles(const Operand* psOperandA, const Operand* psOperandB)
{
    uint32_t maskA = ConvertOperandSwizzleToComponentMask(psOperandA);
    uint32_t maskB = ConvertOperandSwizzleToComponentMask(psOperandB);

    return maskA & maskB;
}


void TranslateOperandSwizzle(HLSLCrossCompilerContext* psContext, const Operand* psOperand)
{
    bstring glsl = *psContext->currentGLSLString;

    if (psOperand->eType == OPERAND_TYPE_INPUT)
    {
        if (psContext->psShader->abScalarInput[psOperand->ui32RegisterNumber])
        {
            return;
        }
    }

    if (psOperand->eType == OPERAND_TYPE_CONSTANT_BUFFER)
    {
        /*ConstantBuffer* psCBuf = NULL;
        ShaderVar* psVar = NULL;
        int32_t index = -1;
        GetConstantBufferFromBindingPoint(psOperand->aui32ArraySizes[0], &psContext->psShader->sInfo, &psCBuf);

        //Access the Nth vec4 (N=psOperand->aui32ArraySizes[1])
        //then apply the sizzle.

        GetShaderVarFromOffset(psOperand->aui32ArraySizes[1], psOperand->aui32Swizzle, psCBuf, &psVar, &index);

        bformata(glsl, ".%s", psVar->Name);
        if(index != -1)
        {
        bformata(glsl, "[%d]", index);
        }*/

        //return;
    }

    if (psOperand->iWriteMaskEnabled &&
        psOperand->iNumComponents == 4)
    {
        //Comonent Mask
        if (psOperand->eSelMode == OPERAND_4_COMPONENT_MASK_MODE)
        {
            if (psOperand->ui32CompMask != 0 && psOperand->ui32CompMask != (OPERAND_4_COMPONENT_MASK_X | OPERAND_4_COMPONENT_MASK_Y | OPERAND_4_COMPONENT_MASK_Z | OPERAND_4_COMPONENT_MASK_W))
            {
                bcatcstr(glsl, ".");
                if (psOperand->ui32CompMask & OPERAND_4_COMPONENT_MASK_X)
                {
                    bcatcstr(glsl, "x");
                }
                if (psOperand->ui32CompMask & OPERAND_4_COMPONENT_MASK_Y)
                {
                    bcatcstr(glsl, "y");
                }
                if (psOperand->ui32CompMask & OPERAND_4_COMPONENT_MASK_Z)
                {
                    bcatcstr(glsl, "z");
                }
                if (psOperand->ui32CompMask & OPERAND_4_COMPONENT_MASK_W)
                {
                    bcatcstr(glsl, "w");
                }
            }
        }
        else
        //Component Swizzle
        if (psOperand->eSelMode == OPERAND_4_COMPONENT_SWIZZLE_MODE)
        {
            if (psOperand->ui32Swizzle != (NO_SWIZZLE))
            {
                uint32_t i;

                bcatcstr(glsl, ".");

                for (i = 0; i < 4; ++i)
                {
                    if (psOperand->aui32Swizzle[i] == OPERAND_4_COMPONENT_X)
                    {
                        bcatcstr(glsl, "x");
                    }
                    else
                    if (psOperand->aui32Swizzle[i] == OPERAND_4_COMPONENT_Y)
                    {
                        bcatcstr(glsl, "y");
                    }
                    else
                    if (psOperand->aui32Swizzle[i] == OPERAND_4_COMPONENT_Z)
                    {
                        bcatcstr(glsl, "z");
                    }
                    else
                    if (psOperand->aui32Swizzle[i] == OPERAND_4_COMPONENT_W)
                    {
                        bcatcstr(glsl, "w");
                    }
                }
            }
        }
        else
        if (psOperand->eSelMode == OPERAND_4_COMPONENT_SELECT_1_MODE)
        {
            bcatcstr(glsl, ".");

            if (psOperand->aui32Swizzle[0] == OPERAND_4_COMPONENT_X)
            {
                bcatcstr(glsl, "x");
            }
            else
            if (psOperand->aui32Swizzle[0] == OPERAND_4_COMPONENT_Y)
            {
                bcatcstr(glsl, "y");
            }
            else
            if (psOperand->aui32Swizzle[0] == OPERAND_4_COMPONENT_Z)
            {
                bcatcstr(glsl, "z");
            }
            else
            if (psOperand->aui32Swizzle[0] == OPERAND_4_COMPONENT_W)
            {
                bcatcstr(glsl, "w");
            }
        }

        //Component Select 1
    }
}

int GetFirstOperandSwizzle(HLSLCrossCompilerContext* psContext, const Operand* psOperand)
{
    if (psOperand->eType == OPERAND_TYPE_INPUT)
    {
        if (psContext->psShader->abScalarInput[psOperand->ui32RegisterNumber])
        {
            return -1;
        }
    }

    if (psOperand->iWriteMaskEnabled &&
        psOperand->iNumComponents == 4)
    {
        //Comonent Mask
        if (psOperand->eSelMode == OPERAND_4_COMPONENT_MASK_MODE)
        {
            if (psOperand->ui32CompMask != 0 && psOperand->ui32CompMask != (OPERAND_4_COMPONENT_MASK_X | OPERAND_4_COMPONENT_MASK_Y | OPERAND_4_COMPONENT_MASK_Z | OPERAND_4_COMPONENT_MASK_W))
            {
                if (psOperand->ui32CompMask & OPERAND_4_COMPONENT_MASK_X)
                {
                    return 0;
                }
                if (psOperand->ui32CompMask & OPERAND_4_COMPONENT_MASK_Y)
                {
                    return 1;
                }
                if (psOperand->ui32CompMask & OPERAND_4_COMPONENT_MASK_Z)
                {
                    return 2;
                }
                if (psOperand->ui32CompMask & OPERAND_4_COMPONENT_MASK_W)
                {
                    return 3;
                }
            }
        }
        else
        //Component Swizzle
        if (psOperand->eSelMode == OPERAND_4_COMPONENT_SWIZZLE_MODE)
        {
            if (psOperand->ui32Swizzle != (NO_SWIZZLE))
            {
                uint32_t i;

                for (i = 0; i < 4; ++i)
                {
                    if (psOperand->aui32Swizzle[i] == OPERAND_4_COMPONENT_X)
                    {
                        return 0;
                    }
                    else
                    if (psOperand->aui32Swizzle[i] == OPERAND_4_COMPONENT_Y)
                    {
                        return 1;
                    }
                    else
                    if (psOperand->aui32Swizzle[i] == OPERAND_4_COMPONENT_Z)
                    {
                        return 2;
                    }
                    else
                    if (psOperand->aui32Swizzle[i] == OPERAND_4_COMPONENT_W)
                    {
                        return 3;
                    }
                }
            }
        }
        else
        if (psOperand->eSelMode == OPERAND_4_COMPONENT_SELECT_1_MODE)
        {
            if (psOperand->aui32Swizzle[0] == OPERAND_4_COMPONENT_X)
            {
                return 0;
            }
            else
            if (psOperand->aui32Swizzle[0] == OPERAND_4_COMPONENT_Y)
            {
                return 1;
            }
            else
            if (psOperand->aui32Swizzle[0] == OPERAND_4_COMPONENT_Z)
            {
                return 2;
            }
            else
            if (psOperand->aui32Swizzle[0] == OPERAND_4_COMPONENT_W)
            {
                return 3;
            }
        }

        //Component Select 1
    }

    return -1;
}

void TranslateOperandIndex(HLSLCrossCompilerContext* psContext, const Operand* psOperand, int index)
{
    int i = index;
    int isGeoShader = psContext->psShader->eShaderType == GEOMETRY_SHADER ? 1 : 0;

    bstring glsl = *psContext->currentGLSLString;

    ASSERT(index < psOperand->iIndexDims);

    switch (psOperand->eIndexRep[i])
    {
    case OPERAND_INDEX_IMMEDIATE32:
    {
        if (i > 0 || isGeoShader)
        {
            bformata(glsl, "[%d]", psOperand->aui32ArraySizes[i]);
        }
        else
        {
            bformata(glsl, "%d", psOperand->aui32ArraySizes[i]);
        }
        break;
    }
    case OPERAND_INDEX_RELATIVE:
    {
        bcatcstr(glsl, "[int(");     //Indexes must be integral.
        TranslateOperand(psContext, psOperand->psSubOperand[i], TO_FLAG_INTEGER);
        bcatcstr(glsl, ")]");
        break;
    }
    case OPERAND_INDEX_IMMEDIATE32_PLUS_RELATIVE:
    {
        bcatcstr(glsl, "[int(");     //Indexes must be integral.
        TranslateOperand(psContext, psOperand->psSubOperand[i], TO_FLAG_INTEGER);
        bformata(glsl, ") + %d]", psOperand->aui32ArraySizes[i]);
        break;
    }
    default:
    {
        break;
    }
    }
}

void TranslateOperandIndexMAD(HLSLCrossCompilerContext* psContext, const Operand* psOperand, int index, uint32_t multiply, uint32_t add)
{
    int i = index;
    int isGeoShader = psContext->psShader->eShaderType == GEOMETRY_SHADER ? 1 : 0;

    bstring glsl = *psContext->currentGLSLString;

    ASSERT(index < psOperand->iIndexDims);

    switch (psOperand->eIndexRep[i])
    {
    case OPERAND_INDEX_IMMEDIATE32:
    {
        if (i > 0 || isGeoShader)
        {
            bformata(glsl, "[%d*%d+%d]", psOperand->aui32ArraySizes[i], multiply, add);
        }
        else
        {
            bformata(glsl, "%d*%d+%d", psOperand->aui32ArraySizes[i], multiply, add);
        }
        break;
    }
    case OPERAND_INDEX_RELATIVE:
    {
        bcatcstr(glsl, "[int(");     //Indexes must be integral.
        TranslateOperand(psContext, psOperand->psSubOperand[i], TO_FLAG_INTEGER);
        bformata(glsl, ")*%d+%d]", multiply, add);
        break;
    }
    case OPERAND_INDEX_IMMEDIATE32_PLUS_RELATIVE:
    {
        bcatcstr(glsl, "[(int(");     //Indexes must be integral.
        TranslateOperand(psContext, psOperand->psSubOperand[i], TO_FLAG_INTEGER);
        bformata(glsl, ") + %d)*%d+%d]", psOperand->aui32ArraySizes[i], multiply, add);
        break;
    }
    default:
    {
        break;
    }
    }
}

void TranslateVariableNameByOperandType(HLSLCrossCompilerContext* psContext, const Operand* psOperand, uint32_t ui32TOFlag, uint32_t* pui32IgnoreSwizzle)
{
    bstring glsl = *psContext->currentGLSLString;

    switch (psOperand->eType)
    {
    case OPERAND_TYPE_IMMEDIATE32:
    {
        if (psOperand->iNumComponents == 1)
        {
            if (ui32TOFlag & TO_FLAG_UNSIGNED_INTEGER)
            {
                bformata(glsl, "%uu",
                    *((unsigned int*)(&psOperand->afImmediates[0])));
            }
            else
            if ((ui32TOFlag & TO_FLAG_INTEGER) || ((ui32TOFlag & TO_FLAG_FLOAT) == 0 && psOperand->iIntegerImmediate) || fpcheck(psOperand->afImmediates[0]))
            {
                if (ui32TOFlag & TO_FLAG_FLOAT)
                {
                    bcatcstr(glsl, "float");
                }
                else if (ui32TOFlag & TO_FLAG_INTEGER)
                {
                    bcatcstr(glsl, "int");
                }
                bcatcstr(glsl, "(");

                //  yet another Qualcomm's special case
                //  GLSL compiler thinks that -2147483648 is an integer overflow which is not
                if (*((int*)(&psOperand->afImmediates[0])) == 2147483648)
                {
                    bformata(glsl, "-2147483647-1");
                }
                else
                {
                    //  this is expected to fix paranoid compiler checks such as Qualcomm's
                    if (*((unsigned int*)(&psOperand->afImmediates[0])) >= 2147483648)
                    {
                        bformata(glsl, "%d",
                            *((int*)(&psOperand->afImmediates[0])));
                    }
                    else
                    {
                        bformata(glsl, "%d",
                            *((int*)(&psOperand->afImmediates[0])));
                    }
                }
                bcatcstr(glsl, ")");
            }
            else
            {
                bformata(glsl, "%e",
                    psOperand->afImmediates[0]);
            }
        }
        else
        {
            if (ui32TOFlag & TO_FLAG_UNSIGNED_INTEGER)
            {
                bformata(glsl, "uvec4(%uu, %uu, %uu, %uu)",
                    *(unsigned int*)&psOperand->afImmediates[0],
                    *(unsigned int*)&psOperand->afImmediates[1],
                    *(unsigned int*)&psOperand->afImmediates[2],
                    *(unsigned int*)&psOperand->afImmediates[3]);
            }
            else
            if ((ui32TOFlag & TO_FLAG_INTEGER) ||
                ((ui32TOFlag & TO_FLAG_FLOAT) == 0 && psOperand->iIntegerImmediate) ||
                fpcheck(psOperand->afImmediates[0]) ||
                fpcheck(psOperand->afImmediates[1]) ||
                fpcheck(psOperand->afImmediates[2]) ||
                fpcheck(psOperand->afImmediates[3]))
            {
                //  this is expected to fix paranoid compiler checks such as Qualcomm's
                if (ui32TOFlag & TO_FLAG_FLOAT)
                {
                    bcatcstr(glsl, "vec4");
                }
                else if (ui32TOFlag & TO_FLAG_INTEGER)
                {
                    bcatcstr(glsl, "ivec4");
                }
                else if (ui32TOFlag & TO_FLAG_UNSIGNED_INTEGER)
                {
                    bcatcstr(glsl, "uvec4");
                }
                bcatcstr(glsl, "(");
                
                if ((*(unsigned int*)&psOperand->afImmediates[0]) == 2147483648u)
                {
                    bformata(glsl, "int(-2147483647-1), ");
                }
                else
                {
                    bformata(glsl, "%d, ", *(int*)&psOperand->afImmediates[0]);
                }
                if ((*(unsigned int*)&psOperand->afImmediates[1]) == 2147483648u)
                {
                    bformata(glsl, "int(-2147483647-1), ");
                }
                else
                {
                    bformata(glsl, "%d, ", *(int*)&psOperand->afImmediates[1]);
                }
                if ((*(unsigned int*)&psOperand->afImmediates[2]) == 2147483648u)
                {
                    bformata(glsl, "int(-2147483647-1), ");
                }
                else
                {
                    bformata(glsl, "%d, ", *(int*)&psOperand->afImmediates[2]);
                }
                if ((*(unsigned int*)&psOperand->afImmediates[3]) == 2147483648u)
                {
                    bformata(glsl, "int(-2147483647-1)) ");
                }
                else
                {
                    bformata(glsl, "%d)", *(int*)&psOperand->afImmediates[3]);
                }
            }
            else
            {
                bformata(glsl, "vec4(%e, %e, %e, %e)",
                    psOperand->afImmediates[0],
                    psOperand->afImmediates[1],
                    psOperand->afImmediates[2],
                    psOperand->afImmediates[3]);
            }
            if (psOperand->iNumComponents != 4)
            {
                AddSwizzleUsingElementCount(psContext, psOperand->iNumComponents);
            }
        }
        break;
    }
    case OPERAND_TYPE_IMMEDIATE64:
    {
        if (psOperand->iNumComponents == 1)
        {
            bformata(glsl, "%e",
                psOperand->adImmediates[0]);
        }
        else
        {
            bformata(glsl, "dvec4(%e, %e, %e, %e)",
                psOperand->adImmediates[0],
                psOperand->adImmediates[1],
                psOperand->adImmediates[2],
                psOperand->adImmediates[3]);
            if (psOperand->iNumComponents != 4)
            {
                AddSwizzleUsingElementCount(psContext, psOperand->iNumComponents);
            }
        }
        break;
    }
    case OPERAND_TYPE_INPUT:
    {
        switch (psOperand->iIndexDims)
        {
        case INDEX_2D:
        {
            if (psOperand->aui32ArraySizes[1] == 0)       //Input index zero - position.
            {
                bcatcstr(glsl, "gl_in");
                TranslateOperandIndex(psContext, psOperand, TO_FLAG_NONE);        //Vertex index
                bcatcstr(glsl, ".gl_Position");
            }
            else
            {
                const char* name = "Input";
                if (ui32TOFlag & TO_FLAG_DECLARATION_NAME)
                {
                    name = GetDeclaredInputName(psContext, psContext->psShader->eShaderType, psOperand);
                }

                bformata(glsl, "%s%d", name, psOperand->aui32ArraySizes[1]);
                if (ui32TOFlag & TO_FLAG_DECLARATION_NAME)
                {
                    bcstrfree((char*)name);
                }
                TranslateOperandIndex(psContext, psOperand, TO_FLAG_NONE);        //Vertex index
            }
            break;
        }
        default:
        {
            if (psOperand->eIndexRep[0] == OPERAND_INDEX_IMMEDIATE32_PLUS_RELATIVE)
            {
                bformata(glsl, "Input%d[int(", psOperand->ui32RegisterNumber);
                TranslateOperand(psContext, psOperand->psSubOperand[0], TO_FLAG_INTEGER);
                bcatcstr(glsl, ")]");
            }
            else
            {
                if (psContext->psShader->aIndexedInput[psOperand->ui32RegisterNumber] != 0)
                {
                    const uint32_t parentIndex = psContext->psShader->aIndexedInputParents[psOperand->ui32RegisterNumber];
                    bformata(glsl, "Input%d[%d]", parentIndex,
                        psOperand->ui32RegisterNumber - parentIndex);
                }
                else
                {
                    if (ui32TOFlag & TO_FLAG_DECLARATION_NAME)
                    {
                        char* name = GetDeclaredInputName(psContext, psContext->psShader->eShaderType, psOperand);
                        bcatcstr(glsl, name);
                        bcstrfree(name);
                    }
                    else
                    {
                        bformata(glsl, "Input%d", psOperand->ui32RegisterNumber);
                    }
                }
            }
            break;
        }
        }
        break;
    }
    case OPERAND_TYPE_OUTPUT:
    {
        bformata(glsl, "Output%d", psOperand->ui32RegisterNumber);
        if (psOperand->psSubOperand[0])
        {
            bcatcstr(glsl, "[int(");     //Indexes must be integral.
            TranslateOperand(psContext, psOperand->psSubOperand[0], TO_FLAG_INTEGER);
            bcatcstr(glsl, ")]");
        }
        break;
    }
    case OPERAND_TYPE_OUTPUT_DEPTH:
    case OPERAND_TYPE_OUTPUT_DEPTH_GREATER_EQUAL:
    case OPERAND_TYPE_OUTPUT_DEPTH_LESS_EQUAL:
    {
        bcatcstr(glsl, "gl_FragDepth");
        break;
    }
    case OPERAND_TYPE_TEMP:
    {
        SHADER_VARIABLE_TYPE eType = GetOperandDataType(psContext, psOperand);
        bcatcstr(glsl, "Temp");

        if ((psContext->flags & HLSLCC_FLAG_AVOID_TEMP_REGISTER_ALIASING) == 0 || psContext->psShader->eShaderType == HULL_SHADER)
        {
            if (eType == SVT_INT)
            {
                bcatcstr(glsl, "_int");
            }
            else if (eType == SVT_UINT)
            {
                bcatcstr(glsl, "_uint");
            }
            else if (eType == SVT_DOUBLE)
            {
                bcatcstr(glsl, "_double");
            }
            else if (eType == SVT_VOID ||
                     (ui32TOFlag & TO_FLAG_DESTINATION))
            {
                if (ui32TOFlag & TO_FLAG_INTEGER)
                {
                    bcatcstr(glsl, "_int");
                }
                else
                if (ui32TOFlag & TO_FLAG_UNSIGNED_INTEGER)
                {
                    bcatcstr(glsl, "_uint");
                }
            }

            bformata(glsl, "[%d]", psOperand->ui32RegisterNumber);
        }
        else
        {
            if (psContext->flags & HLSLCC_FLAG_QUALCOMM_GLES30_DRIVER_WORKAROUND)
                bformata(glsl, "%d[0]", psOperand->ui32RegisterNumber);
            else
                bformata(glsl, "%d", psOperand->ui32RegisterNumber);
        }
        break;
    }
    case OPERAND_TYPE_SPECIAL_IMMCONSTINT:
    {
        bformata(glsl, "IntImmConst%d", psOperand->ui32RegisterNumber);
        break;
    }
    case OPERAND_TYPE_SPECIAL_IMMCONST:
    {
        if (psOperand->psSubOperand[0] != NULL)
        {
            bformata(glsl, "ImmConstArray[%d + ", psContext->psShader->aui32Dx9ImmConstArrayRemap[psOperand->ui32RegisterNumber]);
            TranslateOperand(psContext, psOperand->psSubOperand[0], TO_FLAG_NONE);
            bcatcstr(glsl, "]");
        }
        else
        {
            bformata(glsl, "ImmConst%d", psOperand->ui32RegisterNumber);
        }
        break;
    }
    case OPERAND_TYPE_SPECIAL_OUTBASECOLOUR:
    {
        bcatcstr(glsl, "BaseColour");
        break;
    }
    case OPERAND_TYPE_SPECIAL_OUTOFFSETCOLOUR:
    {
        bcatcstr(glsl, "OffsetColour");
        break;
    }
    case OPERAND_TYPE_SPECIAL_POSITION:
    {
        bcatcstr(glsl, "gl_Position");
        break;
    }
    case OPERAND_TYPE_SPECIAL_FOG:
    {
        bcatcstr(glsl, "Fog");
        break;
    }
    case OPERAND_TYPE_SPECIAL_POINTSIZE:
    {
        bcatcstr(glsl, "gl_PointSize");
        break;
    }
    case OPERAND_TYPE_SPECIAL_ADDRESS:
    {
        bcatcstr(glsl, "Address");
        break;
    }
    case OPERAND_TYPE_SPECIAL_LOOPCOUNTER:
    {
        bcatcstr(glsl, "LoopCounter");
        pui32IgnoreSwizzle[0] = 1;
        break;
    }
    case OPERAND_TYPE_SPECIAL_TEXCOORD:
    {
        bformata(glsl, "TexCoord%d", psOperand->ui32RegisterNumber);
        break;
    }
    case OPERAND_TYPE_CONSTANT_BUFFER:
    {
        ConstantBuffer* psCBuf = NULL;
        ShaderVarType* psVarType = NULL;
        int32_t index = -1;
        bool addParentheses = false;
        GetConstantBufferFromBindingPoint(RGROUP_CBUFFER, psOperand->aui32ArraySizes[0], &psContext->psShader->sInfo, &psCBuf);

        if (ui32TOFlag & TO_FLAG_DECLARATION_NAME)
        {
            pui32IgnoreSwizzle[0] = 1;
        }

        if ((psContext->flags & HLSLCC_FLAG_UNIFORM_BUFFER_OBJECT) != HLSLCC_FLAG_UNIFORM_BUFFER_OBJECT)
        {
            if (psCBuf)
            {
                //$Globals.
                if (psCBuf->Name[0] == '$')
                {
                    ConvertToUniformBufferName(glsl, psContext->psShader, "$Globals");
                }
                else
                {
                    ConvertToUniformBufferName(glsl, psContext->psShader, psCBuf->Name);
                }
                if ((ui32TOFlag & TO_FLAG_DECLARATION_NAME) != TO_FLAG_DECLARATION_NAME)
                {
                    bcatcstr(glsl, ".");
                }
            }
            else
            {
                //bformata(glsl, "cb%d", psOperand->aui32ArraySizes[0]);
            }
        }

        if ((ui32TOFlag & TO_FLAG_DECLARATION_NAME) != TO_FLAG_DECLARATION_NAME)
        {
            //Work out the variable name. Don't apply swizzle to that variable yet.
            int32_t rebase = 0;

            if (psCBuf && !psCBuf->blob)
            {
                GetShaderVarFromOffset(psOperand->aui32ArraySizes[1], psOperand->aui32Swizzle, psCBuf, &psVarType, &index, &rebase);
                if (psContext->flags & HLSLCC_FLAG_QUALCOMM_GLES30_DRIVER_WORKAROUND)
                {
                    if (psVarType->Class == SVC_VECTOR || psVarType->Class == SVC_MATRIX_COLUMNS || psVarType->Class == SVC_MATRIX_ROWS)
                    {
                        switch (psVarType->Type)
                        {
                        case SVT_FLOAT:
                        case SVT_FLOAT16:
                        case SVT_FLOAT10:
                        {
                            bformata(glsl, "vec%d(", psVarType->Columns);
                            break;
                        }
                        case SVT_UINT:
                        case SVT_UINT16:
                        {
                            bformata(glsl, "uvec%d(", psVarType->Columns);
                            break;
                        }
                        case SVT_INT:
                        case SVT_INT16:
                        case SVT_INT12:
                        {
                            bformata(glsl, "ivec%d(", psVarType->Columns);
                            break;
                        }
                        default:
                        {
                            ASSERT(0);
                            break;
                        }
                        }
                        addParentheses = true;
                    }
                    else if (psVarType->Class == SVC_SCALAR)
                    {
                        switch (psVarType->Type)
                        {
                        case SVT_FLOAT:
                        case SVT_FLOAT16:
                        case SVT_FLOAT10:
                        {
                            bformata(glsl, "float(");
                            break;
                        }
                        case SVT_UINT:
                        case SVT_UINT16:
                        {
                            bformata(glsl, "uint(");
                            break;
                        }
                        case SVT_INT:
                        case SVT_INT16:
                        case SVT_INT12:
                        {
                            bformata(glsl, "int(");
                            break;
                        }
                        default:
                        {
                            ASSERT(0);
                            break;
                        }
                        }
                        addParentheses = true;
                    }
                }
                ShaderVarFullName(glsl, psContext->psShader, psVarType);
            }
            else if (psCBuf)
            {
                ConvertToUniformBufferName(glsl, psContext->psShader, psCBuf->Name);
                bcatcstr(glsl, "_data");
                index = psOperand->aui32ArraySizes[1];
            }
            else
            // We don't have a semantic for this variable, so try the raw dump appoach.
            {
                bformata(glsl, "cb%d.data", psOperand->aui32ArraySizes[0]);    //
                index = psOperand->aui32ArraySizes[1];
            }

            //Dx9 only?
            if (psOperand->psSubOperand[0] != NULL)
            {
                SHADER_VARIABLE_TYPE eType = GetOperandDataType(psContext, psOperand->psSubOperand[0]);
                if (eType != SVT_INT && eType != SVT_UINT)
                {
                    bcatcstr(glsl, "[int(");     //Indexes must be integral.
                    TranslateOperand(psContext, psOperand->psSubOperand[0], TO_FLAG_INTEGER);
                    bcatcstr(glsl, ")]");
                }
                else
                {
                    bcatcstr(glsl, "[");     //Indexes must be integral.
                    TranslateOperand(psContext, psOperand->psSubOperand[0], TO_FLAG_INTEGER);
                    bcatcstr(glsl, "]");
                }
            }
            else
            if (index != -1 && psOperand->psSubOperand[1] != NULL)
            {
                //Array of matrices is treated as array of vec4s
                if (index != -1)
                {
                    SHADER_VARIABLE_TYPE eType = GetOperandDataType(psContext, psOperand->psSubOperand[1]);
                    if (eType != SVT_INT && eType != SVT_UINT)
                    {
                        bcatcstr(glsl, "[int(");
                        TranslateOperand(psContext, psOperand->psSubOperand[1], TO_FLAG_INTEGER);
                        bformata(glsl, ") + %d]", index);
                    }
                    else
                    {
                        bcatcstr(glsl, "[");
                        TranslateOperand(psContext, psOperand->psSubOperand[1], TO_FLAG_INTEGER);
                        bformata(glsl, " + %d]", index);
                    }
                }
            }
            else if (index != -1)
            {
                bformata(glsl, "[%d]", index);
            }
            else if (psOperand->psSubOperand[1] != NULL)
            {
                SHADER_VARIABLE_TYPE eType = GetOperandDataType(psContext, psOperand->psSubOperand[1]);
                if (eType != SVT_INT && eType != SVT_UINT)
                {
                    bcatcstr(glsl, "[");
                    TranslateOperand(psContext, psOperand->psSubOperand[1], TO_FLAG_INTEGER);
                    bcatcstr(glsl, "]");
                }
                else
                {
                    bcatcstr(glsl, "[int(");
                    TranslateOperand(psContext, psOperand->psSubOperand[1], TO_FLAG_INTEGER);
                    bcatcstr(glsl, ")]");
                }
            }

            if (addParentheses)
                bcatcstr(glsl, ")");

            if (psVarType && psVarType->Class == SVC_VECTOR)
            {
                switch (rebase)
                {
                case 4:
                {
                    if (psVarType->Columns == 2)
                    {
                        //.x(GLSL) is .y(HLSL). .y(GLSL) is .z(HLSL)
                        bcatcstr(glsl, ".xxyx");
                    }
                    else if (psVarType->Columns == 3)
                    {
                        //.x(GLSL) is .y(HLSL). .y(GLSL) is .z(HLSL) .z(GLSL) is .w(HLSL)
                        bcatcstr(glsl, ".xxyz");
                    }
                    break;
                }
                case 8:
                {
                    if (psVarType->Columns == 2)
                    {
                        //.x(GLSL) is .z(HLSL). .y(GLSL) is .w(HLSL)
                        bcatcstr(glsl, ".xxxy");
                    }
                    break;
                }
                case 0:
                default:
                {
                    //No rebase, but extend to vec4.
                    if (psVarType->Columns == 2)
                    {
                        bcatcstr(glsl, ".xyxx");
                    }
                    else if (psVarType->Columns == 3)
                    {
                        bcatcstr(glsl, ".xyzx");
                    }
                    break;
                }
                }
            }

            if (psVarType && psVarType->Class == SVC_SCALAR)
            {
                *pui32IgnoreSwizzle = 1;
            }
        }
        break;
    }
    case OPERAND_TYPE_RESOURCE:
    {
        TextureName(*psContext->currentGLSLString, psContext->psShader, psOperand->ui32RegisterNumber, MAX_RESOURCE_BINDINGS, 0);
        *pui32IgnoreSwizzle = 1;
        break;
    }
    case OPERAND_TYPE_SAMPLER:
    {
        bformata(glsl, "Sampler%d", psOperand->ui32RegisterNumber);
        *pui32IgnoreSwizzle = 1;
        break;
    }
    case OPERAND_TYPE_FUNCTION_BODY:
    {
        const uint32_t ui32FuncBody = psOperand->ui32RegisterNumber;
        const uint32_t ui32FuncTable = psContext->psShader->aui32FuncBodyToFuncTable[ui32FuncBody];
        //const uint32_t ui32FuncPointer = psContext->psShader->aui32FuncTableToFuncPointer[ui32FuncTable];
        const uint32_t ui32ClassType = psContext->psShader->sInfo.aui32TableIDToTypeID[ui32FuncTable];
        const char* ClassTypeName = &psContext->psShader->sInfo.psClassTypes[ui32ClassType].Name[0];
        const uint32_t ui32UniqueClassFuncIndex = psContext->psShader->ui32NextClassFuncName[ui32ClassType]++;

        bformata(glsl, "%s_Func%d", ClassTypeName, ui32UniqueClassFuncIndex);
        break;
    }
    case OPERAND_TYPE_INPUT_FORK_INSTANCE_ID:
    {
        bcatcstr(glsl, "forkInstanceID");
        *pui32IgnoreSwizzle = 1;
        return;
    }
    case OPERAND_TYPE_IMMEDIATE_CONSTANT_BUFFER:
    {
        bcatcstr(glsl, "immediateConstBufferF");

        if (psOperand->psSubOperand[0])
        {
            bcatcstr(glsl, "(int(");     //Indexes must be integral.
            TranslateOperand(psContext, psOperand->psSubOperand[0], TO_FLAG_INTEGER);
            bcatcstr(glsl, "))");
        }
        break;
    }
    case OPERAND_TYPE_INPUT_DOMAIN_POINT:
    {
        bcatcstr(glsl, "gl_TessCoord");
        break;
    }
    case OPERAND_TYPE_INPUT_CONTROL_POINT:
    {
        if (psOperand->aui32ArraySizes[1] == 0)   //Input index zero - position.
        {
            bformata(glsl, "gl_in[%d].gl_Position", psOperand->aui32ArraySizes[0]);
        }
        else
        {
            bformata(glsl, "Input%d[%d]", psOperand->aui32ArraySizes[1], psOperand->aui32ArraySizes[0]);
        }
        break;
    }
    case OPERAND_TYPE_NULL:
    {
        // Null register, used to discard results of operations
        bcatcstr(glsl, "//null");
        break;
    }
    case OPERAND_TYPE_OUTPUT_CONTROL_POINT_ID:
    {
        bcatcstr(glsl, "gl_InvocationID");
        *pui32IgnoreSwizzle = 1;
        break;
    }
    case OPERAND_TYPE_OUTPUT_COVERAGE_MASK:
    {
        bcatcstr(glsl, "gl_SampleMask[0]");
        *pui32IgnoreSwizzle = 1;
        break;
    }
    case OPERAND_TYPE_INPUT_COVERAGE_MASK:
    {
        bcatcstr(glsl, "gl_SampleMaskIn[0]");
        //Skip swizzle on scalar types.
        *pui32IgnoreSwizzle = 1;
        break;
    }
    case OPERAND_TYPE_INPUT_THREAD_ID://SV_DispatchThreadID
    {
        bcatcstr(glsl, "gl_GlobalInvocationID.xyzz");
        break;
    }
    case OPERAND_TYPE_INPUT_THREAD_GROUP_ID://SV_GroupThreadID
    {
        bcatcstr(glsl, "gl_WorkGroupID.xyzz");
        break;
    }
    case OPERAND_TYPE_INPUT_THREAD_ID_IN_GROUP://SV_GroupID
    {
        bcatcstr(glsl, "gl_LocalInvocationID.xyzz");
        break;
    }
    case OPERAND_TYPE_INPUT_THREAD_ID_IN_GROUP_FLATTENED://SV_GroupIndex
    {
        bcatcstr(glsl, "gl_LocalInvocationIndex.xyzz");
        break;
    }
    case OPERAND_TYPE_UNORDERED_ACCESS_VIEW:
    {
        UAVName(*psContext->currentGLSLString, psContext->psShader, psOperand->ui32RegisterNumber);
        break;
    }
    case OPERAND_TYPE_THREAD_GROUP_SHARED_MEMORY:
    {
        bformata(glsl, "TGSM%d", psOperand->ui32RegisterNumber);
        *pui32IgnoreSwizzle = 1;
        break;
    }
    case OPERAND_TYPE_INPUT_PRIMITIVEID:
    {
        bcatcstr(glsl, "gl_PrimitiveID");
        break;
    }
    case OPERAND_TYPE_INDEXABLE_TEMP:
    {
        bformata(glsl, "TempArray%d", psOperand->aui32ArraySizes[0]);
        bformata(glsl, "[%d", psOperand->aui32ArraySizes[1]);

        if (psOperand->psSubOperand[1])
        {
            bcatcstr(glsl, "+");
            TranslateOperand(psContext, psOperand->psSubOperand[1], TO_FLAG_UNSIGNED_INTEGER);
        }
        bcatcstr(glsl, "]");
        break;
    }
    case OPERAND_TYPE_STREAM:
    {
        bformata(glsl, "%d", psOperand->ui32RegisterNumber);
        break;
    }
    case OPERAND_TYPE_INPUT_GS_INSTANCE_ID:
    {
        bcatcstr(glsl, "gl_InvocationID");
        break;
    }
    case OPERAND_TYPE_THIS_POINTER:
    {
        /*
        The "this" register is a register that provides up to 4 pieces of information:
        X: Which CB holds the instance data
        Y: Base element offset of the instance data within the instance CB
        Z: Base sampler index
        W: Base Texture index

        Can be different for each function call
        */
        break;
    }
    default:
    {
        ASSERT(0);
        break;
    }
    }
}

void TranslateVariableName(HLSLCrossCompilerContext* psContext, const Operand* psOperand, uint32_t ui32TOFlag, uint32_t* pui32IgnoreSwizzle)
{
    bool hasConstructor = false;
    bstring glsl = *psContext->currentGLSLString;

    *pui32IgnoreSwizzle = 0;

    if (psOperand->eType != OPERAND_TYPE_IMMEDIATE32 &&
        psOperand->eType != OPERAND_TYPE_IMMEDIATE64)
    {
        if (ui32TOFlag != TO_FLAG_NONE && !(ui32TOFlag & (TO_FLAG_DESTINATION | TO_FLAG_NAME_ONLY | TO_FLAG_DECLARATION_NAME)))
        {
            SHADER_VARIABLE_TYPE requestedType = TypeFlagsToSVTType(ui32TOFlag);
            const uint32_t swizCount = psOperand->iNumComponents;
            SHADER_VARIABLE_TYPE eType = GetOperandDataType(psContext, psOperand);

            if (!AreTypesCompatible(eType, ui32TOFlag))
            {
                if (CanDoDirectCast(eType, requestedType))
                {
                    bformata(glsl, "%s(", GetConstructorForTypeGLSL(psContext, requestedType, swizCount, false));
                }
                else
                {
                    // Direct cast not possible, need to do bitcast.
                    bformata(glsl, "%s(", GetBitcastOp(eType, requestedType));
                }

                hasConstructor = true;
            }
        } 
    }

    if (ui32TOFlag & TO_FLAG_COPY)
    {
        bcatcstr(glsl, "TempCopy");
        if ((psContext->flags & HLSLCC_FLAG_AVOID_TEMP_REGISTER_ALIASING) == 0)
        {
            SHADER_VARIABLE_TYPE eType = GetOperandDataType(psContext, psOperand);
            switch (eType)
            {
            case SVT_FLOAT:
                break;
            case SVT_INT:
                bcatcstr(glsl, "_int");
                break;
            case SVT_UINT:
                bcatcstr(glsl, "_uint");
                break;
            case SVT_DOUBLE:
                bcatcstr(glsl, "_double");
                break;
            default:
                ASSERT(0);
                break;
            }
        }
    }
    else
    {
        TranslateVariableNameByOperandType(psContext, psOperand, ui32TOFlag, pui32IgnoreSwizzle);
    }

    if (hasConstructor)
    {
        bcatcstr(glsl, ")");
    }
}
SHADER_VARIABLE_TYPE GetOperandDataType(HLSLCrossCompilerContext* psContext, const Operand* psOperand)
{
    if (HavePrecisionQualifers(psContext->psShader->eTargetLanguage))
    {
        // The min precision qualifier overrides all of the stuff below
        switch (psOperand->eMinPrecision)
        {
        case OPERAND_MIN_PRECISION_FLOAT_16:
            return SVT_FLOAT16;
        case OPERAND_MIN_PRECISION_FLOAT_2_8:
            return SVT_FLOAT10;
        case OPERAND_MIN_PRECISION_SINT_16:
            return SVT_INT16;
        case OPERAND_MIN_PRECISION_UINT_16:
            return SVT_UINT16;
        default:
            break;
        }
    }

    switch (psOperand->eType)
    {
    case OPERAND_TYPE_TEMP:
    {
        SHADER_VARIABLE_TYPE eCurrentType = SVT_VOID;
        int i = 0;

        if (psContext->flags & HLSLCC_FLAG_AVOID_TEMP_REGISTER_ALIASING && psContext->psShader->eShaderType != HULL_SHADER)
        {
            return psContext->psShader->aeCommonTempVecType[psOperand->ui32RegisterNumber];
        }

        if (psOperand->eSelMode == OPERAND_4_COMPONENT_SELECT_1_MODE)
        {
            return psOperand->aeDataType[psOperand->aui32Swizzle[0]];
        }
        if (psOperand->eSelMode == OPERAND_4_COMPONENT_SWIZZLE_MODE)
        {
            if (psOperand->ui32Swizzle == (NO_SWIZZLE))
            {
                return psOperand->aeDataType[0];
            }

            return psOperand->aeDataType[psOperand->aui32Swizzle[0]];
        }

        if (psOperand->eSelMode == OPERAND_4_COMPONENT_MASK_MODE)
        {
            uint32_t ui32CompMask = psOperand->ui32CompMask;
            if (!psOperand->ui32CompMask)
            {
                ui32CompMask = OPERAND_4_COMPONENT_MASK_ALL;
            }
            for (; i < 4; ++i)
            {
                if (ui32CompMask & (1 << i))
                {
                    eCurrentType = psOperand->aeDataType[i];
                    break;
                }
            }

#ifdef _DEBUG
            //Check if all elements have the same basic type.
            for (; i < 4; ++i)
            {
                if (psOperand->ui32CompMask & (1 << i))
                {
                    if (eCurrentType != psOperand->aeDataType[i])
                    {
                        ASSERT(0);
                    }
                }
            }
#endif
            return eCurrentType;
        }

        ASSERT(0);

        break;
    }
    case OPERAND_TYPE_OUTPUT:
    {
        const uint32_t ui32Register = psOperand->aui32ArraySizes[psOperand->iIndexDims - 1];
        InOutSignature* psOut;

        if (GetOutputSignatureFromRegister(ui32Register, psOperand->ui32CompMask, 0, &psContext->psShader->sInfo, &psOut))
        {
            if (psOut->eComponentType == INOUT_COMPONENT_UINT32)
            {
                return SVT_UINT;
            }
            else if (psOut->eComponentType == INOUT_COMPONENT_SINT32)
            {
                return SVT_INT;
            }
        }
        break;
    }
    case OPERAND_TYPE_INPUT:
    {
        const uint32_t ui32Register = psOperand->aui32ArraySizes[psOperand->iIndexDims - 1];
        InOutSignature* psIn;

        //UINT in DX, INT in GL.
        if (psOperand->eSpecialName == NAME_PRIMITIVE_ID)
        {
            return SVT_INT;
        }

        if (GetInputSignatureFromRegister(ui32Register, &psContext->psShader->sInfo, &psIn))
        {
            if (psIn->eComponentType == INOUT_COMPONENT_UINT32)
            {
                return SVT_UINT;
            }
            else if (psIn->eComponentType == INOUT_COMPONENT_SINT32)
            {
                return SVT_INT;
            }
        }
        break;
    }
    case OPERAND_TYPE_CONSTANT_BUFFER:
    {
        ConstantBuffer* psCBuf = NULL;
        ShaderVarType* psVarType = NULL;
        int32_t index = -1;
        int32_t rebase = -1;
        int foundVar;
        GetConstantBufferFromBindingPoint(RGROUP_CBUFFER, psOperand->aui32ArraySizes[0], &psContext->psShader->sInfo, &psCBuf);
        if (psCBuf && !psCBuf->blob)
        {
            foundVar = GetShaderVarFromOffset(psOperand->aui32ArraySizes[1], psOperand->aui32Swizzle, psCBuf, &psVarType, &index, &rebase);
            if (foundVar && index == -1 && psOperand->psSubOperand[1] == NULL)
            {
                return psVarType->Type;
            }
        }
        else
        {
            // Todo: this isn't correct yet.
            return SVT_FLOAT;
        }
        break;
    }
    case OPERAND_TYPE_IMMEDIATE32:
    {
        return psOperand->iIntegerImmediate ? SVT_INT : SVT_FLOAT;
    }

    case OPERAND_TYPE_INPUT_THREAD_ID:
    case OPERAND_TYPE_INPUT_THREAD_GROUP_ID:
    case OPERAND_TYPE_INPUT_THREAD_ID_IN_GROUP:
    case OPERAND_TYPE_INPUT_THREAD_ID_IN_GROUP_FLATTENED:
    {
        return SVT_UINT;
    }
    case OPERAND_TYPE_SPECIAL_ADDRESS:
    {
        return SVT_INT;
    }
    default:
    {
        return SVT_FLOAT;
    }
    }

    return SVT_FLOAT;
}

void TranslateOperand(HLSLCrossCompilerContext* psContext, const Operand* psOperand, uint32_t ui32TOFlag)
{
    bstring glsl = *psContext->currentGLSLString;
    uint32_t ui32IgnoreSwizzle = 0;

    if (ui32TOFlag & TO_FLAG_NAME_ONLY)
    {
        TranslateVariableName(psContext, psOperand, ui32TOFlag, &ui32IgnoreSwizzle);
        return;
    }

    switch (psOperand->eModifier)
    {
    case OPERAND_MODIFIER_NONE:
    {
        break;
    }
    case OPERAND_MODIFIER_NEG:
    {
        bcatcstr(glsl, "-");
        break;
    }
    case OPERAND_MODIFIER_ABS:
    {
        bcatcstr(glsl, "abs(");
        break;
    }
    case OPERAND_MODIFIER_ABSNEG:
    {
        bcatcstr(glsl, "-abs(");
        break;
    }
    }

    TranslateVariableName(psContext, psOperand, ui32TOFlag, &ui32IgnoreSwizzle);

    if (!ui32IgnoreSwizzle || IsGmemReservedSlot(FBF_ANY, psOperand->ui32RegisterNumber))
    {
        TranslateOperandSwizzle(psContext, psOperand);
    }

    switch (psOperand->eModifier)
    {
    case OPERAND_MODIFIER_NONE:
    {
        break;
    }
    case OPERAND_MODIFIER_NEG:
    {
        break;
    }
    case OPERAND_MODIFIER_ABS:
    {
        bcatcstr(glsl, ")");
        break;
    }
    case OPERAND_MODIFIER_ABSNEG:
    {
        bcatcstr(glsl, ")");
        break;
    }
    }
}

char ShaderTypePrefix(Shader* psShader)
{
    switch (psShader->eShaderType)
    {
    default:
        ASSERT(0);
    case PIXEL_SHADER:
        return 'p';
    case VERTEX_SHADER:
        return 'v';
    case GEOMETRY_SHADER:
        return 'g';
    case HULL_SHADER:
        return 'h';
    case DOMAIN_SHADER:
        return 'd';
    case COMPUTE_SHADER:
        return 'c';
    }
}

char ResourceGroupPrefix(ResourceGroup eResGroup)
{
    switch (eResGroup)
    {
    default:
        ASSERT(0);
    case RGROUP_CBUFFER:
        return 'c';
    case RGROUP_TEXTURE:
        return 't';
    case RGROUP_SAMPLER:
        return 's';
    case RGROUP_UAV:
        return 'u';
    }
}

void ResourceName(bstring output, Shader* psShader, const char* szName, ResourceGroup eGroup, const char* szSecondaryName, ResourceGroup eSecondaryGroup, uint32_t ui32ArrayOffset, const char* szModifier)
{

    const char* pBracket;

    bconchar(output, ShaderTypePrefix(psShader));
    bcatcstr(output, szModifier);

    bconchar(output, ResourceGroupPrefix(eGroup));
    while ((pBracket = strpbrk(szName, "[]")) != NULL)
    {
        //array syntax [X] becomes _0_
        //Otherwise declarations could end up as:
        //uniform sampler2D SomeTextures[0];
        //uniform sampler2D SomeTextures[1];
        bcatblk(output, (const void*)szName, (int)(pBracket - szName));
        bconchar(output, '_');
        szName = pBracket + 1;
    }
    bcatcstr(output, szName);

    if (ui32ArrayOffset)
    {
        bformata(output, "%d", ui32ArrayOffset);
    }

    if (szSecondaryName != NULL)
    {
        bconchar(output, ResourceGroupPrefix(eSecondaryGroup));
        bcatcstr(output, szSecondaryName);
    }
}

void TextureName(bstring output, Shader* psShader, const uint32_t ui32TextureRegister, const uint32_t ui32SamplerRegister, const int bCompare)
{
    ResourceBinding* psTextureBinding = 0;
    ResourceBinding* psSamplerBinding = 0;
    int found;
    const char* szModifier = bCompare ? "c" : "";

    found = GetResourceFromBindingPoint(RGROUP_TEXTURE, ui32TextureRegister, &psShader->sInfo, &psTextureBinding);
    if (ui32SamplerRegister < MAX_RESOURCE_BINDINGS)
    {
        found &= GetResourceFromBindingPoint(RGROUP_SAMPLER, ui32SamplerRegister, &psShader->sInfo, &psSamplerBinding);
    }

    if (found)
    {
        if (IsGmemReservedSlot(FBF_EXT_COLOR, ui32TextureRegister) || IsGmemReservedSlot(FBF_ARM_COLOR, ui32TextureRegister)) // FRAMEBUFFER FETCH
        {
            int regNum = GetGmemInputResourceSlot(ui32TextureRegister);
            bformata(output, "GMEM_Input%d", regNum);
        }
        else if (IsGmemReservedSlot(FBF_ARM_DEPTH, ui32TextureRegister))
        {
            bcatcstr(output, "GMEM_Depth");
        }
        else if (IsGmemReservedSlot(FBF_ARM_STENCIL, ui32TextureRegister))
        {
            bcatcstr(output, "GMEM_Stencil");
        }
        else
        {
            ResourceName(output, psShader, psTextureBinding->Name, RGROUP_TEXTURE, psSamplerBinding ? psSamplerBinding->Name : NULL, RGROUP_SAMPLER, ui32TextureRegister - psTextureBinding->ui32BindPoint, szModifier);
        }
    }
    else if (ui32SamplerRegister < MAX_RESOURCE_BINDINGS)
    {
        bformata(output, "UnknownTexture%s_%d_%d", szModifier, ui32TextureRegister, ui32SamplerRegister);
    }
    else
    {
        bformata(output, "UnknownTexture%s_%d", szModifier, ui32TextureRegister);
    }
}

void UAVName(bstring output, Shader* psShader, const uint32_t ui32RegisterNumber)
{
    ResourceBinding* psBinding = 0;
    int found;

    found = GetResourceFromBindingPoint(RGROUP_UAV, ui32RegisterNumber, &psShader->sInfo, &psBinding);

    if (found)
    {
        ResourceName(output, psShader, psBinding->Name, RGROUP_UAV, NULL, RGROUP_COUNT, ui32RegisterNumber - psBinding->ui32BindPoint, "");
    }
    else
    {
        bformata(output, "UnknownUAV%d", ui32RegisterNumber);
    }
}

void UniformBufferName(bstring output, Shader* psShader, const uint32_t ui32RegisterNumber)
{
    ResourceBinding* psBinding = 0;
    int found;

    found = GetResourceFromBindingPoint(RGROUP_CBUFFER, ui32RegisterNumber, &psShader->sInfo, &psBinding);

    if (found)
    {
        ResourceName(output, psShader, psBinding->Name, RGROUP_CBUFFER, NULL, RGROUP_COUNT, ui32RegisterNumber - psBinding->ui32BindPoint, "");
    }
    else
    {
        bformata(output, "UnknownUniformBuffer%d", ui32RegisterNumber);
    }
}

void ShaderVarName(bstring output, Shader* psShader, const char* OriginalName)
{
    bconchar(output, ShaderTypePrefix(psShader));
    bcatcstr(output, OriginalName);
}

void ShaderVarFullName(bstring output, Shader* psShader, const ShaderVarType* psShaderVar)
{
    if (psShaderVar->Parent != NULL)
    {
        ShaderVarFullName(output, psShader, psShaderVar->Parent);
        bconchar(output, '.');
    }
    ShaderVarName(output, psShader, psShaderVar->Name);
}

void ConvertToTextureName(bstring output, Shader* psShader, const char* szName, const char* szSamplerName, const int bCompare)
{
    (void)bCompare;

    ResourceName(output, psShader, szName, RGROUP_TEXTURE, szSamplerName, RGROUP_SAMPLER, 0, "");
}

void ConvertToUAVName(bstring output, Shader* psShader, const char* szOriginalUAVName)
{
    ResourceName(output, psShader, szOriginalUAVName, RGROUP_UAV, NULL, RGROUP_COUNT, 0, "");
}

void ConvertToUniformBufferName(bstring output, Shader* psShader, const char* szConstantBufferName)
{
    ResourceName(output, psShader, szConstantBufferName, RGROUP_CBUFFER, NULL, RGROUP_COUNT, 0, "");
}

uint32_t GetGmemInputResourceSlot(uint32_t const slotIn)
{
    if (slotIn == GMEM_ARM_COLOR_SLOT)
    {
        // ARM framebuffer fetch only works with COLOR0
        return 0;
    }
    if (slotIn >= GMEM_FLOAT4_START_SLOT)
    {
        return slotIn - GMEM_FLOAT4_START_SLOT;
    }
    if (slotIn >= GMEM_FLOAT3_START_SLOT)
    {
        return slotIn - GMEM_FLOAT3_START_SLOT;
    }
    if (slotIn >= GMEM_FLOAT2_START_SLOT)
    {
        return slotIn - GMEM_FLOAT2_START_SLOT;
    }
    if (slotIn >= GMEM_FLOAT_START_SLOT)
    {
        return slotIn - GMEM_FLOAT_START_SLOT;
    }
    return slotIn;
}

uint32_t GetGmemInputResourceNumElements(uint32_t const slotIn)
{
    if (slotIn >= GMEM_FLOAT4_START_SLOT)
    {
        return 4;
    }
    if (slotIn >= GMEM_FLOAT3_START_SLOT)
    {
        return 3;
    }
    if (slotIn >= GMEM_FLOAT2_START_SLOT)
    {
        return 2;
    }
    if (slotIn >= GMEM_FLOAT_START_SLOT)
    {
        return 1;
    }
    return 0;
}

void TranslateGmemOperandSwizzleWithMask(HLSLCrossCompilerContext* psContext, const Operand* psOperand, uint32_t ui32ComponentMask, uint32_t gmemNumElements)
{
    // Similar as TranslateOperandSwizzleWithMaskMETAL but need to considerate max # of elements

    bstring metal = *psContext->currentGLSLString;

    if (psOperand->eType == OPERAND_TYPE_INPUT)
    {
        if (psContext->psShader->abScalarInput[psOperand->ui32RegisterNumber])
        {
            return;
        }
    }

    if (psOperand->iWriteMaskEnabled &&
        psOperand->iNumComponents != 1)
    {
        //Component Mask
        if (psOperand->eSelMode == OPERAND_4_COMPONENT_MASK_MODE)
        {
            uint32_t mask;
            if (psOperand->ui32CompMask != 0)
            {
                mask = psOperand->ui32CompMask & ui32ComponentMask;
            }
            else
            {
                mask = ui32ComponentMask;
            }

            if (mask != 0 && mask != OPERAND_4_COMPONENT_MASK_ALL)
            {
                bcatcstr(metal, ".");
                if (mask & OPERAND_4_COMPONENT_MASK_X)
                {
                    bcatcstr(metal, "x");
                }
                if (mask & OPERAND_4_COMPONENT_MASK_Y)
                {
                    if (gmemNumElements < 2)
                    {
                        bcatcstr(metal, "x");
                    }
                    else
                    {
                        bcatcstr(metal, "y");
                    }
                }
                if (mask & OPERAND_4_COMPONENT_MASK_Z)
                {
                    if (gmemNumElements < 3)
                    {
                        bcatcstr(metal, "x");
                    }
                    else
                    {
                        bcatcstr(metal, "z");
                    }
                }
                if (mask & OPERAND_4_COMPONENT_MASK_W)
                {
                    if (gmemNumElements < 4)
                    {
                        bcatcstr(metal, "x");
                    }
                    else
                    {
                        bcatcstr(metal, "w");
                    }
                }
            }
        }
        else
        //Component Swizzle
        if (psOperand->eSelMode == OPERAND_4_COMPONENT_SWIZZLE_MODE)
        {
            if (ui32ComponentMask != OPERAND_4_COMPONENT_MASK_ALL ||
                !(psOperand->aui32Swizzle[0] == OPERAND_4_COMPONENT_X &&
                  psOperand->aui32Swizzle[1] == OPERAND_4_COMPONENT_Y &&
                  psOperand->aui32Swizzle[2] == OPERAND_4_COMPONENT_Z &&
                  psOperand->aui32Swizzle[3] == OPERAND_4_COMPONENT_W
                  )
                )
            {
                uint32_t i;

                bcatcstr(metal, ".");

                for (i = 0; i < 4; ++i)
                {
                    if (!(ui32ComponentMask & (OPERAND_4_COMPONENT_MASK_X << i)))
                    {
                        continue;
                    }

                    if (psOperand->aui32Swizzle[i] == OPERAND_4_COMPONENT_X)
                    {
                        bcatcstr(metal, "x");
                    }
                    else if (psOperand->aui32Swizzle[i] == OPERAND_4_COMPONENT_Y)
                    {
                        if (gmemNumElements < 2)
                        {
                            bcatcstr(metal, "x");
                        }
                        else
                        {
                            bcatcstr(metal, "y");
                        }
                    }
                    else if (psOperand->aui32Swizzle[i] == OPERAND_4_COMPONENT_Z)
                    {
                        if (gmemNumElements < 3)
                        {
                            bcatcstr(metal, "x");
                        }
                        else
                        {
                            bcatcstr(metal, "z");
                        }
                    }
                    else if (psOperand->aui32Swizzle[i] == OPERAND_4_COMPONENT_W)
                    {
                        if (gmemNumElements < 4)
                        {
                            bcatcstr(metal, "x");
                        }
                        else
                        {
                            bcatcstr(metal, "w");
                        }
                    }
                }
            }
        }
        else
        if (psOperand->eSelMode == OPERAND_4_COMPONENT_SELECT_1_MODE)         // ui32ComponentMask is ignored in this case
        {
            bcatcstr(metal, ".");

            if (psOperand->aui32Swizzle[0] == OPERAND_4_COMPONENT_X)
            {
                bcatcstr(metal, "x");
            }
            else
            if (psOperand->aui32Swizzle[0] == OPERAND_4_COMPONENT_Y)
            {
                if (gmemNumElements < 2)
                {
                    bcatcstr(metal, "x");
                }
                else
                {
                    bcatcstr(metal, "y");
                }
            }
            else
            if (psOperand->aui32Swizzle[0] == OPERAND_4_COMPONENT_Z)
            {
                if (gmemNumElements < 3)
                {
                    bcatcstr(metal, "x");
                }
                else
                {
                    bcatcstr(metal, "z");
                }
            }
            else
            if (psOperand->aui32Swizzle[0] == OPERAND_4_COMPONENT_W)
            {
                if (gmemNumElements < 4)
                {
                    bcatcstr(metal, "x");
                }
                else
                {
                    bcatcstr(metal, "w");
                }
            }
        }

        //Component Select 1
    }
}
