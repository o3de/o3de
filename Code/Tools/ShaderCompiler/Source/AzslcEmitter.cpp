/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "AzslcEmitter.h"

#include <tuple>
#include <cmath>
#include <filesystem>
namespace StdFs = std::filesystem;

// We should only include the base platform emitter
// Every specific implementation is supplied via a factory get method
#include "AzslcPlatformEmitter.h"

namespace AZ::ShaderCompiler
{
    // to activate argument dependent lookup from template utilities in AzslcUtils, this must be in a reachable namespace
    Streamable& operator << (Streamable& out, const AttributeInfo::Argument& arg)
    {
        if (holds_alternative<string>(arg))
        {
            return out << get<string>(arg);
        }

        else if (holds_alternative<ConstNumericVal>(arg))
        {
            const auto& constVal = get<ConstNumericVal>(arg);

            if (holds_alternative<int32_t>(constVal) || holds_alternative<uint32_t>(constVal))
            {
                return out << ExtractValueAsInt64(constVal, std::numeric_limits<int64_t>::min());
            }
            else if (holds_alternative<float>(constVal))
            {
                return out << ExtractValueAsFloat(constVal, std::numeric_limits<float>::infinity());
            }
        }

        else if (holds_alternative<bool>(arg))
        {
            return out << get<bool>(arg);
        }

        return out;
    }

    Streamable& operator << (Streamable& out, const AttributeInfo& attr)
    {
        if (!attr.m_namespace.empty())
        {
            out << attr.m_namespace << "::";
        }

        out << attr.m_attribute;

        if (!attr.m_argList.empty())
        {
            out << "(" << Join(attr.m_argList, ", ") << ")";
        }
        return out;
    }

    using std::for_each;

    void CodeEmitter::Run(const Options& options)
    {
        const uint32_t numOf32bitConst = GetNumberOf32BitConstants(options, m_ir->m_rootConstantStructUID);
        const RootSigDesc rootSig = BuildSignatureDescription(options, numOf32bitConst);

        SetupScopeMigrations(options);
        SetupOptionsSpecializationId(options);

        // Emit global attributes
        for (const auto& attr : m_ir->m_symbols.GetGlobalAttributeList())
        {
            EmitAttribute(attr);
        }
        
        EmitGetterFunctionDeclarationsForRootConstants(m_ir->m_rootConstantStructUID);

        // loop on the (mostly) user-defined order of code entities, and emit
        for (const IdentifierUID& iteratedSymbolUid : m_ir->m_symbols.GetOrderedSymbols())
        {
            const QualifiedNameView iteratedSymbolName = iteratedSymbolUid.GetName();
            const Kind iteratedSymbolKind = m_ir->GetKind(iteratedSymbolUid);

            switch (iteratedSymbolKind)
            {
                // top-level enums, structs and classes, as well as immediate-type-declaration enum/structs (`struct S{} s;`)
            case Kind::Interface:
            case Kind::Struct:
            case Kind::Class:
            case Kind::Enum:
            {
                if (IsTopLevelThroughTranslation(iteratedSymbolUid))
                {
                    EmitPreprocessorLineDirective(iteratedSymbolName);

                    auto* classInfo = m_ir->GetSymbolSubAs<ClassInfo>(iteratedSymbolName);
                    iteratedSymbolKind == Kind::Enum ?
                          EmitEnum(iteratedSymbolUid, *classInfo, options)
                        : EmitStruct(*classInfo, iteratedSymbolName, options);
                }
                break;
            }
                // typedefs
            case Kind::TypeAlias:
            {
                if (IsTopLevelThroughTranslation(iteratedSymbolUid))
                {
                    EmitPreprocessorLineDirective(iteratedSymbolName);

                    auto* aliasInfo = m_ir->GetSymbolSubAs<TypeAliasInfo>(iteratedSymbolName);
                    EmitTypeAlias(iteratedSymbolUid, *aliasInfo, options);
                }
                break;
            }
                // global variables
            case Kind::Variable:
            {
                if (IsTopLevelThroughTranslation(iteratedSymbolUid))
                {
                    auto* varInfo = m_ir->GetSymbolSubAs<VarInfo>(iteratedSymbolName);
                    if (varInfo->CheckHasStorageFlag(StorageFlag::Enumerator))
                    {   // Enumerators have already been emitted in the Kind::Enum case.
                        // They act as static const, but no need to emit them here
                        break;
                    }

                    // Option variables are emitted after the ShaderVariantFallback SRG
                    if (varInfo->CheckHasStorageFlag(StorageFlag::Option))
                    {
                        EmitShaderVariantOptionVariableDeclaration(iteratedSymbolUid, options);
                        break;
                    }

                    // a non-global extern is an SRG-variable. it should be emitted by EmitSRG
                    bool global = IsGlobal(iteratedSymbolName);
                    if (!global && !varInfo->StorageFlagIsLocalLinkage(global || varInfo->m_srgMember))
                    {
                        break;
                    }

                    EmitPreprocessorLineDirective(iteratedSymbolName);
                    EmitVariableDeclaration(*varInfo, iteratedSymbolUid, options, VarDeclHasFlag(VarDeclHas::Initializer));
                    m_out << ";\n";
                }
                break;
            }
                // SRG
            case Kind::ShaderResourceGroup:
            {
                EmitPreprocessorLineDirective(iteratedSymbolName);

                auto* srgSub = m_ir->GetSymbolSubAs<SRGInfo>(iteratedSymbolName);
                EmitSRG(*srgSub, iteratedSymbolUid, options, rootSig);
                break;
            }
                // function
            case Kind::Function:
            {
                EmitPreprocessorLineDirective(iteratedSymbolName);

                auto* funcSub = m_ir->GetSymbolSubAs<FunctionInfo>(iteratedSymbolName);
                const bool alreadyDeclared = AlreadyEmittedFunctionDeclaration(iteratedSymbolUid);
                assert(!funcSub->IsEmpty());
                const EmitFunctionAs form = (funcSub->HasUniqueDeclarationThroughDefinition() || alreadyDeclared) ?
                    EmitFunctionAs::Definition : EmitFunctionAs::Declaration;
                EmitFunction(*funcSub, iteratedSymbolUid, form, options);
                break;
            }
            default: break;
            } // end switch on code entities kind
        } // end for all entities

        EmitRootConstants(rootSig, options);

        EmitShaderVariantOptionGetters(options);

        if (options.m_emitRootSig)
        {
            m_out << GetPlatformEmitter().GetRootSig(*this, rootSig, options, BindingPair::Set::Merged);
        }
    }

    //! azslSymbolName is going to be considered as a startup point of migration (e.g a scope)
    //! and all symbols under that scope will be migrated equivalently.
    void CodeEmitter::MigrateASTSubTree(const IdentifierUID& azslSymbol, QualifiedNameView landingScope)
    {
        m_translations.RegisterLandingScope(azslSymbol, landingScope);

        // special case for non-scoped enums because they have dependent symbols that are not children
        if (m_ir->GetKind(azslSymbol) == Kind::Enum)
        {
            auto* sub = m_ir->GetSymbolSubAs<ClassInfo>(azslSymbol.GetName());
            bool scoped = sub->Get<EnumerationInfo>()->m_isScoped;
            if (!scoped)
            {
                // migrate enumerators too
                for (auto& enumerator : sub->GetMemberFields())
                {
                    m_translations.RegisterLandingScope(enumerator, landingScope);
                }
            }
        }
    }

    bool CodeEmitter::IsTopLevelThroughTranslation(const IdentifierUID& uid) const
    {
        return m_translations.GetLandingScope(uid.GetName()) == QualifiedNameView{"/"};
    }

