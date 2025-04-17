/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#if !defined(Q_MOC_RUN)
#include <AzToolsFramework/UI/PropertyEditor/PropertyStringLineEditCtrl.hxx>
#endif

class PropertyHandlerChar
    : private QObject
    , public AzToolsFramework::PropertyHandler<uint32_t, AzToolsFramework::PropertyStringLineEditCtrl>
{
    // this is a Qt Object purely so it can connect to slots with context.  This is the only reason its in this header.
    Q_OBJECT
public:
    AZ_CLASS_ALLOCATOR(PropertyHandlerChar, AZ::SystemAllocator);

    AZ::u32 GetHandlerName(void) const override  { return AZ_CRC_CE("Char"); }
    bool IsDefaultHandler() const override { return true; }
    QWidget* CreateGUI(QWidget* pParent) override;
    void ConsumeAttribute(AzToolsFramework::PropertyStringLineEditCtrl* GUI, AZ::u32 attrib, AzToolsFramework::PropertyAttributeReader* attrValue, const char* debugName) override;
    void WriteGUIValuesIntoProperty(size_t index, AzToolsFramework::PropertyStringLineEditCtrl* GUI, property_t& instance, AzToolsFramework::InstanceDataNode* node) override;
    bool ReadValuesIntoGUI(size_t index, AzToolsFramework::PropertyStringLineEditCtrl* GUI, const property_t& instance, AzToolsFramework::InstanceDataNode* node)  override;

    static void Register();
};
