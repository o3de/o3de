/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Math/Vector3.h>
#include <AzCore/std/string/string.h>
#include <cstdlib> // std::atof

namespace WhiteBox
{
    enum class NumericOpMode
    {
        None,
        Move,
        Rotate,
        Scale,
        Extrude, //!< Polygon extrude along normal OR edge extrude along displacement
        Inset,   //!< Face inset (scale-append, ratio 0=collapse → 1=no-inset)
        Draw,    //!< Draw-shape pull depth (+ = add, - = carve)
    };

    enum class NumericAxisConstraint
    {
        Free,
        X,
        Y,
        Z,
    };

    //! Tracks the state of a Blender-style numeric input session.
    //!
    //! Supports simple two-operand expressions:  lhs [op] rhs
    //!   e.g.  "5"          → 5
    //!         "-5"         → -5
    //!         "10*2"       → 20
    //!         "10/4"       → 2.5
    //!         "3+2"        → 5
    //!         "10-3"       → 7
    struct NumericInputState
    {
        // ------------------------------------------------------------------ //
        // Mutation                                                            //
        // ------------------------------------------------------------------ //

        void Begin(NumericOpMode mode)
        {
            Reset();
            m_mode   = mode;
            m_active = true;
        }

        void SetAxis(NumericAxisConstraint axis)
        {
            // pressing the same axis key again → back to Free (Blender behaviour)
            m_axis = (m_axis == axis) ? NumericAxisConstraint::Free : axis;
        }

        void AppendDigit(char digit)
        {
            auto& buf = ActiveBuffer();
            if (buf.size() < 12)
            {
                buf += digit;
            }
        }

        void AppendDecimal()
        {
            auto& buf = ActiveBuffer();
            if (buf.find('.') == AZStd::string::npos)
            {
                // auto-insert a leading 0 before the decimal point
                if (buf.empty() || buf == "-")
                {
                    buf += '0';
                }
                buf += '.';
            }
        }

        //! Handle an operator key: +  -  *  /
        //! Rules:
        //!   – leading '-' when nothing typed yet → start a negative number
        //!   – leading '+' when nothing typed yet → no-op (already positive)
        //!   – any operator after the first operand → record it and start the rhs buffer
        //!   – pressing the same operator again → cancel it (back to lhs editing)
        //! Handle an operator key: +  -  *  /
        //!
        //! Leading '-' → start a negative first operand (e.g. "-5").
        //! Leading '+' → no-op (already positive).
        //! Leading '*' or '/' → implicit LHS of "1"  (so "*2" = 1×2 = 2;  "/4" = 1÷4 = 0.25).
        //!   This lets you type  J *2 Enter  to scale×2, or  J /2 Enter  to scale×0.5.
        //! After a first operand is complete → set the binary operator and start the RHS.
        //! Pressing the same operator again → cancel it (back to LHS editing).
        //! Pressing a different operator while RHS is empty → change operator.
        void AppendOperator(char op)
        {
            if (m_op == '\0')
            {
                if (m_lhs.empty())
                {
                    if (op == '-')
                    {
                        m_lhs = "-"; // leading minus → negative number
                    }
                    else if (op == '*' || op == '/')
                    {
                        // Leading * or / : treat as "1 op rhs"
                        m_lhs = "1";
                        m_op  = op;
                    }
                    // '+' at start is implicit – ignore
                }
                else if (m_lhs != "-")
                {
                    // first operand complete → set binary operator
                    m_op = op;
                }
            }
            else
            {
                if (op == m_op)
                {
                    // pressing same operator again → cancel
                    m_op = '\0';
                    m_rhs.clear();
                }
                else if (m_rhs.empty())
                {
                    // change operator before typing RHS
                    m_op = op;
                }
                else if (op == '-' && m_rhs.empty())
                {
                    // leading minus on RHS
                    m_rhs = "-";
                }
            }
        }

        void Backspace()
        {
            auto& buf = ActiveBuffer();
            if (!buf.empty())
            {
                buf.pop_back();
            }
            else if (m_op != '\0')
            {
                // backspace past the operator
                m_op = '\0';
            }
        }

        void Reset()
        {
            m_mode   = NumericOpMode::None;
            m_axis   = NumericAxisConstraint::Free;
            m_lhs.clear();
            m_op     = '\0';
            m_rhs.clear();
            m_active = false;
        }

        // ------------------------------------------------------------------ //
        // Queries                                                             //
        // ------------------------------------------------------------------ //

        bool IsActive() const { return m_active; }

        //! Returns true if no digits have been entered yet (expression is empty).
        bool IsEmpty() const { return m_lhs.empty() && m_op == '\0'; }

        //! Evaluates the expression and returns the result.
        float GetValue() const
        {
            const float lval = m_lhs.empty() ? 0.0f : static_cast<float>(std::atof(m_lhs.c_str()));

            if (m_op == '\0')
            {
                return lval;
            }

            const float rval = m_rhs.empty() ? 0.0f : static_cast<float>(std::atof(m_rhs.c_str()));

            switch (m_op)
            {
            case '+': return lval + rval;
            case '-': return lval - rval;
            case '*': return lval * rval;
            case '/': return (rval != 0.0f) ? lval / rval : lval;
            default:  return lval;
            }
        }

        AZ::Vector3 GetAxisVector() const
        {
            switch (m_axis)
            {
            case NumericAxisConstraint::X: return AZ::Vector3::CreateAxisX();
            case NumericAxisConstraint::Y: return AZ::Vector3::CreateAxisY();
            case NumericAxisConstraint::Z: return AZ::Vector3::CreateAxisZ();
            default:                       return AZ::Vector3::CreateZero();
            }
        }

        //! Status string suitable for a viewport overlay.
        //! Example:  "Move X: 5*2_"   or   "Rotate: 90_"   or   "Scale: -0.5_"
        AZStd::string GetStatusText() const
        {
            if (!m_active)
            {
                return {};
            }

            const char* modeStr = [this]
            {
                switch (m_mode)
                {
                case NumericOpMode::Move:    return "Move";
                case NumericOpMode::Rotate:  return "Rotate";
                case NumericOpMode::Scale:   return "Scale";
                case NumericOpMode::Extrude: return "Extrude";
                case NumericOpMode::Inset:   return "Inset";
                case NumericOpMode::Draw:    return "Depth";
                default:                     return "?";
                }
            }();

            const char* axisStr = [this]
            {
                switch (m_axis)
                {
                case NumericAxisConstraint::X: return " X";
                case NumericAxisConstraint::Y: return " Y";
                case NumericAxisConstraint::Z: return " Z";
                default:                       return "";
                }
            }();

            // Build the expression string
            AZStd::string expr = m_lhs;
            if (m_op != '\0')
            {
                expr += m_op;
                expr += m_rhs;
            }

            return AZStd::string::format("%s%s: %s_", modeStr, axisStr, expr.c_str());
        }

        // ------------------------------------------------------------------ //
        // State                                                               //
        // ------------------------------------------------------------------ //
        NumericOpMode         m_mode   = NumericOpMode::None;
        NumericAxisConstraint m_axis   = NumericAxisConstraint::Free;
        AZStd::string         m_lhs;          //!< First operand (left-hand side)
        char                  m_op    = '\0'; //!< Binary operator: '\0', '+', '-', '*', '/'
        AZStd::string         m_rhs;          //!< Second operand (right-hand side)
        bool                  m_active = false;

    private:
        AZStd::string& ActiveBuffer() { return (m_op != '\0') ? m_rhs : m_lhs; }
    };

} // namespace WhiteBox
