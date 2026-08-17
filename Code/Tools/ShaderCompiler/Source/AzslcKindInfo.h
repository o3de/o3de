/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include "AzslcTypes.h"

namespace AZ::ShaderCompiler
{
    // This randomly generated GUID has less chance to collide with a user-defined variable, for example m_shaderVariantKey
    static constexpr char       kShaderVariantKeyFallbackVarName[] = "m_SHADER_VARIANT_KEY_NAME_";
    static constexpr uint32_t   kShaderVariantKeyElementSize       =   32;
    static constexpr uint32_t   kShaderVariantKeyRegisterSize      =  128;

    // Maximum number of values for integer options (currently matched in Atom in ShaderAssetCreator.cpp and to be improved later).
    static constexpr uint32_t   kIntegerMaxShaderVariantValues = 1000;

    MAKE_REFLECTABLE_ENUM (Kind,
        Namespace,
        Type,    // non UDT only. Since UDT are struct/class/interface
        TypeAlias,
        Variable,
        Function,
        OverloadSet,
        Struct,
        Class,
        Enum,
        Interface,
        ShaderResourceGroup,
        ShaderResourceGroupSemantic
    );

    inline bool IsKindOneOfTypeRelated(Kind kind)
    {
        return kind.IsOneOf(Kind::Type, Kind::TypeAlias, Kind::Struct, Kind::Class, Kind::Enum, Kind::Interface);
    }

    struct SRGSemanticInfo
    {
        optional<int64_t> m_frequencyId;
        optional<int64_t> m_variantFallback;
    };

    struct EnumerationInfo
    {
        bool         m_isScoped = false;
        TypeRefInfo  m_underlyingType;
    };

    MAKE_REFLECTABLE_ENUM(AttributeScope,
        Global,
        Attached
    );

    MAKE_REFLECTABLE_ENUM(AttributeCategory,
        Single,
        Sequence
    );

    //! Store [[attributes(and, their, arguments)]]
    struct AttributeInfo
    {
        AttributeScope    m_scope;
        AttributeCategory m_category;
        size_t            m_lineNumber;
        string            m_namespace;
        string            m_attribute;

        struct Argument : variant< monostate, bool, ConstNumericVal, string >
        {
            using variant::variant;
        };
        vector<Argument>  m_argList;
    };

    static bool TypeHasStorageFlag(const TypeQualifiers& typeQualifier, StorageFlag flag)
    {
        // The mask operation returns an rvalue and it isn't converted to bool
        return static_cast<bool>(typeQualifier.m_flag & flag);
    }

    //! store base list, member list...
    struct ClassInfo
    {
        void PushMember(const IdentifierUID& uid, Kind kind)
        {
            if (kind == Kind::Variable)
            {
                m_memberFields.push_back(uid);
            }
            m_ordered.push_back(uid);
            m_members.insert(uid);
        }

        void InsertBefore(const IdentifierUID& newUid, Kind newKind, const IdentifierUID& beforeUid)
        {
            if (newKind == Kind::Variable)
            {
                auto itor = std::find(m_memberFields.begin(), m_memberFields.end(), beforeUid);
                assert(itor != m_memberFields.end());
                m_memberFields.insert(itor, newUid);
            }
            m_members.insert(newUid);

            auto itor = std::find(m_ordered.begin(), m_ordered.end(), beforeUid);
            assert(itor != m_ordered.end());
            m_ordered.insert(itor, newUid);
        }

        bool HasBase(const IdentifierUID& query) const
        {
            return std::find(m_bases.begin(), m_bases.end(), query) != m_bases.end();
        }

        bool HasAnyBases() const
        {
            return !m_bases.empty();
        }

        void PushBase(const IdentifierUID& newBase)
        {
            if (!HasBase(newBase))
            {
                m_bases.push_back(newBase);
            }
        }

        const vector<IdentifierUID>& GetBases() const
        {
            return m_bases;
        }

        const vector<IdentifierUID>& GetMemberFields() const
        {
            return m_memberFields;
        }

        const vector<IdentifierUID>& GetOrderedMembers() const
        {
            return m_ordered;
        }

        bool HasMember(const IdentifierUID& uid) const
        {
            return m_members.find(uid) != m_members.end();
        }

        bool HasMember(UnqualifiedNameView uqName) const
        {
            return FindMemberFromLeafName(uqName) != none;
        }

        //! To help for method name comparison (when doing overrides), only the leaf name needs to match
        optional<IdentifierUID> FindMemberFromLeafName(UnqualifiedNameView uqName) const
        {
            assert(IsLeaf(uqName));
            auto item = std::find_if(m_members.cbegin(), m_members.cend(), [&](decltype(*m_members.cbegin()) elem) {return elem.GetNameLeaf() == uqName; });
            return (item != m_members.cend()) ? optional<IdentifierUID>{*item} : none;
        }

        //! upcast to get a generic declaration context
        const antlr4::ParserRuleContext* GetDeclNode() const
        {
            // This might look like witchcraft to some, so allow me to explain.
            // auto-parameter lambdas (C++14) are templates, so we can
            // define a single generic visitor functor for the variant.
            // We are lucky that all AST nodes have a start object.
            // So this code compiles and work for all 5 types of nodes.
            return StdUtils::visit([](auto&& arg)
                                   {
                                       return static_cast<antlr4::ParserRuleContext*>(arg);
                                   }, m_declNodeVt);
        }

        //! non-const version
        antlr4::ParserRuleContext* GetDeclNode()
        {
            return const_cast<antlr4::ParserRuleContext*>(const_cast<const ClassInfo*>(this)->GetDeclNode());
        };

        //! get Name token from declaration contexts
        const Token* GetDeclNodeNameToken() const
        {
            return StdUtils::visit([](auto&& arg)
                                   {
                                       return arg->Name;
                                   }, m_declNodeVt);
        }

