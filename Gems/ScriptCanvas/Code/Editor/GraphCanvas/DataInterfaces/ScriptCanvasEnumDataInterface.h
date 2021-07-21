/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <GraphCanvas/Components/NodePropertyDisplay/ComboBoxDataInterface.h>
#include <GraphCanvas/Widgets/ComboBox/ComboBoxItemModels.h>

#include <Editor/GraphCanvas/PropertyInterfaces/ScriptCanvasPropertyDataInterface.h>

#include "ScriptCanvasDataInterface.h"

namespace ScriptCanvasEditor
{
    // Used for fixed values value input.
    class ScriptCanvasEnumDataInterface
        : public ScriptCanvasDataInterface<GraphCanvas::ComboBoxDataInterface>
    {
    public:
        AZ_CLASS_ALLOCATOR(ScriptCanvasEnumDataInterface, AZ::SystemAllocator, 0);

        ScriptCanvasEnumDataInterface(const AZ::EntityId& nodeId, const ScriptCanvas::SlotId& slotId)
            : ScriptCanvasDataInterface(nodeId, slotId)
        {
        }

        ~ScriptCanvasEnumDataInterface() = default;

        void AddElement(int32_t element, const AZStd::string& displayName)
        {
            QString qtName = displayName.c_str();
            m_comboBoxModel.AddElement(element, qtName);
        }

        // GraphCanvas::ComboBoxDataInterface
        GraphCanvas::ComboBoxItemModelInterface* GetItemInterface() override
        {
            return &m_comboBoxModel;
        }

        void AssignIndex(const QModelIndex& index) override
        {
            ScriptCanvas::ModifiableDatumView datumView;
            ModifySlotObject(datumView);

            if (datumView.IsValid())
            {
                int32_t value = m_comboBoxModel.GetValueForIndex(index);

                datumView.SetAs(value);

                PostUndoPoint();
                PropertyGridRequestBus::Broadcast(&PropertyGridRequests::RefreshPropertyGrid);
            }
        }

        QModelIndex GetAssignedIndex() const override
        {
            const ScriptCanvas::Datum* object = GetSlotObject();
            
            if (object)
            {
                const int* element = object->GetAs<int>();
                
                if (element)
                {
                    return m_comboBoxModel.GetIndexForValue(*element);
                }
            }
            
            return m_comboBoxModel.GetDefaultIndex();
        }

        QString GetDisplayString() const override
        {
            const ScriptCanvas::Datum* object = GetSlotObject();
            
            if (object)
            {
                const int* element = object->GetAs<int>();
                
                if (element)
                {
                    return m_comboBoxModel.GetNameForValue((*element));
                }
            }
            return GraphCanvas::ComboBoxDataInterface::GetDisplayString();
        }
        ////

    private:
        GraphCanvas::GraphCanvasListComboBoxModel<int32_t> m_comboBoxModel;
    };    
}
