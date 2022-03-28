/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <SaveUtilities/AsyncSaveRunner.h>
#include <AzToolsFramework/SourceControl/SourceControlAPI.h>
#include <AzCore/IO/SystemFile.h>
#include <ActionOutput.h>

namespace AZ
{
    /* -------------------------- *
     * == Save Operation Cache == *
     * -------------------------- */

    SaveOperationController::SaveOperationCache::SaveOperationCache(const AZStd::string& fullPath, SynchronousSaveOperation saveOperation, SaveOperationController& owner, bool isDelete)
        : m_fullSavePath(fullPath)
        , m_saveOperation(saveOperation)
        , m_owner(owner)
        , m_isDelete(isDelete)
    {
    }

    void SaveOperationController::SaveOperationCache::Run(const AZStd::shared_ptr<ActionOutput>& actionOutput)
    {
        if (m_isDelete)
        {
            RunDelete(actionOutput);
            return;
        }
        // Create the callback to pass to the SourceControlAPI
        AzToolsFramework::SourceControlResponseCallback callback =
        [this, actionOutput](bool success, const AzToolsFramework::SourceControlFileInfo& info)
        {
            if (success || !info.IsReadOnly())
            {
                if (m_saveOperation && !m_saveOperation(m_fullSavePath, actionOutput))
                {
                    success = false;

                    if (actionOutput)
                    {
                        actionOutput->AddError("Failed to save entries/dependencies", m_fullSavePath);
                    }
                }
            }

            if (!success && actionOutput)
            {
                AZStd::string message;
                AZStd::string details = m_fullSavePath;

                // If there's no attempt to save any data it's assumed that this function was called to add an existing file to source control.
                //      Rather than report this as an error, report it as a warning as no data was lost.
                bool reportAsWarning = !m_saveOperation;
                bool moreDetails = true;

                // Be more specific with errors so as to give the user the best chance at fixing them
                if (!info.HasFlag(AzToolsFramework::SCF_OpenByUser))
                {
                    if (info.HasFlag(AzToolsFramework::SCF_OutOfDate))
                    {
                        message = "The file being worked on doesn't contain the latest changes from source control";
                        moreDetails = false;
                    }
                    else if (info.IsLockedByOther())
                    {
                        message = "The file is already exclusively opened by another user";
                        details = info.m_StatusUser + " -> " + m_fullSavePath;
                        moreDetails = false;
                    }
                    else if (info.m_status == AzToolsFramework::SourceControlStatus::SCS_ProviderIsDown)
                    {
                        message = "Failed to put entries/dependencies into source control as the provider is not available.\n";
                        reportAsWarning = true;
                    }
                    else if (info.m_status == AzToolsFramework::SourceControlStatus::SCS_CertificateInvalid)
                    {
                        message = "Failed to put entries/dependencies into source control as the source control has an invalid certificate.\n";
                    }
                    else if (info.m_status == AzToolsFramework::SourceControlStatus::SCS_ProviderError)
                    {
                        message = "Failed to put entries/dependencies into source control as the provider reported an error.\n";
                    }
                    else if (!info.IsManaged())
                    {
                        message = "Failed to put entries/dependencies into source control as they are outside the current workspace mapping.\n";
                        reportAsWarning = true;
                    }
                    else
                    {
                        message = "Make sure the disk is not full or the file is not write-protected or not currently in use.\n";
                    }
                }
                else
                {
                    message = "File marked as 'Open By User' but still failed.\n";
                }

                if (moreDetails)
                {
                    message += "Please see the source control icon in the status bar for further details";
                }

                if (reportAsWarning)
                {
                    actionOutput->AddWarning(message, details);
                    success = true;
                }
                else
                {
                    actionOutput->AddError(message, details);
                }
            }

            m_owner.HandleOperationComplete(this, success);
        };

        using SCCommandBus = AzToolsFramework::SourceControlCommandBus;
        SCCommandBus::Broadcast(&SCCommandBus::Events::RequestEdit, m_fullSavePath.c_str(), true, callback);
    }