        size_t GetOriginalLineNumber() const
        {
            if (GetDeclNode())
            {
                return GetDeclNode()->start->getLine();
            }
            return 0;
        }

        //! Gets the concrete variant kind from SubInfo
        template<typename T>
        T* Get()
        {
            return (holds_alternative<T>(m_subInfo)) ? &get<T>(m_subInfo) : (T*)nullptr;
        }

        template<typename T>
        const T* Get() const
        {
            return (holds_alternative<T>(m_subInfo)) ? &get<T>(m_subInfo) : (T*)nullptr;
        }

        Kind                           m_kind;   // which of class/struct/interface/srgsemantic ? (repetition of data in the upper KindInfo)

        using DeclNode = variant< AstClassDeclNode*, AstStructDeclNode*, AstEnumDeclNode*, AstInterfaceDeclNode*, AstSRGSemanticDeclNode* >;
        DeclNode                       m_declNodeVt;

        using SubKind  = variant< monostate, SRGSemanticInfo, EnumerationInfo >;
        SubKind                        m_subInfo;

    private:
        unordered_set< IdentifierUID > m_members;      //!< Fast lookup
        vector< IdentifierUID >        m_memberFields; //!< Only the member fields, in order of declaration. All member fields are members.
        vector< IdentifierUID >        m_ordered;      //!< Ordered. all contained symbols
        vector< IdentifierUID >        m_bases;
    };
    
    //! an extended type information gathers:
    //!  + core type info (immutable and limited amount, all stored in the fixed symbol table)
    //!  + array dimensions
    //!  + matrix majorness storage flag / all other qualifiers (unsigned, const, volatile, precise, inline, groupshared...)
    struct ExtendedTypeInfo
    {
        const auto& GetDimensions() const
        {
            return m_arrayDims;
        }

        //! True if the extended type is an array type.
        const bool IsArray() const
        {
            return m_arrayDims.IsArray();
        }

        //! Get the size of a single element, ignoring array dimensions
        const uint32_t GetSingleElementSize(Packing::Layout layout, bool defaultRowMajor) const
        {
            auto baseSize = m_coreType.m_arithmeticInfo.m_baseSize;
            bool isRowMajor = (m_mtxMajor == Packing::MatrixMajor::RowMajor ||
                              (m_mtxMajor == Packing::MatrixMajor::Default && defaultRowMajor));
            auto rows = m_coreType.m_arithmeticInfo.m_rows;
            auto cols = m_coreType.m_arithmeticInfo.m_cols;
            return PackAsVectorMatrix(layout, baseSize, rows, cols, isRowMajor);
        }

        //! Get the total size
        const uint32_t GetTotalSize(Packing::Layout layout, bool defaultRowMajor) const
        {
            auto packSize = GetSingleElementSize(layout, defaultRowMajor);
            return Packing::PackIntoArray(layout, packSize, m_arrayDims);
        }

        //! Set the storage flag for row major (true) or column major (false)
        void SetMatrixMajor(Packing::MatrixMajor mtxMajor)
        {
            m_mtxMajor = mtxMajor;
            m_qualifiers.m_flag &= StorageFlag::ColumnMajor;
            m_qualifiers.m_flag &= StorageFlag::RowMajor;
            if (mtxMajor == Packing::MatrixMajor::ColumnMajor)
            {
                m_qualifiers.m_flag |= StorageFlag::ColumnMajor;
            }
            else if (mtxMajor == Packing::MatrixMajor::RowMajor)
            {
                m_qualifiers.m_flag |= StorageFlag::RowMajor;
            }
        }

        const Packing::MatrixMajor GetMatrixMajor() const
        {
            return m_mtxMajor;
        }

        //! this is the secret of the whole typeof/reference-tracking system.
        //! because this function is the leaf of any type query, we can transform types at will :)
        QualifiedNameView GetMimickedType() const
        {
            return IsChameleon(m_coreType.m_typeClass) ? m_genericParameter.m_typeId.GetName() : m_coreType.m_typeId.GetName();
        }

        //! Check whether some type has been assigned or not (or plain not-initialized)
        bool IsEmpty() const
        {
            return m_coreType.IsEmpty() && m_genericParameter.IsEmpty();
        }

        //! Verify that the extended type is fully understood.
        bool IsClassFound() const
        {
            return !m_coreType.IsEmpty() && m_coreType.m_typeClass != TypeClass::IsNotType
                && (m_genericParameter.IsEmpty() || m_genericParameter.m_typeClass != TypeClass::IsNotType);
        }

        bool CheckHasStorageFlag(StorageFlag flag) const
        {
            return TypeHasStorageFlag(m_qualifiers, flag);
        }

        friend bool operator == (const ExtendedTypeInfo& lhs, const ExtendedTypeInfo& rhs)
        {
            static const Modifiers inconsequentialModifiers = Modifiers{StorageFlag::Extern}
                | StorageFlag::Inline | StorageFlag::Static | StorageFlag::Volatile | StorageFlag::Uniform;
            Modifiers lhsTM = lhs.m_qualifiers.m_flag & ~inconsequentialModifiers;
            Modifiers rhsTM = rhs.m_qualifiers.m_flag & ~inconsequentialModifiers;
            return lhs.m_coreType == rhs.m_coreType
                && lhs.m_genericParameter == rhs.m_genericParameter
                && lhs.m_arrayDims == rhs.m_arrayDims
                && lhsTM == rhsTM;
        }
        friend bool operator != (const ExtendedTypeInfo& lhs, const ExtendedTypeInfo& rhs)
        {
            return !operator==(lhs,rhs);
        }

        //! recreate a user-readable mangled name of the whole type, for diagnostic purposes
        //! this is not rigorous and does not constitute a mangling. the full feature is better served at emission side by GetExtendedTypeInfo function.
        string GetDisplayName() const
        {
             return m_qualifiers.GetDisplayName() + " " + m_coreType.m_typeId.m_name +
                 (m_genericParameter.IsEmpty() ? "" : Decorate("<", m_genericParameter.m_typeId.m_name, ">"));
        }

