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

#if 0

#include "EntityTransformCommand.h"
#include <HexEdFramework/FrameworkCore/SelectionMessages.h>
#include <HexEd/WorldEditor/ToolsComponents/TransformComponentBus.h>
#include "PreemptiveUndoCache.h"

namespace AzToolsFramework
{
    TransformCommand::TransformCommand(const AZ::u64& contextId, const AZStd::string& friendlyName, const EntityList& captureEntities)
        : UndoSystem::URSequencePoint(friendlyName)
        , m_contextId(contextId)
    {
        for (auto it = captureEntities.begin(); it != captureEntities.end(); ++it)
        {
            SRT current;
            EBUS_EVENT_ID_RESULT(current, *it, TransformComponentMessages::Bus, GetLocalSRT);
            m_priorTransforms[*it] = current;
            m_nextTransforms[*it] = current;
        }
    }

    void TransformCommand::Post()
    {
        // add to undo stack
        UndoSystem::UndoStack* undoStack = NULL;
        EBUS_EVENT_ID_RESULT(undoStack, m_contextId, SelectionMessages::Bus, GetUndoStack);

        if (undoStack)
        {
            undoStack->Post(this);
        }

        for (auto it = m_priorTransforms.begin(); it != m_priorTransforms.end(); ++it)
        {
            PreemptiveUndoCache::Get()->UpdateCache(it->first);
        }
    }

    void TransformCommand::Undo()
    {
        for (auto it = m_priorTransforms.begin(); it != m_priorTransforms.end(); ++it)
        {
            EBUS_EVENT_ID(it->first, TransformComponentMessages::Bus, SetLocalSRT, it->second);
            PreemptiveUndoCache::Get()->UpdateCache(it->first);
        }
    }

    void TransformCommand::Redo()
    {
        for (auto it = m_nextTransforms.begin(); it != m_nextTransforms.end(); ++it)
        {
            EBUS_EVENT_ID(it->first, TransformComponentMessages::Bus, SetLocalSRT, it->second);
            PreemptiveUndoCache::Get()->UpdateCache(it->first);
        }
    }

    void TransformCommand::CaptureNewTransform(const AZ::EntityId entityId)
    {
        AZ_Assert(m_priorTransforms.find(entityId) != m_priorTransforms.end(), "You can't add new transforms during an operation");
        AZ_Assert(m_nextTransforms.find(entityId) != m_nextTransforms.end(), "You can't add new transforms during an operation");

        SRT current;
        EBUS_EVENT_ID_RESULT(current, entityId, TransformComponentMessages::Bus, GetLocalSRT);
        m_nextTransforms[entityId] = current;
    }

    void TransformCommand::RevertToPriorTransform(const AZ::EntityId entityId)
    {
        AZ_Assert(m_priorTransforms.find(entityId) != m_priorTransforms.end(), "No such entity!");

        m_nextTransforms[entityId] = m_priorTransforms[entityId];
        EBUS_EVENT_ID(entityId, TransformComponentMessages::Bus, SetLocalSRT, m_priorTransforms[entityId]);
    }
}

#endif