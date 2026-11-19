/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/ComponentBus.h>
#include <AzCore/Math/Transform.h>

struct IEditor;

namespace WhiteBox
{
    class EditorWhiteBoxComponent;

    //! The White Box component named by @p entityComponentIdPair, or null. Shared to avoid unity-build collisions.
    EditorWhiteBoxComponent* FindWhiteBoxComponent(const AZ::EntityComponentIdPair& entityComponentIdPair);

    //! The space White Box overlays and manipulators live in: the entity transform with the active layer's
    //! transform folded in (non-uniform layer scale as its largest component).
    AZ::Transform EditorSpaceFromLocal(const AZ::EntityComponentIdPair& entityComponentIdPair);
    //! As above, for the callers that only carry an entity id. Uses the entity's White Box component.
    AZ::Transform EditorSpaceFromLocal(AZ::EntityId entityId);

    //! Small wrapper around an EBus call to request the file at the given path be added to source control.
    void RequestEditSourceControl(const char* absoluteFilePath);

    //! Small wrapper around an EBus call to retrieve the IEditor interface.
    IEditor* GetIEditor();
} // namespace WhiteBox
