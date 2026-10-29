/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include "AzslcSemanticOrchestrator.h"
#include "AzslcTokenToAst.h"
#include "AzslcKindInfo.h"
#include "PadToAttributeMutator.h"

namespace AZ::ShaderCompiler
{
    //! We limit the maximum number of render targets to 8, with indices in the range [0..7]
    static const uint32_t kMaxRenderTargets = 8;

    struct IRMetaData
    {
        //! the input source file path
        string m_insource = "Stdin";

        //! the activated namespaces on the command line
        unordered_set<string> m_attributeNamespaceFilters;
        //! The namespace for the platform code emitter
        string m_platformEmitterNamespace;

        //! some platforms require source-coded target formats
        OutputFormat m_outputFormatHint[kMaxRenderTargets] = { OutputFormat::R16G16B16A16_FLOAT };
    };

    // Holder responsible for owning post-parsed source data (in the form of an object graph).
    // Note that this is not IL. e.g. not a serialized form of IR with op-codes (like DXIL).
    struct IntermediateRepresentation
    {
        IntermediateRepresentation(azslLexer* lexer)
            : m_scope{[&](QualifiedNameView sym)  // Initialize the scope object with a decoupled identifier getter. (SOLID's D.I.P)
                      {
                          return m_symbols.GetIdAndKindInfo(sym);
                      }
                     }
            , m_lexer{ lexer }
            , m_sema{&m_symbols, &m_scope, lexer}
            , m_padToAttributeMutator(*this)
        {
            // Default output format for all targets
            std::fill_n(m_metaData.m_outputFormatHint, kMaxRenderTargets, m_metaData.m_outputFormatHint[0]);
        }

        //! Shortcut helper to symbols
        auto GetIdAndKindInfo(QualifiedNameView symbol) const
        {
            return m_symbols.GetIdAndKindInfo(symbol);
        }

         //! Shortcut helper to symbols
        auto GetIdAndKindInfo(QualifiedNameView symbol)
        {
            return m_symbols.GetIdAndKindInfo(symbol);
        }

        //! Only the second part of IdAndKindInfo, when we already have the UID
        KindInfo* GetKindInfo(IdentifierUID uid)
        {
            auto* idKindPtr = m_symbols.GetIdAndKindInfo(uid.GetName());
            return idKindPtr ? &idKindPtr->second : nullptr;
        }

        //! Just query the kind of a symbol
        Kind GetKind(IdentifierUID symbol) const
        {
            auto* ptr = m_symbols.GetIdAndKindInfo(symbol.GetName());
            return ptr ? ptr->second.GetKind() : Kind{Kind::EndEnumeratorSentinel_};
        }

        //! Query if a symbol is a structure/class/enum nested in a structure or class
        bool IsNestedStruct(const IdentifierUID& symbol) const
        {
            return GetKind(symbol).IsOneOf(Kind::Class, Kind::Struct, Kind::Enum)
                && GetKind({GetParentName(symbol.GetName())}).IsOneOf(Kind::Class, Kind::Struct);
        }

        //! overload from IdentifierUID
        bool IsNestedStructOrEnum(const IdentifierUID& symbol) const
        {
            return IsNestedStruct(symbol);
        }

        //! Extracts the contained SubInfo from a symbol lookup
        template<typename T>
        const T* GetSymbolSubAs(QualifiedNameView symbol) const
        {
            const auto* symbolLookUp = GetIdAndKindInfo(symbol);
            if (!symbolLookUp)
            {
                return nullptr;
            }
            return symbolLookUp->second.GetSubAs<T>();
        }

        template<typename T>
        T* GetSymbolSubAs(QualifiedNameView symbol)
        {
            auto* symbolLookUp = GetIdAndKindInfo(symbol);
            if (!symbolLookUp)
            {
                return nullptr;
            }
            return symbolLookUp->second.GetSubAs<T>();
        }

        //! Returns a copy of the Ordered list filtered by SubInfo (of type `Sub`) only. elements are only the subinfo.
        template<typename Sub>
        const vector<Sub*> GetOrderedSubInfosOfSubType()
        {
            return m_symbols.GetOrderedSubInfosOfSubType<Sub>();
        }

