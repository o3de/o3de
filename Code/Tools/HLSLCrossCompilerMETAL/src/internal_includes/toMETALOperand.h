// Modifications copyright Amazon.com, Inc. or its affiliates
// Modifications copyright Crytek GmbH

#ifndef TO_METAL_OPERAND_H
#define TO_METAL_OPERAND_H

#include "internal_includes/structs.h"

#define TO_FLAG_NONE    0x0
#define TO_FLAG_INTEGER 0x1
#define TO_FLAG_NAME_ONLY 0x2
#define TO_FLAG_DECLARATION_NAME 0x4
#define TO_FLAG_DESTINATION 0x8 //Operand is being written to by assignment.
#define TO_FLAG_UNSIGNED_INTEGER 0x10
#define TO_FLAG_DOUBLE 0x20
#define TO_FLAG_FLOAT16 0x40
// --- TO_AUTO_BITCAST_TO_FLOAT ---
//If the operand is an integer temp variable then this flag
//indicates that the temp has a valid floating point encoding
//and that the current expression expects the operand to be floating point
//and therefore intBitsToFloat must be applied to that variable.
#define TO_AUTO_BITCAST_TO_FLOAT 0x80
#define TO_AUTO_BITCAST_TO_INT 0x100
#define TO_AUTO_BITCAST_TO_UINT 0x200
#define TO_AUTO_BITCAST_TO_FLOAT16 0x400
// AUTO_EXPAND flags automatically expand the operand to at least (i/u)vecX
// to match HLSL functionality.
#define TO_AUTO_EXPAND_TO_VEC2 0x800
#define TO_AUTO_EXPAND_TO_VEC3 0x1000
#define TO_AUTO_EXPAND_TO_VEC4 0x2000

void TranslateOperandMETAL(HLSLCrossCompilerContext* psContext, const Operand* psOperand, uint32_t ui32TOFlag);
// Translate operand but add additional component mask
void TranslateOperandWithMaskMETAL(HLSLCrossCompilerContext* psContext, const Operand* psOperand, uint32_t ui32TOFlag, uint32_t ui32ComponentMask);

int GetMaxComponentFromComponentMaskMETAL(const Operand* psOperand);
void TranslateOperandIndexMETAL(HLSLCrossCompilerContext* psContext, const Operand* psOperand, int index);
void TranslateOperandIndexMADMETAL(HLSLCrossCompilerContext* psContext, const Operand* psOperand, int index, uint32_t multiply, uint32_t add);
void TranslateOperandSwizzleMETAL(HLSLCrossCompilerContext* psContext, const Operand* psOperand);
void TranslateOperandSwizzleWithMaskMETAL(HLSLCrossCompilerContext* psContext, const Operand* psOperand, uint32_t ui32ComponentMask);

void TranslateGmemOperandSwizzleWithMaskMETAL(HLSLCrossCompilerContext* psContext, const Operand* psOperand, uint32_t ui32ComponentMask, uint32_t gmemNumElements);

uint32_t GetNumSwizzleElementsMETAL(const Operand* psOperand);
uint32_t GetNumSwizzleElementsWithMaskMETAL(const Operand *psOperand, uint32_t ui32CompMask);
void AddSwizzleUsingElementCountMETAL(HLSLCrossCompilerContext* psContext, uint32_t count);
int GetFirstOperandSwizzleMETAL(HLSLCrossCompilerContext* psContext, const Operand* psOperand);
uint32_t IsSwizzleReplicatedMETAL(const Operand* psOperand);

void ResourceNameMETAL(bstring targetStr, HLSLCrossCompilerContext* psContext, ResourceGroup group, const uint32_t ui32RegisterNumber, const int bZCompare);

bstring TextureSamplerNameMETAL(ShaderInfo* psShaderInfo, const uint32_t ui32TextureRegisterNumber, const uint32_t ui32SamplerRegisterNumber, const int bZCompare);
void ConcatTextureSamplerNameMETAL(bstring str, ShaderInfo* psShaderInfo, const uint32_t ui32TextureRegisterNumber, const uint32_t ui32SamplerRegisterNumber, const int bZCompare);

//Non-zero means the components overlap
int CompareOperandSwizzlesMETAL(const Operand* psOperandA, const Operand* psOperandB);

// Returns the write mask for the operand used for destination
uint32_t GetOperandWriteMaskMETAL(const Operand *psOperand);

SHADER_VARIABLE_TYPE GetOperandDataTypeMETAL(HLSLCrossCompilerContext* psContext, const Operand* psOperand);
SHADER_VARIABLE_TYPE GetOperandDataTypeExMETAL(HLSLCrossCompilerContext* psContext, const Operand* psOperand, SHADER_VARIABLE_TYPE ePreferredTypeForImmediates);

const char * GetConstructorForTypeMETAL(const SHADER_VARIABLE_TYPE eType,
    const int components);

const char * GetConstructorForTypeFlagMETAL(const uint32_t ui32Flag,
    const int components);

uint32_t SVTTypeToFlagMETAL(const SHADER_VARIABLE_TYPE eType);
SHADER_VARIABLE_TYPE TypeFlagsToSVTTypeMETAL(const uint32_t typeflags);


uint32_t GetGmemInputResourceSlotMETAL(uint32_t const slotIn);

uint32_t GetGmemInputResourceNumElementsMETAL(uint32_t const slotIn);

#endif
