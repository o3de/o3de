/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
