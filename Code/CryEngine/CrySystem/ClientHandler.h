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

#ifndef CRYINCLUDE_CRYSYSTEM_CLIENTHANDLER_H
#define CRYINCLUDE_CRYSYSTEM_CLIENTHANDLER_H
#pragma once

#include "HandlerBase.h"
#include "SyncLock.h"

struct ClientHandler
    : public HandlerBase
{
    ClientHandler(const char* bucket, int affinity, int clientTimeout);

    void Reset();
    bool ServerIsValid();
    bool Sync();

private:
    int m_clientTimeout;
    std::unique_ptr<SSyncLock> m_clientLock;
    std::unique_ptr<SSyncLock> m_srvLock;
};

#endif
