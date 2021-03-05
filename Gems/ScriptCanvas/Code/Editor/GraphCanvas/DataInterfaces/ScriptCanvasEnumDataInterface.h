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

#include <GraphCanvas/Components/NodePropertyDisplay/ComboBoxDataInterface.h>
#include <GraphCanvas/Widgets/ComboBox/ComboBoxItemModels.h>

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

        const QString& GetDisplayString() const override
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
