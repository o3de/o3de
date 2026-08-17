/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once


#include "AzslcMangling.h"
#include "AzslcCommon.h"
#include "AzslcException.h"
#include "DiagnosticStream.h"

#include "antlr4-runtime.h"
#include <Grammar/azslLexer.h>
#include <Grammar/azslParserBaseListener.h>
#include <Grammar/azslParser.h>

#include "AzslcPredefinedTypes.h"

#include "ReflectableEnums.h"
#include "ReflectableEnumsUtils.h"

#if defined(_WIN32) || defined(_WIN64)
# define AzslcStrnicmp _strnicmp
#elif defined(__APPLE__)
# define AzslcStrnicmp strncasecmp
#elif defined (__linux__) || defined(__unix__)
# include <strings.h>
# define AzslcStrnicmp strncasecmp
#endif

namespace AZ::ShaderCompiler
{
    extern DiagnosticStream verboseCout;
    extern DiagnosticStream warningCout;
    extern Endl             azEndl;

    using AstType                       = azslParser::TypeContext;                  // all usertypes and predefined, but cannot be Void
    using AstTypeofNode                 = azslParser::TypeofExpressionContext;
    using AstPredefinedTypeNode         = azslParser::PredefinedTypeContext;
    using AstClassDeclNode              = azslParser::ClassDefinitionContext;
    using AstStructDeclNode             = azslParser::StructDefinitionContext;
    using AstEnumDeclNode               = azslParser::EnumDefinitionContext;
    using AstEnumeratorDecl             = azslParser::EnumeratorDeclaratorContext;
    using AstInterfaceDeclNode          = azslParser::InterfaceDefinitionContext;
    using AstSRGSemanticDeclNode        = azslParser::SrgSemanticContext;
    using AstSRGSemanticMemberDeclNode  = azslParser::SrgSemanticMemberDeclarationContext;
    using AstSRGDeclNode                = azslParser::SrgDefinitionContext;
    using AstSRGMemberNode              = azslParser::SrgMemberDeclarationContext;
    using AstNamedVarDecl               = azslParser::NamedVariableDeclaratorContext;
    using AstUnnamedVarDecl             = azslParser::UnnamedVariableDeclaratorContext;
    using AstFuncSig                    = azslParser::LeadingTypeFunctionSignatureContext;
    using AstIdExpr                     = azslParser::IdExpressionContext;
    using AstExpr                       = azslParser::ExpressionContext;
    using AstMemberAccess               = azslParser::MemberAccessExpressionContext;
    using AstVarInitializer             = azslParser::VariableInitializerContext;

    inline constexpr ssize_t operator ""_ssz(unsigned long long n)
    {
        return n;
    }

    inline bool EqualNoCase(string_view str1, string_view str2)
    {
        if (str1.length() != str2.length())
        {
            return false;
        }

        return AzslcStrnicmp(str1.data(), str2.data(), str2.length()) == 0;
    }

    inline bool StartsWithNoCase(string_view str1, string_view str2)
    {
        if (str1.length() < str2.length())
        {
            return false;
        }

        return AzslcStrnicmp(str1.data(), str2.data(), str2.length()) == 0;
    }

    // this format is the Microsoft standard for error list parsing "file(line,column): message"
    inline string DiagLine(size_t line)
    {
        using namespace std::string_literals;
        return AzslcException::s_lineFinder->GetVirtualFileName(line) + "("s + std::to_string(line) + "):";
    }

    inline string DiagLine(optional<int> line)
    {
        return line ? DiagLine(static_cast<size_t>(*line)) : string{};
    }

    inline string DiagLine(optional<size_t> line)
    {
        return line ? DiagLine(*line) : string{};
    }

    inline string DiagLine(antlr4::Token* token)
    {
        return DiagLine(token->getLine());
    }

    inline string DiagLine(tree::TerminalNode* astNode)
    {
        return DiagLine(astNode->getSymbol());
    }

    //! low level version with everything parameterizable
    template<typename... Types>
    inline void PrintWarning(DiagnosticStream& stream, Warn::EnumType level, optional<size_t> lineNumber, optional<size_t> column, Types&&... messageBits)
    {
        stream << PushLevel{} << level
               << AzslcException::MakeErrorMessage(lineNumber ? AzslcException::s_lineFinder->GetVirtualFileName(*lineNumber) : "",
                                                   lineNumber ? ToString(AzslcException::s_lineFinder->GetVirtualLineNumber(*lineNumber)) : "", column ? ToString(*column) : "",
                                                   "", false, "", ConcatString(messageBits..., "\n"))
               << PopLevel{};
    }

    //! version for clients with only, maybe, a line number
    template<typename... Types>
    inline void PrintWarning(Warn::EnumType level, optional<size_t> line, Types&&... messageBits)
    {
        PrintWarning(warningCout, level, line, none, messageBits...);
    }

    //! version for clients with a token (richest, preferred way)
    template<typename... Types>
    inline void PrintWarning(Warn::EnumType level, antlr4::Token* token, Types&&... messageBits)
    {
        PrintWarning(warningCout, level, token->getLine(), token->getCharPositionInLine() + 1, messageBits...);
    }

    inline AstTypeofNode* ExtractTypeofAstNode(AstType* ctx)
    {
        return ctx->typeofExpression();
    }

    template<typename AnyOther>
    inline AstTypeofNode* ExtractTypeofAstNode(AnyOther*)
    {
        return nullptr;
    }

    using ConstNumericVal = variant<monostate, int32_t, uint32_t, float>;


    //! Extracts <float> from <ConstNumericVal>. When not possible it will throw if <defval> is not set or fall back to <defval> if set.
    inline float ExtractValueAsFloat(const ConstNumericVal& var, optional<float> defval = none)
    {
        if (holds_alternative<monostate>(var))
        {
            if (defval == none)
            {
                throw std::logic_error{ "Constant value did not hold anything. Set defval if you want a fallback option." };
            }
            else
            {
                return *defval;
            }
        }
        else if (holds_alternative<int32_t>(var))
        {
            auto intVal = get<int32_t>(var);
            // Casting to float has precision loss: https://onlinegdb.com/By0AJTUVE
            if (intVal > 16777216 || intVal < -16777216)
            {
                PrintWarning(Warn::W3, none, "warning: Casting integer ", intVal, " to float, will result in ", static_cast<float>(intVal));
            }
            return static_cast<float>(intVal);
        }
        else if (holds_alternative<uint32_t>(var))
        {
            auto uintVal = get<uint32_t>(var);
            // Same comment
            if (uintVal > 16777216)
            {
                PrintWarning(Warn::W3, none, "warning: Casting integer ", uintVal, " to float, will result in ", static_cast<float>(uintVal));
            }
            return static_cast<float>(uintVal);
        }

        assert(holds_alternative<float>(var));
        return get<float>(var);
    }

