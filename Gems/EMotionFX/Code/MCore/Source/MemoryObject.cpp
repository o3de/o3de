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
        mReferenceCount.SetValue(1);
    }


    // destructor
    MemoryObject::~MemoryObject()
    {
        MCORE_ASSERT(mReferenceCount.GetValue() == 0);
    }


    // increase the reference count
    void MemoryObject::IncreaseReferenceCount()
    {
        mReferenceCount.Increment();
    }


    // decrease the reference count
    void MemoryObject::DecreaseReferenceCount()
    {
        MCORE_ASSERT(mReferenceCount.GetValue() > 0);
        mReferenceCount.Decrement();
    }


    // destroy the object
    void MemoryObject::Destroy()
    {
        MCORE_ASSERT(mReferenceCount.GetValue() > 0);
        if (mReferenceCount.Decrement() == 1) 
        {
            Delete();
        }
    }


    // get the reference count
    uint32 MemoryObject::GetReferenceCount() const
    {
        return mReferenceCount.GetValue();
    }


    // delete it
    void MemoryObject::Delete()
    {
        delete this;
    }
}   // namespace MCore
