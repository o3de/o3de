/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/ComponentBus.h>
#include <AzToolsFramework/Viewport/ViewportTypes.h>

namespace WhiteBox
{
    struct WhiteBoxMesh;

    //! Record an undo/redo step and notify the white box mesh
    void RecordWhiteBoxAction(
        WhiteBoxMesh& whiteBox, const AZ::EntityComponentIdPair entityComponentIdPair, const char* undoRedoDesc);

    //! Returns true if user input is for flipping an edge
    bool InputFlipEdge(const AzToolsFramework::ViewportInteraction::MouseInteractionEvent& mouseInteraction);

    //! Returns true if user input is for restoring a vertex/edge
    bool InputRestore(const AzToolsFramework::ViewportInteraction::MouseInteractionEvent& mouseInteraction);
} // namespace WhiteBox
