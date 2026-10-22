/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include "AzslcListener.h"

namespace AZ::ShaderCompiler
{
    MAKE_REFLECTABLE_ENUM_POWER(BehaviorEvent, OnDeclaration, OnReference);
    using BehaviorEventFlag = Flag<BehaviorEvent>;

    //! DeclarationSite is for usages like XX in "struct XX {};" or "int XX = 2;"
    //! ReferenceSite   is for usages in expressions
    enum class UsageContext
    {
        DeclarationSite = BehaviorEvent::OnDeclaration,
        ReferenceSite   = BehaviorEvent::OnReference,
    };

    //! store translated names (map of names to new names)
    //! used to transform /SRG/cb0/pistils to SRG0_cb0_pistils in a disambiguated form
    class SymbolTranslation
    {
    public:
        //! accessSymbol    is a function to query a symbol in the symbol table. (it is recommend to bind it to SymbolAggregator::GetIdAndKindInfo())
        void SetAccessSymbolQueryFunctor(std::function<KindInfo*(QualifiedNameView)> accessSymbol);

        void SetGetSeenatFunctor(std::function< vector<Seenat>& (QualifiedNameView) > getSeenats);

        //! landingScope is the destination scope for the (translated) declaration of the symbol.
        void RegisterLandingScope(const IdentifierUID& originalSymbol, QualifiedNameView landingScope);

        //! respect this signature for your event handlers.
        //!   originalSymbol        is a reminder of the symbol you are mutating
        //!   qualificationStrategy is in case you chose a flag of both events, you can distinguish the context.
        //!   naturalRename         is the "otherwise produced" translation result, if you return it as is, that'd be an "identity" custom behavior.
        //!   tokenId               often unavailable (=NotOverToken ==-1) but when available it indicates which token index we're iterating over in the original stream
        using TranslationBehaviorDelegate =
            std::function<
                string (QualifiedNameView originalSymbol, UsageContext qualificationStrategy, string naturalRename, ssize_t tokenId) >;

        //! register specific behaviors to post-mutate names differently than the default algorithm.
        //! a behavior is for one original symbol. it can trigger on declaration or reference or both
        void AddCustomBehavior(QualifiedNameView originalSymbol, BehaviorEventFlag on, TranslationBehaviorDelegate action);

        //! Create a declaration in the IR for a symbol "originalSymbol" at its destination scope registered previously.
        void MigrateDisambiguateAndCache(QualifiedNameView originalSymbol) const;

        //! Query the registrar
        QualifiedNameView GetLandingScope(QualifiedNameView originalSymbol) const;

        //! Get the translated name for a symbol in AZSL
        string GetTranslatedName(QualifiedNameView originalSymbol, UsageContext qualificationStrategy, ssize_t tokenId) const;

        //! In the idExpression:
        //! nested1::nested2::mutatedRightMost::expressionRightMost
        //! Even if nested2 has been mutated, it doesn't matter because it's expressionRightMost that we are qualifying.
        //! So it's the destination of expressionRightMost that matters, since we will rewrite all its left-hand nesting elements
        //! to fix this expression to refer to the new path for expressionRightMost.
        //! If expressionRightMost itself doesn't have a mutation registered, that doesn't mean one of its parent doesn't.
        //! This is the one we really care about: "the deepest parent with a mutation". here: mutatedRightMost
        //! Thus, the part of the expression that will mutate is this:
        //! [nested1::nested2::mutatedRightMost]::expressionRightMost
        //! nested1 and nested2 will change to the new path (possibly empty), and mutatedRightMost will change to the new name.
        struct IDExpressionDesc
        {
            bool IsEmpty() const
            {
                return m_mutatedRightMost.m_symbol.m_name.empty();
            }
            struct MutatedRightMostInfo
            {
                IdentifierUID m_symbol;    // original name (un-mutated)
                ssize_t       m_index = 0; // in the token stream (same space as m_span.a/b)
            } m_mutatedRightMost;
            misc::Interval m_span;  // nested::mut::most
            //                         ^               ^
        };

