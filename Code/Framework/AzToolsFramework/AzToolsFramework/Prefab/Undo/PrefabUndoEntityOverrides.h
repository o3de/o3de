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

    //! Undo class for handling updating entity list to an instance as override of focused instance.
    class PrefabUndoEntityOverrides
        : public UndoSystem::URSequencePoint
    {
    public:
        AZ_RTTI(PrefabUndoEntityOverrides, "{97F210B2-86BE-4A16-9B3E-6ADAFD9BCDA3}", UndoSystem::URSequencePoint);
        AZ_CLASS_ALLOCATOR(PrefabUndoEntityOverrides, AZ::SystemAllocator, 0);

        explicit PrefabUndoEntityOverrides(const AZStd::string& undoOperationName);

        bool Changed() const override;
        void Undo() override;
        void Redo() override;

        // The function to generate undo/redo an override subtree for updating the provided entity list as overrides.
        void Capture(const AZStd::vector<const AZ::Entity*>& entityList, Instance& owningInstance, const Instance& focusedInstance);

        // The function to update link during undo and redo.
        void UpdateLink();

    private:
        // Map that stores entities' override patch paths and override subtrees.
        AZStd::unordered_map<AZ::Dom::Path, PrefabOverridePrefixTree> m_overrideSubTrees;

        // Link that connects the linked instance and the focused instance.
        LinkId m_linkId;
        bool m_changed;

        PrefabSystemComponentInterface* m_prefabSystemComponentInterface = nullptr;
        InstanceToTemplateInterface* m_instanceToTemplateInterface = nullptr;
    };
} // namespace AzToolsFramework::Prefab
