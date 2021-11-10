/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#ifndef CRYINCLUDE_EDITOR_UTILS_PROPERTYGENERICCTRL_H
#define CRYINCLUDE_EDITOR_UTILS_PROPERTYGENERICCTRL_H
#pragma once

#if !defined(Q_MOC_RUN)
#include <AzCore/base.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzToolsFramework/UI/PropertyEditor/PropertyEditorAPI.h>
#include "ReflectedVar.h"
#include "Util/VariablePropertyType.h"
#include <QWidget>
#endif

class QStringListModel;
class QListView;
class QLabel;
class QLineEdit;

class GenericPopupPropertyEditor
    : public QWidget
{
    Q_OBJECT
public:
    AZ_CLASS_ALLOCATOR(GenericPopupPropertyEditor, AZ::SystemAllocator, 0);
    GenericPopupPropertyEditor(QWidget* pParent = nullptr, bool showTwoButtons = false);

    void SetValue(const QString& value, bool notify = true);
    QString GetValue() const { return m_value; }
    void SetPropertyType(PropertyType type);
    PropertyType GetPropertyType() const { return m_propertyType; }

    //override in derived classes to show appropriate editor
    virtual void onEditClicked() {};
    virtual void onButton2Clicked() {};

signals:
    void ValueChanged(const QString& value);

private:
    QLabel* m_valueLabel;
    PropertyType m_propertyType;
    QString m_value;
};

template <class T, AZ::u32 CRC>
class GenericPopupWidgetHandler
    : public QObject
    , public AzToolsFramework::PropertyHandler < CReflectedVarGenericProperty, GenericPopupPropertyEditor >
{
public:
    AZ_CLASS_ALLOCATOR(GenericPopupWidgetHandler, AZ::SystemAllocator, 0);
    virtual bool IsDefaultHandler() const override { return false; }

    virtual AZ::u32 GetHandlerName(void) const override  { return CRC; }
    virtual QWidget* CreateGUI(QWidget* pParent) override
    {
        GenericPopupPropertyEditor* newCtrl = aznew T(pParent);
        connect(newCtrl, &GenericPopupPropertyEditor::ValueChanged, newCtrl, [newCtrl]()
            {
                EBUS_EVENT(AzToolsFramework::PropertyEditorGUIMessages::Bus, RequestWrite, newCtrl);
            });
        return newCtrl;
    }
    virtual void ConsumeAttribute(GenericPopupPropertyEditor* GUI, AZ::u32 attrib, AzToolsFramework::PropertyAttributeReader* attrValue, const char* debugName) override
    {
        Q_UNUSED(GUI);
        Q_UNUSED(attrib);
        Q_UNUSED(attrValue);
        Q_UNUSED(debugName);
    }
    virtual void WriteGUIValuesIntoProperty(size_t index, GenericPopupPropertyEditor* GUI, property_t& instance, AzToolsFramework::InstanceDataNode* node) override
    {
        Q_UNUSED(index);
        Q_UNUSED(node);
        CReflectedVarGenericProperty val = instance;
        val.m_propertyType = GUI->GetPropertyType();
        val.m_value = GUI->GetValue().toUtf8().data();
        instance = static_cast<property_t>(val);
    }
    virtual bool ReadValuesIntoGUI(size_t index, GenericPopupPropertyEditor* GUI, const property_t& instance, AzToolsFramework::InstanceDataNode* node)  override
    {
        Q_UNUSED(index);
        Q_UNUSED(node);
        CReflectedVarGenericProperty val = instance;
        GUI->SetPropertyType(val.m_propertyType);
        GUI->SetValue(val.m_value.c_str(), false);
        return false;
    }
};

class MissionObjPropertyEditor
    : public GenericPopupPropertyEditor
{
public:
    MissionObjPropertyEditor(QWidget* pParent = nullptr)
        : GenericPopupPropertyEditor(pParent){}
    void onEditClicked() override;
};

class SequencePropertyEditor
    : public GenericPopupPropertyEditor
{
public:
    SequencePropertyEditor(QWidget* pParent = nullptr)
        : GenericPopupPropertyEditor(pParent){}
    void onEditClicked() override;
};

class SequenceIdPropertyEditor
    : public GenericPopupPropertyEditor
{
public:
    SequenceIdPropertyEditor(QWidget* pParent = nullptr)
        : GenericPopupPropertyEditor(pParent){}
    void onEditClicked() override;
};

class LocalStringPropertyEditor
    : public GenericPopupPropertyEditor
{
public:
    LocalStringPropertyEditor(QWidget* pParent = nullptr)
        : GenericPopupPropertyEditor(pParent){}
    void onEditClicked() override;
};

