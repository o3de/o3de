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

// Description : Reference counted base object.


#ifndef CRYINCLUDE_EDITOR_UTIL_REFCOUNTBASE_H
#define CRYINCLUDE_EDITOR_UTIL_REFCOUNTBASE_H
#pragma once

#include <Include/EditorCoreAPI.h>

//! Derive from this class to get reference counting in your class.
class EDITOR_CORE_API CRefCountBase
{
public:
    CRefCountBase() {}

    //! Add a new reference to this object.
    int AddRef()
    {
        m_nRefCount++;
        return m_nRefCount;
    }

    //! Release reference to this object.
    //! When the reference count reaches zero, the object is deleted.
    int Release()
    {
        int refs = --m_nRefCount;
        if (m_nRefCount == 0)
        {
            delete this;
        }
        else if (m_nRefCount < 0)
        {
            CryFatalError("Negative ref count");
        }
        return refs;
    }

protected:
    virtual ~CRefCountBase() {}

private:
    int m_nRefCount = 0;
};

#endif // CRYINCLUDE_EDITOR_UTIL_REFCOUNTBASE_H
