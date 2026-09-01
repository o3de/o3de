/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include "AzslcKindInfo.h"

namespace AZ::ShaderCompiler
{
    //! Strategy for the visitation extent of relationship
    MAKE_REFLECTABLE_ENUM_POWER( RelationshipExtent
                                ,Self                  // include the startup symbol
                                ,Reference             // the seenat collection of a symbol
                                ,Family                // up and down list of overrides
                                ,OverloadSet           // group of overloads
                                ,Recursive             // explore the entire homonymous graph, including e.g children's overloads's references. And e.g the base of an overload of a child
    );
    using RelationshipExtentFlag = Flag<RelationshipExtent>;

    //! Facility to iterate over a graph of symbol appearances and relationships throughout the source.
    //! as defined by the document image ../../Documentation/HomonymVisitation.png
    class HomonymVisitor
    {
    public:
        using SymbolGetterType = std::function< KindInfo* (QualifiedNameView) >;

        //! @param symbolGetter     A closure callback that allows this object to be decoupled from other classes in this software.
        //!                         Usually it would be bound to GetIdAndKindInfo of the SymbolAggregator.
        //!                         Its signature is "takes a QualifiedNameView and return KindInfo*"
        HomonymVisitor(SymbolGetterType symbolGetter)
            : m_getInfo(symbolGetter)
        {}

        //! Start function. Clients call this.
        //! @param symbol               Startup symbol of the discovery
        //! @param functor              A callback function-object of any sort, of the signature "takes a Seenat, a RelationshipExtent and returns void"
        //! @param walkConfiguration    The visitation strategy. You can add flag elements using operator |
        //!                             e.g  RelationshipExtentFlag{ RelationshipExtent::Self } | RelationshipExtent::Reference
        template< typename FunctorType >
        void operator() (const IdentifierUID& symbol, FunctorType&& functor, RelationshipExtentFlag walkConfiguration) const
        {
            KindInfo* info = m_getInfo(symbol.GetName());
            assert(info);
            unordered_set<IdentifierUID> visited;
            // visit self if requested
            if (walkConfiguration & RelationshipExtent::Self)
            {
                Seenat at {symbol, GetDefinitionTokensLocation(*info)};
                functor(at, RelationshipExtent::Self);
            }
            return Walk(symbol, std::forward<FunctorType&&>(functor), walkConfiguration, visited);
        }
    private:

        template< typename FunctorType >
        void Walk(const IdentifierUID& symbol, FunctorType&& functor, RelationshipExtentFlag walkConfiguration, unordered_set<IdentifierUID>& visitedSet) const
        {
            if (visitedSet.find(symbol) != visitedSet.end())
            {
                return;
            }
            // mark self as visited, immediately; to avoid re-entrancy.
            visitedSet.insert(symbol);

            // visit natural seenats if requested
            if (walkConfiguration & RelationshipExtent::Reference)
            {
                VisitSeenats(symbol, functor);
            }
            // visit family if requested
            if (walkConfiguration & RelationshipExtent::Family)
            {
                VisitFamily(symbol, functor, walkConfiguration, visitedSet);
            }
            // visit overload-set if requested
            if (walkConfiguration & RelationshipExtent::OverloadSet)
            {
                VisitOverloadSet(symbol, functor, walkConfiguration, visitedSet);
            }
        }

        template< typename FunctorType >
        void VisitOverloadSet(const IdentifierUID &symbol, FunctorType&& functor, RelationshipExtentFlag walkConfiguration, unordered_set<IdentifierUID>& visitedSet) const
        {
            KindInfo& kind = *m_getInfo(symbol.GetName());
            if (kind.GetKind() == Kind::Function)
            {
                string_view core = RemoveLastParenthesisGroup(symbol.GetName());  // strip the function decoration to find the symbol
                // visit itself (it's interesting to report the naked set itself, and its references are all the unresolved call sites)
                VisitDefinitionIdentifier(IdentifierUID{core}, functor, RelationshipExtent::OverloadSet, walkConfiguration, visitedSet);
                KindInfo* overloadSet = m_getInfo(QualifiedNameView{core});
                // visit the concrete members of that set:
                auto& setInfo = overloadSet->GetSubRefAs<OverloadSetInfo>();
                setInfo.ForEach([&](const IdentifierUID& brother)
                                {
                                    VisitDefinitionIdentifier(brother, functor, RelationshipExtent::OverloadSet, walkConfiguration, visitedSet);
                                });
            }
        }

        template< typename FunctorType >
        void VisitFamily(const IdentifierUID& symbol, FunctorType&& functor, RelationshipExtentFlag walkConfiguration, unordered_set<IdentifierUID>& visitedSet) const
        {
            KindInfo& kind = *m_getInfo(symbol.GetName());
            if (kind.GetKind() == Kind::Function)  // for now only functions have families. we'll have to abstract family access if we implement properties.
            {
                auto& fInfo = kind.GetSubRefAs<FunctionInfo>();
                // children visit
                for (const IdentifierUID& overrideUid : fInfo.m_overrides)
                {
                    VisitDefinitionIdentifier(overrideUid, functor, RelationshipExtent::Family, walkConfiguration, visitedSet);
                }
                // parent visit
                if (fInfo.m_base)
                {
                    VisitDefinitionIdentifier(*fInfo.m_base, functor, RelationshipExtent::Family, walkConfiguration, visitedSet);
                }
            }
        }

        template< typename FunctorType >
        void VisitSeenats(const IdentifierUID& symbol, FunctorType&& functor) const
        {
            auto& seenats = GetSeenats(symbol);
            for (auto&& at : seenats)
            {
                functor(at, RelationshipExtent::Reference); // callback on reference. which are leaves. no need to add to visited set.
            }
        }

        //! This function is named this way because of the opposition with "direct references" which already have seenats.
        //! definition locations don't register a seenat, so we need to re-synthesize one.
        template< typename FunctorType >
        void VisitDefinitionIdentifier(const IdentifierUID& symbol, FunctorType&& functor, RelationshipExtent visitCategory, RelationshipExtentFlag walkConfiguration, unordered_set<IdentifierUID>& visitedSet) const
        {
            if (visitedSet.find(symbol) == visitedSet.end())
            {
                auto& info = *m_getInfo(symbol.GetName());
                Seenat at{symbol, GetDefinitionTokensLocation(info)};
                functor(at, visitCategory); // callback client
                if (walkConfiguration & RelationshipExtent::Recursive)
                {
                    Walk(symbol, functor, walkConfiguration, visitedSet);  // walk will mark symbol as visited upon entrance
                }
                else
                {
                    visitedSet.insert(symbol); // mark as visited
                }
            }
        }

        const vector<Seenat>& GetSeenats(const IdentifierUID& uid) const
        {
            auto& kind = *m_getInfo(uid.GetName());
            return kind.GetSeenats();
        }

        SymbolGetterType m_getInfo;
    };
}
