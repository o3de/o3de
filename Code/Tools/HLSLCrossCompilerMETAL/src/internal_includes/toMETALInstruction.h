// Modifications copyright Amazon.com, Inc. or its affiliates
// Modifications copyright Crytek GmbH

#ifndef TO_METAL_INSTRUCTION_H
#define TO_METAL_INSTRUCTION_H

#include "internal_includes/structs.h"
#include "structsMetal.h"

void TranslateInstructionMETAL(HLSLCrossCompilerContext* psContext, Instruction* psInst, Instruction* psNextInst);
void DetectAtomicInstructionMETAL(HLSLCrossCompilerContext* psContext, Instruction* psInst, Instruction* psNextInst, AtomicVarList* psAtomicList);

//For each MOV temp, immediate; check to see if the next instruction
//using that temp has an integer opcode. If so then the immediate value
//is flaged as having an integer encoding.
void MarkIntegerImmediatesMETAL(HLSLCrossCompilerContext* psContext);

void SetDataTypesMETAL(HLSLCrossCompilerContext* psContext, Instruction* psInst, const int32_t i32InstCount);

#endif
