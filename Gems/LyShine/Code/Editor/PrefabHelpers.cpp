/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "UiCanvasEditor_precompiled.h"

#include "EditorCommon.h"
#include "AzFramework/StringFunc/StringFunc.h"
#include "Util/PathUtil.h"

#include <QMessageBox>
#include <QFileDialog>

namespace PrefabHelpers
{
    QAction* CreateSavePrefabAction(HierarchyWidget* hierarchy)
    {
        QAction* action = new QAction("(Deprecated) Save as Prefab...", hierarchy);
        QObject::connect(action,
            &QAction::triggered,
            hierarchy,
            [ hierarchy ]([[maybe_unused]] bool checked)
            {
                // Note that selectedItems() can be expensive, so call it once and save the value.
                QTreeWidgetItemRawPtrQList selectedItems(hierarchy->selectedItems());
                if (selectedItems.isEmpty())
                {
                    QMessageBox(QMessageBox::Information,
                        "Selection Needed",
                        "Please select an element in the Hierarchy pane",
                        QMessageBox::Ok, hierarchy->GetEditorWindow()).exec();

                    return;
                }
                else if (selectedItems.size() > 1)
                {
                    QMessageBox(QMessageBox::Information,
                        "Too Many Items Selected",
                        "Please select only one element in the Hierarchy pane",
                        QMessageBox::Ok, hierarchy->GetEditorWindow()).exec();

                    return;
                }

                QString selectedFile = QFileDialog::getSaveFileName(nullptr,
                        QString(),
                        FileHelpers::GetAbsoluteGameDir(),
                        "*." UICANVASEDITOR_PREFAB_EXTENSION,
                        nullptr,
                        QFileDialog::DontConfirmOverwrite);
                if (selectedFile.isEmpty())
                {
                    // Nothing to do.
                    return;
                }

                FileHelpers::AppendExtensionIfNotPresent(selectedFile, UICANVASEDITOR_PREFAB_EXTENSION);

                AZ::EntityId canvasEntityId = hierarchy->GetEditorWindow()->GetCanvas();

                // We've already checked if selectedItems is empty, so calling front() should be fine here
                HierarchyItem* hierarchyItem = HierarchyItem::RttiCast(selectedItems.front());
                AZ::Entity* element = hierarchyItem->GetElement();

                // Check if this element is OK to save as a prefab
                UiCanvasInterface::ErrorCode errorCode = UiCanvasInterface::ErrorCode::NoError;
                EBUS_EVENT_ID_RESULT(errorCode, canvasEntityId, UiCanvasBus, CheckElementValidToSaveAsPrefab,
                    element);

                if (errorCode != UiCanvasInterface::ErrorCode::NoError)
                {
                    if (errorCode == UiCanvasInterface::ErrorCode::PrefabContainsExternalEntityRefs)
                    {
                        QMessageBox box(QMessageBox::Question,
                            "External references",
                            "The selected element contains references to elements that will not be in the prefab.\n"
                            "If saved these references will be cleared in the prefab.\n\n"
                            "Do you wish to save as prefab anyway?",
                            (QMessageBox::Yes | QMessageBox::No), hierarchy->GetEditorWindow());
                        box.setDefaultButton(QMessageBox::No);

                        int result = box.exec();
                        if (result == QMessageBox::No)
                        {
                            return;
                        }
                    }
                    else
                    {
                        // this should never happen, but will if we forget to update this code when a new error is
                        // added
                        QMessageBox(QMessageBox::Information,
                            "Cannot save as prefab",
                            "Unknown error",
                            QMessageBox::Ok, hierarchy->GetEditorWindow()).exec();
                        return;
                    }
                }

                FileHelpers::SourceControlAddOrEdit(selectedFile.toStdString().c_str(), hierarchy->GetEditorWindow());

                bool saveSuccessful = false;
                EBUS_EVENT_ID_RESULT(saveSuccessful, canvasEntityId, UiCanvasBus, SaveAsPrefab,
                    selectedFile.toStdString().c_str(), element);

                // Refresh the menu to update "Add prefab...".
                if (saveSuccessful)
                {
                    QString gamePath(Path::FullPathToGamePath(selectedFile));
                    hierarchy->GetEditorWindow()->AddPrefabFile(gamePath);

                    return;
                }

                QMessageBox(QMessageBox::Critical,
                    "Error",
                    "Unable to save file. Is the file read-only?",
                    QMessageBox::Ok, hierarchy->GetEditorWindow()).exec();
            });

        return action;
    }

    void CreateAddPrefabMenu(HierarchyWidget* hierarchy,
        QTreeWidgetItemRawPtrQList& selectedItems,
        QMenu* parent,
        bool addAtRoot,
        const QPoint* optionalPos)
    {
        // Find all the prefabs in the project directory and in any enabled Gems
        IFileUtil::FileArray& files = hierarchy->GetEditorWindow()->GetPrefabFiles();
        if (files.empty())
        {
            // Since this feature is deprecated we don't show the menu unles there are prefabs
            return;
        }

        QMenu* prefabMenu = parent->addMenu(QString("(Deprecated) Element%1 from prefab").arg(!addAtRoot && selectedItems.size() > 1 ? "s" : ""));

        QList<QAction*> result;
        {
            for (auto file : files)
            {
                // Get the filepath from the engine root directory
                QString fullFileName = Path::GamePathToFullPath(file.filename);
                QString filepath(fullFileName);

                // Extract the filename without its extension, get it from fullFileName rather than
                // file.filename because the former preserves case
                AZStd::string filename;
                AzFramework::StringFunc::Path::GetFileName(fullFileName.toUtf8().data(), filename);

                QAction* action = new QAction(filename.c_str(), prefabMenu);
                QObject::connect(action,
                    &QAction::triggered,
                    hierarchy,
                    [filepath, hierarchy, addAtRoot, optionalPos]([[maybe_unused]] bool checked)
                    {
                        if (addAtRoot)
                        {
                            hierarchy->clearSelection();
                        }

                        CommandHierarchyItemCreateFromData::Push(hierarchy->GetEditorWindow()->GetActiveStack(),
                            hierarchy,
                            hierarchy->selectedItems(),
                            true,
                            [hierarchy, filepath, optionalPos](HierarchyItem* parent,
                                                               LyShine::EntityArray& listOfNewlyCreatedTopLevelElements)
                            {
                                AZ::Entity* newEntity = nullptr;
                                EBUS_EVENT_ID_RESULT(newEntity,
                                    hierarchy->GetEditorWindow()->GetCanvas(),
                                    UiCanvasBus,
                                    LoadFromPrefab,
                                    filepath.toStdString().c_str(),
                                    true,
                                    (parent ? parent->GetElement() : nullptr));
                                if (newEntity)
                                {
                                    if (optionalPos)
                                    {
                                        EntityHelpers::MoveElementToGlobalPosition(newEntity, *optionalPos);
                                    }

                                    listOfNewlyCreatedTopLevelElements.push_back(newEntity);
                                }
                            },
                            "Prefab");
                    });
                result.push_back(action);
            }
        }

        prefabMenu->addActions(result);
    }
}   // namespace PrefabHelpers
