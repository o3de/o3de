// Modifications copyright Amazon.com, Inc. or its affiliates
// Modifications copyright Crytek GmbH

#include "internal_includes/toMETALOperand.h"
#include "internal_includes/toMETALDeclaration.h"
#include "bstrlib.h"
#include "hlslcc.h"
#include "internal_includes/debug.h"

#include <float.h>
#include <stdlib.h>

#ifdef _MSC_VER
#define isnan(x) _isnan(x)
#define isinf(x) (!_finite(x))
#endif

#define fpcheck(x) (isnan(x) || isinf(x))
#define MAX_STR_LENGTH 128

extern void AddIndentation(HLSLCrossCompilerContext* psContext);

uint32_t SVTTypeToFlagMETAL(const SHADER_VARIABLE_TYPE eType)
{
    if (eType == SVT_UINT)
    {
        return TO_FLAG_UNSIGNED_INTEGER;
    }
    else if (eType == SVT_INT)
    {
        return TO_FLAG_INTEGER;
    }
    else if (eType == SVT_BOOL)
    {
        return TO_FLAG_INTEGER; // TODO bools?
    }
    else if (eType == SVT_FLOAT16)
    {
        return TO_FLAG_FLOAT16;
    }
    else
    {
        return TO_FLAG_NONE;
    }
}

SHADER_VARIABLE_TYPE TypeFlagsToSVTTypeMETAL(const uint32_t typeflags)
{
    if (typeflags & (TO_FLAG_INTEGER | TO_AUTO_BITCAST_TO_INT))
    {
        return SVT_INT;
    }
    if (typeflags & (TO_FLAG_UNSIGNED_INTEGER | TO_AUTO_BITCAST_TO_UINT))
    {
        return SVT_UINT;
    }
    if (typeflags & (TO_FLAG_FLOAT16 | TO_AUTO_BITCAST_TO_FLOAT16))
    {
        return SVT_FLOAT16;
    }
    return SVT_FLOAT;
}

uint32_t GetOperandWriteMaskMETAL(const Operand* psOperand)
{
    if (psOperand->eSelMode != OPERAND_4_COMPONENT_MASK_MODE || psOperand->ui32CompMask == 0)
    {
        return OPERAND_4_COMPONENT_MASK_ALL;
    }

    return psOperand->ui32CompMask;
}


const char* GetConstructorForTypeMETAL(const SHADER_VARIABLE_TYPE eType,
    const int components)
{
    static const char* const uintTypes[] = { " ", "uint", "uint2", "uint3", "uint4" };
    static const char* const intTypes[] = { " ", "int", "int2", "int3", "int4" };
    static const char* const floatTypes[] = { " ", "float", "float2", "float3", "float4" };
    static const char* const float16Types[] = { " ", "half", "half2", "half3", "half4" };

    if (components < 1 || components > 4)
    {
        return "ERROR TOO MANY COMPONENTS IN VECTOR";
    }

    switch (eType)
    {
    case SVT_UINT:
        return uintTypes[components];
    case SVT_INT:
        return intTypes[components];
    case SVT_FLOAT:
        return floatTypes[components];
    case SVT_FLOAT16:
        return float16Types[components];
    default:
        return "ERROR UNSUPPORTED TYPE";
    }
}


const char* GetConstructorForTypeFlagMETAL(const uint32_t ui32Flag,
    const int components)
{
    if (ui32Flag & TO_FLAG_UNSIGNED_INTEGER || ui32Flag & TO_AUTO_BITCAST_TO_UINT)
    {
        return GetConstructorForTypeMETAL(SVT_UINT, components);
    }
    else if (ui32Flag & TO_FLAG_INTEGER || ui32Flag & TO_AUTO_BITCAST_TO_INT)
    {
        return GetConstructorForTypeMETAL(SVT_INT, components);
    }
    else
    {
        return GetConstructorForTypeMETAL(SVT_FLOAT, components);
    }
}

