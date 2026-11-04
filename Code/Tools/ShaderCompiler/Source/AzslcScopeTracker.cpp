/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "AzslcScopeTracker.h"

namespace AZ::ShaderCompiler
{
    ScopeTracker::ScopeTracker(SymbolGetterFunctorT symbolFinder)
        : m_symFinder{symbolFinder}
    {
        // global scope is represented as "/"
        // and it is an invariant of this program that m_currentScopePath will never be empty (always at least "/")
        EnterScope("/", 0);
        assert(IsRooted(m_currentScopePath));
    }

    // variation that takes an absolute path in (used by entry into deported method definitions, they can jump scope)
    void ScopeTracker::EnterScope(QualifiedNameView scopeName, ssize_t tokenStreamPosition)
    {
        m_oldScopePaths.push(m_currentScopePath);
        m_currentScopePath = scopeName;
        UpdateCurScopeUID();
        // keep track of the scope begin:
        m_scopeIntervals[m_currentScopeUID].a = tokenStreamPosition;
        verboseCout << "Scope Entry. new current path is " << m_currentScopePath << azEndl;
    }

    // classic variation that takes a cumulative leaf
    void ScopeTracker::EnterScope(string_view scopeName, ssize_t tokenStreamPosition)
    {
        auto accumulatedPath = JoinPath(m_currentScopePath, scopeName, JoinPolicy::EmptyMeansEmpty);
        EnterScope(QualifiedNameView{accumulatedPath}, tokenStreamPosition);
    }

    void ScopeTracker::ExitScope(ssize_t tokenStreamPosition)
    {
        // keep track of the scope end token:
        m_scopeIntervals[m_currentScopeUID].b = tokenStreamPosition;
        m_currentScopePath = m_oldScopePaths.top();
        m_oldScopePaths.pop();
        verboseCout << "Exit scope. new current path is " << m_currentScopePath << azEndl;

        UpdateCurScopeUID();
    }

    void ScopeTracker::UpdateCurScopeUID()
    {
        auto& [uid, _] = *m_symFinder(m_currentScopePath);
        // In this case, register the scope identifier before calling EnterScope. This is a sequential coupling antipattern for now.
        assert(IsRooted(m_currentScopePath));
        m_currentScopeUID = uid;
    }

    QualifiedNameView ScopeTracker::GetNameOfCurScope() const
    {
        return m_currentScopeUID.m_name;
    }

    QualifiedNameView ScopeTracker::GetNameOfCurParentScope() const
    {
        auto nameOfParentScope = QualifiedNameView{GetParentName(GetNameOfCurScope())};
        return nameOfParentScope;
    }

    // For debugging
    void ScopeTracker::DumpScopeIntervals() const
    {
        std::cout << "\nScopeTracker::DumpScopeIntervals:\n";
        for (auto const& [symId, interval] : m_scopeIntervals)
        {
            std::cout << symId.GetName() << ": a=" << interval.a << ", b=" << interval.b << "\n";
        }
        std::cout << "\n";
    }
}
