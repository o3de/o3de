/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "AzToolsFramework_precompiled.h"
#include "DetachSubSliceInstanceCommand.h"

#include <AzToolsFramework/API/ToolsApplicationAPI.h>

namespace AzToolsFramework
{
    DetachSubsliceInstanceCommand::DetachSubsliceInstanceCommand(const AZ::SliceComponent::SliceInstanceEntityIdRemapList& subsliceRootList, const AZStd::string& friendlyName)
        : BaseSliceCommand(friendlyName)
        , m_subslices(subsliceRootList)
    {
    }

    void DetachSubsliceInstanceCommand::Undo()
    {
        RestoreEntities(m_entityUndoRestoreInfoArray, true);
    }

    void DetachSubsliceInstanceCommand::Redo()
    {
        AzToolsFramework::ToolsApplicationRequestBus::BroadcastResult(
            m_changed, &AzToolsFramework::ToolsApplicationRequestBus::Events::DetachSubsliceInstances, m_subslices, m_entityUndoRestoreInfoArray);
    }
}
