/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include "AzslcUtils.h"

namespace AZ::ShaderCompiler
{
    // type classes before canonicalization.
    // post canonicalization, you can't have generic arithmetic nor typeof.
    MAKE_REFLECTABLE_ENUM (TypeClass,
        // Error case
        IsNotType,
        // The void type
        Void,
        // Predefined
        Scalar,
        Vector,
        GenericVector,  // vector<t,d>
        Matrix,
        GenericMatrix,  // matrix<t,d1,d2>
        Texture,
        GenericTexture,
        MultisampledTexture,
        SubpassInput,
        GenericSubpassInput,
        Sampler,
        StructuredBuffer,
        Buffer,
        ByteAddressBuffer,
        ConstantBuffer,
        StreamOutput,
        LibrarySubobject,
        OtherViewBufferType,
        OtherPredefined,  // IO patch
        // Not predefined
        Struct,
        Class,
        Interface,
        Enum,
        TypeofExpression,
        Alias
    );

    // == category queries ==

    inline bool IsUserDefined(TypeClass typeClass)
    {
        return typeClass.IsOneOf(TypeClass::Struct, TypeClass::Class, TypeClass::Interface, TypeClass::Enum);
    }

    //! a product type is a (non-union) 'structured' type. (Cartesian product in type theory)
    inline bool IsProductType(TypeClass typeClass)
    {
        return typeClass.IsOneOf(TypeClass::Struct, TypeClass::Class, TypeClass::Interface);
    }

    // predefined types are all HLSL base types (void included)
    inline bool IsPredefinedType(TypeClass typeClass)
    {
        return !typeClass.IsOneOf(TypeClass::IsNotType, TypeClass::TypeofExpression, TypeClass::Alias)
            && !IsUserDefined(typeClass);
    }

    // an arithmetic type, but not non-generic
    inline bool IsNonGenericArithmetic(TypeClass typeClass)
    {
        return typeClass.IsOneOf(TypeClass::Scalar, TypeClass::Vector, TypeClass::Matrix);
    }

    // has a generic to canonicalize
    inline bool IsGenericArithmetic(TypeClass typeClass)
    {
        return typeClass.IsOneOf(TypeClass::GenericVector, TypeClass::GenericMatrix);
    }

    inline bool IsArithmetic(TypeClass typeClass)
    {
        return IsGenericArithmetic(typeClass) || IsNonGenericArithmetic(typeClass);
    }

    // a fundamental is void or arithmetic
    inline bool IsFundamental(TypeClass typeClass)
    {
        return typeClass == TypeClass::Void || IsArithmetic(typeClass);
    }

    // type that behaves like its generic parameter
    inline bool IsChameleon(TypeClass typeClass)
    {  // IO patch are not strict chameleon because they behave like an array. if we consider array collapsing then maybe they are.
        return typeClass.IsOneOf(TypeClass::StructuredBuffer, TypeClass::Buffer, TypeClass::StreamOutput, TypeClass::ConstantBuffer);
    }

    inline bool HasGenericParameter(TypeClass typeClass)
    { // TODO: to add InputPath/OutputPatch because it has a generic parameter. (when you do it, update ExtractGenericTypeParameterNameFromAstContext)
        return IsChameleon(typeClass) || IsGenericArithmetic(typeClass) || typeClass.IsOneOf(TypeClass::GenericTexture, TypeClass::MultisampledTexture, TypeClass::GenericSubpassInput);
    }

    inline bool IsViewTypeBuffer(TypeClass typeClass)
    {
        return typeClass.IsOneOf(TypeClass::ConstantBuffer, TypeClass::StructuredBuffer, TypeClass::Buffer, TypeClass::ByteAddressBuffer, TypeClass::OtherViewBufferType);
    }

    inline bool IsViewType(TypeClass typeClass)
    {   // note that a constant buffer is not a view type
        return IsViewTypeBuffer(typeClass)
            || typeClass.IsOneOf(TypeClass::Texture, TypeClass::GenericTexture, TypeClass::MultisampledTexture, TypeClass::Sampler, TypeClass::SubpassInput, TypeClass::GenericSubpassInput);
    }

