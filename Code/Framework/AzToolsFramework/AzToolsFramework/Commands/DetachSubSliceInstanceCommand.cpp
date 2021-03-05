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
