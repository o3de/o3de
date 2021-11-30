/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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

        QString GetDisplayString() const override
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