    //! setup all scope migrations (srg content to global, local structs to global)
    void CodeEmitter::SetupScopeMigrations(const Options& options)
    {
        m_translations.SetAccessSymbolQueryFunctor([this](QualifiedNameView qnv){return m_ir->GetKindInfo(IdentifierUID{qnv});});
        m_translations.SetGetSeenatFunctor([this](QualifiedNameView qnv) -> vector<Seenat>&
                                            {
                                                auto* uidkind = m_ir->GetIdAndKindInfo(qnv);
                                                if (uidkind)
                                                {
                                                    return uidkind->second.GetSeenats();
                                                }
                                                static vector<Seenat> s_empty;
                                                return s_empty;
                                            });

        auto globalScope = QualifiedNameView{ "/" };
        
        // Global root constant custom behavior
        for (auto&[uid, info] : m_ir->GetOrderedSymbolsOfSubType_2<VarInfo>())
        {
            bool isAlreadyGlobal = IsGlobal(uid.GetName());
            bool isNotContainedInType = !m_ir->IsNestedStructOrEnum(uid);
            if (isAlreadyGlobal && isNotContainedInType)
            {
                auto* varInfo = m_ir->GetSymbolSubAs<VarInfo>(uid.GetName());
                const auto& rootconstantClassInfo = *m_ir->GetSymbolSubAs<ClassInfo>(m_ir->m_rootConstantStructUID.m_name);

                if (options.m_rootConstantsMaxSize && rootconstantClassInfo.HasMember(uid.GetNameLeaf())
                    && varInfo->CheckHasAllStorageFlags({ StorageFlag::Rootconstant }))
                {
                    IdentifierUID constructRootconstantReference = *rootconstantClassInfo.FindMemberFromLeafName(uid.GetNameLeaf());
                    m_translations.RegisterLandingScope(uid, m_ir->m_rootConstantStructUID.GetName());
                    m_translations.AddCustomBehavior(uid.GetName(),
                        BehaviorEvent::OnReference,
                        [constructRootconstantReference](QualifiedNameView, UsageContext, [[maybe_unused]] string proposition, ssize_t)
                    {
                        // Construct the rootconstant member name which is declared as global variable with the use of get functions.
                        // Type _g_MyRootConstVar = GetShaderRootConst_Member(); So now we just return _g_MyRootConstVar;
                        return GetGlobalRootConstantVarName(constructRootconstantReference.GetName());
                    });
                }
            }
        }

        // all SRGs -> erased. Migrate all their contents to global.
        for (auto& [srgUID, srgInfo] : m_ir->GetOrderedSymbolsOfSubType_2<SRGInfo>())
        {
            array<decltype(srgInfo->m_structs)*, 6> allSrgMembersUidArrays = {&srgInfo->m_structs,
                                                                              &srgInfo->m_srViews,
                                                                              &srgInfo->m_samplers,
                                                                              &srgInfo->m_CBs,
                                                                              &srgInfo->m_nonexternVariables,
                                                                              &srgInfo->m_functions};
            for (auto& array : allSrgMembersUidArrays)
            {
                for (auto& member : *array)
                {
                    MigrateASTSubTree(member, globalScope);
                }
            }

            // variables get special treatment in case of non-emitConstantBufferBody, because SRG-constants go in a generated-struct: <SRGNAME>_SRGConstantStruct
            for (auto& member : srgInfo->m_implicitStruct.GetMemberFields())
            {
                auto constantsStruct = MakeSrgConstantsStructName(srgUID);
                MigrateASTSubTree(member, options.m_emitConstantBufferBody ? globalScope : QualifiedNameView{constantsStruct});
            }

            // add a special behavior for CB, because under cb-body switch, CBs are views, thus must be indexed.
            if (options.m_emitConstantBufferBody)
            {
                for_each(srgInfo->m_CBs.begin(), srgInfo->m_CBs.end(), [this](const IdentifierUID& viewUid)
                         {
                             // all buffer types references must be mutated to [0]
                             assert(m_ir->GetSymbolSubAs<VarInfo>(viewUid.GetName())->GetTypeClass() == TypeClass::ConstantBuffer);
                             m_translations.AddCustomBehavior(viewUid.GetName(),
                                                              BehaviorEvent::OnReference,
                                                              [this](QualifiedNameView, UsageContext, const string& proposition, ssize_t tokenId)
                                                              {
                                                                  // in cb-body mode, CB are converted to views, a non-subscripted expression
                                                                  // needs to be abstracted away (by adding [0]) to feel like a constant buffer access.
                                                                  // but since we can have arrays of constant buffers in SRG, the user may have already subscripted
                                                                  // the access, in that case we can't double the subscript or we'll emit an ill-formed program.
                                                                  auto* ast = m_ir->m_tokenMap.GetNode(tokenId);
                                                                  return proposition + (IsNextToArrayAccessExpression(ast) ? "" : "[0]");
                                                              });
                         });
            }
            else // in the else-case, we have dedicated structures for SRG constants, and references need a custom behavior
            {
                for_each(srgInfo->m_implicitStruct.GetMemberFields().begin(), srgInfo->m_implicitStruct.GetMemberFields().end(), [this, srgUID=srgUID](IdentifierUID fieldUid)
                         {
                            // the natural suggestion from the translation system is going to be
                            // to mutate MyRsc::m_f4 to MyRsc_SRGConstantStruct::MyRsc_m_f4
                            // which is the "address" of the variable declaration, but not where the instance reside.
                            // we need to mutate this to MyRsc_SRGConstantBuffer.MyRsc_m_f4
                            m_translations.AddCustomBehavior(fieldUid.GetName(),
                                                            BehaviorEvent::OnReference,
                                                            [srgUID = srgUID](QualifiedNameView, UsageContext, string proposition, ssize_t)
                                                            {
                                                                string constantBufferId = UnMangle(MakeSrgConstantsCBName(srgUID));
                                                                string translatedFieldId = UnMangle(string{ExtractLeaf(ReMangle(proposition))});
                                                                return constantBufferId + "." + translatedFieldId;
                                                            });
                         });
            }
        }

        // structs/classes in functions/arguments/generic-parameters, may not be valid HLSL scopes to hold types -> migrate them to global
        for (auto& [uid, info] : m_ir->GetOrderedSymbolsOfSubType_2<ClassInfo>())
        {
            bool isAlreadyGlobal = IsGlobal(uid.GetName());
            bool isNotContainedInType = !m_ir->IsNestedStructOrEnum(uid);
            if (!isAlreadyGlobal && isNotContainedInType)
            {
                MigrateASTSubTree(uid, globalScope);
            }
        }

        // DXC has made into an error the definition of typedef in classes -> move them all to global
        for (auto& [typeAliasUid, typeAliasInfo] : m_ir->GetOrderedSymbolsOfSubType_2<TypeAliasInfo>())
        {
            if (!IsGlobal(typeAliasUid.GetName()))
            {
                MigrateASTSubTree(typeAliasUid, globalScope);
            }
        }
    }

