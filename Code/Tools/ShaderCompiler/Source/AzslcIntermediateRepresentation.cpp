/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "AzslcIntermediateRepresentation.h"

namespace AZ::ShaderCompiler
{
    using std::cout;
    using std::endl;

    void IntermediateRepresentation::RegisterAttributeSpecifier(AttributeScope scope,
                                                                AttributeCategory category,
                                                                size_t declarationLine,
                                                                string_view space,
                                                                string_view name,
                                                                azslParser::AttributeArgumentListContext* argList)
    {
        AttributeInfo attrInfo;
        attrInfo.m_scope      = scope;
        attrInfo.m_category   = category;
        attrInfo.m_lineNumber = declarationLine;
        attrInfo.m_namespace  = space;
        attrInfo.m_attribute  = name;

        if (argList)
        {
            for (auto arg : argList->attributeArguments()->literal())
            {
                if (!arg->StringLiteral().empty())
                {
                    for (int i = 0; i < arg->StringLiteral().size(); ++i)
                    {
                        attrInfo.m_argList.push_back(arg->StringLiteral(i)->getText());
                    }
                }
                else if (arg->IntegerLiteral())
                {
                    attrInfo.m_argList.push_back(m_sema.FoldEvalStaticConstExprNumericValue(arg->IntegerLiteral()));
                }
                else if (arg->FloatLiteral())
                {
                    attrInfo.m_argList.push_back(m_sema.FoldEvalStaticConstExprNumericValue(arg->FloatLiteral(), false));
                }
                else if (arg->True())
                {
                    attrInfo.m_argList.push_back(true);
                }
                else if (arg->False())
                {
                    attrInfo.m_argList.push_back(false);
                }
                else
                {
                    // A new case has been added - it should be resolved properly
                    assert(false);
                }
            }
        }

        if (ProcessAttributeSpecifier(attrInfo))
        {
            m_symbols.PushPendingAttribute(attrInfo, scope);
        }
    }

    bool IntermediateRepresentation::ProcessAttributeSpecifier(const AttributeInfo& attrInfo)
    {
        // Hint for the pixel output format for all or one of the render targets
        if (attrInfo.m_attribute == "output_format")
        {
            bool isDefault = (attrInfo.m_argList.size() == 1 && holds_alternative<string>(attrInfo.m_argList[0]));
            bool isIndexed = (attrInfo.m_argList.size() == 2 && holds_alternative<string>(attrInfo.m_argList[1]) && holds_alternative<ConstNumericVal>(attrInfo.m_argList[0]));
            if (!isDefault && !isIndexed)
            {
                return true;
            }

            OutputFormat hint = OutputFormat::FromStr(Trim(get<string>(attrInfo.m_argList[isDefault ? 0 : 1]), "\""));

            if (isDefault)
            {
                std::for_each(m_metaData.m_outputFormatHint, m_metaData.m_outputFormatHint + 8, [&](OutputFormat& fmt) { fmt = hint; });
            }
            else
            {
                auto rtIndex = ExtractValueAsInt64(get<ConstNumericVal>(attrInfo.m_argList[0]), std::numeric_limits<int64_t>::min());
                if (rtIndex >= 0 && rtIndex <= 7)
                {
                    m_metaData.m_outputFormatHint[rtIndex] = hint;
                }
            }
        }
        else if (attrInfo.m_attribute == "pad_to")
        {
            m_padToAttributeMutator.ProcessPadToAttribute(attrInfo);
            return false; //Do not store this attribute
        }
        // The attribute has no special meaning to AZSLc, just pass it
        return true;
    }

    void IntermediateRepresentation::RegisterTokenToNodeAssociation(ssize_t tokenId, antlr4::ParserRuleContext* node)
    {
        m_tokenMap.SetAssociation(tokenId, node);
    }

    //! execute any logic that relates to intermediate treatment that would need to be done between front end and back end
    void IntermediateRepresentation::MiddleEnd(const MiddleEndConfiguration& middleEndconfigration,
                                               PreprocessorLineDirectiveFinder* lineFinder)
    {
        // At this point we have an order apparition vector that stores symbols in the immediate naive
        // order in which they are first seen in the source code.
        //
        // However for code re-emission, it's not really the ordering we need. The ordering we need 
        // corresponds exactly to the "head recursion visit of AST nodes".
        // We're not going to transform our flat vector to tree and to vector again,
        // instead we'll shift around the elements to respect that order.
        // And to do so, we'll use a dependency DAG and a topological solver.
        verboseCout << "--Middle End--\n";

        m_symbols.ReorderBySymbolDependency();

        RegisterRootConstantStruct(middleEndconfigration);

        m_padToAttributeMutator.RunMutationsForPadToAttributes(middleEndconfigration);

        if (!middleEndconfigration.m_skipAlignmentValidation)
        {
            ValidateAlignmentIssueWhenScalarOrFloat2PrecededByMatrix(middleEndconfigration, lineFinder);
        }
    }

