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
