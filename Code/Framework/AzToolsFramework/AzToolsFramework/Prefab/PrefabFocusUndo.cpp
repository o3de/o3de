/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/Prefab/PrefabFocusUndo.h>

namespace AzToolsFramework::Prefab
{
    PrefabFocusUndo::PrefabFocusUndo(RootAliasPath before, RootAliasPath after, ApplyFocus apply)
        : UndoSystem::URSequencePoint("Focus Prefab")
        , m_before(AZStd::move(before))
        , m_after(AZStd::move(after))
        , m_apply(AZStd::move(apply))
    {
    }

    bool PrefabFocusUndo::Changed() const
    {
        return true;
    }

    void PrefabFocusUndo::Undo()
    {
        m_apply(m_before);
    }

    void PrefabFocusUndo::Redo()
    {
        m_apply(m_after);
    }

} // namespace AzToolsFramework::Prefab
