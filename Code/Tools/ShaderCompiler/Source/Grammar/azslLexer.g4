/*
Work from Tim Jones
MIT license
-
modifications by Amazon. C 2018
 */
lexer grammar azslLexer;

channels { PREPROCESSOR, COMMENTS }

AppendStructuredBuffer : 'AppendStructuredBuffer';
Bool : 'bool';
Bool1 : 'bool1';
Bool2 : 'bool2';
Bool3 : 'bool3';
Bool4 : 'bool4';
Bool1x1 : 'bool1x1';
Bool1x2 : 'bool1x2';
Bool1x3 : 'bool1x3';
Bool1x4 : 'bool1x4';
Bool2x1 : 'bool2x1';
Bool2x2 : 'bool2x2';
Bool2x3 : 'bool2x3';
Bool2x4 : 'bool2x4';
Bool3x1 : 'bool3x1';
Bool3x2 : 'bool3x2';
Bool3x3 : 'bool3x3';
Bool3x4 : 'bool3x4';
Bool4x1 : 'bool4x1';
Bool4x2 : 'bool4x2';
Bool4x3 : 'bool4x3';
Bool4x4 : 'bool4x4';
Buffer : 'Buffer';
BuiltInTriangleIntersectionAttributes : 'BuiltInTriangleIntersectionAttributes';
ByteAddressBuffer : 'ByteAddressBuffer';
Break : 'break';
Case : 'case';
CBuffer : 'cbuffer';
Centroid : 'centroid';
ConstantBuffer : 'constantbuffer';
ConstantBufferCamel : 'ConstantBuffer';
Class : 'class';
ColumnMajor : 'column_major';
Const : 'const';
ConsumeStructuredBuffer : 'ConsumeStructuredBuffer';
Continue : 'continue';
Default : 'default';
Discard : 'discard';
Do : 'do';
Double : 'double';
Double1 : 'double1';
Double2 : 'double2';
Double3 : 'double3';
Double4 : 'double4';
Double1x1 : 'double1x1';
Double1x2 : 'double1x2';
Double1x3 : 'double1x3';
Double1x4 : 'double1x4';
Double2x1 : 'double2x1';
Double2x2 : 'double2x2';
Double2x3 : 'double2x3';
Double2x4 : 'double2x4';
Double3x1 : 'double3x1';
Double3x2 : 'double3x2';
Double3x3 : 'double3x3';
Double3x4 : 'double3x4';
Double4x1 : 'double4x1';
Double4x2 : 'double4x2';
Double4x3 : 'double4x3';
Double4x4 : 'double4x4';
Else : 'else';
Enum : 'enum';
Export : 'export';
Extern : 'extern';
FeedbackTexture2D : 'FeedbackTexture2D';
FeedbackTexture2DArray : 'FeedbackTexture2DArray';
Float : 'float';
Float1 : 'float1';
Float2 : 'float2';
Float3 : 'float3';
Float4 : 'float4';
Float1x1 : 'float1x1';
Float1x2 : 'float1x2';
Float1x3 : 'float1x3';
Float1x4 : 'float1x4';
Float2x1 : 'float2x1';
Float2x2 : 'float2x2';
Float2x3 : 'float2x3';
Float2x4 : 'float2x4';
Float3x1 : 'float3x1';
Float3x2 : 'float3x2';
Float3x3 : 'float3x3';
Float3x4 : 'float3x4';
Float4x1 : 'float4x1';
Float4x2 : 'float4x2';
Float4x3 : 'float4x3';
Float4x4 : 'float4x4';
For : 'for';
Groupshared : 'groupshared';
Globallycoherent : 'globallycoherent';
Global: 'global';
Half : 'half';
Half1 : 'half1';
Half2 : 'half2';
Half3 : 'half3';
Half4 : 'half4';
Half1x1 : 'half1x1';
Half1x2 : 'half1x2';
Half1x3 : 'half1x3';
Half1x4 : 'half1x4';
Half2x1 : 'half2x1';
Half2x2 : 'half2x2';
Half2x3 : 'half2x3';
Half2x4 : 'half2x4';
Half3x1 : 'half3x1';
Half3x2 : 'half3x2';
Half3x3 : 'half3x3';
Half3x4 : 'half3x4';
Half4x1 : 'half4x1';
Half4x2 : 'half4x2';
Half4x3 : 'half4x3';
Half4x4 : 'half4x4';
If : 'if';
In : 'in';
Inline : 'inline';
Rootconstant : 'rootconstant';   // Amazon extension
Inout : 'inout' | 'in out';
InputPatch : 'InputPatch';
Int : 'int';
Int16_t : 'int16_t';
Int32_t : 'int32_t';
Int64_t : 'int64_t';
Int1 : 'int1';
Int2 : 'int2';
Int3 : 'int3';
Int4 : 'int4';
Int1x1 : 'int1x1';
Int1x2 : 'int1x2';
Int1x3 : 'int1x3';
Int1x4 : 'int1x4';
Int2x1 : 'int2x1';
Int2x2 : 'int2x2';
Int2x3 : 'int2x3';
Int2x4 : 'int2x4';
Int3x1 : 'int3x1';
Int3x2 : 'int3x2';
Int3x3 : 'int3x3';
Int3x4 : 'int3x4';
Int4x1 : 'int4x1';
Int4x2 : 'int4x2';
Int4x3 : 'int4x3';
Int4x4 : 'int4x4';
Interface : 'interface';
Line_ : 'line';
LineAdj : 'lineadj';
Linear : 'linear';
LineStream : 'LineStream';
Long : 'long';
Matrix : 'matrix';
Nointerpolation : 'nointerpolation';
Noperspective : 'noperspective';
Option : 'option';
Out : 'out';
OutputPatch : 'OutputPatch';
Override : 'override';   // Amazon extension
Partial : 'partial';   // Amazon extension
Packoffset : 'packoffset';
Point : 'point';
PointStream : 'PointStream';
Precise : 'precise';
// 'payload' is a free identifier as well, DXC has a hard literal keyword that disrupts semantic understanding in some contexts
// notably as variable declarator, if you mention payload it will say «'payload' object must be an in parameter»
// but actually the name the programmer choses doesn't have to be 'payload'. (Also SV_RayPayload appears to be a fake semantic)
RasterizerOrderedBuffer : 'RasterizerOrderedBuffer';
RasterizerOrderedByteAddressBuffer : 'RasterizerOrderedByteAddressBuffer';
RasterizerOrderedStructuredBuffer : 'RasterizerOrderedStructuredBuffer';
RasterizerOrderedTexture1D : 'RasterizerOrderedTexture1D';
RasterizerOrderedTexture1DArray : 'RasterizerOrderedTexture1DArray';
RasterizerOrderedTexture2D : 'RasterizerOrderedTexture2D';
RasterizerOrderedTexture2DArray : 'RasterizerOrderedTexture2DArray';
RasterizerOrderedTexture3D : 'RasterizerOrderedTexture3D';
RayDesc : 'RayDesc';
RaytracingAccelerationStructure : 'RaytracingAccelerationStructure';
Register : 'register';
Return : 'return';
RowMajor : 'row_major';
RWBuffer : 'RWBuffer';
RWByteAddressBuffer : 'RWByteAddressBuffer';
RWStructuredBuffer : 'RWStructuredBuffer';
RWTexture1D : 'RWTexture1D';
RWTexture1DArray : 'RWTexture1DArray';
RWTexture2D : 'RWTexture2D';
RWTexture2DArray : 'RWTexture2DArray';
RWTexture3D : 'RWTexture3D';
Sample : 'sample';
Sampler : 'sampler';
SamplerCapitalS : 'Sampler';
SamplerComparisonState : 'SamplerComparisonState';
SamplerStateCamel : 'SamplerState';
SamplerState : 'sampler_state';
Shared : 'shared';
SNorm : 'snorm';
Static : 'static';
Struct : 'struct';
StructuredBuffer : 'StructuredBuffer';
SubpassInput : 'SubpassInput';
SubpassInputMS : 'SubpassInputMS';
SubpassInputDS : 'SubpassInputDS';
SubpassInputDSMS : 'SubpassInputDSMS';
Switch : 'switch';
TBuffer : 'tbuffer';
Texture1D : 'Texture1D';
Texture1DArray : 'Texture1DArray';
Texture2D : 'Texture2D';
Texture2DArray : 'Texture2DArray';
Texture2DMS : 'Texture2DMS';
Texture2DMSArray : 'Texture2DMSArray';
Texture3D : 'Texture3D';
TextureCube : 'TextureCube';
TextureCubeArray : 'TextureCubeArray';
Triangle : 'triangle';
TriangleAdj : 'triangleadj';
TriangleStream : 'TriangleStream';
Uniform : 'uniform';
// For the --listpredefined option to work, we absolutely need all types to exist in (simple) rules
// that can return strings from Vocabulary::getLiteralName()
// so that rule: Uint : 'uint' | 'dword' # can't be used because it's non-trivialness makes it a non-citizen.
// I tried to recompose the rule with fragments, but fragment also are not listed in the vocabulary. (_literalNames)
Uint : 'uint';
Uint1 : 'uint1';
Uint2 : 'uint2';
Uint3 : 'uint3';
Uint4 : 'uint4';
Uint1x1 : 'uint1x1';
Uint1x2 : 'uint1x2';
Uint1x3 : 'uint1x3';
Uint1x4 : 'uint1x4';
Uint2x1 : 'uint2x1';
Uint2x2 : 'uint2x2';
Uint2x3 : 'uint2x3';
Uint2x4 : 'uint2x4';
Uint3x1 : 'uint3x1';
Uint3x2 : 'uint3x2';
Uint3x3 : 'uint3x3';
Uint3x4 : 'uint3x4';
Uint4x1 : 'uint4x1';
Uint4x2 : 'uint4x2';
Uint4x3 : 'uint4x3';
Uint4x4 : 'uint4x4';
Uint16_t : 'uint16_t';
Uint32_t : 'uint32_t';
Uint64_t : 'uint64_t';
UNorm : 'unorm';
Unsigned : 'unsigned';
Dword : 'dword';
// Amazon addition: DXC didn't advertise it, but the vector/matrix forms of dword are now accepted
Dword1 : 'dword1';
Dword2 : 'dword2';
Dword3 : 'dword3';
Dword4 : 'dword4';
Dword1x1 : 'dword1x1';
Dword1x2 : 'dword1x2';
Dword1x3 : 'dword1x3';
Dword1x4 : 'dword1x4';
Dword2x1 : 'dword2x1';
Dword2x2 : 'dword2x2';
Dword2x3 : 'dword2x3';
Dword2x4 : 'dword2x4';
Dword3x1 : 'dword3x1';
Dword3x2 : 'dword3x2';
Dword3x3 : 'dword3x3';
Dword3x4 : 'dword3x4';
Dword4x1 : 'dword4x1';
Dword4x2 : 'dword4x2';
Dword4x3 : 'dword4x3';
Dword4x4 : 'dword4x4';
Vector : 'vector';
Volatile : 'volatile';
Void : 'void';
While : 'while';

