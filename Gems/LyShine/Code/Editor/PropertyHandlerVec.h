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
        return AZ_CRC("Legacy_Vector_Property_Handler", 0x67bdda50);
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
    AZ_CLASS_ALLOCATOR(PropertyHandlerVec2, AZ::SystemAllocator, 0);

    PropertyHandlerVec2()
        : LegacyVectorPropertyHandlerBase(2)
    {
    }

    AZ::u32 GetHandlerName(void) const override { return AZ_CRC("Vec2", 0xa70b4073); }
};

//////////////////////////////////////////////////////////////////////////

class PropertyHandlerVec3
    : public LegacyVectorPropertyHandlerBase<Vec3>
{
public:
    AZ_CLASS_ALLOCATOR(PropertyHandlerVec3, AZ::SystemAllocator, 0);

    PropertyHandlerVec3()
        : LegacyVectorPropertyHandlerBase(3)
    {
    }

    AZ::u32 GetHandlerName(void) const override { return AZ_CRC("Vec3", 0xd00c70e5); }
};

//////////////////////////////////////////////////////////////////////////

class PropertyHandlerVec4
    : public LegacyVectorPropertyHandlerBase<Vec4>
{
public:
    AZ_CLASS_ALLOCATOR(PropertyHandlerVec3, AZ::SystemAllocator, 0);

    PropertyHandlerVec4()
        : LegacyVectorPropertyHandlerBase(4)
    {
    }

    AZ::u32 GetHandlerName(void) const override { return AZ_CRC("Vec4", 0x4e68e546); }
};
//////////////////////////////////////////////////////////////////////////

void PropertyHandlerVecRegister();