    //Example Texture2D m_myTex[]; is supported.
    inline bool CanBeDeclaredAsUnboundedArray(TypeClass typeClass)
    {
        return IsViewType(typeClass);
    }

    // Get TypeClass for type inside a predefined context
    inline TypeClass AnalyzeTypeClass(azslParser::PredefinedTypeContext* predefinedNode)
    {
        TypeClass toReturn = TypeClass::OtherPredefined;  // default value if nothing of the under was non null.
        using PredefinedNodeT = std::remove_pointer_t<decltype(predefinedNode)>;  // DRY
        // map TypeClasses to the same indexes than the context functions list
        array<TypeClass, 18> contextClasses = { TypeClass::Scalar,
                                                TypeClass::Vector,
                                                TypeClass::GenericVector,
                                                TypeClass::Matrix,
                                                TypeClass::GenericMatrix,
                                                TypeClass::Texture,
                                                TypeClass::MultisampledTexture,
                                                TypeClass::SubpassInput,
                                                TypeClass::GenericTexture,
                                                TypeClass::GenericSubpassInput,
                                                TypeClass::Sampler,
                                                TypeClass::StructuredBuffer,
                                                TypeClass::Buffer,
                                                TypeClass::ByteAddressBuffer,
                                                TypeClass::ConstantBuffer,
                                                TypeClass::StreamOutput,
                                                TypeClass::OtherViewBufferType,
                                                TypeClass::LibrarySubobject};
        // create a constexpr valuelist of member function pointers
        using FunctionList = ValueTplList< &PredefinedNodeT::scalarType,
                                           &PredefinedNodeT::vectorType,
                                           &PredefinedNodeT::genericVectorType,
                                           &PredefinedNodeT::matrixType,
                                           &PredefinedNodeT::genericMatrixPredefinedType,
                                           &PredefinedNodeT::texturePredefinedType,
                                           &PredefinedNodeT::msTexturePredefinedType,
                                           &PredefinedNodeT::subpassInputPredefinedType,
                                           &PredefinedNodeT::genericTexturePredefinedType,
                                           &PredefinedNodeT::genericSubpassInputPredefinedType,
                                           &PredefinedNodeT::samplerStatePredefinedType,
                                           &PredefinedNodeT::structuredBufferPredefinedType,
                                           &PredefinedNodeT::bufferPredefinedType,
                                           &PredefinedNodeT::byteAddressBufferTypes,
                                           &PredefinedNodeT::constantBufferTemplated,
                                           &PredefinedNodeT::streamOutputPredefinedType,
                                           &PredefinedNodeT::otherViewResourceType,
                                           &PredefinedNodeT::subobjectType>;
        static_assert( countTemplateParameters_v<FunctionList> == contextClasses.size() );
        // This meta-loop will be unfolded at build time, therefore the generic lambda will be instantiated individually (by each type)
        ForEachValue<FunctionList>([&toReturn, &predefinedNode, &contextClasses](auto ctxMemFn, auto index)
                                    {
                                        if ((predefinedNode->*ctxMemFn)())  // pointer-to-member-function call.
                                        {
                                            toReturn = contextClasses[index];
                                        }
                                    });
        return toReturn;
    }

    // Get TypeClass for type inside a type context
    inline TypeClass AnalyzeTypeClass(AstType* typeNode)
    {
        TypeClass toReturn = TypeClass::IsNotType;
        if (typeNode != nullptr)
        {
            if (typeNode->predefinedType())
            {
                toReturn = AnalyzeTypeClass(typeNode->predefinedType());
            }
            else if (typeNode->userDefinedType() && typeNode->userDefinedType()->anyStructuredTypeDefinition())
            {
                if (typeNode->userDefinedType()->anyStructuredTypeDefinition()->classDefinition())
                {
                    toReturn = TypeClass::Class;
                }
                else if (typeNode->userDefinedType()->anyStructuredTypeDefinition()->interfaceDefinition())
                {
                    toReturn = TypeClass::Interface;
                }
                else if (typeNode->userDefinedType()->anyStructuredTypeDefinition()->structDefinition())
                {
                    toReturn = TypeClass::Struct;
                }
                else if (typeNode->userDefinedType()->anyStructuredTypeDefinition()->enumDefinition())
                {
                    toReturn = TypeClass::Enum;
                }
            }
            else if (typeNode->typeofExpression())
            {
                toReturn = TypeClass::TypeofExpression;
            }
            else if (typeNode->Void())
            {
                toReturn = TypeClass::Void;
            }
        }
        return toReturn;
    }

