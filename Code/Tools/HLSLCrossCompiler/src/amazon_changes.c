// Modifications copyright Amazon.com, Inc. or its affiliates
// Modifications copyright Crytek GmbH

#include "internal_includes/toGLSLInstruction.h"
#include "internal_includes/toGLSLOperand.h"
#include "internal_includes/languages.h"
#include "bstrlib.h"
#include "stdio.h"
#include "internal_includes/debug.h"
#include "internal_includes/hlslcc_malloc.h"
#include "amazon_changes.h"

#if defined(__clang__)
#pragma clang diagnostic ignored "-Wpointer-sign"
#endif

extern void AddIndentation(HLSLCrossCompilerContext* psContext);

// These are .c files, so no C++ or C++11 for us :(
#define MAX_VARIABLE_LENGTH 16

// This struct is used to keep track of each valid occurance of xxxBitsToxxx(variable) and store all relevant information for fixing that instance
typedef struct ShaderCastLocation
{
    char tempVariableName[MAX_VARIABLE_LENGTH];
    char replacementVariableName[MAX_VARIABLE_LENGTH];
    unsigned int castType;

    // Since we have no stl, here's our list
    struct ShaderCastLocation* next;
} ShaderCastLocation;

// Structure used to prebuild the list of all functions that need to be replaced.
typedef struct ShaderCastType
{
    const char* functionName;
    unsigned int castType;
    const char* variableTypeName; // String for the variable type used when declaring a temporary variable to replace the source temp vector
} ShaderCastType;

enum ShaderCasts
{
    CAST_UINTBITSTOFLOAT,
    CAST_INTBITSTOFLOAT,
    CAST_FLOATBITSTOUINT,
    CAST_FLOATBITSTOINT,
    CAST_NUMCASTS
};

// NOTICE: Order is important here because intBitsToFloat is a substring of uintBitsToFloat, so do not change the ordering here!
static const ShaderCastType s_castFunctions[CAST_NUMCASTS] =
{
    { "uintBitsToFloat",    CAST_UINTBITSTOFLOAT,   "uvec4" },
    { "intBitsToFloat",     CAST_INTBITSTOFLOAT,    "ivec4" },
    { "floatBitsToUint",    CAST_FLOATBITSTOUINT,   "vec4" },
    { "floatBitsToInt",     CAST_FLOATBITSTOINT,    "vec4" }
};

int IsValidUseCase( char* variableStart, char* outVariableName, ShaderCastLocation* foundShaderCastsHead, int currentType )
{
    // Cases we have to replace (this is very strict in definition):
    // 1) floatBitsToInt(Temp2)
    // 2) floatBitsToInt(Temp2.x)
    // 3) floatBitsToInt(Temp[0])
    // 4) floatBitsToInt(Temp[0].x)
    // Cases we do not have to replace:
    // 1) floatBitsToInt(vec4(Temp2))
    // 2) floatBitsToInt(Output0.x != 0.0f ? 1.0f : 0.0f)
    // 3) Any other version that evaluates an expression within the ()
    if ( strncmp(variableStart, "Temp", 4) != 0 )
        return 0;

    unsigned int lengthOfVariable = 4; // Start at 4 for temp

    while ( 1 )
    {
        char val = *(variableStart + lengthOfVariable);

        // If alphanumeric or [] (array), we have a valid variable name
        if ( isalnum( val ) || (val == '[') || (val == ']') )
        {
            lengthOfVariable++;
        }
        else if ( (val == ')') || (val == '.') )
        {
            // Found end of variable
            break;
        }
        else
        {
            // Found something unexpected, so abort
            return 0;
        }
    }

    ASSERT( lengthOfVariable < MAX_VARIABLE_LENGTH );

    // Now ensure that no duplicates of this declaration already exist
    ShaderCastLocation* currentLink = foundShaderCastsHead;
    while ( currentLink )
    {
        // If we have the same type and the same name
        if ( (currentType == currentLink->castType) && (strncmp(variableStart, currentLink->tempVariableName, lengthOfVariable) == 0) )
            return 0; // Do not add because an entry already exists for this variable and this cast function

        // Hmm...I guess this scenario is possible, but it has not shown up in any shaders.
        // The only time we could ever hit this is if the same line casts a float to both an int and uint in separate calls
        // Seems highly unlikely, so let's just assert for now and fix it if we have to.
        if ( strncmp(variableStart, currentLink->tempVariableName, lengthOfVariable) == 0 )
        {
            // TODO: Implement this case where we cast the same variable to multiple types on the same line of GLSL
            ASSERT(0);
        }

        currentLink = currentLink->next;
    }

    // We found a unique instance, so store it
    strncpy( outVariableName, variableStart, lengthOfVariable );
    return 1;
}