    inline int64_t ExtractValueAsInt64(const ConstNumericVal& var, optional<int64_t> defval = none)
    {
        if (holds_alternative<monostate>(var))
        {
            if (defval == none)
            {
                throw std::logic_error{ "Constant value did not hold anything. Set defval if you want a fallback option." };
            }
            else
            {
                return *defval;
            }
        }
        else if (holds_alternative<float>(var))
        {
            // Casting from float has precision loss: https://onlinegdb.com/By0AJTUVE
            auto floatVal = get<float>(var);
            auto intVal = static_cast<int64_t>(floatVal);
            PrintWarning(Warn::W3, none, "warning: Casting float ", floatVal, " to integer, will result in ", intVal);
            return intVal;
        }
        else if (holds_alternative<int32_t>(var))
        {
            return get<int32_t>(var);
        }
        return static_cast<int64_t>(get<uint32_t>(var));
    }

    template<class T>
    inline T ExtractValueAs(const ConstNumericVal& var, optional<T> defval = none)
    {
        if (holds_alternative<monostate>(var))
        {
            if (defval == none)
            {
                throw std::logic_error{ "Constant value did not hold anything. Set defval if you want a fallback option." };
            }
            else
            {
                return *defval;
            }
        }
        else if (holds_alternative<float>(var))
        {
            // Casting from float has precision loss: https://onlinegdb.com/By0AJTUVE
            auto floatVal = get<float>(var);
            auto intVal = static_cast<int64_t>(floatVal);
            PrintWarning(Warn::W3, none, "warning: Casting float ", floatVal, " to integer, will result in ", intVal);
            return static_cast<T>(intVal);
        }
        else if (holds_alternative<int32_t>(var))
        {
            return static_cast<T>(get<int32_t>(var));
        }
        return static_cast<T>(get<uint32_t>(var));
    }


    //! Safe way to call ExtractValueAsInt64 which returns false instead of throwing
    inline bool TryGetConstExprValueAsInt64(const ConstNumericVal& foldedIdentifier, int64_t& returnValue) noexcept(true)
    {
        if (holds_alternative<monostate>(foldedIdentifier))
        {
            return false;
        }

        returnValue = ExtractValueAsInt64(foldedIdentifier); // Won't throw because we have already checked holds_alternative<monostate>
        return true;
    }

    MAKE_REFLECTABLE_ENUM_POWER (StorageFlag,
        Static, Const, Unsigned, RowMajor, ColumnMajor, Extern, Inline, Rootconstant, Option, Precise, Groupshared, Uniform, Volatile, Globallycoherent, In, Out, InOut, Enumerator, Other
    );

    inline Streamable& operator << (Streamable& out, StorageFlag::EnumType sf)
    {
        return out << ToLower(StorageFlag::ToStr(sf));
    }

    inline Streamable& operator << (Streamable& out, StorageFlag sf)
    {
        return out << sf.m_value;
    }

    using Modifiers = Flag<StorageFlag>;
    struct TypeQualifiers
    {
        Modifiers      m_flag;
        vector<string> m_others;    // For qualifiers we didn't add to the enum

        string GetDisplayName() const
        {
            vector<StorageFlag> bag;
            std::copy_if(StorageFlag::Enumerate{}.begin(), StorageFlag::Enumerate{}.end(), std::back_inserter(bag),
                                    [&](auto sf) -> bool { return (m_flag & sf) && (sf & ~StorageFlag::Other); });
            // Join will call operator<< on StorageFlag::EnumType for stringification
            return string{Trim(Join(bag.begin(), bag.end(), " ") + " " + Join(m_others.begin(), m_others.end(), " "))};
        }

        void OrMerge(const TypeQualifiers& src)
        {
            m_flag |= src.m_flag;
            StableMerge(m_others, src.m_others);  // stable is key to not disturb emission tests accross platforms that could hash differently if using unordered_sets.
        }
    };

    struct ArrayDimensions
    {
        const bool IsArray() const
        {
            return !m_dimensions.empty();
        }

        const bool IsUnbounded() const
        {
            // For now m_tex[][4][5] won't be supported.
            // But m_tex[] is supported.
            return (m_dimensions.size() == 1) && (m_dimensions[0] == Unbounded);
        }

        //! Have all dimensions been statically resolved ?
        const bool AreAllDimsFullyConstantFolded() const
        {
            return std::all_of(m_dimensions.begin(), m_dimensions.end(), [](int d) { return (d >= 0); });
        }

        int GetDimensionAt_OrDefault(int dimensionIndex, int defaultValueIfNoSuchDim) const
        {
            return m_dimensions.size() > dimensionIndex ? m_dimensions[dimensionIndex] : defaultValueIfNoSuchDim;
        }

        //! pretty print a representation of an array variable declarator suffix in C-like form
        //! {} is ""
        //! {unbounded} is "[]"
        //! {unknown} is "[?]"
        //! {1,2} is "[1][2]"
        string ToString(const string& prefix = "[", const string& separator = "][", const string& suffix = "]") const
        {
            vector<string> asStrs;
            TransformCopy(m_dimensions, asStrs,
                          [](int d) -> string { return d == Unknown ? "<unrecognized-expr>" : d == Unbounded ? "" : std::to_string(d); });
            return asStrs.empty() ? "" : prefix + Join(asStrs.begin(), asStrs.end(), separator) + suffix;
        }

        void PushBack(int newDimension)
        {
            m_dimensions.push_back(newDimension);
        }

        void Clear()
        {
            m_dimensions.clear();
        }

        bool Empty() const
        {
            return m_dimensions.empty();
        }

        auto begin() const
        {
            return m_dimensions.begin();
        }

        auto end() const
        {
            return m_dimensions.end();
        }

        friend bool operator == (const ArrayDimensions& lhs, const ArrayDimensions& rhs)
        {
            return lhs.m_dimensions == rhs.m_dimensions;
        }
        friend bool operator != (const ArrayDimensions& lhs, const ArrayDimensions& rhs)
        {
            return !operator==(lhs,rhs);
        }

        //! value taken by dimensions that couldn't be constant-folded (variable initializer)
        static constexpr int Unknown = -1;
        //! dimension with an empty bracket []
        static constexpr int Unbounded = -2;

        //! multi array dimensions. e.g. a[2][3] will be a vector of {2,3}
        //! a[][3] (or a[var][3]) will be a vector of {unknown,3}
        //!  (note: if `var` is a statically foldable const expression, then it has a chance to resolved to its initial value)
        //! empty {} means "not an array"
        vector<int> m_dimensions;
    };

    //! OutputFormat
    //!  Specifies the pixel output format hint for the render target
    //! 
    //! None - hints that the target is unused
    //! R32 - one channel (R), can be Float32, Uint32 or Sint32
    //! R32G32 - two channels (RG), can be Float32, Uint32 or Sint32
    //! R32A32- two channels (RA), can be Float32, Uint32 or Sint32
    //! R16G16B16A16_FLOAT - Default. Four or less channels (RGBA), Float16
    //! R16G16B16A16_UNORM - four or less channels (RGBA), Unorm16
    //! R16G16B16A16_SNORM - four or less channels (RGBA), Snorm16
    //! R16G16B16A16_UINT - four or less channels (RGBA), Uint16
    //! R16G16B16A16_SINT - four or less channels (RGBA), Sint16
    //! R32G32B32A32 - four channels (RGBA), can be Float32, Uint32 or Sint32
    
    MAKE_REFLECTABLE_ENUM(OutputFormat,
        None, R32, R32G32, R32A32, R16G16B16A16_FLOAT, R16G16B16A16_UNORM, R16G16B16A16_SNORM, R16G16B16A16_UINT, R16G16B16A16_SINT, R32G32B32A32
    );

