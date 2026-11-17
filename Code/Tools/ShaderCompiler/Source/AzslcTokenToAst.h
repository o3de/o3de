/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include "antlr4-runtime.h"

namespace AZ::ShaderCompiler
{
    /// Observation: A CST's leaves are "token object" pointers.
    ///  But there is no way to find the reverse association: from token to AST.
    /// This class fills that gap.
    class TokenToAst
    {
    public:
        using AstNode = antlr4::ParserRuleContext;

        AstNode* GetNode(ssize_t tokenId)
        {
            auto iterator = m_tokenToAst.find(tokenId);
            return iterator == m_tokenToAst.end() ? nullptr : iterator->second;
        }

        void SetAssociation(ssize_t tokenId, AstNode* node)
        {
            m_tokenToAst[tokenId] = node;
        }

    protected:

        // generic tokenid to ast pointer map
        unordered_map< ssize_t, AstNode* > m_tokenToAst;
    };
}