    bool IntermediateRepresentation::Validate()
    {
        auto hasOptions = false;
        for (auto& [uid, varInfo] : m_symbols.GetOrderedSymbolsOfSubType_2<VarInfo>())
        {
            if (varInfo->CheckHasStorageFlag(StorageFlag::Option))
            {
                hasOptions = true;
                break;
            }
        }

        auto variantFallbackGroups = 0;
        for (auto srgInfo : GetOrderedSubInfosOfSubType<SRGInfo>())
        {
            if (!srgInfo->m_semantic)
            {
                auto errorMsg = FormatString("Missing ShaderResourceGroupSemantic for ShaderResourceGroup [%s]", srgInfo->m_declNode->getText().c_str());
                throw AzslcIrException(IR_SRG_WITHOUT_SEMANTIC, errorMsg);
            }

            auto* semanticAsClassInfo = GetSymbolSubAs<ClassInfo>(srgInfo->m_semantic->GetName());
            optional<int64_t>& variantFallback = semanticAsClassInfo->Get<SRGSemanticInfo>()->m_variantFallback;
            if (variantFallback)
            {
                variantFallbackGroups++;

                if (!hasOptions)
                {
                    PrintWarning(Warn::W1, semanticAsClassInfo->GetDeclNode()->start,
                                 "If you have no options, SRG do not need a ShaderVariantFallback");
                    variantFallback = none;
                }
            }
        }

        if (variantFallbackGroups >= 2)
        {
            throw AzslcIrException(IR_MULTIPLE_SRG_FALLBACK, "Two or more SRGs cannot all be designated as the default ShaderVariantFallback");
        }

        if (hasOptions && variantFallbackGroups == 0)
        {
            throw AzslcIrException(IR_NO_FALLBACK_ASSIGNED, "If you have non-static options, one SRG must be designated as the default ShaderVariantFallback");
        }

        return true;
    }

    vector<IdentifierUID> IntermediateRepresentation::GetChildren(const IdentifierUID& parentUid) const
    {
        vector<IdentifierUID> results;
        for (const auto& uid : m_symbols.GetOrderedSymbols())
        {
            if (uid.IsParent(parentUid))
            {
                results.push_back(uid);
            }
        }
        return results;
    }

    void IntermediateRepresentation::RemoveUnusedSrgs()
    {
        // We use this function pointer to find SRGs that have no references.
        auto unreferencedSymbolFilterFunc = +[](KindInfo* kindInfo) {
            const SRGInfo* srgInfo = kindInfo->GetSubAs<SRGInfo>();
            const bool isFallbackSrg = !!srgInfo->m_shaderVariantFallback;
            return !isFallbackSrg && (kindInfo->GetSeenats().size() == 0);
        };

        vector<IdentifierUID> unusedSrgs = GetFilteredSymbolsOfSubType<SRGInfo>(unreferencedSymbolFilterFunc);
        while (!unusedSrgs.empty())
        {
            // For any given symbol, collects all the scope intervals owned by the symbol or any of its children.
            auto getSymbolScopesFunc = [&](const IdentifierUID& srgId) -> vector<misc::Interval>
            {
                vector<misc::Interval> intervals;
                for (const auto & [symId, interval] : m_scope.m_scopeIntervals)
                {
                    if ((symId.m_name == srgId.m_name) || symId.IsParent(srgId))
                    {
                        intervals.push_back(interval);
                    }
                }
                return intervals;
            };

            // With this filter we can extract symbols that are being referenced.
            auto referencedSymbolFilterFunc = +[](KindInfo* kindInfo) {
                return !kindInfo->GetSeenats().empty();
            };

            decltype(unusedSrgs) usedSrgs = GetFilteredSymbolsOfSubType<SRGInfo>(referencedSymbolFilterFunc);

            for (const auto& srgUid : unusedSrgs)
            {
                // We will place here the scopes owned by the unused SRG.
                // it must be a vector of intervals because thanks to "partial" SRGs
                // the intervals may split. The key idea is that before an unused SRG is removed from
                // the symbols table, we will loop through the used SRGs and check if their Seenats
                // fall within an unused SRG scope interval. The matching Seenats will be removed from the used SRGs.
                // In turn if all Seenats for any given used SRG are removed then the SRG becomes unused.
                vector<misc::Interval> unusedSrgScopes = getSymbolScopesFunc(srgUid);

                vector<IdentifierUID> symbolsToDelete = GetChildren(srgUid);
                for (const auto& uid : symbolsToDelete)
                {
                    // Before removing the symbol let's remove it from the Seenat of the Srgs
                    // that may have a reference to it.
                    m_symbols.DeleteIdentifier(uid);
                }

                // In this loop used SRGs may become unused SRGs.
                // The loop removes Seenats from used SRGs when they fall within the scope intervals
                // of an unused SRG.
                for (const auto& usedSrgId : usedSrgs)
                {
                    KindInfo* usedSrgKindInfo = GetKindInfo(usedSrgId);
                    vector<Seenat>& seenAts = usedSrgKindInfo->GetSeenats();
                    seenAts.erase(std::remove_if(seenAts.begin(), seenAts.end(), [&](const Seenat& seenAt) -> bool {
                        return std::any_of(unusedSrgScopes.begin(), unusedSrgScopes.end(), [&](const misc::Interval& interval) -> bool {
                            return (seenAt.m_where.m_focusedTokenId >= interval.a) && (seenAt.m_where.m_focusedTokenId <= interval.b);
                            });
                    }), seenAts.end());
                }

                //Finally, remove the SRG.
                m_symbols.DeleteIdentifier(srgUid);
            }

            //Refresh, there could be new unused SRGs.
            unusedSrgs = GetFilteredSymbolsOfSubType<SRGInfo>(unreferencedSymbolFilterFunc);
        }

    }