    namespace Packing
    {
        constexpr uint32_t s_bytesPerRegister = 16;
        constexpr uint32_t s_bytesPerComponent = 4;

        enum class Layout : uint32_t
        {
            CStylePacking,           //!< Dense packing with no padding. Also known as Scalar. Default for structured buffers in DirectX.
            DirectXPacking,          //!< DirectX style packing, default for cbuffer layouts in DirectX
            RelaxedDirectXPacking,   //!< As DirectX standard, but assumes dense array packing and invariant to row-column major (best fit)
            RelaxedStd140Packing,    //!< Vector-relaxed OpenGL std140, default for Uniform buffer packing for Vulkan (base alignment)
            RelaxedStd430Packing,    //!< Vector-relaxed OpenGL std430, default for Storage buffer packing for Vulkan (scalar alignment)
            StrictStd140Packing,     //!< Strict OpenGL std140, default for Uniform buffer packing for OpenGL
            StrictStd430Packing,     //!< Strict OpenGL std430, default for Storage buffer packing for OpenGL
            DirectXStoragePacking = CStylePacking,   //!< DirectX style packing for Storage buffers. That's actually Scalar
        };

        //! Used together with Layout. Defines what alignment rules the next chunk should follow
        enum class Alignment : uint32_t
        {
            asVectorStart,
            asVectorEnd,
            asMatrixStart,
            asMatrixEnd,
            asStructStart,
            asStructEnd,
            asArrayStart,
            asArrayEnd,
        };

        // Some standards (std140/std430) have extended alignment rules for structures, which dictate that
        //  the alignment inside a struct behaves like base rather than scalar.
        // For all other cases the alignment doesn't change so we can return the input.
        [[maybe_unused]] static Layout GetExtendedLayout(const Layout& scalarLayout)
        {
            if (scalarLayout == Layout::RelaxedStd430Packing)
            {
                return Layout::RelaxedStd140Packing;                
            }
            else if (scalarLayout == Layout::StrictStd430Packing)
            {
                return Layout::StrictStd140Packing;                                
            }

            return scalarLayout;
        }

        static uint32_t AlignUp(const uint32_t value, const uint32_t alignment)
        {
            if (alignment <= 1)
            {
                return value;
            }

            uint32_t mask = alignment - 1;
            return (value + mask) & ~mask;
        }

        [[maybe_unused]] static uint32_t AlignStructToLargestMember(Layout layout, uint32_t currentSize, uint32_t memberSize)
        {
            if (memberSize == 0)
            {
                return currentSize;
            }

            if (layout == Layout::RelaxedStd140Packing ||
                layout == Layout::RelaxedStd430Packing ||
                layout == Layout::StrictStd140Packing ||
                layout == Layout::StrictStd430Packing)
            {
                // Alignment starts as s_bytesPerComponent, can only double (power of two) and cannot exceed s_bytesPerRegister
                auto alignment = s_bytesPerComponent;
                while (alignment < memberSize && alignment < s_bytesPerRegister)
                {
                    alignment *= 2;
                }
                return AlignUp(currentSize, alignment);
            }

            return currentSize;
        }

        // Checks if the parameters correspond to either a Vector or a Matrix collapsed to a Vector.
        static bool IsVectorAligned(const Alignment& alignment, uint32_t rows, uint32_t cols)
        {
            if (alignment == Alignment::asVectorStart ||
                alignment == Alignment::asVectorEnd)
            {
                return true;
            }

            if (alignment == Alignment::asMatrixStart ||
                alignment == Alignment::asMatrixEnd)
            {
                if (rows <= 1 || cols <= 1)
                {
                    return true;
                }
            }

            return false;
        }

        //! Aligns the offset for the next structure
        //! Vulkan : https://www.khronos.org/registry/vulkan/specs/1.1-extensions/html/vkspec.html#interfaces-resources-layout
        //!          https://github.com/microsoft/DirectXShaderCompiler/blob/master/docs/SPIR-V.rst#memory-layout-rules
        //! OpenGL : https://www.khronos.org/registry/OpenGL/specs/gl/glspec45.core.pdf#page=159
        //!          https://github.com/Microsoft/DirectXShaderCompiler/blob/master/docs/SPIR-V.rst
        [[maybe_unused]] static uint32_t AlignOffset(Layout layout, uint32_t currentSize, const Alignment& alignment, uint32_t rows, uint32_t cols)
        {
            switch (layout)
            {
            case Layout::CStylePacking:
                return currentSize;

            case Layout::DirectXPacking:
            case Layout::RelaxedDirectXPacking:
                if (alignment == Alignment::asMatrixStart ||
                    alignment == Alignment::asArrayStart ||
                    alignment == Alignment::asStructStart)
                {
                    return AlignUp(currentSize, s_bytesPerRegister);
                }

                // Vectors, matrix endings or array endings don't require special alignment
                return currentSize;

            case Layout::RelaxedStd140Packing:
                if (alignment == Alignment::asArrayStart ||
                    alignment == Alignment::asStructStart ||
                    alignment == Alignment::asArrayEnd ||
                    alignment == Alignment::asStructEnd )
                {
                    return AlignUp(currentSize, s_bytesPerRegister);
                }

                if (IsVectorAligned(alignment, rows, cols))
                {
                    // Matrices with only one row or only one column collapse to vectors and don't require alignment if the size fits the remaining register
                    // const auto elements = std::max(rows, cols);
                    // According to the base alignment packing rules vectors with 3 elements align as 4-element vectors with strict std140. 
                    // dxc doesn't seem to follow that standard with relaxed std140.
                    return currentSize;
                }

                if (alignment == Alignment::asMatrixStart ||
                    alignment == Alignment::asMatrixEnd)
                {
                    return AlignUp(currentSize, s_bytesPerRegister);
                }

                return currentSize;

            case Layout::RelaxedStd430Packing:
            case Layout::StrictStd140Packing:
            case Layout::StrictStd430Packing:
                // The relaxed std430 (= scalar packing) for Vulkan doesn't dictate any special rules for matrix, array or struct
                // start or end decorations, but individual case are handled uniquely
                // dxc is also not without exceptions: https://github.com/microsoft/DirectXShaderCompiler/issues/2639
                // so fully handling complex structs might take more effort
                return currentSize;

            default:
                throw std::runtime_error{ "PackNextChunk: Unknown format should be handled properly!" };
                break;
            }

            return 0; // Prevents warning C4715
        }

        //! Packs the next chunk of data to the current offset and returns the new offset
        //! Alignment - dictates the alignment rules to follow for packing nextChunkSize
        [[maybe_unused]] static uint32_t PackNextChunk(Layout layout, uint32_t nextChunkSize, uint32_t& offset)
        {
            switch (layout)
            {
            case Layout::CStylePacking:
                return offset + nextChunkSize;

            case Layout::DirectXPacking:
            case Layout::RelaxedDirectXPacking:
            case Layout::RelaxedStd140Packing:
            case Layout::StrictStd140Packing:
            {
                // Rule - sizes within 16 bytes should not cross the 16-byte boundary
                const auto remaining = offset % s_bytesPerRegister;
                if (remaining > 0 && remaining + nextChunkSize > s_bytesPerRegister)
                {
                    offset = AlignUp(offset, s_bytesPerRegister);
                }
                return offset + nextChunkSize;
            }

            case Layout::RelaxedStd430Packing:
            case Layout::StrictStd430Packing:
                return offset + nextChunkSize;

            default:
                throw std::runtime_error{ "PackNextChunk: Unknown format should be handled properly!" };
                break;
            }

            return 0; // Prevents warning C4715
        }

