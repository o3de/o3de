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

#ifndef CRYINCLUDE_EDITOR_INCLUDE_ICOMMANDMANAGER_H
#define CRYINCLUDE_EDITOR_INCLUDE_ICOMMANDMANAGER_H
#pragma once
#include "Command.h"
#include "functor.h"

typedef void (* TPfnDeleter)(void*);

class ICommandManager
{
public:
    virtual ~ICommandManager() = default;
    
    virtual bool AddCommand(CCommand* pCommand, TPfnDeleter deleter = nullptr) = 0;
    virtual bool UnregisterCommand(const char* module, const char* name) = 0;
    virtual bool AttachUIInfo(const char* fullCmdName, const CCommand0::SUIInfo& uiInfo) = 0;
    virtual bool IsRegistered(const char* module, const char* name) const = 0;
    virtual bool IsRegistered(const char* cmdLine) const = 0;
    virtual bool IsRegistered(int commandId) const = 0;
};

/// A set of helper template methods for an easy registration of commands
namespace CommandManagerHelper
{
    bool RegisterCommand(ICommandManager* pCmdMgr, const char* module, const char* name,
        const char* description, const char* example,
        const Functor0& functor);
    template <typename RT>
    bool RegisterCommand(ICommandManager* pCmdMgr, const char* module, const char* name,
        const char* description, const char* example,
        const Functor0wRet<RT>& functor);
    template <LIST(1, typename P)>
    bool RegisterCommand(ICommandManager* pCmdMgr, const char* module, const char* name,
        const char* description, const char* example,
        const Functor1<LIST(1, P)>& functor);
    template <LIST(1, typename P), typename RT>
    bool RegisterCommand(ICommandManager* pCmdMgr, const char* module, const char* name,
        const char* description, const char* example,
        const Functor1wRet<LIST(1, P), RT>& functor);
    template <LIST(2, typename P)>
    bool RegisterCommand(ICommandManager* pCmdMgr, const char* module, const char* name,
        const char* description, const char* example,
        const Functor2<LIST(2, P)>& functor);
    template <LIST(2, typename P), typename RT>
    bool RegisterCommand(ICommandManager* pCmdMgr, const char* module, const char* name,
        const char* description, const char* example,
        const Functor2wRet<LIST(2, P), RT>& functor);
    template <LIST(3, typename P)>
    bool RegisterCommand(ICommandManager* pCmdMgr, const char* module, const char* name,
        const char* description, const char* example,
        const Functor3<LIST(3, P)>& functor);
    template <LIST(3, typename P), typename RT>
    bool RegisterCommand(ICommandManager* pCmdMgr, const char* module, const char* name,
        const char* description, const char* example,
        const Functor3wRet<LIST(3, P), RT>& functor);
    template <LIST(4, typename P)>
    bool RegisterCommand(ICommandManager* pCmdMgr, const char* module, const char* name,
        const char* description, const char* example,
        const Functor4<LIST(4, P)>& functor);
    template <LIST(4, typename P), typename RT>
    bool RegisterCommand(ICommandManager* pCmdMgr, const char* module, const char* name,
        const char* description, const char* example,
        const Functor4wRet<LIST(4, P), RT>& functor);
    template <LIST(5, typename P)>
    bool RegisterCommand(ICommandManager* pCmdMgr, const char* module, const char* name,
        const char* description, const char* example,
        const Functor5<LIST(5, P)>& functor);
    template <LIST(6, typename P)>
    bool RegisterCommand(ICommandManager* pCmdMgr, const char* module, const char* name,
        const char* description, const char* example,
        const Functor6<LIST(6, P)>& functor);

    namespace Private
    {
        template <typename FunctorType, typename CommandType>
        bool RegisterCommand(ICommandManager* pCmdMgr, const char* module, const char* name,
            const char* description, const char* example,
            const FunctorType& functor);
    }
};

template <typename FunctorType, typename CommandType>
bool CommandManagerHelper::Private::RegisterCommand(ICommandManager* pCmdMgr,
    const char* module, const char* name,
    const char* description, const char* example,
    const FunctorType& functor)
{
    assert(functor);
    if (!functor)
    {
        return false;
    }

    CommandType* pCommand
        = new CommandType(module, name, description, example, functor);
    if (pCmdMgr->AddCommand(pCommand) == false)
    {
        delete pCommand;
        return false;
    }
    return true;
}

inline
bool CommandManagerHelper::RegisterCommand(ICommandManager* pCmdMgr, const char* module, const char* name,
    const char* description, const char* example,
    const Functor0& functor)
{
    return Private::RegisterCommand<Functor0, CCommand0>(pCmdMgr, module, name, description, example, functor);
}

