/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <QApplication>
#include <QClipboard>
#include <QMimeData>

#include <GraphCanvas/Widgets/EditorContextMenu/ContextMenuActions/EditMenuActions/EditContextMenuActions.h>

#include <GraphCanvas/Components/SceneBus.h>

namespace GraphCanvas
{
    ////////////////////////////////
    // CutGraphSelectionMenuAction
    ////////////////////////////////

    CutGraphSelectionMenuAction::CutGraphSelectionMenuAction(QObject* parent)
        : EditContextMenuAction("Cut", parent)
    {
    }

    ContextMenuAction::SceneReaction CutGraphSelectionMenuAction::TriggerAction(const AZ::Vector2& /*scenePos*/)
    {
        const GraphId& graphId = GetGraphId();

        SceneRequestBus::Event(graphId, &SceneRequests::CutSelection);
        return SceneReaction::PostUndo;
    }

    /////////////////////////////////
    // CopyGraphSelectionMenuAction
    /////////////////////////////////

    CopyGraphSelectionMenuAction::CopyGraphSelectionMenuAction(QObject* parent)
        : EditContextMenuAction("Copy", parent)
    {
    }

    ContextMenuAction::SceneReaction CopyGraphSelectionMenuAction::TriggerAction(const AZ::Vector2& /*scenePos*/)
    {
        const GraphId& graphId = GetGraphId();

        SceneRequestBus::Event(graphId, &SceneRequests::CopySelection);
        return SceneReaction::PostUndo;
    }

    //////////////////////////////////
    // PasteGraphSelectionMenuAction
    //////////////////////////////////

    PasteGraphSelectionMenuAction::PasteGraphSelectionMenuAction(QObject* parent)
        : EditContextMenuAction("Paste", parent)
    {
    }

    void PasteGraphSelectionMenuAction::RefreshAction()
    {
        const GraphId& graphId = GetGraphId();

        AZStd::string mimeType;
        SceneRequestBus::EventResult(mimeType, graphId, &SceneRequests::GetCopyMimeType);

        bool isPasteable = QApplication::clipboard()->mimeData()->hasFormat(mimeType.c_str());
        setEnabled(isPasteable);
    }

    ContextMenuAction::SceneReaction PasteGraphSelectionMenuAction::TriggerAction(const AZ::Vector2& scenePos)
    {
        const GraphId& graphId = GetGraphId();

        SceneRequestBus::Event(graphId, &SceneRequests::PasteAt, QPoint(aznumeric_cast<int>(scenePos.GetX()), aznumeric_cast<int>(scenePos.GetY())));
        return SceneReaction::PostUndo;
    }
    
    ///////////////////////////////////
    // DeleteGraphSelectionMenuAction
    ///////////////////////////////////

    DeleteGraphSelectionMenuAction::DeleteGraphSelectionMenuAction(QObject* parent)
        : EditContextMenuAction("Delete", parent)
    {
    }

    ContextMenuAction::SceneReaction DeleteGraphSelectionMenuAction::TriggerAction(const AZ::Vector2& /*scenePos*/)
    {
        const GraphId& graphId = GetGraphId();

        SceneRequestBus::Event(graphId, &SceneRequests::DeleteSelection);
        return SceneReaction::PostUndo;
    }

    //////////////////////////////////////
    // DuplicateGraphSelectionMenuAction
    //////////////////////////////////////

    DuplicateGraphSelectionMenuAction::DuplicateGraphSelectionMenuAction(QObject* parent)
        : EditContextMenuAction("Duplicate", parent)
    {
    }

    ContextMenuAction::SceneReaction DuplicateGraphSelectionMenuAction::TriggerAction(const AZ::Vector2& scenePos)
    {
        const GraphId& graphId = GetGraphId();

        SceneRequestBus::Event(graphId, &SceneRequests::DuplicateSelectionAt, QPoint(aznumeric_cast<int>(scenePos.GetX()), aznumeric_cast<int>(scenePos.GetY())));
        return SceneReaction::PostUndo;
    }
}
