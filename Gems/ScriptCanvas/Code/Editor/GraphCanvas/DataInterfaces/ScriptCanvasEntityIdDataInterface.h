/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <GraphCanvas/Components/NodePropertyDisplay/EntityIdDataInterface.h>

#include "ScriptCanvasDataInterface.h"

#include <QWidget>
#include <QMenu>
#include <QAction>

namespace ScriptCanvasEditor
{
    class ScriptCanvasEntityIdDataInterface
        : public ScriptCanvasDataInterface<GraphCanvas::EntityIdDataInterface>
    {
    public:
        AZ_CLASS_ALLOCATOR(ScriptCanvasEntityIdDataInterface, AZ::SystemAllocator);
        ScriptCanvasEntityIdDataInterface(const AZ::EntityId& nodeId, const ScriptCanvas::SlotId& slotId)
            : ScriptCanvasDataInterface(nodeId, slotId)
        {   
        }
        
        ~ScriptCanvasEntityIdDataInterface() = default;
        
        // EntityIdDataInterface
        AZ::EntityId GetEntityId() const override
        {
            const ScriptCanvas::Datum* object = GetSlotObject();

            if (object)
            {
                const AZ::EntityId* retVal = object->GetAs<AZ::EntityId>();

                if (retVal)
                {
                    return (*retVal);
                }
            }
            
            return AZ::EntityId();
        }

        const AZStd::string GetNameOverride() const override
        {
            const ScriptCanvas::Datum* object = GetSlotObject();

            if (object && object->IS_A(ScriptCanvas::Data::Type::EntityID()))
            {
                if (const AZ::EntityId* entityId = object->GetAs<AZ::EntityId>())
                {
                    if (*entityId == ScriptCanvas::GraphOwnerId)
                    {
                        return "Self";
                    }
                    else if (*entityId == ScriptCanvas::UniqueId)
                    {
                        return "Unique";
                    }
                }
            }

            return {};
        }

        void OnShowContextMenu(QWidget* nodePropertyDisplay, const QPoint& pos) override
        {
            QPoint globalPos = nodePropertyDisplay->mapToGlobal(pos);

            enum EntityMenuAction
            {
                SetToSelf
            };

            QMenu entityMenu;
            QAction* setToSelf = entityMenu.addAction("Set to Self");
            setToSelf->setToolTip("Reset the EntityId to the Entity that owns this graph.");
            setToSelf->setData(EntityMenuAction::SetToSelf);

            QAction* selectedItem = entityMenu.exec(globalPos);
            if (selectedItem)
            {
                if (selectedItem->data().toInt() == EntityMenuAction::SetToSelf)
                {
                    SetEntityId(ScriptCanvas::GraphOwnerId);
                }
            }
        }

        void SetEntityId(const AZ::EntityId& entityId) override
        {
            ScriptCanvas::ModifiableDatumView datumView;
            ModifySlotObject(datumView);

            if (datumView.IsValid() && datumView.IsType(ScriptCanvas::Data::Type::EntityID()))
            {
                const AZ::EntityId* storedId = datumView.GetAs<AZ::EntityId>();

                if (storedId == nullptr || (*storedId) != entityId)
                {
                    datumView.SetAs(entityId);

                    PostUndoPoint();
                    PropertyGridRequestBus::Broadcast(&PropertyGridRequests::RefreshPropertyGrid);
                }
            }
        }
        ////
    };
}
