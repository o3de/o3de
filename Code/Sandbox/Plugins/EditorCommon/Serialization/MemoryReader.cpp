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

#include "EditorCommon_precompiled.h"
#include <platform.h>
#include "Serialization/Assert.h"
#include "MemoryReader.h"
#include <stdlib.h>
#include <memory.h>

namespace Serialization {
    MemoryReader::MemoryReader()
        : size_(0)
        , position_(0)
        , memory_(0)
        , ownedMemory_(false)
    {
    }


    MemoryReader::MemoryReader(const void* memory, std::size_t size, bool ownAndFree)
        : size_(size)
        , position_((const char*)(memory))
        , memory_((const char*)(memory))
        , ownedMemory_(ownAndFree)
    {
    }

    MemoryReader::~MemoryReader()
    {
        if (ownedMemory_)
        {
            free(const_cast<char*>(memory_));
            memory_ = 0;
            size_ = 0;
        }
    }

    void MemoryReader::setPosition(const char* position)
    {
        position_ = position;
    }

    void MemoryReader::read(void* data, std::size_t size)
    {
        YASLI_ASSERT(memory_ && position_);
        YASLI_ASSERT(position_ - memory_ + size <= size_);
        memcpy(data, position_, size);
        position_ += size;
    }

    bool MemoryReader::checkedRead(void* data, std::size_t size)
    {
        if (!memory_ || !position_)
        {
            return false;
        }
        if (position_ - memory_ + size > size_)
        {
            return false;
        }

        memcpy(data, position_, size);
        position_ += size;
        return true;
    }

    bool MemoryReader::checkedSkip(std::size_t size)
    {
        if (!memory_ || !position_)
        {
            return false;
        }
        if (position_ - memory_ + size > size_)
        {
            return false;
        }

        position_ += size;
        return true;
    }
}
