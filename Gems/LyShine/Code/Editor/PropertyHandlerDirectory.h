/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#if !defined(Q_MOC_RUN)
#include <AzCore/base.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <QtWidgets/QWidget>
#include <AzToolsFramework/UI/PropertyEditor/PropertyEditorAPI.h>

#include <AzToolsFramework/UI/PropertyEditor/PropertyAssetCtrl.hxx>
#include <AzToolsFramework/AssetBrowser/AssetSelectionModel.h>
#endif

class PropertyDirectoryCtrl
    : public QWidget
{
    Q_OBJECT

public:

    AZ_CLASS_ALLOCATOR(PropertyDirectoryCtrl, AZ::SystemAllocator);

    PropertyDirectoryCtrl(QWidget* parent = nullptr);

    void dragEnterEvent(QDragEnterEvent* ev) override;
    void dragLeaveEvent(QDragLeaveEvent* ev) override;
    void dropEvent(QDropEvent* ev) override;

    AzToolsFramework::PropertyAssetCtrl* GetPropertyAssetCtrl();

private:

    AzToolsFramework::PropertyAssetCtrl* m_propertyAssetCtrl;
};

//-------------------------------------------------------------------------------

class PropertyAssetDirectorySelectionCtrl
    : public AzToolsFramework::PropertyAssetCtrl
{
public:
    AZ_CLASS_ALLOCATOR(PropertyAssetDirectorySelectionCtrl, AZ::SystemAllocator)
    PropertyAssetDirectorySelectionCtrl(QWidget *pParent = NULL)
        : PropertyAssetCtrl(pParent) {}
    AzToolsFramework::AssetBrowser::AssetSelectionModel GetAssetSelectionModel() override;
    void SetFolderSelection(const AZStd::string& folderPath) override;
    const AZStd::string GetFolderSelection() const override { return m_folderPath; }
    void ClearAssetInternal() override;

private:
    AZStd::string m_folderPath;
};

//-------------------------------------------------------------------------------

class PropertyHandlerDirectory
    : public AzToolsFramework::PropertyHandler<AZStd::string, PropertyDirectoryCtrl>
{
public:
    AZ_CLASS_ALLOCATOR(PropertyHandlerDirectory, AZ::SystemAllocator);
    
    AZ::u32 GetHandlerName(void) const override  { return AZ_CRC_CE("Directory"); }

    QWidget* CreateGUI(QWidget* pParent) override;
    void ConsumeAttribute(PropertyDirectoryCtrl* GUI, AZ::u32 attrib, AzToolsFramework::PropertyAttributeReader* attrValue, const char* debugName) override;
    void WriteGUIValuesIntoProperty(size_t index, PropertyDirectoryCtrl* GUI, property_t& instance, AzToolsFramework::InstanceDataNode* node) override;
    bool ReadValuesIntoGUI(size_t index, PropertyDirectoryCtrl* GUI, const property_t& instance, AzToolsFramework::InstanceDataNode* node)  override;

    static void Register();
};
