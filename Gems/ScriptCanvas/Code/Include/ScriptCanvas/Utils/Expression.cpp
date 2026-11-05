/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */
#include "Expression.h"

#include <AzCore/Console/IConsole.h>
#include <AzCore/Console/ILogger.h>
#include <AzCore/Interface/Interface.h>
#include <AzCore/Serialization/Locale.h>
#include <AzCore/std/algorithm.h>
#include <AzCore/std/math.h>
#include <AzCore/std/utils.h>
#include <cctype>
#include <cerrno>
#include <cmath>
#include <cstdlib>

namespace ScriptCanvas::Expression
{
    namespace
    {
        struct Operation
        {
            const char* m_name;
            int m_precedence; //!< Zero denotes a named function.
            int m_arity;
            double (*m_apply)(double, double);
        };

        template<double (*Function)(double)>
        double ApplyUnary(double a, double)
        {
            return Function(a);
        }

        const Operation Operations[] = {
            { "||",
              1,
              2,
              [](double a, double b) -> double
              {
                  return a || b;
              } },
            { "&&",
              2,
              2,
              [](double a, double b) -> double
              {
                  return a && b;
              } },
            { "==",
              3,
              2,
              [](double a, double b) -> double
              {
                  return a == b;
              } },
            { "!=",
              3,
              2,
              [](double a, double b) -> double
              {
                  return a != b;
              } },
            { "<",
              4,
              2,
              [](double a, double b) -> double
              {
                  return a < b;
              } },
            { "<=",
              4,
              2,
              [](double a, double b) -> double
              {
                  return a <= b;
              } },
            { ">",
              4,
              2,
              [](double a, double b) -> double
              {
                  return a > b;
              } },
            { ">=",
              4,
              2,
              [](double a, double b) -> double
              {
                  return a >= b;
              } },
            { "+",
              5,
              2,
              [](double a, double b)
              {
                  return a + b;
              } },
            { "-",
              5,
              2,
              [](double a, double b)
              {
                  return a - b;
              } },
            { "*",
              6,
              2,
              [](double a, double b)
              {
                  return a * b;
              } },
            { "/",
              6,
              2,
              [](double a, double b)
              {
                  return a / b;
              } },
            { "%", 6, 2, AZStd::fmod },
            { "u+",
              7,
              1,
              [](double a, double)
              {
                  return a;
              } },
            { "u-",
              7,
              1,
              [](double a, double)
              {
                  return -a;
              } },
            { "!",
              7,
              1,
              [](double a, double) -> double
              {
                  return !a;
              } },
            { "^", 8, 2, AZStd::pow },
            { "**", 8, 2, AZStd::pow },
            { "sin", 0, 1, ApplyUnary<AZStd::sin> },
            { "cos", 0, 1, ApplyUnary<AZStd::cos> },
            { "tan", 0, 1, ApplyUnary<AZStd::tan> },
            { "asin", 0, 1, ApplyUnary<AZStd::asin> },
            { "acos", 0, 1, ApplyUnary<AZStd::acos> },
            { "atan", 0, 1, ApplyUnary<AZStd::atan> },
            { "sinh", 0, 1, ApplyUnary<std::sinh> },
            { "cosh", 0, 1, ApplyUnary<std::cosh> },
            { "tanh", 0, 1, ApplyUnary<std::tanh> },
            { "asinh", 0, 1, ApplyUnary<std::asinh> },
            { "acosh", 0, 1, ApplyUnary<std::acosh> },
            { "atanh", 0, 1, ApplyUnary<std::atanh> },
            { "sqrt", 0, 1, ApplyUnary<AZStd::sqrt> },
            { "cbrt", 0, 1, ApplyUnary<std::cbrt> },
            { "exp", 0, 1, ApplyUnary<AZStd::exp> },
            { "exp2", 0, 1, ApplyUnary<AZStd::exp2> },
            { "log", 0, 1, ApplyUnary<std::log> },
            { "ln", 0, 1, ApplyUnary<std::log> },
            { "log2", 0, 1, ApplyUnary<std::log2> },
            { "log10", 0, 1, ApplyUnary<std::log10> },
            { "abs", 0, 1, ApplyUnary<AZStd::abs> },
            { "floor", 0, 1, ApplyUnary<AZStd::floor> },
            { "ceil", 0, 1, ApplyUnary<AZStd::ceil> },
            { "round", 0, 1, ApplyUnary<AZStd::round> },
            { "trunc", 0, 1, ApplyUnary<AZStd::trunc> },
            { "pow", 0, 2, AZStd::pow },
            { "atan2", 0, 2, AZStd::atan2 },
            { "hypot", 0, 2, std::hypot },
            { "min", 0, 2, std::fmin },
            { "max", 0, 2, std::fmax },
        };

