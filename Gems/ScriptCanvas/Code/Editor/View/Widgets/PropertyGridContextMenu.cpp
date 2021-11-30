/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "PropertyGrid.h"

#include <QAction>

#include <GraphCanvas/Components/Slots/SlotBus.h>
#include <GraphCanvas/Components/Nodes/NodeBus.h>
#include <GraphCanvas/Components/VisualBus.h>
#include <ScriptCanvas/Bus/RequestBus.h>

#include <Editor/View/Widgets/PropertyGridContextMenu.h>

namespace
{
    struct SlotInfo
    {
        AZ::EntityId m_id;
        QString m_name;
        bool m_isSetter; // false means "is getter".
        bool m_isVisible;
    };

    using SlotInfoList = AZStd::list< SlotInfo >;

    bool IsGraphCanvasActive()
    {
        ScriptCanvas::ScriptCanvasId scriptCanvasId;
        ScriptCanvasEditor::GeneralRequestBus::BroadcastResult(scriptCanvasId, &ScriptCanvasEditor::GeneralRequests::GetActiveScriptCanvasId);

        AZ::EntityId graphCanvasGraphId;
        ScriptCanvasEditor::GeneralRequestBus::BroadcastResult(graphCanvasGraphId, &ScriptCanvasEditor::GeneralRequests::GetActiveGraphCanvasGraphId);

        return (scriptCanvasId.IsValid() &&
                graphCanvasGraphId.IsValid());
    }

    AZ::EntityId GetEntityId(AzToolsFramework::InstanceDataNode* node)
    {
        while (node)
        {
            if ((node->GetClassMetadata()) && (node->GetClassMetadata()->m_azRtti))
            {
                if (node->GetClassMetadata()->m_azRtti->IsTypeOf(AZ::Component::RTTI_Type()))
                {
                    return static_cast<AZ::Component*>(node->GetInstance(0))->GetEntityId();
                }
            }

            node = node->GetParent();
        }

        return AZ::EntityId();
    }

    SlotInfoList BuildSlotList(const AZ::EntityId& entityId)
    {
        SlotInfoList slotsList;

        AZStd::vector<AZ::EntityId> slotIds;
        GraphCanvas::NodeRequestBus::EventResult(slotIds, entityId, &GraphCanvas::NodeRequests::GetSlotIds);
        for (AZ::EntityId slotId : slotIds)
        {
            GraphCanvas::SlotType type(GraphCanvas::SlotTypes::Invalid);
            GraphCanvas::SlotRequestBus::EventResult(type, slotId, &GraphCanvas::SlotRequests::GetSlotType);

            if (type != GraphCanvas::SlotTypes::DataSlot)
            {
                // This ISN'T a setter or getter slot.
                // Nothing to do.
                continue;
            }

            GraphCanvas::ConnectionType connectionType(GraphCanvas::ConnectionType::CT_None);
            GraphCanvas::SlotRequestBus::EventResult(connectionType, slotId, &GraphCanvas::SlotRequests::GetConnectionType);

            AZStd::string name;
            GraphCanvas::SlotRequestBus::EventResult(name, slotId, &GraphCanvas::SlotRequests::GetName);

            bool isVisible = false;
            GraphCanvas::VisualRequestBus::EventResult(isVisible, slotId, &GraphCanvas::VisualRequests::IsVisible);

            slotsList.push_back({slotId,
                                 name.c_str(),
                                 (connectionType == GraphCanvas::ConnectionType::CT_Output),
                                 isVisible});
        }

        return slotsList;
    }

    void AddVisibilityActions(ScriptCanvasEditor::Widget::PropertyGridContextMenu* rootMenu,
        const SlotInfoList& slotsList)
    {
        for (auto& slot : slotsList)
        {
            QString title(QString("%1 : %2").arg(slot.m_name,
                    slot.m_isSetter ? "setter" : "getter"));

            QAction* action = new QAction(title, rootMenu);
            action->setCheckable(true);
            action->setChecked(slot.m_isVisible);

            QObject::connect(action,
                &QAction::triggered,
                [slot]([[maybe_unused]] bool checked)
                {
                    // slot.m_isVisible is the current state, and "checked" is the new state.
                    AZ_Assert(checked != slot.m_isVisible, "Visibility out of synch");

                    GraphCanvas::VisualRequestBus::Event(slot.m_id, &GraphCanvas::VisualRequests::SetVisible, !slot.m_isVisible);
                });
            rootMenu->addAction(action);
        }
    }
} // anonymous namespace.

namespace ScriptCanvasEditor
{
    namespace Widget
    {
        PropertyGridContextMenu::PropertyGridContextMenu(AzToolsFramework::InstanceDataNode* node)
            : QMenu()
        {
            if (!IsGraphCanvasActive())
            {
                // Nothing active.
                return;
            }

            AZ::EntityId graphCanvasNodeId = GetEntityId(node);
            if (!graphCanvasNodeId.IsValid())
            {
                // Nothing to do.
                return;
            }

            AddVisibilityActions(this, BuildSlotList(graphCanvasNodeId));
        }

        #include <Editor/View/Widgets/moc_PropertyGridContextMenu.cpp>
    }
}
