/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "AzslcSymbolTable.h"

namespace AZ::ShaderCompiler
{
    bool SymbolTable::HasIdentifier(QualifiedNameView symbol) const
    {
        return m_symbols.find({symbol}) != m_symbols.cend();
    }

    IdAndKind* SymbolTable::GetIdAndKindInfo(QualifiedNameView symbol)
    {
        auto iter = m_symbols.find({symbol});
        return iter == m_symbols.cend() ? nullptr : &(*iter);
    }

    const IdAndKind* SymbolTable::GetIdAndKindInfo(QualifiedNameView symbol) const
    {
        auto iter = m_symbols.find({symbol});
        return iter == m_symbols.cend() ? nullptr : &(*iter);
    }

    bool SymbolTable::DeleteIdentifier(const IdentifierUID& name)
    {
        auto iter = m_symbols.find(name);
        bool found = iter != m_symbols.end();
        if (found)
        {
            m_symbols.erase(iter);
            // remove from order list too
            m_order.erase(std::remove(m_order.begin(), m_order.end(), name), m_order.end());
        }
        return found;
    }

    void SymbolTable::MigrateOrder(const IdentifierUID& symbol, const IdentifierUID& before)
    {
        // For use in PadToAttributeMutator::InsertPaddingVariables::createVariableInSymbolTable
        auto beforeSymbolIter = std::find(m_order.begin(), m_order.end(), before);
        if (beforeSymbolIter != m_order.end())
        {
            m_order.erase(std::remove(m_order.begin(), m_order.end(), symbol), m_order.end());
            m_order.insert(beforeSymbolIter, symbol);
        }
    }

    IdAndKind& SymbolTable::AddIdentifier(QualifiedNameView symbol, Kind kind, optional<size_t> lineNumber /*= none*/)
    {
        IdentifierUID idUID{symbol};
        auto fetchedIdIt = m_symbols.find(idUID);
        if (fetchedIdIt != m_symbols.end())
        {   // found
            if (kind != fetchedIdIt->second.GetKind())
            {
                ThrowRedeclarationAsDifferentKind(symbol, kind, fetchedIdIt->second, lineNumber);
            }
            else
            {
                throw AzslcException(ORCHESTRATOR_ODR_VIOLATION, "Semantic", lineNumber, none,
                                     ConcatString("ODR (One Definition Rule) violation. Redeclaration of ",
                                                  Kind::ToStr(kind), " ", ExtractLeaf(symbol), " in ", LevelUp(symbol), "  ",
                                                  GetFirstSeenLineMessage(fetchedIdIt->second), "\n"));
            }
        }
        else
        {   // not found
            fetchedIdIt = m_symbols.insert(IdToKindMap::value_type{idUID, {}}).first;
            verboseCout << "new identifier added to database. name " << idUID.m_name << " kind " << Kind::ToStr(kind) << "\n";
        }
        // InitAs completely reassigns the subinfo with a freshly constructed type.
        fetchedIdIt->second.InitAs(kind);
        // remember the apparition order:
        m_order.push_back(fetchedIdIt->first);
        return *fetchedIdIt;
    }
}  // end of namespace AZ::ShaderCompiler