    string ToYaml(const TypeRefInfo& tref, const IntermediateRepresentation& ir, string_view indent)
    {
        if (tref.IsEmpty())
        {
            return "<NA>";
        }
        string r;
        r += "{name: \"" + tref.m_typeId.m_name + "\", ";
        r += "validity: ";
        r += ir.m_symbols.HasIdentifier(tref.m_typeId.m_name) ? "found, " : "undeclared, ";
        r += "tclass: ";
        r += TypeClass::ToStr(tref.m_typeClass);
        r += ", ";
        r += "underlying_scalar: " + tref.m_arithmeticInfo.UnderlyingScalarToStr() + "}";
        return r;
    }

    string ToYaml(const ExtendedTypeInfo& ext, const IntermediateRepresentation& ir, string_view indent)
    {
        auto coreName = ext.m_coreType;
        auto gnicName = ext.m_genericParameter;
        string r{indent};
        r += "core: ";
        r += ToYaml(coreName, ir, string{indent} + "  ");
        r += "\n";
        r += indent.data();
        r += "generic: ";
        r += ToYaml(gnicName, ir, string{indent} + "  ");
        return r;
    }

    void DumpSymbols(IntermediateRepresentation& ir)
    {
        std::unordered_set<IdentifierUID> seen; // avoid functions delc/def repetitions
        // in YAML form
        for (auto& uid : ir.m_symbols.GetOrderedSymbols())
        {
            if (seen.find(uid) != seen.end())
            {
                continue;
            }
            seen.insert(uid);
            auto& [_, sym] = *ir.m_symbols.GetIdAndKindInfo(uid.m_name);
            assert(uid == _);
            cout << "Symbol " << Decorate("'", uid.m_name) << ":\n";
            cout << "  kind: " << Kind::ToStr(sym.GetKind()) << "\n";
            cout << "  references:\n" << ToYaml(sym.GetSeenats().begin(), sym.GetSeenats().end(), "    ");
            switch (sym.GetKind())
            {
            case Kind::Variable:
            {
                auto& sub = sym.GetSubRefAs<VarInfo>();
                // We treat some attributes as static const-s for consistency with the grammar and rest of the code, 
                //  but they don't have actual type or declaration line, so we skip them here.
                cout << "  line: " << (sub.m_declNode ? std::to_string(sub.m_declNode->start->getLine()) : "NA") << "\n";
                cout << "  type:\n" << ToYaml(sub.m_typeInfoExt, ir, "    ") << "\n";
                cout << "  storage: " << sub.m_typeInfoExt.m_qualifiers.GetDisplayName() << "\n";
                cout << "  array dim: \"" << sub.m_typeInfoExt.m_arrayDims.ToString() << "\"\n";
                cout << "  has sampler state: " << (sub.m_samplerState ? "yes\n" : "no\n");
                if (!holds_alternative<monostate>(sub.m_constVal))
                {
                    cout << "  val: " << ExtractValueAsInt64(sub.m_constVal) << "\n";
                }
            }
            break;
            case Kind::Interface:
            case Kind::Struct:
            case Kind::Class:
            case Kind::Enum:
            {
                auto& sub = sym.GetSubRefAs<ClassInfo>();
                cout << "  line: " << sub.GetOriginalLineNumber() << "\n";
                cout << "  members:\n";
                for (auto&& member : sub.GetOrderedMembers())
                {
                    auto* subFieldSym = ir.m_symbols.GetIdAndKindInfo(member.m_name);
                    cout << "    - {kind: " << Kind::ToStr(subFieldSym->second.GetKind())
                        << ", name: " << Decorate("'", member.m_name) << "}\n";
                }
            }
            break;
            case Kind::Function:
            {
                auto& sub = sym.GetSubRefAs<FunctionInfo>();
                cout << "  line: " << sym.VisitSub(GetOrigSourceLine_Visitor{}) << "\n";
                cout << "  def line: " << (sub.IsUndefinedFunction() ? "undef" : std::to_string(sub.m_defNode->start->getLine())) << "\n";
                cout << "  must override: " << sub.m_mustOverride << "\n";
                cout << "  is method: " << sub.m_isMethod << "\n";
                cout << "  is virtual: " << sub.m_isVirtual << "\n";
                cout << "  return type:\n" << ToYaml(sub.m_returnType, ir, "    ") << "\n";
                cout << "  storage: " << sub.m_returnType.m_qualifiers.GetDisplayName() << "\n";
                cout << "  has overriding children:\n" << ToYaml(sub.m_overrides.begin(), sub.m_overrides.end(), "    ");
                cout << "  is hiding base symbol: '" << (sub.m_base ? sub.m_base->m_name.c_str() : "") << "'\n";
                cout << "  parameters:\n";
                for (auto& param : sub.GetParameters(1))
                {
                    auto* varInfo = ir.GetSymbolSubAs<VarInfo>(param.m_varId.GetName());
                    if (varInfo)
                    {
                        cout << "    - name: '" << (varInfo->m_identifier.empty() ? "<unnamed>" : static_cast<string&>(varInfo->m_identifier)) << "'\n"
                             << "      type:\n" << ToYaml(varInfo->m_typeInfoExt, ir, "        ") << "\n";
                    }
                    else
                    {
                        cout << "    - non-var identifier (type?): '" << ToYaml(param.m_typeInfo, ir, "        ") << "'\n";
                    }
                }
            }
            break;
            case Kind::OverloadSet:
            {
                auto& sub = sym.GetSubRefAs<OverloadSetInfo>();
                cout << "  functions:\n";
                sub.ForEach([](auto& uid2) {cout << "    - '" << uid2.GetName() << "'\n";});
            }
            break;
            case Kind::ShaderResourceGroup:
            {
                auto& sub = sym.GetSubRefAs<SRGInfo>();
                cout << "  line: " << sub.m_declNode->start->getLine() << "\n";
                cout << "  structs: ";
                std::for_each(sub.m_structs.begin(), sub.m_structs.end(), [](auto& uid2) {cout << uid2.GetNameLeaf() << ", ";});
                cout << "\n";
                cout << "  srViews: ";
                std::for_each(sub.m_srViews.begin(), sub.m_srViews.end(), [](auto& uid2) {cout << uid2.GetNameLeaf() << ", ";});
                cout << "\n";
                cout << "  samplers: ";
                std::for_each(sub.m_samplers.begin(), sub.m_samplers.end(), [](auto& uid2) {cout << uid2.GetNameLeaf() << ", ";});
                cout << "\n";
                cout << "  CBs: ";
                std::for_each(sub.m_CBs.begin(), sub.m_CBs.end(), [](auto& uid2) {cout << uid2.GetNameLeaf() << ", ";});
                cout << "\n";
            }
            break;
            case Kind::Type:
            {
                auto& sub = sym.GetSubRefAs<TypeRefInfo>();
                cout << ToYaml(sub, ir, "  ") << "\n";
            }
            break;
            case Kind::TypeAlias:
            {
                auto& sub = sym.GetSubRefAs<TypeAliasInfo>();
                cout << "  line: " << sym.VisitSub(GetOrigSourceLine_Visitor{}) << "\n";
                cout << "  canonical type:\n" << ToYaml(sub.m_canonicalType, ir, "    ") << "\n";
            }
            break;
            default:
                break;
            }
        }
    }

