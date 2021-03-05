/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#include "AzToolsFramework_precompiled.h"
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
