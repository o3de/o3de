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
#include "ChatPlay_precompiled.h"
#include "BroadcastCVars.h"

#define BROADCAST_DEFAULT_ENDPOINT "https://api.twitch.tv/kraken/"
#define BROADCAST_CLIENT_ID ""


/******************************************************************************/
// Actual implementation of BroadcastCVars is private to this translation unit

namespace
{
    class BroadcastCVarsImpl
        : public BroadcastCVars
    {
    public:
        BroadcastCVarsImpl();
        ~BroadcastCVarsImpl() override;

        void RegisterCVars() override;
        void UnregisterCVars() override;

    protected:
        // Stores a reference to each registered CVar for de-registration later
        std::list<ICVar*> m_vars;
    };
}

/******************************************************************************/
// Implementation of BroadcastCVars methods

std::shared_ptr<BroadcastCVars> BroadcastCVars::GetInstance()
{
    // Singleton is stored here as static local variable; this is the only
    // global/static reference held by the system.
    static std::weak_ptr<BroadcastCVarsImpl> instance;

    // Attempt to lock the current instance or, if that fails, create a new one
    std::shared_ptr<BroadcastCVarsImpl> ptr = instance.lock();
    if (!ptr)
    {
        ptr = std::make_shared<BroadcastCVarsImpl>();
        instance = ptr;
    }

    return ptr;
}


BroadcastCVars::BroadcastCVars()
    : m_broadcastEndpoint(BROADCAST_DEFAULT_ENDPOINT)
    , m_clientID(BROADCAST_CLIENT_ID)
{
}

/******************************************************************************/
// Implementation of BroadcastCVarsImpl methods

BroadcastCVarsImpl::BroadcastCVarsImpl()
{
}

BroadcastCVarsImpl::~BroadcastCVarsImpl()
{
}

void BroadcastCVarsImpl::RegisterCVars()
{
    ICVar* ptr;

    ptr = REGISTER_CVAR2("broadcast_Endpoint", &m_broadcastEndpoint, BROADCAST_DEFAULT_ENDPOINT, VF_NULL, "The base endpoint for BroadcastAPI.");
    m_vars.push_back(ptr);

    ptr = REGISTER_CVAR2("broadcast_ClientID", &m_clientID, BROADCAST_CLIENT_ID, VF_NULL, "The Client-ID to include in the request header.");
    m_vars.push_back(ptr);
}

void BroadcastCVarsImpl::UnregisterCVars()
{
    for (ICVar* v : m_vars)
    {
        string name = v->GetName();
        UNREGISTER_CVAR(name);
    }

    m_vars.clear();
}
