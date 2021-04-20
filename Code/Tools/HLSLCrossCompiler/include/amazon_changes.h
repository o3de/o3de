// Modifications copyright Amazon.com, Inc. or its affiliates
// Modifications copyright Crytek GmbH

#ifndef AMAZON_CHANGES_H
#define AMAZON_CHANGES_H

// There is a bug on the Adreno 420 driver where reinterpret casts can destroy a variable.  We need to replace all instances that look like this:
//      floatBitsToInt(Temp2);
// We do not need to change cases that evaluate an expression within the cast operation, like so:
//      floatBitsToInt(Temp2 + 1.0f);
void ModifyLineForQualcommReinterpretCastBug( HLSLCrossCompilerContext* psContext, bstring* originalString, bstring* overloadString );

#endif // AMAZON_CHANGES_H
