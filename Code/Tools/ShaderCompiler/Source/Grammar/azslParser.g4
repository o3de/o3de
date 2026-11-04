/*
Original grammar by Tim Jones
MIT license
-
Edits by Amazon, C 2018
*/

parser grammar azslParser;

options
   { tokenVocab = azslLexer; }

// entry rule
compilationUnit:
    Declarations+=topLevelDeclaration* EOF
;

topLevelDeclaration:
        anyStructuredTypeDefinitionStatement
    |   variableDeclarationStatement
    |   attributedFunctionDefinition
    |   attributedFunctionDeclaration
    |   attributeSpecifierSequence
    |   compilerExtensionStatement      // AZSLc specific
    |   typeAliasingDefinitionStatement // AZSLc specific
    |   attributedSrgDefinition         // AZSLc specific
    |   attributedSrgSemantic           // AZSLc specific
    |   Semi
;

// Amazon: AZSL has scopes, and identifiers can be qualified
idExpression:
        unqualifiedId   // stricly unqualified (no nested specifiers at all)
    |   qualifiedId     // could be relatively qualified OR fully qualified.
;

unqualifiedId:
    Identifier
;

qualifiedId:
    nestedNameSpecifier unqualifiedId
;

nestedNameSpecifier:
    GlobalSROToken='::'? (Identifier '::')*
;

classDefinitionStatement:
    classDefinition Semi
;

classDefinition:
    Class Name=Identifier BaseListOpt=baseList?
    LeftBrace classMemberDeclaration* RightBrace
;

baseList:
    Colon idExpression (Comma idExpression)*
;

classMemberDeclaration:
        variableDeclarationStatement
    |   attributedFunctionDefinition
    |   attributedFunctionDeclaration
    |   typeAliasingDefinitionStatement  // AZSL specificity
    |   anyStructuredTypeDefinitionStatement   // Amazon extension (DXC supports it)
    |   attributeSpecifierAny // AZSL+. Allows [[pad_to(N)]].
;

structDefinitionStatement:
    structDefinition Semi
;

structDefinition:
    Struct Name=Identifier
    LeftBrace structMemberDeclaration* RightBrace
;

structMemberDeclaration:
        variableDeclarationStatement
    |   attributedFunctionDefinition //AZSL+, forbidden, but allows us to provide better error message.
    |   attributedFunctionDeclaration //AZSL+, forbidden, but allows us to provide better error message.
    |   anyStructuredTypeDefinitionStatement  // AZSL+
    |   typeAliasingDefinitionStatement // AZSL+
    |   attributeSpecifierAny // AZSL+. Allows [[pad_to(N)]].
;

anyStructuredTypeDefinitionStatement:
    attributeSpecifierAny* anyStructuredTypeDefinition Semi
;

enumDefinitionStatement:
    enumDefinition Semi
;

enumDefinition:
    enumKey Name=Identifier
    LeftBrace List=enumeratorListDefinition? RightBrace
;

enumKey:
        Enum                                # UnscopedEnum
    |   Enum (Class|Struct)                 # ScopedEnum
;

enumeratorListDefinition:
    Enumerators+=enumeratorDeclarator (',' Enumerators+=enumeratorDeclarator)* ','?
;

enumeratorDeclarator:
    Name=Identifier ('=' Value=expression)?
;

// AZSL+
anyStructuredTypeDefinition:
        classDefinition
    |   interfaceDefinition
    |   structDefinition
    |   enumDefinition
;

interfaceDefinitionStatement:
    interfaceDefinition Semi;

interfaceDefinition:
    Interface Name=Identifier
    LeftBrace interfaceMemberDeclaration* RightBrace
;

interfaceMemberDeclaration:
        attributedFunctionDeclaration
    |   associatedTypeDeclaration            // AZSL+
    |   anyStructuredTypeDefinitionStatement  // AZSL+
;

constantBufferTemplated:
    CBCoreType=(ConstantBuffer | ConstantBufferCamel) '<' GenericTypeName=type '>'
;

variableDeclarationStatement:
    variableDeclaration Semi
;

functionParams:
    Void | functionParam (Comma functionParam)*
;

functionParam:
    attributeSpecifierAny* type Name=Identifier? unnamedVariableDeclarator
;

hlslSemantic:
    Colon Name=hlslSemanticName
;

hlslSemanticName:
    HLSLSemanticStream         // Semantics related to shader stages streams
  | HLSLSemanticSystem         // Semantics related to system provided attributes
  | Identifier                 // Anything we might have missed, including new or platform-specific semantics
