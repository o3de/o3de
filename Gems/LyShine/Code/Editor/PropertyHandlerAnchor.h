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

class AnchorPresetsWidget;

class PropertyAnchorCtrl
    : public QWidget
{
    Q_OBJECT

public:

    AZ_CLASS_ALLOCATOR(PropertyAnchorCtrl, AZ::SystemAllocator);

    PropertyAnchorCtrl(QWidget* parent = nullptr);

    void ConsumeAttribute(AZ::u32 attrib, AzToolsFramework::PropertyAttributeReader* attrValue, const char* debugName);

    AnchorPresetsWidget* GetAnchorPresetsWidget();
    AzQtComponents::VectorInput* GetPropertyVectorCtrl();

    bool IsReadOnly() { return m_isReadOnly; }

private:

    AzToolsFramework::VectorPropertyHandlerCommon m_common;
    AzQtComponents::VectorInput* m_propertyVectorCtrl;
    AnchorPresetsWidget* m_anchorPresetsWidget;
    QLabel* m_disabledLabel;
    QLabel* m_controlledByFitterLabel;
    bool m_isReadOnly;
};

//-------------------------------------------------------------------------------

class PropertyHandlerAnchor
    : public AzToolsFramework::PropertyHandler < UiTransform2dInterface::Anchors, PropertyAnchorCtrl >
{
public:
    AZ_CLASS_ALLOCATOR(PropertyHandlerAnchor, AZ::SystemAllocator);

    AZ::u32 GetHandlerName(void) const override  { return AZ_CRC_CE("Anchor"); }
    bool IsDefaultHandler() const override { return true; }

    QWidget* CreateGUI(QWidget* pParent) override;
    void ConsumeAttribute(PropertyAnchorCtrl* GUI, AZ::u32 attrib, AzToolsFramework::PropertyAttributeReader* attrValue, const char* debugName) override;
    void WriteGUIValuesIntoProperty(size_t index, PropertyAnchorCtrl* GUI, property_t& instance, AzToolsFramework::InstanceDataNode* node) override;
    bool ReadValuesIntoGUI(size_t index, PropertyAnchorCtrl* GUI, const property_t& instance, AzToolsFramework::InstanceDataNode* node)  override;
    bool ModifyTooltip(QWidget* widget, QString& toolTipString) override;

    AZ::EntityId GetParentEntityId(AzToolsFramework::InstanceDataNode* node, size_t index);

    static void Register();
};