    void CodeEmitter::EmitShaderVariantOptionVariableDeclaration(const IdentifierUID& symbolUid, const Options& options) const
    {
        assert(m_ir->GetKind(symbolUid) == Kind::Variable);
        assert(IsTopLevelThroughTranslation(symbolUid));
        auto* varInfo = m_ir->GetSymbolSubAs<VarInfo>(symbolUid.GetName());

        if (options.m_useSpecializationConstantsForOptions && varInfo->m_specializationId >= 0)
        {
            m_out << GetPlatformEmitter().GetSpecializationConstant(*this, symbolUid, options);
        }
        else
        {

            EmitGetShaderKeyFunctionDeclaration(symbolUid, varInfo->GetTypeRefInfo());
            m_out << ";\n\n";

            m_out << "#if defined(" + JoinAllNestedNamesWithUnderscore(symbolUid.m_name) + "_OPTION_DEF)\n";
            EmitVariableDeclaration(*varInfo, symbolUid, options, VarDeclHasFlag(VarDeclHas::OptionDefine));
            m_out << "_OPTION_DEF ;\n#else\n";
            EmitVariableDeclaration(*varInfo, symbolUid, options, VarDeclHasFlag(VarDeclHas::OptionDefine) | VarDeclHas::Initializer);
            m_out << ";\n#endif\n";
        }
    }

    void CodeEmitter::EmitShaderVariantOptionGetters(const Options& options) const
    {
        vector<pair<IdentifierUID, VarInfo*>> symbols;

        // browse all variables
        for (const auto& [uid, varInfo] : m_ir->m_symbols.GetOrderedSymbolsOfSubType_2<VarInfo>())
        {
            // For now only emit top level options
            if (!IsTopLevelThroughTranslation(uid) || !varInfo->CheckHasStorageFlag(StorageFlag::Option))
            {
                continue;
            }

            symbols.emplace_back(uid, varInfo);
        }

        if (!symbols.empty())
        {
            m_out << "// Generated code: ShaderVariantOptions fallback value getters:\n";

            auto shaderOptions = GetVariantList(options, true);
            for (uint32_t shaderOptionIndex = 0; shaderOptionIndex < symbols.size(); ++shaderOptionIndex)
            {
                const auto& [uid, varInfo] = symbols[shaderOptionIndex];
                if (options.m_useSpecializationConstantsForOptions && varInfo->m_specializationId >= 0)
                {
                    continue;
                }

                const auto keySizeInBits = shaderOptions["ShaderOptions"][shaderOptionIndex]["keySize"].asUInt();
                const auto keyOffsetBits = shaderOptions["ShaderOptions"][shaderOptionIndex]["keyOffset"].asUInt();
                const auto defaultValue = shaderOptions["ShaderOptions"][shaderOptionIndex]["defaultValue"].asString();

                EmitGetShaderKeyFunction(m_shaderVariantFallbackUid, uid, keySizeInBits, keyOffsetBits, defaultValue, varInfo->GetTypeRefInfo());
            }
        }
    }

    void CodeEmitter::EmitGetFunctionsForRootConstants(const ClassInfo& rootConstInfo, string_view bufferName) const
    {
        for (const auto& memberVar : rootConstInfo.GetMemberFields())
        {
            const auto* varInfo = m_ir->GetSymbolSubAs<VarInfo>(memberVar.m_name);

            if (!varInfo->m_isPublic)
            {
                continue;
            }

            // Construct function return type and function name
            m_out << GetTranslatedName(varInfo->GetTypeRefInfo(), UsageContext::ReferenceSite) << " ";
            m_out << GetRootConstFunctionName(memberVar);

            m_out << "\n{\n";
            // The get function of root constants should return members of root ConstantBuffer object
            // Ex: return rootCB.ExampleSRG_myRootCstfloat4;
            m_out << "    return " << bufferName << "." << memberVar.GetNameLeaf() << ";\n";
            m_out << "}\n\n";
        }
    }

    void CodeEmitter::EmitRootConstants(const RootSigDesc& rootSig, const Options& options) const
    {
        if (const auto* rootConstInfo = m_ir->GetSymbolSubAs<ClassInfo>(m_ir->m_rootConstantStructUID.GetName()))
        {
            if (!rootConstInfo->GetMemberFields().empty())
            {
                m_out << GetPlatformEmitter().GetRootConstantsView(*this, rootSig, options, BindingPair::Set::Merged);

                // Emit the get function definitions which returns root const member
                EmitGetFunctionsForRootConstants(*rootConstInfo,
                                                 GetTranslatedName(RootConstantsViewName, UsageContext::ReferenceSite));
            }
        }
    }

    void CodeEmitter::EmitPreprocessorLineDirective(size_t azslLineNumber)
    {
        if (azslLineNumber == 0)
            return;  // protect for this invalid case. seems to happen for "virtual" symbols (like OverloadSet)

        size_t supposedVirtualLine = m_lineFinder->GetVirtualLineNumber(azslLineNumber);
        size_t curHlslLine = m_out.GetLineCount() + 1;  // "lines" is a space that is 1-based indexed.
        auto lastEmitted = Infimum(m_alreadyEmittedPreprocessorLineDirectives, curHlslLine);
        if (lastEmitted != m_alreadyEmittedPreprocessorLineDirectives.cend())
        {
            // verify if we can skip the line emission if current stream line feed is still in sync with expectations
            //  image:
            //            in sync                  out of sync
            //  1 | #line 1                1 | #line 1
            //  2 | code                   2 | code
            //  3 | newSymbol              3 | code
            //                             4 | #line 2
            //                             5 | newSymbol

            size_t curHlslPhysicalDistance = curHlslLine - lastEmitted->first;
            size_t lastVirtualSet = lastEmitted->second;
            size_t nonAdjustedCurrentLandingLine = lastVirtualSet + curHlslPhysicalDistance - 1;  // -1 because line directives specify the NEXT line
            if (nonAdjustedCurrentLandingLine == supposedVirtualLine)
                return; // no need to emit. we can skip
        }
        // get the original file as absolute path:
        const string& originalFileName = StdFs::absolute( m_lineFinder->GetVirtualFileName(azslLineNumber) ).lexically_normal().generic_string();
        // emit the line:
        m_out << "#line " << supposedVirtualLine << " \"" << originalFileName << "\"\n";
        // remember it:
        m_alreadyEmittedPreprocessorLineDirectives[curHlslLine] = supposedVirtualLine;
    }

    void CodeEmitter::EmitPreprocessorLineDirective(QualifiedNameView symbolName)
    {
        IdAndKind* idAndkindInfo = m_ir->GetIdAndKindInfo(symbolName);
        KindInfo& info = idAndkindInfo->second;
        const auto origSourceLine = info.VisitSub(GetOrigSourceLine_Visitor{});
        EmitPreprocessorLineDirective(origSourceLine);
    }

    string CodeEmitter::EmitInheritanceList(const ClassInfo& clInfo)
    {
        string hlsl = clInfo.HasAnyBases() ? " : " : "";
        vector<string> mutatedBaseNames;
        TransformCopy(clInfo.GetBases(), mutatedBaseNames,
                      [&](const IdentifierUID& uid)
                      {
                          return GetTranslatedName(uid, UsageContext::ReferenceSite);
                      });
        hlsl += Join(mutatedBaseNames, ", ");
        return hlsl;
    }