;

// --------------------------------------
// ATTRIBUTES
// --------------------------------------

attributeArguments:
    literal (Comma literal)*
;

attributeArgumentList:
    LeftParen attributeArguments RightParen
;

attribute:
    Global ColonColon (Namespace=Identifier ColonColon)? Name=Identifier attributeArgumentList?     # GlobalAttribute
  | (Namespace=Identifier ColonColon)? Name=Identifier attributeArgumentList?                       # AttachedAttribute
;

attributeSpecifier:
    LeftBracket attribute RightBracket
;

attributeSpecifierSequence:
    LeftDoubleBracket  Attributes+=attribute (Comma Attributes+=attribute)* RightBracket RightBracket
;

attributeSpecifierAny:
    attributeSpecifier
  | attributeSpecifierSequence
;

// --------------------------------------
// STATEMENTS
// --------------------------------------

block:
    LeftBrace Stmts+=statement* RightBrace
;

statement:
        variableDeclarationStatement
    |   embeddedStatement
    |   anyStructuredTypeDefinitionStatement
;

forInitializer:
        variableDeclaration
    |   expressionExt
;

switchLabel:
        Case Expr=expression Colon # CaseSwitchLabel
    |   Default Colon              # DefaultSwitchLabel
;

switchSection:
    switchLabel+ statement+
;

switchBlock:
    LeftBrace switchSection* RightBrace
;

embeddedStatement:
        Semi # EmptyStatement
    |   block # BlockStatement
    |   Expr=expressionExt Semi # ExpressionStatement

    // Selection statement
    |   attributeSpecifier* If LeftParen Condition=expressionExt RightParen Stmt=embeddedStatement elseClause? # IfStatement
    |   attributeSpecifier* Switch LeftParen Expr=expressionExt RightParen switchBlock # SwitchStatement

    // Iteration statement
    |   attributeSpecifier* While LeftParen Condition=expressionExt RightParen embeddedStatement # WhileStatement
    |   attributeSpecifier* Do embeddedStatement While LeftParen Condition=expressionExt RightParen Semi # DoStatement
    |   attributeSpecifier* For LeftParen forInitializer? Semi Condition=expressionExt? Semi iterator=expressionExt? RightParen embeddedStatement # ForStatement

    // Jump statement
    |   Break Semi # BreakStatement
    |   Continue Semi # ContinueStatement
    |   Discard Semi # DiscardStatement
    |   Return Expr=expressionExt? Semi # ReturnStatement

    // AZSLc specific
    |   compilerExtensionStatement # ExtenstionStatement
    |   typeAliasingDefinitionStatement # TypeAliasingDefinitionStatementLabel
;

elseClause:
    Else Stmt=embeddedStatement
;

// --------------------------------------
// EXPRESSIONS
// --------------------------------------
expression:
        literal                                                                                 # LiteralExpression
    |   idExpression                                                                            # IdentifierExpression
    |   LeftParen Expr=expressionExt RightParen                                                 # ParenthesizedExpression
    |   LHSExpr=expression DotToken=Dot Member=idExpression                                     # MemberAccessExpression
    |   Expr=expression LeftBracket Index=expression RightBracket                               # ArrayAccessExpression
    |   Expr=expression argumentList                                                            # FunctionCallExpression
    |   scalarOrVectorOrMatrixType argumentList                                                 # NumericConstructorExpression
    |<assoc=right> LeftParen type (ArrayRankSpecifiers+=arrayRankSpecifier)* RightParen Expr=expression  # CastExpression
    |   Expr=expression Operator=postfixUnaryOperator                                           # PostfixUnaryExpression
    |<assoc=right> Operator=prefixUnaryOperator Expr=expression                                 # PrefixUnaryExpression
    |   Left=expression Operator=binaryOperator Right=expression                                # BinaryExpression
    |<assoc=right> Condition=expression Question TrueExpr=expressionExt Colon FalseExpr=expressionExt # ConditionalExpression  // ternary
    |<assoc=right> Left=expression Operator=assignmentOperator Right=expressionExt              # AssignmentExpression
;

// comma-expr are problematic because the parser is greedy,
// and it catches comma-expr situations in undesired places. like function arguments or enum declarations.
// therefore we need to cut in two categories, to restrict these situations.
// all expressions + comma-expressions
expressionExt:
        Expr=expression                                       # OtherExpression
    |   Left=expressionExt Operator=Comma Right=expression    # CommaExpression
