// Modifications copyright Amazon.com, Inc. or its affiliates
// Modifications copyright Crytek GmbH

#ifndef TO_GLSL_OPERAND_H
#define TO_GLSL_OPERAND_H

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


void TranslateOperand(HLSLCrossCompilerContext* psContext, const Operand* psOperand, uint32_t ui32TOFlag);
// Translate operand but add additional component mask
void TranslateOperandWithMask(HLSLCrossCompilerContext* psContext, const Operand* psOperand, uint32_t ui32TOFlag, uint32_t ui32ComponentMask);

int GetMaxComponentFromComponentMask(const Operand* psOperand);
void TranslateOperandIndex(HLSLCrossCompilerContext* psContext, const Operand* psOperand, int index);
void TranslateOperandIndexMAD(HLSLCrossCompilerContext* psContext, const Operand* psOperand, int index, uint32_t multiply, uint32_t add);
void TranslateOperandSwizzle(HLSLCrossCompilerContext* psContext, const Operand* psOperand);
void TranslateOperandSwizzleWithMask(HLSLCrossCompilerContext* psContext, const Operand* psOperand, uint32_t ui32ComponentMask);

uint32_t GetNumSwizzleElements(const Operand* psOperand);
uint32_t GetNumSwizzleElementsWithMask(const Operand *psOperand, uint32_t ui32CompMask);
void AddSwizzleUsingElementCount(HLSLCrossCompilerContext* psContext, uint32_t count);
int GetFirstOperandSwizzle(HLSLCrossCompilerContext* psContext, const Operand* psOperand);
uint32_t IsSwizzleReplicated(const Operand* psOperand);

void ResourceName(bstring targetStr, HLSLCrossCompilerContext* psContext, ResourceGroup group, const uint32_t ui32RegisterNumber, const int bZCompare);

bstring TextureSamplerName(ShaderInfo* psShaderInfo, const uint32_t ui32TextureRegisterNumber, const uint32_t ui32SamplerRegisterNumber, const int bZCompare);
void ConcatTextureSamplerName(bstring str, ShaderInfo* psShaderInfo, const uint32_t ui32TextureRegisterNumber, const uint32_t ui32SamplerRegisterNumber, const int bZCompare);

//Non-zero means the components overlap
int CompareOperandSwizzles(const Operand* psOperandA, const Operand* psOperandB);

// Returns the write mask for the operand used for destination
uint32_t GetOperandWriteMask(const Operand *psOperand);

SHADER_VARIABLE_TYPE GetOperandDataType(HLSLCrossCompilerContext* psContext, const Operand* psOperand);
SHADER_VARIABLE_TYPE GetOperandDataTypeEx(HLSLCrossCompilerContext* psContext, const Operand* psOperand, SHADER_VARIABLE_TYPE ePreferredTypeForImmediates);

const char * GetConstructorForType(const SHADER_VARIABLE_TYPE eType,
    const int components);

const char * GetConstructorForTypeFlag(const uint32_t ui32Flag,
    const int components);

uint32_t SVTTypeToFlag(const SHADER_VARIABLE_TYPE eType);
SHADER_VARIABLE_TYPE TypeFlagsToSVTType(const uint32_t typeflags);

#endif
