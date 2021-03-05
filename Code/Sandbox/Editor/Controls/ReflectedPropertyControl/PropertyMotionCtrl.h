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

#ifndef CRYINCLUDE_EDITOR_UTILS_PROPERTYMOTIONCTRL_H
#define CRYINCLUDE_EDITOR_UTILS_PROPERTYMOTIONCTRL_H
#pragma once

#if !defined(Q_MOC_RUN)
#include <AzCore/base.h>
#include <AzCore/Memory/SystemAllocator.h>
#include "ReflectedVar.h"
#include <QWidget>
#include <QPointer>
#include <AzToolsFramework/UI/PropertyEditor/PropertyEditorAPI.h>

#endif

class QToolButton;
class QLabel;
class QHBoxLayout;

namespace AzToolsFramework
{
    class PropertyAssetCtrl;
}

class MotionPropertyCtrl
    : public QWidget
{
    Q_OBJECT
public:
    AZ_CLASS_ALLOCATOR(MotionPropertyCtrl, AZ::SystemAllocator, 0);

    MotionPropertyCtrl(QWidget* pParent = nullptr);
    virtual ~MotionPropertyCtrl();

    CReflectedVarMotion value() const;

    QWidget* GetFirstInTabOrder();
    QWidget* GetLastInTabOrder();
    void UpdateTabOrder();

signals:
    void ValueChanged(CReflectedVarMotion value);

public slots:
    void SetValue(const CReflectedVarMotion& motion);

protected slots:
    void OnBrowseClicked();
    void OnApplyClicked();

private:
    void SetLabelText(const AZStd::string& motion);

    QToolButton* m_pBrowseButton;
    QToolButton* m_pApplyButton;
    QLabel* m_motionLabel;

    CReflectedVarMotion m_motion;
};

class MotionPropertyWidgetHandler
    : QObject
    , public AzToolsFramework::PropertyHandler < CReflectedVarMotion, MotionPropertyCtrl >
{
public:
    AZ_CLASS_ALLOCATOR(MotionPropertyWidgetHandler, AZ::SystemAllocator, 0);

    virtual AZ::u32 GetHandlerName(void) const override  { return AZ_CRC("Motion", 0xf5fea1e8); }
    virtual bool IsDefaultHandler() const override { return true; }
    virtual QWidget* GetFirstInTabOrder(MotionPropertyCtrl* widget) override { return widget->GetFirstInTabOrder(); }
    virtual QWidget* GetLastInTabOrder(MotionPropertyCtrl* widget) override { return widget->GetLastInTabOrder(); }
    virtual void UpdateWidgetInternalTabbing(MotionPropertyCtrl* widget) override { widget->UpdateTabOrder(); }

    virtual QWidget* CreateGUI(QWidget* pParent) override;
    virtual void ConsumeAttribute(MotionPropertyCtrl* GUI, AZ::u32 attrib, AzToolsFramework::PropertyAttributeReader* attrValue, const char* debugName) override;
    virtual void WriteGUIValuesIntoProperty(size_t index, MotionPropertyCtrl* GUI, property_t& instance, AzToolsFramework::InstanceDataNode* node) override;
    virtual bool ReadValuesIntoGUI(size_t index, MotionPropertyCtrl* GUI, const property_t& instance, AzToolsFramework::InstanceDataNode* node)  override;
};


#endif // CRYINCLUDE_EDITOR_UTILS_PROPERTYMOTIONCTRL_H
