/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <AzToolsFramework/UI/PropertyEditor/PropertyAssetCtrl.hxx>

#include <LyShine/UiAssetTypes.h>
#endif

namespace LyShineEditor
{
    class CanvasAssetPropertyHandler
        : QObject
        , public AzToolsFramework::PropertyHandler<AzFramework::SimpleAssetReference<LyShine::CanvasAsset>, AzToolsFramework::PropertyAssetCtrl>
    {
        Q_OBJECT

    public:
        AZ_CLASS_ALLOCATOR(CanvasAssetPropertyHandler, AZ::SystemAllocator);

        AZ::u32 GetHandlerName() const override;
        bool IsDefaultHandler() const override;
        QWidget* GetFirstInTabOrder(AzToolsFramework::PropertyAssetCtrl* widget) override;
        QWidget* GetLastInTabOrder(AzToolsFramework::PropertyAssetCtrl* widget) override;
        void UpdateWidgetInternalTabbing(AzToolsFramework::PropertyAssetCtrl* widget);

        QWidget* CreateGUI(QWidget* parent) override;
        void ConsumeAttribute(AzToolsFramework::PropertyAssetCtrl* GUI, AZ::u32 attrib, AzToolsFramework::PropertyAttributeReader* attrValue, const char* debugName) override;
        void WriteGUIValuesIntoProperty(size_t index, AzToolsFramework::PropertyAssetCtrl* GUI, property_t& instance, AzToolsFramework::InstanceDataNode* node) override;
        bool ReadValuesIntoGUI(size_t index, AzToolsFramework::PropertyAssetCtrl* GUI, const property_t& instance, AzToolsFramework::InstanceDataNode* node)  override;

        static void Register();
    };
}