    int IntermediateRepresentation::ValidateRootConstantStruct(const MiddleEndConfiguration& middleEndconfigration)
    {
        int rootConstantsSize = 0;
        for (auto& [uid, varInfo] : m_symbols.GetOrderedSymbolsOfSubType_2<VarInfo>())
        {
            if (varInfo->CheckHasAnyStorageFlags({ StorageFlag::Rootconstant }) && IsGlobal(uid.GetName()))
            {
                assert(!IsChameleon(varInfo->GetTypeClass()));

                auto exportedType = varInfo->m_typeInfoExt.m_coreType;
                if (!exportedType.IsPackable())
                {
                    throw std::logic_error{ "Internal Error: The type '"
                        + exportedType.m_typeId.m_name
                        + "' in layout member '"
                        + uid.m_name
                        + "' cannot be packed." };
                }

                // GetTotalSize of each member of the structure
                rootConstantsSize += varInfo->m_typeInfoExt.GetTotalSize(middleEndconfigration.m_packDataBuffers, middleEndconfigration.m_isRowMajor);
            }
        }

        // We have a limited space for rootconstant (specified by --root-const=N) check that we're not going over
        if (rootConstantsSize > middleEndconfigration.m_rootConstantsMaxSize)
        {
            throw std::runtime_error{ "The rootconstants size of "
                + ToString(rootConstantsSize)
                + " bytes exceeds the limit of "
                + ToString(middleEndconfigration.m_rootConstantsMaxSize)
                + "bytes supported on the target platform." };
        }

        // Spec requires at least 128 bytes of block, bigger values (>128 bytes) need to be checked against support by implementation
        constexpr int MinByteSizeRequired = 128;
        if (rootConstantsSize > MinByteSizeRequired)
        {
            PrintWarning(Warn::W2, none, "The root constant size ", rootConstantsSize, " is greater than 128 bytes. ",
                "Large root constant size may not be supported by the implementation.");
        }

        return rootConstantsSize;
    }