    void CodeEmitter::EmitStruct(const ClassInfo& classInfo, string_view structuredSymName, const Options& options)
    {
        auto HlslStructuredDelcTagFromKind = [](Kind k)
        {
            switch (k)
            {
            case Kind::Struct: return "struct";
            case Kind::Class: return "class";
            case Kind::Interface: return "interface";
            default: return " ";
            }
        };

        const bool hasName = (structuredSymName.length() > 0);
        const auto tabs = "    ";
        if (hasName)
        {
            EmitAllAttachedAttributes(IdentifierUID { QualifiedNameView{structuredSymName} });
            m_out << HlslStructuredDelcTagFromKind(classInfo.m_kind) << " "
                  << GetTranslatedName(QualifiedNameView{structuredSymName}, UsageContext::DeclarationSite)
                  << EmitInheritanceList(classInfo)
                  << "\n{\n"; // conclusion of "class X : ::Stuff {"
        }

        for (const IdentifierUID& memberUid : classInfo.GetOrderedMembers())
        {
            if (structuredSymName.empty() || m_translations.GetLandingScope(memberUid.GetName()) == structuredSymName)
            {
                auto& [uid, info] = *m_ir->GetIdAndKindInfo(memberUid.GetName());
                if (info.IsKindOneOf(Kind::Class, Kind::Struct, Kind::Interface))
                {
                    // recurse
                    EmitStruct(info.GetSubRefAs<ClassInfo>(), uid.GetName(), options);
                }
                else if (info.IsKindOneOf(Kind::TypeAlias))
                {
                    EmitTypeAlias(uid, info.GetSubRefAs<TypeAliasInfo>(), options);
                }
                else if (info.IsKindOneOf(Kind::Enum))
                {
                    EmitEnum(uid, info.GetSubRefAs<ClassInfo>(), options);
                }
                else if (info.IsKindOneOf(Kind::Variable))
                {
                    const auto* varInfo = m_ir->GetSymbolSubAs<VarInfo>(uid.m_name);
                    assert(varInfo);

                    m_out << tabs;
                    EmitVariableDeclaration(*varInfo, uid, options, VarDeclHasFlag(VarDeclHas::HlslSemantics));
                    m_out << ";\n";
                }
                else if (info.IsKindOneOf(Kind::Function))
                {
                    const auto* func = m_ir->GetSymbolSubAs<FunctionInfo>(uid.m_name);
                    assert(func);

                    m_out << tabs;
                    EmitFunctionAs form = func->HasUniqueDeclarationThroughDefinition() ? EmitFunctionAs::Definition : EmitFunctionAs::Declaration;
                    EmitFunction(*func, uid, form, options);
                }
                else
                {
                    assert(false);// "Unhandled KindInfo case in struct emission must be handled properly!"
                }
            }
        }

        if (hasName)
        {
            m_out << "};\n\n";
        }
    }

    void CodeEmitter::EmitAllAttachedAttributes(const IdentifierUID& uid, Except omissionList/*= {}*/) const
    {
        if (auto attrList = m_ir->m_symbols.GetAttributeList(uid))
        {
            for_each(attrList->begin(), attrList->end(),
                     [this, omissionList](auto&& attrInfo)
                     {
                         if (!IsIn(attrInfo.m_attribute, omissionList))
                         {
                            this->EmitAttribute(attrInfo);
                         }
                     });
        }
    }

    void CodeEmitter::EmitGetterFunctionDeclarationsForRootConstants(const IdentifierUID& uid) const
    {
        if (const auto* rootConstInfo = m_ir->GetSymbolSubAs<ClassInfo>(uid.GetName()))
        {
            for (const auto& memberVar : rootConstInfo->GetMemberFields())
            {
                const auto* varInfo = m_ir->GetSymbolSubAs<VarInfo>(memberVar.m_name);

                if (!varInfo->m_isPublic)
                {
                    continue;
                }

                m_out << GetTranslatedName(varInfo->GetTypeRefInfo(), UsageContext::ReferenceSite) << " ";
                m_out << GetRootConstFunctionName(memberVar);
                m_out << ";\n\n";

                // Emit a global variable to call the above declared function ex: static const float _gRootConstA = GetRootConst();
                // The global variable will be the reference variable to access the root constant
                m_out << "static const ";
                m_out << GetTranslatedName(varInfo->GetTypeRefInfo(), UsageContext::ReferenceSite) << " ";
                m_out << GetGlobalRootConstantVarName(memberVar.m_name) + " = ";
                m_out << GetRootConstFunctionName(memberVar);
                m_out << ";\n\n";
            }
        }
    }

    // special adapter version for AttributeInfo argument
    static string Undecorate(string_view decoration, const AttributeInfo::Argument& arg)
    {
        std::stringstream ss;
        MakeOStreamStreamable soss(ss);
        (Streamable&)soss << arg;
        return string{AZ::Undecorate(decoration, ss.str())};
    }

    void CodeEmitter::EmitAttribute(const AttributeInfo& attrInfo) const
    {
        return EmitAttribute(attrInfo, m_out);
    }

    void CodeEmitter::EmitAttribute(const AttributeInfo& attrInfo, Streamable& outstream)
    {
        if (attrInfo.m_attribute == "verbatim")
        {
            outstream << (attrInfo.m_argList.begin() == attrInfo.m_argList.end() ? "" : Unescape(Undecorate("\"", *attrInfo.m_argList.begin())));
            auto outVer = [&](const AttributeInfo::Argument& arg) { outstream << " " << Unescape(Undecorate("\"", arg)); };
            std::for_each(std::next(attrInfo.m_argList.begin()), attrInfo.m_argList.end(), outVer);
            outstream << '\n';
        }

        else if (attrInfo.m_attribute == "output_format")
        {
            if (!attrInfo.m_argList.empty())
            {
                if (holds_alternative<string>(attrInfo.m_argList[0]))
                {
                    string poFormat{ Trim(get<string>(attrInfo.m_argList[0]), "\"") };
                    outstream << "#pragma OutputFormatHint(default " << poFormat << ")\n";
                }
                else if (attrInfo.m_argList.size() > 1 &&
                         holds_alternative<ConstNumericVal>(attrInfo.m_argList[0]) &&
                         holds_alternative<string>(attrInfo.m_argList[1]))
                {
                    const auto rtIndex = ExtractValueAsInt64(get<ConstNumericVal>(attrInfo.m_argList[0]), std::numeric_limits<int64_t>::min());
                    if (rtIndex >= 0 && rtIndex <= 7)
                    {
                        string poFormat{ Trim(get<string>(attrInfo.m_argList[1]), "\"") };
                        outstream << "#pragma OutputFormatHint(target " << rtIndex << " " << poFormat << ")\n";
                    }
                }
            }
        }

        else if (attrInfo.m_attribute == "partial")
        {
            // Reserved for ShaderResourceGroup use. Do not re-emit
            outstream << "// original attribute: [[" << attrInfo << "]]\n ";
        }

        else if (attrInfo.m_attribute == "range")
        {
            // Reserved for integer type option variables. Do not re-emit
            outstream << "// original attribute: [[" << attrInfo << "]]\n ";
        }
        else if (attrInfo.m_attribute == "no_specialization")
        {
            // Reserved for avoiding specialization of a shader option. Do not re-emit
            outstream << "// original attribute: [[" << attrInfo << "]]\n ";
        }

        else
        {
            // We don't block any attributes we don't understand - pass them through
            outstream << ((attrInfo.m_category == AttributeCategory::Single) ? "[" : "[[")
                      << attrInfo
                      << ((attrInfo.m_category == AttributeCategory::Single) ? "]" : "]]")
                      << "\n";
        }
    }

    void CodeEmitter::EmitTypeAlias(const IdentifierUID& uid, const TypeAliasInfo& aliasInfo, const Options& options) const
    {
        m_out << "typedef " << GetTranslatedName(aliasInfo.m_canonicalType, UsageContext::ReferenceSite, options)
              << " " << GetTranslatedName(uid, UsageContext::DeclarationSite) << ";\n";
    }

    void CodeEmitter::EmitEnum(const IdentifierUID& uid, const ClassInfo& classInfo, const Options& options)
    {
        const auto& enumInfo = get<EnumerationInfo>(classInfo.m_subInfo);

        EmitAllAttachedAttributes(uid);

        m_out << "enum " << (enumInfo.m_isScoped ? "class " : "") << GetTranslatedName(uid.m_name, UsageContext::DeclarationSite) << "\n";
        m_out << "{\n";

        for (const auto& memberUid : classInfo.GetMemberFields())
        {
            // The variable can potentially have an initialization declaration as well
            const auto* varInfoPtr = m_ir->GetSymbolSubAs<VarInfo>(memberUid.m_name);
            EmitVariableDeclaration(*varInfoPtr,
                                    memberUid,
                                    options,
                                    VarDeclHasFlag(VarDeclHas::Initializer) | VarDeclHas::NoType | VarDeclHas::NoModifiers);
            m_out << ",\n";
        }
        m_out << "};\n\n";
    }

