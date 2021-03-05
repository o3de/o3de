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

#include <GraphCanvas/Components/NodePropertyDisplay/DataInterface.h>

#include "ScriptCanvas/Core/Slot.h"
#include "ScriptCanvas/Core/NodeBus.h"

#include "ScriptCanvas/Core/Datum.h"

#include <Editor/View/Widgets/PropertyGridBus.h>

namespace ScriptCanvasEditor
{
    template<class InterfaceType>
    class ScriptCanvasDataInterface
        : public ScriptCanvas::NodeNotificationsBus::Handler
        , public InterfaceType
    {
    protected:
        ScriptCanvasDataInterface(const AZ::EntityId& nodeId, const ScriptCanvas::SlotId& slotId)
            : m_nodeId(nodeId)
            , m_slotId(slotId)
        {
            static_assert((AZStd::is_base_of<GraphCanvas::DataInterface, InterfaceType>::value), "ScriptCanvasDataInterface given invalid GraphCanvas::DataInterface to inherit.");
            ScriptCanvas::NodeNotificationsBus::Handler::BusConnect(nodeId);
        }

        using InterfaceType::SignalValueChanged;

    public:
        virtual ~ScriptCanvasDataInterface() = default;

        const ScriptCanvas::ScriptCanvasId GetScriptCanvasId() const
        {
            ScriptCanvas::ScriptCanvasId scriptCanvasId;
            ScriptCanvas::NodeRequestBus::EventResult(scriptCanvasId, GetNodeId(), &ScriptCanvas::NodeRequests::GetOwningScriptCanvasId);

            return scriptCanvasId;
        }

        const GraphCanvas::GraphId GetGraphCanvasGraphId() const
        {
            GraphCanvas::GraphId graphCanvasGraphId;
            GeneralRequestBus::BroadcastResult(graphCanvasGraphId, &GeneralRequests::GetGraphCanvasGraphId, GetScriptCanvasId());
            return graphCanvasGraphId;
        }

        const AZ::EntityId& GetNodeId() const
        {
            return m_nodeId;
        }
        
        const ScriptCanvas::SlotId& GetSlotId() const
        {
            return m_slotId;
        }
        
        const ScriptCanvas::Datum* GetSlotObject() const
        {
            const ScriptCanvas::Datum* object = nullptr;
            ScriptCanvas::NodeRequestBus::EventResult(object, GetNodeId(), &ScriptCanvas::NodeRequests::FindDatum, GetSlotId());
            
            return object;
        }
        
        void ModifySlotObject(ScriptCanvas::ModifiableDatumView& datumView)
        {
            ScriptCanvas::NodeRequestBus::Event(GetNodeId(), &ScriptCanvas::NodeRequests::FindModifiableDatumView, GetSlotId(), datumView);
        }

        // NodeNotificationsBus
        void OnInputChanged(const ScriptCanvas::SlotId& slotId) override
        {
            if (slotId == m_slotId)
            {
                SignalValueChanged();
            }
        }

        void PostUndoPoint()
        {
            GeneralRequestBus::Broadcast(&GeneralRequests::PostUndoPoint, GetScriptCanvasId());
        }
        ////
        
    private:
        AZ::EntityId            m_nodeId;
        ScriptCanvas::SlotId    m_slotId;
    };
}