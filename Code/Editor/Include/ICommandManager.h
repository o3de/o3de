/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#ifndef CRYINCLUDE_EDITOR_INCLUDE_ICOMMANDMANAGER_H
#define CRYINCLUDE_EDITOR_INCLUDE_ICOMMANDMANAGER_H
#pragma once
#include "Command.h"

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
        const AZStd::function<void()>& functor);
    template <typename RT>
    bool RegisterCommand(ICommandManager* pCmdMgr, const char* module, const char* name,
        const char* description, const char* example,
        const AZStd::function<RT()>& functor);
    template <LIST(1, typename P)>
    bool RegisterCommand(ICommandManager* pCmdMgr, const char* module, const char* name,
        const char* description, const char* example,
        const AZStd::function<void(LIST(1, P))>& functor);
    template <LIST(1, typename P), typename RT>
    bool RegisterCommand(ICommandManager* pCmdMgr, const char* module, const char* name,
        const char* description, const char* example,
        const AZStd::function<RT(LIST(1, P))>& functor);
    template <LIST(2, typename P)>
    bool RegisterCommand(ICommandManager* pCmdMgr, const char* module, const char* name,
        const char* description, const char* example,
        const AZStd::function<void(LIST(2, P))>& functor);
    template <LIST(2, typename P), typename RT>
    bool RegisterCommand(ICommandManager* pCmdMgr, const char* module, const char* name,
        const char* description, const char* example,
        const AZStd::function<RT(LIST(2, P))>& functor);
    template <LIST(3, typename P)>
    bool RegisterCommand(ICommandManager* pCmdMgr, const char* module, const char* name,
        const char* description, const char* example,
        const AZStd::function<void(LIST(3, P))>& functor);
    template <LIST(3, typename P), typename RT>
    bool RegisterCommand(ICommandManager* pCmdMgr, const char* module, const char* name,
        const char* description, const char* example,
        const AZStd::function<RT(LIST(3, P))>& functor);
    template <LIST(4, typename P)>
    bool RegisterCommand(ICommandManager* pCmdMgr, const char* module, const char* name,
        const char* description, const char* example,
        const AZStd::function<void(LIST(4, P))>& functor);
    template <LIST(4, typename P), typename RT>
    bool RegisterCommand(ICommandManager* pCmdMgr, const char* module, const char* name,
        const char* description, const char* example,
        const AZStd::function<RT(LIST(4, P))>& functor);
    template <LIST(5, typename P)>
    bool RegisterCommand(ICommandManager* pCmdMgr, const char* module, const char* name,
        const char* description, const char* example,
        const AZStd::function<void(LIST(5, P))>& functor);
    template <LIST(6, typename P)>
    bool RegisterCommand(ICommandManager* pCmdMgr, const char* module, const char* name,
        const char* description, const char* example,
        const AZStd::function<void(LIST(6, P))>& functor);

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
    const AZStd::function<void()>& functor)
{
    return Private::RegisterCommand<AZStd::function<void()>, CCommand0>(pCmdMgr, module, name, description, example, functor);
}

template <typename RT>
bool CommandManagerHelper::RegisterCommand(ICommandManager* pCmdMgr, const char* module, const char* name,
    const char* description, const char* example,
    const AZStd::function<RT()>& functor)
{
    return Private::RegisterCommand<AZStd::function<RT()>, CCommand0wRet<RT> >(pCmdMgr, module, name, description, example, functor);
}

template <LIST(1, typename P)>
bool CommandManagerHelper::RegisterCommand(ICommandManager* pCmdMgr, const char* module, const char* name,
    const char* description, const char* example,
    const AZStd::function<void(LIST(1, P))>& functor)
{
    return Private::RegisterCommand<AZStd::function<void(LIST(1, P))>, CCommand1<LIST(1, P)> >(pCmdMgr, module, name, description, example, functor);
}