template <typename RT>
bool CommandManagerHelper::RegisterCommand(ICommandManager* pCmdMgr, const char* module, const char* name,
    const char* description, const char* example,
    const Functor0wRet<RT>& functor)
{
    return Private::RegisterCommand<Functor0wRet<RT>, CCommand0wRet<RT> >(pCmdMgr, module, name, description, example, functor);
}

template <LIST(1, typename P)>
bool CommandManagerHelper::RegisterCommand(ICommandManager* pCmdMgr, const char* module, const char* name,
    const char* description, const char* example,
    const Functor1<LIST(1, P)>& functor)
{
    return Private::RegisterCommand<Functor1<LIST(1, P)>, CCommand1<LIST(1, P)> >(pCmdMgr, module, name, description, example, functor);
}

template <LIST(1, typename P), typename RT>
bool CommandManagerHelper::RegisterCommand(ICommandManager* pCmdMgr, const char* module, const char* name,
    const char* description, const char* example,
    const Functor1wRet<LIST(1, P), RT>& functor)
{
    return Private::RegisterCommand<Functor1wRet<LIST(1, P), RT>, CCommand1wRet<LIST(1, P), RT> >(pCmdMgr, module, name, description, example, functor);
}

template <LIST(2, typename P)>
bool CommandManagerHelper::RegisterCommand(ICommandManager* pCmdMgr, const char* module, const char* name,
    const char* description, const char* example,
    const Functor2<LIST(2, P)>& functor)
{
    return Private::RegisterCommand<Functor2<LIST(2, P)>, CCommand2<LIST(2, P)> >(pCmdMgr, module, name, description, example, functor);
}

template <LIST(2, typename P), typename RT>
bool CommandManagerHelper::RegisterCommand(ICommandManager* pCmdMgr, const char* module, const char* name,
    const char* description, const char* example,
    const Functor2wRet<LIST(2, P), RT>& functor)
{
    return Private::RegisterCommand<Functor2wRet<LIST(2, P), RT>, CCommand2wRet<LIST(2, P), RT> >(pCmdMgr, module, name, description, example, functor);
}

template <LIST(3, typename P)>
bool CommandManagerHelper::RegisterCommand(ICommandManager* pCmdMgr, const char* module, const char* name,
    const char* description, const char* example,
    const Functor3<LIST(3, P)>& functor)
{
    return Private::RegisterCommand<Functor3<LIST(3, P)>, CCommand3<LIST(3, P)> >(pCmdMgr, module, name, description, example, functor);
}

template <LIST(3, typename P), typename RT>
bool CommandManagerHelper::RegisterCommand(ICommandManager* pCmdMgr, const char* module, const char* name,
    const char* description, const char* example,
    const Functor3wRet<LIST(3, P), RT>& functor)
{
    return Private::RegisterCommand<Functor3wRet<LIST(3, P), RT>, CCommand3wRet<LIST(3, P), RT> >(pCmdMgr, module, name, description, example, functor);
}

template <LIST(4, typename P)>
bool CommandManagerHelper::RegisterCommand(ICommandManager* pCmdMgr, const char* module, const char* name,
    const char* description, const char* example,
    const Functor4<LIST(4, P)>& functor)
{
    return Private::RegisterCommand<Functor4<LIST(4, P)>, CCommand4<LIST(4, P)> >(pCmdMgr, module, name, description, example, functor);
}

template <LIST(4, typename P), typename RT>
bool CommandManagerHelper::RegisterCommand(ICommandManager* pCmdMgr, const char* module, const char* name,
    const char* description, const char* example,
    const Functor4wRet<LIST(4, P), RT>& functor)
{
    return Private::RegisterCommand<Functor4wRet<LIST(4, P), RT>, CCommand4wRet<LIST(4, P), RT> >(pCmdMgr, module, name, description, example, functor);
}

template <LIST(5, typename P)>
bool CommandManagerHelper::RegisterCommand(ICommandManager* pCmdMgr, const char* module, const char* name,
    const char* description, const char* example,
    const Functor5<LIST(5, P)>& functor)
{
    return Private::RegisterCommand<Functor5<LIST(5, P)>, CCommand5<LIST(5, P)> >(pCmdMgr, module, name, description, example, functor);
}

template <LIST(6, typename P)>
bool CommandManagerHelper::RegisterCommand(ICommandManager* pCmdMgr, const char* module, const char* name,
    const char* description, const char* example,
    const Functor6<LIST(6, P)>& functor)
{
    return Private::RegisterCommand<Functor6<LIST(6, P)>, CCommand6<LIST(6, P)> >(pCmdMgr, module, name, description, example, functor);
}
#endif // CRYINCLUDE_EDITOR_INCLUDE_ICOMMANDMANAGER_H