        const Operation* FindOperation(AZStd::string_view name)
        {
            for (const auto& op : Operations)
            {
                if (op.m_name == name)
                {
                    return &op;
                }
            }
            return nullptr;
        }

        bool BracketsMatch(const AZStd::string& open, const AZStd::string& close)
        {
            return (open == "(" && close == ")") || (open == "[" && close == "]") || (open == "{" && close == "}");
        }
    } // namespace

    Result<AZStd::vector<Token>> Tokenize(const AZStd::string& source, bool bracedVariables)
    {
        AZ::Locale::ScopedSerializationLocale locale;
        AZStd::vector<Token> tokens;
        for (size_t i = 0; i < source.size();)
        {
            const unsigned char c = static_cast<unsigned char>(source[i]);
            if (std::isspace(c))
            {
                ++i;
                continue;
            }
            if (std::isdigit(c) || c == '.')
            {
                const char* begin = source.c_str() + i;
                char* end = nullptr;
                errno = 0;
                const double value = std::strtod(begin, &end);
                if (end == begin || errno == ERANGE || !std::isfinite(value) ||
                    (source[i] == '0' && i + 1 < source.size() && (source[i + 1] == 'x' || source[i + 1] == 'X')))
                {
                    return { {}, "Invalid number at " + AZStd::string::format("%zu", i) };
                }
                tokens.push_back({ Kind::Number, {}, value });
                i = static_cast<size_t>(end - source.c_str());
            }
            else if (c == '{' && bracedVariables)
            {
                const auto close = source.find('}', i + 1);
                if (close == AZStd::string::npos || close == i + 1)
                {
                    return { {}, "Invalid variable at " + AZStd::string::format("%zu", i) };
                }
                tokens.push_back({ Kind::Variable, source.substr(i + 1, close - i - 1) });
                i = close + 1;
            }
            else if (std::isalpha(c) || c == '_')
            {
                const auto start = i++;
                while (i < source.size() && (std::isalnum(static_cast<unsigned char>(source[i])) || source[i] == '_'))
                {
                    ++i;
                }
                tokens.push_back({ Kind::Name, source.substr(start, i - start) });
            }
            else if (c == '?' || c == ':')
            {
                tokens.push_back({ c == '?' ? Kind::Question : Kind::Colon, source.substr(i++, 1) });
            }
            else if (AZStd::string_view("([{)]},").find(static_cast<char>(c)) != AZStd::string_view::npos)
            {
                const auto kind = c == ','                                                             ? Kind::Comma
                    : AZStd::string_view("([{").find(static_cast<char>(c)) != AZStd::string_view::npos ? Kind::Open
                                                                                                       : Kind::Close;
                tokens.push_back({ kind, source.substr(i++, 1) });
            }
            else
            {
                auto text = source.substr(i, 2);
                const auto* op = FindOperation(text);
                if (!op)
                {
                    text = source.substr(i, 1);
                    op = FindOperation(text);
                }
                if (!op || op->m_precedence == 0)
                {
                    return { {}, "Unexpected character at " + AZStd::string::format("%zu", i) };
                }
                tokens.push_back({ Kind::Operator, text });
                i += text.size();
            }
        }
        return { AZStd::move(tokens), {} };
    }

