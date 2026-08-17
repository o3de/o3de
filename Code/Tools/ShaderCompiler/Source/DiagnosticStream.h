/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include "ReflectableEnums.h"

#include <iostream>

namespace AZ
{
    //! stream manipulator: new line and flush
    struct Endl {};

    //! stream manipulator: warning severity level
    MAKE_REFLECTABLE_ENUM(Warn,
        W0,     // Suppress all warnings
        W1,     // Display level 1 (severe) warnings [default]
        W2,     // Display level 1 & 2 warnings
        W3,     // Display level 1 & 2 & 3 warnings
        Wx,     // Treats any currently ativated warning, as error
        Wx1,    // Warnings up to level 1 are errors
        Wx2,    // Warnings up to level 2 are errors
        Wx3     // Warnings up to level 3 are errors
    );

    //! stream manipulator: stack up a severity level on a DiagnosticStream object (streams used for warnings have stateful warning levels, they can be backup-ed and restored thanks to this)
    //! you can use it by making an instance: e.g.: warningCout << PushLevel{} << Warn::W3 << "my warning level 3" << PopLevel{};
    struct PushLevel {};

    //! stream manipulator: pop a severity level on a DiagnosticStream object (restore the previous level before the push)
    struct PopLevel {};

    //! recover the level (as integer) value of an enumerator
    inline int ExtractLevel(Warn level)
    {
        return level == Warn::Wx ? -1
            : (level >= Warn::Wx1 ? level - Warn::Wx1 + 1
                : level);
    }

    struct DiagnosticStream
    {
        DiagnosticStream() : m_wrappedStream(std::cout)
        {}
        DiagnosticStream(decltype(std::cout)& streamToWrap) : m_wrappedStream(streamToWrap)
        {}

        typedef DiagnosticStream Self;

        template< typename Any >
        Self& operator<< (Any&& thing)
        {
            if (m_on && PassLevelFilter())
            {
                m_wrappedStream << std::forward<Any>(thing);
            }

            if (AsError() && m_onErrorCallback)
            {
                m_onErrorCallback(ConcatString("Warnings of level ", ExtractLevel(m_activeManipulator.top()), " are errors under current settings"));
            }
            return *this;
        }

        Self& operator<< (Warn::EnumType& level)
        {
            return operator<<(Warn::EnumType{ level });  // create an xvalue
        }

        Self& operator<< (Warn::EnumType&& level)
        {
            assert(level > Warn::W0); // silent warning messages make no sense.
            assert(level < Warn::Wx); // don't stream manipulators other than warning levels (error manipulators don't make sense)
            m_activeManipulator.top() = level;
            return *this;
        }

        Self& operator<< (PushLevel& level)
        {
            return operator<<(PushLevel{ level });
        }

        Self& operator<< (PushLevel&& level)
        {
            m_activeManipulator.push(m_activeManipulator.top());
            return *this;
        }

        Self& operator<< (PopLevel& level)
        {
            return operator<<(PopLevel{ level });
        }

        Self& operator<< (PopLevel&& level)
        {
            if (m_activeManipulator.size() > 1)
            {
                m_activeManipulator.pop();
            }
            return *this;
        }

        // non-templates functions have priority over templates, when the type matches.
        Self& operator<< (Endl&)
        {
            return operator<<(Endl{});  // pass an unnamed temporary, therefore an xvalue, therefore matching the function thereunder.
        }

        // Endl types here are not compatible with any << operator overload defined in basic_ostream. so we need to 'branch' out, using overloading.
        Self& operator<< (Endl&&)
        {
            if (m_on)
            {
                m_wrappedStream << std::endl;
            }
            return *this;
        }

        void SetRevealedWarningLevel(Warn level)
        {
            assert(level < Warn::Wx);
            m_warningLevel = level;
        }

        void SetAsErrorLevel(Warn level)
        {
            assert(level >= Warn::Wx);
            m_warningAsErrorLevel = level;
        }

    private:
        int CurrentErrorLevel() const
        {
            int asInt = ExtractLevel(m_warningAsErrorLevel);
            asInt = asInt == -1 ? ExtractLevel(m_warningLevel) : asInt;
            return m_warningAsErrorLevel == Warn::EndEnumeratorSentinel_ ? -1 : asInt;
        }

        // active level is an error according to settings
        bool AsError() const
        {
            return ExtractLevel(m_activeManipulator.top()) <= CurrentErrorLevel();
        }

        // current level should be visible
        bool PassLevelFilter() const
        {
            return AsError()  // errors must be displayed, even if the stream setting is stricter
                || ExtractLevel(m_activeManipulator.top()) <= ExtractLevel(m_warningLevel);
        }

    public:
        bool m_on = true;
        decltype(std::cout)& m_wrappedStream;
        std::function< void(string_view) > m_onErrorCallback;  //!< receive a message in case of a streamed element of a warning level enough to trigger an error

    private:
        Warn m_warningAsErrorLevel = Warn::EndEnumeratorSentinel_;  //!< no warning is an error by default
        Warn m_warningLevel = Warn::W1;                             //!< current activated level setting. default warning is W1
        stack<Warn> m_activeManipulator{ {Warn::W1} };              //!< store manipulators. start with an initial value corresponding to the default filter.
    };
}
