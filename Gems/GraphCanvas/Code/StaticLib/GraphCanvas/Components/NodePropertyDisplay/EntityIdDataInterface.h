/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <QMimeData>
#include <AzToolsFramework/ToolsComponents/EditorEntityIdContainer.h>

#include "DataInterface.h"

#include <GraphCanvas/Components/Slots/SlotBus.h>

class QWidget;

namespace GraphCanvas
{
    class EntityIdDataInterface
        : public DataInterface
    {
    public:
        virtual AZ::EntityId GetEntityId() const = 0;
        virtual void SetEntityId(const AZ::EntityId& entityId) = 0;

        virtual const AZStd::string GetNameOverride() const = 0;
        virtual void OnShowContextMenu(QWidget* nodePropertyDisplay, const QPoint& pos) = 0;

        bool EnableDropHandling() const override
        {
            return true;
        }

        AZ::Outcome<DragDropState> ShouldAcceptMimeData(const QMimeData* mimeData) override
        {
            auto nodePropertyDisplay = GetDisplay();

            if (nodePropertyDisplay == nullptr)
            {
                return AZ::Failure();
            }

            bool hasConnections = false;
            SlotRequestBus::EventResult(hasConnections, nodePropertyDisplay->GetSlotId(), &SlotRequests::HasConnections);

            if (hasConnections)
            {
                return AZ::Failure();
            }
            
            if (mimeData->hasFormat(AzToolsFramework::EditorEntityIdContainer::GetMimeType()))
            {
                QByteArray arrayData = mimeData->data(AzToolsFramework::EditorEntityIdContainer::GetMimeType());

                AzToolsFramework::EditorEntityIdContainer entityIdListContainer;
                if (entityIdListContainer.FromBuffer(arrayData.constData(), arrayData.size()))
                {
                    // We can only support a drop containing a single entityId.
                    if (entityIdListContainer.m_entityIds.size() == 1)
                    {
                        return AZ::Success(DragDropState::Valid);
                    }
                }
            }

            return AZ::Failure();
        }

        bool HandleMimeData(const QMimeData* mimeData) override
        {
            if (mimeData->hasFormat(AzToolsFramework::EditorEntityIdContainer::GetMimeType()))
            {
                QByteArray arrayData = mimeData->data(AzToolsFramework::EditorEntityIdContainer::GetMimeType());

                AzToolsFramework::EditorEntityIdContainer entityIdListContainer;
                if (entityIdListContainer.FromBuffer(arrayData.constData(), arrayData.size()))
                {
                    if (entityIdListContainer.m_entityIds.size() == 1)
                    {
                        SetEntityId(entityIdListContainer.m_entityIds.front());
                        return true;
                    }
                }
            }

            return false;
        }

    };
}
