/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


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