        //! Returns a copy of the Ordered list filtered by SubInfo (of type `Sub`) only. elements are pairs (uid, sub)
        template<typename Sub>
        vector<pair<IdentifierUID, Sub*>> GetOrderedSymbolsOfSubType_2()
        {
            return m_symbols.GetOrderedSymbolsOfSubType_2<Sub>();
        }

        const string& OriginalSource() const
        {
            return m_metaData.m_insource;
        }

        bool IsAttributeNamespaceActivated(const string& attr)
        {
            // while we don't have C++20, we can't benefit from heterogeneous key search in unordered container.
            return (m_metaData.m_attributeNamespaceFilters.find(attr) != m_metaData.m_attributeNamespaceFilters.end());
        }

        void AddAttributeNamespaceFilter(const string& attr)
        {
            std::regex validNamespace(R"__([a-zA-Z_]+)__");
            if (!std::regex_match(attr, validNamespace))
            {
                throw std::runtime_error("Invalid namespace '" + attr + "'");
            }

            m_metaData.m_attributeNamespaceFilters.emplace(attr);
            // We can have multiple attribute scopes enabled
            // By design only one can have associated platform emitter, so just use the first one.
            if(m_metaData.m_platformEmitterNamespace.empty())
            {
                m_metaData.m_platformEmitterNamespace = attr;
            }
        }

        void RegisterAttributeSpecifier(AttributeScope scope,
                                        AttributeCategory category,
                                        size_t declarationLine,
                                        string_view space,
                                        string_view name,
                                        azslParser::AttributeArgumentListContext* argList);

        //! called internally after a new attribute is registered
        //! Returns true if the attribute must be registered, otherwise
        //! returns false.
        bool ProcessAttributeSpecifier(const AttributeInfo& attrInfo);

        void RegisterTokenToNodeAssociation(ssize_t tokenId, antlr4::ParserRuleContext* node);

        //! Validate the root variables & the size of the root constants structure (and return it)
        int ValidateRootConstantStruct(const MiddleEndConfiguration& middleEndConfig);

        //! Register a struct Root_Constants and its members to ir
        void RegisterRootConstantStruct(const MiddleEndConfiguration& middleEndConfig);

        //! Calculates the size of the given member of the RootConstant CB.
        //! It also calculates the offset at which the next member of the RootConstant CB should start.
        uint32_t CalculateLayoutForRootConstantMember(
            const IdentifierUID& memberId,
            const bool isRowMajor,
            const AZ::ShaderCompiler::Packing::Layout layoutPacking,
            const uint32_t startingOffset,
            uint32_t& nextMemberStartingOffset) const;

        //! The shader developer declares each rootconstant variable independently, but AZSLc
        //! combines all those variables into a single ConstantBuffer. This function calculates
        //! the total size of the generated ConstantBuffer.
        //! @param exportedTypeId: This is the Uid created at runtime by AZSLc to identify the CB that contains
        //!                        all the rootconstants.
        uint32_t CalculateSizeOfRootConstantsCB(
            const IdentifierUID& exportedTypeId,
            const bool isRowMajor,
            const AZ::ShaderCompiler::Packing::Layout layoutPacking) const;

        //! execute any logic that relates to intermediate treatment that would need to be done between front end and back end
        void MiddleEnd(const MiddleEndConfiguration& middleEndconfig,
                       PreprocessorLineDirectiveFinder* lineFinder);

        bool Validate();

        //! Returns a list with all symbols that have their QualifiedName starting with
        //! the QualifiedName of parentIdUid.
        vector<IdentifierUID> GetChildren(const IdentifierUID& parentUid) const;

        //! Removes all unused SRGs from the symbol table.
        void RemoveUnusedSrgs();

        //! Helper function that returns a filtered list of identifiers for symbols
        //! of a specific KindInfo Subtype. The LambdaFilter should return true when it finds a match.
        template<typename Sub, typename LambdaFilter>
        vector<IdentifierUID> GetFilteredSymbolsOfSubType(LambdaFilter filterFunc)
        {
            vector<IdentifierUID> filteredSymbols;

            for (const auto& pairUidSym : GetOrderedSymbolsOfSubType_2<Sub>())
            {
                auto& uid = pairUidSym.first;
                KindInfo* kindInfo = GetKindInfo(uid);
                if (filterFunc(kindInfo))
                {
                    filteredSymbols.push_back(uid);
                }
            }
            return filteredSymbols;
        }

