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

// Description : A wrapper that counts the number of times the wrapped object
//               has been set  This is useful for netserializing an object
//               that might be given a  new value  that s the same as the old value


#ifndef CRYINCLUDE_CRYCOMMON_COUNTEDVALUE_H
#define CRYINCLUDE_CRYCOMMON_COUNTEDVALUE_H
#pragma once


template <typename T>
struct CountedValue
{
public:
    CountedValue()
        : m_lastProducedId(0)
        , m_lastConsumedId(0) {}

    typedef uint32 TCountedID;

    void SetAndDirty(const T& value)
    {
        m_value = value;
        ++m_lastProducedId;
        CRY_ASSERT(m_lastProducedId > 0);
    }

    const T* GetLatestValue()
    {
        bool bHasNewValue = IsDirty(); // check for dirtiness before updating ids
        m_lastConsumedId = m_lastProducedId;

        return bHasNewValue ? &m_value : NULL;
    }

    inline bool IsDirty() const
    {
        return m_lastProducedId != m_lastConsumedId;
    }

    const T& Peek() const
    {
        return m_value;
    }

    TCountedID GetLatestID() const
    {
        return m_lastProducedId;
    }

    // This method should only be used to update the object during serialization!
    void UpdateDuringSerializationOnly(const T& value, TCountedID lastProducedId)
    {
        m_value = value;
        m_lastProducedId = lastProducedId;
    }

private:
    TCountedID m_lastProducedId;
    TCountedID m_lastConsumedId;
    T m_value;
};

#endif // CRYINCLUDE_CRYCOMMON_COUNTEDVALUE_H