void ModifyLineForQualcommReinterpretCastBug( HLSLCrossCompilerContext* psContext, bstring* originalString, bstring* overloadString )
{
    unsigned int numFoundCasts = 0;

    ShaderCastLocation* foundShaderCastsHead = NULL;
    ShaderCastLocation* currentShaderCasts = NULL;

    // Find all occurances of the *BitsTo* functions
    // Note that this would be cleaner, but 'intBitsToFloat' is a substring of 'uintBitsToFloat' so parsing order is important here.
    char* parsingString = bdataofs(*overloadString, 0);
    while ( parsingString )
    {
        char* result = NULL;

        for ( int index=0; index<CAST_NUMCASTS; ++index )
        {
            result = strstr( parsingString, s_castFunctions[index].functionName );
            if ( result != NULL )
            {
                // Now determine if this is a case that requires a workaround
                char* variableStart = result + strlen( s_castFunctions[index].functionName ) + 1; // Add the function name + first parenthesis
                char tempVariableName[MAX_VARIABLE_LENGTH];
                memset( tempVariableName, 0, MAX_VARIABLE_LENGTH );

                // Now the next word must be Temp, or this is not a valid case
                if ( IsValidUseCase( variableStart, tempVariableName, foundShaderCastsHead, index ) )
                {
                    // Now store the information about this cast.  Allocate a new link in the list.
                    if ( !foundShaderCastsHead )
                    {
                        foundShaderCastsHead = (ShaderCastLocation*)hlslcc_malloc( sizeof(ShaderCastLocation) );
                        memset( foundShaderCastsHead, 0x0, sizeof(ShaderCastLocation) );
                        currentShaderCasts = foundShaderCastsHead;
                    }
                    else
                    {
                        ASSERT( !currentShaderCasts->next );
                        currentShaderCasts->next = (ShaderCastLocation*)hlslcc_malloc( sizeof(ShaderCastLocation) );
                        memset( currentShaderCasts->next, 0x0, sizeof(ShaderCastLocation) );
                        currentShaderCasts = currentShaderCasts->next;
                    }

                    currentShaderCasts->castType = index;
                    strcpy( currentShaderCasts->tempVariableName, tempVariableName );

                    numFoundCasts++;
                }
                result += strlen( s_castFunctions[index].functionName );

                // Break out of the loop because we have to advance the search string and start over with uintBitsToFloat again due to the problem with intBitsToFloat being a substring
                break;
            }
        }

        parsingString = result;
    }

    // If we have found no casts, then append the line to the primary string
    if ( numFoundCasts == 0 )
    {
        bconcat( *originalString, *overloadString );
        return;
    }

    // Now we start creating our temporary variables to workaround the crash
    currentShaderCasts = foundShaderCastsHead;

    // NOTE: We want a count of all variables processed for this entire shader.  This could be fancier...
    static unsigned int currentVariableIndex = 0;

    while ( currentShaderCasts )
    {
        // Generate new variable name
        sprintf( currentShaderCasts->replacementVariableName, "LYTemp%i", currentVariableIndex );

        // Write out the new variable name declaration and initialize it
        AddIndentation( psContext );
        bformata( *originalString, "%s %s=%s;\n", s_castFunctions[currentShaderCasts->castType].variableTypeName, currentShaderCasts->replacementVariableName, currentShaderCasts->tempVariableName );

        // Now replace all instances of the variable in question with the new variable name.
        // Note: We can't do a breplace on the temp variable name because the variable can still be legally used without a reinterpret cast in that line.
        // Do a full replace on the xxBitsToxx(TempVar) here
        bstring tempVarName = bformat( "%s(%s)", s_castFunctions[currentShaderCasts->castType].functionName, currentShaderCasts->tempVariableName );
        bstring replacementVarName = bformat( "%s(%s)", s_castFunctions[currentShaderCasts->castType].functionName, currentShaderCasts->replacementVariableName );
        bfindreplace( *overloadString, tempVarName, replacementVarName, 0 );

        // Cleanup bstrings allocated from bformat
        bdestroy( tempVarName );
        bdestroy( replacementVarName );

        currentVariableIndex++;
        currentShaderCasts = currentShaderCasts->next;
    }

    // Now append our modified string to the full shader file
    bconcat( *originalString, *overloadString );
}
