/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "AzToolsFramework_precompiled.h"

#include <AzToolsFramework/SourceControl/PerforceConnection.h>
#include <AzFramework/Process/ProcessWatcher.h>

#define SCC_WINDOW "Source Control"

namespace AzToolsFramework
{
    namespace
    {
        char s_EndOfFileChar(26);
    }

    AZStd::string PerforceCommand::GetCurrentChangelistNumber(const PerforceMap* map) const
    {
        return GetOutputValue("change", map);
    }

    AZStd::string PerforceCommand::GetHaveRevision(const PerforceMap* map) const
    {
        return GetOutputValue("haveRev", map); 
    }

    AZStd::string PerforceCommand::GetHeadRevision(const PerforceMap* map) const
    {
        return GetOutputValue("headRev", map);
    }

    AZStd::string PerforceCommand::GetOtherUserCheckedOut(const PerforceMap* map) const
    {
        return GetOutputValue("otherOpen0", map);
    }

    int PerforceCommand::GetOtherUserCheckOutCount(const PerforceMap* map) const
    {
        return atoi(GetOutputValue("otherOpen", map).c_str());
    }

    bool PerforceCommand::CurrentActionIsAdd(const PerforceMap* map) const
    {
        return GetOutputValue("action", map) == "add";
    }

    bool PerforceCommand::CurrentActionIsEdit(const PerforceMap* map) const
    {
        return GetOutputValue("action", map) == "edit";
    }

    bool PerforceCommand::CurrentActionIsDelete(const PerforceMap* map) const
    {
        return GetOutputValue("action", map) == "delete";
    }

    bool PerforceCommand::CurrentActionIsMove(const PerforceMap* map) const
    {
        AZStd::string value = GetOutputValue("action", map);

        return value == "move/delete" || value == "move/add";
    }

    bool PerforceCommand::FileExists() const
    {
        return m_rawOutput.errorResult.find("no such file(s)") == AZStd::string::npos;
    }

    bool PerforceCommand::FileExists(const char* searchFile) const
    {
        return m_rawOutput.errorResult.find(AZStd::string::format("%s - no such file(s)", searchFile)) == AZStd::string::npos;
    }

    bool PerforceCommand::HasRevision(const PerforceMap* map) const
    {
        return atoi(GetOutputValue("haveRev", map).c_str()) > 0;
    }

    bool PerforceCommand::HeadActionIsDelete(const PerforceMap* map) const
    {
        return GetOutputValue("headAction", map) == "delete";
    }

    bool PerforceCommand::IsMarkedForAdd() const
    {
        return m_rawOutput.outputResult.find("can't edit (already opened for add)") != AZStd::string::npos;
    }

    bool PerforceCommand::NeedsReopening() const
    {
        return m_rawOutput.outputResult.find("use 'reopen'") != AZStd::string::npos;
    }

    bool PerforceCommand::IsOpenByOtherUsers(const PerforceMap* map) const
    {
        return OutputKeyExists("otherOpen", map);
    }

    bool PerforceCommand::IsOpenByCurrentUser(const PerforceMap* map) const
    {
        return OutputKeyExists("action", map);
    }

    bool PerforceCommand::NewFileAfterDeletedRev(const PerforceMap* map) const
    {
        return (HeadActionIsDelete(map) && !HasRevision(map));
    }

    bool PerforceCommand::ApplicationFound() const
    {
        return m_applicationFound;
    }

    bool PerforceCommand::HasTrustIssue() const
    {
        if (m_rawOutput.errorResult.find("The authenticity of ") != AZStd::string::npos &&
            m_rawOutput.errorResult.find("can't be established,") != AZStd::string::npos)
        {
            return true;
        }

        return false;
    }

    bool PerforceCommand::ExclusiveOpen(const PerforceMap* map) const
    {
        AZStd::string fileType = GetOutputValue("headType", map);
        if (!fileType.empty())
        {
            size_t modLocation = fileType.find('+');
            if (modLocation != AZStd::string::npos)
            {
                return fileType.find('l', modLocation) != AZStd::string::npos;
            }
        }
        return false;
    }

    AZStd::string PerforceCommand::GetOutputValue(const AZStd::string& key, const PerforceMap* perforceMap) const
    {
        if(!perforceMap)
        {
            perforceMap = &m_commandOutputMap;
        }

        PerforceMap::const_iterator kvp = perforceMap->find(key);
        if (kvp != perforceMap->end())
        {
            return kvp->second;
        }
        else
        {
            return "";
        }
    }

    bool PerforceCommand::OutputKeyExists(const AZStd::string& key, const PerforceMap* perforceMap) const
    {
        if(!perforceMap)
        {
            perforceMap = &m_commandOutputMap;
        }

        PerforceMap::const_iterator kvp = perforceMap->find(key);
        return kvp != perforceMap->end();
    }

