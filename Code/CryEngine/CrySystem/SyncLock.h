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

#ifndef CRYINCLUDE_CRYSYSTEM_SYNCLOCK_H
#define CRYINCLUDE_CRYSYSTEM_SYNCLOCK_H

#pragma once

#if defined(LINUX) || defined(APPLE)
#include <semaphore.h>
#endif

struct SSyncLock
{
#if defined(LINUX) || defined(APPLE)
    typedef sem_t* HandleType;
#else
    typedef HANDLE HandleType;
#endif

    SSyncLock(const char* name, int id, bool own);
    SSyncLock(const char* name, int minId, int maxId);
    ~SSyncLock();

    void Own(const char* name);
    bool Open(const char* name);
    bool Create(const char* name);
    void Signal();
    bool Wait(int ms);
    void Close();
    bool IsValid() const;

    HandleType ev;
    int number;
    string o_name;
};

#endif // CRYINCLUDE_CRYSYSTEM_SYNCLOCK_H
