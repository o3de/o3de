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

#pragma once

#include <cstddef>

namespace Serialization {
    class MemoryReader
    {
    public:

        MemoryReader();
        MemoryReader(const void* memory, size_t size, bool ownAndFree = false);
        ~MemoryReader();

        void setPosition(const char* position);
        const char* position(){ return position_; }

        template<class T>
        void read(T& value)
        {
            read(reinterpret_cast<void*>(&value), sizoef(value));
        }
        void read(void* data, size_t size);
        bool checkedSkip(size_t size);
        bool checkedRead(void* data, size_t size);
        template<class T>
        bool checkedRead(T& t)
        {
            return checkedRead((void*)&t, sizeof(t));
        }

        const char* buffer() const{ return memory_; }
        size_t size() const{ return size_; }

        const char* begin() const{ return memory_; }
        const char* end() const{ return memory_ + size_; }
    private:
        size_t size_;
        const char* position_;
        const char* memory_;
        bool ownedMemory_;
    };
}
// vim:ts=4 sw=4:
