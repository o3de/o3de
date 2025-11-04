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
#include <AzToolsFramework/UI/PropertyEditor/PropertyEditorAPI.h>
#include "ReflectedVar.h"
#include "Util/VariablePropertyType.h"
#include "Controls/SplineCtrl.h"
#include <QWidget>
#endif

class QLabel;
class QLineEdit;

class UserPropertyEditor : public QWidget
{
    Q_OBJECT
public:
    AZ_CLASS_ALLOCATOR(UserPropertyEditor, AZ::SystemAllocator);
    UserPropertyEditor(QWidget *pParent = nullptr);

    void SetValue(const QString &value, bool notify = true);
    QString GetValue() const { return m_value; }

    void SetData(bool canEdit, bool useTree, const QString &treeSeparator, const QString &dialogTitle, const std::vector<IVariable::IGetCustomItems::SItem>& items);
    void onEditClicked();

signals:
    void ValueChanged(const QString &value);
    void RefreshItems();

private:
    QLabel *m_valueLabel;
    QString m_value;

    bool m_canEdit;
    bool m_useTree;
    QString m_treeSeparator;
    QString m_dialogTitle;
    std::vector<IVariable::IGetCustomItems::SItem> m_items;
};

class UserPopupWidgetHandler : public QObject, public AzToolsFramework::PropertyHandler < CReflectedVarUser, UserPropertyEditor>
{
    Q_OBJECT
public:
    AZ_CLASS_ALLOCATOR(UserPopupWidgetHandler, AZ::SystemAllocator);
    bool IsDefaultHandler() const override { return false; }
    QWidget* CreateGUI(QWidget *pParent) override;

    AZ::u32 GetHandlerName(void) const override  {return AZ_CRC_CE("ePropertyUser"); }
    void ConsumeAttribute(UserPropertyEditor* GUI, AZ::u32 attrib, AzToolsFramework::PropertyAttributeReader* attrValue, const char* debugName) override;
    void WriteGUIValuesIntoProperty(size_t index, UserPropertyEditor* GUI, property_t& instance, AzToolsFramework::InstanceDataNode* node) override;
    bool ReadValuesIntoGUI(size_t index, UserPropertyEditor* GUI, const property_t& instance, AzToolsFramework::InstanceDataNode* node)  override;

};

class FloatCurveHandler : public QObject, public AzToolsFramework::PropertyHandler < CReflectedVarSpline, CSplineCtrl>
{
    Q_OBJECT
public:
    AZ_CLASS_ALLOCATOR(FloatCurveHandler, AZ::SystemAllocator);
    bool IsDefaultHandler() const override { return false; }
    QWidget* CreateGUI(QWidget *pParent) override;

    AZ::u32 GetHandlerName(void) const override { return AZ_CRC_CE("ePropertyFloatCurve"); }

    void ConsumeAttribute(CSplineCtrl* GUI, AZ::u32 attrib, AzToolsFramework::PropertyAttributeReader* attrValue, const char* debugName) override;
    void WriteGUIValuesIntoProperty(size_t index, CSplineCtrl* GUI, property_t& instance, AzToolsFramework::InstanceDataNode* node) override;
    bool ReadValuesIntoGUI(size_t index, CSplineCtrl* GUI, const property_t& instance, AzToolsFramework::InstanceDataNode* node)  override;

    void OnSplineChange(CSplineCtrl*);
};