    AZStd::vector<PerforceMap>::iterator PerforceCommand::FindMapWithPartiallyMatchingValueForKey(const AZStd::string& key, const AZStd::string& value)
    {
        for (AZStd::vector<PerforceMap>::iterator perforceIter = m_commandOutputMapList.begin();
                perforceIter != m_commandOutputMapList.end(); ++perforceIter)
        {
            const PerforceMap currentMap = *perforceIter;
            PerforceMap::const_iterator kvp = currentMap.find(key);
            if (kvp != currentMap.end() && kvp->second.find(value) != AZStd::string::npos)
            {
                return perforceIter;
            }
        }

        return nullptr;
    }

    AZStd::string PerforceCommand::CreateChangelistForm(const AZStd::string& client, const AZStd::string& user, const AZStd::string& description)
    {
        AZStd::string changelistForm = "Change:\tnew\n\nClient:\t";
        changelistForm.append(client);

        if (!user.empty() && user != "*unknown*")
        {
            changelistForm.append("\n\nUser:\t");
            changelistForm.append(user);
        }

        changelistForm.append("\n\nStatus:\tnew\n\nDescription:\n\t");
        changelistForm.append(description);
        changelistForm.append("\n\n");
        changelistForm += s_EndOfFileChar;
        return changelistForm;
    }

    void PerforceCommand::ExecuteAdd(const AZStd::string& changelist, const AZStd::string& filePath)
    {
        m_commandArgs = "add -c " + changelist + " \"" + filePath + "\"";
        ExecuteCommand();
    }

    void PerforceCommand::ExecuteAdd(const AZStd::string& changelist, const AZStd::unordered_set<AZStd::string>& filePaths)
    {
        m_commandArgs = "add -c " + changelist + " ";

        for (const AZStd::string& filePath : filePaths)
        {
            m_commandArgs += "\"" + filePath + "\" ";
        }

        ExecuteCommand();
    }

    void PerforceCommand::ExecuteClaimChangedFile(const AZStd::string& filePath, const AZStd::string& changeList)
    {
        m_commandArgs = "reopen -c " + changeList + " \"" + filePath + "\"";
        ExecuteCommand();
    }

    void PerforceCommand::ExecuteDelete(const AZStd::string& changelist, const AZStd::string& filePath)
    {
        m_commandArgs = "delete -c " + changelist + " \"" + filePath + "\"";
        ExecuteCommand();
    }

    void PerforceCommand::ExecuteEdit(const AZStd::string& changelist, const AZStd::string& filePath)
    {
        m_commandArgs = "edit -c " + changelist + " \"" + filePath + "\"";
        ExecuteCommand();
    }

    void PerforceCommand::ExecuteEdit(const AZStd::string& changelist, const AZStd::unordered_set<AZStd::string>& filePaths)
    {
        m_commandArgs = "edit -c " + changelist + " ";

        for (const AZStd::string& filePath : filePaths)
        {
            m_commandArgs += "\"" + filePath + "\" ";
        }

        ExecuteCommand();
    }

    void PerforceCommand::ExecuteFstat(const AZStd::string& filePath)
    {
        m_commandArgs = "fstat \"" + filePath + "\"";
        ExecuteCommand();
    }

    void PerforceCommand::ExecuteFstat(const AZStd::unordered_set<AZStd::string>& filePaths)
    {
        m_commandArgs = "fstat ";

        for (const AZStd::string& filePath : filePaths)
        {
            m_commandArgs += "\"" + filePath + "\" ";
        }

        ExecuteCommand();
    }

    void PerforceCommand::ExecuteSync(const AZStd::string& filePath)
    {
        m_commandArgs = "sync -f \"" + filePath + "\"";
        ExecuteCommand();
    }

    void PerforceCommand::ExecuteMove(const AZStd::string& changelist, const AZStd::string& sourcePath, const AZStd::string& destPath)
    {
        m_commandArgs = "move -c " + changelist + " \"" + sourcePath + "\"" + " \"" + destPath + "\"";
        ExecuteCommand();
    }

    void PerforceCommand::ExecuteSet()
    {
        m_commandArgs = "set";
        ExecuteRawCommand();
    }

    void PerforceCommand::ExecuteSet(const AZStd::string& key, const AZStd::string& value)
    {
        m_commandArgs = "set " + key + '=' + value;
        ExecuteIOCommand();
    }

    void PerforceCommand::ExecuteInfo()
    {
        m_commandArgs = "info";
        ExecuteCommand();
    }

    void PerforceCommand::ExecuteShortInfo()
    {
        m_commandArgs = "info -s";
        ExecuteCommand();
    }

    void PerforceCommand::ExecuteTicketStatus()
    {
        m_commandArgs = "login -s";
        ExecuteCommand();
    }

    void PerforceCommand::ExecuteTrust(bool enable, const AZStd::string& fingerprint)
    {
        if (enable)
        {
            m_commandArgs = "trust -i ";
        }
        else
        {
            m_commandArgs = "trust -d ";
        }
        m_commandArgs += fingerprint;
        ExecuteCommand();
    }

    AzFramework::ProcessWatcher* PerforceCommand::ExecuteNewChangelistInput()
    {
        m_commandArgs = "change -i";
        return ExecuteIOCommand();
    }

    void PerforceCommand::ExecuteNewChangelistOutput()
    {
        m_commandArgs = "change -o";
        ExecuteRawCommand();
    }

