/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include "GenericUtils.h"
#include "AzslcKindInfo.h"
#include "DiagnosticStream.h"

namespace AZ::ShaderCompiler
{
    // A symbol database
    class SymbolTable
    {
    public:
        auto HasIdentifier(QualifiedNameView symbol) const -> bool;

        auto GetIdAndKindInfo(QualifiedNameView symbol) -> IdAndKind*;

        auto GetIdAndKindInfo(QualifiedNameView symbol) const -> const IdAndKind*;

        //! Register a fresh entry in the symbol map. No KindInfo filled up, the client must do it.
        //! Will return a reference to the newly inserted data.
        //! Will throw in case of ODR violation.
        auto AddIdentifier(QualifiedNameView symbol, Kind kind, optional<size_t> lineNumber = none) -> IdAndKind&;

        //! Can be used to hack the position of a symbol added late, after the phase of semantic check.
        //! Typically hidden symbols added by the compiler such as implicit structs or padding fields.
        void MigrateOrder(const IdentifierUID& symbol, const IdentifierUID& before);

        //! Return true if found and deleted
        bool DeleteIdentifier(const IdentifierUID& name);

        // [GFX TODO]2: use densemap/oahm to avoid fragmentation (depends on [Task 5])
        IdToKindMap           m_symbols;   // declarations of any kind and attached information (unordered bag, but O(1+) lookup)
        vector<IdentifierUID> m_order;     // remember order of apparition in the original source (useful for iteration during dump or emission)
                                           // the order gets altered during Mid-end treatment, by the SymbolAggregator to be arranged in the head-visit AST order instead.
    };
}
