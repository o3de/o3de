/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "EditorCommon.h"

#include <QMimeData>
#include <QApplication>
#include <QClipboard>

SerializeHelpers::SerializedEntryList& HierarchyClipboard::Serialize(HierarchyWidget* widget,
    const QTreeWidgetItemRawPtrQList& selectedItems,
    HierarchyItemRawPtrList* optionalItemsToSerialize,
    SerializeHelpers::SerializedEntryList& entryList,
    bool isUndo)
{
    HierarchyItemRawPtrList itemsToSerialize;
    if (optionalItemsToSerialize)
    {
        // copy the list so we can sort it
        itemsToSerialize = *optionalItemsToSerialize;
    }
    else
    {
        SelectionHelpers::GetListOfTopLevelSelectedItems(widget,
            selectedItems,
            widget->invisibleRootItem(),
            itemsToSerialize);
    }

    // Sort the itemsToSerialize by order in the hierarchy, this is important for reliably restoring
    // them given that we maintain the order by remembering which item to insert before
    HierarchyHelpers::SortByHierarchyOrder(itemsToSerialize);

    // HierarchyItemRawPtrList -> SerializeHelpers::SerializedEntryList
    for (auto i : itemsToSerialize)
    {
        AZ::Entity* e = i->GetElement();
        AZ_Assert(e, "No entity found for item");

        // serialize this entity (and its descendants) to XML and get the set of prefab references
        // used by the serialized entities
        AZStd::unordered_set<AZ::Data::AssetId> referencedSliceAssets;
        AZStd::string xml = GetXml(widget, LyShine::EntityArray(1, e), false, referencedSliceAssets);
        AZ_Assert(!xml.empty(), "Failed to serialize");

        if (isUndo)
        {
            AZ::EntityId parentId;
            {
                HierarchyItem* parent = i->Parent();
                parentId = (parent ? parent->GetEntityId() : AZ::EntityId());
            }

            AZ::EntityId insertAboveThisId;
            {
                QTreeWidgetItem* parentItem = (i->QTreeWidgetItem::parent() ? i->QTreeWidgetItem::parent() : widget->invisibleRootItem());

                // +1 : Because the insertion point is the next sibling.
                QTreeWidgetItem* insertAboveThisBaseItem = parentItem->child(parentItem->indexOfChild(i) + 1);
                HierarchyItem* insertAboveThisItem = HierarchyItem::RttiCast(insertAboveThisBaseItem);

                insertAboveThisId = (insertAboveThisItem ? insertAboveThisItem->GetEntityId() : AZ::EntityId());
            }

            entryList.push_back(SerializeHelpers::SerializedEntry
                {
                    i->GetEntityId(),
                    parentId,
                    insertAboveThisId,
                    xml,
                    "",
                    referencedSliceAssets
                }
            );
        }
        else // isRedo.
        {
            AZ::EntityId id = i->GetEntityId();

            auto iter = std::find_if(entryList.begin(),
                    entryList.end(),
                    [ id ](const SerializeHelpers::SerializedEntry& entry)
                    {
                        return (entry.m_id == id);
                    });

            // This function should ALWAYS be called
            // with ( isUndo == true ) first.
            AZ_Assert(entryList.size() > 0, "Empty entry list");
            AZ_Assert(iter != entryList.end(), "Entity ID not found in entryList");

            iter->m_redoXml = xml;
        }
    }

    return entryList;
}

bool HierarchyClipboard::Unserialize(HierarchyWidget* widget,
    SerializeHelpers::SerializedEntryList& entryList,
    bool isUndo)
{
    if (!HierarchyHelpers::AllItemExists(widget, entryList))
    {
        // At least one item is missing.
        // Nothing to do.
        return false;
    }

    // Runtime-side: Replace element.
    for (auto && e : entryList)
    {
        HierarchyItem* item = HierarchyItem::RttiCast(HierarchyHelpers::ElementToItem(widget, e.m_id, false));
        item->ReplaceElement(isUndo ? e.m_undoXml : e.m_redoXml, e.m_referencedSliceAssets);
    }

    // Editor-side: Highlight.
    widget->clearSelection();
    HierarchyHelpers::SetSelectedItems(widget, &entryList);

    return true;
}

