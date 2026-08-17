/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "AzslcSymbolTranslation.h"
#include "AzslcHomonymVisitor.h"

namespace AZ::ShaderCompiler
{
    void SymbolTranslation::SetAccessSymbolQueryFunctor(std::function<KindInfo*(QualifiedNameView)> accessSymbol)
    {
        m_accessSymbol = accessSymbol;
    }

    void SymbolTranslation::SetGetSeenatFunctor(std::function< vector<Seenat>& (QualifiedNameView) > getSeenats)
    {
        m_getSeenats = getSeenats;
    }

    void SymbolTranslation::RegisterLandingScope(const IdentifierUID& originalSymbol, QualifiedNameView landingScope)
    {
        using RE = RelationshipExtent;
        using std::make_pair;
        assert(GetParentName(originalSymbol.GetName()) != landingScope);
        auto findIterator = m_landingScope.find(originalSymbol.GetName());
        if (findIterator != m_landingScope.end())
        {   // if we are re-registering the same symbol, nothing to do. but check that the landingScope is the same as originally registered:
            assert(findIterator->second.m_landingScope == landingScope);
            return;
        }
        m_landingScope[originalSymbol.GetName()].m_landingScope = landingScope;
        // let's pre-cache the locations of all occurrences of this symbol in the program, for quick lookup later when we only have tokenID.
        HomonymVisitor hv(m_accessSymbol);
        hv(originalSymbol,
           [&, this](const Seenat& at, RE relation)
           {
               if (relation == RE::Self)
               {   // that's the definition point itself...
                   auto span = at.m_where.m_expressionSpan;
                   m_definitionCtxStartTokenOfMigratedSymbol[span.a] = make_pair(at.m_referredDefinition.GetName(), span.b);
               }
               else if (relation == RE::Reference)
               {
                   PreCacheTokenOfReference_(at);  // register tokenId of the seenat, to lookup maps that will give us the IDExpressionDesc& quickly later
               }
               else if (relation == RE::OverloadSet)
               {
                   // when we are over the definition of an overload, we don't need to register it, since it will have its own RegisterLandingScope call.
                   // however! there will be no such call from the "naked" overload-set, even though it may store references to call sites that hasn't been resolved.
                   // so we will do a shallow recursion here to get its references, to be sure to get them to mutate too.
                   if (!IsLeafDecoratedByArguments(at.m_referredDefinition.GetName()))
                   {
                       RegisterLandingScope(at.m_referredDefinition, landingScope);
                   }
               }
           },
           RelationshipExtentFlag{RE::Self} | RE::Reference | RE::OverloadSet);
    }

    void SymbolTranslation::AddCustomBehavior(QualifiedNameView originalSymbol, BehaviorEventFlag on, TranslationBehaviorDelegate action)
    {
        m_landingScope[originalSymbol].m_customBehavior = CustomBehavior{action, on};
    }

    void SymbolTranslation::MigrateDisambiguateAndCache(QualifiedNameView originalSymbol) const
    {
        // can't execute that action more than once
        assert(m_renames.find(originalSymbol) == m_renames.end());
        const auto& landingScope = m_landingScope.find(originalSymbol)->second;
        bool kindIsFunction = m_accessSymbol(originalSymbol)->IsKindOneOf(Kind::Function, Kind::OverloadSet);
        // the reason we treat functions differently is that some call sites are un-resolved, it's up to the target compiler to do the final overload resolution.
        // as long as we tolerate this loose semantics, we cannot uniquify function names, we need to preserve their "homonymous-ness".
        // however, there other kinds of symbols, like variables and types within function scopes, need to be unique to avoid collisions.
        auto flatteningStrategy = kindIsFunction ? CollapseArguments : PreserveArgumentsUnicity;
        // flatten will preserve the original qualification for unicity purposes. SRG::Inner::m_color becomes SRG_Inner_m_color
        string flattened = Flatten(originalSymbol.data(), flatteningStrategy);
        // check if the new symbol name is free in the desired scope
        QualifiedName collisionCandidate{JoinPath(landingScope.m_landingScope, flattened)};
        int counter = 1;
        auto mappedRenameIter = m_mappedRenames.find(collisionCandidate);
        while (m_accessSymbol(collisionCandidate) || mappedRenameIter != m_mappedRenames.end())
        {
            if (mappedRenameIter != m_mappedRenames.end() && kindIsFunction)
            {   // overloads HAVE to end up with the same name (if we disambiguate we're losing overloading).
                // verify that the original symbol of that collision, refers to the same overload-set:
                if (BelongsToOverloadSet_(mappedRenameIter->second, originalSymbol))
                {
                    break;
                }
            }
            // mutate it a bit until it's free.
            auto attempt = flattened + std::to_string(counter);
            collisionCandidate = QualifiedName{JoinPath(landingScope.m_landingScope, attempt)};
            ++counter;
            mappedRenameIter = m_mappedRenames.find(collisionCandidate);
        }
        m_renames[originalSymbol] = collisionCandidate;  // cache for rename lookups
        m_mappedRenames[collisionCandidate] = originalSymbol;  // cache for reverse lookups
    }

    QualifiedNameView SymbolTranslation::GetLandingScope(QualifiedNameView originalSymbol) const
    {
        auto it = m_landingScope.find(originalSymbol);
        return it == m_landingScope.end() ? QualifiedNameView{GetParentName(originalSymbol)} : QualifiedNameView{it->second.m_landingScope};
    }

