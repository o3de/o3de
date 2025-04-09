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
    
    class InstanceToTemplateInterface;
    class PrefabFocusInterface;
    class PrefabOverridePublicInterface;
    class PrefabSystemComponentInterface;

    //! Undo class for handling updating component properties of a prefab as overrides from focused prefab.
    class PrefabUndoComponentPropertyOverride : public UndoSystem::URSequencePoint
    {
    public:
        AZ_RTTI(PrefabUndoComponentPropertyOverride, "{DF46772A-4D01-4267-A218-778758804C66}", UndoSystem::URSequencePoint);
        AZ_CLASS_ALLOCATOR(PrefabUndoComponentPropertyOverride, AZ::SystemAllocator);

        explicit PrefabUndoComponentPropertyOverride(const AZStd::string& undoOperationName);

        bool Changed() const override;
        void Undo() override;
        void Redo() override;

        // The function to generate override subtrees for updating the provided entity as overrides.
        // Redo should not be called after. It does redo in capture because adding override patches here allows us
        // to generate patches with correct indices and add them to tree in one place. Two operations won't be disconnected.
        void CaptureAndRedo(
            Instance& owningInstance,
            const AZ::Dom::Path pathToPropertyFromOwningPrefab,
            const PrefabDomValue& afterStateOfComponentProperty);

    private:
        // The function to update link during undo and redo.
        void UpdateLink();

        // Link that connects the linked instance and the focused instance.
        LinkId m_linkId;

        // Set to true if property value is changed compared to last edit.
        bool m_changed;

        // SubTree for the overridden property.
        PrefabOverridePrefixTree m_overriddenPropertySubTree;

        // Path to the property.
        AZ::Dom::Path m_overriddenPropertyPath;

        PrefabSystemComponentInterface* m_prefabSystemComponentInterface = nullptr;
        InstanceToTemplateInterface* m_instanceToTemplateInterface = nullptr;
        PrefabOverridePublicInterface* m_prefabOverridePublicInterface = nullptr;
        Prefab::PrefabFocusInterface* m_prefabFocusInterface = nullptr;
    };
} // namespace AzToolsFramework::Prefab
