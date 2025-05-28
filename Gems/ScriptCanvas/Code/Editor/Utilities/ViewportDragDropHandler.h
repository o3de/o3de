
/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#pragma once

#include <QMimeData>
#include <QDropEvent>

#include <AzCore/std/string/string.h>
#include <AzCore/Math/Vector3.h>

#include <AzQtComponents/Buses/DragAndDrop.h>

#include <AzToolsFramework/Entity/EntityTypes.h>

class ScriptCanvasAssetDragDropHandler : AzQtComponents::DragAndDropEventsBus::Handler
{
public:

    ScriptCanvasAssetDragDropHandler();
    ~ScriptCanvasAssetDragDropHandler();

    int GetDragAndDropEventsPriority() const override;

    //! Sent when a drag and drop action enters a widget.
    //! The context is intentionally non const, so that higher level listeners can add additional
    //! contextual information such as decoding the data and caching it, or partially consuming the data.
    void DragEnter(QDragEnterEvent* event, AzQtComponents::DragAndDropContextBase& /*context*/) override;

    //! Sent when a drag and drop action completes.
    void Drop(QDropEvent* event, AzQtComponents::DragAndDropContextBase& context) override;

private:

    void CreateEntitiesAtPoint(QStringList fileList, AZ::Vector3 location, AZ::EntityId parentEntityId, AzToolsFramework::EntityIdList& createdEntities);

    QStringList GetFileList(QDropEvent* event);

    static bool m_dragAccepted;
};