    bool CodeEmitter::AlreadyEmittedFunctionDeclaration(const IdentifierUID& uid) const
    {
        return m_alreadyEmittedFunctionDeclarations.find(uid) != m_alreadyEmittedFunctionDeclarations.end();
    }

    bool CodeEmitter::AlreadyEmittedFunctionDefinition(const IdentifierUID& uid) const
    {
        return m_alreadyEmittedFunctionDefinitions.find(uid) != m_alreadyEmittedFunctionDefinitions.end();
    }

    void CodeEmitter::EmitFunction(const FunctionInfo& funcSub, const IdentifierUID& uid, EmitFunctionAs entityConfiguration, const Options& options)
    {
        // reproduce the signature `[attr] [modifiers] rettype [classnameFQN::] Name(params) [semantics]`

        bool emitAsDeclaration = entityConfiguration == EmitFunctionAs::Declaration;
        bool emitAsDefinition  = entityConfiguration == EmitFunctionAs::Definition;

        bool riskDoubleEmission = emitAsDeclaration && AlreadyEmittedFunctionDeclaration(uid)
                               || emitAsDefinition && AlreadyEmittedFunctionDefinition(uid);
        bool undefinedFunction = funcSub.IsUndefinedFunction();
        if (riskDoubleEmission
            || undefinedFunction && emitAsDefinition)
        {
            return;
        }

        AstFuncSig* node = funcSub.m_defNode ? funcSub.m_defNode : funcSub.m_declNode;

        EmitAllAttachedAttributes(uid);

        // emit some modifiers only in case of first declaration/definition
        // because:    class C{ static vd A(); };
        //             static vd C::A(){}    // ill-formed HLSL. we can't repeat static here.
        Modifiers forbidden;
        bool firstDecl = !AlreadyEmittedFunctionDeclaration(uid);
        if (!firstDecl)
        {
            forbidden = Modifiers{StorageFlag::Static} | StorageFlag::Inline | StorageFlag::Extern;
        }
        // emit return type:
        m_out << GetTranslatedName(funcSub.m_returnType, UsageContext::ReferenceSite, options, forbidden) << " ";
        // emit Name
        if (entityConfiguration == EmitFunctionAs::Definition && funcSub.HasDeportedDefinition())
        {
            // emit fully qualified function name (with classname prefix).
            // surrounded by round braces because otherwise clang's greedy parsing will wreak havoc.
            // indeed `RetType ::Class::Method() {}` will be parsed as `RetType::Class::Method(){}` one id-expr.
            // since we can't use trailing return type in HLSL, the fix is to use round braces.
            //  solution from https://stackoverflow.com/a/3185232/893406
            m_out << "(" << GetTranslatedName(uid, UsageContext::ReferenceSite) << ")";
        }
        else
        {
            // emit leaf function name (in AZSL all declaration sites are Identifier and not idExpressions)
            m_out << GetTranslatedName(uid, UsageContext::DeclarationSite);
        }
        // emit parameters:
        m_out << "(";
        bool inhibitInitializers = AlreadyEmittedFunctionDeclaration(uid) && !emitAsDeclaration;
        EmitParameters(funcSub.GetParameters(emitAsDeclaration).begin(), funcSub.GetParameters(emitAsDeclaration).end(), options, !inhibitInitializers);
        m_out << ")";

        if (const auto hlslSemantic = node->hlslSemantic())
        {
            m_out << " " << hlslSemantic->getText();
        }

        if (entityConfiguration == EmitFunctionAs::Declaration)
        {
            m_out << ";\n";
            m_alreadyEmittedFunctionDeclarations.insert(uid);
        }
        else
        {
            auto scopeInterval = m_ir->m_scope.m_scopeIntervals[uid];
            auto astNode = m_ir->m_tokenMap.GetNode(scopeInterval.a);
            auto funcDefNode = ExtractSpecificParent<azslParser::HlslFunctionDefinitionContext>(astNode);
            auto blockInterval = funcDefNode->block()->getSourceInterval();
            m_out << "\n";
            EmitTranspiledTokens(blockInterval);
            m_out << "\n\n";
            m_alreadyEmittedFunctionDefinitions.insert(uid);
        }
    }

    const CodeMutation* CodeEmitter::GetCodeMutation(size_t tokenIndex) const
    {
        for (auto* codeMutator : m_codeMutators)
        {
            if (const CodeMutation* codeMutation = codeMutator->GetMutation(tokenIndex))
            {
                return codeMutation;
            }
        }
        return nullptr;
    }


    void CodeEmitter::EmitVariableDeclaration(const VarInfo& varInfo, const IdentifierUID& uid, const Options& options, VarDeclHasFlag declOptions) const
    {
        // from MSDN: https://docs.microsoft.com/en-us/windows/desktop/direct3dhlsl/dx-graphics-hlsl-variable-syntax
        // [Storage_Class] [Type_Modifier] Type Name[Index] [: Semantic] [: Packoffset] [: Register]; [Annotations] [= Initial_Value]
        // example of valid HLSL statement:
        //    static const int dim = 2;
        //    extern uniform bool stuff[dim][1] : Color0 : register(b0) <int blabla=27; string blacksheep="Hello There";> = {{false, true}};

        const CodeMutation* codeMutation = nullptr;
        if (varInfo.m_declNode)
        {
            const auto tokenIndex = varInfo.m_declNode->start->getTokenIndex();
            codeMutation = GetCodeMutation(tokenIndex);
        }   

        if (codeMutation && codeMutation->m_prepend)
        {
            m_out << codeMutation->m_prepend.value();
        }

        if (codeMutation && codeMutation->m_replace)
        {
            m_out << codeMutation->m_replace.value();
        }
        else
        {
            EmitAllAttachedAttributes(uid);

            // parameter in/out modifiers
            if (declOptions & VarDeclHas::InOutModifiers)
            {
                m_out << GetInputModifier(varInfo.m_typeInfoExt.m_qualifiers) << " ";
            }
            // type
            if (!(declOptions & VarDeclHas::NoType) || (declOptions & VarDeclHas::OptionDefine))
            {
                auto bannedModifiers = (declOptions & VarDeclHas::NoModifiers) ? ~Modifiers{(StorageFlag::EnumType)0} : Modifiers{};
                m_out << GetTranslatedName(varInfo.m_typeInfoExt, UsageContext::ReferenceSite, options, bannedModifiers) + " ";
            }
            // var name
            m_out << GetTranslatedName(uid.m_name, UsageContext::DeclarationSite);
            // array dimensions
            if (varInfo.m_declNode && !varInfo.m_declNode->ArrayRankSpecifiers.empty())
            {
                for (auto* rankCtx : varInfo.m_declNode->ArrayRankSpecifiers)
                {
                    // the brackets are included by the rule arrayRankSpecifier
                    EmitTranspiledTokens(rankCtx->getSourceInterval());
                }
            }
            else if (!varInfo.GetArrayDimensions().Empty())
            {
                m_out << varInfo.GetArrayDimensions().ToString();
            }
            // semantics
            if (varInfo.m_declNode && (declOptions & VarDeclHas::HlslSemantics))
            {
                if (auto* semanticOption = varInfo.m_declNode->SemanticOpt)
                {
                    m_out << " " + semanticOption->getText();
                }
            }

            // init clause
            if (declOptions & VarDeclHas::OptionDefine)
            {
                if (declOptions & VarDeclHas::Initializer)
                {
                    m_out << " = " << GetShaderKeyFunctionName(uid);
                }
                else
                {
                    m_out << " = " << JoinAllNestedNamesWithUnderscore(uid.m_name);
                }
            }
            else if (declOptions & VarDeclHas::Initializer)
            {
                auto* initClause = varInfo.m_declNode ? varInfo.m_declNode->variableInitializer() : nullptr;
                if (initClause)
                {
                    m_out << " ";
                    EmitTranspiledTokens(initClause->getSourceInterval());
                }
                else  // fallback on a potentially folded value, that has chances to work for constants like enumerators
                {
                    int64_t constVal;
                    if (TryGetConstExprValueAsInt64(varInfo.m_constVal, constVal))
                    {
                        m_out << " = " << constVal;
                    }
                }
            }
        }
        if (codeMutation && codeMutation->m_append)
        {
            m_out << codeMutation->m_append.value();
        }
    }

