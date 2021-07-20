/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


// Description : Reference counted base object.


#ifndef CRYINCLUDE_EDITOR_UTIL_TREFCOUNTBASE_H
#define CRYINCLUDE_EDITOR_UTIL_TREFCOUNTBASE_H
#pragma once

AZ_PUSH_DISABLE_DLL_EXPORT_BASECLASS_WARNING
//////////////////////////////////////////////////////////////////////////
//! Derive from this class to get reference counting for your class.
//////////////////////////////////////////////////////////////////////////
template <class ParentClass>
class CRYEDIT_API TRefCountBase
    : public ParentClass
{
AZ_POP_DISABLE_DLL_EXPORT_BASECLASS_WARNING
public:
    TRefCountBase() { m_nRefCount = 0; };

    //! Add new refrence to this object.
    unsigned long AddRef()
    {
        m_nRefCount++;
        return m_nRefCount;
    };

    //! Release refrence to this object.
    //! when reference count reaches zero, object is deleted.
    unsigned long Release()
    {
        int refs = --m_nRefCount;
        if (m_nRefCount <= 0)
        {
            delete this;
        }
        return refs;
    }

protected:
    virtual ~TRefCountBase() {};

private:
    int m_nRefCount;
};


#endif // CRYINCLUDE_EDITOR_UTIL_TREFCOUNTBASE_H