    uint32_t IntermediateRepresentation::CalculateLayoutForRootConstantMember(
        const IdentifierUID& memberId,
        const bool isRowMajor,
        const AZ::ShaderCompiler::Packing::Layout layoutPacking,
        const uint32_t startingOffset,
        uint32_t& nextMemberStartingOffset) const
    {
        const auto* varInfoPtr = GetSymbolSubAs<VarInfo>(memberId.m_name);
        uint32_t size = 0;

        if (varInfoPtr)
        {
            const auto& varInfo = *varInfoPtr;

            // View types should only be called from GetViewStride until we decide to support them as struct constants
            assert(!IsChameleon(varInfo.GetTypeClass()));

            auto exportedType = varInfo.m_typeInfoExt.m_coreType;

            if (!exportedType.IsPackable())
            {
                throw std::logic_error{ "error: unpackable type ("
                    + exportedType.m_typeId.m_name
                    + ") in layout member "
                    + memberId.m_name };
            }
            size = varInfo.m_typeInfoExt.GetTotalSize(layoutPacking, isRowMajor);
            auto startAt = startingOffset;
            nextMemberStartingOffset = startingOffset;

            // Alignment start
            if (exportedType.m_arithmeticInfo.IsMatrix() || exportedType.m_arithmeticInfo.IsVector())
            {
                const auto rows = exportedType.m_arithmeticInfo.m_rows;
                const auto cols = exportedType.m_arithmeticInfo.m_cols;
                const auto packAlignment = exportedType.m_arithmeticInfo.IsMatrix() ? Packing::Alignment::asMatrixStart : Packing::Alignment::asVectorStart;
                startAt = nextMemberStartingOffset = Packing::AlignOffset(layoutPacking, nextMemberStartingOffset, packAlignment, rows, cols);
            }

            // Compound types are not supported in root constants.
            assert(!IsProductType(exportedType.m_typeClass));

            if (varInfo.GetTypeClass() == TypeClass::Enum)
            {
                auto* asClassInfo = GetSymbolSubAs<ClassInfo>(varInfo.GetTypeId().GetName());
                size = asClassInfo->Get<EnumerationInfo>()->m_underlyingType.m_arithmeticInfo.m_baseSize;
            }

            nextMemberStartingOffset = Packing::PackNextChunk(layoutPacking, size, startAt);

            // Alignment end
            if (exportedType.m_arithmeticInfo.IsMatrix() || exportedType.m_arithmeticInfo.IsVector())
            {
                const auto rows = exportedType.m_arithmeticInfo.m_rows;
                const auto cols = exportedType.m_arithmeticInfo.m_cols;
                const auto packAlignment = exportedType.m_arithmeticInfo.IsMatrix() ? Packing::Alignment::asMatrixEnd : Packing::Alignment::asVectorEnd;
                nextMemberStartingOffset = Packing::AlignOffset(layoutPacking, nextMemberStartingOffset, packAlignment, rows, cols);
            }

            size = nextMemberStartingOffset - startAt;
        }

        return size;
    }

    uint32_t IntermediateRepresentation::CalculateSizeOfRootConstantsCB(
        const IdentifierUID& exportedTypeId,
        const bool isRowMajor,
        const AZ::ShaderCompiler::Packing::Layout layoutPacking) const
    {
        uint32_t memberOffset = 0;
        uint32_t largestMemberSize = 0;

        const auto* classInfo = GetSymbolSubAs<ClassInfo>(exportedTypeId.m_name);
        for (const auto& memberField : classInfo->GetMemberFields())
        {
            const auto currentStride = memberOffset;
            CalculateLayoutForRootConstantMember(memberField, isRowMajor, GetExtendedLayout(layoutPacking), currentStride, memberOffset);
            largestMemberSize = std::max(largestMemberSize, memberOffset - currentStride);
        }

        memberOffset = Packing::AlignStructToLargestMember(layoutPacking, memberOffset, largestMemberSize);
        return memberOffset;
    }

