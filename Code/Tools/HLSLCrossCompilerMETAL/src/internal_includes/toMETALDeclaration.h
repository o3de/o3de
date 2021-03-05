// Modifications copyright Amazon.com, Inc. or its affiliates
// Modifications copyright Crytek GmbH

#ifndef TO_METAL_DECLARATION_H
#define TO_METAL_DECLARATION_H

#include "internal_includes/structs.h"
#include "internal_includes/structsMetal.h"

void TranslateDeclarationMETAL(HLSLCrossCompilerContext* psContext, const Declaration* psDecl, AtomicVarList* psAtomicList);

char* GetDeclaredInputNameMETAL(const HLSLCrossCompilerContext* psContext, const SHADER_TYPE eShaderType, const Operand* psOperand);
char* GetDeclaredOutputNameMETAL(const HLSLCrossCompilerContext* psContext, const SHADER_TYPE eShaderType, const Operand* psOperand);

#endif
