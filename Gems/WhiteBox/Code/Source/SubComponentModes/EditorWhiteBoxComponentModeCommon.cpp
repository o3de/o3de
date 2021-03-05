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

#include "WhiteBox_precompiled.h"

#include "EditorWhiteBoxComponentModeCommon.h"

#include <AzCore/Component/ComponentBus.h>
#include <AzToolsFramework/ComponentMode/EditorComponentModeBus.h>
#include <WhiteBox/EditorWhiteBoxComponentBus.h>
#include <WhiteBox/WhiteBoxToolApi.h>

namespace WhiteBox
{
    using ComponentModeRequestBus = AzToolsFramework::ComponentModeFramework::ComponentModeRequestBus;

    void RecordWhiteBoxAction(
        WhiteBoxMesh& whiteBox, const AZ::EntityComponentIdPair entityComponentIdPair, const char* undoRedoDesc)
    {
        // update UVs
        Api::CalculatePlanarUVs(whiteBox);

        // record an undo step
        AzToolsFramework::ScopedUndoBatch undoBatch(undoRedoDesc);
        AzToolsFramework::ScopedUndoBatch::MarkEntityDirty(entityComponentIdPair.GetEntityId());

        // notify the component things have changed
        EditorWhiteBoxComponentRequestBus::Event(
            entityComponentIdPair, &EditorWhiteBoxComponentRequests::SerializeWhiteBox);

        // notify the white box mesh things have changed
        EditorWhiteBoxComponentNotificationBus::Event(
            entityComponentIdPair, &EditorWhiteBoxComponentNotifications::OnWhiteBoxMeshModified);

        // notify the component mode things have changed
        ComponentModeRequestBus::Event(entityComponentIdPair, &ComponentModeRequestBus::Events::Refresh);
    }

    bool InputFlipEdge(const AzToolsFramework::ViewportInteraction::MouseInteractionEvent& mouseInteraction)
    {
        return mouseInteraction.m_mouseInteraction.m_mouseButtons.Right() &&
            mouseInteraction.m_mouseEvent == AzToolsFramework::ViewportInteraction::MouseEvent::Down;
    }

    bool InputRestore(const AzToolsFramework::ViewportInteraction::MouseInteractionEvent& mouseInteraction)
    {
        return mouseInteraction.m_mouseInteraction.m_mouseButtons.Left() &&
            mouseInteraction.m_mouseEvent == AzToolsFramework::ViewportInteraction::MouseEvent::Down;
    }
} // namespace WhiteBox