    void CodeEmitter::EmitSRGCBUnified(const SRGInfo& srgInfo, IdentifierUID srgId, const Options& options, const RootSigDesc& rootSig)
    {
        auto bindSet = BindingPair::Set::Merged;
        // Use the uId of the SRG instead of a CBV, because we create a dummy placeholder CBV to hold the rest of the declarations:
        if (!options.m_emitConstantBufferBody)
        {
            if (srgInfo.m_implicitStruct.GetMemberFields().size() > 0)
            {
                const auto& bindInfo = rootSig.Get(srgId);

                const QualifiedName implicitStruct = MakeSrgConstantsStructName(srgId);
                const QualifiedName implicitCB = MakeSrgConstantsCBName(srgId);
                EmitStruct(srgInfo.m_implicitStruct, implicitStruct, options);

                const auto spaceX = ", space" + std::to_string(bindInfo.m_registerBinding.m_pair[bindSet].m_logicalSpace);
                const auto implicitStructForEmission = GetTranslatedName(implicitStruct, UsageContext::ReferenceSite);
                const auto implicitCBForEmission = GetTranslatedName(implicitCB, UsageContext::DeclarationSite);
                m_out << "ConstantBuffer<" << implicitStructForEmission << "> " << implicitCBForEmission << " : register(b" << bindInfo.m_registerBinding.m_pair[bindSet].m_registerIndex << spaceX << ");\n\n";
            }

            for (auto cId : srgInfo.m_CBs)
            {
                if (!m_ir->GetSymbolSubAs<VarInfo>(cId.GetName())->StorageFlagIsLocalLinkage(true))
                {
                    EmitSRGCB(cId, options, rootSig);
                }
            }

            return;
        }

        if (srgInfo.m_CBs.empty() && srgInfo.m_implicitStruct.GetMemberFields().empty())
        {
            return;
        }

        const auto& bindInfo = rootSig.Get(srgId);
        m_out << "ConstantBuffer " << srgInfo.m_declNode->Name->getText() << "_CBContainer : register(b" << bindInfo.m_registerBinding.m_pair[bindSet].m_registerIndex << ")\n{\n";

        for (const auto& cId : srgInfo.m_CBs)
        {
            if (!m_ir->GetSymbolSubAs<VarInfo>(cId.GetName())->StorageFlagIsLocalLinkage(true))
            {
                const auto& uqName = cId.GetNameLeaf();
                m_out << "    CBVArrayView " << uqName << ";\n";
            }
        }

        if (!srgInfo.m_implicitStruct.GetMemberFields().empty())
        {
            m_out << "\n";
            EmitStruct(srgInfo.m_implicitStruct, "", options);
        }

        m_out << "\n};\n";

        for (const auto& cId : srgInfo.m_CBs)
        {
            if (!m_ir->GetSymbolSubAs<VarInfo>(cId.GetName())->StorageFlagIsLocalLinkage(true))
            {
                const auto* memberInfo = m_ir->GetSymbolSubAs<VarInfo>(cId.m_name);
                const auto& cbName = ReplaceSeparators(cId.m_name, Underscore);
                const auto& uqName = cId.GetNameLeaf();

                if (memberInfo->IsConstantBuffer())
                {
                    const auto& templatedCB = "RegularBuffer<" + GetTranslatedName(memberInfo->GetGenericParameterTypeId(), UsageContext::ReferenceSite) + "> ";

                    m_out << "static const " << templatedCB << cbName << " = " << templatedCB << "(" << uqName << ");\n";
                }
            }
        }

        m_out << "\n\n";
    }

    void CodeEmitter::EmitSRGCB(const IdentifierUID& cId, [[maybe_unused]] const Options& options, const RootSigDesc& rootSig) const
    {
        EmitAllAttachedAttributes(cId);
        auto bindSet = BindingPair::Set::Merged;

        assert(!options.m_emitConstantBufferBody);

        const auto& bindInfo = rootSig.Get(cId);
        const auto* varInfo = m_ir->GetSymbolSubAs<VarInfo>(cId.m_name);
        auto cbName = ReplaceSeparators(cId.m_name, Underscore);

        assert(varInfo->IsConstantBuffer());
        // note: instead of redoing this work ad-hoc, EmitText could be used directly on the ext type.
        const auto genericType = "<" + GetTranslatedName(varInfo->m_typeInfoExt.m_genericParameter, UsageContext::ReferenceSite) + ">";

        const string spaceX = ", space" + std::to_string(bindInfo.m_registerBinding.m_pair[bindSet].m_logicalSpace);
        m_out << "ConstantBuffer " << genericType << " " << cbName;
        if (bindInfo.m_isUnboundedArray)
        {
            m_out << "[]";
        }
        else if (bindInfo.m_registerRange > 1)
        {
            m_out << "[" << bindInfo.m_registerRange << "]";
        }
        m_out << " : register(b" << bindInfo.m_registerBinding.m_pair[bindSet].m_registerIndex << spaceX << ");\n\n";
    }

    void CodeEmitter::EmitSRGSampler(const IdentifierUID& sId, [[maybe_unused]] const Options& options, const RootSigDesc& rootSig) const
    {
        EmitAllAttachedAttributes(sId);
        auto bindSet = BindingPair::Set::Merged;

        const auto& bindInfo = rootSig.Get(sId);
        const auto* varInfo = m_ir->GetSymbolSubAs<VarInfo>(sId.m_name);

        const string spaceX = ", space" + std::to_string(bindInfo.m_registerBinding.m_pair[bindSet].m_logicalSpace);
        m_out << (varInfo->m_samplerState->m_isComparison ? "SamplerComparisonState " : "SamplerState ")
              << ReplaceSeparators(sId.m_name, Underscore);
        if (bindInfo.m_isUnboundedArray)
        {
            m_out << "[]";
        }
        else if (bindInfo.m_registerRange > 1)
        {
            m_out << "[" << bindInfo.m_registerRange << "]";
        }
        m_out << " : register(s" << bindInfo.m_registerBinding.m_pair[bindSet].m_registerIndex << spaceX << ")";
        m_out << ";\n\n";
    }

    //! For scope-migration-aware name emission of symbol names
    string CodeEmitter::GetTranslatedName(QualifiedNameView mangledName, UsageContext context, ssize_t tokenId /*= NotOverToken*/) const
    {
        return m_translations.GetTranslatedName(mangledName, context, tokenId);
    }

    string CodeEmitter::GetTranslatedName(const IdentifierUID& uid, UsageContext context, ssize_t tokenId /*= NotOverToken*/) const
    {
        return GetTranslatedName(uid.m_name, context, tokenId);
    }

