/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <Atom/RPI.Reflect/Image/StreamingImageAsset.h>

#include <AzToolsFramework/AssetBrowser/AssetPicker/AssetPickerDialog.h>
#include <AzToolsFramework/UI/PropertyEditor/PropertyAssetCtrl.hxx>
#endif

namespace GradientSignal
{
    class SupportedImageAssetPickerDialog
        : public AzToolsFramework::AssetBrowser::AssetPickerDialog
    {
        Q_OBJECT
    public:
        AZ_CLASS_ALLOCATOR(SupportedImageAssetPickerDialog, AZ::SystemAllocator);

        explicit SupportedImageAssetPickerDialog(AssetSelectionModel& selection, QWidget* parent = nullptr);

    protected:
        bool EvaluateSelection() const override;
    };

    class StreamingImagePropertyAssetCtrl
        : public AzToolsFramework::PropertyAssetCtrl
    {
        Q_OBJECT

    public:
        AZ_CLASS_ALLOCATOR(StreamingImagePropertyAssetCtrl, AZ::SystemAllocator);

        StreamingImagePropertyAssetCtrl(QWidget* parent = nullptr);

        void PickAssetSelectionFromDialog(AssetSelectionModel& selection, QWidget* parent) override;
        bool CanAcceptAsset(const AZ::Data::AssetId& assetId, const AZ::Data::AssetType& assetType) const override;

    public Q_SLOTS:
        void OnAutocomplete(const QModelIndex& index) override;
        void UpdateAssetDisplay() override;
    };

    //! We need a custom asset property handler for the AZ::RPI::StreamingImageAsset on our
    //! Image Gradient component because only a subset of the streaming image asset pixel formats
    //! are currently supported by the image data pixel retrieval API that the Image Gradient
    //! relies on
    class StreamingImagePropertyHandler
        : QObject
        , public AzToolsFramework::PropertyHandler<AZ::Data::Asset<AZ::Data::AssetData>, StreamingImagePropertyAssetCtrl>
    {
        Q_OBJECT

    public:
        AZ_CLASS_ALLOCATOR(StreamingImagePropertyHandler, AZ::SystemAllocator);

        AZ::TypeId GetHandledType() const override;
        AZ::u32 GetHandlerName() const override;
        bool IsDefaultHandler() const override;
        QWidget* GetFirstInTabOrder(StreamingImagePropertyAssetCtrl* widget) override;
        QWidget* GetLastInTabOrder(StreamingImagePropertyAssetCtrl* widget) override;
        void UpdateWidgetInternalTabbing(StreamingImagePropertyAssetCtrl* widget);

        QWidget* CreateGUI(QWidget* parent) override;
        void ConsumeAttribute(StreamingImagePropertyAssetCtrl* GUI, AZ::u32 attrib, AzToolsFramework::PropertyAttributeReader* attrValue, const char* debugName) override;
        void WriteGUIValuesIntoProperty(size_t index, StreamingImagePropertyAssetCtrl* GUI, property_t& instance, AzToolsFramework::InstanceDataNode* node) override;
        bool ReadValuesIntoGUI(size_t index, StreamingImagePropertyAssetCtrl* GUI, const property_t& instance, AzToolsFramework::InstanceDataNode* node)  override;
        AZ::Data::Asset<AZ::Data::AssetData>* CastTo(void* instance, const AzToolsFramework::InstanceDataNode* node, const AZ::Uuid& fromId, const AZ::Uuid& toId) const override;

        static void Register();
    };
}
