/*
 * All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
 * its licensors.
 *
 * For complete copyright and license terms please see the LICENSE at the root of this
 * distribution (the "License"). All use of this software is governed by the License,
 * or, if provided, by the license below or the license accompanying this file. Do not
 * remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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
