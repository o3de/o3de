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

#include <Core/Core.h>
#endif

namespace ScriptCanvasEditor
{
    class SourceHandlePropertyAssetCtrl
        : public AzToolsFramework::PropertyAssetCtrl
    {
        Q_OBJECT

    public:
        AZ_CLASS_ALLOCATOR(SourceHandlePropertyAssetCtrl, AZ::SystemAllocator, 0);

        SourceHandlePropertyAssetCtrl(QWidget* parent = nullptr);

        AzToolsFramework::AssetBrowser::AssetSelectionModel GetAssetSelectionModel() override;
        void PopupAssetPicker() override;
        void ClearAssetInternal() override;
        void ConfigureAutocompleter() override;

        void SetSourceAssetFilterPattern(const QString& filterPattern);

        AZ::IO::Path GetSelectedSourcePath() const;
        void SetSelectedSourcePath(const AZ::IO::Path& sourcePath);

    public Q_SLOTS:
        void OnAutocomplete(const QModelIndex& index) override;

    private:
        //! A regular expression pattern for filtering by source assets
        //! If this is set, the PropertyAssetCtrl will be dealing with source assets
        //! instead of a specific asset type
        QString m_sourceAssetFilterPattern;

        AZ::IO::Path m_selectedSourcePath;
    };

    class SourceHandlePropertyHandler
        : QObject
        , public AzToolsFramework::PropertyHandler<SourceHandle, SourceHandlePropertyAssetCtrl>
    {
        Q_OBJECT

    public:
        AZ_CLASS_ALLOCATOR(SourceHandlePropertyHandler, AZ::SystemAllocator, 0);

        AZ::u32 GetHandlerName(void) const override { return AZ_CRC_CE("SourceHandle"); }
        bool IsDefaultHandler() const override { return true; }
        QWidget* GetFirstInTabOrder(SourceHandlePropertyAssetCtrl* widget) override { return widget->GetFirstInTabOrder(); }
        QWidget* GetLastInTabOrder(SourceHandlePropertyAssetCtrl* widget) override { return widget->GetLastInTabOrder(); }
        void UpdateWidgetInternalTabbing(SourceHandlePropertyAssetCtrl* widget) override { widget->UpdateTabOrder(); }

        QWidget* CreateGUI(QWidget* pParent) override;
        void ConsumeAttribute(SourceHandlePropertyAssetCtrl* GUI, AZ::u32 attrib, AzToolsFramework::PropertyAttributeReader* attrValue, const char* debugName) override;
        void WriteGUIValuesIntoProperty(size_t index, SourceHandlePropertyAssetCtrl* GUI, property_t& instance, AzToolsFramework::InstanceDataNode* node) override;
        bool ReadValuesIntoGUI(size_t index, SourceHandlePropertyAssetCtrl* GUI, const property_t& instance, AzToolsFramework::InstanceDataNode* node)  override;
    };
}