        //! Packs the base size into an array of certain dimensions
        //! Dimensions can be 0, in which case it returns the baseSize, because there is no array rules to follow
        //! out Alignment - The minimum alignment required to pack this structure
        static uint32_t PackIntoArray(Layout layout, uint32_t baseSize, const ArrayDimensions& dimensions)
        {
            if (dimensions.Empty())
            {
                return baseSize;
            }

            // Sanity check. We can't do this earlier as some array dimensions are not resolvable at compile time,
            //  but still valid at runtime.
            if (!dimensions.AreAllDimsFullyConstantFolded())
            {
                throw std::runtime_error{"Layout packing failed for unresolved array dimensions. Please check previous warnings!"};
            }

            switch (layout)
            {
            case Layout::CStylePacking:
            case Layout::RelaxedDirectXPacking:
            case Layout::RelaxedStd430Packing:
            case Layout::StrictStd430Packing:
            case Layout::StrictStd140Packing:
            {
                uint32_t totalElements = 1;
                for (auto dimension : dimensions)
                {
                    totalElements *= dimension;
                }

                return baseSize * totalElements;
            }

            case Layout::DirectXPacking:
            {
                // DirectX elements always start on a new register
                const auto unalignedSize = baseSize;
                baseSize = AlignUp(baseSize, s_bytesPerRegister);

                uint32_t totalElements = 1;
                for (auto dimension : dimensions)
                {
                    totalElements *= dimension;
                }
                return baseSize * (totalElements - 1) + unalignedSize; // The last element doesn't occupy its full register
            }

            case Layout::RelaxedStd140Packing:
            {
                baseSize = AlignUp(baseSize, s_bytesPerRegister);

                uint32_t totalElements = 1;
                for (auto dimension : dimensions)
                {
                    totalElements *= dimension;
                }
                return baseSize * totalElements;
            }

            default:
                throw std::runtime_error{ "PackNextChunk: Unknown format should be handled properly!" };
                break;
            }

            return 0; // Prevents warning C4715
        }

        //! Packs the base size as a vector or a matrix
        //! Rows and/or cols can be 0. However, if Rows is greater than 0, Cols cannot be 0.
        //! Note! Regardless of major, matrices are defined as rows-by-columns! Rows == 0 means it's not a matrix
        static uint32_t PackAsVectorMatrix(Layout layout, uint32_t baseSize, uint32_t rows, uint32_t cols, bool rowMajor)
        {
            if (cols <= 1 && rows <= 1)
            {   // This is neither - return
                return baseSize;
            }

            // Sanity check - if it's a matrix then rows > 0 and cols must be in the [1..4] range
            if (rows > 0)
            {
                // https://docs.microsoft.com/en-us/windows/desktop/direct3dhlsl/dx-graphics-hlsl-per-component-math
                assert(rows <= 4);
                assert(cols >= 1 && cols <= 4);
            }

            switch (layout)
            {
            case Layout::CStylePacking:
                return baseSize
                    * (cols == 0 ? 1 : cols)
                    * (rows == 0 ? 1 : rows);

            case Layout::DirectXPacking:
                if (rows > 0)
                {   // It's a matrix

                    if (rowMajor && rows > 1 && cols == 1)
                    {
                        // This is a very special case which we can't replicate the same way dxc packs the data
                        // Additionally, this results in a lot of wasted space, so it's explicitly forbidden
                        throw std::runtime_error{ "PackAsVectorMatrix: row_major packing for 2x1, 3x1 and 4x1 matrix types is not allowed!" };
                    }

                    const auto fullSize = ((rowMajor) ? rows : cols) * s_bytesPerRegister;           // Use Rows or Cols to calculate the size

                    // Note! A matrix does not occupy the entire last register if its number of elements is less than 4
                    const auto adjustSize = (4 - ((rowMajor) ? cols : rows)) * s_bytesPerComponent;  // Use the opposite (Cols or Rows)
                    return fullSize - adjustSize;
                }

                // It's a vector
                return cols * s_bytesPerComponent;

            case Layout::RelaxedDirectXPacking:
                if (rows > 0)
                {   // It's a matrix
                    auto major = (rows < cols) ? rows : cols; // Take the smaller fit
                    return major * s_bytesPerRegister;
                }

                // It's a vector
                return cols * s_bytesPerComponent;

            case Layout::RelaxedStd140Packing:
            case Layout::RelaxedStd430Packing:
            case Layout::StrictStd140Packing:
            case Layout::StrictStd430Packing:
                if (rows > 0)
                {   // It's a matrix

                    if (rows == 1)
                    {
                        // It collapses it as a vector
                        return cols * s_bytesPerComponent;
                    }

                    if (cols == 1)
                    {
                        // It collapses it as a vector
                        return rows * s_bytesPerComponent;
                    }

                    auto matrixStride = (layout == Layout::RelaxedStd430Packing && rows == 2) ? 2 * s_bytesPerComponent : s_bytesPerRegister;
                    auto matrixElements = rowMajor ? rows : cols;
                    if (rowMajor)
                    {
                        std::swap(matrixStride, matrixElements);
                    }

                    return matrixStride * matrixElements;
                }

                // It's a vector
                return cols * s_bytesPerComponent;
            }

            return 0; // Prevents warning C4715
        }

        inline uint32_t PackedSizeof(int indexInAzslPredefined_Scalar)
        {
            // the array is generated but it's expected to look like:
            // {"bool", "double", "dword", "float", "half", "int", "int16_t", "int32_t", "int64_t", "uint", "uint16_t", "uint32_t", "uint64_t"}
            // just update that code if it changes one day, the assert will pop.
            if (indexInAzslPredefined_Scalar == 1 || indexInAzslPredefined_Scalar == 8 || indexInAzslPredefined_Scalar == 12)
            {
                assert(string_view{"double"} == AZ::ShaderCompiler::Predefined::Scalar[1]);
                assert(string_view{"int64_t"} == AZ::ShaderCompiler::Predefined::Scalar[8]);
                assert(string_view{"uint64_t"} == AZ::ShaderCompiler::Predefined::Scalar[12]);
                // Shader packing reference:
                // https://docs.microsoft.com/en-us/windows/desktop/direct3dhlsl/dx-graphics-hlsl-packing-rules
                return 8;
            }
            else if (indexInAzslPredefined_Scalar == 4 || indexInAzslPredefined_Scalar == 6 || indexInAzslPredefined_Scalar == 10)
            {
                // https://github.com/microsoft/DirectXShaderCompiler/wiki/Buffer-Packing
                //   extract: "with -enable-16bit-types: HLSL half type maps to native 16-bit float16_t type"
                //            "native 16-bit types have storage size of 16-bits (as expected)"
                assert(string_view{"half"} == AZ::ShaderCompiler::Predefined::Scalar[4]);
                assert(string_view{"int16_t"} == AZ::ShaderCompiler::Predefined::Scalar[6]);
                assert(string_view{"uint16_t"} == AZ::ShaderCompiler::Predefined::Scalar[10]);
                return 2;
            }
            else if (indexInAzslPredefined_Scalar < 0)
            {
                return 0;  // non-predefined case, surely meaning UDT.
            }
            assert(indexInAzslPredefined_Scalar < AZ::ShaderCompiler::Predefined::Scalar.size()); // #craefulgang.
            return 4;  // bool is 32 too.
        }
    };

