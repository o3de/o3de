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

#include <ScriptCanvas/Core/Datum.h>
#include <ScriptCanvas/Core/Node.h>
#include <ScriptCanvas/Core/NodeBus.h>
#include <ScriptCanvas/Core/Slot.h>

#include <Editor/View/Widgets/PropertyGridBus.h>

namespace ScriptCanvasEditor
{
    template<class InterfaceType, typename DataType>
    class ScriptCanvasPropertyDataInterface
        : public ScriptCanvas::NodePropertyInterfaceListener
        , public InterfaceType
    {
    protected:
        ScriptCanvasPropertyDataInterface(const AZ::EntityId& nodeId, ScriptCanvas::TypedNodePropertyInterface<DataType>* nodePropertyInterface)        
            : m_nodeId(nodeId)
            , m_nodePropertyInterface(nodePropertyInterface)
        {
            static_assert((AZStd::is_base_of<GraphCanvas::DataInterface, InterfaceType>::value), "ScriptCanvasPropertyDataInterface given invalid GraphCanvas::DataInterface to inherit.");

            if (m_nodePropertyInterface)
            {
                m_nodePropertyInterface->RegisterListener(this);
            }
        }

        using InterfaceType::SignalValueChanged;

    public:
        virtual ~ScriptCanvasPropertyDataInterface() = default;

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
        
        ScriptCanvas::TypedNodePropertyInterface<DataType>* GetNodePropertyInterface()
        {
            return m_nodePropertyInterface;
        }

        // NodeNotificationsBus
        void OnPropertyChanged() override
        {
            SignalValueChanged();
        }

        void PushUndoBlock()
        {
            GeneralRequestBus::Broadcast(&GeneralRequests::PushPreventUndoStateUpdate);
        }

        void PopUndoBlock()
        {
            GeneralRequestBus::Broadcast(&GeneralRequests::PopPreventUndoStateUpdate);
        }

        void PostUndoPoint()
        {
            GeneralRequestBus::Broadcast(&GeneralRequests::PostUndoPoint, GetScriptCanvasId());
        }
        ////

        void SetValue(const DataType& valueType)
        {
            PushUndoBlock();
            m_nodePropertyInterface->SetPropertyData(valueType);
            PopUndoBlock();
            
            PostUndoPoint();
            PropertyGridRequestBus::Broadcast(&PropertyGridRequests::RefreshPropertyGrid);
        }
        
        const DataType& GetValue() const
        {
            static DataType s_staticReturn = {};

            if (m_nodePropertyInterface)
            {
                const DataType* dataValue = m_nodePropertyInterface->GetPropertyData();
                
                if (dataValue)
                {
                    return (*dataValue);
                }
            }
            
            return s_staticReturn;
        }
        
    private:
    
        ScriptCanvas::TypedNodePropertyInterface<DataType>* m_nodePropertyInterface;
    
        AZ::EntityId            m_nodeId;
    };
}