    Result<Program> ShuntingYard(const AZStd::vector<Token>& tokens)
    {
        Program result;
        AZStd::vector<Token> stack;
        struct Frame
        {
            AZStd::string m_bracket;
            bool m_isFunction;
            size_t m_commas = 0;
        };
        AZStd::vector<Frame> frames;
        bool needOperand = true;
        auto pop = [&]
        {
            const auto token = stack.back();
            stack.pop_back();
            if (token.m_kind == Kind::Question)
            {
                return false;
            }
            if (token.m_kind == Kind::Colon)
            {
                result.m_instructions[token.m_index].m_index = result.m_instructions.size();
            }
            else
            {
                result.m_instructions.push_back(token);
            }
            return true;
        };

        for (size_t i = 0; i < tokens.size(); ++i)
        {
            auto token = tokens[i];
            switch (token.m_kind)
            {
            case Kind::Number:
            case Kind::Name:
            case Kind::Variable:
                if (!needOperand)
                {
                    return { {}, "Expected an operator before " + token.m_text };
                }
                if (token.m_kind != Kind::Number)
                {
                    if (token.m_kind == Kind::Name && i + 1 < tokens.size() && tokens[i + 1].m_kind == Kind::Open)
                    {
                        const auto* fn = FindOperation(token.m_text);
                        if (!fn || fn->m_precedence != 0)
                        {
                            return { {}, "Unknown function: " + token.m_text };
                        }
                        token.m_kind = Kind::Function;
                        stack.push_back(token);
                        break;
                    }
                    if (token.m_kind == Kind::Name &&
                        (token.m_text == "pi" || token.m_text == "e" || token.m_text == "true" || token.m_text == "false"))
                    {
                        token.m_kind = Kind::Number;
                        token.m_value = token.m_text == "pi" ? AZStd::acos(-1.0)
                            : token.m_text == "e"            ? AZStd::exp(1.0)
                            : token.m_text == "true"         ? 1.0
                                                             : 0.0;
                    }
                    else
                    {
                        token.m_kind = Kind::Variable;
                        const auto it = AZStd::find(result.m_variables.begin(), result.m_variables.end(), token.m_text);
                        token.m_index = static_cast<size_t>(it - result.m_variables.begin());
                        if (it == result.m_variables.end())
                        {
                            result.m_variables.push_back(token.m_text);
                        }
                    }
                }
                result.m_instructions.push_back(token);
                needOperand = false;
                break;
            case Kind::Open:
                if (!needOperand)
                {
                    return { {}, "Expected an operator before a bracket" };
                }
                frames.push_back({ token.m_text, !stack.empty() && stack.back().m_kind == Kind::Function });
                stack.push_back(token);
                break;
            case Kind::Comma:
                if (needOperand || frames.empty() || !frames.back().m_isFunction)
                {
                    return { {}, "Unexpected comma" };
                }
                while (stack.back().m_kind != Kind::Open)
                {
                    if (!pop())
                    {
                        return { {}, "Missing ':' in conditional" };
                    }
                }
                ++frames.back().m_commas;
                needOperand = true;
                break;
            case Kind::Close:
                if (needOperand || frames.empty() || !BracketsMatch(frames.back().m_bracket, token.m_text))
                {
                    return { {}, "Invalid closing bracket" };
                }
                while (stack.back().m_kind != Kind::Open)
                {
                    if (!pop())
                    {
                        return { {}, "Missing ':' in conditional" };
                    }
                }
                stack.pop_back();
                if (frames.back().m_isFunction)
                {
                    const auto* fn = FindOperation(stack.back().m_text);
                    if (frames.back().m_commas + 1 != static_cast<size_t>(fn->m_arity))
                    {
                        return { {}, "Wrong argument count: " + stack.back().m_text };
                    }
                    pop();
                }
                frames.pop_back();
                needOperand = false;
                break;
            case Kind::Question:
                if (needOperand)
                {
                    return { {}, "Expected a condition before '?'" };
                }
                while (!stack.empty() && stack.back().m_kind == Kind::Operator)
                {
                    pop();
                }
                token.m_index = result.m_instructions.size();
                result.m_instructions.push_back({ Kind::JumpIfFalse, "?" });
                stack.push_back(token);
                needOperand = true;
                break;
            case Kind::Colon:
                {
                    if (needOperand)
                    {
                        return { {}, "Expected an expression before ':'" };
                    }
                    while (!stack.empty() && stack.back().m_kind != Kind::Question)
                    {
                        if (stack.back().m_kind == Kind::Open)
                        {
                            return { {}, "Unexpected ':'" };
                        }
                        pop();
                    }
                    if (stack.empty())
                    {
                        return { {}, "':' without '?'" };
                    }
                    const auto conditional = stack.back().m_index;
                    stack.pop_back();
                    token.m_index = result.m_instructions.size();
                    result.m_instructions.push_back({ Kind::Jump, ":" });
                    result.m_instructions[conditional].m_index = result.m_instructions.size();
                    stack.push_back(token);
                    needOperand = true;
                    break;
                }
            case Kind::Operator:
                {
                    if (needOperand && (token.m_text == "+" || token.m_text == "-"))
                    {
                        token.m_text = "u" + token.m_text;
                    }
                    const auto* op = FindOperation(token.m_text);
                    if (!op || op->m_precedence == 0 || (op->m_arity == 1) != needOperand)
                    {
                        return { {}, "Unexpected operator: " + token.m_text };
                    }
                    // Prefix operators never pop: this makes both -2^2 and 2^-2 work.
                    if (op->m_arity == 2)
                    {
                        const bool rightAssociative = token.m_text == "^" || token.m_text == "**";
                        while (!stack.empty() && stack.back().m_kind == Kind::Operator)
                        {
                            const auto* top = FindOperation(stack.back().m_text);
                            if (top->m_precedence < op->m_precedence || (top->m_precedence == op->m_precedence && rightAssociative))
                            {
                                break;
                            }
                            pop();
                        }
                    }
                    stack.push_back(token);
                    needOperand = true;
                    break;
                }
            default:
                return { {}, "Expected tokens produced by Tokenize" };
            }
        }
        if (needOperand)
        {
            return { {}, "Empty or incomplete expression" };
        }
        if (!frames.empty())
        {
            return { {}, "Unclosed bracket" };
        }
        while (!stack.empty())
        {
            if (!pop())
            {
                return { {}, "Missing ':' in conditional" };
            }
        }
        return { AZStd::move(result), {} };
    }