// library subobject types from modern HLSL:
StateObjectConfig : 'StateObjectConfig';
LocalRootSignature : 'LocalRootSignature';
GlobalRootSignature : 'GlobalRootSignature';
SubobjectToExportsAssociation : 'SubobjectToExportsAssociation';
RaytracingShaderConfig : 'RaytracingShaderConfig';
RaytracingPipelineConfig : 'RaytracingPipelineConfig';
RaytracingPipelineConfig1 : 'RaytracingPipelineConfig1';
TriangleHitGroup : 'TriangleHitGroup';
ProceduralPrimitiveHitGroup : 'ProceduralPrimitiveHitGroup';

// AZSL: Allow Sampler states to be defined in the AZSL code as part of the SRG definition
ADDRESS_U : 'AddressU';
ADDRESS_V : 'AddressV';
ADDRESS_W : 'AddressW';
BORDER_COLOR : 'BorderColor';
MIN_FILTER : 'MinFilter';
MAG_FILTER : 'MagFilter';
MIP_FILTER : 'MipFilter';
MAX_ANISOTROPY : 'MaxAnisotropy';
MAX_LOD : 'MaxLOD';
MIN_LOD : 'MinLOD';
MIP_LOD_BIAS : 'MipLODBias';
COMPARISON_FUNC : 'ComparisonFunc';
REDUCTION_TYPE : 'ReductionType';

