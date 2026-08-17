/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include "StreamableInterface.h"

namespace AZ
{
    //! Wraps and forwards data to a std::ostream,
    //! Overrides operator<< for char, const char * and string data and counts
    //! the number of '\n' (new line) characters that go through it.
    class NewLineCounterStream : public MakeOStreamStreamable
    {
        typedef NewLineCounterStream Self;

    public:

        using MakeOStreamStreamable::MakeOStreamStreamable;

        Self& operator<<(char c) override
        {
            if (c == '\n')
            {
                m_lineCount++;
            }
            m_wrappedStream << c;
            return *this;
        }

        Self& operator<<(const char * nts) override
        {
            const char * tmp = nts;
            while(char c = *tmp++)
            {
                if (c == '\n')
                {
                    m_lineCount++;
                }
            }
            m_wrappedStream << nts;
            return *this;
        }

        Self& operator<<(const string& str) override
        {
            for (char c : str)
            {
                if (c == '\n')
                {
                    m_lineCount++;
                }
            }
            m_wrappedStream << str;
            return *this;
        }

        template <class Any>
        Self& operator<<(Any&& thing)
        {
            m_wrappedStream << std::forward<Any>(thing);
            return *this;
        }

        int GetLineCount() const { return m_lineCount; }

    private:
        int m_lineCount = 0;
    };
}