    struct TokensLocation
    {
        misc::Interval m_expressionSpan;  // eg for `void F()` -> type id-expr brace brace -> m_expressionSpan concerns id-expr. for `nested::leaf` -> all 3 tokens are in the span
        ssize_t        m_focusedTokenId;  // eg for `void F()` -> `F` is the focused token. for id-expr, each nested identifier has a seenat. in `nested::leaf` -> 2 TokensLocation. one with 'nested' as focus, one with 'leaf' as focus; both with the same expression span.
        size_t         m_line;
        size_t         m_charPos;
    };

    // Most contexts (all?) will duck-type into this function
    template <typename ContextT>
    TokensLocation MakeTokensLocation(ContextT* ctx, const Token* focusedToken)
    {
        TokensLocation tl;
        tl.m_expressionSpan = const_cast<std::remove_const_t<ContextT>*>(ctx)->getSourceInterval();
        tl.m_focusedTokenId = static_cast<ssize_t>(focusedToken->getTokenIndex());
        tl.m_line           = focusedToken->getLine();
        tl.m_charPos        = focusedToken->getCharPositionInLine();
        return tl;
    };

    // version for single token
    inline TokensLocation MakeTokensLocation(const Token* tok)
    {
        TokensLocation tl;
        tl.m_expressionSpan = misc::Interval{tok->getTokenIndex(), tok->getTokenIndex()};
        tl.m_focusedTokenId = static_cast<ssize_t>(tok->getTokenIndex());
        tl.m_line           = tok->getLine();
        tl.m_charPos        = tok->getCharPositionInLine();
        return tl;
    };

    struct Seenat
    {
        IdentifierUID  m_referredDefinition;
        TokensLocation m_where;      // location in original stream, of a token group that can refer to a symbol.
                                     // (it's an interval because of nested-name-specifiers)
    };

// before being able to clean that up with C++20 concepts, let's factorize the SFINAE expression to filter the accepted iterator types by an expected trait
#define SFINAE_IS_SAME(DependentTypeToCompare, ExpectedType)\
    enable_if_t< is_same_v<typename DependentTypeToCompare, ExpectedType> >* = nullptr

    // Create a yaml element list for 'any collection' of Seenat (passed as a pseudo range)
    // - {line: x, col: y}
    template <typename SeenatRangeIter, SFINAE_IS_SAME(SeenatRangeIter::value_type, Seenat) >
    string ToYaml(SeenatRangeIter begin, SeenatRangeIter end, string_view indent)
    {
        string s;
        for (SeenatRangeIter it = begin; it != end; ++it)
        {
            s += indent;
            s += "- {line: " + std::to_string(it->m_where.m_line);
            s += ", col: " + std::to_string(it->m_where.m_charPos + 1);  // +1 because charPos starts at 0. most editors start at 1
            s += "}\n";
        }
        return s;
    }

    // version for pseudo-range of IdentifierUID elements
    template <typename UIDRangeIter, SFINAE_IS_SAME(UIDRangeIter::value_type, IdentifierUID) >
    string ToYaml(UIDRangeIter begin, UIDRangeIter end, string_view indent)
    {
        string s;
        for (UIDRangeIter it = begin; it != end; ++it)
        {
            s += indent;
            s += "- {name: '" + it->m_name + "'}\n";
        }
        return s;
    }
#undef SFINAE_IS_SAME

    // like PathPart but more specialized for grammatical elements specificity of an idExpression
    struct IdExpressionPart
    {
        string GetText() const
        {
            return m_token->getText();
        }

        bool IsScopeToken() const
        {
            return m_type == GlobalScopeOperator || m_type == ScopeResolutionOperator;
        }

        // Azslc Intermediate Representation uses slash separators for scopes
        // This function can help constructing internal "mangled" forms of qualified symbol names
        string GetAZIRMangledText() const
        {
            return IsScopeToken() ? "/" : GetText();
        }

        enum Type
        {
            NestedNameSpecifier,      //  nns::leaf    (nns)
            ScopeResolutionOperator,  //  nns::leaf    (::)
            GlobalScopeOperator,      //  ::leaf       (leading ::)
            LoneUnqualifiedId,        //  leaf         (leaf)
            QualifiedLeaf             //  nns::leaf    (leaf)
        };
        Token* m_token;
        Type   m_type;
    };

    template <typename Functor>
    void ForEachIdExpressionPart(AstIdExpr* ctx, Functor action)
    {
        static_assert( is_invocable<Functor, IdExpressionPart>::value, "please pass function objects that takes IdExpressionPart as argument" );
        if (ctx->unqualifiedId())
        {
            Token* loneLeaf = ctx->unqualifiedId()->Identifier()->getSymbol();
            action(IdExpressionPart{loneLeaf, IdExpressionPart::LoneUnqualifiedId});
        }
        else
        {
            Token* globalSROToken = ctx->qualifiedId()->nestedNameSpecifier()->GlobalSROToken;
            if (globalSROToken)
            {
                action(IdExpressionPart{globalSROToken, IdExpressionPart::GlobalScopeOperator});
            }
            for (auto* identifier : ctx->qualifiedId()->nestedNameSpecifier()->Identifier())
            {
                action(IdExpressionPart{identifier->getSymbol(), IdExpressionPart::NestedNameSpecifier});
                std::unique_ptr<Token> nextToken = identifier->getSymbol()->getTokenSource()->nextToken();
                action(IdExpressionPart{nextToken.get(), IdExpressionPart::ScopeResolutionOperator});
            }
            Token* last = ctx->qualifiedId()->unqualifiedId()->Identifier()->getSymbol();
            action(IdExpressionPart{last, IdExpressionPart::QualifiedLeaf});
        }
    }

    // This is a helper to make client code look smoother (free of internal detail about branching in non-null tree paths...)
    // It will reconstruct the mangled string for the identifier by looping over nested name elements.
    // The return type is Unqualified, even if a global SRO token is present.
    // When a global SRO token is present (leading :: in an idExression) then the identifier
    // is necessarily fully qualified (FQ).
    // We assume all identifiers are relative (non FQ), but in this one case,
    // we know that it's FQ.
    // This is too little a case to go out of our way and construct a QualifiedName.
    // Even if that sounds cool and clean, it breaks the principle of canonicality.
    // The QualifiedName and UnqualifiedName types are made to make compile time assignment
    // incompatible.
    // But the distinction here would have to be made at runtime.
    // Effectively changing the return type to a variant<UnqualifiedName, QualifiedName>
    // .. losing the compile time advantage of the whole original design choice.
    // Not only that, but propagating this information upward will pollute the clients, who
    // will feel compelled to preserve this information like it was a precious bit of entropy.
    // Except.. it is not precious. We can treat a FQ symbol as unqualified,
    // because all names will have to go through the MakeFullyQualified cruncher anyway.
    // (and that cruncher is canonical enough to return identity when it sees a FQ symbol)
    // Making this choice, we streamline treatment of names and simplify code.
    inline UnqualifiedName ExtractNameFromIdExpression(azslParser::IdExpressionContext* ctx)
    {
        std::stringstream ss;  // accumulator
        ForEachIdExpressionPart(ctx, [&ss](const IdExpressionPart& part) { ss << part.GetAZIRMangledText(); });
        return UnqualifiedName{ss.str()};
    }