;

postfixUnaryOperator:
        PlusPlus
    |   MinusMinus
;

prefixUnaryOperator:
        Plus
    |   Minus
    |   Not
    |   Tilde
    |   PlusPlus
    |   MinusMinus
;

binaryOperator:
        Star
    |   Div
    |   Mod
    |   Plus
    |   Minus
    |   LeftShift
    |   RightShift
    |   Less
    |   Greater
    |   LessEqual
    |   GreaterEqual
    |   Equal
    |   NotEqual
    |   And
    |   Caret
    |   Or
    |   AndAnd
    |   OrOr
;

assignmentOperator:
        Assign
    |   StarAssign
    |   DivAssign
    |   ModAssign
    |   PlusAssign
    |   MinusAssign
    |   LeftShiftAssign
    |   RightShiftAssign
    |   AndAssign
    |   XorAssign
    |   OrAssign
;

argumentList:
    LeftParen arguments? RightParen
;

arguments:
    expression (Comma expression)*
;

// --------------------------------------
// TYPES
// --------------------------------------

variableDeclaration:
    attributeSpecifierAny* type variableDeclarators
;

variableDeclarators:
    VarDecls+=namedVariableDeclarator (Comma VarDecls+=namedVariableDeclarator)*
;

unnamedVariableDeclarator:
    (ArrayRankSpecifiers+=arrayRankSpecifier)*
    SemanticOpt=hlslSemantic?
    packOffsetNode?
    RegisterAllocation=registerAllocation?
    variableInitializer?
;

namedVariableDeclarator:
    Name=Identifier
    unnamedVariableDeclarator
;

variableInitializer:
        '=' standardVariableInitializer
    |   samplerBodyDeclaration
;

standardVariableInitializer:
        '{' arrayElementInitializers '}'
    |   Expr=expression
;

arrayElementInitializers:
    standardVariableInitializer (Comma standardVariableInitializer)* Comma?
;

arrayRankSpecifier:
    LeftBracket Dimension=expression? RightBracket
;

packOffsetNode:
    Colon PackoffsetKeyword=Packoffset LeftParen
    PackOffsetRegister=Identifier (Dot PackOffsetComponent=Identifier)?
    RightParen
;

storageFlags:
    storageFlag*?    // *? lazy Kleene star. this allows the use of Identifier as a subrule without swallowing typenames under the storage rule.
;

storageFlag:
    // Type modifiers
        Const
    |   Unsigned
    |   RowMajor
    |   ColumnMajor
    // Storage classes
    |   Extern
    |   Inline
    |   Rootconstant   // AZSL specific
    |   Option         // AZSL
    |   Precise
    |   Shared
    |   Groupshared
    |   Static
    |   Uniform
    |   Volatile
    |   Globallycoherent
    |   SNorm
    |   UNorm
    // Interpolation modifiers
    |   Linear
    |   Centroid
    |   Nointerpolation
    |   Noperspective
    |   Sample
    // Parameter modifiers (only valid on function params)
    |   In
    |   Out
    |   Inout
    // Geometry shader primitive type
    |   Point
    |   Line_
    |   Triangle
    |   LineAdj
    |   TriangleAdj
    // catch-all
    |   Identifier
;

// note that in FXC "int static" (that is `storageFlags type storageFlags`) wasn't valid.
// but in DXC it's flexible like in C/C++, so we have a restriction compared to DXC here.
type:
	storageFlags (predefinedType | userDefinedType | typeofExpression | Void)
;

predefinedType:
        bufferPredefinedType
    |   byteAddressBufferTypes
    |   patchPredefinedType
    |   matrixType
    |   genericMatrixPredefinedType
    |   samplerStatePredefinedType
    |   scalarType
    |   streamOutputPredefinedType
    |   structuredBufferPredefinedType
    |   texturePredefinedType
    |   genericTexturePredefinedType
    |   genericSubpassInputPredefinedType
    |   msTexturePredefinedType
    |   subpassInputPredefinedType
    |   vectorType
    |   genericVectorType
    |   constantBufferTemplated   // needed here to get recognized as a predefined type
    |   otherViewResourceType
    |   subobjectType
    |   rtxBuiltInTypes
;

subobjectType:
        StateObjectConfig
    |   LocalRootSignature
    |   GlobalRootSignature
    |   SubobjectToExportsAssociation
    |   RaytracingShaderConfig
    |   RaytracingPipelineConfig
    |   RaytracingPipelineConfig1
    |   TriangleHitGroup
    |   ProceduralPrimitiveHitGroup