    string CodeEmitter::GetTranslatedName(const TypeRefInfo& typeRef, UsageContext context, ssize_t tokenId /*= NotOverToken*/) const
    {
        return GetTranslatedName(typeRef.m_typeId, context, tokenId);
    }

    string CodeEmitter::GetTranslatedName(const ExtendedTypeInfo& extTypeInfo, UsageContext context, const Options& options, Modifiers forbidden /*= {}*/, ssize_t tokenId /*= NotOverToken*/) const
    {
        return GetExtendedTypeInfo(extTypeInfo, options, forbidden,
                                   [this, context, tokenId](const TypeRefInfo& tri){ return GetTranslatedName(tri, context, tokenId); });
    }

    void CodeEmitter::EmitSRGDataView(const IdentifierUID& tId, const Options& options, const RootSigDesc& rootSig) const
    {
        EmitAllAttachedAttributes(tId, Except{ { "input_attachment_index" } });
        auto   bindSet = BindingPair::Set::Merged;
        auto&  bindInfo = rootSig.Get(tId);
        auto*  varInfo = m_ir->GetSymbolSubAs<VarInfo>(tId.m_name);
        string varType = GetTranslatedName(varInfo->m_typeInfoExt, UsageContext::DeclarationSite, options);
        auto   registerTypeLetter = ToLower(BindingType::ToStr(RootParamTypeToBindingType(bindInfo.m_type)));
        optional<string> stringifiedLogicalSpace = std::to_string(bindInfo.m_registerBinding.m_pair[bindSet].m_logicalSpace);

        // depending on platforms we may have supplementary attributes or/and type modifier.
        auto [prefix, suffix] = GetPlatformEmitter().GetDataViewHeaderFooter(*this, tId, bindInfo.m_registerBinding.m_pair[bindSet].m_registerIndex, registerTypeLetter, stringifiedLogicalSpace, options);
        m_out << prefix;
        // declaration of the view variable on the global scope.
        // type unmangled_path_to_symbol [optional_array_dimension] : optional_register_binding_as_suffix
        m_out << varType << " " << ReplaceSeparators(tId.m_name, Underscore);

        if (bindInfo.m_isUnboundedArray)
        {
            m_out << "[]";
        }
        else  if (bindInfo.m_registerRange > 1)
        {
            m_out << "[" << bindInfo.m_registerRange << "]";
        }
        m_out << suffix;

        auto interval = m_ir->m_scope.m_scopeIntervals[tId];
        EmitTranspiledTokens(interval);
        m_out << ";\n\n";
    }

    void CodeEmitter::EmitGetShaderKeyFunctionDeclaration(const IdentifierUID& getterUid, const TypeRefInfo& returnType) const
    {
        m_out << GetTranslatedName(returnType, UsageContext::ReferenceSite) << " ";
        m_out << GetShaderKeyFunctionName(getterUid);
    }

    void CodeEmitter::EmitGetShaderKeyFunction(const IdentifierUID& shaderKeyUid, const IdentifierUID& getterUid, uint32_t keySizeInBits, uint32_t keyOffsetBits, string_view defaultValue, const TypeRefInfo& returnType) const
    {
        // Because we use uint on the shader source side no shader option can cross the 32-bit boundary
        // This is already ensured by the emission side, in Json::Value CodeEmitter::GetVariantList(...)
        uint32_t arraySlot = keyOffsetBits / AZ::ShaderCompiler::kShaderVariantKeyRegisterSize;
        keyOffsetBits -= arraySlot * AZ::ShaderCompiler::kShaderVariantKeyRegisterSize;
        uint32_t swizzle = keyOffsetBits / AZ::ShaderCompiler::kShaderVariantKeyElementSize;
        keyOffsetBits -= swizzle * AZ::ShaderCompiler::kShaderVariantKeyElementSize;

        // Intentional unnamed scope for error-checking
        {
            auto& varInfo = *m_ir->GetSymbolSubAs<VarInfo>(shaderKeyUid.m_name);
            auto dims = varInfo.m_typeInfoExt.GetDimensions();
            assert(dims.m_dimensions.size() == 1); // This is generated variable, it must have exactly 1 array dimension
            if (dims.m_dimensions[0] < 0 || arraySlot >= static_cast<uint32_t>(dims.m_dimensions[0]))
            {
                const string errorMessage = ConcatString("The option {", UnmangleTrimedName(getterUid.m_name), "} exceeds the number of bits (",
                    AZ::ShaderCompiler::kShaderVariantKeyRegisterSize * dims.m_dimensions[0], ") allowed by the ShaderVariantFallback.\n",
                    "Either increase the limit or remove some options!");
                throw AzslcEmitterException(EMITTER_OPTION_EXCEEDING_BITS_COUNT, errorMessage);
            }
        }

        // The most significant bits are put in the first element, then the next, etc.
        // The bit order within an element is swapped so the most significant option
        // is at 0x00000001 and the least significant (32th bit) is at 0x80000000.
        // This is tailored to the runtime to reduce CB compile times.
        const char suffix[] = { 'x', 'y', 'z', 'w' };

        m_out << "\n";

        const auto mask = static_cast<uint32_t>(pow(2, keySizeInBits) - 1);

        EmitGetShaderKeyFunctionDeclaration(getterUid, returnType);

        m_out << "\n{\n";
        if (keySizeInBits > 0)  // Emit the option getter function body using CB access and bit decoding.
        {
            m_out << "    uint shaderKey = ((" << GetTranslatedName(shaderKeyUid, UsageContext::ReferenceSite)
                  << "[" << arraySlot << "]." << suffix[swizzle] << " >> " << keyOffsetBits << ") & " << mask << ")";
            // Also we need to reproduce the "range minimal" value as an addition post big-field extraction.
            // because the CB compact storage into bits of an uint4 does not incorporate the offset of the range
            // because they are "useless" (compressable) entropy bits that would take space in the key.
            if (returnType.m_typeId.m_name.find("int") != string::npos)  // if type int/uint/int16_t... then it's an integer range.
            {
                if (auto attrInfo = m_ir->m_symbols.GetAttribute(getterUid, "range"))
                {
                    // Presence of the correct arglist has already been verified in Backend::AppendOptionRange
                    m_out << " + " << ExtractValueAsInt64(get<ConstNumericVal>(attrInfo->m_argList[0]));
                }
            }
            else if (returnType.m_typeClass == TypeClass::Enum)
            {
                // In the case of an enumeration,
                // recovering the correct value will depend if there are jumps in the enumerators.
                // To discover that, we will use the presence of initializers.
                // one initializer on the first enumerator is a special case because it still allows
                // us to reconstruct the value from an integer cast and a +first enumerator.
                // In a case of disparate enumerators, we'll need to reconstruct the values
                // by emitting a switch case.
                // We don't want that switch case as a generic case, that could simplify azslc's source
                // but it risks slowing down the shader runtime. Sort of a pay-what-you-use principle.
                auto& enumClassInfo = *m_ir->GetSymbolSubAs<ClassInfo>(returnType.m_typeId.GetName());
                auto& enumerators = enumClassInfo.GetOrderedMembers();
                bool otherInitializers = std::any_of(enumerators.begin() + 1, enumerators.end(),
                                                     [&](const IdentifierUID& e)
                                                     {
                                                         auto& var = *m_ir->GetSymbolSubAs<VarInfo>(e.GetName());
                                                         return !!var.m_declNodeEnum->Value;
                                                     });
                if (otherInitializers)
                {
                    m_out << ";\n    switch (shaderKey)\n    {\n";
                    for (int i = 0; i < enumerators.size(); ++i)
                    {
                        m_out << "        case " << i << ": shaderKey = (" << GetTranslatedName(returnType, UsageContext::ReferenceSite) << ")"
                              << GetTranslatedName(enumerators[i], UsageContext::ReferenceSite) << "; break;\n";
                    }
                    m_out << "    }";
                }
                else
                {
                    // add the value of the first enumerator (by emitting its name)
                    m_out << " + (uint)" << GetTranslatedName(enumerators.front(), UsageContext::ReferenceSite);  // we can access front with no defense because keySize would be 0 if there are no enumerators.
                }
            }
            m_out << ";\n";
            m_out << "    return (" << GetTranslatedName(returnType, UsageContext::ReferenceSite) << ") shaderKey;\n";
        }
        else
        {
            // In the case of an empty enumeration type (no enumerators) or empty range, the keySize will be 0
            // and this if-branch will be taken. if the programmer specified no initializer clause,
            // this would produce unbuildable HLSL and error in generatored code. We really don't want that
            // as it would be hard to diagnose for users. As a reasonable behavior, we can emit val = 0.
            string typeAsStr = GetTranslatedName(returnType, UsageContext::ReferenceSite);
            // "<type> val = (cast expr to <type>) 0 or default;"
            m_out << "    " << typeAsStr << " val = (" << typeAsStr << ")" << (defaultValue.empty() ? "0" : defaultValue) << ";\n";
            m_out << "    return val;\n";
        }
        m_out << "}\n\n";
    }

