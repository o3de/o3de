/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzToolsFramework/Prefab/Link/Link.h>
#include <AzToolsFramework/Undo/UndoSystem.h>

namespace AzToolsFramework::Prefab
{
    class PrefabSystemComponentInterface;

    class PrefabUndoApplyOverrides : public UndoSystem::URSequencePoint
    {
    public:
        AZ_RTTI(PrefabUndoApplyOverrides, "{48E8E12E-C21C-4EAB-AB17-A6159BF66AA3}", UndoSystem::URSequencePoint);
        AZ_CLASS_ALLOCATOR(PrefabUndoApplyOverrides, AZ::SystemAllocator);

        explicit PrefabUndoApplyOverrides(const AZStd::string& undoOperationName);

        bool Changed() const override;
        void Undo() override;
        void Redo() override;

        void Capture(const AZ::Dom::Path& pathToSubTree, PrefabOverridePrefixTree&& m_overrideSubTree, LinkId linkId);

    private:
        PrefabOverridePrefixTree m_overrideSubTree;
        AZ::Dom::Path m_pathToSubTree;
        PrefabSystemComponentInterface* m_prefabSystemComponentInterface = nullptr;
        LinkId m_linkId;
        bool m_changed;
    };
} // namespace AzToolsFramework::Prefab
