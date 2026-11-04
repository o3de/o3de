/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */
#pragma once

#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/string/string_view.h>

// O3DE_DEPRECATION_NOTICE(GHI-20270): replaces the proposed-to-be-removed ExpressionEvaluation gem API.
// Removal timing and compatibility are pending SIG Content review: o3de/sig-content#152.
namespace ScriptCanvas::Expression
{
    enum class Kind
    {
        Number,
        Name,
        Operator,
        Open,
        Close,
        Comma,
        Function,
        Variable,
        Question,
        Colon,
        JumpIfFalse,
        Jump
    };

    struct Token
    {
        Kind m_kind;
        AZStd::string m_text;
        double m_value = 0;
        size_t m_index = 0; // Variable slot or forward jump target.
    };

    struct Program
    {
        AZStd::vector<Token> m_instructions; // RPN plus forward jumps for ?:.
        AZStd::vector<AZStd::string> m_variables;
    };

    using Bindings = AZStd::unordered_map<AZStd::string, double>;

    template<class T>
    struct Result
    {
        T m_value{};
        AZStd::string m_error;
        explicit operator bool() const
        {
            return m_error.empty();
        }
    };

    // C++17. Numbers are doubles; comparisons/logical operators return 0 or 1.
    // Grouping: (), [], {}. Power: ^ or ** (right associative). Angles: radians.
    // Constants: pi, e, true, false. Names are case sensitive.
    // ?: is right associative, below ||, and evaluates only the selected branch.
    // Bind requires finite values for every variable, including both branches.
    // shortcut: && and || evaluate both sides; add jumps if short-circuiting is needed.
    // Errors are returned explicitly because the engine builds without exceptions.
    // In braced-variable mode, {any pin name} is one variable, including constants.
    Result<AZStd::vector<Token>> Tokenize(const AZStd::string& source, bool bracedVariables = false);
    Result<Program> ShuntingYard(const AZStd::vector<Token>& tokens);
    Result<AZStd::vector<double>> Bind(const Program& expression, const Bindings& values);
    Result<double> Evaluate(const Program& expression, const AZStd::vector<double>& values);
} // namespace ScriptCanvas::Expression