    template <typename TemplateContext>
    UnqualifiedName ExtractNameFromAnyContextWithName(TemplateContext* ctx)
    {
        return UnqualifiedName{ctx->Name->getText()};
    }

    //! Verify filiation of an AST rule
    inline bool IsParentOf(tree::ParseTree* assumedParent, tree::ParseTree* assumedChild)
    {
        tree::ParseTree* parent =  assumedChild->parent;
        while (parent)
        {
            if (parent == assumedParent)
            {
                return true;
            }
            parent = parent->parent;
        }
        return false;
    }

    //! Get a pointer to the first parent that happens to be of type `SearchType`
    //! with a limit depth of `maxDepth` parents to search through
    template <typename SearchType>
    SearchType DeepParentAs(tree::ParseTree* ctx, int maxDepth)
    {
        if (auto* searchTypeNode = As<SearchType>(ctx))
        {
            return searchTypeNode;
        }
        return maxDepth <= 0 || !ctx ? nullptr : DeepParentAs<SearchType>(ctx->parent, maxDepth - 1);
    }

    template <typename ParentType>
    bool Is3ParentRuleOfType(tree::ParseTree* ctx)
    {
        return DeepParentAs<ParentType>(ctx, 3);
    }

    // is def
    inline bool IsParentRuleAFunctionDefinition(azslParser::FunctionParamContext* ctx)
    {
        return Is3ParentRuleOfType<azslParser::HlslFunctionDefinitionContext*>(ctx);
    }

    // is decl
    inline bool IsParentRuleAFunctionDeclaration(azslParser::FunctionParamContext* ctx)
    {
        return Is3ParentRuleOfType<azslParser::HlslFunctionDeclarationContext*>(ctx);
    }

    inline bool IsRHSOfMemberAccess(tree::ParseTree* ctx)
    {
        auto* asMemberAccess = As<AstMemberAccess*>(ctx->parent);
        return asMemberAccess && asMemberAccess->Member == ctx;
    }

    //! Checks if your current rule is at distance 0 of a rule you want to target; (according to pointer arithmetic distance)
    //!   example with target = ArrayAccessExpresison
    //!     $ ((T)a::b.c)[x]   from c: unqualifiedId -> idExpression -> (right of) MemberAccessExpression -> CastExpression -> ParenthesizedExpression -> ArrayAccessExpression
    //!                                all rules involved are 0-away (transparent) for pointer arithmetic, thus-> return true
    //!                        from b: unqualifiedId -> qualifiedId -> idExpression -> (left of) MemberAccessExpression
    //!                                stop here. because LHS of 'dot'. the "pointer-like" operation is offseted by RHS. thus -> return false
    //!     $ (c + d)[x]       from any of c or d: the link is broken here because '+' introduced a distance, the subscript applies on the temporary that results. thus -> return false
    //! returns:    the found target, or nullptr if not found or not 0-away
    template< typename TargetRule >
    inline TargetRule* FindRuleInAstThatIs0AwayWrtPointerDistance(antlr4::ParserRuleContext* ctx)
    {
        // possible intermediates (that don't introduce a distance):
        using Nested    = azslParser::NestedNameSpecifierContext;
        using Qualif    = azslParser::QualifiedIdContext;
        using UnQualif  = azslParser::UnqualifiedIdContext;
        using IdExpr    = azslParser::IdentifierExpressionContext;
        using BraceExpr = azslParser::ParenthesizedExpressionContext;
        using Cast      = azslParser::CastExpressionContext;
        //and AstIdExpr, AstMemberAccess too

        if (ctx == nullptr)
        {
            return nullptr;
        }
        if (TargetRule* callCtx = Is<TargetRule*>(ctx) ? As<TargetRule*>(ctx) : As<TargetRule*>(ctx->parent)) // goal
        {
            return callCtx;
        }
        bool zeroDist = DynamicTypeIsAnyOf<Nested, Qualif, UnQualif, IdExpr, BraceExpr, Cast, AstIdExpr>(ctx)
                        || IsRHSOfMemberAccess(ctx);
        auto* parentAsParserRuleCtx = polymorphic_downcast<ParserRuleContext*>(ctx->parent);
        return zeroDist ? FindRuleInAstThatIs0AwayWrtPointerDistance<TargetRule>(parentAsParserRuleCtx) // if still valid, recurse
                        : nullptr;
    }

    //! checks if your current rule is at distance 0 of a subscript []; according to pointer arithmetic distance.
    //! refer to comment of function FindRuleInAstThatIs0AwayWrtPointerDistance for examples
    inline bool IsNextToArrayAccessExpression(antlr4::ParserRuleContext* ctx)
    {
        using AAExpr = azslParser::ArrayAccessExpressionContext; // end condition
        return FindRuleInAstThatIs0AwayWrtPointerDistance<AAExpr>(ctx);
    }

    //! checks if your current rule is at distance 0 of a function call syntax f(); according to pointer arithmetic distance
    //! example: $ ((T)a::b.c)(x)
    //! returns: the argument list of the found call expression, if found. nullptr otherwise
    inline azslParser::ArgumentListContext* GetArgumentListIfBelongsToFunctionCall(antlr4::ParserRuleContext* ctx)
    {
        using Call = azslParser::FunctionCallExpressionContext; // end condition
        Call* found = FindRuleInAstThatIs0AwayWrtPointerDistance<Call>(ctx);
        return found ? found->argumentList() : nullptr;
    }

    //! access the argument count at a function call site (from the AST)
    inline size_t NumArgs(azslParser::FunctionCallExpressionContext* callCtx)
    {
        azslParser::ArgumentsContext* argsNode = callCtx->argumentList()->arguments();
        return argsNode ? argsNode->expression().size() : 0;
    }

    //! try to find a specific context type that this context would be a child of.
    template <typename LookedUp>
    inline LookedUp* ExtractSpecificParent(antlr4::ParserRuleContext* ctx)
    {
        if (ctx == nullptr)
        {
            return nullptr;
        }
        auto* asCtx = As<LookedUp*>(ctx);
        if (asCtx) // goal
        {
            return asCtx;
        }
        return ctx->parent == nullptr ? nullptr : ExtractSpecificParent<LookedUp>(polymorphic_downcast<ParserRuleContext*>(ctx->parent));
    }

    //! try to find a variable initializer that this context would be a child of.
    inline AstVarInitializer* ExtractInitializerParent(antlr4::ParserRuleContext* ctx)
    {
        return ExtractSpecificParent<AstVarInitializer>(ctx);
    }

