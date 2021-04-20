// Modifications copyright Amazon.com, Inc. or its affiliates
// Modifications copyright Crytek GmbH

#ifndef TO_METAL_DECLARATION_H
#define TO_METAL_DECLARATION_H

#include "internal_includes/structs.h"

void TranslateDeclarationMETAL(HLSLCrossCompilerContext* psContext, const Declaration* psDecl);

char* GetDeclaredInputNameMETAL(const HLSLCrossCompilerContext* psContext, const SHADER_TYPE eShaderType, const Operand* psOperand);
char* GetDeclaredOutputNameMETAL(const HLSLCrossCompilerContext* psContext, const SHADER_TYPE eShaderType, const Operand* psOperand);

const char* GetMangleSuffixMETAL(const SHADER_TYPE eShaderType);

#endif
