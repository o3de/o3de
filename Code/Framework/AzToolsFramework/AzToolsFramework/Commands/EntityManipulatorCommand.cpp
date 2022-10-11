/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "EntityManipulatorCommand.h"

#include <AzToolsFramework/Viewport/ViewportMessages.h>

namespace AzToolsFramework
{
    EntityManipulatorCommand::State::State(const AZ::u8 pivotOverride, const AZ::Transform& transform)
        : m_transform(transform)
        , m_pivotOverride(pivotOverride)
    {
    }

    bool operator==(const EntityManipulatorCommand::State& lhs, const EntityManipulatorCommand::State& rhs)
    {
        return lhs.m_pivotOverride == rhs.m_pivotOverride && lhs.m_transform.IsClose(rhs.m_transform);
    }

    bool operator!=(const EntityManipulatorCommand::State& lhs, const EntityManipulatorCommand::State& rhs)
    {
        return !(lhs == rhs);
    }

    EntityManipulatorCommand::EntityManipulatorCommand(const State& stateBefore, const AZStd::string& friendlyName)
        : URSequencePoint(friendlyName)
        , m_stateBefore(stateBefore)
        , m_stateAfter(stateBefore) // default to the same as before in case it does not move
    {
    }

    void EntityManipulatorCommand::Undo()
    {
        EditorManipulatorCommandUndoRedoRequestBus::Event(
            GetEntityContextId(), &EditorManipulatorCommandUndoRedoRequests::UndoRedoEntityManipulatorCommand,
            m_stateBefore.m_pivotOverride, m_stateBefore.m_transform);
    }

    void EntityManipulatorCommand::Redo()
    {
        EditorManipulatorCommandUndoRedoRequestBus::Event(
            GetEntityContextId(), &EditorManipulatorCommandUndoRedoRequests::UndoRedoEntityManipulatorCommand, m_stateAfter.m_pivotOverride,
            m_stateAfter.m_transform);
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

    AZ::u8 BuildPivotOverride(const bool translationOverride, const bool orientationOverride)
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