;

// Acceleration structures
otherViewResourceType:
    RaytracingAccelerationStructure
;

rtxBuiltInTypes:
        BuiltInTriangleIntersectionAttributes
    |   RayDesc
;

bufferPredefinedType:
    bufferType Less scalarOrVectorOrMatrixType Greater
;

bufferType:
        Buffer
    |   RWBuffer
    |   RasterizerOrderedBuffer
;

byteAddressBufferTypes:
        ByteAddressBuffer
    |   RWByteAddressBuffer
    |   RasterizerOrderedByteAddressBuffer
;

patchPredefinedType:
    patchType Less
    Name=userDefinedType Comma ControlPoints=IntegerLiteral
    Greater
;

patchType:
        InputPatch
    |   OutputPatch
;

samplerStatePredefinedType:
        Sampler
    |   SamplerCapitalS
    |   SamplerState
    |   SamplerStateCamel
    |   SamplerComparisonState
;

scalarType:
        Bool
    |   Int
    |   Int16_t
    |   Int32_t
    |   Int64_t
    |   Uint
    |   Uint16_t
    |   Uint32_t
    |   Uint64_t
    |   Dword
    |   Half
    |   Float
    |   Double
;

streamOutputPredefinedType:
    streamOutputObjectType Less type Greater
;

streamOutputObjectType:
        PointStream
    |   LineStream
    |   TriangleStream
;

structuredBufferPredefinedType:
    structuredBufferName Less type Greater
;

structuredBufferName:
        AppendStructuredBuffer
    |   ConsumeStructuredBuffer
    |   RWStructuredBuffer
    |   StructuredBuffer
    |   RasterizerOrderedStructuredBuffer
;

textureType:
        Texture1D
    |   Texture1DArray
    |   RasterizerOrderedTexture1D
    |   RasterizerOrderedTexture1DArray
    |   Texture2D
    |   Texture2DArray
    |   RasterizerOrderedTexture2D
    |   RasterizerOrderedTexture2DArray
    |   Texture3D
    |   RasterizerOrderedTexture3D
    |   TextureCube
    |   TextureCubeArray
    |   RWTexture1D
    |   RWTexture1DArray
    |   RWTexture2D
    |   RWTexture2DArray
    |   RWTexture3D
;

texturePredefinedType:
    textureType
;

genericTexturePredefinedType:
    textureType Less scalarOrVectorType Greater
;

textureTypeMS:
        Texture2DMS
    |   Texture2DMSArray
;

msTexturePredefinedType:
    textureTypeMS Less scalarOrVectorType (Comma Samples=IntegerLiteral)? Greater
;

subpassInputType:    
        SubpassInput
    |   SubpassInputDS
    |   SubpassInputMS
    |   SubpassInputDSMS
;

subpassInputPredefinedType:
    subpassInputType
;

genericSubpassInputPredefinedType:
    subpassInputType Less scalarOrVectorType Greater
;

vectorType:
        Vector
    |   Bool1
    |   Bool2
    |   Bool3
    |   Bool4
    |   Int1
    |   Int2
    |   Int3
    |   Int4
    |   Uint1
    |   Uint2
    |   Uint3
    |   Uint4
    |   Dword1
    |   Dword2
    |   Dword3
    |   Dword4
    |   Half1
    |   Half2
    |   Half3
    |   Half4
    |   Float1
    |   Float2
    |   Float3
    |   Float4
    |   Double1
    |   Double2
    |   Double3
    |   Double4
;

genericVectorType:
    Vector Less scalarType Comma Size_=IntegerLiteral Greater
;

scalarOrVectorType:
        scalarType
    |   vectorType
;

scalarOrVectorOrMatrixType:
        scalarType
    |   vectorType
    |   matrixType
;

