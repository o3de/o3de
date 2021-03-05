/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
#pragma once

class HierarchyClipboard
{
public:

    //-------------------------------------------------------------------------------

    //! The return value is the same as the parameter entryList.
    static SerializeHelpers::SerializedEntryList& Serialize(HierarchyWidget* widget,
        const QTreeWidgetItemRawPtrQList& selectedItems,
        HierarchyItemRawPtrList* optionalItemsToSerialize,
        SerializeHelpers::SerializedEntryList& entryList,
        bool isUndo);

    static bool Unserialize(HierarchyWidget* widget,
        SerializeHelpers::SerializedEntryList& entryList,
        bool isUndo);

    //-------------------------------------------------------------------------------

    static void CopySelectedItemsToClipboard(HierarchyWidget* widget,
        const QTreeWidgetItemRawPtrQList& selectedItems);
    static void CreateElementsFromClipboard(HierarchyWidget* widget,
        const QTreeWidgetItemRawPtrQList& selectedItems,
        bool createAsChildOfSelection);

    //-------------------------------------------------------------------------------

    //! Get the XML from the given elements (and their descendants)
    //! \param widget the HiearchyWidget in the Ui Editor editing the canvas
    //! \param elements the elements to serialize
    //! \param isCopyOperation True if this is a copy or cut operation, false if it is part of undo/redo
    //! \param referencedSliceAssets an out parameter listing the prefab assets used by the serialized elements
    //! \return a string containing XML or empty
    static AZStd::string GetXml(HierarchyWidget* widget,
        const LyShine::EntityArray& elements,
        bool isCopyOperation,
        AZStd::unordered_set<AZ::Data::AssetId>& referencedSliceAssets);

    //! Get the XML from the given canvas.
    //! The output SHOULDN'T be serialized to disk.
    //! The output should ONLY be used to determine if any changes
    //! have occurred between a "before" and an "after" states.
    //! \return a string containing XML or empty
    static AZStd::string GetXmlForDiff(AZ::EntityId canvasEntityId);

    //-------------------------------------------------------------------------------

    //! Record the state of all selected entities before a change
    static void BeginUndoableEntitiesChange(EditorWindow* editorWindow, SerializeHelpers::SerializedEntryList& preChangeState);

    //! Record an undo command of the changes to the selected entities
    static void EndUndoableEntitiesChange(EditorWindow* editorWindow, const char* commandName, SerializeHelpers::SerializedEntryList& preChangeState);
};