    void SymbolTranslation::FindTranslation_(FindTranslation_Parameters& params) const
    {
        auto landingScopeIter = m_landingScope.find(params.m_originalPath);
        params.m_foundMutation = (landingScopeIter == m_landingScope.end()) ? nullptr : &landingScopeIter->second;
        const auto cacheIter = m_renames.find(params.m_originalPath);
        if (cacheIter == m_renames.end())  // no translation-cache
        {
            if (!params.m_foundMutation)
            {   // no migration registered
                // check parents.
                auto parent = GetParentName(params.m_originalPath);
                if (parent.empty() || parent == "/")
                {   // no parent, just copy the input.
                    params.m_result = params.m_originalPath;
                }
                else
                {   // go down the stack (leftward in the nestings) A/B/C -> A/B
                    FindTranslation_Parameters recursiveParams{parent};
                    FindTranslation_(recursiveParams);
                    // as we climb the callstack, reconstruct the result by restitching left and right parts G/B -> G/B/C
                    params.m_result = QualifiedName{JoinPath(recursiveParams.m_result, ExtractLeaf(params.m_originalPath))};
                }
            }
            else
            {   // in case of a registered migration request
                MigrateDisambiguateAndCache(params.m_originalPath);  // execute the evolved logic and cache it.
                params.m_result = m_renames.find(params.m_originalPath)->second;  // get from cache
            }
        }
        else
        {   // we have a cache, use that.
            params.m_result = cacheIter->second;
        }
    }

    string SymbolTranslation::GetTranslatedName(QualifiedNameView originalSymbol, UsageContext qualificationStrategy, ssize_t tokenId) const
    {
        FindTranslation_Parameters translationParams{originalSymbol};
        FindTranslation_(translationParams);
        const QualifiedName& renamed = translationParams.m_result;  // QualifiedName are mangled
        // DeclarationSite is for grammar contexts that usually don't accept more than identifiers. (no nested name specifiers)
        // the ReferenceSite strategy is to emit the fully qualified HLSL all the time.
        //   we could try to reduce verbosity by using FindLeastQualifiedName but that'll be for later.
        // Unmangling by convention will un-decorate function names to strip them to their core name. So be consistent for both strategy.
        auto translation = qualificationStrategy == UsageContext::DeclarationSite ? string{RemoveLastParenthesisGroup(ExtractLeaf(renamed))}
                                                                                  : UnMangle(string{renamed});
        // find a potential callback
        if (translationParams.m_foundMutation
            && translationParams.m_foundMutation->m_customBehavior
            && (translationParams.m_foundMutation->m_customBehavior->m_when & static_cast<BehaviorEvent::EnumType>(qualificationStrategy)))
        {
            auto callback = &(*translationParams.m_foundMutation->m_customBehavior);
            translation = callback->m_action(originalSymbol, qualificationStrategy, translation, tokenId);
        }
        return translation;
    }

    pair<QualifiedNameView, ssize_t> SymbolTranslation::OverOriginalDefinitionOf(ssize_t tokenId) const
    {
        auto iterator = m_definitionCtxStartTokenOfMigratedSymbol.find(tokenId);
        return iterator != m_definitionCtxStartTokenOfMigratedSymbol.end() ? std::make_pair(QualifiedNameView{iterator->second.first}, iterator->second.second)
                                                                           : std::make_pair(QualifiedNameView{}, -1_ssz);
    }

    SymbolTranslation::IDExpressionDesc SymbolTranslation::GetIdExpression(ssize_t tokenId) const
    {
        // double step lookup:
        // tok1::tok3::tok5
        // ^         |
        // +---maps--+
        //
        // then tok1 maps to idexpr
        auto& t2t    = m_anyTokenIdToFirstTokenIdOfAnIdExpr;
        auto& ft2ide = m_firstTokenIdToIdExpression;

        auto ftIt = t2t.find(tokenId);
        ssize_t tokenIdToSearch = ftIt == t2t.end() ? tokenId : ftIt->second;

        auto idIt = ft2ide.find(tokenIdToSearch);
        return idIt == ft2ide.end() ? IDExpressionDesc{} : idIt->second;
    }

    void SymbolTranslation::PreCacheTokenOfReference_(const Seenat &at)
    {
        IDExpressionDesc& idExprDesc = m_firstTokenIdToIdExpression[at.m_where.m_expressionSpan.a];
        idExprDesc.m_span = at.m_where.m_expressionSpan;
        // fillup the spans map
        for (ssize_t i = at.m_where.m_expressionSpan.a; i <= at.m_where.m_expressionSpan.b; ++i)
        {
            m_anyTokenIdToFirstTokenIdOfAnIdExpr[i] = at.m_where.m_expressionSpan.a;
        }
        // verify that we are the rightmost and update.  (for def of 'rightmost' refer to IDExpressionDesc's comment)
        ssize_t index = at.m_where.m_focusedTokenId;
        if (index >= idExprDesc.m_mutatedRightMost.m_index)
        {
            idExprDesc.m_mutatedRightMost.m_index = index;
            idExprDesc.m_mutatedRightMost.m_symbol = at.m_referredDefinition;
        }
    }

    // checks whether 2 functions are overloads
    bool SymbolTranslation::BelongsToOverloadSet_(QualifiedNameView originalName1, QualifiedNameView originalName2) const
    {
        string_view core = RemoveLastParenthesisGroup(originalName1);
        auto* coreSymbol = m_accessSymbol(QualifiedNameView{core});
        return coreSymbol &&   // not nullptr (the set exists), and...
            (core == originalName2    // either the overload-set itself is directly the same symbol, or...
             || coreSymbol->GetSubRefAs<OverloadSetInfo>().Has({originalName2}));   // the symbol we're looking for, belongs to that set.
    }
}
