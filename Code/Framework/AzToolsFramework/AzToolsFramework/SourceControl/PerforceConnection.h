/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzToolsFramework/SourceControl/PerforceComponent.h>
#include <AzFramework/Process/ProcessCommunicator.h>

namespace AzFramework
{
    class ProcessWatcher;
}

namespace AzToolsFramework
{
    class PerforceCommand
    {
    public:
        AzFramework::ProcessOutput m_rawOutput;
        PerforceMap m_commandOutputMap;             // doesn't allow duplicate kvp's
        AZStd::vector<PerforceMap> m_commandOutputMapList; // allows duplicate kvp's

        AZStd::mutex m_commandMutex;

        PerforceCommand() {}
        virtual ~PerforceCommand() = default;

        AZStd::string GetCurrentChangelistNumber(const PerforceMap* map = nullptr) const;
        AZStd::string GetHaveRevision(const PerforceMap* map = nullptr) const;
        AZStd::string GetHeadRevision(const PerforceMap* map = nullptr) const;
        AZStd::string GetOtherUserCheckedOut(const PerforceMap* map = nullptr) const;
        int GetOtherUserCheckOutCount(const PerforceMap* map = nullptr) const;

        bool CurrentActionIsAdd(const PerforceMap* map = nullptr) const;
        bool CurrentActionIsEdit(const PerforceMap* map = nullptr) const;
        bool CurrentActionIsDelete(const PerforceMap* map = nullptr) const;
        bool CurrentActionIsMove(const PerforceMap* map = nullptr) const;
        bool FileExists() const;
        bool FileExists(const char* searchFile) const;
        bool HasRevision(const PerforceMap* map = nullptr) const;
        bool HeadActionIsDelete(const PerforceMap* map = nullptr) const;
        bool IsMarkedForAdd() const;
        bool NeedsReopening() const;
        bool IsOpenByOtherUsers(const PerforceMap* map = nullptr) const;
        bool IsOpenByCurrentUser(const PerforceMap* map = nullptr) const;
        bool NewFileAfterDeletedRev(const PerforceMap* map = nullptr) const;

        bool ApplicationFound() const;
        bool HasTrustIssue() const;
        bool ExclusiveOpen(const PerforceMap* map = nullptr) const;

        AZStd::string GetOutputValue(const AZStd::string& key, const PerforceMap* perforceMap = nullptr) const;

        bool OutputKeyExists(const AZStd::string& key, const PerforceMap* perforceMap = nullptr) const;

        AZStd::vector<PerforceMap>::iterator FindMapWithPartiallyMatchingValueForKey(const AZStd::string& key, const AZStd::string& value);
        AZStd::string CreateChangelistForm(const AZStd::string& client, const AZStd::string& user, const AZStd::string& description);

        void ExecuteAdd(const AZStd::string& changelist, const AZStd::string& filePath);
        void ExecuteAdd(const AZStd::string& changelist, const AZStd::unordered_set<AZStd::string>& filePaths);
        void ExecuteClaimChangedFile(const AZStd::string& filePath, const AZStd::string& changeList);
        void ExecuteDelete(const AZStd::string& changelist, const AZStd::string& filePath);
        void ExecuteEdit(const AZStd::string& changelist, const AZStd::string& filePath);
        void ExecuteEdit(const AZStd::string& changelist, const AZStd::unordered_set<AZStd::string>& filePaths);
        void ExecuteFstat(const AZStd::string& filePath);
        void ExecuteFstat(const AZStd::unordered_set<AZStd::string>& filePaths);
        void ExecuteSync(const AZStd::string& filePath);
        void ExecuteMove(const AZStd::string& changelist, const AZStd::string& sourcePath, const AZStd::string& destPath);
        void ExecuteSet();
        void ExecuteSet(const AZStd::string& key, const AZStd::string& value);
        void ExecuteInfo();
        void ExecuteShortInfo();
        void ExecuteTicketStatus();
        void ExecuteTrust(bool enable, const AZStd::string& fingerprint);

        AzFramework::ProcessWatcher* ExecuteNewChangelistInput();
        void ExecuteNewChangelistOutput();
        void ExecuteRevert(const AZStd::string& filePath);
        void ExecuteShowChangelists(const AZStd::string& currentUser, const AZStd::string& currentClient);

        void ThrowWarningMessage();

    protected:
        AZStd::string m_commandArgs;
        bool m_applicationFound = false;

        virtual void ExecuteCommand();
        virtual AzFramework::ProcessWatcher* ExecuteIOCommand();
        virtual void ExecuteRawCommand();
    };

    class PerforceConnection
    {
    public:
        PerforceMap m_infoResultMap;
        PerforceCommand& m_command;

        PerforceConnection() : m_command(m_commandInternal) {}
        ~PerforceConnection() {}

        AZStd::string GetUser() const;
        AZStd::string GetClientName() const;
        AZStd::string GetClientRoot() const;
        AZStd::string GetServerAddress() const;
        AZStd::string GetServerUptime() const;
        AZStd::string GetInfoValue(const AZStd::string& key) const;

        bool CommandHasFailed();
        bool CommandHasOutput() const;
        bool CommandHasError() const;
        bool CommandHasTrustIssue() const;
        bool CommandApplicationFound() const;
        AZStd::string GetCommandOutput() const;
        AZStd::string GetCommandError() const;

    protected:
        PerforceConnection(PerforceCommand& command) : m_command(command) {}

        PerforceCommand m_commandInternal;
    };
} // namespace AzToolsFramework
