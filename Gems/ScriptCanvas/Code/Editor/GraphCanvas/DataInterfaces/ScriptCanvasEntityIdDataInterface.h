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
        AZ_CLASS_ALLOCATOR(ScriptCanvasEntityIdDataInterface, AZ::SystemAllocator, 0);
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
                const AZ::EntityId* entityId = object->GetAs<AZ::EntityId>();
                if (entityId && *entityId == ScriptCanvas::GraphOwnerId)
                {
                    return "Self";
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
                datumView.SetAs(entityId);

                PostUndoPoint();
                PropertyGridRequestBus::Broadcast(&PropertyGridRequests::RefreshPropertyGrid);
            }
        }
        ////
    };
}
