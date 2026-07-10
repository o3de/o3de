/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "UiPrefabManager.h"

#include "Commands/CommandHierarchyItemCreateFromData.h"
#include "Helpers/EntityHelpers.h"
#include "Helpers/FileHelpers.h"
#include "Helpers/HierarchyHelpers.h"
#include "Helpers/SelectionHelpers.h"
#include "Helpers/SerializeHelpers.h"
#include "Widgets/HierarchyWidget/HierarchyClipboard.h"
#include "Widgets/HierarchyWidget/HierarchyItem.h"
#include "Widgets/HierarchyWidget/HierarchyWidget.h"
#include "Windows/EditorCommon.h"
#include "Windows/EditorWindow/EditorWindow.h"

#include <AzCore/IO/SystemFile.h>
#include <AzQtComponents/Components/Widgets/FileDialog.h>
#include <Shine/Bus/UiCanvasBus.h>
#include <Shine/Bus/UiElementBus.h>

#include <QFileDialog>
#include <QMessageBox>
#include <QString>

static const char* s_uiPrefabExtension = "uiprefab";
static const char* s_uiPrefabFileFilter = "UI Prefab Files (*.uiprefab)";
static const char* s_uiPrefabDirectory = "UI/Prefabs";

UiPrefabManager::UiPrefabManager(EditorWindow* editorWindow)
    : m_editorWindow(editorWindow)
{
}

const char* UiPrefabManager::GetPrefabFileFilter()
{
    return s_uiPrefabFileFilter;
}

const char* UiPrefabManager::GetPrefabExtension()
{
    return s_uiPrefabExtension;
}

void UiPrefabManager::InstantiateUsingBrowser(HierarchyWidget* hierarchy)
{
    // Start in the project's UI/Prefabs directory if it exists
    QString dir = FileHelpers::GetAbsoluteDir(s_uiPrefabDirectory);

    QString filePath = QFileDialog::getOpenFileName(
        m_editorWindow,
        QObject::tr("Instantiate UI Prefab"),
        dir,
        s_uiPrefabFileFilter);

    if (filePath.isEmpty())
    {
        return;
    }

    AZStd::string path(filePath.toUtf8().constData());
    InstantiatePrefab(path, hierarchy);
}

void UiPrefabManager::InstantiatePrefab(const AZStd::string& prefabPath, HierarchyWidget* hierarchy)
{
    // Read the file
    AZ::IO::SystemFile file;
    if (!file.Open(prefabPath.c_str(), AZ::IO::SystemFile::SF_OPEN_READ_ONLY))
    {
        QMessageBox::warning(
            m_editorWindow,
            QObject::tr("Error"),
            QObject::tr("Failed to open UI prefab file:\n%1").arg(prefabPath.c_str()));
        return;
    }

    AZ::IO::SizeType fileSize = file.Length();
    if (fileSize == 0)
    {
        file.Close();
        QMessageBox::warning(
            m_editorWindow,
            QObject::tr("Error"),
            QObject::tr("UI prefab file is empty:\n%1").arg(prefabPath.c_str()));
        return;
    }

    AZStd::string xml;
    xml.resize_no_construct(fileSize);
    file.Read(fileSize, xml.data());
    file.Close();

    // Use the same creation path as paste -- this gives us undo/redo support
    QTreeWidgetItemRawPtrQList selectedItems = hierarchy->selectedItems();

    CommandHierarchyItemCreateFromData::Push(
        m_editorWindow->GetActiveStack(),
        hierarchy,
        selectedItems,
        true, // createAsChildOfSelection
        [this, xml](HierarchyItem* parent, Shine::EntityArray& listOfNewlyCreatedTopLevelElements)
        {
            SerializeHelpers::RestoreSerializedElements(
                m_editorWindow->GetCanvas(),
                (parent ? parent->GetElement() : nullptr),
                nullptr,
                m_editorWindow->GetEntityContext(),
                xml,
                true, // isCopyOperation -- generates new entity IDs
                &listOfNewlyCreatedTopLevelElements);
        },
        "Instantiate UI Prefab");
}

void UiPrefabManager::CreatePrefabFromSelection(HierarchyWidget* hierarchy)
{
    QTreeWidgetItemRawPtrQList selectedItems = hierarchy->selectedItems();
    if (selectedItems.isEmpty())
    {
        QMessageBox::information(
            m_editorWindow,
            QObject::tr("Create UI Prefab"),
            QObject::tr("Please select one or more elements to save as a UI prefab."));
        return;
    }

    // Gather the top-level selected entities (excluding children of selected parents)
    HierarchyItemRawPtrList topLevelItems;
    SelectionHelpers::GetListOfTopLevelSelectedItems(hierarchy, selectedItems, hierarchy->invisibleRootItem(), topLevelItems);

    Shine::EntityArray elements;
    for (auto item : topLevelItems)
    {
        elements.push_back(item->GetElement());
    }

    // Serialize the selected elements (and their descendants) to XML
    AZStd::string xml = SerializeHelpers::SaveElementsToXmlString(elements, true);
    if (xml.empty())
    {
        QMessageBox::warning(
            m_editorWindow,
            QObject::tr("Error"),
            QObject::tr("Failed to serialize selected elements."));
        return;
    }

    // Open a save file dialog
    QString dir = FileHelpers::GetAbsoluteDir(s_uiPrefabDirectory);

    // Default filename from first selected element name
    AZStd::string elementName;
    UiElementBus::EventResult(elementName, topLevelItems.front()->GetEntityId(), &UiElementBus::Events::GetName);
    if (!elementName.empty())
    {
        dir += "/" + QString::fromUtf8(elementName.c_str());
    }

    QString filePath = AzQtComponents::FileDialog::GetSaveFileName(
        m_editorWindow,
        QObject::tr("Save as UI Prefab"),
        dir,
        s_uiPrefabFileFilter);

    if (filePath.isEmpty())
    {
        return;
    }

    // Ensure the extension is present
    if (!filePath.endsWith(QString(".") + s_uiPrefabExtension, Qt::CaseInsensitive))
    {
        filePath += QString(".") + s_uiPrefabExtension;
    }

    // Write the file
    AZStd::string path(filePath.toUtf8().constData());
    AZ::IO::SystemFile outFile;
    if (!outFile.Open(path.c_str(),
            AZ::IO::SystemFile::SF_OPEN_CREATE | AZ::IO::SystemFile::SF_OPEN_WRITE_ONLY | AZ::IO::SystemFile::SF_OPEN_CREATE_PATH))
    {
        QMessageBox::warning(
            m_editorWindow,
            QObject::tr("Error"),
            QObject::tr("Failed to create file:\n%1").arg(filePath));
        return;
    }

    outFile.Write(xml.data(), xml.size());
    outFile.Close();
}
