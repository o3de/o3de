/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/base.h>
#include <AzCore/Memory/SystemAllocator.h>

#include <AzToolsFramework/UI/PropertyEditor/PropertyVectorCtrl.hxx>
#include <AzToolsFramework/UI/PropertyEditor/PropertyEditorAPI.h>

#include <LyShine/UiBase.h>

template <class TypeBeingHandled>
class LegacyVectorPropertyHandlerBase
    : public AzToolsFramework::PropertyHandler < TypeBeingHandled, AzQtComponents::VectorInput >
{
protected:
    AzToolsFramework::VectorPropertyHandlerCommon m_common;
public:
    LegacyVectorPropertyHandlerBase(int elementCount, int elementsPerRow = -1, AZStd::string customLabels = "")
        : m_common(elementCount, elementsPerRow, customLabels)
    {
    }

    AZ::u32 GetHandlerName(void) const override
    {
        return AZ_CRC_CE("Legacy_Vector_Property_Handler");
    }

    bool IsDefaultHandler() const override
    {
        return true;
    }

    QWidget* GetFirstInTabOrder(AzQtComponents::VectorInput* widget) override
    {
        return widget->GetFirstInTabOrder();
    }

    QWidget* GetLastInTabOrder(AzQtComponents::VectorInput* widget) override
    {
        return widget->GetLastInTabOrder();
    }

    void UpdateWidgetInternalTabbing(AzQtComponents::VectorInput* widget) override
    {
        widget->UpdateTabOrder();
    }

    QWidget* CreateGUI(QWidget* pParent) override
    {
        return m_common.ConstructGUI(pParent);
    }

    void ConsumeAttribute(AzQtComponents::VectorInput* GUI, AZ::u32 attrib, AzToolsFramework::PropertyAttributeReader* attrValue, const char* debugName) override
    {
        m_common.ConsumeAttributes(GUI, attrib, attrValue, debugName);
    }

    void WriteGUIValuesIntoProperty(size_t, AzQtComponents::VectorInput* GUI, TypeBeingHandled& instance, AzToolsFramework::InstanceDataNode*) override
    {
        AzQtComponents::VectorElement** elements = GUI->getElements();
        TypeBeingHandled actualValue = instance;
        for (int idx = 0; idx < m_common.GetElementCount(); ++idx)
        {
            if (elements[idx]->wasValueEditedByUser())
            {
                actualValue[idx] = aznumeric_cast<typename TypeBeingHandled::value_type>(elements[idx]->getValue());
            }
        }
        instance = actualValue;
    }

    bool ReadValuesIntoGUI(size_t, AzQtComponents::VectorInput* GUI, const TypeBeingHandled& instance, AzToolsFramework::InstanceDataNode*) override
    {
        GUI->blockSignals(true);

        for (int idx = 0; idx < m_common.GetElementCount(); ++idx)
        {
            GUI->setValuebyIndex(instance[idx], idx);
        }

        GUI->blockSignals(false);
        return false;
    }
};

class PropertyHandlerVec2
    : public LegacyVectorPropertyHandlerBase<Vec2>
{
public:
    AZ_CLASS_ALLOCATOR(PropertyHandlerVec2, AZ::SystemAllocator);

    PropertyHandlerVec2()
        : LegacyVectorPropertyHandlerBase(2)
    {
    }

    AZ::u32 GetHandlerName(void) const override { return AZ_CRC_CE("Vec2"); }
};

//////////////////////////////////////////////////////////////////////////

class PropertyHandlerVec3
    : public LegacyVectorPropertyHandlerBase<Vec3>
{
public:
    AZ_CLASS_ALLOCATOR(PropertyHandlerVec3, AZ::SystemAllocator);

    PropertyHandlerVec3()
        : LegacyVectorPropertyHandlerBase(3)
    {
    }

    AZ::u32 GetHandlerName(void) const override { return AZ_CRC_CE("Vec3"); }
};

//////////////////////////////////////////////////////////////////////////

class PropertyHandlerVec4
    : public LegacyVectorPropertyHandlerBase<Vec4>
{
public:
    AZ_CLASS_ALLOCATOR(PropertyHandlerVec3, AZ::SystemAllocator);

    PropertyHandlerVec4()
        : LegacyVectorPropertyHandlerBase(4)
    {
    }

    AZ::u32 GetHandlerName(void) const override { return AZ_CRC_CE("Vec4"); }
};
//////////////////////////////////////////////////////////////////////////

void PropertyHandlerVecRegister();