    // Analyze a type (by parsing it) and return what class it belongs to.
    // Don't let too wild uncontrolled input sneak in there, there may be vectors of attack.
    inline TypeClass AnalyzeTypeClass(string_view typeName)
    {
        assert(typeName.find("/") == string::npos);  // forgot to unmangle ? use the TentativeName overload in these cases
        // Construct a mini program of the form "type a();" and check the AST.
        // Deduce the type class from the node.
        string miniprogram { typeName };
        miniprogram += " a();";
        ANTLRInputStream input(miniprogram);
        azslLexer lexer(&input);
        lexer.removeErrorListeners();
        CommonTokenStream tokens(&lexer);
        azslParser parser(&tokens);
        parser.removeErrorListeners();
        azslParser::CompilationUnitContext* unit = parser.compilationUnit();
        bool failCondition = parser.getNumberOfSyntaxErrors() > 0
                          || unit->Declarations.empty()
                          || unit->Declarations[0]->attributedFunctionDeclaration() == nullptr
                          || unit->Declarations[0]->attributedFunctionDeclaration()->functionDeclaration() == nullptr
                          || unit->Declarations[0]->attributedFunctionDeclaration()->functionDeclaration()->hlslFunctionDeclaration() == nullptr;
        AstType* typeNode = nullptr;
        if (!failCondition)
        {
            typeNode = unit->Declarations[0]->
                            attributedFunctionDeclaration()->
                                functionDeclaration()->
                                    hlslFunctionDeclaration()->
                                        leadingTypeFunctionSignature()->
                                            type();
        }
        return AnalyzeTypeClass(typeNode);
    }

    struct TentativeName
    {
        string mangled;
    };
    // try to analyze a type with a few iterations to remove mangling while the result is NotAType
    inline TypeClass AnalyzeTypeClass(TentativeName typeName)
    {
        string try1 = UnMangle(typeName.mangled);
        TypeClass result = AnalyzeTypeClass(try1);
        if (result == TypeClass::IsNotType)
        {
            // this version does not preserve the global one, this can help for fundamentals
            string try2 = ReplaceSeparators(typeName.mangled, "::");
            result = AnalyzeTypeClass(try2);
        }
        return result;
    }

    /// Verifies that our hardcoded strings don't have typo, by checking against the lexer-extracted keywords stored in the Scalar array.
    inline bool CheckExistScalarType(string_view scalarName)
    {
        return std::find(Predefined::Scalar.begin(),
                         Predefined::Scalar.end(),
                         scalarName) != Predefined::Scalar.end();
    };

    /// Assert validity of the type string, and form a "?<type>" tainted string to host a scalar type
    inline QualifiedName MangleScalarType(string_view typeName)
    {
        assert(CheckExistScalarType(typeName));
        return QualifiedName{"?" + string{typeName}};
    };