    inline azslParser::FunctionParamContext* ParamContextOverUnnamedVariableDeclarator(AstUnnamedVarDecl* ctx)
    {
        return As<azslParser::FunctionParamContext*>(ctx->parent);
    }

    inline azslParser::VariableDeclarationContext* VarDeclContextOverVariableDeclarator(AstNamedVarDecl* ctx)
    {
        return As<azslParser::VariableDeclarationContext*>(ctx->parent->parent);
    }

    inline azslParser::TypeContext* ExtractTypeFromUnnamedVariableDeclarator(AstUnnamedVarDecl* ctx, azslParser::FunctionParamContext** funcParamContextOut = nullptr)
    {
        auto* paramCtx = ParamContextOverUnnamedVariableDeclarator(ctx);
        if (paramCtx != nullptr)
        {
            if (funcParamContextOut)
            {
                *funcParamContextOut = paramCtx;
            }
            return paramCtx->type();
        }
        auto* varDeclCtx = VarDeclContextOverVariableDeclarator(As<AstNamedVarDecl*>(ctx->parent));
        if (varDeclCtx != nullptr)
        {
            return varDeclCtx->type();
        }
        return nullptr;
    }

    /// move up the AST into parent rules to ry to get a name. in case of function parameters, name can be omitted so this function may return null
    inline Token* ExtractVariableNameIdentifier(AstUnnamedVarDecl* ctx)
    {
        return Is<AstNamedVarDecl>(ctx->parent) ? polymorphic_downcast<AstNamedVarDecl*>(ctx->parent)->Name
                                                : polymorphic_downcast<azslParser::FunctionParamContext*>(ctx->parent)->Name;
    }

    inline ParserRuleContext* GetParentIfIsNamedVarDecl_OtherwiseIdentity(AstUnnamedVarDecl* ctx)
    {
        if (!ctx || !Is<AstNamedVarDecl>(ctx->parent))
        {
            return ctx;
        }
        return polymorphic_downcast<AstNamedVarDecl*>(ctx->parent);
    }

    inline string ExtractVariableNameSamplerBodyDeclaration(azslParser::SamplerBodyDeclarationContext* ctx)
    {
        // parent1 is variableInitializer ; parent2 is unnamedVariableDeclarator
        return ExtractVariableNameIdentifier(polymorphic_downcast<AstUnnamedVarDecl*>(ctx->parent->parent))->getText();
    }

    inline bool TypeIsSamplerComparisonState(AstUnnamedVarDecl* ctx)
    {
        auto* typeCtx = ExtractTypeFromUnnamedVariableDeclarator(ctx);
        return typeCtx->predefinedType() &&
               typeCtx->predefinedType()->samplerStatePredefinedType() &&
               typeCtx->predefinedType()->samplerStatePredefinedType()->SamplerComparisonState();
    }

    inline azslParser::StorageFlagsContext* ExtractStorageFlagsFromUnnamedVariableDeclarator(AstUnnamedVarDecl* ctx)
    {
        return ExtractTypeFromUnnamedVariableDeclarator(ctx)->storageFlags();
    }

    inline StorageFlag AsFlag(azslParser::StorageFlagContext* ctx)
    {
        return ctx->Const()            ? StorageFlag::Const
             : ctx->Extern()           ? StorageFlag::Extern
             : ctx->Groupshared()      ? StorageFlag::Groupshared
             : ctx->Precise()          ? StorageFlag::Precise
             : ctx->Static()           ? StorageFlag::Static
             : ctx->Uniform()          ? StorageFlag::Uniform
             : ctx->Volatile()         ? StorageFlag::Volatile
             : ctx->Globallycoherent() ? StorageFlag::Globallycoherent
             : ctx->RowMajor()         ? StorageFlag::RowMajor
             : ctx->ColumnMajor()      ? StorageFlag::ColumnMajor
             : ctx->In()               ? StorageFlag::In
             : ctx->Out()              ? StorageFlag::Out
             : ctx->Inout()            ? StorageFlag::InOut
             : ctx->Inline()           ? StorageFlag::Inline
             : ctx->Option()           ? StorageFlag::Option
             : ctx->Rootconstant()     ? StorageFlag::Rootconstant
             : ctx->Unsigned()         ? StorageFlag::Unsigned
            // Everything else can still be stored, but won't be checked in any special way:
            // linear, centroid, noninterpolation, noperspective, sample, point, line, triangle, lineadk, triangleadj, indices, vertices, etc...
             : StorageFlag::Other;
    }

    inline bool IsFlag(azslParser::StorageFlagContext* ctx, StorageFlag flag)
    {
        return AsFlag(ctx) == flag;
    }

    // Either just a string, or string + its original source node.
    // keeping the source node allows for typeof(expression) resolution, and reversely keeping the name allows for clear strings during debugging.
    //  Ext for extended.
    struct ExtractedTypeExt
    {
        UnqualifiedName m_name;
        AstType*        m_node = nullptr;
    };
    // composed type parts. 'Buffer<float4>' will have 'Buffer' as core and 'float4' as genericParam
    struct ExtractedComposedType
    {
        ExtractedTypeExt m_core;
        ExtractedTypeExt m_genericParam; // note: could be made recursive one day. use unique_ptr here
        ArrayDimensions  m_genericDimensions; // can expand generic dimensions too later. for MSTexture, IO patch, matrix<t,N,M> etc...
    };

    // Compose middleEnd configurations, wraps minimal user options from command line.
    struct MiddleEndConfiguration
    {
        int m_rootConstantsMaxSize;
        AZ::ShaderCompiler::Packing::Layout m_packConstantBuffers;
        AZ::ShaderCompiler::Packing::Layout m_packDataBuffers;
        bool m_isRowMajor;
        bool m_padRootConstantCB;
        bool m_skipAlignmentValidation;
    };

    ExtractedComposedType ExtractComposedTypeNamesFromAstContext(AstType* ctx, vector<tree::TerminalNode*>* genericDims = nullptr);

