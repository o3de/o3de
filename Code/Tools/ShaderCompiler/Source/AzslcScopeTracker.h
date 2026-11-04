/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include "AzslcSymbolAggregator.h"

namespace AZ::ShaderCompiler
{
    using SymbolGetterFunctorT = std::function< IdAndKind* (QualifiedNameView) >;

    struct ScopeTracker
    {
        // please pass me a closure that has access to the symbol database, at construction. (RAII principle: no halfass initialized state)
        explicit ScopeTracker(SymbolGetterFunctorT symbolFinder);

        /// relative descent into a normal scope tree
        void EnterScope(string_view scopeName, ssize_t tokenStreamPosition);

        /// jump to unrelated scope
        void EnterScope(QualifiedNameView scopeName, ssize_t tokenStreamPosition);

        void ExitScope(ssize_t tokenStreamPosition);

        void UpdateCurScopeUID();

        // For debugging
        void DumpScopeIntervals() const;

        QualifiedNameView GetNameOfCurScope() const;

        QualifiedNameView GetNameOfCurParentScope() const;

        SymbolGetterFunctorT  m_symFinder;
        QualifiedName         m_currentScopePath; // example: "/sisdk3/gfx/"
        IdentifierUID         m_currentScopeUID;
        stack<QualifiedName>  m_oldScopePaths;
        // Store each scopes begin/end in the original source
        using MapOfScopeIntervals = unordered_map<IdentifierUID, misc::Interval>;
        MapOfScopeIntervals   m_scopeIntervals;
    };
}