template <LIST(1, typename P), typename RT>
bool CommandManagerHelper::RegisterCommand(ICommandManager* pCmdMgr, const char* module, const char* name,
    const char* description, const char* example,
    const AZStd::function<RT(LIST(1, P))>& functor)
{
    return Private::RegisterCommand<AZStd::function<RT(LIST(1, P))>, CCommand1wRet<LIST(1, P), RT> >(pCmdMgr, module, name, description, example, functor);
}

template <LIST(2, typename P)>
bool CommandManagerHelper::RegisterCommand(ICommandManager* pCmdMgr, const char* module, const char* name,
    const char* description, const char* example,
    const AZStd::function<void(LIST(2, P))>& functor)
{
    return Private::RegisterCommand<AZStd::function<void(LIST(2, P))>, CCommand2<LIST(2, P)> >(pCmdMgr, module, name, description, example, functor);
}

template <LIST(2, typename P), typename RT>
bool CommandManagerHelper::RegisterCommand(ICommandManager* pCmdMgr, const char* module, const char* name,
    const char* description, const char* example,
    const AZStd::function<RT(LIST(2, P))>& functor)
{
    return Private::RegisterCommand<AZStd::function<RT(LIST(2, P))>, CCommand2wRet<LIST(2, P), RT> >(pCmdMgr, module, name, description, example, functor);
}

template <LIST(3, typename P)>
bool CommandManagerHelper::RegisterCommand(ICommandManager* pCmdMgr, const char* module, const char* name,
    const char* description, const char* example,
    const AZStd::function<void(LIST(3, P))>& functor)
{
    return Private::RegisterCommand<AZStd::function<void(LIST(3, P))>, CCommand3<LIST(3, P)> >(pCmdMgr, module, name, description, example, functor);
}

template <LIST(3, typename P), typename RT>
bool CommandManagerHelper::RegisterCommand(ICommandManager* pCmdMgr, const char* module, const char* name,
    const char* description, const char* example,
    const AZStd::function<RT(LIST(3, P))>& functor)
{
    return Private::RegisterCommand<AZStd::function<RT(LIST(3, P))>, CCommand3wRet<LIST(3, P), RT> >(pCmdMgr, module, name, description, example, functor);
}

template <LIST(4, typename P)>
bool CommandManagerHelper::RegisterCommand(ICommandManager* pCmdMgr, const char* module, const char* name,
    const char* description, const char* example,
    const AZStd::function<void(LIST(4, P))>& functor)
{
    return Private::RegisterCommand<AZStd::function<void(LIST(4, P))>, CCommand4<LIST(4, P)> >(pCmdMgr, module, name, description, example, functor);
}

template <LIST(4, typename P), typename RT>
bool CommandManagerHelper::RegisterCommand(ICommandManager* pCmdMgr, const char* module, const char* name,
    const char* description, const char* example,
    const AZStd::function<RT(LIST(4, P))>& functor)
{
    return Private::RegisterCommand<AZStd::function<RT(LIST(4, P))>, CCommand4wRet<LIST(4, P), RT> >(pCmdMgr, module, name, description, example, functor);
}

template <LIST(5, typename P)>
bool CommandManagerHelper::RegisterCommand(ICommandManager* pCmdMgr, const char* module, const char* name,
    const char* description, const char* example,
    const AZStd::function<void(LIST(5, P))>& functor)
{
    return Private::RegisterCommand<AZStd::function<void(LIST(5, P))>, CCommand5<LIST(5, P)> >(pCmdMgr, module, name, description, example, functor);
}

template <LIST(6, typename P)>
bool CommandManagerHelper::RegisterCommand(ICommandManager* pCmdMgr, const char* module, const char* name,
    const char* description, const char* example,
    const AZStd::function<void(LIST(6, P))>& functor)
{
    return Private::RegisterCommand<AZStd::function<void(LIST(6, P))>, CCommand6<LIST(6, P)> >(pCmdMgr, module, name, description, example, functor);
}
#endif // CRYINCLUDE_EDITOR_INCLUDE_ICOMMANDMANAGER_H