        //! only use leaf form of names to compose a displayable reconstituted name
        string GetDisplayShortName() const
        {
            string coreLeaf = m_coreType.m_typeId.GetNameLeaf();
            return m_genericParameter.IsEmpty() ? coreLeaf : coreLeaf + Decorate("<", m_genericParameter.m_typeId.GetNameLeaf(), ">");
        }

        // Those can't be stored at the same place because of a problem of identity.
        //  Types are identified by their core types.
        //  i.e `int` and `int[2]` both have "?int" as a (mangled) qualified name.
        //  This is because of a limitation called array collapsing. (which is kind of important for typeof and the seenat feature)
        //  If we store the array dimensions directly in TypeRefInfo, it will not be possible
        //  to support multiple different arrays of `int`s throughout the whole compilation unit.
        TypeRefInfo          m_coreType;
        TypeRefInfo          m_genericParameter;  // in case of Buffer<float> coreType is Buffer, genericParameter is float. note that genericParams can't be arrays since brackets are not part of the "type:" rule
        TypeQualifiers       m_qualifiers;
        ArrayDimensions      m_arrayDims;
        ArrayDimensions      m_genericDims;
        Packing::MatrixMajor m_mtxMajor = Packing::MatrixMajor::Default; // Can be changed by #pragmapack_matrix directive, or with the row_major or the column_major qualifiers.
    };

    //! Holds typealias or typedef data
    struct TypeAliasInfo
    {
        azslParser::TypeAliasingDefinitionStatementContext* m_declNode = nullptr;
        ExtendedTypeInfo m_canonicalType;  // ultimate (existing) target of the alias
    };

    struct VarInfo
    {
        inline bool                CheckHasStorageFlag(StorageFlag flag) const;
        // variadic version for `all` (conjunction) check
        inline bool                CheckHasAllStorageFlags(std::initializer_list<StorageFlag> vararg) const;
        // variadic version for `any` (disjunction) check
        inline bool                CheckHasAnyStorageFlags(std::initializer_list<StorageFlag> vararg) const;
        // checks if the storage flag should be treated as local linkage
        inline bool                StorageFlagIsLocalLinkage(bool isGlobalOrSrgScope) const;
        // access variable's type's core identifier
        inline IdentifierUID       GetTypeId() const;
        // access variable's type's generic parameter if any (will return an empty identifier if not)
        inline IdentifierUID       GetGenericParameterTypeId() const;
        // get the variable's type's class
        inline TypeClass           GetTypeClass() const;
        // get the variable's type's generic parameter type's class
        inline TypeClass           GetGenericParameterTypeClass() const;
        // get the variable's type's ref info
        inline TypeRefInfo         GetTypeRefInfo() const;
        // is the variable's type a sampler ?
        inline bool                IsSampler() const;
        inline bool                IsConstantBuffer() const;
        // is texture, sampler, buffer (all sorts except ConstantBuffer)
        inline bool                IsViewType() const;
        // access array dimensions. think of it as it they apply on the whole variable's type. (so not the generic type)
        // returns an ArrayDimensions struct const ref.
        inline const auto&         GetArrayDimensions() const;
        // Returns the line number, in the AZSL file, where this symbol is declared. 
        inline size_t              GetOriginalLineNumber () const;

        AstUnnamedVarDecl*         m_declNode = nullptr;
        AstEnumeratorDecl*         m_declNodeEnum = nullptr;
        UnqualifiedName            m_identifier;
        bool                       m_srgMember = false;
        bool                       m_isPublic = true;  // this isn't class access visibility, this is for generated fields
        ConstNumericVal            m_constVal;   // (attempted folded) initializer value for simple scalars
        optional<SamplerStateDesc> m_samplerState;
        ExtendedTypeInfo           m_typeInfoExt;
        int                        m_estimatedCostImpact = -1;  //!< Cached value calculated by AnalyzeOptionRanks
        int                        m_specializationId= -1; //< id of the specialization. -1 means no specialization.
    };

    // VarInfo methods definitions
    bool VarInfo::CheckHasStorageFlag(StorageFlag flag) const
    {
       return m_typeInfoExt.CheckHasStorageFlag(flag);
    }

    bool VarInfo::CheckHasAllStorageFlags(std::initializer_list<StorageFlag> vararg) const
    {
        return std::all_of(vararg.begin(), vararg.end(), [this](StorageFlag f) { return CheckHasStorageFlag(f); });
    }

    bool VarInfo::CheckHasAnyStorageFlags(std::initializer_list<StorageFlag> vararg) const
    {
        return std::any_of(vararg.begin(), vararg.end(), [this](StorageFlag f) { return CheckHasStorageFlag(f); });
    }

    bool VarInfo::StorageFlagIsLocalLinkage(bool isGlobalOrSrgScope) const
    {
        // Options are hybrid, but we consider them local. Because hard options are defined (by macro).
        bool forcedLocal = CheckHasAnyStorageFlags({ StorageFlag::Static, StorageFlag::Groupshared, StorageFlag::Option });
        bool impliedExtern = isGlobalOrSrgScope && !forcedLocal;  // [not-explicitely-local AND global-scope (or SRG-scope)] implies extern
        bool isExtern = impliedExtern
            || CheckHasAnyStorageFlags({ StorageFlag::Extern, StorageFlag::Rootconstant });
        return !isExtern;
    }

    //! Get the type identifier of the core type
    IdentifierUID VarInfo::GetTypeId() const
    {
        return m_typeInfoExt.m_coreType.m_typeId;
    }

    IdentifierUID VarInfo::GetGenericParameterTypeId() const
    {
        return m_typeInfoExt.m_genericParameter.m_typeId;
    }