int GetMaxComponentFromComponentMaskMETAL(const Operand* psOperand)
{
    if (psOperand->iWriteMaskEnabled &&
        psOperand->iNumComponents == 4)
    {
        //Component Mask
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
uint32_t IsSwizzleReplicatedMETAL(const Operand* psOperand)
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

static uint32_t METALGetNumberBitsSet(uint32_t a)
{
    // Calculate number of bits in a
    // Taken from https://graphics.stanford.edu/~seander/bithacks.html#CountBitsSet64
    // Works only up to 14 bits (we're only using up to 4)
    return (a * 0x200040008001ULL & 0x111111111111111ULL) % 0xf;
}

//e.g.
//.z = 1
//.x = 1
//.yw = 2
uint32_t GetNumSwizzleElementsMETAL(const Operand* psOperand)
{
    return GetNumSwizzleElementsWithMaskMETAL(psOperand, OPERAND_4_COMPONENT_MASK_ALL);
}

// Get the number of elements returned by operand, taking additional component mask into account
uint32_t GetNumSwizzleElementsWithMaskMETAL(const Operand* psOperand, uint32_t ui32CompMask)
{
    uint32_t count = 0;

    switch (psOperand->eType)
    {
    case OPERAND_TYPE_INPUT_THREAD_ID_IN_GROUP_FLATTENED:
        return 1;     // TODO: does mask make any sense here?
    case OPERAND_TYPE_INPUT_THREAD_ID_IN_GROUP:
    case OPERAND_TYPE_INPUT_THREAD_ID:
    case OPERAND_TYPE_INPUT_THREAD_GROUP_ID:
        // Adjust component count and break to more processing
        ((Operand*)psOperand)->iNumComponents = 3;
        break;
    case OPERAND_TYPE_IMMEDIATE32:
    case OPERAND_TYPE_IMMEDIATE64:
    case OPERAND_TYPE_OUTPUT_DEPTH_GREATER_EQUAL:
    case OPERAND_TYPE_OUTPUT_DEPTH_LESS_EQUAL:
    case OPERAND_TYPE_OUTPUT_DEPTH:
    {
        // Translate numComponents into bitmask
        // 1 -> 1, 2 -> 3, 3 -> 7 and 4 -> 15
        uint32_t compMask = (1 << psOperand->iNumComponents) - 1;

        compMask &= ui32CompMask;
        // Calculate bits left in compMask
        return METALGetNumberBitsSet(compMask);
    }
    default:
    {
        break;
    }
    }

    if (psOperand->iWriteMaskEnabled &&
        psOperand->iNumComponents != 1)
    {
        //Component Mask
        if (psOperand->eSelMode == OPERAND_4_COMPONENT_MASK_MODE)
        {
            uint32_t compMask = psOperand->ui32CompMask;
            if (compMask == 0)
            {
                compMask = OPERAND_4_COMPONENT_MASK_ALL;
            }
            compMask &= ui32CompMask;

            if (compMask == OPERAND_4_COMPONENT_MASK_ALL)
            {
                return 4;
            }

            if (compMask & OPERAND_4_COMPONENT_MASK_X)
            {
                count++;
            }
            if (compMask & OPERAND_4_COMPONENT_MASK_Y)
            {
                count++;
            }
            if (compMask & OPERAND_4_COMPONENT_MASK_Z)
            {
                count++;
            }
            if (compMask & OPERAND_4_COMPONENT_MASK_W)
            {
                count++;
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
                    if ((ui32CompMask & (1 << i)) == 0)
                    {
                        continue;
                    }

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
        // Translate numComponents into bitmask
        // 1 -> 1, 2 -> 3, 3 -> 7 and 4 -> 15
        uint32_t compMask = (1 << psOperand->iNumComponents) - 1;

        compMask &= ui32CompMask;
        // Calculate bits left in compMask
        return METALGetNumberBitsSet(compMask);
    }

    return count;
}

void AddSwizzleUsingElementCountMETAL(HLSLCrossCompilerContext* psContext, uint32_t count)
{
    bstring metal = *psContext->currentShaderString;
    if (count == 4)
    {
        return;
    }
    if (count)
    {
        bcatcstr(metal, ".");
        bcatcstr(metal, "x");
        count--;
    }
    if (count)
    {
        bcatcstr(metal, "y");
        count--;
    }
    if (count)
    {
        bcatcstr(metal, "z");
        count--;
    }
    if (count)
    {
        bcatcstr(metal, "w");
        count--;
    }
}

static uint32_t METALConvertOperandSwizzleToComponentMask(const Operand* psOperand)
{
    uint32_t mask = 0;

    if (psOperand->iWriteMaskEnabled &&
        psOperand->iNumComponents == 4)
    {
        //Component Mask
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
int CompareOperandSwizzlesMETAL(const Operand* psOperandA, const Operand* psOperandB)
{
    uint32_t maskA = METALConvertOperandSwizzleToComponentMask(psOperandA);
    uint32_t maskB = METALConvertOperandSwizzleToComponentMask(psOperandB);

    return maskA & maskB;
}


void TranslateOperandSwizzleMETAL(HLSLCrossCompilerContext* psContext, const Operand* psOperand)
{
    TranslateOperandSwizzleWithMaskMETAL(psContext, psOperand, OPERAND_4_COMPONENT_MASK_ALL);
}

void TranslateOperandSwizzleWithMaskMETAL(HLSLCrossCompilerContext* psContext, const Operand* psOperand, uint32_t ui32ComponentMask)
{
    bstring metal = *psContext->currentShaderString;

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

        bformata(metal, ".%s", psVar->Name);
        if(index != -1)
        {
            bformata(metal, "[%d]", index);
        }*/

        //return;
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
                    bcatcstr(metal, "y");
                }
                if (mask & OPERAND_4_COMPONENT_MASK_Z)
                {
                    bcatcstr(metal, "z");
                }
                if (mask & OPERAND_4_COMPONENT_MASK_W)
                {
                    bcatcstr(metal, "w");
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
                        bcatcstr(metal, "y");
                    }
                    else if (psOperand->aui32Swizzle[i] == OPERAND_4_COMPONENT_Z)
                    {
                        bcatcstr(metal, "z");
                    }
                    else if (psOperand->aui32Swizzle[i] == OPERAND_4_COMPONENT_W)
                    {
                        bcatcstr(metal, "w");
                    }
                }
            }
        }
        else
        if (psOperand->eSelMode == OPERAND_4_COMPONENT_SELECT_1_MODE) // ui32ComponentMask is ignored in this case
        {
            bcatcstr(metal, ".");

            if (psOperand->aui32Swizzle[0] == OPERAND_4_COMPONENT_X)
            {
                bcatcstr(metal, "x");
            }
            else
            if (psOperand->aui32Swizzle[0] == OPERAND_4_COMPONENT_Y)
            {
                bcatcstr(metal, "y");
            }
            else
            if (psOperand->aui32Swizzle[0] == OPERAND_4_COMPONENT_Z)
            {
                bcatcstr(metal, "z");
            }
            else
            if (psOperand->aui32Swizzle[0] == OPERAND_4_COMPONENT_W)
            {
                bcatcstr(metal, "w");
            }
        }

        //Component Select 1
    }
}

void TranslateGmemOperandSwizzleWithMaskMETAL(HLSLCrossCompilerContext* psContext, const Operand* psOperand, uint32_t ui32ComponentMask, uint32_t gmemNumElements)
{
    // Similar as TranslateOperandSwizzleWithMaskMETAL but need to considerate max # of elements

    bstring metal = *psContext->currentShaderString;

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

int GetFirstOperandSwizzleMETAL(HLSLCrossCompilerContext* psContext, const Operand* psOperand)
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
        //Component Mask
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

void TranslateOperandIndexMETAL(HLSLCrossCompilerContext* psContext, const Operand* psOperand, int index)
{
    int i = index;

    bstring metal = *psContext->currentShaderString;

    ASSERT(index < psOperand->iIndexDims);

    switch (psOperand->eIndexRep[i])
    {
    case OPERAND_INDEX_IMMEDIATE32:
    {
        if (i > 0)
        {
            bformata(metal, "[%d]", psOperand->aui32ArraySizes[i]);
        }
        else
        {
            bformata(metal, "%d", psOperand->aui32ArraySizes[i]);
        }
        break;
    }
    case OPERAND_INDEX_RELATIVE:
    {
        bcatcstr(metal, "[");
        TranslateOperandMETAL(psContext, psOperand->psSubOperand[i], TO_FLAG_INTEGER);
        bcatcstr(metal, "]");
        break;
    }
    case OPERAND_INDEX_IMMEDIATE32_PLUS_RELATIVE:
    {
        bcatcstr(metal, "[");     //Indexes must be integral.
        TranslateOperandMETAL(psContext, psOperand->psSubOperand[i], TO_FLAG_INTEGER);
        bformata(metal, " + %d]", psOperand->aui32ArraySizes[i]);
        break;
    }
    default:
    {
        break;
    }
    }
}

void TranslateOperandIndexMADMETAL(HLSLCrossCompilerContext* psContext, const Operand* psOperand, int index, uint32_t multiply, uint32_t add)
{
    int i = index;

    bstring metal = *psContext->currentShaderString;

    ASSERT(index < psOperand->iIndexDims);

    switch (psOperand->eIndexRep[i])
    {
    case OPERAND_INDEX_IMMEDIATE32:
    {
        if (i > 0)
        {
            bformata(metal, "[%d*%d+%d]", psOperand->aui32ArraySizes[i], multiply, add);
        }
        else
        {
            bformata(metal, "%d*%d+%d", psOperand->aui32ArraySizes[i], multiply, add);
        }
        break;
    }
    case OPERAND_INDEX_RELATIVE:
    {
        bcatcstr(metal, "[int(");     //Indexes must be integral.
        TranslateOperandMETAL(psContext, psOperand->psSubOperand[i], TO_FLAG_NONE);
        bformata(metal, ")*%d+%d]", multiply, add);
        break;
    }
    case OPERAND_INDEX_IMMEDIATE32_PLUS_RELATIVE:
    {
        bcatcstr(metal, "[(int(");     //Indexes must be integral.
        TranslateOperandMETAL(psContext, psOperand->psSubOperand[i], TO_FLAG_NONE);
        bformata(metal, ") + %d)*%d+%d]", psOperand->aui32ArraySizes[i], multiply, add);
        break;
    }
    default:
    {
        break;
    }
    }
}

// Returns nonzero if a direct constructor can convert src->dest
static int METALCanDoDirectCast( SHADER_VARIABLE_TYPE src, SHADER_VARIABLE_TYPE dest)
{
    // uint<->int<->bool conversions possible
    if ((src == SVT_INT || src == SVT_UINT || src == SVT_BOOL) && (dest == SVT_INT || dest == SVT_UINT || dest == SVT_BOOL))
    {
        return 1;
    }

    // float<->double possible
    if ((src == SVT_FLOAT || src == SVT_DOUBLE) && (dest == SVT_FLOAT || dest == SVT_DOUBLE))
    {
        return 1;
    }

    return 0;
}

// Returns true if one of the src or dest is half float while the other is not
static int IsHalfFloatCastNeeded(SHADER_VARIABLE_TYPE src, SHADER_VARIABLE_TYPE dest)
{
    // uint<->int<->bool conversions possible
    if ((src == SVT_FLOAT16) && (dest != SVT_FLOAT16))
    {
        return 1;
    }

    // float<->double possible
    if ((src != SVT_FLOAT16) && (dest == SVT_FLOAT16))
    {
        return 1;
    }

    return 0;
}

static const char* GetOpDestType(SHADER_VARIABLE_TYPE to)
{
    switch (to)
    {
    case SVT_FLOAT:
        return "float";
        break;
    case SVT_FLOAT16:
        return "half";
        break;
    case SVT_INT:
        return "int";
        break;
    case SVT_UINT:
        return "uint";
        break;
    default:
        ASSERT(0);
        return "";
    }
}

static const char* GetOpCastType(SHADER_VARIABLE_TYPE from, SHADER_VARIABLE_TYPE to)
{
    if (to == SVT_FLOAT && (from == SVT_INT || from == SVT_UINT))
    {
        return "as_type";
    }
    else if (to == SVT_INT && (from == SVT_FLOAT || from == SVT_UINT))
    {
        return "as_type";
    }
    else if (to == SVT_UINT && (from == SVT_FLOAT || from == SVT_INT))
    {
        return "as_type";
    }
    
    ASSERT(0);
    return "ERROR missing components in GetBitcastOp()";
}

// Helper function to print out a single 32-bit immediate value in desired format
static void METALprintImmediate32(HLSLCrossCompilerContext* psContext, uint32_t value, SHADER_VARIABLE_TYPE eType)
{
    bstring metal = *psContext->currentShaderString;
    int needsParenthesis = 0;

    if (eType == SVT_FLOAT || eType == SVT_FLOAT16)
    {
        // Print floats as bit patterns.
        switch (eType)
        {
        case SVT_FLOAT:
            bcatcstr(metal, "as_type<float>(");
            break;
        case SVT_FLOAT16:
            bcatcstr(metal, "static_cast<half>(");
            break;
        }
        
        eType = SVT_INT;
        needsParenthesis = 1;
    }
    


    switch (eType)
    {
    default:
    case SVT_INT:
        // Need special handling for anything >= uint 0x3fffffff
        if (value > 0x3ffffffe)
        {
            bformata(metal, "int(0x%Xu)", value);
        }
        else
        {
            bformata(metal, "0x%X", value);
        }
        break;
    case SVT_UINT:
        bformata(metal, "%uu", value);
        break;
    case SVT_FLOAT:
        bformata(metal, "%f", *((float*)(&value)));
        break;
    }
    if (needsParenthesis)
    {
        bcatcstr(metal, ")");
    }
}

static void METALMETALTranslateVariableNameWithMask(HLSLCrossCompilerContext* psContext, const Operand* psOperand, uint32_t ui32TOFlag, uint32_t* pui32IgnoreSwizzle, uint32_t ui32CompMask)
{
    int numParenthesis = 0;
    int hasCtor = 0;
    bstring metal = *psContext->currentShaderString;
    SHADER_VARIABLE_TYPE requestedType = TypeFlagsToSVTTypeMETAL(ui32TOFlag);
    SHADER_VARIABLE_TYPE eType = GetOperandDataTypeExMETAL(psContext, psOperand, requestedType);
    int numComponents = GetNumSwizzleElementsWithMaskMETAL(psOperand, ui32CompMask);
    int requestedComponents = 0;

    if (ui32TOFlag & TO_AUTO_EXPAND_TO_VEC2)
    {
        requestedComponents = 2;
    }
    else if (ui32TOFlag & TO_AUTO_EXPAND_TO_VEC3)
    {
        requestedComponents = 3;
    }
    else if (ui32TOFlag & TO_AUTO_EXPAND_TO_VEC4)
    {
        requestedComponents = 4;
    }

    requestedComponents = max(requestedComponents, numComponents);

    *pui32IgnoreSwizzle = 0;


    if (!(ui32TOFlag & (TO_FLAG_DESTINATION | TO_FLAG_NAME_ONLY | TO_FLAG_DECLARATION_NAME)))
    {
        if (psOperand->eType == OPERAND_TYPE_IMMEDIATE32 || psOperand->eType == OPERAND_TYPE_IMMEDIATE64)
        {
            // Mark the operand type to match whatever we're asking for in the flags.
            ((Operand*)psOperand)->aeDataType[0] = requestedType;
            ((Operand*)psOperand)->aeDataType[1] = requestedType;
            ((Operand*)psOperand)->aeDataType[2] = requestedType;
            ((Operand*)psOperand)->aeDataType[3] = requestedType;
        }

        if (eType != requestedType)
        {
            if (METALCanDoDirectCast(eType, requestedType))
            {
                bformata(metal, "%s(", GetConstructorForTypeMETAL(requestedType, requestedComponents));               
                hasCtor = 1;
            }
            else if (IsHalfFloatCastNeeded(eType, requestedType))
            {
                // half float static cast needed
                if (requestedComponents > 1)
                {
                    bformata(metal, "static_cast<%s%i>(", GetOpDestType(requestedType), requestedComponents);
                }
                else
                {
                    bformata(metal, "static_cast<%s>(", GetOpDestType(requestedType));
                }                
            }
            else
            {
                // Direct cast not possible, need to do bitcast.
                if (requestedComponents > 1)
                {
                    bformata(metal, "%s<%s%i>(", GetOpCastType(eType, requestedType), GetOpDestType(requestedType), requestedComponents);
                }
                else
                {
                    bformata(metal, "%s<%s>(", GetOpCastType(eType, requestedType), GetOpDestType(requestedType));
                }               
            }
            numParenthesis++;
        }

        // Add ctor if needed (upscaling)
        if (numComponents < requestedComponents && (hasCtor == 0))
        {
            ASSERT(numComponents == 1);
            bformata(metal, "%s(", GetConstructorForTypeMETAL(requestedType, requestedComponents));
            numParenthesis++;
            hasCtor = 1;
        }
    }


    switch (psOperand->eType)
    {
    case OPERAND_TYPE_IMMEDIATE32:
    {
        if (psOperand->iNumComponents == 1)
        {
            METALprintImmediate32(psContext, *((unsigned int*)(&psOperand->afImmediates[0])), requestedType);
        }
        else
        {
            int i;
            int firstItemAdded = 0;
            if (hasCtor == 0)
            {
                bformata(metal, "%s(", GetConstructorForTypeMETAL(requestedType, numComponents));
                numParenthesis++;
                hasCtor = 1;
            }
            for (i = 0; i < 4; i++)
            {
                uint32_t uval;
                if (!(ui32CompMask & (1 << i)))
                {
                    continue;
                }

                if (firstItemAdded)
                {
                    bcatcstr(metal, ", ");
                }
                uval = *((uint32_t*)(&psOperand->afImmediates[i]));
                METALprintImmediate32(psContext, uval, requestedType);
                firstItemAdded = 1;
            }
            bcatcstr(metal, ")");
            *pui32IgnoreSwizzle = 1;
            numParenthesis--;
        }
        break;
    }
    case OPERAND_TYPE_IMMEDIATE64:
    {
        if (psOperand->iNumComponents == 1)
        {
            bformata(metal, "%f",
                psOperand->adImmediates[0]);
        }
        else
        {
            bformata(metal, "float4(%f, %f, %f, %f)",
                psOperand->adImmediates[0],
                psOperand->adImmediates[1],
                psOperand->adImmediates[2],
                psOperand->adImmediates[3]);
            if (psOperand->iNumComponents != 4)
            {
                AddSwizzleUsingElementCountMETAL(psContext, psOperand->iNumComponents);
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
                bcatcstr(metal, "stageIn");
                TranslateOperandIndexMETAL(psContext, psOperand, 0);        //Vertex index
                bcatcstr(metal, ".position");
            }
            else
            {
                const char* name = "Input";
                if (ui32TOFlag & TO_FLAG_DECLARATION_NAME)
                {
                    name = GetDeclaredInputNameMETAL(psContext, psContext->psShader->eShaderType, psOperand);
                }

                bformata(metal, "%s%d", name, psOperand->aui32ArraySizes[1]);
                TranslateOperandIndexMETAL(psContext, psOperand, 0);        //Vertex index
            }
            break;
        }
        default:
        {
            if (psOperand->eIndexRep[0] == OPERAND_INDEX_IMMEDIATE32_PLUS_RELATIVE)
            {
                bformata(metal, "Input%d[", psOperand->ui32RegisterNumber);
                TranslateOperandMETAL(psContext, psOperand->psSubOperand[0], TO_FLAG_INTEGER);
                bcatcstr(metal, "]");
            }
            else
            {
                if (psContext->psShader->aIndexedInput[psOperand->ui32RegisterNumber] != 0)
                {
                    const uint32_t parentIndex = psContext->psShader->aIndexedInputParents[psOperand->ui32RegisterNumber];
                    bformata(metal, "Input%d[%d]", parentIndex,
                        psOperand->ui32RegisterNumber - parentIndex);
                }
                else
                {
                    if (ui32TOFlag & TO_FLAG_DECLARATION_NAME)
                    {
                        const char* name = GetDeclaredInputNameMETAL(psContext, psContext->psShader->eShaderType, psOperand);
                        bcatcstr(metal, name);
                    }
                    else
                    {
                        bformata(metal, "Input%d", psOperand->ui32RegisterNumber);
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
        bformata(metal, "Output%d", psOperand->ui32RegisterNumber);
        if (psOperand->psSubOperand[0])
        {
            bcatcstr(metal, "[");
            TranslateOperandMETAL(psContext, psOperand->psSubOperand[0], TO_AUTO_BITCAST_TO_INT);
            bcatcstr(metal, "]");
        }
        break;
    }
    case OPERAND_TYPE_OUTPUT_DEPTH:
    {
        bcatcstr(metal, "DepthAny");
        break;
    }
    case OPERAND_TYPE_OUTPUT_DEPTH_GREATER_EQUAL:
    {
        bcatcstr(metal, "DepthGreater");
        break;
    }
    case OPERAND_TYPE_OUTPUT_DEPTH_LESS_EQUAL:
    {
        bcatcstr(metal, "DepthLess");
        break;
    }
    case OPERAND_TYPE_TEMP:
    {
        SHADER_VARIABLE_TYPE eType2 = GetOperandDataTypeMETAL(psContext, psOperand);
        bcatcstr(metal, "Temp");

        if (eType2 == SVT_INT)
        {
            bcatcstr(metal, "_int");
        }
        else if (eType2 == SVT_UINT)
        {
            bcatcstr(metal, "_uint");
        }
        else if (eType2 == SVT_DOUBLE)
        {
            bcatcstr(metal, "_double");
        }
        else if (eType2 == SVT_FLOAT16)
        {
            bcatcstr(metal, "_half");
        }
        else if (eType2 == SVT_VOID &&
                 (ui32TOFlag & TO_FLAG_DESTINATION))
        {
            ASSERT(0 && "Should never get here!");
            /*                if(ui32TOFlag & TO_FLAG_INTEGER)
                            {
                                bcatcstr(metal, "_int");
                            }
                            else
                            if(ui32TOFlag & TO_FLAG_UNSIGNED_INTEGER)
                            {
                                bcatcstr(metal, "_uint");
                            }*/
        }

        bformata(metal, "[%d]", psOperand->ui32RegisterNumber);

        break;
    }
    case OPERAND_TYPE_SPECIAL_IMMCONSTINT:
    {
        bformata(metal, "IntImmConst%d", psOperand->ui32RegisterNumber);
        break;
    }
    case OPERAND_TYPE_SPECIAL_IMMCONST:
    {
        if (psOperand->psSubOperand[0] != NULL)
        {
            if (psContext->psShader->aui32Dx9ImmConstArrayRemap[psOperand->ui32RegisterNumber] != 0)
            {
                bformata(metal, "ImmConstArray[%d + ", psContext->psShader->aui32Dx9ImmConstArrayRemap[psOperand->ui32RegisterNumber]);
            }
            else
            {
                bcatcstr(metal, "ImmConstArray[");
            }
            TranslateOperandWithMaskMETAL(psContext, psOperand->psSubOperand[0], TO_FLAG_INTEGER, OPERAND_4_COMPONENT_MASK_X);
            bcatcstr(metal, "]");
        }
        else
        {
            bformata(metal, "ImmConst%d", psOperand->ui32RegisterNumber);
        }
        break;
    }
    case OPERAND_TYPE_SPECIAL_OUTBASECOLOUR:
    {
        bcatcstr(metal, "BaseColour");
        break;
    }
    case OPERAND_TYPE_SPECIAL_OUTOFFSETCOLOUR:
    {
        bcatcstr(metal, "OffsetColour");
        break;
    }
    case OPERAND_TYPE_SPECIAL_POSITION:
    {
        switch (psContext->psShader->eShaderType)
        {
        case PIXEL_SHADER:
        {
            if ((ui32TOFlag & TO_FLAG_DECLARATION_NAME) != TO_FLAG_DECLARATION_NAME)
            {
                bcatcstr(metal, "stageIn.");
            }
            bcatcstr(metal, "position");
            break;
        }
        case VERTEX_SHADER:
        {
            if ((ui32TOFlag & TO_FLAG_DECLARATION_NAME) != TO_FLAG_DECLARATION_NAME)
            {
                bcatcstr(metal, "output.");
            }
            bcatcstr(metal, "position");
            break;
        }
        default:
        {
            break;
        }
        }
        break;
    }
    case OPERAND_TYPE_SPECIAL_FOG:
    {
        bcatcstr(metal, "Fog");
        break;
    }
    case OPERAND_TYPE_SPECIAL_POINTSIZE:
    {
        switch (psContext->psShader->eShaderType)
        {
        case PIXEL_SHADER:
        {
            if ((ui32TOFlag & TO_FLAG_DECLARATION_NAME) != TO_FLAG_DECLARATION_NAME)
            {
                bcatcstr(metal, "stageIn.");
            }
            bcatcstr(metal, "pointSize");
            break;
        }
        case VERTEX_SHADER:
        {
            if ((ui32TOFlag & TO_FLAG_DECLARATION_NAME) != TO_FLAG_DECLARATION_NAME)
            {
                bcatcstr(metal, "output.");
            }
            bcatcstr(metal, "pointSize");
            break;
        }
        default:
        {
            break;
        }
        }
        break;
    }
    case OPERAND_TYPE_SPECIAL_ADDRESS:
    {
        bcatcstr(metal, "Address");
        break;
    }
    case OPERAND_TYPE_SPECIAL_LOOPCOUNTER:
    {
        bcatcstr(metal, "LoopCounter");
        pui32IgnoreSwizzle[0] = 1;
        break;
    }
    case OPERAND_TYPE_SPECIAL_TEXCOORD:
    {
        bformata(metal, "TexCoord%d", psOperand->ui32RegisterNumber);
        break;
    }
    case OPERAND_TYPE_CONSTANT_BUFFER:
    {
        const char* StageName = "VS";
        ConstantBuffer* psCBuf = NULL;
        ShaderVarType* psVarType = NULL;
        int32_t index = -1;
        GetConstantBufferFromBindingPoint(RGROUP_CBUFFER, psOperand->aui32ArraySizes[0], &psContext->psShader->sInfo, &psCBuf);

        switch (psContext->psShader->eShaderType)
        {
        case PIXEL_SHADER:
        {
            StageName = "PS";
            break;
        }
        ////////////////////// FOLLOWING SHOULDN'T HIT IN METAL AS IT'S NOT SUPPORTED //////////////////////////////////////////
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
        ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
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

        if (ui32TOFlag & TO_FLAG_DECLARATION_NAME)
        {
            pui32IgnoreSwizzle[0] = 1;
        }

        // FIXME: With ES 3.0 the buffer name is often not prepended to variable names
        if (((psContext->flags & HLSLCC_FLAG_UNIFORM_BUFFER_OBJECT) != HLSLCC_FLAG_UNIFORM_BUFFER_OBJECT) &&
            ((psContext->flags & HLSLCC_FLAG_DISABLE_GLOBALS_STRUCT) != HLSLCC_FLAG_DISABLE_GLOBALS_STRUCT))
        {
            if (psCBuf)
            {
                //$Globals.
                if (psCBuf->Name[0] == '$')
                {
                    bformata(metal, "Globals%s", StageName);
                }
                else
                {
                    bformata(metal, "%s%s", psCBuf->Name, StageName);
                }
                if ((ui32TOFlag & TO_FLAG_DECLARATION_NAME) != TO_FLAG_DECLARATION_NAME)
                {
                    bcatcstr(metal, ".");
                }
            }
            else
            {
                //bformata(metal, "cb%d", psOperand->aui32ArraySizes[0]);
            }
        }

        if ((ui32TOFlag & TO_FLAG_DECLARATION_NAME) != TO_FLAG_DECLARATION_NAME)
        {
            //Work out the variable name. Don't apply swizzle to that variable yet.
            int32_t rebase = 0;

            if (psCBuf && !psCBuf->blob)
            {
                GetShaderVarFromOffset(psOperand->aui32ArraySizes[1], psOperand->aui32Swizzle, psCBuf, &psVarType, &index, &rebase);

                bformata(metal, "%s", psVarType->FullName);
            }
            else if (psCBuf)
            {
                bformata(metal, "%s%s_data", psCBuf->Name, StageName);
                if (psContext->psShader->eShaderType == PIXEL_SHADER)
                {
                    bformata(metal, ".%s", psCBuf->asVars->Name);
                }
                else if (psContext->psShader->eShaderType == VERTEX_SHADER)
                {
                    bformata(metal, "->%s", psCBuf->asVars->Name);
                }
                else
                {
                    ASSERT(0);
                }
                index = psOperand->aui32ArraySizes[1];
            }
            else     // We don't have a semantic for this variable, so try the raw dump appoach.
            {
                bformata(metal, "cb%d.data", psOperand->aui32ArraySizes[0]);    //
                index = psOperand->aui32ArraySizes[1];
            }

            //Dx9 only?
            if (psOperand->psSubOperand[0] != NULL)
            {
                // Array of matrices is treated as array of vec4s in HLSL,
                // but that would mess up uniform types in metal. Do gymnastics.
                SHADER_VARIABLE_TYPE eType2 = GetOperandDataTypeMETAL(psContext, psOperand->psSubOperand[0]);
                uint32_t opFlags = TO_FLAG_INTEGER;

                if (psVarType && (psVarType->Class == SVC_MATRIX_COLUMNS || psVarType->Class == SVC_MATRIX_ROWS) && (psVarType->Elements > 1))
                {
                    // Special handling for matrix arrays
                    bcatcstr(metal, "[(");
                    TranslateOperandMETAL(psContext, psOperand->psSubOperand[0], opFlags);
                    bformata(metal, ") / 4]");
                    if (psContext->psShader->eTargetLanguage <= LANG_120)
                    {
                        bcatcstr(metal, "[int(mod(float(");
                        TranslateOperandWithMaskMETAL(psContext, psOperand->psSubOperand[0], opFlags, OPERAND_4_COMPONENT_MASK_X);
                        bformata(metal, "), 4.0))]");
                    }
                    else
                    {
                        bcatcstr(metal, "[((");
                        TranslateOperandWithMaskMETAL(psContext, psOperand->psSubOperand[0], opFlags, OPERAND_4_COMPONENT_MASK_X);
                        bformata(metal, ") %% 4)]");
                    }
                }
                else
                {
                    bcatcstr(metal, "[");
                    TranslateOperandMETAL(psContext, psOperand->psSubOperand[0], opFlags);
                    bformata(metal, "]");
                }
            }
            else
            if (index != -1 && psOperand->psSubOperand[1] != NULL)
            {
                // Array of matrices is treated as array of vec4s in HLSL,
                // but that would mess up uniform types in metal. Do gymnastics.
                SHADER_VARIABLE_TYPE eType2 = GetOperandDataTypeMETAL(psContext, psOperand->psSubOperand[1]);
                uint32_t opFlags = TO_FLAG_INTEGER;
                if (eType2 != SVT_INT && eType2 != SVT_UINT)
                {
                    opFlags = TO_AUTO_BITCAST_TO_INT;
                }

                if (psVarType && (psVarType->Class == SVC_MATRIX_COLUMNS || psVarType->Class == SVC_MATRIX_ROWS) && (psVarType->Elements > 1))
                {
                    // Special handling for matrix arrays
                    bcatcstr(metal, "[(");
                    TranslateOperandMETAL(psContext, psOperand->psSubOperand[1], opFlags);
                    bformata(metal, " + %d) / 4]", index);
                    if (psContext->psShader->eTargetLanguage <= LANG_120)
                    {
                        bcatcstr(metal, "[int(mod(float(");
                        TranslateOperandMETAL(psContext, psOperand->psSubOperand[1], opFlags);
                        bformata(metal, " + %d), 4.0))]", index);
                    }
                    else
                    {
                        bcatcstr(metal, "[((");
                        TranslateOperandMETAL(psContext, psOperand->psSubOperand[1], opFlags);
                        bformata(metal, " + %d) %% 4)]", index);
                    }
                }
                else
                {
                    bcatcstr(metal, "[");
                    TranslateOperandMETAL(psContext, psOperand->psSubOperand[1], opFlags);
                    bformata(metal, " + %d]", index);
                }
            }
            else if (index != -1)
            {
                if (psVarType && (psVarType->Class == SVC_MATRIX_COLUMNS || psVarType->Class == SVC_MATRIX_ROWS) && (psVarType->Elements > 1))
                {
                    // Special handling for matrix arrays, open them up into vec4's
                    size_t matidx = index / 4;
                    size_t rowidx = index - (matidx * 4);
                    bformata(metal, "[%d][%d]", matidx, rowidx);
                }
                else
                {
                    bformata(metal, "[%d]", index);
                }
            }
            else if (psOperand->psSubOperand[1] != NULL)
            {
                SHADER_VARIABLE_TYPE eType2 = GetOperandDataTypeMETAL(psContext, psOperand->psSubOperand[1]);
                bcatcstr(metal, "[");
                TranslateOperandMETAL(psContext, psOperand->psSubOperand[1], TO_FLAG_INTEGER);
                bcatcstr(metal, "]");
            }

            if (psVarType && psVarType->Class == SVC_VECTOR)
            {
                switch (rebase)
                {
                case 4:
                {
                    if (psVarType->Columns == 2)
                    {
                        //.x(metal) is .y(HLSL). .y(metal) is .z(HLSL)
                        bcatcstr(metal, ".xxyx");
                    }
                    else if (psVarType->Columns == 3)
                    {
                        //.x(metal) is .y(HLSL). .y(metal) is .z(HLSL) .z(metal) is .w(HLSL)
                        bcatcstr(metal, ".xxyz");
                    }
                    break;
                }
                case 8:
                {
                    if (psVarType->Columns == 2)
                    {
                        //.x(metal) is .z(HLSL). .y(metal) is .w(HLSL)
                        bcatcstr(metal, ".xxxy");
                    }
                    break;
                }
                case 0:
                default:
                {
                    //No rebase, but extend to vec4.
                    if (psVarType->Columns == 2)
                    {
                        bcatcstr(metal, ".xyxx");
                    }
                    else if (psVarType->Columns == 3)
                    {
                        bcatcstr(metal, ".xyzx");
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
        ResourceNameMETAL(metal, psContext, RGROUP_TEXTURE, psOperand->ui32RegisterNumber, 0);
        *pui32IgnoreSwizzle = 1;
        break;
    }
    case OPERAND_TYPE_SAMPLER:
    {
        bformata(metal, "Sampler%d", psOperand->ui32RegisterNumber);
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

        bformata(metal, "%s_Func%d", ClassTypeName, ui32UniqueClassFuncIndex);
        break;
    }
    case OPERAND_TYPE_INPUT_FORK_INSTANCE_ID:
    {
        bcatcstr(metal, "forkInstanceID");
        *pui32IgnoreSwizzle = 1;
        return;
    }
    case OPERAND_TYPE_IMMEDIATE_CONSTANT_BUFFER:
    {
        bcatcstr(metal, "immediateConstBufferF");

        if (psOperand->psSubOperand[0])
        {
            bcatcstr(metal, "(");     //Indexes must be integral.
            TranslateOperandMETAL(psContext, psOperand->psSubOperand[0], TO_FLAG_INTEGER);
            bcatcstr(metal, ")");
        }
        break;
    }
    case OPERAND_TYPE_INPUT_DOMAIN_POINT:
    {
        bcatcstr(metal, "gl_TessCoord");
        break;
    }
    case OPERAND_TYPE_INPUT_CONTROL_POINT:
    {
        if (psOperand->aui32ArraySizes[1] == 0)   //Input index zero - position.
        {
            if ((ui32TOFlag & TO_FLAG_DECLARATION_NAME) != TO_FLAG_DECLARATION_NAME)
            {
                bcatcstr(metal, "stageIn.");
            }
            bformata(metal, "position", psOperand->aui32ArraySizes[0]);
        }
        else
        {
            bformata(metal, "Input%d[%d]", psOperand->aui32ArraySizes[1], psOperand->aui32ArraySizes[0]);
        }
        break;
    }
    case OPERAND_TYPE_NULL:
    {
        // Null register, used to discard results of operations
        bcatcstr(metal, "//null");
        break;
    }
    case OPERAND_TYPE_OUTPUT_CONTROL_POINT_ID:
    {
        break;
    }
    case OPERAND_TYPE_OUTPUT_COVERAGE_MASK:
    {
        if ((ui32TOFlag & TO_FLAG_DECLARATION_NAME) != TO_FLAG_DECLARATION_NAME)
        {
            bcatcstr(metal, "output.");
        }
        bcatcstr(metal, "sampleMask");
        *pui32IgnoreSwizzle = 1;
        break;
    }
    case OPERAND_TYPE_INPUT_COVERAGE_MASK:
    {
        if ((ui32TOFlag & TO_FLAG_DECLARATION_NAME) != TO_FLAG_DECLARATION_NAME)
        {
            bcatcstr(metal, "stageIn.");
        }
        bcatcstr(metal, "sampleMask");
        //Skip swizzle on scalar types.
        *pui32IgnoreSwizzle = 1;
        break;
    }
    case OPERAND_TYPE_INPUT_THREAD_ID:    //SV_DispatchThreadID
    {
        bcatcstr(metal, "vThreadID");
        break;
    }
    case OPERAND_TYPE_INPUT_THREAD_GROUP_ID:    //SV_GroupThreadID
    {
        bcatcstr(metal, "vThreadGroupID");
        break;
    }
    case OPERAND_TYPE_INPUT_THREAD_ID_IN_GROUP:    //SV_GroupID
    {
        bcatcstr(metal, "vThreadIDInGroup");
        break;
    }
    case OPERAND_TYPE_INPUT_THREAD_ID_IN_GROUP_FLATTENED:    //SV_GroupIndex
    {
        bcatcstr(metal, "vThreadIDInGroupFlattened");
        *pui32IgnoreSwizzle = 1;     // No swizzle meaningful for scalar.
        break;
    }
    case OPERAND_TYPE_UNORDERED_ACCESS_VIEW:
    {
        ResourceNameMETAL(metal, psContext, RGROUP_UAV, psOperand->ui32RegisterNumber, 0);
        if (ui32TOFlag | TO_FLAG_NAME_ONLY)
        {
            *pui32IgnoreSwizzle = 1;
        }
        break;
    }
    case OPERAND_TYPE_THREAD_GROUP_SHARED_MEMORY:
    {
        bformata(metal, "TGSM%d", psOperand->ui32RegisterNumber);
        *pui32IgnoreSwizzle = 1;     // No swizzle meaningful for scalar.
        break;
    }
    case OPERAND_TYPE_INPUT_PRIMITIVEID:
    {
        break;
    }
    case OPERAND_TYPE_INDEXABLE_TEMP:
    {
        bformata(metal, "TempArray%d", psOperand->aui32ArraySizes[0]);
        bcatcstr(metal, "[");
        if (psOperand->aui32ArraySizes[1] != 0 || !psOperand->psSubOperand[1])
        {
            bformata(metal, "%d", psOperand->aui32ArraySizes[1]);
        }

        if (psOperand->psSubOperand[1])
        {
            if (psOperand->aui32ArraySizes[1] != 0)
            {
                bcatcstr(metal, "+");
            }
            TranslateOperandMETAL(psContext, psOperand->psSubOperand[1], TO_FLAG_INTEGER);
        }
        bcatcstr(metal, "]");
        break;
    }
    case OPERAND_TYPE_STREAM:
    {
        bformata(metal, "%d", psOperand->ui32RegisterNumber);
        break;
    }
    case OPERAND_TYPE_INPUT_GS_INSTANCE_ID:
    {
        // No GS in METAL
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
    case OPERAND_TYPE_INPUT_PATCH_CONSTANT:
    {
        bformata(metal, "myPatchConst%d", psOperand->ui32RegisterNumber);
        break;
    }
    default:
    {
        ASSERT(0);
        break;
    }
    }

    if (hasCtor && (*pui32IgnoreSwizzle == 0))
    {
        TranslateOperandSwizzleWithMaskMETAL(psContext, psOperand, ui32CompMask);
        *pui32IgnoreSwizzle = 1;
    }

    if (*pui32IgnoreSwizzle == 0)
    {
        TranslateOperandSwizzleWithMaskMETAL(psContext, psOperand, ui32CompMask);
    }

    while (numParenthesis != 0)
    {
        bcatcstr(metal, ")");
        numParenthesis--;
    }
}

static void METALTranslateVariableName(HLSLCrossCompilerContext* psContext, const Operand* psOperand, uint32_t ui32TOFlag, uint32_t* pui32IgnoreSwizzle)
{
    METALMETALTranslateVariableNameWithMask(psContext, psOperand, ui32TOFlag, pui32IgnoreSwizzle, OPERAND_4_COMPONENT_MASK_ALL);
}


SHADER_VARIABLE_TYPE GetOperandDataTypeMETAL(HLSLCrossCompilerContext* psContext, const Operand* psOperand)
{
    return GetOperandDataTypeExMETAL(psContext, psOperand, SVT_INT);
}

SHADER_VARIABLE_TYPE GetOperandDataTypeExMETAL(HLSLCrossCompilerContext* psContext, const Operand* psOperand, SHADER_VARIABLE_TYPE ePreferredTypeForImmediates)
{

    // The min precision qualifier overrides all of the stuff below
    if (psOperand->eMinPrecision == OPERAND_MIN_PRECISION_FLOAT_16)
    {   
        return SVT_FLOAT16;    
    }

    switch (psOperand->eType)
    {
    case OPERAND_TYPE_TEMP:
    {
        SHADER_VARIABLE_TYPE eCurrentType = SVT_VOID;
        int i = 0;

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

        if (GetOutputSignatureFromRegister(psContext->currentPhase,
                ui32Register,
                psOperand->ui32CompMask,
                0,
                &psContext->psShader->sInfo,
                &psOut))
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
        if (psOperand->eSpecialName == NAME_IS_FRONT_FACE)
        {
            return SVT_BOOL;
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
        return ePreferredTypeForImmediates;
    }

    case OPERAND_TYPE_INPUT_THREAD_ID:
    case OPERAND_TYPE_INPUT_THREAD_GROUP_ID:
    case OPERAND_TYPE_INPUT_THREAD_ID_IN_GROUP:
    case OPERAND_TYPE_INPUT_THREAD_ID_IN_GROUP_FLATTENED:
    {
        return SVT_UINT;
    }
    case OPERAND_TYPE_SPECIAL_ADDRESS:
    case OPERAND_TYPE_SPECIAL_LOOPCOUNTER:
    {
        return SVT_INT;
    }
    case OPERAND_TYPE_INPUT_GS_INSTANCE_ID:
    {
        return SVT_UINT;
    }
    case OPERAND_TYPE_OUTPUT_COVERAGE_MASK:
    {
        return SVT_INT;
    }
    case OPERAND_TYPE_OUTPUT_CONTROL_POINT_ID:
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

void TranslateOperandMETAL(HLSLCrossCompilerContext* psContext, const Operand* psOperand, uint32_t ui32TOFlag)
{
    TranslateOperandWithMaskMETAL(psContext, psOperand, ui32TOFlag, OPERAND_4_COMPONENT_MASK_ALL);
}

void TranslateOperandWithMaskMETAL(HLSLCrossCompilerContext* psContext, const Operand* psOperand, uint32_t ui32TOFlag, uint32_t ui32ComponentMask)
{
    bstring metal = *psContext->currentShaderString;
    uint32_t ui32IgnoreSwizzle = 0;
    SHADER_VARIABLE_TYPE eType = GetOperandDataTypeExMETAL(psContext, psOperand, TypeFlagsToSVTTypeMETAL(ui32TOFlag));

    if (ui32TOFlag & TO_FLAG_NAME_ONLY)
    {
        METALTranslateVariableName(psContext, psOperand, ui32TOFlag, &ui32IgnoreSwizzle);
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
        bcatcstr(metal, "(-");
        break;
    }
    case OPERAND_MODIFIER_ABS:
    {
        bcatcstr(metal, "abs(");
        break;
    }
    case OPERAND_MODIFIER_ABSNEG:
    {
        bcatcstr(metal, "-abs(");
        break;
    }
    }

    METALMETALTranslateVariableNameWithMask(psContext, psOperand, ui32TOFlag, &ui32IgnoreSwizzle, ui32ComponentMask);

    switch (psOperand->eModifier)
    {
    case OPERAND_MODIFIER_NONE:
    {
        break;
    }
    case OPERAND_MODIFIER_NEG:
    {
        bcatcstr(metal, ")");
        break;
    }
    case OPERAND_MODIFIER_ABS:
    {
        bcatcstr(metal, ")");
        break;
    }
    case OPERAND_MODIFIER_ABSNEG:
    {
        bcatcstr(metal, ")");
        break;
    }
    }
}

void ResourceNameMETAL(bstring targetStr, HLSLCrossCompilerContext* psContext, ResourceGroup group, const uint32_t ui32RegisterNumber, const int bZCompare)
{
    bstring metal = (targetStr == NULL) ? *psContext->currentShaderString : targetStr;
    ResourceBinding* psBinding = 0;
    int found;

    found = GetResourceFromBindingPoint(group, ui32RegisterNumber, &psContext->psShader->sInfo, &psBinding);

    if (found)
    {
        int i = 0;
        char name[MAX_REFLECT_STRING_LENGTH];
        uint32_t ui32ArrayOffset = ui32RegisterNumber - psBinding->ui32BindPoint;

        while (psBinding->Name[i] != '\0' && i < (MAX_REFLECT_STRING_LENGTH - 1))
        {
            name[i] = psBinding->Name[i];

            //array syntax [X] becomes _0_
            //Otherwise declarations could end up as:
            //uniform sampler2D SomeTextures[0];
            //uniform sampler2D SomeTextures[1];
            if (name[i] == '[' || name[i] == ']')
            {
                name[i] = '_';
            }

            ++i;
        }

        name[i] = '\0';

        if (ui32ArrayOffset)
        {
            bformata(metal, "%s%d", name, ui32ArrayOffset);
        }
        else
        {
            bformata(metal, "%s", name);
        }

        if (RGROUP_SAMPLER == group)
        {
            if (bZCompare)
            {
                bcatcstr(metal, "_cmp");
            }
            else
            {
                bcatcstr(metal, "_s");
            }
        }
    }
    else
    {
        bformata(metal, "UnknownResource%d", ui32RegisterNumber);
    }
}

bstring TextureSamplerNameMETAL(ShaderInfo* psShaderInfo, const uint32_t ui32TextureRegisterNumber, const uint32_t ui32SamplerRegisterNumber, const int bZCompare)
{
    bstring result;
    ResourceBinding* psTextureBinding = 0;
    ResourceBinding* psSamplerBinding = 0;
    int foundTexture, foundSampler;
    uint32_t i = 0;
    char samplerName[MAX_REFLECT_STRING_LENGTH];
    uint32_t ui32ArrayOffset;

    foundTexture = GetResourceFromBindingPoint(RGROUP_TEXTURE, ui32TextureRegisterNumber, psShaderInfo, &psTextureBinding);
    foundSampler = GetResourceFromBindingPoint(RGROUP_SAMPLER, ui32SamplerRegisterNumber, psShaderInfo, &psSamplerBinding);

    if (!foundTexture || !foundSampler)
    {
        result = bformat("UnknownResource%d_%d", ui32TextureRegisterNumber, ui32SamplerRegisterNumber);
        return result;
    }

    ui32ArrayOffset = ui32SamplerRegisterNumber - psSamplerBinding->ui32BindPoint;

    while (psSamplerBinding->Name[i] != '\0' && i < (MAX_REFLECT_STRING_LENGTH - 1))
    {
        samplerName[i] = psSamplerBinding->Name[i];

        //array syntax [X] becomes _0_
        //Otherwise declarations could end up as:
        //uniform sampler2D SomeTextures[0];
        //uniform sampler2D SomeTextures[1];
        if (samplerName[i] == '[' || samplerName[i] == ']')
        {
            samplerName[i] = '_';
        }

        ++i;
    }
    samplerName[i] = '\0';

    result = bfromcstr("");



    if (ui32ArrayOffset)
    {
        bformata(result, "%s%d", samplerName, ui32ArrayOffset);
    }
    else
    {
        bformata(result, "%s", samplerName);
    }

    if (bZCompare)
    {
        bcatcstr(result, "_cmp");
    }
    else
    {
        bcatcstr(result, "_s");
    }

    return result;
}

void ConcatTextureSamplerNameMETAL(bstring str, ShaderInfo* psShaderInfo, const uint32_t ui32TextureRegisterNumber, const uint32_t ui32SamplerRegisterNumber, const int bZCompare)
{
    bstring texturesamplername = TextureSamplerNameMETAL(psShaderInfo, ui32TextureRegisterNumber, ui32SamplerRegisterNumber, bZCompare);
    bconcat(str, texturesamplername);
    bdestroy(texturesamplername);
}

uint32_t GetGmemInputResourceSlotMETAL(uint32_t const slotIn)
{
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

uint32_t GetGmemInputResourceNumElementsMETAL(uint32_t const slotIn)
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
