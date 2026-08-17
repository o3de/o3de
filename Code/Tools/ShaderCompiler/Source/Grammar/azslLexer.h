// Copyright (c) Contributors to the Open 3D Engine Project.
// For complete copyright and license terms please see the LICENSE at the root of this distribution.
//
// SPDX-License-Identifier: Apache-2.0 OR MIT
//

// Generated from azslLexer.g4 by ANTLR 4.13.2

#pragma once


#include "antlr4-runtime.h"




class  azslLexer : public antlr4::Lexer {
public:
  enum {
    AppendStructuredBuffer = 1, Bool = 2, Bool1 = 3, Bool2 = 4, Bool3 = 5, 
    Bool4 = 6, Bool1x1 = 7, Bool1x2 = 8, Bool1x3 = 9, Bool1x4 = 10, Bool2x1 = 11, 
    Bool2x2 = 12, Bool2x3 = 13, Bool2x4 = 14, Bool3x1 = 15, Bool3x2 = 16, 
    Bool3x3 = 17, Bool3x4 = 18, Bool4x1 = 19, Bool4x2 = 20, Bool4x3 = 21, 
    Bool4x4 = 22, Buffer = 23, BuiltInTriangleIntersectionAttributes = 24, 
    ByteAddressBuffer = 25, Break = 26, Case = 27, CBuffer = 28, Centroid = 29, 
    ConstantBuffer = 30, ConstantBufferCamel = 31, Class = 32, ColumnMajor = 33, 
    Const = 34, ConsumeStructuredBuffer = 35, Continue = 36, Default = 37, 
    Discard = 38, Do = 39, Double = 40, Double1 = 41, Double2 = 42, Double3 = 43, 
    Double4 = 44, Double1x1 = 45, Double1x2 = 46, Double1x3 = 47, Double1x4 = 48, 
    Double2x1 = 49, Double2x2 = 50, Double2x3 = 51, Double2x4 = 52, Double3x1 = 53, 
    Double3x2 = 54, Double3x3 = 55, Double3x4 = 56, Double4x1 = 57, Double4x2 = 58, 
    Double4x3 = 59, Double4x4 = 60, Else = 61, Enum = 62, Export = 63, Extern = 64, 
    FeedbackTexture2D = 65, FeedbackTexture2DArray = 66, Float = 67, Float1 = 68, 
    Float2 = 69, Float3 = 70, Float4 = 71, Float1x1 = 72, Float1x2 = 73, 
    Float1x3 = 74, Float1x4 = 75, Float2x1 = 76, Float2x2 = 77, Float2x3 = 78, 
    Float2x4 = 79, Float3x1 = 80, Float3x2 = 81, Float3x3 = 82, Float3x4 = 83, 
    Float4x1 = 84, Float4x2 = 85, Float4x3 = 86, Float4x4 = 87, For = 88, 
    Groupshared = 89, Globallycoherent = 90, Global = 91, Half = 92, Half1 = 93, 
    Half2 = 94, Half3 = 95, Half4 = 96, Half1x1 = 97, Half1x2 = 98, Half1x3 = 99, 
    Half1x4 = 100, Half2x1 = 101, Half2x2 = 102, Half2x3 = 103, Half2x4 = 104, 
    Half3x1 = 105, Half3x2 = 106, Half3x3 = 107, Half3x4 = 108, Half4x1 = 109, 
    Half4x2 = 110, Half4x3 = 111, Half4x4 = 112, If = 113, In = 114, Inline = 115, 
    Rootconstant = 116, Inout = 117, InputPatch = 118, Int = 119, Int16_t = 120, 
    Int32_t = 121, Int64_t = 122, Int1 = 123, Int2 = 124, Int3 = 125, Int4 = 126, 
    Int1x1 = 127, Int1x2 = 128, Int1x3 = 129, Int1x4 = 130, Int2x1 = 131, 
    Int2x2 = 132, Int2x3 = 133, Int2x4 = 134, Int3x1 = 135, Int3x2 = 136, 
    Int3x3 = 137, Int3x4 = 138, Int4x1 = 139, Int4x2 = 140, Int4x3 = 141, 
    Int4x4 = 142, Interface = 143, Line_ = 144, LineAdj = 145, Linear = 146, 
    LineStream = 147, Long = 148, Matrix = 149, Nointerpolation = 150, Noperspective = 151, 
    Option = 152, Out = 153, OutputPatch = 154, Override = 155, Partial = 156, 
    Packoffset = 157, Point = 158, PointStream = 159, Precise = 160, RasterizerOrderedBuffer = 161, 
    RasterizerOrderedByteAddressBuffer = 162, RasterizerOrderedStructuredBuffer = 163, 
    RasterizerOrderedTexture1D = 164, RasterizerOrderedTexture1DArray = 165, 
    RasterizerOrderedTexture2D = 166, RasterizerOrderedTexture2DArray = 167, 
    RasterizerOrderedTexture3D = 168, RayDesc = 169, RaytracingAccelerationStructure = 170, 
    Register = 171, Return = 172, RowMajor = 173, RWBuffer = 174, RWByteAddressBuffer = 175, 
    RWStructuredBuffer = 176, RWTexture1D = 177, RWTexture1DArray = 178, 
    RWTexture2D = 179, RWTexture2DArray = 180, RWTexture3D = 181, Sample = 182, 
    Sampler = 183, SamplerCapitalS = 184, SamplerComparisonState = 185, 
    SamplerStateCamel = 186, SamplerState = 187, Shared = 188, SNorm = 189, 
    Static = 190, Struct = 191, StructuredBuffer = 192, SubpassInput = 193, 
    SubpassInputMS = 194, SubpassInputDS = 195, SubpassInputDSMS = 196, 
    Switch = 197, TBuffer = 198, Texture1D = 199, Texture1DArray = 200, 
    Texture2D = 201, Texture2DArray = 202, Texture2DMS = 203, Texture2DMSArray = 204, 
    Texture3D = 205, TextureCube = 206, TextureCubeArray = 207, Triangle = 208, 
    TriangleAdj = 209, TriangleStream = 210, Uniform = 211, Uint = 212, 
    Uint1 = 213, Uint2 = 214, Uint3 = 215, Uint4 = 216, Uint1x1 = 217, Uint1x2 = 218, 
    Uint1x3 = 219, Uint1x4 = 220, Uint2x1 = 221, Uint2x2 = 222, Uint2x3 = 223, 
    Uint2x4 = 224, Uint3x1 = 225, Uint3x2 = 226, Uint3x3 = 227, Uint3x4 = 228, 
    Uint4x1 = 229, Uint4x2 = 230, Uint4x3 = 231, Uint4x4 = 232, Uint16_t = 233, 
    Uint32_t = 234, Uint64_t = 235, UNorm = 236, Unsigned = 237, Dword = 238, 
    Dword1 = 239, Dword2 = 240, Dword3 = 241, Dword4 = 242, Dword1x1 = 243, 
    Dword1x2 = 244, Dword1x3 = 245, Dword1x4 = 246, Dword2x1 = 247, Dword2x2 = 248, 
    Dword2x3 = 249, Dword2x4 = 250, Dword3x1 = 251, Dword3x2 = 252, Dword3x3 = 253, 
    Dword3x4 = 254, Dword4x1 = 255, Dword4x2 = 256, Dword4x3 = 257, Dword4x4 = 258, 
    Vector = 259, Volatile = 260, Void = 261, While = 262, StateObjectConfig = 263, 
    LocalRootSignature = 264, GlobalRootSignature = 265, SubobjectToExportsAssociation = 266, 
    RaytracingShaderConfig = 267, RaytracingPipelineConfig = 268, RaytracingPipelineConfig1 = 269, 
    TriangleHitGroup = 270, ProceduralPrimitiveHitGroup = 271, ADDRESS_U = 272, 
    ADDRESS_V = 273, ADDRESS_W = 274, BORDER_COLOR = 275, MIN_FILTER = 276, 
    MAG_FILTER = 277, MIP_FILTER = 278, MAX_ANISOTROPY = 279, MAX_LOD = 280, 
    MIN_LOD = 281, MIP_LOD_BIAS = 282, COMPARISON_FUNC = 283, REDUCTION_TYPE = 284, 
    FILTER_MODE_POINT = 285, FILTER_MODE_LINEAR = 286, REDUCTION_TYPE_FILTER = 287, 
    REDUCTION_TYPE_COMPARISON = 288, REDUCTION_TYPE_MINIMUM = 289, REDUCTION_TYPE_MAXIMUM = 290, 
    ADDRESS_MODE_WRAP = 291, ADDRESS_MODE_MIRROR = 292, ADDRESS_MODE_CLAMP = 293, 
    ADDRESS_MODE_BORDER = 294, ADDRESS_MODE_MIRROR_ONCE = 295, COMPARISON_FUNCTION_NEVER = 296, 
    COMPARISON_FUNCTION_LESS = 297, COMPARISON_FUNCTION_EQUAL = 298, COMPARISON_FUNCTION_LESS_EQUAL = 299, 
    COMPARISON_FUNCTION_GREATER = 300, COMPARISON_FUNCTION_NOT_EQUAL = 301, 
    COMPARISON_FUNCTION_GREATER_EQUAL = 302, COMPARISON_FUNCTION_ALWAYS = 303, 
    BORDER_COLOR_OPAQUE_BLACK = 304, BORDER_COLOR_TRANSPARENT_BLACK = 305, 
    BORDER_COLOR_OPAQUE_WHITE = 306, LeftParen = 307, RightParen = 308, 
    LeftBracket = 309, RightBracket = 310, LeftBrace = 311, RightBrace = 312, 
    LeftDoubleBracket = 313, Less = 314, LessEqual = 315, Greater = 316, 
    GreaterEqual = 317, LeftShift = 318, RightShift = 319, Plus = 320, PlusPlus = 321, 
    Minus = 322, MinusMinus = 323, Star = 324, Div = 325, Mod = 326, And = 327, 
    Or = 328, AndAnd = 329, OrOr = 330, Caret = 331, Not = 332, Tilde = 333, 
    Question = 334, Colon = 335, ColonColon = 336, Semi = 337, Comma = 338, 
    Assign = 339, StarAssign = 340, DivAssign = 341, ModAssign = 342, PlusAssign = 343, 
    MinusAssign = 344, LeftShiftAssign = 345, RightShiftAssign = 346, AndAssign = 347, 
    XorAssign = 348, OrAssign = 349, Equal = 350, NotEqual = 351, Dot = 352, 
    True = 353, False = 354, KW_AssociatedType = 355, KW_TypeAlias = 356, 
    KW_Typedef = 357, KW_Fundamental = 358, KW_Typeof = 359, FrequencyId = 360, 
    ShaderVariantFallback = 361, ShaderResourceGroupSemantic = 362, ShaderResourceGroup = 363, 
    KW_ext_print_message = 364, KW_ext_print_symbol = 365, KW_ext_prtsym_fully_qualified = 366, 
    KW_ext_prtsym_least_qualified = 367, KW_ext_prtsym_constint_value = 368, 
    HLSLSemanticStream = 369, HLSLSemanticSystem = 370, Identifier = 371, 
    IntegerLiteral = 372, FloatLiteral = 373, StringLiteral = 374, PragmaDirective = 375, 
    LineDirective = 376, Whitespace = 377, Newline = 378, BlockComment = 379, 
    LineComment = 380
  };

  enum {
    PREPROCESSOR = 2, COMMENTS = 3
  };

  explicit azslLexer(antlr4::CharStream *input);

  ~azslLexer() override;


  std::string getGrammarFileName() const override;

  const std::vector<std::string>& getRuleNames() const override;

  const std::vector<std::string>& getChannelNames() const override;

  const std::vector<std::string>& getModeNames() const override;

  const antlr4::dfa::Vocabulary& getVocabulary() const override;

  antlr4::atn::SerializedATNView getSerializedATN() const override;

  const antlr4::atn::ATN& getATN() const override;

  // By default the static state used to implement the lexer is lazily initialized during the first
  // call to the constructor. You can call this function if you wish to initialize the static state
  // ahead of time.
  static void initialize();

private:

  // Individual action functions triggered by action() above.

  // Individual semantic predicate functions triggered by sempred() above.

};

