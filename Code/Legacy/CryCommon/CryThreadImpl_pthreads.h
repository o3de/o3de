/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#ifndef CRYINCLUDE_CRYCOMMON_CRYTHREADIMPL_PTHREADS_H
#define CRYINCLUDE_CRYCOMMON_CRYTHREADIMPL_PTHREADS_H
#pragma once


#include "CryThread_pthreads.h"

///////////////////////////////////////////////////////////////////////////////
// CryCriticalSection implementation
///////////////////////////////////////////////////////////////////////////////
typedef CryLockT<CRYLOCK_RECURSIVE> TCritSecType;

void  CryDeleteCriticalSection(void* cs)
{
    delete ((TCritSecType*)cs);
}

void  CryEnterCriticalSection(void* cs)
{
    ((TCritSecType*)cs)->Lock();
}

bool  CryTryCriticalSection(void* cs)
{
    return false;
}

void  CryLeaveCriticalSection(void* cs)
{
    ((TCritSecType*)cs)->Unlock();
}

void  CryCreateCriticalSectionInplace(void* pCS)
{
    new (pCS) TCritSecType;
}

void CryDeleteCriticalSectionInplace(void*)
{
}

void* CryCreateCriticalSection()
{
    return (void*) new TCritSecType;
}

#endif // CRYINCLUDE_CRYCOMMON_CRYTHREADIMPL_PTHREADS_H
