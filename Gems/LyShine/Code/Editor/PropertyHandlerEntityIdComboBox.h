/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#if !defined(Q_MOC_RUN)
#include "EditorCommon.h"

#include <AzCore/Component/EntityId.h>
#include <QWidget>
#endif

class PropertyEntityIdComboBoxCtrl
    : public QWidget
{
    Q_OBJECT
public:
    AZ_CLASS_ALLOCATOR(PropertyEntityIdComboBoxCtrl, AZ::SystemAllocator);

    PropertyEntityIdComboBoxCtrl(QWidget *pParent = NULL);
    virtual ~PropertyEntityIdComboBoxCtrl() = default;

    AZ::EntityId value() const;
    void addEnumValue(AZStd::pair<AZ::EntityId, AZStd::string>& val);
    void addEnumValues(AZStd::vector<AZStd::pair<AZ::EntityId, AZStd::string> >& vals);

    QWidget* GetFirstInTabOrder();
    QWidget* GetLastInTabOrder();
    void UpdateTabOrder();

Q_SIGNALS:
    void valueChanged(AZ::EntityId newValue);

    public Q_SLOTS:
    void setValue(AZ::EntityId val);

    protected Q_SLOTS:
    void onChildComboBoxValueChange(int comboBoxIndex);

private:
    QComboBox *m_pComboBox;
    AZStd::vector< AZStd::pair<AZ::EntityId, AZStd::string>  > m_enumValues;
};

class PropertyHandlerEntityIdComboBox 
    : private QObject
    , public AzToolsFramework::PropertyHandler<AZ::EntityId, PropertyEntityIdComboBoxCtrl>
{
    // this is a Qt Object purely so it can connect to slots with context.  This is the only reason its in this header.
    Q_OBJECT
public:
    AZ_CLASS_ALLOCATOR(PropertyHandlerEntityIdComboBox, AZ::SystemAllocator);

    virtual void WriteGUIValuesIntoProperty(size_t index, PropertyEntityIdComboBoxCtrl* GUI, property_t& instance, AzToolsFramework::InstanceDataNode* node) override;
    virtual bool ReadValuesIntoGUI(size_t index, PropertyEntityIdComboBoxCtrl* GUI, const property_t& instance, AzToolsFramework::InstanceDataNode* node)  override;
    virtual QWidget* CreateGUI(QWidget* pParent) override;

    static void Register();

    virtual AZ::u32 GetHandlerName(void) const override { return AZ::Edit::UIHandlers::ComboBox; }
    virtual QWidget* GetFirstInTabOrder(PropertyEntityIdComboBoxCtrl* widget) override { return widget->GetFirstInTabOrder(); }
    virtual QWidget* GetLastInTabOrder(PropertyEntityIdComboBoxCtrl* widget) override { return widget->GetLastInTabOrder(); }
    virtual void UpdateWidgetInternalTabbing(PropertyEntityIdComboBoxCtrl* widget) override { widget->UpdateTabOrder(); }
    virtual void ConsumeAttribute(PropertyEntityIdComboBoxCtrl* GUI, AZ::u32 attrib, AzToolsFramework::PropertyAttributeReader* attrValue, const char* debugName) override;
};