    Result<AZStd::vector<double>> Bind(const Program& expression, const Bindings& values)
    {
        AZStd::vector<double> bound;
        bound.reserve(expression.m_variables.size());
        for (const auto& name : expression.m_variables)
        {
            const auto it = values.find(name);
            if (it == values.end())
            {
                return { {}, "Missing variable: " + name };
            }
            if (!std::isfinite(it->second))
            {
                return { {}, "Non-finite variable: " + name };
            }
            bound.push_back(it->second);
        }
        return { AZStd::move(bound), {} };
    }

    namespace
    {
        Result<double> Apply(AZStd::string_view name, double a, double b)
        {
            const auto* op = FindOperation(name);
            if (!op)
            {
                return { {}, "Unknown operation: " + AZStd::string(name) };
            }
            if (!std::isfinite(a) || !std::isfinite(b))
            {
                return { {}, "Non-finite operand" };
            }
            if ((name == "/" || name == "%") && b == 0)
            {
                return { {}, "Division by zero" };
            }
            const double value = op->m_apply(a, b);
            if (!std::isfinite(value))
            {
                return { {}, "Non-finite result: " + AZStd::string(name) };
            }
            return { value, {} };
        }
    }

    Result<double> Evaluate(const Program& expression, const AZStd::vector<double>& values)
    {
        if (values.size() != expression.m_variables.size())
        {
            return { {}, "Wrong binding count" };
        }
        for (double value : values)
        {
            if (!std::isfinite(value))
            {
                return { {}, "Non-finite binding" };
            }
        }
        AZStd::vector<double> stack;
        stack.reserve(expression.m_instructions.size());
        for (size_t pc = 0; pc < expression.m_instructions.size();)
        {
            const auto& token = expression.m_instructions[pc++];
            if (token.m_kind == Kind::JumpIfFalse || token.m_kind == Kind::Jump)
            {
                if (token.m_index < pc || token.m_index > expression.m_instructions.size())
                {
                    return { {}, "Invalid jump target" };
                }
                bool takeJump = true;
                if (token.m_kind == Kind::JumpIfFalse)
                {
                    if (stack.empty())
                    {
                        return { {}, "Missing condition in RPN" };
                    }
                    takeJump = stack.back() == 0;
                    stack.pop_back();
                }
                if (takeJump)
                {
                    pc = token.m_index;
                }
                continue;
            }
            double value = 0;
            if (token.m_kind == Kind::Number)
            {
                value = token.m_value;
            }
            else if (token.m_kind == Kind::Variable)
            {
                if (token.m_index >= values.size())
                {
                    return { {}, "Invalid variable index" };
                }
                value = values[token.m_index];
            }
            else
            {
                const auto* op = FindOperation(token.m_text);
                if ((token.m_kind != Kind::Operator && token.m_kind != Kind::Function) || !op ||
                    stack.size() < static_cast<size_t>(op->m_arity))
                {
                    return { {}, "Invalid RPN" };
                }
                double b = 0;
                if (op->m_arity == 2)
                {
                    b = stack.back();
                    stack.pop_back();
                }
                const double a = stack.back();
                stack.pop_back();
                const auto applied = Apply(token.m_text, a, b);
                if (!applied)
                {
                    return applied;
                }
                value = applied.m_value;
            }
            if (!std::isfinite(value))
            {
                return { {}, "Non-finite result: " + token.m_text };
            }
            stack.push_back(value);
        }
        if (stack.size() != 1)
        {
            return { {}, "Invalid RPN" };
        }
        return { stack.back(), {} };
    }

