/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <GraphCanvas/Components/NodePropertyDisplay/DataInterface.h>

#include <ScriptCanvas/Core/Datum.h>
#include <ScriptCanvas/Core/Node.h>
#include <ScriptCanvas/Core/NodeBus.h>
#include <ScriptCanvas/Core/Slot.h>

#include <Editor/View/Widgets/PropertyGridBus.h>

// ComboBoxDataInterface
#include <GraphCanvas/Components/NodePropertyDisplay/ComboBoxDataInterface.h>
#include <GraphCanvas/Widgets/ComboBox/ComboBoxItemModelInterface.h>
#include <GraphCanvas/Widgets/ComboBox/ComboBoxItemModels.h>
////

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

    // ComboBoxDataInterface
    template<typename DataType>
    class ScriptCanvasComboBoxPropertyDataInterface
        : public ScriptCanvasPropertyDataInterface<GraphCanvas::ComboBoxDataInterface, DataType>
    {
    public:

        AZ_CLASS_ALLOCATOR(ScriptCanvasComboBoxPropertyDataInterface<DataType>, AZ::SystemAllocator);

        ScriptCanvasComboBoxPropertyDataInterface(AZ::EntityId scriptCanvasNodeId, ScriptCanvas::TypedComboBoxNodePropertyInterface<DataType>* propertyInterface)
            : ScriptCanvasPropertyDataInterface<GraphCanvas::ComboBoxDataInterface, DataType>(scriptCanvasNodeId, propertyInterface)
            , m_propertyInterface(propertyInterface)
        {
            const auto& displaySet = m_propertyInterface->GetValueSet();

            for (const auto& displayPair : displaySet)
            {
                QString displayString = displayPair.first.c_str();
                m_comboBoxModel.AddElement(displayPair.second, displayString);
            }
        }

        ~ScriptCanvasComboBoxPropertyDataInterface() = default;

        // GraphCanvas::ComboBoxDataInterface

        // Necessary to deal with Clang invoking base methods from this templated class
        using ScriptCanvasPropertyDataInterface<GraphCanvas::ComboBoxDataInterface, DataType>::GetValue;
        using ScriptCanvasPropertyDataInterface<GraphCanvas::ComboBoxDataInterface, DataType>::SetValue;

        // Returns the EnumModel used to populate the DropDown and AutoCompleter Menu
        GraphCanvas::ComboBoxItemModelInterface* GetItemInterface() override
        {
            return &m_comboBoxModel;
        }

        // Interfaces for manipulating the values. Indexes will refer to the elements within the ComboBoxModel
        void AssignIndex(const QModelIndex& index) override
        {
            DataType dataValue = m_comboBoxModel.GetValueForIndex(index);
            this->SetValue(dataValue);
        }

        QModelIndex GetAssignedIndex() const override
        {
            DataType dataValue = this->GetValue();
            return m_comboBoxModel.GetIndexForValue(dataValue);
        }

        QString GetDisplayString() const override
        {
            DataType dataValue = this->GetValue();
            return m_comboBoxModel.GetNameForValue(dataValue);
        }

        ////

    private:

        ScriptCanvas::TypedComboBoxNodePropertyInterface<DataType>* m_propertyInterface;
        GraphCanvas::GraphCanvasListComboBoxModel<DataType> m_comboBoxModel;
    };
    ////
}
