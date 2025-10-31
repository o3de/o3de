/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
    AZ_CLASS_ALLOCATOR(MotionPropertyWidgetHandler, AZ::SystemAllocator);

    virtual AZ::u32 GetHandlerName(void) const override
    {
        return AZ_CRC_CE("Motion");
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
