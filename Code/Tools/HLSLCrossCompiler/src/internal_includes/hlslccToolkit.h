// Modifications copyright Amazon.com, Inc. or its affiliates
#ifndef HLSLCC_TOOLKIT_DECLARATION_H
#define HLSLCC_TOOLKIT_DECLARATION_H

#include "hlslcc.h"
#include "bstrlib.h"
#include "internal_includes/structs.h"

#include <stdbool.h>

// Check if "src" type can be assigned directly to the "dest" type.
bool DoAssignmentDataTypesMatch(SHADER_VARIABLE_TYPE dest, SHADER_VARIABLE_TYPE src);

// Returns the constructor needed depending on the type, the number of components and the use of precision qualifier.
const char * GetConstructorForTypeGLSL(HLSLCrossCompilerContext* psContext, const SHADER_VARIABLE_TYPE eType, const int components, bool useGLSLPrecision);

// Transform from a variable type to a shader variable flag.
uint32_t SVTTypeToFlag(const SHADER_VARIABLE_TYPE eType);

// Transform from a shader variable flag to a shader variable type.
SHADER_VARIABLE_TYPE TypeFlagsToSVTType(const uint32_t typeflags);

// Check if the "src" type can be casted using a constructor to the "dest" type (without bitcasting).
bool CanDoDirectCast(SHADER_VARIABLE_TYPE src, SHADER_VARIABLE_TYPE dest);

// Returns the bitcast operation needed to assign the "src" type to the "dest" type
const char* GetBitcastOp(SHADER_VARIABLE_TYPE src, SHADER_VARIABLE_TYPE dest);

// Check if the register number is part of the ones we used for signaling GMEM input
bool IsGmemReservedSlot(FRAMEBUFFER_FETCH_TYPE type, const uint32_t regNumber);

// Return the name of an auxiliary variable used to save intermediate values to bypass driver issues
const char * GetAuxArgumentName(const SHADER_VARIABLE_TYPE varType);

#endif