/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzToolsFramework/Prefab/Undo/PrefabUndoBase.h>

namespace AzToolsFramework::Prefab
{
    class InstanceEntityMapperInterface;

    //! Undo class for handling updating component properties of the focused prefab.
    class PrefabUndoComponentPropertyEdit : public PrefabUndoBase
    {
    public:
        AZ_RTTI(PrefabUndoComponentPropertyEdit, "{2B54AD53-329F-45B7-BAEB-737592D0B726}", PrefabUndoBase);
        AZ_CLASS_ALLOCATOR(PrefabUndoComponentPropertyEdit, AZ::SystemAllocator);

        explicit PrefabUndoComponentPropertyEdit(const AZStd::string& undoOperationName);

        void Capture(
            const PrefabDomValue& initialState,
            const PrefabDomValue& endState,
            AZ::EntityId entityId,
            AZStd::string_view pathToComponentProperty,
            bool updateCache = true);

        void Undo() override;
        void Redo() override;
        void Redo(InstanceOptionalConstReference instanceToExclude) override;

    private:
        InstanceEntityMapperInterface* m_instanceEntityMapperInterface = nullptr;
    };
} // namespace AzToolsFramework::Prefab