FILTER_MODE_POINT : 'Point';
FILTER_MODE_LINEAR : 'Linear';

REDUCTION_TYPE_FILTER : 'Filter';
REDUCTION_TYPE_COMPARISON : 'Comparison';
REDUCTION_TYPE_MINIMUM : 'Minimum';
REDUCTION_TYPE_MAXIMUM : 'Maximum';

ADDRESS_MODE_WRAP : 'Wrap';
ADDRESS_MODE_MIRROR : 'Mirror';
ADDRESS_MODE_CLAMP : 'Clamp';
ADDRESS_MODE_BORDER : 'Border';
ADDRESS_MODE_MIRROR_ONCE : 'MirrorOnce';

COMPARISON_FUNCTION_NEVER : 'Never';
COMPARISON_FUNCTION_LESS : 'Less';
COMPARISON_FUNCTION_EQUAL : 'Equal';
COMPARISON_FUNCTION_LESS_EQUAL : 'LessEqual';
COMPARISON_FUNCTION_GREATER : 'Greater';
COMPARISON_FUNCTION_NOT_EQUAL : 'NotEqual';
COMPARISON_FUNCTION_GREATER_EQUAL : 'GreaterEqual';
COMPARISON_FUNCTION_ALWAYS : 'Always';

BORDER_COLOR_OPAQUE_BLACK : 'OpaqueBlack';
BORDER_COLOR_TRANSPARENT_BLACK : 'TransparentBlack';
BORDER_COLOR_OPAQUE_WHITE : 'OpaqueWhite';

