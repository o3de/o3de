/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
// Original file Copyright Crytek GMBH or its affiliates, used under license.

#ifndef CRYINCLUDE_EDITORCOMMON_SERIALIZATION_MEMORYWRITER_H
#define CRYINCLUDE_EDITORCOMMON_SERIALIZATION_MEMORYWRITER_H
#pragma once


#include <cstddef>
#include "Pointers.h"
#include "EditorCommonAPI.h"

namespace Serialization {
    class MemoryWriter
        : public RefCounter
    {
    public:
        MemoryWriter(std::size_t size = 128, bool reallocate = true);
        ~MemoryWriter();

        const char* c_str() { return memory_; };
        const wchar_t* w_str() { return (wchar_t*)memory_; };
        char* buffer() { return memory_; }
        const char* buffer() const { return memory_; }
        std::size_t size() const{ return size_; }
        void clear() { position_ = memory_; }

        // String interface (after this calls '\0' is always written)
        MemoryWriter& operator<<(int value);
        MemoryWriter& operator<<(long value);
        MemoryWriter& operator<<(unsigned long value);
        MemoryWriter& operator<<(unsigned int value);
        MemoryWriter& operator<<(long long value);
        MemoryWriter& operator<<(unsigned long long value);
        MemoryWriter& operator<<(float value) { return (*this) << double(value); }
        MemoryWriter& operator<<(double value);
        MemoryWriter& operator<<(signed char value);
        MemoryWriter& operator<<(unsigned char value);
        MemoryWriter& operator<<(char value);
        MemoryWriter& operator<<(const char* value);
        MemoryWriter& operator<<(const wchar_t* value);

        // Binary interface (does not writes trailing '\0')
        template<class T>
        void write(const T& value)
        {
            write(reinterpret_cast<const T*>(&value), sizeof(value));
        }
        void write(char c);
        void write(const char* str);
        bool write(const void* data, std::size_t size);

        std::size_t position() const { return position_ - memory_; }
        void setPosition(std::size_t pos);

        MemoryWriter& setDigits(int digits) { digits_ = (unsigned char)digits; return *this; }

    private:
        void allocate(std::size_t initialSize);
        void reallocate(std::size_t newSize);

        std::size_t size_;
        char* position_;
        char* memory_;
        bool reallocate_;
        unsigned char digits_;
    };
}

#endif // CRYINCLUDE_EDITORCOMMON_SERIALIZATION_MEMORYWRITER_H