    void IntermediateRepresentation::RegisterRootConstantStruct(const MiddleEndConfiguration& middleEndconfigration)
    {
        int rootConstantByteSize = ValidateRootConstantStruct(middleEndconfigration);

        QualifiedName name{ "/Root_Constants" };

        assert(!m_symbols.HasIdentifier(name));  // reserved name semantic error must have prevented this state to happen.

        if (rootConstantByteSize == 0)
        {
            return;
        }

        // Register a struct Root_Constants and its members to ir
        auto& [rootConstantStructUid, rootConstantStructKindInfo] = m_symbols.AddIdentifier(name, Kind::Struct, none, AddIdentifierChecks::None);
        rootConstantStructKindInfo.GetSubRefAs<ClassInfo>().m_kind = Kind::Struct;
        
        for (auto& [uid, varInfo] : m_symbols.GetOrderedSymbolsOfSubType_2<VarInfo>())
        {
            if (varInfo->CheckHasAnyStorageFlags({ StorageFlag::Rootconstant })
                && IsGlobal(uid.GetName()))
            {
                assert(!IsChameleon(varInfo->GetTypeClass()));
                auto exportedType = varInfo->m_typeInfoExt.m_coreType;

                // copy the variable into a new symbol. so that its path can make sense
                // as a child of the rootconstants struct.
                QualifiedName copiedFieldName{ JoinPath(rootConstantStructUid.GetName(), ReplaceSeparators(uid.m_name, Underscore)) };
                auto& [newVarUid, newVarKind] = m_symbols.AddIdentifier(copiedFieldName, Kind::Variable);
                newVarKind.GetSubRefAs<VarInfo>() = *varInfo;
                newVarKind.GetSubRefAs<VarInfo>().m_declNode = nullptr;  // the original declaration in the AST isn't inherited by a copied symbol.
                // Add root constants to the registered struct as member
                rootConstantStructKindInfo.GetSubRefAs<ClassInfo>().PushMember(newVarUid, Kind::Variable);
            }
        }

        if (middleEndconfigration.m_padRootConstantCB)
        {
            const auto layoutPacking = middleEndconfigration.m_packConstantBuffers;
            uint32_t strideSize = CalculateSizeOfRootConstantsCB(rootConstantStructUid, middleEndconfigration.m_isRowMajor, layoutPacking);
            uint32_t endOffset = 0;
            switch (layoutPacking)
            {
                case Packing::Layout::DirectXPacking:          
                case Packing::Layout::RelaxedDirectXPacking:
                    // asStructEnd does nothing in DirectX, but in case of padding the root constant We force
                    // asStructStart in order to get 16byte alignment.
                    endOffset = Packing::AlignOffset(layoutPacking, strideSize, Packing::Alignment::asStructStart, 0, 0);
                    break;
                default:
                    endOffset = Packing::AlignOffset(layoutPacking, strideSize, Packing::Alignment::asStructEnd, 0, 0);
                    break;
            }

            // If the structure is NOT aligned, we inject a 'pad' member to enforce that padding.
            // There are 4 cases: uint1, uint2, uint3, or uint4.
            if (endOffset > strideSize)
            {
                uint32_t padSize = endOffset - strideSize;

                QualifiedName padFieldName{ JoinPath(rootConstantStructUid.GetName(), "__pad__") };
                auto& [newVarUid, newVarKind] = m_symbols.AddIdentifier(padFieldName, Kind::Variable);

                VarInfo varInfo;
                varInfo.m_declNode = nullptr;
                varInfo.m_isPublic = false;

                assert(padSize == 16 || padSize == 12 || padSize == 8 || padSize == 4);
                string typeName = FormatString("uint%d", padSize / 4);

                ExtractedTypeExt padType = { UnqualifiedNameView(typeName), nullptr };
                varInfo.m_typeInfoExt = ExtendedTypeInfo{m_sema.CreateTypeRefInfo(padType), {},
                                                         {}, {}, {}, Packing::MatrixMajor::Default };

                newVarKind.GetSubRefAs<VarInfo>() = varInfo;
                rootConstantStructKindInfo.GetSubRefAs<ClassInfo>().PushMember(newVarUid, Kind::Variable);
            }
        }

        m_rootConstantStructUID = rootConstantStructUid;
    }

