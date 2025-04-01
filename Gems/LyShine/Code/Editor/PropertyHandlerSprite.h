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
#include <QtWidgets/QWidget>
#include <AzToolsFramework/UI/PropertyEditor/PropertyEditorAPI.h>

#include <AzToolsFramework/UI/PropertyEditor/PropertyAssetCtrl.hxx>
#endif

class PropertySpriteCtrl
    : public QWidget
{
    Q_OBJECT

public:

    AZ_CLASS_ALLOCATOR(PropertySpriteCtrl, AZ::SystemAllocator);

    PropertySpriteCtrl(QWidget* parent = nullptr);

    void dragEnterEvent(QDragEnterEvent* ev) override;
    void dragLeaveEvent(QDragLeaveEvent* ev) override;
    void dropEvent(QDropEvent* ev) override;

    AzToolsFramework::PropertyAssetCtrl* GetPropertyAssetCtrl();

private:

    AzToolsFramework::PropertyAssetCtrl* m_propertyAssetCtrl;
};

//-------------------------------------------------------------------------------

class PropertyHandlerSprite
    : public AzToolsFramework::PropertyHandler<AzFramework::SimpleAssetReferenceBase, PropertySpriteCtrl>
{
public:
    AZ_CLASS_ALLOCATOR(PropertyHandlerSprite, AZ::SystemAllocator);
    
    AZ::u32 GetHandlerName(void) const override  { return AZ_CRC_CE("Sprite"); }

    QWidget* CreateGUI(QWidget* pParent) override;
    void ConsumeAttribute(PropertySpriteCtrl* GUI, AZ::u32 attrib, AzToolsFramework::PropertyAttributeReader* attrValue, const char* debugName) override;
    void WriteGUIValuesIntoProperty(size_t index, PropertySpriteCtrl* GUI, property_t& instance, AzToolsFramework::InstanceDataNode* node) override;
    bool ReadValuesIntoGUI(size_t index, PropertySpriteCtrl* GUI, const property_t& instance, AzToolsFramework::InstanceDataNode* node)  override;

    static void Register();
};
