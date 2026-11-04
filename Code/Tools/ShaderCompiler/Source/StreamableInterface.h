/*
* Copyright (c) Contributors to the Open 3D Engine Project.
* For complete copyright and license terms please see the LICENSE at the root of this distribution.
* 
* SPDX-License-Identifier: Apache-2.0 OR MIT
*
*/
#pragma once

#include <ostream>

namespace AZ
{
    //! This class aims at being an abstract base to replace std::ostream which is not deriveable.
    //! With this facility, stream operator << becomes virtualizable, and emitters/reflectors may stream to
    //! various types of special streaming service, notably the "linefeed counter" you'll find in NewLineCounterStream.h
    class Streamable
    {
    public:
        virtual Streamable& operator<<(char) = 0;    // template virtual method are forbidden in C++, so manual listing :(
        virtual Streamable& operator<<(const char*) = 0;
        virtual Streamable& operator<<(double) = 0;
        virtual Streamable& operator<<(int64_t) = 0;
        virtual Streamable& operator<<(uint32_t) = 0;
        virtual Streamable& operator<<(size_t) = 0;
        virtual Streamable& operator<<(bool) = 0;
        virtual Streamable& operator<<(const std::string&) = 0;
    };

    // trivial concrete version to wrap classic std::ostream objects
    class MakeOStreamStreamable : public Streamable
    {
    public:
        MakeOStreamStreamable(std::ostream& streamToWrap)
            : m_wrappedStream(streamToWrap)
        {}

        Streamable& operator<<(char c) override                 { m_wrappedStream << c; return *this; }
        Streamable& operator<<(const char* nts) override        { m_wrappedStream << nts; return *this; }
        Streamable& operator<<(const std::string& str) override { m_wrappedStream << str; return *this; }
        Streamable& operator<<(double n) override               { m_wrappedStream << n; return *this; }
        Streamable& operator<<(int64_t n) override              { m_wrappedStream << n; return *this; }
        Streamable& operator<<(uint32_t n) override             { m_wrappedStream << n; return *this; }
        Streamable& operator<<(size_t n) override               { m_wrappedStream << n; return *this; }
        Streamable& operator<<(bool b) override                 { m_wrappedStream << b; return *this; }

    protected:
        std::ostream& m_wrappedStream;
    };

}
