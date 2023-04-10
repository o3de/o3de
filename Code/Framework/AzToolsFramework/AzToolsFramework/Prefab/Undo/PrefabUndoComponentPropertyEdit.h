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
    //! Undo class for handling updating component properties of the focused prefab (which is also the owning prefab).
    class PrefabUndoComponentPropertyEdit : public PrefabUndoBase
    {
    public:
        AZ_RTTI(PrefabUndoComponentPropertyEdit, "{2B54AD53-329F-45B7-BAEB-737592D0B726}", PrefabUndoBase);
        AZ_CLASS_ALLOCATOR(PrefabUndoComponentPropertyEdit, AZ::SystemAllocator);

        explicit PrefabUndoComponentPropertyEdit(const AZStd::string& undoOperationName);

        void Capture(
            Instance& owningInstance,
            const AZStd::string& pathToPropertyFromOwningPrefab,
            const PrefabDomValue& afterStateOfComponentProperty,
            bool updateCache = true);

        bool Changed() const override;
        void Undo() override;
        void Redo() override;
        void Redo(InstanceOptionalConstReference instanceToExclude) override;

    private:
        // Set to true if property value is changed compared to last edit.
        bool m_changed;
    };
} // namespace AzToolsFramework::Prefab
