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

#ifndef CRYINCLUDE_CRYSYSTEM_HANDLERBASE_H
#define CRYINCLUDE_CRYSYSTEM_HANDLERBASE_H

#pragma once

const int MAX_CLIENTS_NUM = 100;

struct HandlerBase
{
    HandlerBase(const char* bucket, int affinity);
    ~HandlerBase();

    void SetAffinity();
    uint32 SyncSetAffinity(uint32 cpuMask);

    string m_serverLockName;
    string m_clientLockName;
    uint32 m_affinity;
    uint32 m_prevAffinity;
};

#endif // CRYINCLUDE_CRYSYSTEM_HANDLERBASE_H