    //! Get the type information of the core type
    TypeRefInfo VarInfo::GetTypeRefInfo() const
    {
        return m_typeInfoExt.m_coreType;
    }

    //! Get the type class of the core type
    TypeClass VarInfo::GetTypeClass() const
    {
        return m_typeInfoExt.m_coreType.m_typeClass;
    }

    TypeClass VarInfo::GetGenericParameterTypeClass() const
    {
        return m_typeInfoExt.m_genericParameter.m_typeClass;
    }

    bool VarInfo::IsSampler() const
    {
        return GetTypeClass() == TypeClass::Sampler;
    }

    bool VarInfo::IsConstantBuffer() const
    {
        return GetTypeClass() == TypeClass::ConstantBuffer;
    }

    bool VarInfo::IsViewType() const
    {
        return AZ::ShaderCompiler::IsViewType(GetTypeClass());
    }

    const auto& VarInfo::GetArrayDimensions() const
    {
        return m_typeInfoExt.GetDimensions();
    }

    size_t VarInfo::GetOriginalLineNumber() const
    {
        if (m_declNode)
        {
            return m_declNode->start->getLine();
        }
        return 0;
    }

    struct OverloadSetInfo
    {
        //! Set the name of the set
        void SetSetName(const IdentifierUID& myId)
        {
            m_setFullName = myId;
        }

        //! All functions belonging to this overload-set have
        //! the same UDT return type, or all have return types of the predefined class.
        bool HasHomogeneousReturnType() const
        {
            return m_returnTypeSet.m_state == ReturnTypeSet::Homogeneous
                || m_returnTypeSet.m_state == ReturnTypeSet::NonInit;
        }

        string GetUniformReturnTypeDisplayName() const
        {
            if (!HasHomogeneousReturnType())
            {
                return "<non-homogeneous-type>";
            }
            if (!m_returnTypeSet.m_type)
            {
                return "<predefined-class-type>";
            }
            return m_returnTypeSet.m_type->GetDisplayName();
        }

        const ExtendedTypeInfo& GetUniformReturnType() const
        {
            assert(HasHomogeneousReturnType());
            return *m_returnTypeSet.m_type;
        }

        //! if there is a unique function that corresponds to a given arity, return its UID. otherwise return an empty UID
        IdentifierUID FindCandidateOfArity(size_t arity)
        {
            auto lookup = m_argCounts.find(arity);
            return lookup != m_argCounts.end() ? lookup->second : IdentifierUID{};
        }

        //! attempt a lookup within
        IdentifierUID GetConcreteFunctionThatMatchesArgumentList(string_view mangledArgumentList)
        {
            if (m_functions.size() == 1)
            {
                return *m_functions.begin();
            }
            auto reconstructedId = IdentifierUID{QualifiedName{ConcatString(m_setFullName.m_name, mangledArgumentList)}};
            bool directMatch = m_functions.find(reconstructedId) != m_functions.end();
            if (!directMatch)
            {   // attempt a fallback matching logic using arity.
                auto numArgs = CountParameters(mangledArgumentList);
                return FindCandidateOfArity(numArgs);
            }
            return reconstructedId; // if found it in the set
        }

        //! queries whether this set has at least two functions registered to it.
        bool HasOverloads() const
        {
            return m_functions.size() > 1;
        }

        //! queries whether this set contains a given candidate (in: fully decorated global unique identifiers only)
        bool Has(const IdentifierUID& member) const
        {
            return m_functions.find(member) != m_functions.end();
        }

        //! construct your overload-set progressively, thanks to that adder.
        void PushConcreteFunction(IdentifierUID functionBelongingToTheOverloadSet, ExtendedTypeInfo thatFunctionsReturnType)
        {
            assert(IsLeafDecoratedByArguments(functionBelongingToTheOverloadSet.GetName()));
            string_view core = RemoveLastParenthesisGroup(functionBelongingToTheOverloadSet.GetName());
            if (core != m_setFullName.m_name)
            {
                throw std::logic_error{ConcatString("Impossible to add function ", functionBelongingToTheOverloadSet.m_name,
                                                    " to an overload-set that doesn't have the same core name or scope")};
            }
            if (m_returnTypeSet.m_state != ReturnTypeSet::HeterogeneousUserDefinedType)
            {
                bool ok = m_returnTypeSet.Merge(thatFunctionsReturnType);
                if (!ok)
                {
                    verboseCout << "WARN: function " << functionBelongingToTheOverloadSet.m_name
                                << " is introducing heterogeneity of return types to its overload-set\n";
                }
            }
            m_functions.insert(functionBelongingToTheOverloadSet);
            // cache the arguments count:
            size_t argCount = CountParameters(functionBelongingToTheOverloadSet.GetName());
            auto previousCandidateWithSameArgCount = m_argCounts.find(argCount);
            if (previousCandidateWithSameArgCount == m_argCounts.end())
            {
                m_argCounts[argCount] = functionBelongingToTheOverloadSet;
            }
            else
            {
                previousCandidateWithSameArgCount->second.Clear();  // keep the count key in the map, but map to empty as a sentinel for "unusable arity"
            }
        }

        //! run whatever you want on all overloads UID registered in the set
        template< typename FunctorTakingUid >
        void ForEach(FunctorTakingUid&& functor)
        {
            std::for_each(m_functions.begin(), m_functions.end(), functor);
        }

        //! queries whether any registered overloads has a positive result on the functor predicate
        template< typename FunctorTakingUidReturningBool >
        bool AnyOf(FunctorTakingUidReturningBool&& functor)
        {
            return std::any_of(m_functions.begin(), m_functions.end(), functor);
        }

    private:

        //! helper to track their return type
        struct ReturnTypeSet
        {
            //! validate that all functions return manageable types
            bool Merge(ExtendedTypeInfo newType)
            {
                if (m_state == NonInit)   // first call case
                {
                    m_type = newType;
                    m_state = Homogeneous;
                }
                else if (m_state == Homogeneous)
                {
                    if (newType != *m_type)  // overloaded functions must in principle have the same return type
                    {                        // but we can tolerate differences in case of simple types.
                                             // (because they can't have renamed members that should go through translation)
                        bool newIsPredefined = IsPredefinedType(newType.m_coreType.m_typeClass);
                        bool oldIsPredefined = IsPredefinedType(m_type->m_coreType.m_typeClass);
                        if(!(newIsPredefined && oldIsPredefined))
                        {
                            m_state = HeterogeneousUserDefinedType;
                        }
                        m_type = none; // in case of a difference the cache becomes useless.
                    }
                }
                return m_state == Homogeneous;
            }
            optional<ExtendedTypeInfo> m_type;
            enum State
            {
                NonInit,
                Homogeneous,
                HeterogeneousUserDefinedType
            } m_state = NonInit;
        } m_returnTypeSet;                //!< if the returnType is uniform across all overloads, it'll be cached here.
        IdentifierUID m_setFullName;      //!< stores our own ID to be able to check that added functions share the scope and name.
        set<IdentifierUID> m_functions;   //!< IDs of functions participating to an overload set.
        unordered_map<size_t, IdentifierUID> m_argCounts; //!< number of parameters as keys to a unique candidate that has this count.
    };

    //! This is a small state machine for function registration step tracking (e.g. decl, decl, def)
    enum FunctionMultiForwards
    {
        FMF_None,  // no redundant declarators
        FMF_FwdDeclRedundancy,  // at least one redundant declarators (warning has been produced)
        FMF_SeenDef  // now encountered definition block
    };

    struct FunctionInfo
    {
        //! tells if the FunctionInfo object hasn't been initialized
        //!  note that this is an invalid state and would be illegal as an actual symbol
        bool IsEmpty() const
        {
            return m_declNode == nullptr && m_declNode == nullptr;
        }

        //! tells if a function has no definition (it has a lone declaration)
        bool IsUndefinedFunction() const
        {
            return !IsEmpty() && m_defNode == nullptr;
        }

        //! tells if a function has 2 apparition sites
        bool HasPreDeclarationAndLaterDefinition() const
        {
            return m_declNode
                && m_declNode != m_defNode
                && !IsUndefinedFunction();
        }

        //! tells if a function has only a definition (acting as both declaration and definition)
        bool HasUniqueDeclarationThroughDefinition() const
        {
            return m_defNode == m_declNode && !IsEmpty();
        }

        //! out-of-class definition exists
        bool HasDeportedDefinition() const
        {
            return m_isMethod && HasPreDeclarationAndLaterDefinition();
        }

        //! queries whether this function has a default value, registered for at least one of its parameters.
        //! e.g such as in "f(int i = 2)"
        bool HasAnyDefaultParameterValue() const
        {
            bool list0has = std::any_of(m_parameters[0].begin(), m_parameters[0].end(), [](auto&& p){return !!p.m_defaultValueExpression;});
            assert(!std::any_of(m_parameters[1].begin(), m_parameters[1].end(), [](auto&& p){return !!p.m_defaultValueExpression;}));  // check that nobody runs this query in the middle of Stash() + MergeDefaultParameters()
            return list0has;
        }

        //! add a parameter
        void PushParameter(IdentifierUID varName, const ExtendedTypeInfo& typeInfo, AstUnnamedVarDecl* unnamedCtx)
        {
            Parameter param;
            param.m_varId = varName;
            param.m_typeInfo = typeInfo;
            param.m_semanticCtx = unnamedCtx->SemanticOpt;
            param.m_arrayRankSpecifiers = unnamedCtx->ArrayRankSpecifiers;
            param.m_defaultValueExpression = unnamedCtx->variableInitializer();
            m_parameters[m_currentList].push_back(param);
        }

        //! use during semantic parsing when a function signature is re-encountered
        void StashParameters()
        {
            assert(m_currentList < 1);
            ++m_currentList;
            for (Parameter& p : m_parameters[0])
            {
                p.m_varId.Clear();  // mutate all variables to unnamed in the first list. because these symbols are getting deleted.
            }
        }

        //! @param firstDeclaration get the list from the first signature site
        auto& GetParameters(bool firstDeclaration) const
        {
            return m_parameters[std::min(m_currentList, firstDeclaration ? 0 : 1)];
        }

        bool DeleteParameter(const IdentifierUID& varName)
        {
            auto& paramList = m_parameters[m_currentList];
            const auto originalSize = paramList.size();
            paramList.erase(std::remove_if(paramList.begin(), paramList.end(), [&](const Parameter& param)
                {
                    return param.m_varId == varName;
                }), paramList.end());
            return paramList.size() != originalSize;
        }

        //! verify that if we went through a "stash", that:
        //!    the redeclaration has the same number of parameter than the original
        //!    there is no double definition of defaults arguments
        //!    and merge all defaults to the first declaration. this way we can exploit default values even before they are declared. note that in the initializer expression, if the user calls an identifier that isn't declared yet at the first site, it will fail to build.
        void MergeDefaultParameters()
        {
            if (m_currentList > 0) // we have 2 declaration sites (2 arg lists)
            {
                if (m_parameters[0].size() != m_parameters[1].size())
                {   // not a mainstreamed EC diagnostic, because this is probably an internal error. because functions identity incorporate arguments.
                    throw std::runtime_error{ConcatString(DiagLine(m_declNode->start), " function redeclaration not matching earlier appearance, during validation of parameters. Look for earlier warnings?")};
                }
                // merge all default values to first declaration
                for (size_t i = 0; i < m_parameters[0].size(); ++i)
                {
                    Parameter& p0 = m_parameters[0][i];
                    Parameter& p1 = m_parameters[1][i];
                    if (p0.m_defaultValueExpression && p1.m_defaultValueExpression)
                    {
                        throw AzslcOrchestratorException{ORCHESTRATOR_NO_DOUBLE_DEFAULT_DECLARATION, p1.m_defaultValueExpression->start,
                            ConcatString("default argument value declared twice. first seen at ",
                                         p0.m_defaultValueExpression->start->getLine(), ":",
                                         p1.m_defaultValueExpression->start->getCharPositionInLine() + 1)};
                    }
                    if (p1.m_defaultValueExpression)
                    {
                        using std::swap;
                        swap(p0.m_defaultValueExpression, p1.m_defaultValueExpression);
                    }
                }
            }
        }

