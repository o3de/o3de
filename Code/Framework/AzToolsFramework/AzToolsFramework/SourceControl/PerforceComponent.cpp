/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include <AzToolsFramework/SourceControl/PerforceComponent.h>

#include <AzCore/Component/TickBus.h>
#include <AzCore/IO/SystemFile.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/std/string/conversions.h>
#include <AzCore/std/parallel/lock.h>

#include <AzFramework/StringFunc/StringFunc.h>

#include <AzFramework/Process/ProcessWatcher.h>
#include <AzToolsFramework/SourceControl/PerforceConnection.h>
#include "PerforceSettings.h"

#include <QProcess>

namespace AzToolsFramework
{
    namespace
    {
        PerforceConnection* s_perforceConn = nullptr;
    }

    void PerforceSettingResult::UpdateSettingInfo(const AZStd::string& value)
    {
        if (value.empty())
        {
            m_settingInfo.m_status = SourceControlSettingStatus::Unset;
            return;
        }

        AZStd::size_t pos = value.find(" (set)");
        if (pos != AZStd::string::npos)
        {
            m_settingInfo.m_status = SourceControlSettingStatus::Set;
            m_settingInfo.m_value = value.substr(0, pos);
            return;
        }

        pos = value.find(" (config");
        if (pos != AZStd::string::npos)
        {
            m_settingInfo.m_status = SourceControlSettingStatus::Config;
            m_settingInfo.m_value = value.substr(0, pos);
            AZStd::size_t fileStart = value.find('\'', pos);
            AZStd::size_t fileEnd = value.find('\'', pos + 10);  // 10: (config '
            if (fileStart != AZStd::string::npos && fileEnd > fileStart)
            {
                m_settingInfo.m_context = value.substr(fileStart, fileEnd - fileStart);
            }
            return;
        }

        m_settingInfo.m_status = SourceControlSettingStatus::None;
        m_settingInfo.m_value = value;
    }

    void PerforceComponent::Activate()
    {
        AZ_Assert(!s_perforceConn, "You may only have one Perforce component.\n");
        m_shutdownThreadSignal = false;
        m_waitingOnTrust = false;
        m_autoChangelistDescription = "*Open 3D Engine Auto";
        m_connectionState = SourceControlState::Disabled;
        m_validConnection = false;
        m_testConnection = false;
        m_offlineMode = true;
        m_trustedKey = false;
        m_resolveKey = true;
        m_testTrust = false;


        // set up signals before we start thread.
        m_shutdownThreadSignal = false;
        m_WorkerThread = AZStd::thread(
            { /*m_name =*/ "Perforce worker" },
            [this]
            {
                ThreadWorker();
            }
        );

        SourceControlConnectionRequestBus::Handler::BusConnect();
        SourceControlCommandBus::Handler::BusConnect();
    }

    void PerforceComponent::Deactivate()
    {
        SourceControlCommandBus::Handler::BusDisconnect();
        SourceControlConnectionRequestBus::Handler::BusDisconnect();

        m_shutdownThreadSignal = true; // tell the thread to die.
        m_WorkerSemaphore.release(1); // wake up the thread so that it sees the signal
        m_WorkerThread.join(); // wait for the thread to finish.
        m_WorkerThread = AZStd::thread();

        SetConnection(nullptr);
    }

    void PerforceComponent::SetConnection(PerforceConnection* connection)
    {
        delete s_perforceConn;

        s_perforceConn = connection;
    }