LeftParen : '(';
RightParen : ')';
LeftBracket : '[';
RightBracket : ']';
LeftBrace : '{';
RightBrace : '}';
LeftDoubleBracket : '[[';

Less : '<';
LessEqual : '<=';
Greater : '>';
GreaterEqual : '>=';
LeftShift : '<<';
RightShift : '>>';

Plus : '+';
PlusPlus : '++';
Minus : '-';
MinusMinus : '--';
Star : '*';
Div : '/';
Mod : '%';

And : '&';
Or : '|';
AndAnd : '&&';
OrOr : '||';
Caret : '^';
Not : '!';
Tilde : '~';

Question : '?';
Colon : ':';
ColonColon : '::';
Semi : ';';
Comma : ',';
Assign : '=';
// '*=' | '/=' | '%=' | '+=' | '-=' | '<<=' | '>>=' | '&=' | '^=' | '|='
StarAssign : '*=';
DivAssign : '/=';
ModAssign : '%=';
PlusAssign : '+=';
MinusAssign : '-=';
LeftShiftAssign : '<<=';
RightShiftAssign : '>>=';
AndAssign : '&=';
XorAssign : '^=';
OrAssign : '|=';

Equal : '==';
NotEqual : '!=';

Dot : '.';

True : 'true';
False : 'false';

// AZSL extensions
KW_AssociatedType : 'associatedtype' ;
KW_TypeAlias : 'typealias' ;
KW_Typedef : 'typedef' ;
KW_Fundamental : 'fundamental' ; // [GFX TODO]
KW_Typeof : 'typeof' ;  // simple decltype-like operator

// AZSL SRG
FrequencyId : 'FrequencyId';
ShaderVariantFallback : 'ShaderVariantFallback';
ShaderResourceGroupSemantic : 'ShaderResourceGroupSemantic';
ShaderResourceGroup : 'ShaderResourceGroup';

// AZSLc-specific internal access keywords
KW_ext_print_message : '__azslc_print_message' ;
KW_ext_print_symbol : '__azslc_print_symbol' ;
KW_ext_prtsym_fully_qualified : '__azslc_prtsym_fully_qualified' ;
KW_ext_prtsym_least_qualified : '__azslc_prtsym_least_qualified' ;
KW_ext_prtsym_constint_value : '__azslc_prtsym_constint_value' ;


