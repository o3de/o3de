/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

// include required headers
#include "RefCounted.h"

namespace MCore
{
    // destroy a memory object
    void Destroy(RefCounted* object)
    {
        if (object)
        {
            object->Destroy();
        }
    }

    // constructor
    RefCounted::RefCounted()
    {
        m_referenceCount.SetValue(1);
    }

    // destructor
    RefCounted::~RefCounted()
    {
        MCORE_ASSERT(m_referenceCount.GetValue() == 0);
    }

    // increase the reference count
    void RefCounted::IncreaseReferenceCount()
    {
        m_referenceCount.Increment();
    }

    // decrease the reference count
    void RefCounted::DecreaseReferenceCount()
    {
        MCORE_ASSERT(m_referenceCount.GetValue() > 0);
        m_referenceCount.Decrement();
    }

    // destroy the object
    void RefCounted::Destroy()
    {
        MCORE_ASSERT(m_referenceCount.GetValue() > 0);
        if (m_referenceCount.Decrement() == 1) 
        {
            Delete();
        }
    }

    // get the reference count
    uint32 RefCounted::GetReferenceCount() const
    {
        return m_referenceCount.GetValue();
    }

    // delete it
    void RefCounted::Delete()
    {
        delete this;
    }
} // namespace MCore
