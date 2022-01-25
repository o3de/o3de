/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#ifndef CRYINCLUDE_EDITOR_UTILS_PROPERTYRESOURCECTRL_H
#define CRYINCLUDE_EDITOR_UTILS_PROPERTYRESOURCECTRL_H
#pragma once

#if !defined(Q_MOC_RUN)
#include <AzCore/base.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzToolsFramework/UI/PropertyEditor/PropertyEditorAPI.h>
#include "ReflectedVar.h"
#include "Util/VariablePropertyType.h"
#include <QWidget>
#include <QtWidgets/QToolButton>
#include <QtCore/QVector>
#endif

class QLineEdit;
class QHBoxLayout;
class CBitmapToolTip;
class QToolTipWidget;

class BrowseButton
    : public QToolButton
{
    Q_OBJECT
public:
    AZ_CLASS_ALLOCATOR(BrowseButton, AZ::SystemAllocator, 0);

    BrowseButton(PropertyType type, QWidget* parent = nullptr);

    void SetPath(const QString& path) { m_path = path; }
    QString GetPath() const { return m_path; }

    PropertyType GetPropertyType() const {return m_propertyType; }

signals:
    void PathChanged(const QString& path);

protected:
    void SetPathAndEmit(const QString& path);
    virtual void OnClicked() = 0;

    PropertyType m_propertyType;
    QString m_path;
};

class FileResourceSelectorWidget
    : public QWidget
{
    Q_OBJECT
public:
    AZ_CLASS_ALLOCATOR(FileResourceSelectorWidget, AZ::SystemAllocator, 0);
    FileResourceSelectorWidget(QWidget* pParent = nullptr);

    bool SetPath(const QString& path);
    QString GetPath() const;
    void SetPropertyType(PropertyType type);
    PropertyType GetPropertyType() const { return m_propertyType; }

    QWidget* GetFirstInTabOrder();
    QWidget* GetLastInTabOrder();
    void UpdateTabOrder();

    bool eventFilter(QObject* obj, QEvent* event) override;

signals:
    void PathChanged(const QString& path);

protected:
    bool event(QEvent* event) override;

private:
    void OnAssignClicked();
    void OnMaterialClicked();

    void UpdateWidgets();
    void AddButton(BrowseButton* button);
    void OnPathChanged(const QString& path);

private:
    QLineEdit* m_pathEdit;
    PropertyType m_propertyType;
    QString m_path;

    QHBoxLayout* m_mainLayout;
    QVector<BrowseButton*> m_buttons;
    QScopedPointer<CBitmapToolTip> m_previewToolTip;
    QToolTipWidget* m_tooltip;
};

class FileResourceSelectorWidgetHandler
    : QObject
    , public AzToolsFramework::PropertyHandler < CReflectedVarResource, FileResourceSelectorWidget >
{
    Q_OBJECT
public:
    AZ_CLASS_ALLOCATOR(FileResourceSelectorWidgetHandler, AZ::SystemAllocator, 0);

    virtual AZ::u32 GetHandlerName(void) const override  { return AZ_CRC("Resource", 0xbc91f416); }
    virtual bool IsDefaultHandler() const override { return true; }
    virtual QWidget* GetFirstInTabOrder(FileResourceSelectorWidget* widget) override { return widget->GetFirstInTabOrder(); }
    virtual QWidget* GetLastInTabOrder(FileResourceSelectorWidget* widget) override { return widget->GetLastInTabOrder(); }
    virtual void UpdateWidgetInternalTabbing(FileResourceSelectorWidget* widget) override { widget->UpdateTabOrder(); }

    virtual QWidget* CreateGUI(QWidget* pParent) override;
    virtual void ConsumeAttribute(FileResourceSelectorWidget* GUI, AZ::u32 attrib, AzToolsFramework::PropertyAttributeReader* attrValue, const char* debugName) override;
    virtual void WriteGUIValuesIntoProperty(size_t index, FileResourceSelectorWidget* GUI, property_t& instance, AzToolsFramework::InstanceDataNode* node) override;
    virtual bool ReadValuesIntoGUI(size_t index, FileResourceSelectorWidget* GUI, const property_t& instance, AzToolsFramework::InstanceDataNode* node)  override;
};

#endif // CRYINCLUDE_EDITOR_UTILS_PROPERTYRESOURCECTRL_H