void HierarchyClipboard::CopySelectedItemsToClipboard(HierarchyWidget* widget,
    const QTreeWidgetItemRawPtrQList& selectedItems)
{
    // selectedItems -> EntityArray.
    LyShine::EntityArray elements;
    {
        HierarchyItemRawPtrList itemsToSerialize;
        SelectionHelpers::GetListOfTopLevelSelectedItems(widget,
            selectedItems,
            widget->invisibleRootItem(),
            itemsToSerialize);

        for (auto i : itemsToSerialize)
        {
            elements.push_back(i->GetElement());
        }
    }

    // EntityArray -> XML.
    AZStd::unordered_set<AZ::Data::AssetId> referencedSliceAssets;    // returned from GetXML but not used in this case
    AZStd::string xml = GetXml(widget, elements, true, referencedSliceAssets);

    // XML -> Clipboard.
    if (!xml.empty())
    {
        QMimeData* mimeData = new QMimeData();
        {
            // Concatenate all the data we need into a single QByteArray.
            QByteArray data(xml.c_str(), static_cast<int>(xml.size()));
            mimeData->setData(UICANVASEDITOR_MIMETYPE, data);
        }

        QApplication::clipboard()->setMimeData(mimeData);
    }
}

void HierarchyClipboard::CreateElementsFromClipboard(HierarchyWidget* widget,
    const QTreeWidgetItemRawPtrQList& selectedItems,
    bool createAsChildOfSelection)
{
    if (!ClipboardContainsOurDataType())
    {
        // Nothing to do.
        return;
    }

    const QMimeData* mimeData = QApplication::clipboard()->mimeData();

    QByteArray data = mimeData->data(UICANVASEDITOR_MIMETYPE);
    char* rawData = data.data();

    // Extract all the data we need from the source QByteArray.
    char* xml = static_cast<char*>(rawData);

    CommandHierarchyItemCreateFromData::Push(widget->GetEditorWindow()->GetActiveStack(),
        widget,
        selectedItems,
        createAsChildOfSelection,
        [ widget, xml ](HierarchyItem* parent,
                                                  LyShine::EntityArray& listOfNewlyCreatedTopLevelElements)
        {
            SerializeHelpers::RestoreSerializedElements(widget->GetEditorWindow()->GetCanvas(),
                (parent ? parent->GetElement() : nullptr),
                nullptr,
                widget->GetEditorWindow()->GetEntityContext(),
                xml,
                true,
                &listOfNewlyCreatedTopLevelElements);
        },
        "Paste");
}

AZStd::string HierarchyClipboard::GetXml(HierarchyWidget* widget,
    const LyShine::EntityArray& elements,
    bool isCopyOperation,
    AZStd::unordered_set<AZ::Data::AssetId>& referencedSliceAssets)
{
    if (elements.empty())
    {
        // Nothing to do.
        return "";
    }

    AZ::SliceComponent* rootSlice = widget->GetEditorWindow()->GetSliceManager()->GetRootSlice();
    AZStd::string result = SerializeHelpers::SaveElementsToXmlString(elements, rootSlice, isCopyOperation, referencedSliceAssets);

    return result;
}

AZStd::string HierarchyClipboard::GetXmlForDiff(AZ::EntityId canvasEntityId)
{
    AZStd::string xmlString;
    UiCanvasBus::EventResult(xmlString, canvasEntityId, &UiCanvasBus::Events::SaveToXmlString);
    return xmlString;
}

void HierarchyClipboard::BeginUndoableEntitiesChange(EditorWindow* editorWindow, SerializeHelpers::SerializedEntryList& preChangeState)
{
    preChangeState.clear(); // Serialize just adds to the list, so we want to clear it first

    // This is used to save the "before" undo data.
    HierarchyClipboard::Serialize(editorWindow->GetHierarchy(),
        editorWindow->GetHierarchy()->selectedItems(),
        nullptr,
        preChangeState,
        true);
}

void HierarchyClipboard::EndUndoableEntitiesChange(EditorWindow* editorWindow, const char* commandName, SerializeHelpers::SerializedEntryList& preChangeState)
{
    // Before saving the current entity state, make sure that all marked layouts are recomputed.
    // Otherwise they will be recomputed on the next update which will be after the entity state is saved.
    // An example where this is needed is when changing the properties of a layout fitter component
    UiCanvasBus::Event(editorWindow->GetActiveCanvasEntityId(), &UiCanvasBus::Events::RecomputeChangedLayouts);

    // This is used to save the "after" undo data. It puts a command on the undo stack with the given name.
    CommandPropertiesChange::Push(editorWindow->GetActiveStack(),
        editorWindow->GetHierarchy(),
        preChangeState,
        commandName);

    // Notify other systems (e.g. Animation) for each UI entity that changed
    LyShine::EntityArray selectedElements = SelectionHelpers::GetTopLevelSelectedElements(
            editorWindow->GetHierarchy(), editorWindow->GetHierarchy()->selectedItems());
    for (auto element : selectedElements)
    {
        UiElementChangeNotificationBus::Event(element->GetId(), &UiElementChangeNotificationBus::Events::UiElementPropertyChanged);
    }
}


