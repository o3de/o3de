/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Slice/SliceComponent.h>

class UiEditorEntityContext;

namespace SerializeHelpers
{
    //! A struct that represents the data required to recreate one UI element and its
    //! descendant elements for undo/redo.
    //! It stores serialized saves for undo and redo. Each contains the element and its descendant elements
    //! along with any prefab references for the element or its children. It also stores where in the 
    //! element hierarchy to restore it to.
    struct SerializedEntry
    {
        AZ::EntityId m_id;
        AZ::EntityId m_parentId;
        AZ::EntityId m_insertAboveThisId;
        AZStd::string m_undoXml;
        AZStd::string m_redoXml;
        AZStd::unordered_set<AZ::Data::AssetId> m_referencedSliceAssets;
    };

    //! A list of serialized elements
    using SerializedEntryList = std::list< SerializedEntry >;

    //! A vector of EntityRestoreInfo structs
    using EntityRestoreVec = AZStd::vector<AZ::SliceComponent::EntityRestoreInfo>;

    //! Restore UI elements and their children from the given xml
    //! Slice instance info is preserved
    //! \param canvasEntityId   The entity ID of the UI canvas that contains the UI elements
    //! \param parent           The parent element that the unserialized top-level elements will be children of, if nullptr the root element is the parent
    //! \param insertBefore     The sibling element to place the top-level elements before, if nullptr then add as last child
    //! \param entityContext    The UI Editor entity context for this UI canvas
    //! \param xml              The XML string to unserialize, it contains all the elements puts slice restore info
    //! \param isCopyOperation  True if we are creating new elements rather than restoring delete elements
    //! \param cumulativeListOfCreatedEntities If this is non-null then all the entities created are added to this list
    void RestoreSerializedElements(
        AZ::EntityId canvasEntityId,
        AZ::Entity*  parent,
        AZ::Entity*  insertBefore,
        UiEditorEntityContext* entityContext,
        const AZStd::string& xml,
        bool isCopyOperation,
        LyShine::EntityArray* cumulativeListOfCreatedEntities);

    //! Save the given elements to an XML string
    //! \param elements               The top-level elements to save - all descendant elements will be saved also
    //! \param rootSlice              The root slice for the canvas
    //! \param isCopyOperation        True if this is a copy or cut operation, false if it is part of undo/redo
    //! \param referencedSliceAssets  Out param, all prefab assets used by the saved elements
    AZStd::string SaveElementsToXmlString(
        const LyShine::EntityArray& elements,
        AZ::SliceComponent* rootSlice,
        bool isCopyOperation,
        AZStd::unordered_set<AZ::Data::AssetId>& referencedSliceAssets);

    //! Load elements from an XML string that was created by SaveElementsToXmlString
    //! \param canvasEntityId   The entity ID of the UI canvas that contains the UI elements
    //! \param string           The XML string containing the elements and associated data
    //! \param makeNewIDs       If true new entity IDs and Element Ids will be created for the created elements
    //! \param insertBefore     The sibling element to place the top-level elements before, if nullptr then add as last child
    //! \param entityContext    The UI Editor entity context for this UI canvas
    //! \param listOfCreatedTopLevelElements    Out param, the top-level elements created
    //! \param listOfAllCreatedElements         Out param, all elements that were created
    //! \param entityRestoreInfos               Out param, the slice infos in the same order as listOfAllCreatedElements
    void LoadElementsFromXmlString(
        AZ::EntityId canvasEntityId,
        const AZStd::string& string,
        bool makeNewIDs,
        AZ::Entity* insertionPoint,
        AZ::Entity* insertBefore,
        LyShine::EntityArray& listOfCreatedTopLevelElements,
        LyShine::EntityArray& listOfAllCreatedElements,
        EntityRestoreVec& entityRestoreInfos);

}   // namespace EntityHelpers