class LightAnimationPropertyEditor
    : public GenericPopupPropertyEditor
{
public:
    LightAnimationPropertyEditor(QWidget* pParent = nullptr)
        : GenericPopupPropertyEditor(pParent){}
    void onEditClicked() override;
};


// AZ_CRC changed recently - it used to be evaluated by the preprocessor to AZ::u32(value); now it evaluates to Az::Crc32, and can't be used as a const template parameter
// So we use our own
#define CONST_AZ_CRC(name, value) AZ::u32(value)

using MissionObjPropertyHandler = GenericPopupWidgetHandler<MissionObjPropertyEditor, CONST_AZ_CRC("ePropertyMissionObj", 0x4a2d0dc8)>;
using SequencePropertyHandler = GenericPopupWidgetHandler<SequencePropertyEditor, CONST_AZ_CRC("ePropertySequence", 0xdd1c7d44)>;
using SequenceIdPropertyHandler = GenericPopupWidgetHandler<SequenceIdPropertyEditor, CONST_AZ_CRC("ePropertySequenceId", 0x05983dcc)>;
using LocalStringPropertyHandler = GenericPopupWidgetHandler<LocalStringPropertyEditor, CONST_AZ_CRC("ePropertyLocalString", 0x0cd9609a)>;
using LightAnimationPropertyHandler = GenericPopupWidgetHandler<LightAnimationPropertyEditor, CONST_AZ_CRC("ePropertyLightAnimation", 0x277097da)>;

class ListEditWidget : public QWidget
{
    Q_OBJECT
public:
    AZ_CLASS_ALLOCATOR(ListEditWidget, AZ::SystemAllocator, 0);
    ListEditWidget(QWidget *pParent = nullptr);

    void SetValue(const QString &value, bool notify = true);
    QString GetValue() const { return m_value; }

    QWidget* GetFirstInTabOrder();
    QWidget* GetLastInTabOrder();

signals:
    void ValueChanged(const QString &value);

private:
    void OnModelDataChange();
    virtual void OnEditClicked() {};

protected:
    QLineEdit *m_valueEdit;
    QString m_value;
    QListView *m_listView;
    QStringListModel *m_model;
};

template <class T, AZ::u32 CRC>
class ListEditWidgetHandler : public QObject, public AzToolsFramework::PropertyHandler < CReflectedVarGenericProperty, ListEditWidget >
{
public:
    AZ_CLASS_ALLOCATOR(ListEditWidgetHandler, AZ::SystemAllocator, 0);
    virtual bool IsDefaultHandler() const override { return false; }

    virtual AZ::u32 GetHandlerName(void) const override  { return CRC; }
    virtual QWidget* CreateGUI(QWidget *pParent) override
    {
        ListEditWidget* newCtrl = aznew T(pParent);
        connect(newCtrl, &ListEditWidget::ValueChanged, newCtrl, [newCtrl]()
        {
            EBUS_EVENT(AzToolsFramework::PropertyEditorGUIMessages::Bus, RequestWrite, newCtrl);
        });
        return newCtrl;
    }
    virtual void ConsumeAttribute(ListEditWidget* GUI, AZ::u32 attrib, AzToolsFramework::PropertyAttributeReader* attrValue, const char* debugName) override {
        Q_UNUSED(GUI); Q_UNUSED(attrib); Q_UNUSED(attrValue); Q_UNUSED(debugName);
    }
    virtual void WriteGUIValuesIntoProperty(size_t index, ListEditWidget* GUI, property_t& instance, AzToolsFramework::InstanceDataNode* node) override
    {
        Q_UNUSED(index);
        Q_UNUSED(node);
        CReflectedVarGenericProperty val = instance;
        val.m_value = GUI->GetValue().toUtf8().data();
        instance = static_cast<property_t>(val);
    }
    virtual bool ReadValuesIntoGUI(size_t index, ListEditWidget* GUI, const property_t& instance, AzToolsFramework::InstanceDataNode* node)  override
    {
        Q_UNUSED(index);
        Q_UNUSED(node);
        CReflectedVarGenericProperty val = instance;
        GUI->SetValue(val.m_value.c_str(), false);
        return false;
    }
    QWidget* GetFirstInTabOrder(ListEditWidget* widget) override { return widget->GetFirstInTabOrder(); }
    QWidget* GetLastInTabOrder(ListEditWidget* widget) override {return widget->GetLastInTabOrder(); }

};

#endif // CRYINCLUDE_EDITOR_UTILS_PROPERTYGENERICCTRL_H