    //! Holds arithmetic-type-class information in small pieces (row, cols, base, size, rank...)
    struct ArithmeticTraits
    {
        void ResolveBaseSizeAndRank()
        {
            m_baseSize = Packing::PackedSizeof(m_underlyingScalar);
            // establish the conversion rank:
            auto getIndex = [](string_view s) -> int
            {
                auto const& Scalars = Predefined::Scalar;
                return static_cast<int>(::std::distance(Scalars.begin(), ::std::find(Scalars.begin(), Scalars.end(), s)));
            };
            // According to https://en.cppreference.com/w/cpp/language/usual_arithmetic_conversions
            //   - No two signed have the same rank (even if same siezeof)
            //   - rank of unsigned = rank of corresponding signed
            //   - "standard" is > "extended" of same sizeof
            //   - The rank of bool is the smallest
            // That said, we will take inspiration from ASTContext::getIntegerRank of clang
            static const unordered_map<int, int> subranks =
            {
                {getIndex("bool"), 1},
                {getIndex("int16_t"), 2},
                {getIndex("uint16_t"), 3}, // unsigned wins in case of subrank draw, according to arithmetic conversion rules
                {getIndex("int"), 4},
                {getIndex("uint"), 5},
                {getIndex("dword"), 6},
                {getIndex("int32_t"), 7},
                {getIndex("uint32_t"), 8},
                {getIndex("int64_t"), 9},
                {getIndex("uint64_t"), 10},
                {getIndex("half"), 11 << 5},  // floats win all conversions, even halfs
                {getIndex("float"), 12 << 5},
                {getIndex("double"), 13 << 5},
            };
            // `basesize` getter, but 1 for bool: (physical size of extern bool is considered 32bits in HLSL)
            auto getRankSizeof = [&](int scalarId)
            {
                assert(string_view{"bool"} == Predefined::Scalar[0]);  // verify that 0 is the hard index of bool.
                bool isBool = scalarId == 0;
                return isBool ? 1 : m_baseSize;
            };
            // The shift method is taken from clang, I suppose it's a multi-parameter order cramed into bits.
            // so because 10 is the largest subrank, shift by 4 should separate sizeof space and subrank space.
            m_conversionRank = (getRankSizeof(m_underlyingScalar) << 4) + subranks.at(m_underlyingScalar);
        }

        //! Get the size of the whole type considering dimensions
        uint32_t GetTotalSize() const
        {
            return m_baseSize * (m_cols > 0 ? m_cols : 1) * (m_rows > 0 ? m_rows : 1);
        }

        //! True if the type is a vector type. If it's a vector type it cannot be a matrix as well.
        bool IsVector() const
        {
            // This treats special cases like 2x1, 3x1 and 4x1 as vectors
            // The behavior is consistent with dxc packing rules
            return (m_cols == 1 && m_rows > 1) || (m_cols > 1 && m_rows == 0);
        }

        //! True if the type is a matrix type. If it's a matrix type it cannot be a vector as well.
        bool IsMatrix() const
        {
            // This fails special cases like 2x1, 3x1 and 4x1,
            //   but allows cases like 1x2, 1x3 and 1x4.
            // The behavior is consistent with dxc packing rules
            return m_rows > 0 && m_cols > 1;
        }

        bool IsScalar() const
        {
            return m_rows <= 1 && m_cols <= 1;  // float, float1, float1x1 are scalars.
        }

        //! Non-created state
        bool IsEmpty() const
        {
            return m_underlyingScalar == -1;
        }

        //! For pretty print
        string UnderlyingScalarToStr() const
        {
            return m_underlyingScalar >= 0 && m_underlyingScalar < AZ::ShaderCompiler::Predefined::Scalar.size() ?
                AZ::ShaderCompiler::Predefined::Scalar[m_underlyingScalar] : "<NA>";
        }

        //! Create a canonicalized mangled name that should represent the identity of this arithmetic type.
        QualifiedName GenTypeId() const
        {
            if (IsMatrix())
            {
                return QualifiedName{MangleScalarType(UnderlyingScalarToStr()) + ToString(m_rows) + "x" + ToString(m_cols)};
            }
            else if (IsVector())
            {
                return QualifiedName{MangleScalarType(UnderlyingScalarToStr()) + (m_rows > 0 ? ToString(m_rows) : ToString(m_cols))};
            }
            else
            {
                return QualifiedName{MangleScalarType(UnderlyingScalarToStr())};
            }
        }

        uint32_t m_baseSize = 0;                 //< In bytes. Size of 0 indicates TypeRefInfo which hasn't been resolved or is a struct
        uint32_t m_rows = 0;                     //< 0 means it's not a matrix (effective Rows = 1). 1 or more means a Matrix
        uint32_t m_cols = 0;                     //< 0 means it's not a vector (effective Cols = 1). 1 or more means a Vector or Matrix
        int      m_conversionRank = 0;           //< Used in conversions and promotions
        int      m_underlyingScalar = -1;        //< Index into AZ::ShaderCompiler::Predefined::Scalar, all fundamentals end up in a scalar at its leaf.
    };