    void PerforceComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<PerforceComponent, AZ::Component>()
                ;

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<PerforceComponent>(
                    "Perforce Connectivity", "Manages Perforce connectivity and executes Perforce commands.")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Category, "Editor")
                ;
            }
        }
    }

    void PerforceComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("SourceControlService"));
        provided.push_back(AZ_CRC_CE("PerforceService"));
    }

    void PerforceComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("PerforceService"));
    }

    void PerforceComponent::GetFileInfo(const char* fullFilePath, const SourceControlResponseCallback& respCallback)
    {
        AZ_Assert(fullFilePath, "Must specify a file path");
        AZ_Assert(respCallback, "Must specify a callback!");

        PerforceJobRequest topReq(PerforceJobRequest::PJR_Stat, fullFilePath, respCallback);
        QueueJobRequest(AZStd::move(topReq));
    }

    void PerforceComponent::GetBulkFileInfo(const AZStd::unordered_set<AZStd::string>& fullFilePaths, const SourceControlResponseCallbackBulk& respCallback)
    {
        AZ_Assert(!fullFilePaths.empty(), "Must specify a file path");
        AZ_Assert(respCallback, "Must specify a callback!");

        PerforceJobRequest topReq(PerforceJobRequest::PJR_StatBulk, fullFilePaths, respCallback);
        QueueJobRequest(AZStd::move(topReq));
    }

    void PerforceComponent::RequestEdit(const char* fullFilePath, bool allowMultiCheckout, const SourceControlResponseCallback& respCallback)
    {
        AZ_Assert(fullFilePath, "Must specify a file path");
        AZ_Assert(respCallback, "Must specify a callback!");

        PerforceJobRequest topReq(PerforceJobRequest::PJR_Edit, fullFilePath, respCallback);
        topReq.m_allowMultiCheckout = allowMultiCheckout;
        QueueJobRequest(AZStd::move(topReq));
    }

    void PerforceComponent::RequestEditBulk(const AZStd::unordered_set<AZStd::string>& fullFilePaths, bool allowMultiCheckout, const SourceControlResponseCallbackBulk& respCallback)
    {
        AZ_Assert(!fullFilePaths.empty(), "Must specify a file path");
        AZ_Assert(respCallback, "Must specify a callback!");

        PerforceJobRequest topReq(PerforceJobRequest::PJR_EditBulk, fullFilePaths, respCallback);
        topReq.m_allowMultiCheckout = allowMultiCheckout;
        QueueJobRequest(AZStd::move(topReq));
    }

    void PerforceComponent::RequestDelete(const char* fullFilePath, const SourceControlResponseCallback& respCallback)
    {
        RequestDeleteExtended(fullFilePath, false, respCallback);
    }

    void PerforceComponent::RequestDeleteExtended(const char* fullFilePath, bool skipReadOnly, const SourceControlResponseCallback& respCallback)
    {
        AZ_Assert(fullFilePath, "Must specify a file path");
        AZ_Assert(respCallback, "Must specify a callback!");

        PerforceJobRequest topReq(PerforceJobRequest::PJR_Delete, fullFilePath, respCallback);
        topReq.m_skipReadonly = skipReadOnly;
        QueueJobRequest(AZStd::move(topReq));
    }

    void PerforceComponent::RequestDeleteBulk(const char* fullFilePath, const SourceControlResponseCallbackBulk& respCallback)
    {
        RequestDeleteBulkExtended(fullFilePath, false, respCallback);
    }

    void PerforceComponent::RequestDeleteBulkExtended(const char* fullFilePath, bool skipReadOnly, const SourceControlResponseCallbackBulk& respCallback)
    {
        AZ_Assert(fullFilePath, "Must specify a file path");
        AZ_Assert(respCallback, "Must specify a callback!");

        PerforceJobRequest topReq(PerforceJobRequest::PJR_DeleteBulk, fullFilePath, respCallback);
        topReq.m_skipReadonly = skipReadOnly;
        QueueJobRequest(AZStd::move(topReq));
    }

    void PerforceComponent::RequestRevert(const char* fullFilePath, const SourceControlResponseCallback& respCallback)
    {
        AZ_Assert(fullFilePath, "Must specify a file path");
        AZ_Assert(respCallback, "Must specify a callback!");

        PerforceJobRequest topReq(PerforceJobRequest::PJR_Revert, fullFilePath, respCallback);
        QueueJobRequest(AZStd::move(topReq));
    }

    void PerforceComponent::RequestLatest(const char* fullFilePath, const SourceControlResponseCallback& respCallback)
    {
        AZ_Assert(fullFilePath, "Must specify a file path");
        AZ_Assert(respCallback, "Must specify a callback!");

        PerforceJobRequest topReq(PerforceJobRequest::PJR_Sync, fullFilePath, respCallback);
        QueueJobRequest(AZStd::move(topReq));
    }

    void PerforceComponent::RequestRename(const char* sourcePathFull, const char* destPathFull, const SourceControlResponseCallback& respCallback)
    {
        RequestRenameExtended(sourcePathFull, destPathFull, false, respCallback);
    }

    void PerforceComponent::RequestRenameExtended(const char* sourcePathFull, const char* destPathFull, bool skipReadOnly, const SourceControlResponseCallback& respCallback)
    {
        AZ_Assert(sourcePathFull, "Must specify a source file path");
        AZ_Assert(destPathFull, "Must specify a destination file path");
        AZ_Assert(respCallback, "Must specify a callback!");

        PerforceJobRequest topReq(PerforceJobRequest::PJR_Rename, sourcePathFull, respCallback);
        topReq.m_targetPath = destPathFull;
        topReq.m_skipReadonly = skipReadOnly;
        QueueJobRequest(AZStd::move(topReq));
    }

    void PerforceComponent::RequestRenameBulk(const char* sourcePathFull, const char* destPathFull, const SourceControlResponseCallbackBulk& respCallback)
    {
        RequestRenameBulkExtended(sourcePathFull, destPathFull, false, respCallback);
    }

    void PerforceComponent::RequestRenameBulkExtended(const char* sourcePathFull, const char* destPathFull, bool skipReadOnly, const SourceControlResponseCallbackBulk& respCallback)
    {
        AZ_Assert(sourcePathFull, "Must specify a source file path");
        AZ_Assert(destPathFull, "Must specify a destination file path");
        AZ_Assert(respCallback, "Must specify a callback!");

        PerforceJobRequest topReq(PerforceJobRequest::PJR_RenameBulk, sourcePathFull, respCallback);
        topReq.m_targetPath = destPathFull;
        topReq.m_skipReadonly = skipReadOnly;
        QueueJobRequest(AZStd::move(topReq));
    }

    void PerforceComponent::EnableSourceControl(bool enable)
    {
        m_testTrust = enable;
        m_testConnection = enable;
        m_resolveKey = enable;

        m_offlineMode = !enable;
        m_WorkerSemaphore.release(1);
    }

    void PerforceComponent::EnableTrust(bool enable, AZStd::string fingerprint)
    {
        s_perforceConn->m_command.ExecuteTrust(enable, fingerprint);
        m_trustedKey = IsTrustKeyValid();
        m_waitingOnTrust = false;

        // If our key is now valid, prompt user if it expires / becomes invalid
        // If our key is now invalid, wait until we receive a bus message to prompt user
        m_resolveKey = m_trustedKey ? true : false;
        m_WorkerSemaphore.release(1);
    }

    void PerforceComponent::SetConnectionSetting(const char* key, const char* value, const SourceControlSettingCallback& respCallBack)
    {
        PerforceSettingResult result;
        result.m_callback = respCallBack;
        if (ExecuteAndParseSet(key, value))
        {
            result.UpdateSettingInfo(s_perforceConn->m_command.GetOutputValue(key));
        }
        QueueSettingResponse(result);
    }

    void PerforceComponent::GetConnectionSetting(const char* key, const SourceControlSettingCallback& respCallBack)
    {
        PerforceSettingResult result;
        result.m_callback = respCallBack;
        if (ExecuteAndParseSet(nullptr, nullptr))
        {
            result.UpdateSettingInfo(s_perforceConn->m_command.GetOutputValue(key));
        }
        QueueSettingResponse(result);
    }

    void PerforceComponent::OpenSettings()
    {
        PerforceSettings dialog;

        if (dialog.exec() == QDialog::Accepted)
        {
            dialog.Apply();
        }
    }

    SourceControlFileInfo PerforceComponent::GetFileInfo(const char* filePath)
    {
        SourceControlFileInfo newInfo;
        newInfo.m_status = SCS_OpSuccess;
        newInfo.m_filePath = filePath;

        if (AZ::IO::SystemFile::IsWritable(filePath) || !AZ::IO::SystemFile::Exists(filePath))
        {
            newInfo.m_flags |= SCF_Writeable;
        }

        if (!CheckConnectivityForAction("file info", filePath))
        {
            newInfo.m_status = m_trustedKey ? SCS_ProviderIsDown : SCS_CertificateInvalid;
            return newInfo;
        }

        bool sourceAwareFile = false;
        if (!ExecuteAndParseFstat(filePath, sourceAwareFile))
        {
            if (!sourceAwareFile)
            {
                AZ_TracePrintf(SCC_WINDOW, "Perforce - File is not tracked\n");
                return newInfo;
            }

            newInfo.m_status = SCS_ProviderError;
            return newInfo;
        }

        bool newFileAfterDeletionAtHead = s_perforceConn->m_command.NewFileAfterDeletedRev();
        if (!newFileAfterDeletionAtHead)
        {
            if (s_perforceConn->m_command.GetHeadRevision() != s_perforceConn->m_command.GetHaveRevision())
            {
                newInfo.m_flags |= SCF_OutOfDate;
                AZ_TracePrintf(SCC_WINDOW, "Perforce - File is out of date\n");
            }
        }

        if (!s_perforceConn->m_command.ExclusiveOpen())
        {
            newInfo.m_flags |= SCF_MultiCheckOut;
        }

        bool openByOthers = s_perforceConn->m_command.IsOpenByOtherUsers();
        if (openByOthers)
        {
            newInfo.m_flags |= SCF_OtherOpen;
            newInfo.m_StatusUser = s_perforceConn->m_command.GetOtherUserCheckedOut();
        }

        if (s_perforceConn->m_command.IsOpenByCurrentUser())
        {
            newInfo.m_flags |= SCF_Tracked | SCF_OpenByUser;
            if (s_perforceConn->m_command.CurrentActionIsAdd())
            {
                newInfo.m_flags |= SCF_PendingAdd;
                AZ_TracePrintf(SCC_WINDOW, "Perforce - File is marked for add\n");
            }
            else if (s_perforceConn->m_command.CurrentActionIsDelete())
            {
                newInfo.m_flags |= SCF_PendingDelete;
                AZ_TracePrintf(SCC_WINDOW, "Perforce - File is marked for delete\n");
            }
            else
            {
                AZ_TracePrintf(SCC_WINDOW, "Perforce - File is checked out by you\n");
                if (openByOthers)
                {
                    AZ_TracePrintf(SCC_WINDOW, "          and Others\n");
                }
            }
        }
        else if (openByOthers)
        {
            newInfo.m_flags |= SCF_Tracked;
            AZ_TracePrintf(SCC_WINDOW, "Perforce - File is opened by %s\n", newInfo.m_StatusUser.c_str());
        }
        else if (newFileAfterDeletionAtHead)
        {
            AZ_TracePrintf(SCC_WINDOW, "Perforce - File is deleted at head revision\n");
        }
        else
        {
            newInfo.m_flags |= SCF_Tracked;
            AZ_TracePrintf(SCC_WINDOW, "Perforce - File is Checked In\n");
        }

        AZ_TracePrintf(SCC_WINDOW, "Perforce - Received file info %s\n\n", newInfo.m_filePath.c_str());
        return newInfo;
    }

    bool PerforceComponent::RequestEdit(const char* filePath, bool allowMultiCheckout)
    {
        if (!CheckConnectivityForAction("checkout", filePath))
        {
            return false;
        }

        return ExecuteEdit(filePath, allowMultiCheckout, true);
    }

    bool PerforceComponent::RequestEditBulk(const AZStd::unordered_set<AZStd::string>& fullFilePath, bool allowMultiCheckout)
    {
        if (!CheckConnectivityForAction("checkout", fullFilePath.begin()->c_str()))
        {
            return false;
        }

        return ExecuteEditBulk(fullFilePath, allowMultiCheckout, true);
    }

    bool PerforceComponent::RequestDelete(const char* filePath)
    {
        if (!CheckConnectivityForAction("delete", filePath))
        {
            return false;
        }

        return ExecuteDelete(filePath);
    }

    bool PerforceComponent::RequestDeleteBulk(const char* filePath)
    {
        if (!CheckConnectivityForAction("delete", filePath))
        {
            return false;
        }

        return ExecuteDeleteBulk(filePath);
    }

    bool PerforceComponent::RequestLatest(const char* filePath)
    {
        if (!CheckConnectivityForAction("sync", filePath))
        {
            return false;
        }

        return ExecuteSync(filePath);
    }

    bool PerforceComponent::RequestRevert(const char* filePath)
    {
        if (!CheckConnectivityForAction("revert", filePath))
        {
            return false;
        }

        return ExecuteRevert(filePath);
    }

    bool PerforceComponent::RequestRename(const char* sourcePath, const char* destPath)
    {
        if (!CheckConnectivityForAction("rename", sourcePath))
        {
            return false;
        }

        return ExecuteMove(sourcePath, destPath);
    }

    bool PerforceComponent::RequestRenameBulk(const char* sourcePathFull, const char* destPathFull)
    {
        if (!CheckConnectivityForAction("rename", sourcePathFull))
        {
            return false;
        }

        return ExecuteMoveBulk(sourcePathFull, destPathFull);
    }

    // claim changed file just moves the file to the new changelist - it assumes its already on your changelist
    bool PerforceComponent::ClaimChangedFile(const char* path, int changeListNumber)
    {
        AZStd::string changeListStr = AZStd::string::format("claim changed file to changelist %i", changeListNumber);
        if (!CheckConnectivityForAction(changeListStr.c_str(), path))
        {
            return false;
        }

        changeListStr = AZStd::to_string(changeListNumber);
        s_perforceConn->m_command.ExecuteClaimChangedFile(path, changeListStr);
        return CommandSucceeded();
    }

    bool PerforceComponent::ExecuteAdd(const char* filePath)
    {
        bool sourceAwareFile = false;
        if (!ExecuteAndParseFstat(filePath, sourceAwareFile))
        {
            if (sourceAwareFile)
            {
                return false;
            }
        }

        int changeListNumber = GetOrCreateOurChangelist();
        if (changeListNumber <= 0)
        {
            return false;
        }

        AZStd::string changeListNumberStr = AZStd::to_string(changeListNumber);

        s_perforceConn->m_command.ExecuteAdd(changeListNumberStr, filePath);
        if (!CommandSucceeded())
        {
            return false;
        }

        // Interesting result of our intention based system -
        // The perforce Add operation does not remove 'read only' status from a file; however,
        // since our intention is to say "place this file in a state ready for work," we need
        // to remove read only status (otherwise subsequent save operations would fail)
        AZ::IO::SystemFile::SetWritable(filePath, true);

        ExecuteAndParseFstat(filePath, sourceAwareFile);
        if (!s_perforceConn->m_command.CurrentActionIsAdd())
        {
            AZ_Warning(SCC_WINDOW, false, "Perforce - Unable to add file '%s' (Perforce is connected but file was not added)\n", filePath);
            return false;
        }

        AZ_TracePrintf(SCC_WINDOW, "Perforce - Successfully added file %s\n", filePath);
        return true;
    }

    bool PerforceComponent::ExecuteEdit(const char* filePath, bool allowMultiCheckout, bool allowAdd)
    {
        bool sourceAwareFile = false;
        if (!ExecuteAndParseFstat(filePath, sourceAwareFile))
        {
            if (sourceAwareFile)
            {
                return false;
            }

            if (!AZ::IO::SystemFile::Exists(filePath))
            {
                sourceAwareFile = false;
            }
        }

        if (sourceAwareFile && (s_perforceConn->m_command.IsOpenByOtherUsers() && !allowMultiCheckout))
        {
            AZ_Warning(SCC_WINDOW, false, "Perforce - Unable to edit file %s, it's already checked out by %s\n",
                filePath, s_perforceConn->m_command.GetOtherUserCheckedOut().c_str());
            return false;
        }

        if (!sourceAwareFile)
        {
            if (allowAdd)
            {
                if (!ExecuteAdd(filePath))
                {
                    AZ_Warning(SCC_WINDOW, false, "Perforce - Unable to add file %s\n", filePath);
                    // if the file is writable, this does not count as an error as we are likely
                    // simply outside the workspace.
                    return AZ::IO::SystemFile::IsWritable(filePath);
                }

                return true;
            }

            return false;
        }

        int changeListNumber = GetOrCreateOurChangelist();
        if (changeListNumber <= 0)
        {
            return false;
        }

        AZStd::string changeListNumberStr = AZStd::to_string(changeListNumber);

        if (s_perforceConn->m_command.IsOpenByCurrentUser())
        {
            if (s_perforceConn->m_command.CurrentActionIsEdit() || s_perforceConn->m_command.CurrentActionIsAdd())
            {
                // Claim the file to our working changelist if it's in the default changelist.
                const AZStd::string& currentCL = s_perforceConn->m_command.GetCurrentChangelistNumber();
                if (currentCL == "default")
                {
                    if (!ClaimChangedFile(filePath, changeListNumber))
                    {
                        AZ_TracePrintf(SCC_WINDOW, "Perforce - Unable to move file %s to changelist %s\n", filePath, changeListNumberStr.c_str());
                    }
                }

                AZ_TracePrintf(SCC_WINDOW, "Perforce - File %s is already checked out by you\n", filePath);
                return true;
            }
            else if (s_perforceConn->m_command.CurrentActionIsDelete())
            {
                // we have it checked out for delete by us - we need to revert the delete before we can check it out
                AZ_TracePrintf(SCC_WINDOW, "Perforce - Reverting deleted file %s in order to check it out\n", filePath);
                if (!ExecuteRevert(filePath))
                {
                    AZ_Warning(SCC_WINDOW, false, "Perforce - Failed to revert deleted file %s\n", filePath);
                    return false;
                }
            }
            else
            {
                AZ_Warning(SCC_WINDOW, false, "Perforce - Requesting edit on file %s opened by current user, but not marked for add, edit or delete", filePath);
                return false;
            }
        }
        else if (s_perforceConn->m_command.HeadActionIsDelete() && allowAdd)
        {
            // it's currently deleted - we need to fix that before we can check it out
            // this Add operation is all we need since it puts the file under our control
            AZ_TracePrintf(SCC_WINDOW, "Perforce - re-Add fully deleted file %s in order to check it out\n", filePath);
            if (!ExecuteAdd(filePath))
            {
                AZ_Warning(SCC_WINDOW, false, "Perforce - Failed to revert deleted file %s in order to check it out\n", filePath);
                return false;
            }
            return true;
        }

        // If we get here it means it's not opened by us
        // attempt check out.
        s_perforceConn->m_command.ExecuteEdit(changeListNumberStr, filePath);
        if (!CommandSucceeded())
        {
            return false;
        }

        if (s_perforceConn->m_command.NeedsReopening() && !ClaimChangedFile(filePath, changeListNumber))
        {
            AZ_Warning(SCC_WINDOW, false, "Perforce - Failed to reopen file %s\n", filePath);
            return false;
        }

        ExecuteAndParseFstat(filePath, sourceAwareFile);
        if (s_perforceConn->m_command.IsMarkedForAdd())
        {
            AZ_Warning(SCC_WINDOW, false, "Perforce - Can't edit file %s, already marked for add\n", filePath);
            return false;
        }

        if (!s_perforceConn->m_command.CurrentActionIsEdit())
        {
            AZ_TracePrintf(SCC_WINDOW, "Perforce - Failed to edit file %s", filePath);
            return false;
        }

        AZ_TracePrintf(SCC_WINDOW, "Perforce - Checked out file %s\n", filePath);
        return true;
    }

    bool PerforceComponent::ExecuteEditBulk(const AZStd::unordered_set<AZStd::string>& filePaths, bool allowMultiCheckout, bool allowAdd)
    {
        AZStd::vector<PerforceMap> commandMap;
        ExecuteAndParseFstat(filePaths, commandMap);

        for (const PerforceMap& map : commandMap)
        {
            if(s_perforceConn->m_command.IsOpenByOtherUsers(&map) && !allowMultiCheckout)
            {
                AZ_Warning(SCC_WINDOW, false, "Perforce - Unable to edit file %s, it's already checked out by %s\n",
                    s_perforceConn->m_command.GetOutputValue("clientFile", &map).c_str(), s_perforceConn->m_command.GetOtherUserCheckedOut(&map).c_str());
                return false;
            }
        }

        int changeListNumber = GetOrCreateOurChangelist();
        if (changeListNumber <= 0)
        {
            return false;
        }

        AZStd::string changeListNumberStr = AZStd::to_string(changeListNumber);

        if (allowAdd)
        {
            s_perforceConn->m_command.ExecuteAdd(changeListNumberStr, filePaths);
        }

        s_perforceConn->m_command.ExecuteEdit(changeListNumberStr, filePaths);

        return true;
    }

    bool PerforceComponent::ExecuteDelete(const char* filePath)
    {
        bool sourceAwareFile = false;
        if (!ExecuteAndParseFstat(filePath, sourceAwareFile) || !sourceAwareFile)
        {
            return false;
        }

        if (s_perforceConn->m_command.IsOpenByCurrentUser())
        {
            if (!s_perforceConn->m_command.CurrentActionIsDelete())
            {
                if (!ExecuteRevert(filePath))
                {
                    AZ_Warning(SCC_WINDOW, false, "Perforce - Unable to revert file %s\n", filePath);
                    return false;
                }
            }
        }

        if (!s_perforceConn->m_command.CurrentActionIsDelete() || !s_perforceConn->m_command.HeadActionIsDelete())
        {
            /* not already being deleted by current or head action - delete destructively (DANGER) */
            int changeListNumber = GetOrCreateOurChangelist();
            if (changeListNumber <= 0)
            {
                return false;
            }

            AZStd::string changeListNumberStr = AZStd::to_string(changeListNumber);

            s_perforceConn->m_command.ExecuteDelete(changeListNumberStr, filePath);
            if (!CommandSucceeded())
            {
                AZ_Warning(SCC_WINDOW, false, "Perforce - Failed to delete file %s\n", filePath);
                return false;
            }

            AZ_TracePrintf(SCC_WINDOW, "Perforce - Deleted file %s\n", filePath);
        }

        // this will execute even if the file isn't part of SCS yet
        AZ::IO::SystemFile::Delete(filePath);

        return true;
    }

    bool PerforceComponent::ExecuteDeleteBulk(const char* filePath)
    {
        int changeListNumber = GetOrCreateOurChangelist();
        if (changeListNumber <= 0)
        {
            return false;
        }

        AZStd::string changeListNumberStr = AZStd::to_string(changeListNumber);

        s_perforceConn->m_command.ExecuteDelete(changeListNumberStr, filePath);
        if (!CommandSucceeded())
        {
            AZ_Warning(SCC_WINDOW, false, "Perforce - Failed to delete file %s\n", filePath);
            return false;
        }

        AZ_TracePrintf(SCC_WINDOW, "Perforce - Deleted file %s\n", filePath);
        return true;
    }

    bool PerforceComponent::ExecuteSync(const char* filePath)
    {
        bool sourceAwareFile = false;
        if (!ExecuteAndParseFstat(filePath, sourceAwareFile))
        {
            AZ_TracePrintf(SCC_WINDOW, "Perforce - Unable to get latest on file '%s'\n", filePath);
            return false;
        }

        // If we're working on a new file - unknown to p4 or a 'new file' after a deletion
        if (!sourceAwareFile || s_perforceConn->m_command.NewFileAfterDeletedRev())
        {
            return true;
        }

        if (AZ::IO::SystemFile::IsWritable(filePath) && !s_perforceConn->m_command.IsOpenByCurrentUser())
        {
            AZ_TracePrintf(SCC_WINDOW, "Perforce - Unable to get latest on file '%s' (File is not checked out but has changes locally.  Please reconcile offline work!)\n", filePath);
            return false;
        }

        if (s_perforceConn->m_command.GetHeadRevision() == s_perforceConn->m_command.GetHaveRevision())
        {
            return true;
        }

        if (s_perforceConn->m_command.IsOpenByCurrentUser())
        {
            AZ_TracePrintf(SCC_WINDOW, "Perforce - Unable to get latest on file '%s' (File is open by current user, but not at head revision!)\n", filePath);
            return false;
        }

        s_perforceConn->m_command.ExecuteSync(filePath);
        return CommandSucceeded();
    }

    bool PerforceComponent::ExecuteRevert(const char* filePath)
    {
        bool sourceAwareFile = false;
        if (!ExecuteAndParseFstat(filePath, sourceAwareFile) || !sourceAwareFile)
        {
            AZ_TracePrintf(SCC_WINDOW, "Perforce - Unable to revert file '%s' (Perforce is connected but file is not able to be reverted)\n", filePath);
            return false;
        }

        if (!s_perforceConn->m_command.IsOpenByCurrentUser())
        {
            AZ_TracePrintf(SCC_WINDOW, "Perforce - Unable to revert file '%s' (Perforce is connected but file is not open by current user)\n", filePath);
            return false;
        }

        s_perforceConn->m_command.ExecuteRevert(filePath);
        if (!CommandSucceeded())
        {
            return false;
        }

        ExecuteAndParseFstat(filePath, sourceAwareFile);
        if (s_perforceConn->m_command.IsOpenByCurrentUser())
        {
            AZ_Warning(SCC_WINDOW, false, "Perforce - Unable to revert file '%s' (Perforce is connected but file was not reverted)\n", filePath);
            return false;
        }

        AZ_TracePrintf(SCC_WINDOW, "Perforce - Revert succeeded for %s\n", filePath);
        return true;
    }

    bool PerforceComponent::ExecuteMove(const char* sourcePath, const char* destPath)
    {
        if (!AZ::IO::SystemFile::Exists(sourcePath))
        {
            AZ_TracePrintf(SCC_WINDOW, "Perforce - Unable to move file '%s' (source file does not exist)\n", sourcePath);
            return false;
        }

        // Blocked output - File exists or SourceAwareFile and File is not deleted at head
        if (AZ::IO::SystemFile::Exists(destPath))
        {
            AZ_TracePrintf(SCC_WINDOW, "Perforce - Unable to move file '%s' (destination file '%s' already exists)\n", sourcePath, destPath);
            return false;
        }

        bool sourceAwareFile = false;
        ExecuteAndParseFstat(destPath, sourceAwareFile);
        if (sourceAwareFile)
        {
            if (!s_perforceConn->m_command.HeadActionIsDelete())
            {
                AZ_TracePrintf(SCC_WINDOW, "Perforce - Unable to move file '%s' (destination file '%s' exists in perforce)\n", sourcePath, destPath);
                return false;
            }
        }

        // Make sure we're not going to trash any data.

        if (!ExecuteEdit(sourcePath, false, false))
        {
            AZ_TracePrintf(SCC_WINDOW, "Perforce - Unable to mark file '%s' for edit prior to move operation\n", sourcePath);
            return false;
        }

        int changeListNumber = GetOrCreateOurChangelist();
        if (changeListNumber <= 0)
        {
            return false;
        }

        AZStd::string changeListNumberStr = AZStd::to_string(changeListNumber);
        s_perforceConn->m_command.ExecuteMove(changeListNumberStr, sourcePath, destPath);
        return CommandSucceeded();
    }

    bool PerforceComponent::ExecuteMoveBulk(const char* sourcePath, const char* destPath)
    {
        int changeListNumber = GetOrCreateOurChangelist();
        if (changeListNumber <= 0)
        {
            return false;
        }

        AZStd::string changeListNumberStr = AZStd::to_string(changeListNumber);

        s_perforceConn->m_command.ExecuteEdit(changeListNumberStr, sourcePath);
        s_perforceConn->m_command.ExecuteMove(changeListNumberStr, sourcePath, destPath);

        return true;
    }

    void PerforceComponent::QueueJobRequest(PerforceJobRequest&& jobRequest)
    {
        AZStd::lock_guard<AZStd::mutex> locker(m_WorkerQueueMutex);
        m_workerQueue.push(AZStd::move(jobRequest));
        m_WorkerSemaphore.release(1);
    }

    void PerforceComponent::QueueSettingResponse(const PerforceSettingResult& result)
    {
        AZStd::lock_guard<AZStd::mutex> locker(m_SettingsQueueMutex);
        m_settingsQueue.push(AZStd::move(result));
        AZ::TickBus::QueueFunction(&PerforceComponent::ProcessResultQueue, this);
    }

    bool PerforceComponent::CheckConnectivityForAction(const char* actionDesc, const char* filePath) const
    {
        (void)actionDesc;
        AZ_Assert(m_ProcessThreadID != AZStd::thread::id(), "The perforce worker thread has not started.");
        AZ_Assert(AZStd::this_thread::get_id() == m_ProcessThreadID, "You may only call this function from the perforce worker thread.");

        AZStd::string fileMessage = AZStd::string::format(" for file %s", filePath);
        AZ_TracePrintf(SCC_WINDOW, "Perforce - Requested %s%s\n", actionDesc, filePath ? fileMessage.c_str() : "");

        if (m_connectionState != SourceControlState::Active)
        {
            fileMessage = AZStd::string::format(" file %s", filePath);
            AZ_Warning(SCC_WINDOW, false, "Perforce - Unable to %s%s (Perforce is not connected)\n", actionDesc, filePath ? fileMessage.c_str() : "");
            return false;
        }

        return true;
    }

    bool PerforceComponent::ExecuteAndParseFstat(const char* filePath, bool& sourceAwareFile)
    {
        s_perforceConn->m_command.ExecuteFstat(filePath);
        sourceAwareFile = CommandSucceeded();

        AZStd::lock_guard<AZStd::mutex> locker(s_perforceConn->m_command.m_commandMutex);
        return ParseOutput(s_perforceConn->m_command.m_commandOutputMap, s_perforceConn->m_command.m_rawOutput.outputResult);
    }

    bool PerforceComponent::ExecuteAndParseFstat(const char* filePath, AZStd::vector<PerforceMap>& commandMap) const
    {
        s_perforceConn->m_command.ExecuteFstat(filePath);

        AZStd::lock_guard<AZStd::mutex> locker(s_perforceConn->m_command.m_commandMutex);
        return ParseDuplicateOutput(commandMap, s_perforceConn->m_command.m_rawOutput.outputResult);
    }

    bool PerforceComponent::ExecuteAndParseFstat(const AZStd::unordered_set<AZStd::string>& filePaths, AZStd::vector<PerforceMap>& commandMap) const
    {
        s_perforceConn->m_command.ExecuteFstat(filePaths);

        AZStd::lock_guard<AZStd::mutex> locker(s_perforceConn->m_command.m_commandMutex);
        return ParseDuplicateOutput(commandMap, s_perforceConn->m_command.m_rawOutput.outputResult);
    }

    bool PerforceComponent::ExecuteAndParseSet(const char* key, const char* value) const
    {
        // a valid key will attempt to set the setting
        // a null value will clear the setting
        if (key)
        {
            AZStd::string keystr = key;
            AZStd::string valstr = value;
            s_perforceConn->m_command.ExecuteSet(keystr, valstr);
        }

        s_perforceConn->m_command.ExecuteSet();
        if (s_perforceConn->CommandHasFailed())
        {
            return false;
        }

        AZStd::lock_guard<AZStd::mutex> locker(s_perforceConn->m_command.m_commandMutex);
        return ParseOutput(s_perforceConn->m_command.m_commandOutputMap, s_perforceConn->m_command.m_rawOutput.outputResult, "=");
    }

    bool PerforceComponent::CommandSucceeded()
    {
        // This is misleading - FileExists only returns false if the phrase 'no such file(s)'
        // is present in output.  Therefore, this is safe to call even on perforce commands
        // that are not directly related to files (p4 set / p4 change / p4 describe / etc)
        if (!s_perforceConn->m_command.FileExists())
        {
            return false;
        }

        if (s_perforceConn->CommandHasFailed())
        {
            m_testTrust = true;
            m_testConnection = true;
            return false;
        }
        return true;
    }

    bool PerforceComponent::UpdateTrust()
    {
        AZ_Assert(m_ProcessThreadID != AZStd::thread::id(), "The perforce worker thread has not started.");
        AZ_Assert(AZStd::this_thread::get_id() == m_ProcessThreadID, "You may only call this function from the perforce worker thread.");

        if (m_testTrust)
        {
            TestConnectionTrust(m_resolveKey);
        }

        if (!m_trustedKey && m_waitingOnTrust)
        {
            return false;
        }

        return true;
    }

    bool PerforceComponent::IsTrustKeyValid() const
    {
        s_perforceConn->m_command.ExecuteInfo();
        if (s_perforceConn->CommandHasError() && s_perforceConn->m_command.HasTrustIssue())
        {
            return false;
        }
        return true;
    }

    void PerforceComponent::VerifyP4PortIsSet()
    {
        AZ_Assert(m_ProcessThreadID != AZStd::thread::id(), "The perforce worker thread has not started.");
        AZ_Assert(AZStd::this_thread::get_id() == m_ProcessThreadID, "You may only call this function from the perforce worker thread.");

        if (!s_perforceConn)
        {
            s_perforceConn = new PerforceConnection();
        }

        if (m_testConnection)
        {
            bool p4PortSet = false;
            if (ExecuteAndParseSet(nullptr, nullptr))
            {
                p4PortSet = !s_perforceConn->m_command.GetOutputValue("P4PORT").empty();
            }

            if (!p4PortSet)
            {
                // The warnings need to show up repeatedly, as the user might be turning on and off perforce and configuring it.
                if (!s_perforceConn->CommandApplicationFound())
                {
                    AZ_Warning(SCC_WINDOW, false, "Perforce - p4 executable not found on path, will not use Perforce source control!\n");
                }
                else
                {
                    AZ_Warning(SCC_WINDOW, false, "Perforce - p4 executable found, but P4PORT (server address) is not set, Perforce not available!\n");
                }
                // Disable any further connection status testing
                m_testTrust = false;
                m_testConnection = false;
                m_validConnection = false;
            }
        }
    }

    void PerforceComponent::TestConnectionTrust(bool attemptResolve)
    {
        AZ_Assert(m_ProcessThreadID != AZStd::thread::id(), "The perforce worker thread has not started.");
        AZ_Assert(AZStd::this_thread::get_id() == m_ProcessThreadID, "You may only call this function from the perforce worker thread.");

        m_trustedKey = IsTrustKeyValid();
        if (!m_trustedKey && attemptResolve)
        {
            if (SourceControlNotificationBus::HasHandlers())
            {
                m_waitingOnTrust = true;
                AZStd::string errOutput = s_perforceConn->m_command.m_rawOutput.errorResult;

                // tokenize the buffer into line by line key/value pairs
                AZStd::vector<AZStd::string> tokens;
                AzFramework::StringFunc::Tokenize(errOutput.c_str(), tokens, "\r\n");

                // the fingerprint is on the 4th line of error output
                static const AZ::u32 fingerprintIdx = 3;
                if (tokens.size() > fingerprintIdx)
                {
                    AZStd::string fingerprint = tokens[fingerprintIdx];

                    if (AZ::TickBus::IsFunctionQueuing())
                    {
                        // Push to the main thread for convenience.
                        AZStd::function<void()> trustNotify = [fingerprint]()
                        {
                            SourceControlNotificationBus::Broadcast(&SourceControlNotificationBus::Events::RequestTrust, fingerprint.c_str());
                        };

                        AZ::TickBus::QueueFunction(trustNotify);
                    }
                }
            }
            else
            {
                // No one is listening.  No need to hold up the worker thread.
                m_waitingOnTrust = false;
            }
        }

        m_testTrust = false;
    }

    bool PerforceComponent::IsConnectionValid() const
    {
        s_perforceConn->m_command.ExecuteTicketStatus();

        if (!s_perforceConn->CommandApplicationFound())
        {
            return false;
        }

        if (s_perforceConn->CommandHasError())
        {
            return false;
        }

        if (!CacheClientConfig())
        {
            return false;
        }

        return true;
    }

    bool PerforceComponent::CacheClientConfig() const
    {
        s_perforceConn->m_command.ExecuteInfo();
        if (s_perforceConn->CommandHasFailed() || !ParseOutput(s_perforceConn->m_infoResultMap, s_perforceConn->m_command.m_rawOutput.outputResult))
        {
            return false;
        }

        // P4 workaround, according to P4 documentation and forum posts, in order to keep P4 info fast it may
        // not do a server query to fetch the userName field.
        // In that case, we will do an additional p4 info -s and update the original m_infoResultMap with the
        // retrieved user name.
        if (s_perforceConn->GetUser().compare("*unknown*") == 0)
        {
            PerforceMap shortInfoMap;
            s_perforceConn->m_command.ExecuteShortInfo();
            if (s_perforceConn->CommandHasFailed() || !ParseOutput(shortInfoMap, s_perforceConn->m_command.m_rawOutput.outputResult))
            {
                return false;
            }

            s_perforceConn->m_infoResultMap["userName"] = shortInfoMap["userName"];
        }

        if (s_perforceConn->GetUser().empty() || s_perforceConn->GetClientName().empty())
        {
            AZ_Warning(SCC_WINDOW, false, "Perforce - Your client or user is empty, Perforce not available!\n");
            return false;
        }

        if (s_perforceConn->GetClientName().compare("*unknown*") == 0)
        {
            AZ_Warning(SCC_WINDOW, false, "Perforce - client spec not found, Perforce not available!\n");
            return false;
        }

        if (s_perforceConn->GetClientRoot().empty())
        {
            AZ_Warning(SCC_WINDOW, false, "Perforce - Your workspace root is empty, Perforce not available!\n");
            return false;
        }

        if (s_perforceConn->GetServerAddress().empty() || s_perforceConn->GetServerUptime().empty())
        {
            AZ_Warning(SCC_WINDOW, false, "Perforce - Could not get server information, Perforce not available!\n");
            return false;
        }

        AZ_TracePrintf(SCC_WINDOW, "Perforce - Connected, User: %s | Client: %s\n", s_perforceConn->GetUser().c_str(), s_perforceConn->GetClientName().c_str());
        return true;
    }

    void PerforceComponent::TestConnectionValid()
    {
        AZ_Assert(m_ProcessThreadID != AZStd::thread::id(), "The perforce worker thread has not started.");
        AZ_Assert(AZStd::this_thread::get_id() == m_ProcessThreadID, "You may only call this function from the perforce worker thread.");

        if (!s_perforceConn)
        {
            s_perforceConn = new PerforceConnection();
        }

        m_validConnection = IsConnectionValid();
        m_testConnection = false;
    }

    bool PerforceComponent::UpdateConnectivity()
    {
        AZ_Assert(m_ProcessThreadID != AZStd::thread::id(), "The perforce worker thread has not started.");
        AZ_Assert(AZStd::this_thread::get_id() == m_ProcessThreadID, "You may only call this function from the perforce worker thread.");

        SourceControlState prevState = m_connectionState;
        if (m_testConnection)
        {
            TestConnectionValid();
        }

        SourceControlState currentState = SourceControlState::Disabled;
        if (!m_offlineMode)
        {
            currentState = (m_trustedKey && m_validConnection) ? SourceControlState::Active : SourceControlState::ConfigurationInvalid;
        }

        m_connectionState = currentState;

        if (m_connectionState != prevState)
        {
            switch (m_connectionState)
            {
            case SourceControlState::Disabled:
                AZ_TracePrintf(SCC_WINDOW, "Perforce disabled\n");
                break;
            case SourceControlState::ConfigurationInvalid:
                AZ_TracePrintf(SCC_WINDOW, "Perforce configuration invalid\n");
                break;
            case SourceControlState::Active:
                AZ_TracePrintf(SCC_WINDOW, "Perforce connected\n");
                break;
            }

            if (AZ::TickBus::IsFunctionQueuing())
            {
                // Push to the main thread for convenience.
                AZStd::function<void()> connectivityNotify = [currentState]()
                {
                    SourceControlNotificationBus::Broadcast(&SourceControlNotificationBus::Events::ConnectivityStateChanged, currentState);
                };
                AZ::TickBus::QueueFunction(connectivityNotify);
            }
        }

        return true;
    }

    void PerforceComponent::DropConnectivity()
    {
        AZ_Assert(m_ProcessThreadID != AZStd::thread::id(), "The perforce worker thread has not started.");
        AZ_Assert(AZStd::this_thread::get_id() == m_ProcessThreadID, "You may only call this function from the perforce worker thread.");

        if (!s_perforceConn)
        {
            return;
        }

        delete s_perforceConn;
        s_perforceConn = nullptr;

        AZ_TracePrintf(SCC_WINDOW, "Perforce connectivity dropped\n");
    }

    int PerforceComponent::GetOrCreateOurChangelist()
    {
        if (!CheckConnectivityForAction("list changelists", nullptr))
        {
            return 0;
        }

        int changelistNum = FindExistingChangelist();
        if (changelistNum > 0)
        {
            return changelistNum;
        }

        AzFramework::ProcessWatcher* pWatcher = s_perforceConn->m_command.ExecuteNewChangelistInput();
        AZ_Assert(pWatcher, "No process found for p4!");
        if (!pWatcher)
        {
            return 0;
        }

        AzFramework::ProcessCommunicator* pCommunicator = pWatcher->GetCommunicator();
        AZ_Assert(pCommunicator, "No communicator found for p4!");
        if (!pCommunicator)
        {
            return 0;
        }

        AZStd::string changelistForm = s_perforceConn->m_command.CreateChangelistForm(s_perforceConn->GetClientName(), s_perforceConn->GetUser(), m_autoChangelistDescription);
        AZ_TracePrintf(SCC_WINDOW, "Perforce - Changelist Form\n%s", changelistForm.c_str());

        pCommunicator->WriteInput(changelistForm.data(), static_cast<AZ::u32>(changelistForm.size()));
        if (!pWatcher->WaitForProcessToExit(30))
        {
            pWatcher->TerminateProcess(0);
        }
        delete pWatcher;

        changelistNum = FindExistingChangelist();
        if (changelistNum > 0)
        {
            AZ_TracePrintf(SCC_WINDOW, "Perforce - Changelist number is %i\n", changelistNum);
        }
        else
        {
            AZ_TracePrintf(SCC_WINDOW, "Perforce - %s changelist could not be found, problem creating changelist!\n", m_autoChangelistDescription.c_str());
        }

        return changelistNum;
    }

    int PerforceComponent::FindExistingChangelist()
    {
        s_perforceConn->m_command.ExecuteShowChangelists(s_perforceConn->GetUser(), s_perforceConn->GetClientName());
        if (!CommandSucceeded())
        {
            return 0;
        }

        if (ParseDuplicateOutput(s_perforceConn->m_command.m_commandOutputMapList, s_perforceConn->m_command.m_rawOutput.outputResult))
        {
            int foundChangelist = 0;
            AZStd::vector<PerforceMap>::iterator perforceIterator = s_perforceConn->m_command.FindMapWithPartiallyMatchingValueForKey("desc", m_autoChangelistDescription);
            if (perforceIterator != nullptr)
            {
                if (s_perforceConn->m_command.OutputKeyExists("change", perforceIterator))
                {
                    foundChangelist = atoi(s_perforceConn->m_command.GetOutputValue("change", perforceIterator).c_str());
                }
            }

            if (foundChangelist > 0)
            {
                return foundChangelist;
            }
        }

        return 0;
    }

    void PerforceComponent::ThreadWorker()
    {
        m_ProcessThreadID = AZStd::this_thread::get_id();
        while (true)
        {
            // block until signaled:
            m_WorkerSemaphore.acquire();
            if (m_shutdownThreadSignal)
            {
                break; // abandon ship!
            }

            // Verify P4PORT is set before running any commands
            VerifyP4PortIsSet();

            // wait for trust issues to be resolved
            if (!UpdateTrust())
            {
                continue;
            }

            if (!UpdateConnectivity())
            {
                continue;
            }

            if (m_workerQueue.empty())
            {
                continue;
            }

            // if we get here, a job is waiting for us!  pop it off the queue (use the queue for as short a time as possible
            PerforceJobRequest topReq;
            {
                AZStd::lock_guard<AZStd::mutex> locker(m_WorkerQueueMutex);
                topReq = m_workerQueue.front();
                m_workerQueue.pop();
            }

            // execute the queued item
            if (!m_offlineMode)
            {
                ProcessJob(topReq);
            }
            else
            {
                ProcessJobOffline(topReq);
            }
        }
        DropConnectivity();
        m_ProcessThreadID = AZStd::thread::id();
    }

    void PerforceComponent::ProcessJob(const PerforceJobRequest& request)
    {
        AZ_Assert(m_ProcessThreadID != AZStd::thread::id(), "The perforce worker thread has not started.");
        AZ_Assert(AZStd::this_thread::get_id() == m_ProcessThreadID, "You may only call this function from the perforce worker thread.");

        PerforceJobResult resp;
        bool validPerforceRequest = true;
        switch (request.m_requestType)
        {
        case PerforceJobRequest::PJR_Stat:
        {
            resp.m_fileInfo = AZStd::move(GetFileInfo(request.m_requestPath.c_str()));
            resp.m_succeeded = resp.m_fileInfo.m_status == SCS_OpSuccess;
        }
        break;
        case PerforceJobRequest::PJR_StatBulk:
        {
            resp.m_bulkFileInfo = AZStd::move(GetBulkFileInfo(request.m_bulkFilePaths));
            resp.m_succeeded = !resp.m_bulkFileInfo.empty();
        }
        break;
        case PerforceJobRequest::PJR_Edit:
        {
            resp.m_succeeded = RequestEdit(request.m_requestPath.c_str(), request.m_allowMultiCheckout);
            resp.m_fileInfo = AZStd::move(GetFileInfo(request.m_requestPath.c_str()));
        }
        break;
        case PerforceJobRequest::PJR_EditBulk:
        {
            resp.m_succeeded = RequestEditBulk(request.m_bulkFilePaths, request.m_allowMultiCheckout);
            resp.m_bulkFileInfo = GetBulkFileInfo(request.m_bulkFilePaths);
        }
        break;
        case PerforceJobRequest::PJR_Delete:
        {
            resp.m_succeeded = RequestDelete(request.m_requestPath.c_str());
            resp.m_fileInfo = AZStd::move(GetFileInfo(request.m_requestPath.c_str()));
        }
        break;
        case PerforceJobRequest::PJR_DeleteBulk:
        {
            resp.m_succeeded = RequestDeleteBulk(request.m_requestPath.c_str());
            resp.m_bulkFileInfo = GetBulkFileInfo(request.m_requestPath.c_str());
        }
        break;
        case PerforceJobRequest::PJR_Revert:
        {
            resp.m_succeeded = RequestRevert(request.m_requestPath.c_str());
            resp.m_fileInfo = AZStd::move(GetFileInfo(request.m_requestPath.c_str()));
        }
        break;
        case PerforceJobRequest::PJR_Rename:
        {
            resp.m_succeeded = RequestRename(request.m_requestPath.c_str(), request.m_targetPath.c_str());
            resp.m_fileInfo = AZStd::move(GetFileInfo(request.m_targetPath.c_str()));
        }
        break;
        case PerforceJobRequest::PJR_RenameBulk:
        {
            resp.m_succeeded = RequestRenameBulk(request.m_requestPath.c_str(), request.m_targetPath.c_str());
            resp.m_bulkFileInfo = GetBulkFileInfo(request.m_targetPath.c_str());
        }
        break;
        case PerforceJobRequest::PJR_Sync:
        {
            resp.m_succeeded = RequestLatest(request.m_requestPath.c_str());
            resp.m_fileInfo = AZStd::move(GetFileInfo(request.m_requestPath.c_str()));
        }
        break;
        default:
        {
            validPerforceRequest = false;
            AZ_Assert(false, "Invalid type of perforce job request.");
        }
        break;
        }

        if (validPerforceRequest)
        {
            resp.m_callback = request.m_callback;
            resp.m_bulkCallback = request.m_bulkCallback;
            {
                AZStd::lock_guard<AZStd::mutex> locker(m_ResultQueueMutex);
                m_resultQueue.push(AZStd::move(resp));
                AZ::TickBus::QueueFunction(&PerforceComponent::ProcessResultQueue, this); // narrow events to the main thread.
            }
        }
    }

    AZStd::vector<SourceControlFileInfo> PerforceComponent::GetBulkFileInfo(const char* requestPath) const
    {
        return GetBulkFileInfo(AZStd::unordered_set<AZStd::string>{ requestPath });
    }

    AZStd::vector<SourceControlFileInfo> PerforceComponent::GetBulkFileInfo(const AZStd::unordered_set<AZStd::string>& requestPaths) const
    {
        AZStd::vector<SourceControlFileInfo> fileInfoList;
        AZStd::vector<PerforceMap> commandMap;
        ExecuteAndParseFstat(requestPaths, commandMap);

        for(const AZStd::string& filePath : requestPaths)
        {
            if (!s_perforceConn->m_command.FileExists(filePath.c_str()))
            {
                SourceControlFileInfo newInfo;

                newInfo.m_filePath = filePath;
                newInfo.m_status = SCS_OpSuccess;

                if (AZ::IO::SystemFile::IsWritable(filePath.c_str()) || !AZ::IO::SystemFile::Exists(filePath.c_str()))
                {
                    newInfo.m_flags |= SCF_Writeable;
                }

                fileInfoList.push_back(newInfo);
            }
        }

        for (const auto& map : commandMap)
        {
            auto fileItr = map.find("clientFile");

            if (fileItr != map.end())
            {
                SourceControlFileInfo newInfo;
                newInfo.m_filePath = fileItr->second;
                newInfo.m_status = SCS_OpSuccess;
                newInfo.m_flags |= SCF_Tracked;

                if(AZ::IO::SystemFile::IsWritable(newInfo.m_filePath.c_str()) || !AZ::IO::SystemFile::Exists(newInfo.m_filePath.c_str()))
                {
                    newInfo.m_flags |= SCF_Writeable;
                }

                bool newFileAfterDeletionAtHead = s_perforceConn->m_command.NewFileAfterDeletedRev(&map);
                if (!newFileAfterDeletionAtHead)
                {
                    if (s_perforceConn->m_command.GetHeadRevision(&map) != s_perforceConn->m_command.GetHaveRevision(&map))
                    {
                        newInfo.m_flags |= SCF_OutOfDate;
                    }
                }

                if(!s_perforceConn->m_command.ExclusiveOpen(&map))
                {
                    newInfo.m_flags |= SCF_MultiCheckOut;
                }

                bool openByOthers = s_perforceConn->m_command.IsOpenByOtherUsers(&map);
                if (openByOthers)
                {
                    newInfo.m_flags |= SCF_OtherOpen;
                    newInfo.m_StatusUser = s_perforceConn->m_command.GetOtherUserCheckedOut(&map);
                }

                if(s_perforceConn->m_command.IsOpenByCurrentUser(&map))
                {
                    newInfo.m_flags |= SCF_OpenByUser;

                    if (s_perforceConn->m_command.CurrentActionIsMove(&map))
                    {
                        newInfo.m_flags |= SCF_PendingMove;
                    }
                    else if (s_perforceConn->m_command.CurrentActionIsAdd(&map))
                    {
                        newInfo.m_flags |= SCF_PendingAdd;
                    }
                    else if (s_perforceConn->m_command.CurrentActionIsDelete(&map))
                    {
                        newInfo.m_flags |= SCF_PendingDelete;
                    }
                }

                fileInfoList.push_back(newInfo);
            }
        }

        return fileInfoList;
    }

    void PerforceComponent::ProcessJobOffline(const PerforceJobRequest& request)
    {
        AZ_Assert(m_ProcessThreadID != AZStd::thread::id(), "The perforce worker thread has not started.");
        AZ_Assert(AZStd::this_thread::get_id() == m_ProcessThreadID, "You may only call this function from the perforce worker thread.");
        AZ_Assert(m_offlineMode, "Processing Source Control Jobs in offline mode when we are connected to Source Control");

        switch (request.m_requestType)
        {
        case PerforceJobRequest::PJR_Stat:
            m_localFileSCComponent.GetFileInfo(request.m_requestPath.c_str(), request.m_callback);
            break;
        case PerforceJobRequest::PJR_StatBulk:
            m_localFileSCComponent.GetBulkFileInfo(request.m_bulkFilePaths, request.m_bulkCallback);
            break;
        case PerforceJobRequest::PJR_Edit:
            m_localFileSCComponent.RequestEdit(request.m_requestPath.c_str(), request.m_allowMultiCheckout, request.m_callback);
            break;
        case PerforceJobRequest::PJR_EditBulk:
            m_localFileSCComponent.RequestEditBulk(request.m_bulkFilePaths, request.m_allowMultiCheckout, request.m_bulkCallback);
            break;
        case PerforceJobRequest::PJR_Delete:
            m_localFileSCComponent.RequestDeleteExtended(request.m_requestPath.c_str(), request.m_skipReadonly, request.m_callback);
            break;
        case PerforceJobRequest::PJR_DeleteBulk:
            m_localFileSCComponent.RequestDeleteBulkExtended(request.m_requestPath.c_str(), request.m_skipReadonly, request.m_bulkCallback);
            break;
        case PerforceJobRequest::PJR_Revert:
            m_localFileSCComponent.RequestRevert(request.m_requestPath.c_str(), request.m_callback);
            break;
        case PerforceJobRequest::PJR_Rename:
            m_localFileSCComponent.RequestRenameExtended(request.m_requestPath.c_str(), request.m_targetPath.c_str(), request.m_skipReadonly, request.m_callback);
            break;
        case PerforceJobRequest::PJR_RenameBulk:
            m_localFileSCComponent.RequestRenameBulkExtended(request.m_requestPath.c_str(), request.m_targetPath.c_str(), request.m_skipReadonly, request.m_bulkCallback);
            break;
        case PerforceJobRequest::PJR_Sync:
            m_localFileSCComponent.RequestLatest(request.m_requestPath.c_str(), request.m_callback);
            break;
        default:
            AZ_Assert(false, "Invalid type of perforce job request.");
            break;
        }
    }

    // narrow events to the main thread.
    void PerforceComponent::ProcessResultQueue()
    {
        while (!m_resultQueue.empty())
        {
            PerforceJobResult resp;
            {
                AZStd::lock_guard<AZStd::mutex> locker(m_ResultQueueMutex);
                resp = AZStd::move(m_resultQueue.front());
                m_resultQueue.pop();
            }
            // dispatch the callback outside the scope of the lock so that more elements could be queued by threads...

            if (resp.m_callback)
            {
                resp.m_callback(resp.m_succeeded, resp.m_fileInfo);
            }
            else
            {
                resp.m_bulkCallback(resp.m_succeeded, resp.m_bulkFileInfo);
            }
        }

        while (!m_settingsQueue.empty())
        {
            PerforceSettingResult result;
            {
                AZStd::lock_guard<AZStd::mutex> locker(m_SettingsQueueMutex);
                result = AZStd::move(m_settingsQueue.front());
                m_settingsQueue.pop();
            }
            // dispatch the callback outside the scope of the lock so that more elements could be queued by threads...
            result.m_callback(result.m_settingInfo);
        }
    }

    bool PerforceComponent::ParseOutput(PerforceMap& perforceMap, AZStd::string& perforceOutput, const char* lineDelim) const
    {
        const char* delimiter = lineDelim ? lineDelim : " ";

        // replace "..." with empty strings, each line is in the format of "... key value"
        AzFramework::StringFunc::Replace(perforceOutput, "...", "");

        // tokenize the buffer into line by line key/value pairs
        AZStd::vector<AZStd::string> lineTokens;
        AzFramework::StringFunc::Tokenize(perforceOutput.c_str(), lineTokens, "\r\n");

        // add all key/value pairs to a map
        perforceMap.clear();
        AZStd::vector<AZStd::string> keyValueTokens;
        AZStd::string key, value;
        for (auto lineToken : lineTokens)
        {
            // tokenize line, add kvp, clear tokens
            AzFramework::StringFunc::Tokenize(lineToken.c_str(), keyValueTokens, delimiter);

            // ensure we found a key
            if (keyValueTokens.size() > 0)
            {
                key = keyValueTokens[0];
            }
            else
            {
                continue;
            }

            // check for a value, not all keys will have values e.g. isMapped (although most keys do)
            value = "";
            if (keyValueTokens.size() > 1)
            {
                for (int i = 1; i < keyValueTokens.size(); i++)
                {
                    value += keyValueTokens[i];
                }
            }

            perforceMap.insert(AZStd::make_pair(key, value));
            keyValueTokens.clear();
        }

        return !perforceMap.empty();
    }

    bool PerforceComponent::ParseDuplicateOutput(AZStd::vector<PerforceMap>& perforceMapList, AZStd::string& perforceOutput) const
    {
        // replace "..." with empty strings, each line is in the format of "... key value"
        AzFramework::StringFunc::Replace(perforceOutput, "...", "");

        /* insert a group divider in the middle of "\r\n\r\n", a new group of key/values seems to be noted by a double carriage return
           when the group divider is found, the current mapped values will be inserted into the vector and a new map will be started */
        AzFramework::StringFunc::Replace(perforceOutput, "\r\n\r\n", "\r\n|-|\r\n");

        // tokenize the buffer into line by line key/value pairs
        AZStd::vector<AZStd::string> lineTokens;
        AzFramework::StringFunc::Tokenize(perforceOutput.c_str(), lineTokens, "\r\n");

        // add all key/value pairs to a map
        perforceMapList.clear();
        AZStd::vector<AZStd::string> keyValueTokens;
        AZStd::string key, value;
        PerforceMap currentMap;
        for (const auto& lineToken : lineTokens)
        {
            // tokenize line, add kvp, clear tokens
            AzFramework::StringFunc::Tokenize(lineToken.c_str(), keyValueTokens, " ");

            // ensure we found a key
            if (!keyValueTokens.empty())
            {
                key = keyValueTokens[0];

                // insert the map into the vector when the kvp group divider is found
                if (key == "|-|")
                {
                    perforceMapList.push_back(currentMap);
                    currentMap.clear();
                    keyValueTokens.clear();
                    continue;
                }
            }
            else
            {
                continue;
            }

            // check for a value, not all keys will have values e.g. isMapped (although most keys do)
            value = "";
            if (keyValueTokens.size() > 1)
            {
                for (int i = 1; i < keyValueTokens.size(); i++)
                {
                    value.append(keyValueTokens[i]);

                    if (i != keyValueTokens.size() - 1)
                    {
                        value.append(" ");
                    }
                }
            }

            currentMap.insert(AZStd::make_pair(key, value));
            keyValueTokens.clear();
        }

        return !perforceMapList.empty();
    }
} // namespace AzToolsFramework
