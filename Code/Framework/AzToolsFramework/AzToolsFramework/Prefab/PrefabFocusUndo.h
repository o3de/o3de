/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/EntityId.h>
#include <AzToolsFramework/Undo/UndoSystem.h>

namespace AzToolsFramework::Prefab
{
    class PrefabFocusInterface;
    class PrefabFocusPublicInterface;

    //! Undo node for prefab focus change operations.
    class PrefabFocusUndo
        : public UndoSystem::URSequencePoint
    {
    public:
        explicit PrefabFocusUndo(const AZStd::string& undoOperationName);

        bool Changed() const override;
        void Capture(AZ::EntityId entityId);

        void Undo() override;
        void Redo() override;

    protected:
        PrefabFocusInterface* m_prefabFocusInterface = nullptr;
        PrefabFocusPublicInterface* m_prefabFocusPublicInterface = nullptr;

        AZ::EntityId m_beforeEntityId;
        AZ::EntityId m_afterEntityId;
    };
} // namespace AzToolsFramework::Prefab