    void IntermediateRepresentation::ValidateAlignmentIssueWhenScalarOrFloat2PrecededByMatrix(const MiddleEndConfiguration& middleEndconfigration,
                                                                                              PreprocessorLineDirectiveFinder* lineFinder)
    {
        //! Helper lambda to check if a symbol is a matrix, it also returns
        //! the number of columns in @numColumns
        auto isMatrix = [&](const IdentifierUID& symbolUid, int& numColumns)
        {
            KindInfo* kindInfo = GetKindInfo(symbolUid);
            const VarInfo* varInfo = kindInfo->GetSubAs<VarInfo>();
            if (!varInfo)
            {
                // Not a variable.
                return false;
            }
            if (varInfo->GetTypeClass() != TypeClass::Matrix)
            {
                // Not a matrix.
                return false;
            }
            const auto* arithmeticInfo = &varInfo->m_typeInfoExt.m_coreType.m_arithmeticInfo;
            if (arithmeticInfo->m_rows < 2)
            {
                return false;
            }
            numColumns = arithmeticInfo->m_cols;
            if (symbolUid.GetName().find("(") != string::npos)
            {
                // It's the return value of a function or a function argument. Skip.
                return false;
            }
            return true;
        };

        //! Helper lambda. Checks if @varInfo size is 8 or smaller.
        //! The actual size in bytes is returned in sizeInBytes.
        auto isVariableOf8BytesOrLess = [](const MiddleEndConfiguration& middleEndconfigration, const VarInfo* varInfo, size_t& sizeInBytes)
        {
            if (!IsFundamental(varInfo->GetTypeClass()))
            {
                return false;
            }
            if (varInfo->m_typeInfoExt.IsArray())
            {
                return false;
            }
            const auto byteCount = varInfo->m_typeInfoExt.GetTotalSize(middleEndconfigration.m_packDataBuffers, middleEndconfigration.m_isRowMajor);
            if (byteCount > 8)
            {
                return false;
            }
            sizeInBytes = byteCount;
            return true;
        };

        // We'll loop through all symbols.
        const auto& orderedSymbols = m_symbols.m_elastic.m_order;
        // Keep record of the structs that end in float2x2, float3x2, float4x2
        unordered_set<IdentifierUID> structsWithMatrixRx2AsLastField;
        // and also the structs that end in float2x3, float3x3, float4x3
        unordered_set<IdentifierUID> structsWithMatrixRx3AsLastField;

        enum class PrepadType
        {
            Float2,
            Float3,
            Unknown,
        };
        struct UidWithPrepadType
        {
            PrepadType m_prepadType;
            IdentifierUID m_uid;
        };
        vector<UidWithPrepadType> symbolsToPrepad;
        for (size_t symbolIndex = 0; symbolIndex < orderedSymbols.size(); ++symbolIndex)
        {
            const IdentifierUID uid = orderedSymbols[symbolIndex];
            KindInfo* kindInfo = GetKindInfo(uid);
            if (IsGlobal(uid.GetName()))
            {
                continue; // no reason to align things in the global scope,
                          // options and rootconstant will be align-checked through their implicit structs.
            }

            // We need to keep track of the structs for which the last member is
            // a float3x3.
            const bool isStructOrClass = kindInfo->IsKindOneOf(Kind::Class, Kind::Struct);
            if (isStructOrClass)
            {
                auto& classInfo = kindInfo->GetSubRefAs<ClassInfo>();
                const auto& memberFieldList = classInfo.GetMemberFields();
                if (memberFieldList.empty())
                {
                    continue;
                }
                const auto memberCount = memberFieldList.size();
                auto& lastMemberUid = memberFieldList[memberCount - 1];
                int numColumns = 0;
                if (isMatrix(lastMemberUid, numColumns))
                {
                    if (numColumns == 2)
                    {
                        structsWithMatrixRx2AsLastField.insert(uid);
                    }
                    else if (numColumns == 3)
                    {
                        structsWithMatrixRx3AsLastField.insert(uid);
                    }
                }
                continue;
            }

            const VarInfo* varInfo = kindInfo->GetSubAs<VarInfo>();
            if (!varInfo)
            {
                continue;
            }

            size_t variableSizeInBytes;
            if (!isVariableOf8BytesOrLess(middleEndconfigration, varInfo, variableSizeInBytes))
            {
                continue;
            }

            if (uid.GetName().find("(") != string::npos)
            {
                // It's a function argument. Skip.
                continue;
            }

            // We only care if the previous VarInfo is one of:
            // 1- A struct or class whose last member is a float2x2, float3x2, float4x2,
            //    float2x3, float3x3, float4x3.
            // OR
            // 2- a float2x2, float3x2, float4x2,
            //    float2x3, float3x3, float4x3.
            //! Helper lambda to get the previously declared variable.
            auto getPreviousVarInfo = [&](ssize_t& startSearchSymbolIndex /*in out*/, string_view scope) -> VarInfo*
            {
                while (startSearchSymbolIndex >= 0)
                {
                    const auto& prevSymbolUid = orderedSymbols[startSearchSymbolIndex];
                    if (GetParentName(prevSymbolUid.GetName()) != scope)
                    {
                        return nullptr;  // we can't relate variables that don't even belong to same struct or srg
                    }
                    KindInfo* prevKindInfo = GetKindInfo(prevSymbolUid);
                    VarInfo* prevVarInfo = prevKindInfo->GetSubAs<VarInfo>();
                    if (prevVarInfo)
                    {
                        return prevVarInfo;
                    }
                    startSearchSymbolIndex--;
                }
                return nullptr;
            };

            ssize_t prevSymbolIndex = symbolIndex - 1;
            const VarInfo* prevVarInfo = getPreviousVarInfo(prevSymbolIndex, GetParentName(uid.GetName()));
            if (prevVarInfo == nullptr)
            {
                // Nothing to do, keep going.
                continue;
            }

            // Is it a Matrix?
            const auto& prevSymbolUid = orderedSymbols[prevSymbolIndex];
            int numColumns = 0;
            if (isMatrix(prevSymbolUid, numColumns))
            {
                if (numColumns == 2)
                {
                    symbolsToPrepad.push_back(UidWithPrepadType{PrepadType::Float3, uid});
                }
                else if ((numColumns == 3) && (variableSizeInBytes <= 4))
                {
                    symbolsToPrepad.push_back(UidWithPrepadType{PrepadType::Float2, uid});
                }
                continue;
            }
            
            // Is the type a struct or class whose last member is a Matrix of interest?
            const auto prevVarTypeUid = prevVarInfo->m_typeInfoExt.m_coreType.m_typeId;
            if (structsWithMatrixRx2AsLastField.count(prevVarTypeUid))
            {
                symbolsToPrepad.push_back(UidWithPrepadType{PrepadType::Float3, uid});
            }
            else if (structsWithMatrixRx3AsLastField.count(prevVarTypeUid) && (variableSizeInBytes <= 4))
            {
                symbolsToPrepad.push_back(UidWithPrepadType{PrepadType::Float2, uid});
            }

        } // for loop end.

        //! Helper function that returns a string suggesting padding solution
        auto getPaddingSolutionMessage = [&](PrepadType prepadType, IdentifierUID insertBeforeThisUid) -> string
        {
            // We can deduce the name of parent struct, class or SRG from the name of the field that should come AFTER
            // the dummy float2/float3.
            auto parentName = GetParentName(insertBeforeThisUid.GetName());

            // The padding is not needed if the variable @insertBeforeThisUid is the first member
            // of a struct/class/SRG
            KindInfo* parentKindInfo = GetKindInfo({ parentName });
            assert(parentKindInfo);
            const bool isClass = parentKindInfo->IsKindOneOf(Kind::Class);
            const bool isStruct = parentKindInfo->IsKindOneOf(Kind::Struct);
            const bool isSrg = parentKindInfo->IsKindOneOf(Kind::ShaderResourceGroup);
            if (isStruct || isClass)
            {
                auto firstMemberUid = parentKindInfo->GetSubRefAs<ClassInfo>().GetMemberFields()[0];
                if (firstMemberUid == insertBeforeThisUid)
                {
                    return {};
                }
            }
            else if (isSrg)
            {
                auto& srgInfo = parentKindInfo->GetSubRefAs<SRGInfo>();
                auto firstMemberUid = srgInfo.m_implicitStruct.GetMemberFields()[0];
                if (firstMemberUid == insertBeforeThisUid)
                {
                    return {};
                }
            }
            else
            {
                throw std::logic_error{ "error: Was expecting " + string(parentName) +
                    + " to be struct, class or ShaderResourceGroup" };
            }

            string typeName;
            if (isClass)
            {
                typeName = "class";
            }
            else if (isStruct)
            {
                typeName = "struct";
            }
            else
            {
                typeName = "ShaderResourceGroup";
            }

            // Let's get the line number where @insertBeforeThisUid was found in the flat AZSL file.
            const auto* constThis = this; // To disambiguate which cv-version of GetSymbolSubAs<> to call.
            const auto* varInfo = constThis->GetSymbolSubAs<VarInfo>(insertBeforeThisUid.GetName());
            size_t lineOfDeclaration = varInfo->GetOriginalLineNumber();
            const LineDirectiveInfo* lineInfo = lineFinder->GetNearestPreprocessorLineDirective(lineOfDeclaration);
            if (!lineInfo || lineOfDeclaration == 0)
            {
                // When the LineDirectiveInfo* is null (or at 0), it means We have detected a variable that was added
                // by AZSLc itself. e.g. Root Constant padding, etc.
                // In such case, this is not an issue We want to interfere with.
                return {};
            }
            const auto virtualLine = lineFinder->GetVirtualLineNumber(*lineInfo, lineOfDeclaration);
            string solution = FormatString("- A 'float%d' variable should be added before the variable '%s' in '%s %s' at Line number %zu of '%s'\n",
                                           prepadType == PrepadType::Float2 ? 2 : 3, insertBeforeThisUid.GetNameLeaf().c_str(), typeName.c_str(), parentName.data(),
                                           virtualLine, lineInfo->m_containingFilename.c_str());
            return solution;
        };

        string solutionsReport;
        for (const auto& uidAndPrepadType : symbolsToPrepad)
        {
            solutionsReport += getPaddingSolutionMessage(uidAndPrepadType.m_prepadType, uidAndPrepadType.m_uid);
        }

        if (solutionsReport.empty())
        {
            return;
        }

        string errorMessage(
            "Detected potential alignment issues related with DXC flag '-fvk-use-dx-layout'.\n"
            "Alternatively you can use the option '--no-alignment-validation' to void this error and compile as is.:\n");
        errorMessage += solutionsReport;
        throw AzslcIrException(IR_POTENTIAL_DX12_VS_VULKAN_ALIGNMENT_ERROR, errorMessage);
    }