    void PerforceCommand::ExecuteRevert(const AZStd::string& filePath)
    {
        m_commandArgs = "revert \"" + filePath + "\"";
        ExecuteCommand();
    }

    void PerforceCommand::ExecuteShowChangelists(const AZStd::string& currentUser, const AZStd::string& currentClient)
    {
        m_commandArgs = "changes -s pending -c " + currentClient;
        if (!currentUser.empty() && currentUser != "*unknown*")
        {
            m_commandArgs += " -u " + currentUser;
        }
        ExecuteCommand();
    }

    void PerforceCommand::ThrowWarningMessage()
    {
        // This can happen all the time, for various reasons.  AZ_Warning will actually cause the application to
        // take a stack dump and will load pdbs, which can introduce a serious delay during startup.  As such,
        // we send it as a Trace, not a warning.  Background threads retrying may hit this many times.
        AZ_TracePrintf(SCC_WINDOW, "Perforce Warning - Command has failed '%s'\n", m_commandArgs.c_str());
    }

    void PerforceCommand::ExecuteCommand()
    {
        m_rawOutput.Clear();
        AzFramework::ProcessLauncher::ProcessLaunchInfo processLaunchInfo;
        processLaunchInfo.m_commandlineParameters = "p4 -ztag " + m_commandArgs;
        processLaunchInfo.m_showWindow = false;
        AzFramework::ProcessWatcher::LaunchProcessAndRetrieveOutput(processLaunchInfo, AzFramework::ProcessCommunicationType::COMMUNICATOR_TYPE_STDINOUT, m_rawOutput);
        m_applicationFound = processLaunchInfo.m_launchResult == AzFramework::ProcessLauncher::PLR_MissingFile ? false : true;
    }

    AzFramework::ProcessWatcher* PerforceCommand::ExecuteIOCommand()
    {
        AzFramework::ProcessLauncher::ProcessLaunchInfo processLaunchInfo;
        processLaunchInfo.m_commandlineParameters = "p4 " + m_commandArgs;
        processLaunchInfo.m_showWindow = false;
        AzFramework::ProcessWatcher* processWatcher = AzFramework::ProcessWatcher::LaunchProcess(processLaunchInfo, AzFramework::ProcessCommunicationType::COMMUNICATOR_TYPE_STDINOUT);
        m_applicationFound = processLaunchInfo.m_launchResult == AzFramework::ProcessLauncher::PLR_MissingFile ? false : true;
        return processWatcher;
    }

    void PerforceCommand::ExecuteRawCommand()
    {
        m_rawOutput.Clear();
        AzFramework::ProcessLauncher::ProcessLaunchInfo processLaunchInfo;
        processLaunchInfo.m_commandlineParameters = "p4 " + m_commandArgs;
        processLaunchInfo.m_showWindow = false;
        AzFramework::ProcessWatcher::LaunchProcessAndRetrieveOutput(processLaunchInfo, AzFramework::ProcessCommunicationType::COMMUNICATOR_TYPE_STDINOUT, m_rawOutput);
        m_applicationFound = processLaunchInfo.m_launchResult == AzFramework::ProcessLauncher::PLR_MissingFile ? false : true;
    }

    AZStd::string PerforceConnection::GetUser() const
    {
        return GetInfoValue("userName");
    }

    AZStd::string PerforceConnection::GetClientName() const
    { 
        return GetInfoValue("clientName");
    }

    AZStd::string PerforceConnection::GetClientRoot() const
    { 
        return GetInfoValue("clientRoot");
    }

    AZStd::string PerforceConnection::GetServerAddress() const
    { 
        return GetInfoValue("serverAddress");
    }

    AZStd::string PerforceConnection::GetServerUptime() const
    { 
        return GetInfoValue("serverUptime");
    }

    AZStd::string PerforceConnection::GetInfoValue(const AZStd::string& key) const
    {
        PerforceMap::const_iterator kvp = m_infoResultMap.find(key);
        if (kvp != m_infoResultMap.end())
        {
            return kvp->second;
        }
        return "";
    }

    bool PerforceConnection::CommandHasOutput() const
    { 
        return m_command.m_rawOutput.HasOutput();
    }

    bool PerforceConnection::CommandHasError() const
    { 
        return m_command.m_rawOutput.HasError();
    }

    bool PerforceConnection::CommandHasTrustIssue() const
    { 
        return m_command.HasTrustIssue();
    }

    bool PerforceConnection::CommandApplicationFound() const
    {
        return m_command.ApplicationFound();
    }

    AZStd::string PerforceConnection::GetCommandOutput() const
    { 
        return m_command.m_rawOutput.outputResult;
    }

    AZStd::string PerforceConnection::GetCommandError() const
    {
        return m_command.m_rawOutput.errorResult;
    }

    bool PerforceConnection::CommandHasFailed()
    {
        if (!CommandHasOutput())
        {
            if (CommandHasError())
            {
                AZ_TracePrintf(SCC_WINDOW, "Perforce - Error\n%s\n", GetCommandError().c_str());
            }

            m_command.ThrowWarningMessage();
            return true;
        }

        return false;
    }
} // namespace AzToolsFramework
