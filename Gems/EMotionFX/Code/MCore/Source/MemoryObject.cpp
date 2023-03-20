/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

// include required headers
#include "MemoryObject.h"


namespace MCore
{
    // destroy a memory object
    void Destroy(MemoryObject* object)
    {
        if (object)
        {
            object->Destroy();
        }
    }


    // constructor
    MemoryObject::MemoryObject()
    {
        m_referenceCount.SetValue(1);
    }


    // destructor
    MemoryObject::~MemoryObject()
    {
        MCORE_ASSERT(m_referenceCount.GetValue() == 0);
    }


    // increase the reference count
    void MemoryObject::IncreaseReferenceCount()
    {
        m_referenceCount.Increment();
    }


    // decrease the reference count
    void MemoryObject::DecreaseReferenceCount()
    {
        MCORE_ASSERT(m_referenceCount.GetValue() > 0);
        m_referenceCount.Decrement();
    }


    // destroy the object
    void MemoryObject::Destroy()
    {
        MCORE_ASSERT(m_referenceCount.GetValue() > 0);
        if (m_referenceCount.Decrement() == 1) 
        {
            Delete();
        }
    }


    // get the reference count
    uint32 MemoryObject::GetReferenceCount() const
    {
        return m_referenceCount.GetValue();
    }


    // delete it
    void MemoryObject::Delete()
    {
        delete this;
    }
}   // namespace MCore