        //! For when we roam over tokens, try to query for potential migrations
        pair<QualifiedNameView, ssize_t> OverOriginalDefinitionOf(ssize_t tokenId) const;

        //! Query whether this token belongs to an idExpression, and retrieve a descriptor to it
        IDExpressionDesc GetIdExpression(ssize_t tokenId) const;

        IDExpressionDesc GetIdExpression(Token* token) const
        {
            return GetIdExpression(static_cast<ssize_t>(token->getTokenIndex()));
        }

        //! execute the logic explained in the comment above struct IDExpressionDesc
        //! that is: mutate intermediate symbols by rewriting the path.
        //! emit a fully qualified valid HLSL expression after mutations.
        template< typename NextTokenFunctor >
        string TranslateIdExpression(const IDExpressionDesc& idExprDesc, ssize_t tokenId, const NextTokenFunctor& getNext) const
        {
            string part1 = GetTranslatedName(idExprDesc.m_mutatedRightMost.m_symbol.m_name, UsageContext::ReferenceSite, tokenId);
            string part2;
            for (ssize_t t = idExprDesc.m_mutatedRightMost.m_index + 1; t <= idExprDesc.m_span.b; ++t)
            {
                part2 += getNext(t);
            }
            return part1 + part2;
        }

    private:
        struct CustomBehavior
        {
            TranslationBehaviorDelegate m_action;
            BehaviorEventFlag m_when;
        };
        struct SymbolMutation
        {
            QualifiedName m_landingScope;
            optional<CustomBehavior> m_customBehavior;
        };

        struct FindTranslation_Parameters
        {
            QualifiedNameView m_originalPath;
            QualifiedName m_result;
            const SymbolMutation* m_foundMutation = nullptr;
        };
        // recursive entry to be able to treat a parent migration at any level
        //  A/B/C/D will check for D first, but we need to check for the rightmost change.
        // if A has a migration and B has a migration too, only B counts.
        // so we check descending, D, C, B -> stop. and recompose when going up.-> B'/C/D
        void FindTranslation_(FindTranslation_Parameters& params) const;

        // (private) factorization to simplify the "register translation" loop
        void PreCacheTokenOfReference_(const Seenat& at);

        // extracted for readability
        bool BelongsToOverloadSet_(QualifiedNameView originalName1, QualifiedNameView originalName2) const;

        // cache for renames (stateful rename that can store disambiguation attempts):
        mutable unordered_map< QualifiedName, QualifiedName >        m_renames;
        // database of registered mutations:
        unordered_map< QualifiedName, SymbolMutation >               m_landingScope;
        // quick query for "on top of migrated symbol original definition's first token"
        // also store the end token for easy jump-over
        unordered_map< ssize_t, pair<QualifiedName, ssize_t> >       m_definitionCtxStartTokenOfMigratedSymbol;
        // example: tok1::tok3::tok5 (with tok2 and 4 being ::) -> 5 tokens 1 idexpr.
        // since IDExpression objects are not immutable (mutatedRightMostSymbol iteratively updated)
        // we can't tolerate data duplication. so we chose to store only the first token id (tok1) as key for an idexpression
        unordered_map< ssize_t, IDExpressionDesc >                   m_firstTokenIdToIdExpression;
        // to fill the gap we can map left out tokens 'tok2 tok3 tok4 tok5' to tok1
        unordered_map< ssize_t, ssize_t >                            m_anyTokenIdToFirstTokenIdOfAnIdExpr;

        // internal fast-lookup cache to help symbol disambiguation against renames
        // alternatively, boost::bimap could be used, or an equivalent.
        mutable unordered_map< QualifiedName, QualifiedName >        m_mappedRenames;  // disambiguated rename to original-symbol

        // external IR access functors
        std::function< KindInfo* (QualifiedNameView) >               m_accessSymbol;
        std::function< vector<Seenat>& (QualifiedNameView) >         m_getSeenats;
    };
}
