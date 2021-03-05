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

#include "CrySystem_precompiled.h"

#include "ProjectDefines.h"
#if defined(MAP_LOADING_SLICING)

#include "ClientHandler.h"

ClientHandler::ClientHandler(const char* bucket, int affinity, int clientTimeout)
    : HandlerBase(bucket, affinity)
{
    m_clientTimeout = clientTimeout;
    Reset();
}

void ClientHandler::Reset()
{
    m_srvLock.reset(0);
    for (int i = 0; i < MAX_CLIENTS_NUM; i++)
    {
        std::unique_ptr<SSyncLock> srv(new SSyncLock(m_serverLockName, i, false));

        // first get the client lock up!
        if (!srv->IsValid())
        {
            //try to create client lock
            m_clientLock.reset(new SSyncLock(m_clientLockName, i, true));
            if (m_clientLock->IsValid())
            {
                break;
            }
            else
            {
                m_clientLock.reset(0);
            }
        }
    }
}

bool ClientHandler::ServerIsValid()
{
    if (!m_srvLock.get())
    {
        if (m_clientLock.get() && m_clientLock->IsValid())
        {
            m_srvLock.reset(new SSyncLock(m_serverLockName, m_clientLock->number, false));
            if (m_srvLock->IsValid())
            {
                SetAffinity();
                //got synched
                return true;
            }
            m_srvLock.reset(0);
        }
        return false;
    }
    return m_srvLock->IsValid();
}

bool ClientHandler::Sync()
{
    if (ServerIsValid())
    {
        m_clientLock->Signal();//signal that we're done and
        if (m_srvLock->Wait(m_clientTimeout))//wait for server
        {
            //bla bla, track waiting
            return true;
        }
        else
        {
            Reset();
        }
    }
    return false;
}

#endif // defined(MAP_LOADING_SLICING)