    IdAndKind& IntermediateRepresentation::GetCurrentScopeIdAndKind()
    {
        auto nameOfScope = m_scope.GetNameOfCurScope();
        auto* idAndKindPtr = m_symbols.GetIdAndKindInfo(nameOfScope);
        if (!idAndKindPtr)
        {
            throw std::logic_error("Internal error: current scope not registered");
        }
        return *idAndKindPtr;
    }

    IdentifierUID IntermediateRepresentation::GetLastMemberVariable(const IdentifierUID& uid)
    {
        auto kind = GetKind(uid);
        ClassInfo* classInfo = nullptr; 
        if (kind.IsOneOf(Kind::Struct, Kind::Class))
        {
            classInfo = GetSymbolSubAs<ClassInfo>(uid.GetName());
        }
        else if (kind.IsOneOf(Kind::ShaderResourceGroup))
        {
            auto srgInfo = GetSymbolSubAs<SRGInfo>(uid.GetName());
            classInfo = &srgInfo->m_implicitStruct;
        }

        if (!classInfo)
        {
            return {};
        }

        const auto& memberList = classInfo->GetMemberFields();
        if (memberList.empty())
        {
            return {};
        }
        return memberList[memberList.size() - 1];
    }
}  // end of namespace AZ::ShaderCompiler