    // Oftentimes a type rule looks like: `genericTexture: TextureKeyword '<' type '>'` , this function extracts `type`.
    // Types parent of that ctx should have an AnalyzeClass that returns HasGenericParameter
    // for more power, this function can return an AstPointer to a type rule directly in case of availability.
    inline ExtractedComposedType ExtractComposedTypeNamesFromAstContext(AstPredefinedTypeNode* ctx, vector<tree::TerminalNode*>* genericDims = nullptr)
    {
        if (ctx->bufferPredefinedType())
        {   // use aggregate initialization syntax to construct the return type (tops 4 elements, but here we fillup 2 only)
            return {ExtractedTypeExt{UnqualifiedName{ctx->bufferPredefinedType()->bufferType()->getText()}},                   // m_core
                    ExtractedTypeExt{UnqualifiedName{ctx->bufferPredefinedType()->scalarOrVectorOrMatrixType()->getText()}}};  // m_genericParam
        }
        else if (ctx->genericMatrixPredefinedType())
        {
            auto intLit1 = ctx->genericMatrixPredefinedType()->IntegerLiteral(0);
            auto intLit2 = ctx->genericMatrixPredefinedType()->IntegerLiteral(1);
            if (genericDims && intLit1 && intLit2)
            {
                genericDims->push_back(intLit1);
                genericDims->push_back(intLit2);
            }
            return {ExtractedTypeExt{UnqualifiedName{ctx->genericMatrixPredefinedType()->Matrix()->getText()}},       // m_core
                    ExtractedTypeExt{UnqualifiedName{ctx->genericMatrixPredefinedType()->scalarType()->getText()}}};  // m_genericParam
        }
        else if (ctx->streamOutputPredefinedType())
        {
            auto* core = ctx->streamOutputPredefinedType()->streamOutputObjectType();
            AstType* genericCtx = ctx->streamOutputPredefinedType()->type();
            return {ExtractedTypeExt{UnqualifiedName{core->getText()}},                     // m_core
                    ExtractedTypeExt{UnqualifiedName{genericCtx->getText()}, genericCtx}};  // for the generic, since this is a type context, we will return it as a node, and let the client recurse if needed.
        }
        else if (ctx->structuredBufferPredefinedType())
        {
            auto* core = ctx->structuredBufferPredefinedType()->structuredBufferName();
            AstType* genericCtx = ctx->structuredBufferPredefinedType()->type();
            return {ExtractedTypeExt{UnqualifiedName{core->getText()}},                     // m_core
                    ExtractedTypeExt{UnqualifiedName{genericCtx->getText()}, genericCtx}};  // same as above
        }
        else if (ctx->genericTexturePredefinedType())
        {
            return {ExtractedTypeExt{UnqualifiedName{ctx->genericTexturePredefinedType()->textureType()->getText()}},         // m_core
                    ExtractedTypeExt{UnqualifiedName{ctx->genericTexturePredefinedType()->scalarOrVectorType()->getText()}}}; // m_genericParam
        }
        else if (ctx->msTexturePredefinedType())
        {
            auto intLit = ctx->msTexturePredefinedType()->IntegerLiteral();
            if (genericDims && intLit)
            {
                genericDims->push_back(intLit);
            }
            return {ExtractedTypeExt{UnqualifiedName{ctx->msTexturePredefinedType()->textureTypeMS()->getText()}},        // m_core
                    ExtractedTypeExt{UnqualifiedName{ctx->msTexturePredefinedType()->scalarOrVectorType()->getText()}}};  // m_genericParam
        }
        else if (ctx->genericVectorType())
        {
            auto intLit = ctx->genericVectorType()->IntegerLiteral();
            if (genericDims && intLit)
            {
                genericDims->push_back(intLit);
            }
            return {ExtractedTypeExt{UnqualifiedName{ctx->genericVectorType()->Vector()->getText()}},        // m_core
                    ExtractedTypeExt{UnqualifiedName{ctx->genericVectorType()->scalarType()->getText()}}};   // m_genericParam
        }
        else if (ctx->constantBufferTemplated())
        {
            auto coreName = ctx->constantBufferTemplated()->CBCoreType->getText();
            AstType* genericCtx = ctx->constantBufferTemplated()->GenericTypeName;
            return {ExtractedTypeExt{UnqualifiedName{coreName}},                           // m_core
                    ExtractedTypeExt{UnqualifiedName{genericCtx->getText()}, genericCtx}};  // same as previous 2 comments above
        }
        else if (ctx->genericSubpassInputPredefinedType())
        {
            return { ExtractedTypeExt{UnqualifiedName{ctx->genericSubpassInputPredefinedType()->subpassInputType()->getText()}},    // m_core
                    ExtractedTypeExt{UnqualifiedName{ctx->genericSubpassInputPredefinedType()->scalarOrVectorType()->getText()}} }; // m_genericParam
        }
        // all other rules are getable without artefacts for sure from a simple getText
        return {ExtractedTypeExt{UnqualifiedName{ctx->getText()}}};     // only core, no generic part.
    }

    //! from a special rule: scalarOrVectorOrMatrixType
    inline ExtractedComposedType ExtractComposedTypeNamesFromAstContext(azslParser::ScalarOrVectorOrMatrixTypeContext* ctx, vector<tree::TerminalNode*>* genericDims = nullptr)
    {
        return {ExtractedTypeExt{UnqualifiedName{ctx->getText()}}};
                // for now there is no generic part, but mind it when you fix the grammar to support generic vector.
    }

    //! from userDefinedType context
    inline ExtractedComposedType ExtractComposedTypeNamesFromAstContext(azslParser::UserDefinedTypeContext* ctx, vector<tree::TerminalNode*>* genericDims = nullptr)
    {
        if (ctx->idExpression())
        {
            return {ExtractedTypeExt{ExtractNameFromIdExpression(ctx->idExpression())}};
        }
        assert(ctx->anyStructuredTypeDefinition());
        auto* anyStructLikeCtx = ctx->anyStructuredTypeDefinition();
        auto structName = VisitFirstNonNull([](auto* ctx) { return ExtractNameFromAnyContextWithName(ctx); },
                                            anyStructLikeCtx->classDefinition(),
                                            anyStructLikeCtx->interfaceDefinition(),
                                            anyStructLikeCtx->enumDefinition(),
                                            anyStructLikeCtx->structDefinition());
        return {ExtractedTypeExt{UnqualifiedName{structName}}};
    }

    //! from type context (next to highest level)
    inline ExtractedComposedType ExtractComposedTypeNamesFromAstContext(AstType* ctx, vector<tree::TerminalNode*>* genericDims /*= nullptr*/)
    {
        if (ctx->userDefinedType())
        {
            return ExtractComposedTypeNamesFromAstContext(ctx->userDefinedType(), genericDims);
        }
        else if (ctx->predefinedType())
        {
            return ExtractComposedTypeNamesFromAstContext(ctx->predefinedType(), genericDims);
        }
        else if (ctx->Void())
        {
            assert(string_view{AZ::ShaderCompiler::Predefined::Void[0]} == ctx->Void()->getText());
            return {{UnqualifiedName{ctx->Void()->getText()}}}; // "void"
        }
        // this could be a typeof, let's return the node for further resolve!
        return {ExtractedTypeExt{UnqualifiedName{ctx->getText()}, ctx}};
    }

    //! Parse an HLSL semantic from a context into (semantic name, semantic index, is system value)
    inline tuple<string, int, bool> ExtractHlslSemantic(azslParser::HlslSemanticContext* hlslSemantic)
    {
        // Semantic name and index
        auto semanticName = hlslSemantic->getText();
        size_t index      = semanticName.find_last_not_of("0123456789") + 1;
        int semanticIndex = (index == semanticName.length()) ?
                            0 : std::stoi(semanticName.substr(index));

        auto colon = semanticName.find_first_of(":");
        auto firstChar = (colon == string::npos) ? 0 : colon + 1;
        firstChar = semanticName.find_first_not_of(" ", firstChar);
        semanticName = semanticName.substr(firstChar, index - firstChar);

        bool isSystemValue = (hlslSemantic->Name->HLSLSemanticSystem() != nullptr);

        return { semanticName, semanticIndex, isSystemValue };
    }

    inline bool HasStandardInitializer(AstVarInitializer* ctx)
    {
        return ctx && ctx->standardVariableInitializer();
    }

    inline bool HasStandardInitializer(AstUnnamedVarDecl* ctx)
    {
        return ctx && HasStandardInitializer(ctx->variableInitializer());
    }
}
