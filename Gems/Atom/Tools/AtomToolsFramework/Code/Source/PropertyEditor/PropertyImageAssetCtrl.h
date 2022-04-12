/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <Atom/RPI.Reflect/Image/ImageAsset.h>

#include <AzToolsFramework/AssetBrowser/AssetPicker/AssetPickerDialog.h>
#include <AzToolsFramework/UI/PropertyEditor/PropertyAssetCtrl.hxx>
#endif

namespace AtomToolsFramework
{
    class PropertyImageAssetCtrl
        : public AzToolsFramework::PropertyAssetCtrl
    {
    Q_OBJECT

    public:
        AZ_CLASS_ALLOCATOR(PropertyImageAssetCtrl, AZ::SystemAllocator, 0);

        PropertyImageAssetCtrl(QWidget* parent = nullptr);

    public Q_SLOTS:

    protected:
        // PropertyAssetCtrl overrides
        AssetSelectionModel GetAssetSelectionModel() override;
        void ConfigureAutocompleter() override;
        bool CanAcceptAsset(const AZ::Data::AssetId& assetId, const AZ::Data::AssetType& assetType) const override;
    };

    class ImageAssetPropertyHandler
        : QObject
        , public AzToolsFramework::PropertyHandler<AZ::Data::Asset<AZ::Data::AssetData>, PropertyImageAssetCtrl>
    {
        // this is a Qt Object purely so it can connect to slots with context.  This is the only reason its in this header.
        Q_OBJECT

    public:
        AZ_CLASS_ALLOCATOR(ImageAssetPropertyHandler, AZ::SystemAllocator, 0);

        AZ::u32 GetHandlerName(void) const override { return AZ_CRC_CE("ImageAsset"); }
        bool IsDefaultHandler() const override { return false; }
        QWidget* GetFirstInTabOrder(PropertyImageAssetCtrl* widget) override { return widget->GetFirstInTabOrder(); }
        QWidget* GetLastInTabOrder(PropertyImageAssetCtrl* widget) override { return widget->GetLastInTabOrder(); }
        void UpdateWidgetInternalTabbing(PropertyImageAssetCtrl* widget) override { widget->UpdateTabOrder(); }

        QWidget* CreateGUI(QWidget* pParent) override;
        void ConsumeAttribute(PropertyImageAssetCtrl* GUI, AZ::u32 attrib, AzToolsFramework::PropertyAttributeReader* attrValue, const char* debugName) override;
        void WriteGUIValuesIntoProperty(size_t index, PropertyImageAssetCtrl* GUI, property_t& instance, AzToolsFramework::InstanceDataNode* node) override;
        bool ReadValuesIntoGUI(size_t index, PropertyImageAssetCtrl* GUI, const property_t& instance, AzToolsFramework::InstanceDataNode* node) override;

        static void Register();
    };
} // namespace AtomToolsFramework