matrixType:
        Matrix
    |   Bool1x1
    |   Bool1x2
    |   Bool1x3
    |   Bool1x4
    |   Bool2x1
    |   Bool2x2
    |   Bool2x3
    |   Bool2x4
    |   Bool3x1
    |   Bool3x2
    |   Bool3x3
    |   Bool3x4
    |   Bool4x1
    |   Bool4x2
    |   Bool4x3
    |   Bool4x4
    |   Int1x1
    |   Int1x2
    |   Int1x3
    |   Int1x4
    |   Int2x1
    |   Int2x2
    |   Int2x3
    |   Int2x4
    |   Int3x1
    |   Int3x2
    |   Int3x3
    |   Int3x4
    |   Int4x1
    |   Int4x2
    |   Int4x3
    |   Int4x4
    |   Uint1x1
    |   Uint1x2
    |   Uint1x3
    |   Uint1x4
    |   Uint2x1
    |   Uint2x2
    |   Uint2x3
    |   Uint2x4
    |   Uint3x1
    |   Uint3x2
    |   Uint3x3
    |   Uint3x4
    |   Uint4x1
    |   Uint4x2
    |   Uint4x3
    |   Uint4x4
    |   Dword1x1
    |   Dword1x2
    |   Dword1x3
    |   Dword1x4
    |   Dword2x1
    |   Dword2x2
    |   Dword2x3
    |   Dword2x4
    |   Dword3x1
    |   Dword3x2
    |   Dword3x3
    |   Dword3x4
    |   Dword4x1
    |   Dword4x2
    |   Dword4x3
    |   Dword4x4
    |   Half1x1
    |   Half1x2
    |   Half1x3
    |   Half1x4
    |   Half2x1
    |   Half2x2
    |   Half2x3
    |   Half2x4
    |   Half3x1
    |   Half3x2
    |   Half3x3
    |   Half3x4
    |   Half4x1
    |   Half4x2
    |   Half4x3
    |   Half4x4
    |   Float1x1
    |   Float1x2
    |   Float1x3
    |   Float1x4
    |   Float2x1
    |   Float2x2
    |   Float2x3
    |   Float2x4
    |   Float3x1
    |   Float3x2
    |   Float3x3
    |   Float3x4
    |   Float4x1
    |   Float4x2
    |   Float4x3
    |   Float4x4
    |   Double1x1
    |   Double1x2
    |   Double1x3
    |   Double1x4
    |   Double2x1
    |   Double2x2
    |   Double2x3
    |   Double2x4
    |   Double3x1
    |   Double3x2
    |   Double3x3
    |   Double3x4
    |   Double4x1
    |   Double4x2
    |   Double4x3
    |   Double4x4
;

genericMatrixPredefinedType:
    Matrix Less scalarType Comma
    Rows_=IntegerLiteral Comma Cols_=IntegerLiteral
    Greater
;

registerAllocation:
    Colon Register LeftParen Address=Identifier RightParen
;

samplerStateProperty:
    Name=Identifier EqualsToken=Assign Expr=expression Semi
;

literal:
        True
    |   False
    |   FloatLiteral
    |   IntegerLiteral
    |   StringLiteral+
;

// leading type means a function where the return type is stated first (contrary to trailing type)
leadingTypeFunctionSignature:
    type (ClassName=userDefinedType ColonColon)? Name=Identifier
    genericParameterList?
    LeftParen functionParams? RightParen
    Override? hlslSemantic?  // AZSL+
;

hlslFunctionDefinition:
    leadingTypeFunctionSignature
    block
;

hlslFunctionDeclaration:
    leadingTypeFunctionSignature
    Semi
;

userDefinedType:
        idExpression
    |   anyStructuredTypeDefinition   // to allow "struct tag_ {} var;"
;

// ====================
// == AZSL specifics ==

// swift/slang concept of protocol's "virtual type"
associatedTypeDeclaration:
    KW_AssociatedType Name=Identifier genericConstraint? Semi
;

// typedef support (extension of fxc accepted language, but normal for dxc)
typedefStatement:
    KW_Typedef ExistingType=type NewTypeName=Identifier Semi
;

// swift/slang-like manner of writing typedef. also close to C++11 using
typealiasStatement:
    KW_TypeAlias NewTypeName=Identifier '=' ExistingType=type Semi
;

typeAliasingDefinitionStatement:
    typealiasStatement | typedefStatement
;

typeofExpression:
    KW_Typeof '(' (Expr=expressionExt|type) ')' ('::' SubQualification=idExpression)?
;

genericParameterList:
    '<' genericTypeDefinition (',' genericTypeDefinition)* '>'
;

genericTypeDefinition:
    GenericTypeName=Identifier genericConstraint?
;

// defined as its own rule, the language becomes automatically extensible if we want to extend
// it later, to an expression, for protocol set operations: (ILight & IColor)
genericConstraint:
    ':' userDefinedType
// |   ':' languageDefinedConstraint // [GFX TODO]
;

languageDefinedConstraint:
    KW_Fundamental
;

functionDeclaration:
    hlslFunctionDeclaration
;

attributedFunctionDeclaration:
    attributeSpecifierAny* functionDeclaration
;

