/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzToolsFramework/Undo/UndoSystem.h>

namespace AzToolsFramework::Prefab
{
    class PrefabSystemComponentInterface;
    class InstanceToTemplateInterface;
    class PrefabOverridePublicInterface;

    //! Undo class for handling updating entities to an instance as override of focused instance.
    class PrefabUndoEntityOverrides
        : public UndoSystem::URSequencePoint
    {
    public:
        AZ_RTTI(PrefabUndoEntityOverrides, "{97F210B2-86BE-4A16-9B3E-6ADAFD9BCDA3}", UndoSystem::URSequencePoint);
        AZ_CLASS_ALLOCATOR(PrefabUndoEntityOverrides, AZ::SystemAllocator);

        explicit PrefabUndoEntityOverrides(const AZStd::string& undoOperationName);

        bool Changed() const override;
        void Undo() override;
        void Redo() override;

        // The function to generate override subtrees for updating the provided entity list as overrides.
        // Redo should not be called after. It does redo in capture because adding override patches here allows us
        // to generate patches with correct indices and add them to tree in one place. Two operations won't be disconnected.
        void CaptureAndRedo(
            const AZStd::vector<const AZ::Entity*>& entityList,
            Instance& owningInstance,
            const Instance& focusedInstance);

    private:
        // The function to update link during undo and redo.
        void UpdateLink();

        // Map that stores subtree paths as well as before and after states of subtrees.
        AZStd::unordered_map<AZ::Dom::Path, PrefabOverridePrefixTree> m_subtreeStates;

        // Link that connects the linked instance and the focused instance.
        LinkId m_linkId;

        PrefabSystemComponentInterface* m_prefabSystemComponentInterface = nullptr;
        InstanceToTemplateInterface* m_instanceToTemplateInterface = nullptr;
        PrefabOverridePublicInterface* m_prefabOverridePublicInterface = nullptr;
    };
} // namespace AzToolsFramework::Prefab