    namespace
    {
        void EvaluateConsole(const AZ::ConsoleCommandContainer& arguments)
        {
            if (arguments.empty())
            {
                AZLOG_INFO("Usage: sc_eval <expression>; variables read numeric console cvars.");
                return;
            }
            AZStd::string source;
            for (const auto& argument : arguments)
            {
                if (!source.empty())
                {
                    source += ' ';
                }
                source.append(argument.data(), argument.size());
            }
            if (source.size() >= 2 && source.front() == source.back() && (source.front() == '"' || source.front() == '\''))
            {
                source = source.substr(1, source.size() - 2);
            }
            const auto tokens = Tokenize(source, true);
            const auto parsed = tokens ? ShuntingYard(tokens.m_value) : Result<Program>{ {}, tokens.m_error };
            if (!parsed)
            {
                AZLOG_WARN("sc_eval: %s", parsed.m_error.c_str());
                return;
            }
            AZ::IConsole* console = AZ::Interface<AZ::IConsole>::Get();
            AZStd::vector<double> values;
            values.reserve(parsed.m_value.m_variables.size());
            for (const auto& name : parsed.m_value.m_variables)
            {
                double value = 0;
                if (!console || console->GetCvarValue(name.c_str(), value) != AZ::GetValueResult::Success)
                {
                    AZLOG_WARN("sc_eval: Unknown or non-numeric console variable: %s", name.c_str());
                    return;
                }
                values.push_back(value);
            }
            const auto result = Evaluate(parsed.m_value, values);
            if (result)
            {
                AZLOG_INFO("sc_eval: %.17g", result.m_value);
            }
            else
            {
                AZLOG_WARN("sc_eval: %s", result.m_error.c_str());
            }
        }

        AZ_CONSOLEFREEFUNC(
            "sc_eval",
            EvaluateConsole,
            AZ::ConsoleFunctorFlags::DontReplicate,
            "Evaluates math, logic and ternary expressions. Variables read numeric console cvars; e.g. sc_eval sin(pi/2) * vsync_interval");
    }
} // namespace ScriptCanvas::Expression