        //! Same as above, only returns the first match.
        template<typename Sub, typename LambdaFilter>
        IdentifierUID GetFirstFilteredSymbolOfSubType(LambdaFilter filterFunc)
        {
            for (const auto& pairUidSym : GetOrderedSymbolsOfSubType_2<Sub>())
            {
                auto& uid = pairUidSym.first;
                KindInfo* kindInfo = GetKindInfo(uid);
                if (filterFunc(kindInfo))
                {
                    return uid;
                }
            }
            return {};
        }

        IdAndKind& GetCurrentScopeIdAndKind();

        // GFX-TODO: https://github.com/o3de/o3de-azslc/issues/9
        // Upgrade DXC and if the alignment issues are fixed when using
        // -fvk-use-dx-layout then this function can be deleted.
        ///////////////////////////////////////////////////////////////////////
        //! This function is useful to check if the alignment of the data in "struct"s 
        //! or "SRG"s will encounter issues related with  a bug in DXC when
        //! -fvk-use-dx-layout is specified and there are
        //! float or float2 variables
        //! preceded by matrices or structs that end in matrices of type:
        //! float2x2, float3x2, float4x2
        //! In such case the "float" or "float2" should be be prepadded by a float3 variable
        //! and an exception error will be produced with a proposed solution.
        //! 
        //! The other cases are when
        //! "float" type variables
        //! are preceded by matrices or structs that end in matrices of type:
        //! float2x3, float3x3, float4x3
        //! In such case the "float" should be prepadded by a float2 variables.
        //! 
        //! Also it's important to keep track of structs that end in one of the matrices mentioned above.
        //! 
        //! ////////////////////////////////////////////////////////////
        //! Example 1:
        //! 
        //! struct MyStruct {
        //!     float3x3 m_mat;
        //!     float m_value;
        //! }
        //! 
        //! After the Fix:
        //! 
        //! struct MyStruct {
        //!     float3x3 m_mat;
        //!     float2 __pad__;  //<-- Suggested FIX
        //!     float m_value;
        //! }
        //!
        //! ////////////////////////////////////////////////////////////
        //! Example 2:
        //! 
        //! struct MyStructA {
        //!     float4 m_vec;
        //!     float4x2 m_mat;
        //! }
        //! 
        //! struct MyStructB {
        //!     MyStructA m_a;
        //!     float2 m_value;
        //! }
        //! 
        //! After the Fix:
        //! 
        //! struct MyStructA {
        //!     float4 m_vec;
        //!     float4x2 m_mat;
        //! }
        //! 
        //! struct MyStructB {
        //!     MyStructA m_a;
        //!     float3 __pad__; //<-- Suggested FIX
        //!     float2 m_value;
        //! }
        //!  
        void ValidateAlignmentIssueWhenScalarOrFloat2PrecededByMatrix(const MiddleEndConfiguration& middleEndconfigration,
                                                                      PreprocessorLineDirectiveFinder* lineFinder);

        // Returns info for the last variable inside the struct or class named @structUid.
        // If @structUid is not struct or class, then it returns nullptr.
        IdentifierUID GetLastMemberVariable(const IdentifierUID& structUid);

        // the maps of all variables, functions, etc, from the source code (things with declarations and a name).
        SymbolAggregator      m_symbols;
        // stateful helper during parsing
        ScopeTracker          m_scope;
        // the orchestrator holds references to above objects, so for safety we can hold it in the same place.
        // yielding better guarantees about its lifetime wrt its references.
        SemanticOrchestrator  m_sema;
        // object that allows reverse mapping of token pointers to AST rules
        TokenToAst            m_tokenMap;
        azslLexer*            m_lexer;
        // the structure that holds root constants (it's a generated thing, and there is only one)
        IdentifierUID         m_rootConstantStructUID;

        IRMetaData            m_metaData;

        // Helper to deal with the [[pad_to(N)]] attribute.
        PadToAttributeMutator m_padToAttributeMutator;
    };

    string ToYaml(const TypeRefInfo& tref, const IntermediateRepresentation& ir, string_view indent);

    string ToYaml(const ExtendedTypeInfo& ext, const IntermediateRepresentation& ir, string_view indent);

    void DumpSymbols(IntermediateRepresentation& ir);
}