functionDefinition:
    hlslFunctionDefinition
;

attributedFunctionDefinition:
    attributeSpecifierAny* functionDefinition
;

// special debugging intrinsics of the compiler
compilerExtensionStatement:
        KW_ext_print_message '(' Message=StringLiteral ')' Semi
    |   KW_ext_print_symbol '(' (idExpression|typeofExpression) ',' (KW_ext_prtsym_fully_qualified
                                                                   | KW_ext_prtsym_least_qualified
                                                                   | KW_ext_prtsym_constint_value) ')' Semi
;

// AZSL SRG

srgDefinition:
    Partial? ShaderResourceGroup Name=Identifier (':' Semantic=Identifier)?
    LeftBrace srgMemberDeclaration* RightBrace
;

attributedSrgDefinition:
    attributeSpecifierAny* srgDefinition
;

srgMemberDeclaration:
        structDefinitionStatement
    |   attributedFunctionDeclaration
    |   attributedFunctionDefinition
    |   variableDeclarationStatement
    |   enumDefinitionStatement
    |   typeAliasingDefinitionStatement
    |   attributeSpecifierAny // Allows [[pad_to(N)]].
;

srgSemantic:
    ShaderResourceGroupSemantic Name=Identifier srgSemanticBodyDeclaration
;

attributedSrgSemantic:
    attributeSpecifierAny* srgSemantic
;

srgSemanticBodyDeclaration:
    LeftBrace srgSemanticMemberDeclaration* RightBrace
;

srgSemanticMemberDeclaration:
        Frequency=FrequencyId '=' FrequencyValue=literal Semi
    |   VariantFallback=ShaderVariantFallback '=' VariantFallbackValue=literal Semi
;

samplerBodyDeclaration:
    LeftBrace samplerMemberDeclaration* RightBrace
;

samplerMemberDeclaration:
        maxAnisotropyOption
    |   minFilterOption
    |   magFilterOption
    |   mipFilterOption
    |   reductionTypeOption
    |   comparisonFunctionOption
    |   addressUOption
    |   addressVOption
    |   addressWOption
    |   minLodOption
    |   maxLodOption
    |   mipLodBiasOption
    |   borderColorOption
;

// sampler specific options
maxAnisotropyOption: MAX_ANISOTROPY '=' IntegerLiteral ';';
minFilterOption: MIN_FILTER '=' filterModeEnum ';';
magFilterOption: MAG_FILTER '=' filterModeEnum ';';
mipFilterOption: MIP_FILTER '=' filterModeEnum ';';
reductionTypeOption: REDUCTION_TYPE '=' reductionTypeEnum ';';
comparisonFunctionOption: COMPARISON_FUNC '=' comparisonFunctionEnum ';';
addressUOption: ADDRESS_U '=' addressModeEnum ';';
addressVOption: ADDRESS_V '=' addressModeEnum ';';
addressWOption: ADDRESS_W '=' addressModeEnum ';';
minLodOption: MIN_LOD '=' FloatLiteral ';';
maxLodOption: MAX_LOD '=' FloatLiteral ';';
mipLodBiasOption: MIP_LOD_BIAS '=' FloatLiteral ';';
borderColorOption: BORDER_COLOR '=' borderColorEnum ';';

filterModeEnum: FILTER_MODE_POINT | FILTER_MODE_LINEAR;
reductionTypeEnum: REDUCTION_TYPE_FILTER | REDUCTION_TYPE_COMPARISON | REDUCTION_TYPE_MINIMUM | REDUCTION_TYPE_MAXIMUM;
addressModeEnum: ADDRESS_MODE_WRAP | ADDRESS_MODE_MIRROR | ADDRESS_MODE_CLAMP | ADDRESS_MODE_BORDER | ADDRESS_MODE_MIRROR_ONCE;
comparisonFunctionEnum:
  COMPARISON_FUNCTION_NEVER |
  COMPARISON_FUNCTION_LESS |
  COMPARISON_FUNCTION_EQUAL |
  COMPARISON_FUNCTION_LESS_EQUAL |
  COMPARISON_FUNCTION_GREATER |
  COMPARISON_FUNCTION_NOT_EQUAL |
  COMPARISON_FUNCTION_GREATER_EQUAL |
  COMPARISON_FUNCTION_ALWAYS;
borderColorEnum: BORDER_COLOR_OPAQUE_BLACK | BORDER_COLOR_TRANSPARENT_BLACK | BORDER_COLOR_OPAQUE_WHITE;