        size_t GetOriginalLineNumber(bool useDefNode = true) const
        {
            if (useDefNode && m_defNode)
            {
                return m_defNode->start->getLine();
            }
            else if (m_declNode)
            {
                return m_declNode->start->getLine();
            }
            return 0;
        }

        ExtendedTypeInfo          m_returnType;
        AstFuncSig*               m_declNode     = nullptr;   //!< point to the AST node first declaring this function
        AstFuncSig*               m_defNode      = nullptr;   //!< point to the AST node defining this function
        bool                      m_isMethod     = false;
        bool                      m_mustOverride = false;     //!< means is required to override, by override specifier keyword.
        bool                      m_isVirtual    = false;     //!< is a method from an interface
        vector< IdentifierUID >   m_overrides;                //!< list of implementing functions in child classes
        optional< IdentifierUID > m_base;   //!< points to the overridden function in the base interface, if applies. only supports one base
        FunctionMultiForwards     m_multiFwds    = FMF_None;  //!< presence of redundant prototype-only declarations
        int                       m_costScore    = -1;        //!< heuristical static analysis of the amount of instructions contained
        struct Parameter
        {
            IdentifierUID m_varId;
            ExtendedTypeInfo m_typeInfo;
            azslParser::HlslSemanticContext* m_semanticCtx = nullptr;
            std::vector<azslParser::ArrayRankSpecifierContext*> m_arrayRankSpecifiers;
            AstVarInitializer* m_defaultValueExpression = nullptr;
        };
        using ParameterList = vector<Parameter>;
    private:
        array<ParameterList, 2> m_parameters;  //!< two lists. one for the original declaration site, and one for an eventual second site for deported definition
        int m_currentList = 0; //!< store index in m_parameters of currently edited list
    };

    struct SRGInfo
    {
        optional< IdentifierUID > FindMemberFromLeafName(UnqualifiedNameView member) const
        {
            auto inImplicit = m_implicitStruct.FindMemberFromLeafName(member);
            if (inImplicit)
            {
                return inImplicit;
            }
            const vector< IdentifierUID >* memberArrays[] = {&m_structs, &m_srViews, &m_samplers, &m_CBs, &m_nonexternVariables, &m_functions};
            for (auto* vector : memberArrays)
            {
                auto iterator = std::find_if(vector->begin(), vector->end(), [&member](const IdentifierUID& uid) { return uid.GetNameLeaf() == member; });
                if (iterator != vector->end())
                {
                    return *iterator;
                }
            }
            return none;
        }

        bool IsPartial() const { return m_declNode ? !!m_declNode->Partial() : false; }

        size_t GetOriginalLineNumber() const
        {
            return m_declNode ? m_declNode->start->getLine() : 0;
        }

        AstSRGDeclNode*           m_declNode = nullptr;
        ClassInfo                 m_implicitStruct;           // Implicit holding struct for SRG Constants
        optional< IdentifierUID > m_shaderVariantFallback;
        optional< IdentifierUID > m_semantic;
        vector< IdentifierUID >   m_structs;
        vector< IdentifierUID >   m_srViews;
        vector< IdentifierUID >   m_samplers;
        vector< IdentifierUID >   m_CBs;
        vector< IdentifierUID >   m_nonexternVariables;
        vector< IdentifierUID >   m_functions;
        // We will cache here the unbounded arrays as the Semantic Orchestrator
        // discovers them. We are using a vector, instead of a Set/Map because
        // There can not be more than 4 unbounded arrays per SRG. For a small
        // amount of items a linear search in a vector is faster than Set/Map searching.
        // Later, during emission, we can easily change the order of appearance of these
        // variables. 
        vector< IdentifierUID >   m_unboundedArrays;
    };

    static const uint32_t SRGSemanticInfo_MaxAllowedFrequency = 15;  // Maximum CB slots for all graphics API we support (common minimum)

    //! store data about code semantics for any sort of thing in the language. (a "sort of thing in the language" is a Kind)
    struct KindInfo
    {
        using AnyKind = variant< monostate, VarInfo, FunctionInfo, OverloadSetInfo, ClassInfo, SRGInfo, TypeRefInfo, TypeAliasInfo >;

        using MapKindToTypesT = TypeList<
            ConstVal< Kind::Type >,                        TypeRefInfo,
            ConstVal< Kind::TypeAlias >,                   TypeAliasInfo,
            ConstVal< Kind::Variable >,                    VarInfo,
            ConstVal< Kind::Function >,                    FunctionInfo,
            ConstVal< Kind::OverloadSet >,                 OverloadSetInfo,
            ConstVal< Kind::Struct >,                      ClassInfo,
            ConstVal< Kind::Class >,                       ClassInfo,
            ConstVal< Kind::Enum >,                        ClassInfo,
            ConstVal< Kind::Interface >,                   ClassInfo,
            ConstVal< Kind::ShaderResourceGroup >,         SRGInfo,
            ConstVal< Kind::ShaderResourceGroupSemantic >, ClassInfo
        >;

        //! Helper-getter for the m_subInfo, directly accessed as its concrete type.
        //! Returns nullptr in case of wrong template type (T) query, versus currently stored data type.
        template<typename T>
        const T* GetSubAs() const
        {
            return (holds_alternative<T>(m_subInfo)) ? &get<T>(m_subInfo) : (const T*)nullptr;
        }

