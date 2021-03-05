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
