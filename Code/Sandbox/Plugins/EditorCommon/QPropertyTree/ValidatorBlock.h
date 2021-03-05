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

#include "Strings.h"
#include <Serialization/TypeID.h>
#include <vector>
#include <algorithm>

enum ValidatorEntryType
{
    VALIDATOR_ENTRY_WARNING,
    VALIDATOR_ENTRY_ERROR
};

struct ValidatorEntry
{
    const void* handle;
    Serialization::TypeID typeId;

    ValidatorEntryType type;
    string message;

    bool operator<(const ValidatorEntry& rhs) const
    {
        if (handle != rhs.handle)
        {
            return handle < rhs.handle;
        }
        return typeId < rhs.typeId;
    }

    ValidatorEntry(ValidatorEntryType type, const void* handle, const Serialization::TypeID& typeId, const char* message)
        : type(type)
        , handle(handle)
        , message(message)
        , typeId(typeId)
    {
    }

    ValidatorEntry()
        : handle()
        , type(VALIDATOR_ENTRY_WARNING)
    {
    }
};
typedef std::vector<ValidatorEntry> ValidatorEntries;

class ValidatorBlock
{
public:
    ValidatorBlock()
        : m_enabled(false)
    {
    }

    void Clear()
    {
        m_entries.clear();
        m_used.clear();
    }

    void AddEntry(const ValidatorEntry& entry)
    {
        ValidatorEntries::iterator it = std::upper_bound(m_entries.begin(), m_entries.end(), entry);
        m_used.insert(m_used.begin() + (it - m_entries.begin()), false);
        m_entries.insert(it, entry);
        m_enabled = true;
    }

    bool IsEnabled() const { return m_enabled; }

    const ValidatorEntry* GetEntry(int index, int count) const
    {
        if (size_t(index) >= m_entries.size())
        {
            return 0;
        }
        if (size_t(index + count) > m_entries.size())
        {
            return 0;
        }
        return &m_entries[index];
    }

    bool FindHandleEntries(int* outIndex, int* outCount, const void* handle, Serialization::TypeID& typeId)
    {
        if (handle == 0)
        {
            return false;
        }
        ValidatorEntry e;
        e.handle = handle;
        e.typeId = typeId;
        ValidatorEntries::iterator begin = std::lower_bound(m_entries.begin(), m_entries.end(), e);
        ValidatorEntries::iterator end = std::upper_bound(m_entries.begin(), m_entries.end(), e);
        if (begin != end)
        {
            *outIndex = int(begin - m_entries.begin());
            *outCount = int(end - begin);
            return true;
        }
        return false;
    }

    void MarkAsUsed(int start, int count)
    {
        if (start < 0)
        {
            return;
        }
        if (start + count > m_entries.size())
        {
            return;
        }
        for (int i = start; i < start + count; ++i)
        {
            m_used[i] = true;
        }
    }

    void MergeUnusedItemsWithRootItems(int* firstUnusedItem, int* count, const void* newHandle, Serialization::TypeID& typeId)
    {
        size_t numItems = m_used.size();
        for (size_t i = 0; i < numItems; ++i)
        {
            if (m_entries[i].handle == newHandle)
            {
                m_entries.push_back(m_entries[i]);
                m_used.push_back(true);
                m_entries[i].typeId = Serialization::TypeID();
            }
            if (!m_used[i])
            {
                m_entries.push_back(m_entries[i]);
                m_used.push_back(true);
                m_entries.back().handle = newHandle;
                m_entries.back().typeId = typeId;
            }
        }
        *firstUnusedItem = (int)numItems;
        *count = int(m_entries.size() - numItems);
    }

    bool ContainsErrors() const
    {
        for (size_t i = 0; i < m_entries.size(); ++i)
        {
            if (m_entries[i].type == VALIDATOR_ENTRY_ERROR)
            {
                return true;
            }
        }
        return false;
    }

private:
    ValidatorEntries m_entries;
    std::vector<bool> m_used;
    bool m_enabled;
};
