/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/API/ToolsApplicationAPI.h>

AZTF_INSTANTIATE_EBUS_SINGLE_ADDRESS(AzToolsFramework::ToolsApplicationEvents);
AZTF_INSTANTIATE_EBUS_SINGLE_ADDRESS(AzToolsFramework::ToolsApplicationRequests);
AZTF_INSTANTIATE_EBUS_MULTI_ADDRESS(AzToolsFramework::EntitySelectionEvents);
AZTF_INSTANTIATE_EBUS_MULTI_ADDRESS(AzToolsFramework::EditorPickModeRequests);
AZTF_INSTANTIATE_EBUS_MULTI_ADDRESS(AzToolsFramework::EditorPickModeNotifications);
AZTF_INSTANTIATE_EBUS_SINGLE_ADDRESS(AzToolsFramework::EditorRequests);
AZTF_INSTANTIATE_EBUS_SINGLE_ADDRESS(AzToolsFramework::EditorEvents);
AZTF_INSTANTIATE_EBUS_MULTI_ADDRESS(AzToolsFramework::ViewPaneCallbacks);

namespace AzToolsFramework
{
    ScopedUndoBatch::ScopedUndoBatch(const char* batchName)
    {
        ToolsApplicationRequests::Bus::BroadcastResult(
            m_undoBatch, &ToolsApplicationRequests::Bus::Events::BeginUndoBatch, batchName);
    }

    ScopedUndoBatch::~ScopedUndoBatch()
    {
        ToolsApplicationRequests::Bus::Broadcast(&ToolsApplicationRequests::Bus::Events::EndUndoBatch);
    }

    // utility/convenience function for adding dirty entity
    void ScopedUndoBatch::MarkEntityDirty(const AZ::EntityId& id)
    {
        ToolsApplicationRequests::Bus::Broadcast(&ToolsApplicationRequests::Bus::Events::AddDirtyEntity, id);
    }

    UndoSystem::URSequencePoint* ScopedUndoBatch::GetUndoBatch() const
    {
        return m_undoBatch;
    }

    void UnregisterViewPane(const char* viewPaneName)
    {
        EditorRequests::Bus::Broadcast(&EditorRequests::UnregisterViewPane, viewPaneName);
    }

    void OpenViewPane(const char* viewPaneName)
    {
        EditorRequests::Bus::Broadcast(&EditorRequests::OpenViewPane, viewPaneName);
    }

    QDockWidget* InstanceViewPane(const char* viewPaneName)
    {
        QDockWidget* ret = nullptr;
        EditorRequests::Bus::BroadcastResult(ret, &EditorRequests::InstanceViewPane, viewPaneName);

        return ret;
    }

    void CloseViewPane(const char* viewPaneName)
    {
        EditorRequests::Bus::Broadcast(&EditorRequests::CloseViewPane, viewPaneName);
    }

    bool UndoRedoOperationInProgress()
    {
        bool isDuringUndoRedo = false;
        ToolsApplicationRequestBus::BroadcastResult(
            isDuringUndoRedo, &ToolsApplicationRequests::IsDuringUndoRedo);
        return isDuringUndoRedo;
    }
} // namespace AzToolsFramework
