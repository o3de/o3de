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

#include "EntityManipulatorCommand.h"

#include <AzToolsFramework/Viewport/ViewportMessages.h>

namespace AzToolsFramework
{
    EntityManipulatorCommand::State::State(
        const AZ::u8 pivotOverride, const AZ::Transform& transform, const AZ::EntityId entityId)
        : m_transform(transform), m_entityId(entityId), m_pivotOverride(pivotOverride)
    {
    }

    bool operator==(
        const EntityManipulatorCommand::State& lhs, const EntityManipulatorCommand::State& rhs)
    {
        return lhs.m_pivotOverride == rhs.m_pivotOverride
            && lhs.m_entityId == rhs.m_entityId
            && lhs.m_transform.IsClose(rhs.m_transform);
    }

    bool operator!=(
        const EntityManipulatorCommand::State& lhs, const EntityManipulatorCommand::State& rhs)
    {
        return lhs.m_pivotOverride != rhs.m_pivotOverride
            || lhs.m_entityId != rhs.m_entityId
            || !lhs.m_transform.IsClose(rhs.m_transform);
    }

    EntityManipulatorCommand::EntityManipulatorCommand(
        const State& stateBefore, const AZStd::string& friendlyName)
        : URSequencePoint(friendlyName)
        , m_stateBefore(stateBefore)
        , m_stateAfter(stateBefore) // default to the same as before in case it does not move
    {
    }

    void EntityManipulatorCommand::Undo()
    {
        EditorManipulatorCommandUndoRedoRequestBus::Event(
            GetEntityContextId(), &EditorManipulatorCommandUndoRedoRequests::UndoRedoEntityManipulatorCommand,
            m_stateBefore.m_pivotOverride, m_stateBefore.m_transform, m_stateBefore.m_entityId);
    }

    void EntityManipulatorCommand::Redo()
    {
        EditorManipulatorCommandUndoRedoRequestBus::Event(
            GetEntityContextId(), &EditorManipulatorCommandUndoRedoRequests::UndoRedoEntityManipulatorCommand,
            m_stateAfter.m_pivotOverride, m_stateAfter.m_transform, m_stateAfter.m_entityId);
    }

    void EntityManipulatorCommand::SetManipulatorAfter(const State& stateAfter)
    {
        m_stateAfter = stateAfter;
    }

    bool EntityManipulatorCommand::Changed() const
    {
        // note: check if the undo command is worth recording - if nothing has changed
        // then do not record it (shifts the responsibility of the check from the caller)
        return m_stateBefore != m_stateAfter;
    }

    AZ::u8 BuildPivotOverride(
        const bool translationOverride, const bool orientationOverride)
    {
        AZ::u8 pivotOverride = EntityManipulatorCommand::PivotOverride::None;

        if (translationOverride)
        {
            pivotOverride |= EntityManipulatorCommand::PivotOverride::Translation;
        }

        if (orientationOverride)
        {
            pivotOverride |= EntityManipulatorCommand::PivotOverride::Orientation;
        }

        return pivotOverride;
    }

    bool PivotHasTranslationOverride(const AZ::u8 pivotOverride)
    {
        return (pivotOverride & EntityManipulatorCommand::PivotOverride::Translation) != 0;
    }

    bool PivotHasOrientationOverride(const AZ::u8 pivotOverride)
    {
        return (pivotOverride & EntityManipulatorCommand::PivotOverride::Orientation) != 0;
    }

    bool PivotHasTransformOverride(const AZ::u8 pivotOverride)
    {
        return PivotHasTranslationOverride(pivotOverride) || PivotHasOrientationOverride(pivotOverride);
    }

} // namespace AzToolsFramework