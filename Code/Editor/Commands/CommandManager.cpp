/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include "EditorDefs.h"

#include "CommandManager.h"

#include <AzToolsFramework/PythonTerminal/ScriptTermDialog.h>

// Editor
#include "QtViewPaneManager.h"
#include "Include/IIconManager.h"

// AzToolsFramework
#include <AzToolsFramework/PythonTerminal/ScriptTermDialog.h>

CAutoRegisterCommandHelper* CAutoRegisterCommandHelper::s_pFirst = nullptr;
CAutoRegisterCommandHelper* CAutoRegisterCommandHelper::s_pLast = nullptr;

CAutoRegisterCommandHelper* CAutoRegisterCommandHelper::GetFirst()
{
    return s_pFirst;
}

CAutoRegisterCommandHelper::CAutoRegisterCommandHelper(void(*registerFunc)(CEditorCommandManager &))
{
    m_registerFunc = registerFunc;
    m_pNext = nullptr;

    if (!s_pLast)
    {
        s_pFirst = this;
    }
    else
    {
        s_pLast->m_pNext = this;
    }

    s_pLast = this;
}

CEditorCommandManager::CEditorCommandManager()
    : m_bWarnDuplicate(true) {}

void CEditorCommandManager::RegisterAutoCommands()
{
    CAutoRegisterCommandHelper* pHelper = CAutoRegisterCommandHelper::GetFirst();

    while (pHelper)
    {
        pHelper->m_registerFunc(*this);
        pHelper = pHelper->m_pNext;
    }
}

CEditorCommandManager::~CEditorCommandManager()
{
    CommandTable::const_iterator iter = m_commands.begin(), end = m_commands.end();

    for (; iter != end; ++iter)
    {
        if (iter->second.deleter)
        {
            iter->second.deleter(iter->second.pCommand);
        }
        else
        {
            delete iter->second.pCommand;
        }
    }

    m_commands.clear();
    m_uiCommands.clear();
}

AZStd::string CEditorCommandManager::GetFullCommandName(const AZStd::string& module, const AZStd::string& name)
{
    AZStd::string fullName = module;
    fullName += ".";
    fullName += name;
    return fullName;
}

bool CEditorCommandManager::AddCommand(CCommand* pCommand, TPfnDeleter deleter)
{
    assert(pCommand);

    AZStd::string module = pCommand->GetModule();
    AZStd::string name = pCommand->GetName();

    if (IsRegistered(module.c_str(), name.c_str()) && m_bWarnDuplicate)
    {
        QString errMsg;

        errMsg = QStringLiteral("Error: Command %1.%2 already registered!").arg(module.c_str(), name.c_str());
        Warning(errMsg.toUtf8().data());

        return false;
    }

    SCommandTableEntry entry;

    entry.pCommand = pCommand;
    entry.deleter = deleter;

    m_commands.insert(
        CommandTable::value_type(GetFullCommandName(module, name),
            entry));

    return true;
}

bool CEditorCommandManager::UnregisterCommand(const char* module, const char* name)
{
    AZStd::string fullName = GetFullCommandName(module, name);
    CommandTable::iterator itr = m_commands.find(fullName);

    if (itr != m_commands.end())
    {
        if (itr->second.deleter)
        {
            itr->second.deleter(itr->second.pCommand);
        }
        else
        {
            delete itr->second.pCommand;
        }

        m_commands.erase(itr);

        return true;
    }
    return false;
}

bool CEditorCommandManager::RegisterUICommand(
    const char* module,
    const char* name,
    const char* description,
    const char* example,
    const AZStd::function<void()>& functor,
    const CCommand0::SUIInfo& uiInfo)
{
    bool ok = CommandManagerHelper::RegisterCommand(this, module, name, description, example, functor);

    if (ok == false)
    {
        return false;
    }

    return AttachUIInfo(GetFullCommandName(module, name).c_str(), uiInfo);
}

bool CEditorCommandManager::AttachUIInfo(const char* fullCmdName, const CCommand0::SUIInfo& uiInfo)
{
    CommandTable::iterator iter = m_commands.find(fullCmdName);

    if (iter == m_commands.end())
    {
        return false;
    }

    if (iter->second.pCommand->CanBeUICommand() == false)
    {
        return false;
    }

    CCommand0* pCommand = static_cast<CCommand0*>(iter->second.pCommand);

    pCommand->m_uiInfo = uiInfo;

    if (pCommand->m_uiInfo.commandId == 0)
    {
        pCommand->m_uiInfo.commandId = GenNewCommandId();
    }

    m_uiCommands.insert(UICommandTable::value_type(pCommand->m_uiInfo.commandId, pCommand));

    if (uiInfo.iconFilename.empty() == false)
    {
        GetIEditor()->GetIconManager()->RegisterCommandIcon(uiInfo.iconFilename.c_str(), pCommand->m_uiInfo.commandId);
    }

    return true;
}

bool CEditorCommandManager::GetUIInfo(const AZStd::string& module, const AZStd::string& name, CCommand0::SUIInfo& uiInfo) const
{
    AZStd::string fullName = GetFullCommandName(module, name);

    return GetUIInfo(fullName, uiInfo);
}

