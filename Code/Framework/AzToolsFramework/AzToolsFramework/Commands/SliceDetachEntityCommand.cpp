/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "SliceDetachEntityCommand.h"

namespace AzToolsFramework
{
    SliceDetachEntityCommand::SliceDetachEntityCommand(const AzToolsFramework::EntityIdList& entityIds, const AZStd::string& friendlyName)
        : BaseSliceCommand(friendlyName)
        , m_entityIds(entityIds)
    {}

    void SliceDetachEntityCommand::Undo()
    {
        RestoreEntities(m_entityUndoRestoreInfoArray, true);
    }

    void SliceDetachEntityCommand::Redo()
    {
        AzToolsFramework::ToolsApplicationRequestBus::BroadcastResult(
            m_changed, &AzToolsFramework::ToolsApplicationRequestBus::Events::DetachEntities, m_entityIds, m_entityUndoRestoreInfoArray);
    }
}