    void SaveOperationController::SaveOperationCache::RunDelete(const AZStd::shared_ptr<ActionOutput>& actionOutput)
    {
        // Create the callback to pass to the SourceControlAPI
        AzToolsFramework::SourceControlResponseCallback callback =
            [this, actionOutput](bool success, const AzToolsFramework::SourceControlFileInfo& info)
        {
            if (success || !info.IsManaged())
            {
                success = true;
                if (m_saveOperation && !m_saveOperation(m_fullSavePath, actionOutput))
                {
                    success = false;

                    if (actionOutput)
                    {
                        actionOutput->AddError("Failed to delete entry", m_fullSavePath.c_str());
                    }
                }
            }
            else if (actionOutput)
            {
                // Be more specific with errors so as to give the user the best chance at fixing them
                if (!info.HasFlag(AzToolsFramework::SCF_OpenByUser))
                {
                    if (info.HasFlag(AzToolsFramework::SourceControlFlags::SCF_OutOfDate))
                    {
                        actionOutput->AddError("Source Control Issue - You do not have latest changes from source control for file", m_fullSavePath);
                    }
                    else if (info.IsLockedByOther())
                    {
                        actionOutput->AddError("Source Control Issue - File exclusively opened by another user", info.m_StatusUser + " -> " + m_fullSavePath);
                    }
                    else if (info.m_status == AzToolsFramework::SourceControlStatus::SCS_ProviderIsDown
                        || info.m_status == AzToolsFramework::SourceControlStatus::SCS_CertificateInvalid
                        || info.m_status == AzToolsFramework::SourceControlStatus::SCS_ProviderError)
                    {
                        actionOutput->AddError("Source Control Issue - Failed to remove file from source control, check your connection to your source control service", m_fullSavePath.c_str());
                    }
                    else
                    {
                        actionOutput->AddError("Unknown Issue with source control.", m_fullSavePath);
                    }
                }
                else
                {
                    actionOutput->AddError("Source Control Issue - File marked as 'Open By User' but still failed.", m_fullSavePath);
                }
            }

            m_owner.HandleOperationComplete(this, success);
        };

        using SCCommandBus = AzToolsFramework::SourceControlCommandBus;
        SCCommandBus::Broadcast(&SCCommandBus::Events::RequestDelete, m_fullSavePath.c_str(), callback);
    }

    /* ------------------------------- *
     * == Save Operation Controller == *
     * ------------------------------- */

    SaveOperationController::SaveOperationController(AsyncSaveRunner& owner)
        : m_owner(owner)
        , m_completedCount(0)
    {
    }

    void SaveOperationController::AddDeleteOperation(const AZStd::string& fullPath, SynchronousSaveOperation saveOperation)
    {
        m_allSaveOperations.push_back(AZStd::make_shared<SaveOperationCache>(fullPath, saveOperation, *this, true));
    }

    void SaveOperationController::AddSaveOperation(const AZStd::string& fullPath, SynchronousSaveOperation saveOperation)
    {
        m_allSaveOperations.push_back(AZStd::make_shared<SaveOperationCache>(fullPath, saveOperation, *this));
    }

    void SaveOperationController::SetOnCompleteCallback(SaveCompleteCallback onThisRunnerComplete)
    {
        m_onSaveComplete = onThisRunnerComplete;
    }
    void SaveOperationController::RunAll(const AZStd::shared_ptr<ActionOutput>& actionOutput)
    {
        m_completedCount = 0;

        // If for some reason there are no save operations in this controller, then we need
        //  to notify the runner and return so that this controller can be properly counted
        //  as being completed.
        if (m_allSaveOperations.empty())
        {
            m_owner.HandleRunnerFinished(this, true);
            return;
        }

        for (AZStd::shared_ptr<SaveOperationCache>& saveOperation : m_allSaveOperations)
        {
            saveOperation->Run(actionOutput);
        }
    }