bool CEditorCommandManager::GetUIInfo(const AZStd::string& fullCmdName, CCommand0::SUIInfo& uiInfo) const
{
    CommandTable::const_iterator iter = m_commands.find(fullCmdName);

    if (iter == m_commands.end())
    {
        return false;
    }

    if (iter->second.pCommand->CanBeUICommand() == false)
    {
        return false;
    }

    CCommand0* pCommand = static_cast<CCommand0*>(iter->second.pCommand);
    uiInfo = pCommand->m_uiInfo;

    return true;
}

int CEditorCommandManager::GenNewCommandId()
{
    static int uniqueId = CUSTOM_COMMAND_ID_FIRST;
    return uniqueId++;
}

QString CEditorCommandManager::Execute(const AZStd::string& module, const AZStd::string& name, const CCommand::CArgs& args)
{
    AZStd::string fullName = GetFullCommandName(module, name);
    CommandTable::iterator iter = m_commands.find(fullName);

    if (iter != m_commands.end())
    {
        LogCommand(fullName, args);

        return ExecuteAndLogReturn(iter->second.pCommand, args);
    }
    else
    {
        CryLogAlways("Error: Trying to execute a unknown command, '%s'!", fullName.c_str());
    }

    return "";
}

QString CEditorCommandManager::Execute(const AZStd::string& cmdLine)
{
    AZStd::string cmdTxt, argsTxt;
    size_t argStart = cmdLine.find_first_of(' ');

    cmdTxt = cmdLine.substr(0, argStart);
    argsTxt = "";

    if (argStart != AZStd::string::npos)
    {
        argsTxt = cmdLine.substr(argStart + 1);
        AZ::StringFunc::TrimWhiteSpace(argsTxt, true, true);
    }

    CommandTable::iterator itr = m_commands.find(cmdTxt);

    if (itr != m_commands.end())
    {
        CCommand::CArgs argList;

        GetArgsFromString(argsTxt, argList);
        LogCommand(cmdTxt, argList);

        return ExecuteAndLogReturn(itr->second.pCommand, argList);
    }
    else
    {
        CryLogAlways("Error: Trying to execute a unknown command, '%s'!", cmdLine.c_str());
    }

    return "";
}

void CEditorCommandManager::Execute(int commandId)
{
    UICommandTable::iterator iter = m_uiCommands.find(commandId);

    if (iter != m_uiCommands.end())
    {
        LogCommand(
            GetFullCommandName(iter->second->GetModule(), iter->second->GetName()),
            CCommand::CArgs());
        iter->second->Execute(CCommand::CArgs());
    }
    else
    {
        CryLogAlways("Error: Trying to execute a unknown command of ID '%d'!", commandId);
    }
}

void CEditorCommandManager::GetCommandList(std::vector<AZStd::string>& cmds) const
{
    cmds.clear();
    cmds.reserve(m_commands.size());
    CommandTable::const_iterator iter = m_commands.begin(), end = m_commands.end();

    for (; iter != end; ++iter)
    {
        cmds.push_back(iter->first);
    }

    std::sort(cmds.begin(), cmds.end());
}

AZStd::string CEditorCommandManager::AutoComplete(const AZStd::string& substr) const
{
    std::vector<AZStd::string> cmds;
    GetCommandList(cmds);

    // If substring is empty return first command.
    if (substr.empty() && (cmds.empty() == false))
    {
        return cmds[0];
    }

    size_t substrLen = substr.length();

    for (size_t i = 0; i < cmds.size(); ++i)
    {
        size_t cmdLen = cmds[i].length();

        if (cmdLen >= substrLen && !strncmp(cmds[i].c_str(), substr.c_str(), substrLen))
        {
            if (substrLen == cmdLen)
            {
                ++i;

                if (i < cmds.size())
                {
                    return cmds[i];
                }
                else
                {
                    return cmds[i - 1];
                }
            }

            return cmds[i];
        }
    }

    // Not found
    return "";
}

bool CEditorCommandManager::IsRegistered(const char* module, const char* name) const
{
    AZStd::string fullName = GetFullCommandName(module, name);
    CommandTable::const_iterator iter = m_commands.find(fullName);

    if (iter != m_commands.end())
    {
        return true;
    }
    else
    {
        return false;
    }
}

bool CEditorCommandManager::IsRegistered(const char* cmdLine_) const
{
    AZStd::string cmdTxt, argsTxt, cmdLine(cmdLine_);
    size_t argStart = cmdLine.find_first_of(' ');
    cmdTxt = cmdLine.substr(0, argStart);
    CommandTable::const_iterator iter = m_commands.find(cmdTxt);

    if (iter != m_commands.end())
    {
        return true;
    }
    else
    {
        return false;
    }
}

