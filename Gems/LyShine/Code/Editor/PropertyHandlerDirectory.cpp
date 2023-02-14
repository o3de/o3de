/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "EditorCommon.h"

#include "PropertyHandlerDirectory.h"

#include <AzToolsFramework/UI/PropertyEditor/PropertyQTConstants.h>
#include <LyShine/Bus/UiEditorChangeNotificationBus.h>
#include <Editor/Util/PathUtil.h>

#include <QBoxLayout>

PropertyDirectoryCtrl::PropertyDirectoryCtrl(QWidget* parent)
    : QWidget(parent)
    , m_propertyAssetCtrl(aznew PropertyAssetDirectorySelectionCtrl(this))
{
    QObject::connect(m_propertyAssetCtrl,
        &AzToolsFramework::PropertyAssetCtrl::OnAssetIDChanged,
        [ this ]([[maybe_unused]] AZ::Data::AssetId newAssetID)
        {
            AzToolsFramework::PropertyEditorGUIMessages::Bus::Broadcast(
                &AzToolsFramework::PropertyEditorGUIMessages::Bus::Events::RequestWrite, this);
        });

    setAcceptDrops(true);

    QHBoxLayout* layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    layout->addWidget(m_propertyAssetCtrl);

    // Add directory refresh button
    {
        QPushButton* refreshButton = new QPushButton(this);

        refreshButton->setFlat(true);

        QSize fixedSize = QSize(AzToolsFramework::PropertyQTConstant_DefaultHeight, AzToolsFramework::PropertyQTConstant_DefaultHeight);
        refreshButton->setFixedSize(fixedSize);

        refreshButton->setFocusPolicy(Qt::StrongFocus);

        refreshButton->setIcon(QIcon(":/PropertyEditor/Resources/reset_icon.png"));

        // The icon size needs to be smaller than the fixed size to make sure it visually aligns properly.
        QSize iconSize = QSize(fixedSize.width() - 2, fixedSize.height() - 2);
        refreshButton->setIconSize(iconSize);

        QObject::connect(refreshButton,
            &QPushButton::clicked,
            []([[maybe_unused]] bool checked)
        {
            UiEditorRefreshDirectoryNotificationBus::Broadcast(&UiEditorRefreshDirectoryNotificationInterface::OnRefreshDirectory);
        });

        layout->addWidget(refreshButton);
    }
}

void PropertyDirectoryCtrl::dragEnterEvent(QDragEnterEvent* ev)
{
    m_propertyAssetCtrl->dragEnterEvent(ev);
}

void PropertyDirectoryCtrl::dragLeaveEvent(QDragLeaveEvent* ev)
{
    m_propertyAssetCtrl->dragLeaveEvent(ev);
}

void PropertyDirectoryCtrl::dropEvent(QDropEvent* ev)
{
    m_propertyAssetCtrl->dropEvent(ev);
}

AzToolsFramework::PropertyAssetCtrl* PropertyDirectoryCtrl::GetPropertyAssetCtrl()
{
    return m_propertyAssetCtrl;
}

AzToolsFramework::AssetBrowser::AssetSelectionModel PropertyAssetDirectorySelectionCtrl::GetAssetSelectionModel()
{
    AzToolsFramework::AssetBrowser::AssetSelectionModel selectionModel = AzToolsFramework::AssetBrowser::AssetSelectionModel::EverythingSelection();
    EntryTypeFilter* foldersFilter = new EntryTypeFilter();
    foldersFilter->SetEntryType(AssetBrowserEntry::AssetEntryType::Folder);
    selectionModel.SetSelectionFilter(FilterConstType(foldersFilter));
    return selectionModel;
}

void PropertyAssetDirectorySelectionCtrl::SetFolderSelection(const AZStd::string& folderPath)
{
    AZStd::string strFolderPath = folderPath.c_str();
    if (strFolderPath.empty())
    {
        m_folderPath.clear();
    }    
    // AssetBrowser will return relative paths for subdirectories of the game project,
    // with the game project folder included in the path.
    else if (AzFramework::StringFunc::Path::IsRelative(strFolderPath.c_str()))
    {
        // This assumes the asset picker will always a path relative to
        // the project folder, which we need to omit since file IO routines
        // seem to assume this anyways.
        strFolderPath = strFolderPath.substr(strFolderPath.find('/') + 1);
        m_folderPath = PathUtil::MakeGamePath(strFolderPath);
        AZStd::to_lower(m_folderPath.begin(), m_folderPath.end());
    }
    // For paths in gems, absolute paths are returned
    else
    {
        m_folderPath = Path::FullPathToGamePath(strFolderPath.c_str());
        AZStd::to_lower(m_folderPath.begin(), m_folderPath.end());
    }
}

void PropertyAssetDirectorySelectionCtrl::ClearAssetInternal()
{
    SetFolderSelection(AZStd::string());
    PropertyAssetCtrl::ClearAssetInternal();
}

//-------------------------------------------------------------------------------

QWidget* PropertyHandlerDirectory::CreateGUI(QWidget* pParent)
{
    return aznew PropertyDirectoryCtrl(pParent);
}

void PropertyHandlerDirectory::ConsumeAttribute(PropertyDirectoryCtrl* GUI, AZ::u32 attrib, AzToolsFramework::PropertyAttributeReader* attrValue, const char* debugName)
{
    (void)GUI;
    (void)attrib;
    (void)attrValue;
    (void)debugName;
}

void PropertyHandlerDirectory::WriteGUIValuesIntoProperty(size_t index, PropertyDirectoryCtrl* GUI, property_t& instance, AzToolsFramework::InstanceDataNode* node)
{
    (void)index;
    (void)node;

    PropertyAssetDirectorySelectionCtrl* ctrl = static_cast<PropertyAssetDirectorySelectionCtrl*>(GUI->GetPropertyAssetCtrl());
    instance = ctrl->GetFolderSelection();
}

bool PropertyHandlerDirectory::ReadValuesIntoGUI(size_t index, PropertyDirectoryCtrl* GUI, const property_t& instance, AzToolsFramework::InstanceDataNode* node)
{
    (void)index;
    (void)node;

    PropertyAssetDirectorySelectionCtrl* ctrl = static_cast<PropertyAssetDirectorySelectionCtrl*>(GUI->GetPropertyAssetCtrl());

    ctrl->blockSignals(true);
    {
        // Set currently selected folder path
        // Note: this must be done before setting asset type below which updates the GUI display
        ctrl->SetCurrentAssetHint(instance);
        ctrl->SetFolderSelection(instance);

        // We need to set the asset type so the property panel labels get
        // populated properly (via SetCurrentAssetType). To avoid defining
        // directories as assets, we just use a throw-away GUID to get the
        // logic to run (otherwise it will early-out due to invalid asset type).
        const char* throwAwayAssetType = "{43EDD212-F589-43C8-BC02-A8F9243271CB}";
        ctrl->SetCurrentAssetType(AZ::Data::AssetType(throwAwayAssetType));
    }
    ctrl->blockSignals(false);

    return false;
}

void PropertyHandlerDirectory::Register()
{
    AzToolsFramework::PropertyTypeRegistrationMessages::Bus::Broadcast(
        &AzToolsFramework::PropertyTypeRegistrationMessages::Bus::Events::RegisterPropertyType, aznew PropertyHandlerDirectory());
}

#include <moc_PropertyHandlerDirectory.cpp>