    //! TypeRefInfo holds resolved immutable information of a core type (the `matrix2x2` in `column_major matrix2x2 a[3];`)
    //! Its own id (containing mangled core name)
    //! A type class (scalar, matrix, buffer, UDT...)
    //! Information around the fundamental if applicable (not UDT).
    struct TypeRefInfo
    {
        TypeRefInfo() = default;
        TypeRefInfo(IdentifierUID typeId, const ArithmeticTraits& fundamentalInfo, TypeClass typeClass)
            : m_arithmeticInfo{fundamentalInfo},
              m_typeClass{typeClass},
              m_typeId{typeId}
        {}

        //! is plain data: sum type or fundamental. but not enum; because we don't know what underlying they have.
        const bool IsPackable() const
        {
            return m_typeClass.IsOneOf(TypeClass::Struct, TypeClass::Class)
                || !m_arithmeticInfo.IsEmpty();
        }

        //! non assigned TypeRefInfo
        bool IsEmpty() const
        {
            return m_typeId.m_name.empty();
        }

        //! if the type is a SubpassInput, it's an input attachment
        bool IsInputAttachment(const azslLexer* lexer) const
        {
            return IsViewType(m_typeClass)
                 && (  m_typeId.GetNameLeaf() == Trim(lexer->getVocabulary().getLiteralName(azslLexer::SubpassInput), "\'")
                    || m_typeId.GetNameLeaf() == Trim(lexer->getVocabulary().getLiteralName(azslLexer::SubpassInputMS), "\'")
                    || m_typeId.GetNameLeaf() == Trim(lexer->getVocabulary().getLiteralName(azslLexer::SubpassInputDS), "\'")
                    || m_typeId.GetNameLeaf() == Trim(lexer->getVocabulary().getLiteralName(azslLexer::SubpassInputDSMS), "\'"));
        }

        friend bool operator == (const TypeRefInfo& lhs, const TypeRefInfo& rhs)
        {
            return lhs.m_typeId == rhs.m_typeId;
        }
        friend bool operator != (const TypeRefInfo& lhs, const TypeRefInfo& rhs)
        {
            return !operator==(lhs,rhs);
        }

        IdentifierUID     m_typeId;
        TypeClass         m_typeClass;
        ArithmeticTraits  m_arithmeticInfo;
    };