bool CEditorCommandManager::IsRegistered(int commandId) const
{
    if (CUSTOM_COMMAND_ID_FIRST <= commandId && commandId < CUSTOM_COMMAND_ID_LAST)
    {
        UICommandTable::const_iterator iter = m_uiCommands.find(commandId);

        if (iter != m_uiCommands.end())
        {
            return true;
        }
    }
    return false;
}

void CEditorCommandManager::SetCommandAvailableInScripting(const AZStd::string& module, const AZStd::string& name)
{
    AZStd::string fullName = GetFullCommandName(module, name);
    CommandTable::iterator iter = m_commands.find(fullName);

    if (iter != m_commands.end())
    {
        iter->second.pCommand->SetAvailableInScripting();
    }
}

bool CEditorCommandManager::IsCommandAvailableInScripting(const AZStd::string& fullCmdName) const
{
    CommandTable::const_iterator iter = m_commands.find(fullCmdName);

    if (iter != m_commands.end())
    {
        return iter->second.pCommand->IsAvailableInScripting();
    }

    return false;
}

bool CEditorCommandManager::IsCommandAvailableInScripting(const AZStd::string& module, const AZStd::string& name) const
{
    AZStd::string fullName = GetFullCommandName(module, name);

    return IsCommandAvailableInScripting(fullName);
}

void CEditorCommandManager::LogCommand(const AZStd::string& fullCmdName, const CCommand::CArgs& args) const
{
    AZStd::string cmdLine = fullCmdName;

    for (int i = 0; i < args.GetArgCount(); ++i)
    {
        cmdLine += " ";
        bool bString = args.IsStringArg(i);

        if (bString)
        {
            cmdLine += "'";
        }

        cmdLine += args.GetArg(i);

        if (bString)
        {
            cmdLine += "'";
        }
    }

    CLogFile::WriteLine(cmdLine.c_str());

    if (IsCommandAvailableInScripting(fullCmdName) == false)
    {
        return;
    }

    /// If this same command is also available in the script system,
    /// log this to the script terminal, too.

    // First, recreate a command line to be compatible to the script system.
    cmdLine = fullCmdName;
    cmdLine += "(";

    for (int i = 0; i < args.GetArgCount(); ++i)
    {
        bool bString = args.IsStringArg(i);

        if (bString)
        {
            cmdLine += "\"";
        }

        cmdLine += args.GetArg(i);

        if (bString)
        {
            cmdLine += "\"";
        }

        if (i < args.GetArgCount() - 1)
        {
            cmdLine += ",";
        }
    }

    cmdLine += ")";

    // If it's not SandBox main editor (one case is the standalone material editor triggered by 3ds Max exporter),
    // we should not cast it into main editor for further operation.
    if (GetIEditor()->IsInMatEditMode())
    {
        return;
    }

    // Then, register it to the terminal.
    QtViewPane* scriptTermPane = QtViewPaneManager::instance()->GetPane(SCRIPT_TERM_WINDOW_NAME);
    if (!scriptTermPane)
    {
        return;
    }
    AzToolsFramework::CScriptTermDialog* pScriptTermDialog = qobject_cast<AzToolsFramework::CScriptTermDialog*>(scriptTermPane->Widget());

    if (pScriptTermDialog)
    {
        AZStd::string text = "> ";
        text += cmdLine;
        text += "\r\n";
        pScriptTermDialog->AppendText(text.c_str());
    }
}

QString CEditorCommandManager::ExecuteAndLogReturn(CCommand* pCommand, const CCommand::CArgs& args)
{
    const QString result = pCommand->Execute(args);
    const QString returnMsg = QString("Returned: %1").arg(result);

    CLogFile::WriteLine(returnMsg.toUtf8().constData());

    return result;
}

void CEditorCommandManager::GetArgsFromString(const AZStd::string& argsTxt, CCommand::CArgs& argList)
{
    const char quoteSymbol = '\'';
    size_t curPos = 0;
    size_t prevPos = 0;
    AZStd::vector<AZStd::string> tokens;
    AZ::StringFunc::Tokenize(argsTxt, tokens, ' ');
    for(AZStd::string& arg : tokens)
    {
        if (arg[0] == quoteSymbol)   // A special consideration for a quoted string
        {
            if (arg.length() < 2 || arg[arg.length() - 1] != quoteSymbol)
            {
                size_t openingQuotePos = argsTxt.find(quoteSymbol, prevPos);
                size_t closingQuotePos = argsTxt.find(quoteSymbol, curPos);

                if (closingQuotePos != AZStd::string::npos)
                {
                    arg = argsTxt.substr(openingQuotePos + 1, closingQuotePos - openingQuotePos - 1);
                    size_t nextArgPos = argsTxt.find(' ', closingQuotePos + 1);
                    curPos = nextArgPos != AZStd::string::npos ? nextArgPos + 1 : argsTxt.length();

                    for (; curPos < argsTxt.length(); ++curPos)    // Skip spaces.
                    {
                        if (argsTxt[curPos] != ' ')
                        {
                            break;
                        }
                    }
                }
            }
            else
            {
                arg = arg.substr(1, arg.length() - 2);
            }
        }

        argList.Add(arg.c_str());
        prevPos = curPos;
    }
}