HLSLSemanticStream:
    'BINORMAL'      Digit*
  | 'BLENDINDICES'  Digit*
  | 'BLENDWEIGHT'   Digit*
  | 'COLOR'         Digit*
  | 'NORMAL'        Digit*
  | 'POSITION'      Digit*
  | 'POSITIONT'
  | 'PSIZE'         Digit*
  | 'TANGENT'       Digit*
  | 'TEXCOORD'      Digit*
  | 'FOG'
  | 'TESSFACTOR'    Digit*
  | 'TEXCOORD'      Digit*
  | 'VFACE'
  | 'VPOS'          Digit*
  | 'DEPTH'         Digit*
;

HLSLSemanticSystem:
    'SV_'       (Nondigit | Digit)*
  | 'Sv_'       (Nondigit | Digit)*
  | 'sV_'       (Nondigit | Digit)*
  | 'sv_'       (Nondigit | Digit)*
;

// Those keywords are removed from the vocabulary, as of 1.8.8 (were present from 1.8.5)
// since they can be emulated with a lazy Identifier recognition in the qualifier rule
//Center : 'center';
//Indices : 'indices';   // mesh-shader parameter qualifier
//Vertices : 'vertices'; // mesh-shader parameter qualifier
//Payload : 'payload';   // mesh-shader parameter qualifier & hit-shader optional qualifier

Identifier
    :   Nondigit (Nondigit | Digit)*
    ;

fragment
Nondigit
    :   [a-zA-Z_]
    ;

fragment
Digit
    :   [0-9]
    ;

fragment
DecimalOrOctalIntegerLiteral
	:   Digit+
    ;

fragment
HexadecimalIntegerLiteral
    :   ('0x' | '0X') HexadecimalDigit+
    ;

fragment
HexadecimalDigit
    :   [0-9a-fA-F]
    ;

fragment
FractionalConstant
    :   DigitSequence? '.' DigitSequence
    |   DigitSequence '.'
    ;

fragment
ExponentPart
    :   'e' Sign? DigitSequence
    |   'E' Sign? DigitSequence
    ;

fragment
Sign
    :   '+' | '-'
    ;

fragment
DigitSequence
    :   Digit+
    ;

fragment
HexadecimalDigitSequence
    :   HexadecimalDigit+
    ;

fragment
IntegerSuffix
    :   [uU]?[lL]?[lL]?
    |   [lL]?[uU]?
    ;

IntegerLiteral
    :   DecimalOrOctalIntegerLiteral IntegerSuffix?
    |   HexadecimalIntegerLiteral IntegerSuffix?
    ;

// https://docs.microsoft.com/en-us/windows/desktop/direct3dhlsl/dx-graphics-hlsl-appendix-grammar#floating-point-numbers
fragment
FloatingSuffix
    :   [hHflFL]
    ;

FloatLiteral
    :   FractionalConstant ExponentPart? FloatingSuffix?
    |   DigitSequence ExponentPart FloatingSuffix?
    ;

fragment
EscapeSequence
    :   SimpleEscapeSequence
    ;

fragment
SimpleEscapeSequence
    :   '\\' ['"?abfnrtv\\]
    ;

StringLiteral
    :   '"' SCharSequence? '"'
    ;

fragment
SCharSequence
    :   SChar+
    ;

fragment
SChar
    :   ~["\\\r\n]
    |   EscapeSequence
    ;

PragmaDirective
    :   '#' Whitespace? 'pragma' Whitespace ~[\r\n]*
        -> channel(PREPROCESSOR)
    ;

// valid line directives examples:
// #1
// # 1
// # 1 "file"
// #line 1
// #line 1 "file"
// # line 1 "file" 2  // comment
LineDirective
    :   '#' Whitespace? ('line' Whitespace)? IntegerLiteral Whitespace? StringLiteral?
        -> channel(PREPROCESSOR)
    ;

Whitespace
    :   [ \t]+ -> channel(HIDDEN)
    ;

Newline
    :   (   '\r' '\n'?
        |   '\n'
        )  -> channel(HIDDEN)
    ;

// Amazon: The original mode switches from Tim Jones were not working.
BlockComment
    :   '/*' .*? '*/' -> channel(COMMENTS)
    ;

LineComment
    :   '//' ~[\r\n]* -> channel(COMMENTS)
    ;
