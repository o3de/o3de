/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


// Description : the command manager


#ifndef CRYINCLUDE_EDITOR_COMMANDS_COMMANDMANAGER_H
#define CRYINCLUDE_EDITOR_COMMANDS_COMMANDMANAGER_H
#pragma once

#include "platform.h"

#include <ISystem.h>

#include "Include/SandboxAPI.h"
#include "Include/ICommandManager.h"

AZ_PUSH_DISABLE_DLL_EXPORT_BASECLASS_WARNING
class SANDBOX_API CEditorCommandManager
    : public ICommandManager
{
AZ_POP_DISABLE_DLL_EXPORT_BASECLASS_WARNING
public:
    enum
    {
        CUSTOM_COMMAND_ID_FIRST = 10000,
        CUSTOM_COMMAND_ID_LAST  = 15000
    };

    CEditorCommandManager();
    ~CEditorCommandManager();

    void RegisterAutoCommands();

    bool AddCommand(CCommand* pCommand, TPfnDeleter deleter = nullptr);
    bool UnregisterCommand(const char* module, const char* name);
    bool RegisterUICommand(
        const char* module,
        const char* name,
        const char* description,
        const char* example,
        const AZStd::function<void()>& functor,
        const CCommand0::SUIInfo& uiInfo);
    bool AttachUIInfo(const char* fullCmdName, const CCommand0::SUIInfo& uiInfo);
    bool GetUIInfo(const AZStd::string& module, const AZStd::string& name, CCommand0::SUIInfo& uiInfo) const;
    bool GetUIInfo(const AZStd::string& fullCmdName, CCommand0::SUIInfo& uiInfo) const;
    QString Execute(const AZStd::string& cmdLine);
    QString Execute(const AZStd::string& module, const AZStd::string& name, const CCommand::CArgs& args);
    void Execute(int commandId);
    void GetCommandList(std::vector<AZStd::string>& cmds) const;
    //! Used in the console dialog
    AZStd::string AutoComplete(const AZStd::string& substr) const;
    bool IsRegistered(const char* module, const char* name) const;
    bool IsRegistered(const char* cmdLine) const;
    bool IsRegistered(int commandId) const;
    void SetCommandAvailableInScripting(const AZStd::string& module, const AZStd::string& name);
    bool IsCommandAvailableInScripting(const AZStd::string& module, const AZStd::string& name) const;
    bool IsCommandAvailableInScripting(const AZStd::string& fullCmdName) const;
    //! Turning off the warning is needed for reloading the ribbon bar.
    void TurnDuplicateWarningOn() { m_bWarnDuplicate = true; }
    void TurnDuplicateWarningOff() { m_bWarnDuplicate = false; }

protected:
    struct SCommandTableEntry
    {
        CCommand* pCommand;
        TPfnDeleter deleter;
    };

    //! A full command name to an actual command mapping
    typedef std::map<AZStd::string, SCommandTableEntry> CommandTable;
    AZ_PUSH_DISABLE_DLL_EXPORT_MEMBER_WARNING
    CommandTable m_commands;

    //! A command ID to an actual UI command mapping
    //! This table will contain a subset of commands among all registered to the above table.
    typedef std::map<int, CCommand0*> UICommandTable;
    UICommandTable m_uiCommands;
    AZ_POP_DISABLE_DLL_EXPORT_MEMBER_WARNING
    bool m_bWarnDuplicate;

    static int GenNewCommandId();
    static AZStd::string GetFullCommandName(const AZStd::string& module, const AZStd::string& name);
    static void GetArgsFromString(const AZStd::string& argsTxt, CCommand::CArgs& argList);
    void LogCommand(const AZStd::string& fullCmdName, const CCommand::CArgs& args) const;
    QString ExecuteAndLogReturn(CCommand* pCommand, const CCommand::CArgs& args);
};

//! A helper class for an automatic command registration
class SANDBOX_API CAutoRegisterCommandHelper
{
public:
    static CAutoRegisterCommandHelper* GetFirst();

    CAutoRegisterCommandHelper(void(*registerFunc)(CEditorCommandManager &));
    void (* m_registerFunc)(CEditorCommandManager&);
    CAutoRegisterCommandHelper* m_pNext;

private:

    static CAutoRegisterCommandHelper* s_pFirst;
    static CAutoRegisterCommandHelper* s_pLast;
};

#define REGISTER_EDITOR_COMMAND(boundFunction, moduleName, functionName, description, example)                                  \
    void RegisterCommand##moduleName##functionName(CEditorCommandManager & cmdMgr)                                              \
    {                                                                                                                           \
        CommandManagerHelper::RegisterCommand(&cmdMgr, #moduleName, #functionName, description, example, boundFunction);        \
    }                                                                                                                           \
    CAutoRegisterCommandHelper g_AutoRegCmdHelper##moduleName##functionName(RegisterCommand##moduleName##functionName)

#endif // CRYINCLUDE_EDITOR_COMMANDS_COMMANDMANAGER_H