    //! Run a syntactic analysis of an arithmetic type name and extract info on its composition
    inline ArithmeticTraits CreateArithmeticTraits(QualifiedName a_typeName)
    {
        assert(IsArithmetic( /*slow*/AnalyzeTypeClass(TentativeName{a_typeName}) ));  // no need to call this function if you don't have a fundamental (non void)
        assert(!IsGenericArithmetic( /*slow*/AnalyzeTypeClass(TentativeName{a_typeName}) ));
        // Important: The input needs to be canonicalized earlier to minimize this function's complexity.

        ArithmeticTraits toReturn;

        string typeName = UnMangle(a_typeName);
        size_t baseTypeLen = typeName.length();

        // In shading languages we have vector and matrix types which use number of columns and possibly rows (for matrices)
        // The digits always appear at the end of the type, so we can count back to resolve them
        auto& colsChar = typeName.at(baseTypeLen - 1);
        if (isdigit(colsChar))
        {   // Fundamental types ending with a digit are either vectors (1, 2, 3, 4) or matrices (1x1, 2x2, 3x3, 4x4, etc)

            // Vectors' sizes are defined in components, we opt to put those in Columns for consistency with the Matrix type below.
            // A float3x4 matrix in DXC is represented as class.matrix.float.3.4 = type { [3 x <4 x float>] }
            toReturn.m_cols = colsChar - '0';
            baseTypeLen--;
            assert(toReturn.m_cols >= 1 && toReturn.m_cols <= 4); // We should not be hitting asserts with any shader input, this is a bug in the tool

            if (baseTypeLen >= 2)
            {   // It's possible we are in a matrix type so let's check for rows too!

                // Documentation: https://docs.microsoft.com/en-us/windows/desktop/direct3dhlsl/dx-graphics-hlsl-per-component-math
                // A matrix is a data structure that contains rows and columns of data.The data can be any of the scalar data types,
                //   however, every element of a matrix is the same data type.The number of rows and columns is specified with the
                //   row-by-column string that is appended to the data type.

                auto& rowsChar = typeName.at(baseTypeLen - 2);
                if (isdigit(rowsChar))
                {
                    assert(typeName.at(baseTypeLen - 1) == 'x'); // We should not be hitting asserts with any shader input, this is a bug in the tool

                    toReturn.m_rows = rowsChar - '0';
                    baseTypeLen -= 2;
                    assert(toReturn.m_rows >= 1 && toReturn.m_rows <= 4); // We should not be hitting asserts with any shader input, this is a bug in the tool
                }
            }
        }

        // In any case baseTypeLen gives us our base type without vector or matrix information
        string baseType = typeName.substr(0, baseTypeLen);

        // 2 special cases: generic vector and matrix with no generic parameter -> they default to float, 4, 4.
        // https://github.com/Microsoft/DirectXShaderCompiler/issues/2034
        // temporarily support them, but when we canonicalize generics it will be supported earlier.
        if (baseType == "vector" || baseType == "matrix")
        {
            baseType = "float";
        }

        auto it = ::std::find(AZ::ShaderCompiler::Predefined::Scalar.begin(), AZ::ShaderCompiler::Predefined::Scalar.end(), baseType);
        assert(it != AZ::ShaderCompiler::Predefined::Scalar.end()); // baseType must exist in the Scalar bag by program invariant.
        toReturn.m_underlyingScalar = static_cast<int>( std::distance(AZ::ShaderCompiler::Predefined::Scalar.begin(), it) );
        toReturn.ResolveBaseSizeAndRank();
        return toReturn;
    }

    //! Create a new arithmetic traits promoted by necessity (through a binary operation usually)
    //! of compatibility with a second operand of arithmetic typeclass.
    //! For example: type(half{} + int{})->half
    //!              type(float3x3{} * double{})->double3x3
    //! And with columns & rows truncated to the smallest operand,
    //!   as part of the implicit necessary cast for operation compatibility.
    inline ArithmeticTraits PromoteTruncateWith(Pair<ArithmeticTraits> operands)
    {
        auto [_1, _2] = operands;
        // put the scalar last (will be useful later if there is only one scalar operand)
        SwapIf(_1, _2, _1.IsScalar());
        // now, let's construct the result in _1

        if (_2.m_conversionRank > _1.m_conversionRank) // the higher ranking underlying wins; independently of full object size
        {
            _1.m_underlyingScalar = _2.m_underlyingScalar;
        }
        // cases: scalar-scalar : no dim change
        //        vecmat-scalar : no dim change, since result is dim(vecmat) which is in _1
        //        scalar-vecmat : impossible (sorted by swap above)
        //        vecmat-vecmat : min(_1,_2)
        if (!_1.IsScalar() && !_2.IsScalar())
        {
            _1.m_rows = std::min(_1.m_rows, _2.m_rows);
            _1.m_cols = std::min(_1.m_cols, _2.m_cols);
        }
        _1.ResolveBaseSizeAndRank();
        return _1;
    }

    MAKE_REFLECTABLE_ENUM(RootParamType,
        SRV,               // t
        UAV,               // u
        Sampler,           // s
        CBV,               // b, bound under an SrgTable as a View. Used for external buffers
        SrgConstantCB,     // b, bound through a root descriptor. Used for SRG Constants.
        RootConstantCB     // b, bound through a CB under root signature. Used for root constants.
    );
    MAKE_REFLECTABLE_ENUM(BindingType, T, U, S, B);  //!< as HLSL register type. (please keep in the same order as RootParamType for simple mapping)

    //! map SRV->T | UAV->U | Sampler->S | CBV,SrgConsant,RootConstant->B
    BindingType RootParamTypeToBindingType(RootParamType paramType);
}