    void CodeEmitter::EmitSRG(const SRGInfo& srgInfo, const IdentifierUID& srgId, const Options& options, const RootSigDesc& rootSig)
    {
        RootSigDesc::SrgDesc srgDesc;
        srgDesc.m_uid = srgId;

        m_out << "/* Generated code from ";
        // We don't emit the SRG attributes (only as a comment), but they can be accessed by the srgId if needed
        EmitAllAttachedAttributes(srgId);
        m_out << " ShaderResourceGroup " << srgInfo.m_declNode->Name->getText() << "*/\n";

        for (const auto& t : srgInfo.m_srViews)
        {
            // (*1) non-extern symbols are not visible resources, and are emitted by canonical EmitVariableDeclaration path
            if (!m_ir->GetSymbolSubAs<VarInfo>(t.GetName())->StorageFlagIsLocalLinkage(true))
            {
                EmitSRGDataView(t, options, rootSig);
            }
        }

        for (const auto& s : srgInfo.m_samplers)
        {
            // same as (*1)
            if (!m_ir->GetSymbolSubAs<VarInfo>(s.GetName())->StorageFlagIsLocalLinkage(true))
            {
                EmitSRGSampler(s, options, rootSig);
            }
        }

        EmitSRGCBUnified(srgInfo, srgId, options, rootSig);

        if (srgInfo.m_shaderVariantFallback)
        {
            m_shaderVariantFallbackUid = *srgInfo.m_shaderVariantFallback;
        }
    }

    void CodeEmitter::IfIsSrgMemberValidateIsDefined(antlr4::Token* token, TokenToAst::AstNode* nodeFromToken) const
    {
        // A common user mistake is to reference an undefined SRG field like
        // "MySrg::m_someVariable", where "m_someVariable" is undefined inside "MySrg",
        // or because it's a typo. Either way let's catch the scenario and provide a meaningful message.
        if (auto nestedNameSpecifierCtx = As<azslParser::NestedNameSpecifierContext*>(nodeFromToken))
        {
            // Get the parent rule, which should be a QualifiedIdContext.
            auto qualifiedIdCtx = As<azslParser::QualifiedIdContext*>(nestedNameSpecifierCtx->parent);
            if (!qualifiedIdCtx)
            {
                const string errorMessage = FormatString("Unexpected expression '%s'", token->getText().c_str());
                throw AzslcEmitterException(EMITTER_UNEXPECTED_EXPRESSION, token, errorMessage);
            }
            // We only care to call out the error if the token has the name of an SRG.
            QualifiedName qualifiedSymbolName(FormatString("/%s", token->getText().c_str()));
            if (const IdAndKind* idAndKind = m_ir->GetIdAndKindInfo(qualifiedSymbolName))
            {
                const auto& [uid, kindInfo] = *idAndKind;
                if (kindInfo.IsKindOneOf(Kind::ShaderResourceGroup))
                {
                    const string errorMessage = FormatString("Undefined ShaderResourceGroup member '%s'", qualifiedIdCtx->getText().c_str());
                    throw AzslcEmitterException(EMITTER_UNDEFINED_SRG_MEMBER, qualifiedIdCtx->getStart(), errorMessage);
                }
            }
        }
    }

    // override of the base method, to incorporate symbol and expression mutations
    void CodeEmitter::EmitTranspiledTokens(misc::Interval interval, Streamable& output) const
    {
        ssize_t ii = interval.a;
        while (ii <= interval.b)
        {
            auto* token = GetNextToken(ii /*inout*/);
            
            const auto tokenIndex = token->getTokenIndex();

            const CodeMutation* codeMutation = GetCodeMutation(tokenIndex);
            if (codeMutation && codeMutation->m_prepend)
            {
                output << codeMutation->m_prepend.value();
            }

            if (codeMutation && codeMutation->m_replace)
            {
                output << codeMutation->m_replace.value();
            }
            else
            {
                // Access the AST from the token. Note that any processing relying on this, amounts to a hacky shortcut.
                // (taken to make the economy of having to produce a specific emitters, for each AST-node/code-construct in HLSL)
                auto* astNode = m_ir->m_tokenMap.GetNode(tokenIndex);
                // watch for potential mutations in the middle of the original source.
                auto [originalSymbol, endToken] = m_translations.OverOriginalDefinitionOf(token->getTokenIndex());
                bool emitAsIs = originalSymbol.empty();
                if (!emitAsIs)
                {
                    auto* withinVarDecl = ExtractSpecificParent<azslParser::VariableDeclarationStatementContext>(astNode);
                    // if we are over a variable declaration, then it's a more evolved statement than just a struct/class declaration
                    // in such a case, the introduction of the type (declaration) is immediately used in a larger statement,
                    // so we need to re-emit it as a reference.
                    if (withinVarDecl && token == withinVarDecl->variableDeclaration()->type()->start)
                    {
                        output << GetTranslatedName(originalSymbol, UsageContext::ReferenceSite, ii) << " ";
                    }
                    // that and anything else: pure UDT declarations, or typeof/typealias may be jumped over completely
                    ii = endToken + 1;
                }
                else
                {
                    auto idExpr = m_translations.GetIdExpression(token);
                    if (!idExpr.IsEmpty())
                    {
                        auto getToken = [this](ssize_t& tokenId) -> string
                        {
                            assert(tokenId >= 0);
                            auto* token = m_tokens->get(static_cast<size_t>(tokenId));
                            return token->getChannel() == Token::DEFAULT_CHANNEL ? token->getText() : string{};
                        };
                        output << m_translations.TranslateIdExpression(idExpr, ii, getToken) << " ";
                        ii += idExpr.m_span.length() - 1;
                        emitAsIs = false;
                    }
                }
                if (emitAsIs)
                {
                    IfIsSrgMemberValidateIsDefined(token, astNode);
                    
                    // do minimal reformatting to have a pseudo-readable emitted code
                    auto str = token->getText();
                    bool lineFeed = str == ";" || str == "{";
                    output << str << (lineFeed ? '\n' : ' ');
                }
            }

            if (codeMutation && codeMutation->m_append)
            {
                output << codeMutation->m_append.value();
            }
        }
    }
}
