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

#include "ScriptCanvasPropertyDataInterface.h"

namespace ScriptCanvasEditor
{
    class ScriptCanvasEnumComboBoxPropertyDataInterface
        : public ScriptCanvasComboBoxPropertyDataInterface<int>
    {
    public:
        AZ_CLASS_ALLOCATOR(ScriptCanvasEnumComboBoxPropertyDataInterface, AZ::SystemAllocator, 0);

        ScriptCanvasEnumComboBoxPropertyDataInterface(AZ::EntityId scriptCanvasNodeId, ScriptCanvas::EnumComboBoxNodePropertyInterface* propertyInterface)
            : ScriptCanvasComboBoxPropertyDataInterface<int>(scriptCanvasNodeId, propertyInterface)
            , m_propertyInterface(propertyInterface)
        {
            const auto& displaySet = m_propertyInterface->GetValueSet();

            for (const auto& displayPair : displaySet)
            {
                QString displayString = displayPair.first.c_str();
                m_comboBoxModel.AddElement(displayPair.second, displayString);
            }
        }

        ~ScriptCanvasEnumComboBoxPropertyDataInterface() = default;

        // GraphCanvas::ComboBoxDataInterface
        GraphCanvas::ComboBoxItemModelInterface* GetItemInterface() override
        {
            return &m_comboBoxModel;
        }

        void AssignIndex(const QModelIndex& index) override
        {
            int32_t dataValue = m_comboBoxModel.GetValueForIndex(index);
            SetValue(dataValue);
        }

        QModelIndex GetAssignedIndex() const override
        {
            int32_t dataValue = GetValue();
            return m_comboBoxModel.GetIndexForValue(dataValue);
        }

        QString GetDisplayString() const
        {
            int32_t dataValue = GetValue();
            return m_comboBoxModel.GetNameForValue(dataValue);
        }
        ////

    private:

        ScriptCanvas::TypedComboBoxNodePropertyInterface<int>* m_propertyInterface;
        GraphCanvas::GraphCanvasListComboBoxModel<int> m_comboBoxModel;
    };    
}
