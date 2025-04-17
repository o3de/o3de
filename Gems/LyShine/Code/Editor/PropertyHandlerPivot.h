/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#if !defined(Q_MOC_RUN)
#include <AzCore/base.h>
#include <AzCore/Memory/SystemAllocator.h>

#include <AzToolsFramework/UI/PropertyEditor/PropertyVectorCtrl.hxx>
#include <AzToolsFramework/UI/PropertyEditor/PropertyEditorAPI.h>

#include <LyShine/UiBase.h>
#include <LyShine/Bus/UiTransform2dBus.h>
#endif

class PivotPresetsWidget;

class PropertyPivotCtrl
    : public QWidget
{
    Q_OBJECT

public:

    AZ_CLASS_ALLOCATOR(PropertyPivotCtrl, AZ::SystemAllocator);

    PropertyPivotCtrl(QWidget* parent = nullptr);

    void ConsumeAttribute(AZ::u32 attrib, AzToolsFramework::PropertyAttributeReader* attrValue, const char* debugName);

    PivotPresetsWidget* GetPivotPresetsWidget();
    AzQtComponents::VectorInput* GetPropertyVectorCtrl();

private:

    AzToolsFramework::VectorPropertyHandlerCommon m_common;
    AzQtComponents::VectorInput* m_propertyVectorCtrl;
    PivotPresetsWidget* m_pivotPresetsWidget;
};

//-------------------------------------------------------------------------------

class PropertyHandlerPivot
    : public AzToolsFramework::PropertyHandler < AZ::Vector2, PropertyPivotCtrl >
{
public:
    AZ_CLASS_ALLOCATOR(PropertyHandlerPivot, AZ::SystemAllocator);

    AZ::u32 GetHandlerName(void) const override  { return AZ_CRC_CE("Pivot"); }

    QWidget* CreateGUI(QWidget* pParent) override;
    void ConsumeAttribute(PropertyPivotCtrl* GUI, AZ::u32 attrib, AzToolsFramework::PropertyAttributeReader* attrValue, const char* debugName) override;
    void WriteGUIValuesIntoProperty(size_t index, PropertyPivotCtrl* GUI, property_t& instance, AzToolsFramework::InstanceDataNode* node) override;
    bool ReadValuesIntoGUI(size_t index, PropertyPivotCtrl* GUI, const property_t& instance, AzToolsFramework::InstanceDataNode* node)  override;

    AZ::EntityId GetParentEntityId(AzToolsFramework::InstanceDataNode* node, size_t index);

    static void Register();
};
