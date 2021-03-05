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

#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <AzCore/std/smart_ptr/weak_ptr.h>

#include "JoinInCVars.h"

#define JOININ_URI_SCHEME "game"


/******************************************************************************/
// Actual implementation of JoinInCVars is private to this translation unit

namespace
{
    class JoinInCVarsImpl
        : public JoinInCVars
    {
    public:
        JoinInCVarsImpl();
        ~JoinInCVarsImpl() override;

        void RegisterCVars() override;
        void UnregisterCVars() override;

    protected:
        // Stores a reference to each registered CVar for de-registration later
        std::list<ICVar*> m_vars;
    };
}

/******************************************************************************/
// Implementation of JoinInCVars methods

AZStd::shared_ptr<JoinInCVars> JoinInCVars::GetInstance()
{
    // Singleton is stored here as static local variable; this is the only
    // global/static reference held by the system.
    static AZStd::weak_ptr<JoinInCVarsImpl> instance;

    // Attempt to lock the current instance or, if that fails, create a new one
    AZStd::shared_ptr<JoinInCVarsImpl> ptr = instance.lock();
    if (!ptr)
    {
        ptr = AZStd::make_shared<JoinInCVarsImpl>();
        instance = ptr;
    }

    return ptr;
}


JoinInCVars::JoinInCVars()
    : m_uriScheme(JOININ_URI_SCHEME)
{
}

/******************************************************************************/
// Implementation of JoinInCVarsImpl methods

JoinInCVarsImpl::JoinInCVarsImpl()
{
}

JoinInCVarsImpl::~JoinInCVarsImpl()
{
}

void JoinInCVarsImpl::RegisterCVars()
{
    ICVar* ptr;

    ptr = REGISTER_CVAR2("joinin_uriScheme", &m_uriScheme, JOININ_URI_SCHEME, VF_NULL, "The URI scheme for JoinIn link creation");
    m_vars.push_back(ptr);
}

void JoinInCVarsImpl::UnregisterCVars()
{
    for (ICVar* v : m_vars)
    {
        string name = v->GetName();
        UNREGISTER_CVAR(name);
    }

    m_vars.clear();
}
