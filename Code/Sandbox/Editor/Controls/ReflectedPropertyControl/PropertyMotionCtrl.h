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

#pragma once

#if !defined(Q_MOC_RUN)
#include "ReflectedVar.h"
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/base.h>
#include <AzToolsFramework/UI/PropertyEditor/PropertyAssetCtrl.hxx>
#include <AzToolsFramework/UI/PropertyEditor/PropertyEditorAPI.h>
#include <QPointer>
#include <QWidget>
#endif

class MotionPropertyWidgetHandler : QObject,
                                    public AzToolsFramework::PropertyHandler<CReflectedVarMotion, AzToolsFramework::PropertyAssetCtrl>
{
    Q_OBJECT

public:
    AZ_CLASS_ALLOCATOR(MotionPropertyWidgetHandler, AZ::SystemAllocator, 0);

    virtual AZ::u32 GetHandlerName(void) const override
    {
        return AZ_CRC("Motion", 0xf5fea1e8);
    }

    virtual bool IsDefaultHandler() const override
    {
        return true;
    }

    virtual QWidget* GetFirstInTabOrder(AzToolsFramework::PropertyAssetCtrl* widget) override
    {
        return widget->GetFirstInTabOrder();
    }

    virtual QWidget* GetLastInTabOrder(AzToolsFramework::PropertyAssetCtrl* widget) override
    {
        return widget->GetLastInTabOrder();
    }

    virtual void UpdateWidgetInternalTabbing(AzToolsFramework::PropertyAssetCtrl* widget) override
    {
        widget->UpdateTabOrder();
    }

    virtual QWidget* CreateGUI(QWidget* pParent) override;
    virtual void ConsumeAttribute(
        AzToolsFramework::PropertyAssetCtrl* GUI, AZ::u32 attrib, AzToolsFramework::PropertyAttributeReader* attrValue,
        const char* debugName) override;
    virtual void WriteGUIValuesIntoProperty(
        size_t index, AzToolsFramework::PropertyAssetCtrl* GUI, property_t& instance, AzToolsFramework::InstanceDataNode* node) override;
    virtual bool ReadValuesIntoGUI(
        size_t index, AzToolsFramework::PropertyAssetCtrl* GUI, const property_t& instance,
        AzToolsFramework::InstanceDataNode* node) override;
};