    void SaveOperationController::HandleOperationComplete([[maybe_unused]] SaveOperationCache* saveOperation, bool success)
    {
        if (!success)
        {
            m_currentSaveResult = false;
        }

        AZ_Assert(AZStd::find_if(m_allSaveOperations.begin(), m_allSaveOperations.end(),
            [saveOperation](const AZStd::shared_ptr<SaveOperationCache>& target) -> bool
            {
                return target.get() == saveOperation;
            }) != m_allSaveOperations.end(), 
            "Attempting to cleanup completed save operation failed. Operation not found. Target file was: '%s'", 
            saveOperation->m_fullSavePath.c_str());
        
        m_completedCount++;
        if (m_completedCount >= m_allSaveOperations.size())
        {
            if (m_onSaveComplete)
            {
                m_onSaveComplete(m_currentSaveResult);
            }

            m_owner.HandleRunnerFinished(this, m_currentSaveResult);
        }
    }

    /* ----------------------- *
     * == Async Save Runner == *
     * ----------------------- */

    AZStd::shared_ptr<SaveOperationController> AsyncSaveRunner::GenerateController()
    {
        AZStd::shared_ptr<SaveOperationController> saveEntryController = AZStd::make_shared<SaveOperationController>(*this);
        m_allSaveControllers.push_back(saveEntryController);
        return saveEntryController;
    }

    void AsyncSaveRunner::Run(const AZStd::shared_ptr<ActionOutput>& actionOutput, SaveCompleteCallback onSaveAllComplete, ControllerOrder order)
    {
        m_counter = 0;
        m_order = order;
        m_actionOutput = actionOutput;
        m_onSaveAllComplete = onSaveAllComplete;

        // If for some reason there are no save operations in this runner, then we need to run
        //  the callback now and return so that the caller is properly notified.
        if (m_allSaveControllers.empty())
        {
            if (m_onSaveAllComplete)
            {
                m_onSaveAllComplete(true);
            }

            return;
        }

        if (order == ControllerOrder::Random)
        {
            for (auto& saveOp : m_allSaveControllers)
            {
                saveOp->RunAll(actionOutput);
            }
        }
        else if (order == ControllerOrder::Sequential)
        {
            m_allSaveControllers[0]->RunAll(actionOutput);
        }
        else
        {
            AZ_Assert(false, "Invalid ControllerOrder: %i", order);
        }
    }
    void AsyncSaveRunner::HandleRunnerFinished([[maybe_unused]] SaveOperationController* runner, bool success)
    {
        if (!success)
        {
            m_allWereSuccessfull = false;
        }

        if (m_order == ControllerOrder::Random)
        {
            AZ_Assert(AZStd::find_if(m_allSaveControllers.begin(), m_allSaveControllers.end(),
                [runner](const AZStd::shared_ptr<SaveOperationController>& target) -> bool 
                { 
                    return target.get() == runner; 
                }) != m_allSaveControllers.end(), "Attempting to cleanup completed save runner failed");
            
            m_counter++;
        }
        else if (m_order == ControllerOrder::Sequential)
        {
            AZ_Assert(m_counter < m_allSaveControllers.size(), 
                "Counter for save controllers has become invalid (%i vs. %i).", static_cast<int>(m_counter), m_allSaveControllers.size());
            AZ_Assert(m_allSaveControllers[m_counter].get() == runner, "Completed incorrect save runner for index %i.", static_cast<int>(m_counter));

            m_counter++;
            if (m_counter < m_allSaveControllers.size())
            {
                m_allSaveControllers[m_counter]->RunAll(m_actionOutput);
            }
        }
        else
        {
            AZ_Assert(false, "Invalid ControllerOrder: %i", m_order);
        }

        if (m_counter >= m_allSaveControllers.size())
        {
            m_actionOutput.reset();
            if (m_onSaveAllComplete)
            {
                m_onSaveAllComplete(m_allWereSuccessfull);
            }
        }
    }
}
