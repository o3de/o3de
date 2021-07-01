/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#ifndef CRYINCLUDE_EDITOR_UTILS_PROPERTYANIMATIONCTRL_H
#define CRYINCLUDE_EDITOR_UTILS_PROPERTYANIMATIONCTRL_H
#pragma once

#if !defined(Q_MOC_RUN)
#include <AzCore/base.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzToolsFramework/UI/PropertyEditor/PropertyEditorAPI.h>
#include "ReflectedVar.h"
#include <QWidget>
#include <QPointer>
#endif

class QToolButton;
class QLabel;
class QHBoxLayout;

class AnimationPropertyCtrl
    : public QWidget
{
    Q_OBJECT
public:
    AZ_CLASS_ALLOCATOR(AnimationPropertyCtrl, AZ::SystemAllocator, 0);

    AnimationPropertyCtrl(QWidget* pParent = nullptr);
    virtual ~AnimationPropertyCtrl();

    CReflectedVarAnimation value() const;

    QWidget* GetFirstInTabOrder();
    QWidget* GetLastInTabOrder();
    void UpdateTabOrder();

signals:
    void ValueChanged(CReflectedVarAnimation value);

public slots:
    void SetValue(const CReflectedVarAnimation& animation);

protected slots:
    void OnApplyClicked();

private:
    QToolButton* m_pApplyButton;
    QLabel* m_animationLabel;

    CReflectedVarAnimation m_animation;
};

class AnimationPropertyWidgetHandler
    : QObject
    , public AzToolsFramework::PropertyHandler < CReflectedVarAnimation, AnimationPropertyCtrl >
{
public:
    AZ_CLASS_ALLOCATOR(AnimationPropertyWidgetHandler, AZ::SystemAllocator, 0);

    virtual AZ::u32 GetHandlerName(void) const override  { return AZ_CRC("Animation", 0x8d5284dc); }
    virtual bool IsDefaultHandler() const override { return true; }
    virtual QWidget* GetFirstInTabOrder(AnimationPropertyCtrl* widget) override { return widget->GetFirstInTabOrder(); }
    virtual QWidget* GetLastInTabOrder(AnimationPropertyCtrl* widget) override { return widget->GetLastInTabOrder(); }
    virtual void UpdateWidgetInternalTabbing(AnimationPropertyCtrl* widget) override { widget->UpdateTabOrder(); }

    virtual QWidget* CreateGUI(QWidget* pParent) override;
    virtual void ConsumeAttribute(AnimationPropertyCtrl* GUI, AZ::u32 attrib, AzToolsFramework::PropertyAttributeReader* attrValue, const char* debugName) override;
    virtual void WriteGUIValuesIntoProperty(size_t index, AnimationPropertyCtrl* GUI, property_t& instance, AzToolsFramework::InstanceDataNode* node) override;
    virtual bool ReadValuesIntoGUI(size_t index, AnimationPropertyCtrl* GUI, const property_t& instance, AzToolsFramework::InstanceDataNode* node)  override;
};


#endif // CRYINCLUDE_EDITOR_UTILS_PROPERTYANIMATIONCTRL_H
