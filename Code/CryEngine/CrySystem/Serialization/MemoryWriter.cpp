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

#include "CrySystem_precompiled.h"
#include <platform.h>
#include "Serialization/Assert.h"
#include <stdio.h>
#include <stdlib.h>
#include <cstring>
#include <math.h>
#ifdef _MSC_VER
# include <float.h>
# define isnan _isnan
#endif

#include "MemoryWriter.h"

#undef YASLI_ASSERT
#define YASLI_ASSERT(x)

namespace Serialization {
    MemoryWriter::MemoryWriter(std::size_t size, bool reallocate)
        : size_(size)
        , reallocate_(reallocate)
        , digits_(5)
    {
        allocate(size);
    }

    MemoryWriter::~MemoryWriter()
    {
        position_ = 0;
        CryModuleFree(memory_);
    }

    void MemoryWriter::allocate(std::size_t initialSize)
    {
        memory_ = (char*)CryModuleMalloc(initialSize + 1);
        position_ = memory_;
    }

    void MemoryWriter::reallocate(std::size_t newSize)
    {
        YASLI_ASSERT(newSize > size_);
        std::size_t pos = position();
        // Supressing the warning as we generally don't handle malloc errors.
        // cppcheck-suppress memleakOnRealloc
        memory_ = (char*)CryModuleRealloc(memory_, newSize + 1);
        YASLI_ASSERT(memory_ != 0);
        position_ = memory_ + pos;
        size_ = newSize;
    }

    MemoryWriter& MemoryWriter::operator<<(int value)
    {
        // TODO: optimize
        char buffer[12];
        sprintf_s(buffer, "%i", value);
        return operator<<((const char*)buffer);
    }

    MemoryWriter& MemoryWriter::operator<<(long value)
    {
        // TODO: optimize
        char buffer[12];
#ifdef _MSC_VER
        sprintf_s(buffer, "%i", value);
#else
        sprintf_s(buffer, "%li", value);
#endif
        return operator<<((const char*)buffer);
    }

    MemoryWriter& MemoryWriter::operator<<(unsigned long value)
    {
        // TODO: optimize
        char buffer[12];
#ifdef _MSC_VER
        sprintf_s(buffer, "%u", value);
#else
        sprintf_s(buffer, "%lu", value);
#endif
        return operator<<((const char*)buffer);
    }

    MemoryWriter& MemoryWriter::operator<<(long long value)
    {
        // TODO: optimize
        char buffer[24];
#ifdef _MSC_VER
        sprintf_s(buffer, "%I64i", value);
#else
        sprintf_s(buffer, "%lli", value);
#endif
        return operator<<((const char*)buffer);
    }

    MemoryWriter& MemoryWriter::operator<<(unsigned long long value)
    {
        // TODO: optimize
        char buffer[24];
        sprintf_s(buffer, "%llu", value);
        return operator<<((const char*)buffer);
    }

    MemoryWriter& MemoryWriter::operator<<(unsigned int value)
    {
        // TODO: optimize
        char buffer[12];
        sprintf_s(buffer, "%u", value);
        return operator<<((const char*)buffer);
    }

    MemoryWriter& MemoryWriter::operator<<(char value)
    {
        char buffer[12];
        sprintf_s(buffer, "%i", int(value));
        return operator<<((const char*)buffer);
    }

    MemoryWriter& MemoryWriter::operator<<(unsigned char value)
    {
        char buffer[12];
        sprintf_s(buffer, "%i", int(value));
        return operator<<((const char*)buffer);
    }

    MemoryWriter& MemoryWriter::operator<<(signed char value)
    {
        char buffer[12];
        sprintf_s(buffer, "%i", int(value));
        return operator<<((const char*)buffer);
    }

    inline void cutRightZeros(const char* str)
    {
        for (char* p = (char*)str + strlen(str) - 1; p >= str; --p)
        {
            if (*p == '0')
            {
                *p = 0;
            }
            else
            {
                return;
            }
        }
    }

    MemoryWriter& MemoryWriter::operator<<(double value)
    {
        YASLI_ASSERT(!isnan(value));

        char buf[64] = { 0 };
        sprintf_s(buf, "%f", value);
        operator<<(buf);
        return *this;
    }

    MemoryWriter& MemoryWriter::operator<<(const char* value)
    {
        write((void*)value, strlen(value));
        YASLI_ASSERT(position() < size());
        *position_ = '\0';
        return *this;
    }

    MemoryWriter& MemoryWriter::operator<<(const wchar_t* value)
    {
        write((void*)value, wcslen(value) * sizeof(wchar_t));
        YASLI_ASSERT(position() < size());
        *position_ = '\0';
        return *this;
    }

    void MemoryWriter::setPosition(std::size_t pos)
    {
        YASLI_ASSERT(pos < size_);
        YASLI_ASSERT(memory_ + pos <= position_);
        position_ = memory_ + pos;
    }

    void MemoryWriter::write(const char* value)
    {
        write((void*)value, strlen(value));
    }

    bool MemoryWriter::write(const void* data, std::size_t size)
    {
        YASLI_ASSERT(memory_ <= position_);
        YASLI_ASSERT(position() < this->size());
        if (size_ - position() > size)
        {
            memcpy(position_, data, size);
            position_ += size;
        }
        else
        {
            if (!reallocate_)
            {
                return false;
            }

            reallocate(size_ * 2);
            write(data, size);
        }
        YASLI_ASSERT(position() < this->size());
        return true;
    }

    void MemoryWriter::write(char c)
    {
        if (size_ - position() > 1)
        {
            *(char*)(position_) = c;
            ++position_;
        }
        else
        {
            YASLI_ESCAPE(reallocate_, return );
            reallocate(size_ * 2);
            write(c);
        }
        YASLI_ASSERT(position() < this->size());
    }
}
