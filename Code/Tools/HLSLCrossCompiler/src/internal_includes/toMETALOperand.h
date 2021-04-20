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
#define TO_FLAG_FLOAT 0x40

void TranslateOperandMETAL(HLSLCrossCompilerContext* psContext, const Operand* psOperand, uint32_t ui32TOFlag);

int GetMaxComponentFromComponentMaskMETAL(const Operand* psOperand);
void TranslateOperandMETALIndex(HLSLCrossCompilerContext* psContext, const Operand* psOperand, int index);
void TranslateOperandMETALIndexMAD(HLSLCrossCompilerContext* psContext, const Operand* psOperand, int index, uint32_t multiply, uint32_t add);
void TranslateVariableNameMETAL(HLSLCrossCompilerContext* psContext, const Operand* psOperand, uint32_t ui32TOFlag, uint32_t* pui32IgnoreSwizzle);
void TranslateOperandMETALSwizzle(HLSLCrossCompilerContext* psContext, const Operand* psOperand);
uint32_t GetNumSwizzleElementsMETAL(const Operand* psOperand);
void AddSwizzleUsingElementCountMETAL(HLSLCrossCompilerContext* psContext, uint32_t count);
int GetFirstOperandSwizzleMETAL(HLSLCrossCompilerContext* psContext, const Operand* psOperand);
uint32_t IsSwizzleReplacatedMETAL(const Operand* psOperand);

void TextureNameMETAL(HLSLCrossCompilerContext* psContext, const uint32_t ui32RegisterNumber, const int bZCompare);

uint32_t ConvertOperandSwizzleToComponentMaskMETAL(const Operand* psOperand);
//Non-zero means the components overlap
int CompareOperandSwizzlesMETAL(const Operand* psOperandA, const Operand* psOperandB);

SHADER_VARIABLE_TYPE GetOperandDataTypeMETAL(HLSLCrossCompilerContext* psContext, const Operand* psOperand);

#endif