        //! mutable version
        template<typename T>
        T* GetSubAs()
        {
            return (holds_alternative<T>(m_subInfo)) ? &get<T>(m_subInfo) : (T*)nullptr;
        }

        //! Throwing version (bad_access) of above helper, gets a reference on success
        template<typename T>
        const T& GetSubRefAs() const noexcept(false)
        {
            return get<T>(m_subInfo);
        }

        //! mutable version
        template<typename T>
        T& GetSubRefAs() noexcept(false)
        {
            return get<T>(m_subInfo);
        }

        //! Set the kind from a runtime value
        void InitAs(Kind::EnumType k)
        {
            assert( OkToAssignKind(k) ); // don't try to recycle KindInfo. re-create from scratch, or you have a bug.
            m_kind = k;
            int typeIndex = -1;
            ForEachType<MapKindToTypesT>([&typeIndex, k](auto inst, auto ii_c)
                                         {
                                             // the decltype(ii_c)::value is to prevent MSVC from crashing with an internal error. no kidding.
                                             // using KeyAtii = At_t<ii_c, MapKindToTypesT>;  BOOM !
                                             using KeyAtii = At_t<decltype(ii_c)::value, MapKindToTypesT>;
                                             if constexpr (isIntegralConstant_v<KeyAtii>) // protection because we can't compare TypeRefInfo to Kind without a build error.
                                             {
                                                 if (KeyAtii{} == k) // found k in the maplist
                                                 {
                                                     using MappedType = At_t<ii_c + 1, MapKindToTypesT>;
                                                     typeIndex = metaFind_v<MappedType, AnyKind>;
                                                 }
                                             }
                                         });
            // if typeIndex is sill -1, it's possibly a Namespace or a kind that has no subinfo.
            IndexedFactory(m_subInfo, typeIndex);
        }

        //! For when you want to create a Kind that usually has no subinfo. mostly just Namespace.
        template<Kind::EnumType k>
        void InitAs()
        {
            constexpr auto index = metaFind_v< ConstVal<k>, MapKindToTypesT >;
            if constexpr (index >= 0)
            {   // k is applicable as a kind that has a subinfo, so we can use GetSubAfterInitAs to factorize.
                GetSubAfterInitAs<k>();
            }
            else
            {   // factorize with the runtime initializer
                InitAs(k);
            }
        }

        //! At registering time, this helps initialize in an atomic way and respects proper class invariants.
        template<Kind::EnumType k>
        auto& GetSubAfterInitAs()
        {
            static_assert(k != Kind::Namespace, "There is no subinfo for namespaces. just initialize the m_kind member.");

            InitAs(k);

            // lookup the concrete type from the kind since they can be mapped surjectively
            using SubT = MetaFindValueNextToKey_t< ConstVal<k>, MapKindToTypesT >;
            static_assert( !is_same_v<SubT, NotFoundT>, "did you pass a k value out of the Kind enum ?" );
            // initialize the variant with an default constructed sub type:
            m_subInfo = SubT{};
            return GetSubRefAs<SubT>();
        }

        //! read-only access
        Kind GetKind() const
        {
            return m_kind;
        }

        //! direct query for convenience
        template<typename... Those>
        bool IsKindOneOf(Those... kinds) const
        {
            return m_kind.IsOneOf(kinds...);
        }

        //! access the seenat vector directly
        //! NOTE: careful, if you need to iterate references, check the more powerful HomonymVisitor facility.
        auto& GetSeenats()
        {
            return m_seenAt;
        }

        //! reproduce the apply API to publicize visitation of subinfo variant
        //! return: whatever visitation would return. normally deduced from return expressions of the visitor's operators().
        template<typename Visitor>
        auto VisitSub(Visitor&& v) const
        {
            return StdUtils::visit(std::forward<Visitor>(v), m_subInfo);
        }

    private:

        // private method to check class invariants
        bool OkToAssignKind(Kind k) const
        {
            // pre-init conditions, or non-changing re-assignment
            return m_kind == k || m_kind == Kind::EndEnumeratorSentinel_;
        }

        // == Members ==

        //! Precise kind of the symbol
        Kind             m_kind;
        //! Meat of the data for this symbol will have many specific attributes, so it goes in a variant
        AnyKind          m_subInfo;
        //! all reference (use) sites for this symbol
        vector< Seenat > m_seenAt;

    public:
        // Meta API. query "type is alternative?" of subinfo variant
        template<typename Alt>
        static constexpr auto isSubAlternative_v = isContainedIn_v< Alt, decltype(KindInfo::m_subInfo) >;
    };

    // usings for data model. of particular relevance for the SymbolTable.
    using IdToKindMap            = unordered_map< IdentifierUID, KindInfo >;
    using IdAndKind              = IdToKindMap::value_type;  // nicely destructurable! assign it like this: `auto& [uid, kind] = GetIdAndKind..`


    struct GetSubKindInfoTypeName_Visitor
    {
        GetSubKindInfoTypeName_Visitor(IdentifierUID uid, bool forFunctionsGetReturnType)
            : m_uid(uid),
              m_forFunctionsGetReturnType(forFunctionsGetReturnType)
        {}

        QualifiedName operator()(monostate) const
        {
            return QualifiedName{"<fail>"};
        }

        QualifiedName operator()(const VarInfo& var) const
        {   // for simplicity, note that arrays are collapsed to the underlying type (e.g. `int a[2]` has `int` type, not `int[]` type)
            return var.m_typeInfoExt.GetMimickedType();
        }

        QualifiedName operator()(const FunctionInfo& func) const
        {   // maintenance note: historical behavior here was to collapse to return type.
            // today, in intermediate type-chains evaluations, we are going to distinguish between function and function call.
            // this change is introduced to be able to make informed decisions on function-call-expression sites, amidst the bigger picture of overloads support.
            return m_forFunctionsGetReturnType ? func.m_returnType.GetMimickedType() : m_uid.GetName();
        }

