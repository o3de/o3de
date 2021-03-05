// Modifications copyright Amazon.com, Inc. or its affiliates
// Modifications copyright Crytek GmbH

#ifndef DECODE_H
#define DECODE_H

#include "internal_includes/structs.h"

ShaderData* DecodeDXBC(uint32_t* data);

//You don't need to call this directly because DecodeDXBC
//will call DecodeDX9BC if the shader looks
//like it is SM1/2/3.
ShaderData* DecodeDX9BC(const uint32_t* pui32Tokens);

void UpdateOperandReferences(ShaderData* psShader, Instruction* psInst);

#endif