        QualifiedName operator()(const OverloadSetInfo& overloadSet) const
        {
            if (m_forFunctionsGetReturnType)
            {
                return overloadSet.HasHomogeneousReturnType() ? overloadSet.GetUniformReturnType().GetMimickedType()
                                                              : QualifiedNameView{"<fail>"};
            }
            return m_uid.GetName();
        }

        QualifiedName operator()(const ClassInfo&) const
        {   // a class/struct/interface IS a type
            return m_uid.GetName();
        }

        // TODO: unify SRGInfo with ClassInfo. an SRG is a namespace. and a namespace is just a class where members are all static.
        QualifiedName operator()(const SRGInfo&) const
        {
            return m_uid.GetName(); // this is not really a type, but whatever
        }

        QualifiedName operator()(const TypeRefInfo& tri) const
        {
            assert(tri.m_typeId == m_uid);
            return m_uid.GetName();
        }

        QualifiedName operator()(const TypeAliasInfo& tai) const
        {
            return tai.m_canonicalType.GetMimickedType();
        }

        IdentifierUID m_uid;
        bool m_forFunctionsGetReturnType;
    };
    enum class ForFunctionGetType
    {
        Returned,
        SelfIdentity
    };
    //! Extract the: "canonical mimicked collapsed mangled" type name, from a symbol object. (the AZIR name)
    //!   why ?
    //!     canonical: vector<int,2> will be int2
    //!     mimicked : ConstantBuffer<S> will be S
    //!     collapsed: float4[10] will be float4
    //!     mangled  : float is ?float  and  Srg::S is /Srg/S
    //! @param getStrategy   set it to "Returned" to collapse functions to their return types. Can be useful for call expressions.
    inline QualifiedName GetTypeName(const IdAndKind* ptr, ForFunctionGetType getStrategy = ForFunctionGetType::SelfIdentity)
    {
        auto& [uid, kind] = *ptr;
        return kind.VisitSub(GetSubKindInfoTypeName_Visitor{uid, getStrategy == ForFunctionGetType::Returned});
    }

    static const size_t NoLine = 0_sz;
    struct GetOrigSourceLine_Visitor
    {
        size_t operator()(const VarInfo& var) const
        {
            return var.m_declNode ? var.m_declNode->start->getLine() : NoLine;
        }

        size_t operator()(const FunctionInfo& func) const
        {
            return func.GetOriginalLineNumber(false /*useDefNode*/);
        }

        size_t operator()(const ClassInfo& clInfo) const
        {
            return clInfo.GetOriginalLineNumber();
        }

        size_t operator()(const SRGInfo& srgInfo) const
        {
            return srgInfo.GetOriginalLineNumber();
        }

        size_t operator()(const TypeAliasInfo& tai) const
        {
            return tai.m_declNode ? tai.m_declNode->start->getLine() : NoLine;
        }

        template<typename AnyNonCovered>
        size_t operator()(AnyNonCovered) const
        {
            return NoLine;
        }
    };

    inline string GetFirstSeenLineMessage(const KindInfo& kindInfo)
    {
        size_t firstSeen = kindInfo.VisitSub(GetOrigSourceLine_Visitor{});
        if (firstSeen != NoLine)
        {
            return "first seen line " + std::to_string(firstSeen);
        }
        return {};
    }

    //! helper for fatal semantic error of ODR violation
    inline void ThrowRedeclarationAsDifferentKind(string_view symbolName, Kind newKind, const KindInfo& kindInfo, optional<size_t> lineNumber = none)
    {
        const string errorMessage = ConcatString(
            "redeclaration of ", symbolName, " with a different kind: ", Kind::ToStr(newKind),
            " but was ", Kind::ToStr(kindInfo.GetKind()), ", ", GetFirstSeenLineMessage(kindInfo));
        throw AzslcOrchestratorException(ORCHESTRATOR_ODR_VIOLATION, lineNumber, none, errorMessage);
    }

    struct GetDefinitionNodeTokensLocation_Visitor
    {
        TokensLocation operator()(const VarInfo& var) const
        {
            ParserRuleContext* node = GetParentIfIsNamedVarDecl_OtherwiseIdentity(var.m_declNode);
            return node ? MakeTokensLocation(node, ExtractVariableNameIdentifier(var.m_declNode))
                        : TokensLocation{{}, -1, 0, 0};
        }

        TokensLocation operator()(const FunctionInfo& func) const
        {
            return MakeTokensLocation(func.m_defNode ? func.m_defNode : func.m_declNode,
                                      func.m_defNode ? func.m_defNode->Name : func.m_declNode->Name);
        }

        TokensLocation operator()(const ClassInfo& clInfo) const
        {
            return MakeTokensLocation(clInfo.GetDeclNode(), clInfo.GetDeclNodeNameToken());
        }

        TokensLocation operator()(const SRGInfo& srgInfo) const
        {
            return MakeTokensLocation(srgInfo.m_declNode, srgInfo.m_declNode->Name);
        }

        TokensLocation operator()(const TypeAliasInfo taInfo) const
        {
            return MakeTokensLocation(taInfo.m_declNode, taInfo.m_declNode->typealiasStatement() ? taInfo.m_declNode->typealiasStatement()->NewTypeName
                                                                                                 : taInfo.m_declNode->typedefStatement()->NewTypeName);
        }

        template<typename AnyNonCovered>
        TokensLocation operator()(AnyNonCovered) const
        {
            return {{}, -1, 0, 0};
        }
    };

    inline TokensLocation GetDefinitionTokensLocation(const KindInfo& kindInfo)
    {
        return kindInfo.VisitSub(GetDefinitionNodeTokensLocation_Visitor{});
    }

    //! map SRV->T | UAV->U | Sampler->S | CBV,SrgConsant,RootConstant->B
    BindingType GetBindingType(const ExtendedTypeInfo& extendedTypeInfo);
}
