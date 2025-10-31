/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <LyShine/Bus/UiDynamicScrollBoxBus.h>
#include <LyShine/Bus/UiScrollBoxBus.h>
#include <LyShine/Bus/UiInitializationBus.h>
#include <LyShine/Bus/UiTransformBus.h>
#include <LyShine/Bus/UiElementBus.h>
#include <LyShine/UiComponentTypes.h>

#include <AzCore/Component/Component.h>
#include <AzCore/Math/Vector2.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
//! This component dynamically sets up scrollbox content as a horizontal or vertical list of
//! elements that are cloned from prototype entities. Only the minimum number of entities are
//! created for efficient scrolling, and are reused when new elements come into view.
//! The list can consist of only items, or it can be divided into sections that include a header at
//! the beginning of each section, followed by items that belong to that section.
//! The meaning of "element" differs in the public and private interface, mainly for backward
//! compatibility. In the private interface, "element" refers to a generic entry in the list which
//! can be of different types. Currently there are two types of elements: headers and items.
//! In the public interface however, "element" means the same thing as "item" does in the private
//! interface, and "item" is unused (the public interface does not need to define the concept of a
//! generic entry in the list.)
//! Both headers and items can have fixed sizes determined by their corresponding prototype entities,
//! or they could vary in size. If they vary in size, another option is available to indicate whether
//! to auto calculate the sizes or request the sizes via a bus interface. There is also the option to
//! provide an estimated size that will be used until the elements scroll into view and their real
//! size calculated.
//! For lists with a large number of elements, it is advisable to use the estimated size as
//! calculating the sizes of all elements up front could be costly. When elements vary in size,
//! a cache is maintained and the element sizes are only calculated once.
class UiDynamicScrollBoxComponent
    : public AZ::Component
    , public UiDynamicScrollBoxBus::Handler
    , public UiScrollBoxNotificationBus::Handler
    , public UiInitializationBus::Handler
    , public UiTransformChangeNotificationBus::Handler
    , public UiElementNotificationBus::Handler
{
public: // member functions

    AZ_COMPONENT(UiDynamicScrollBoxComponent, LyShine::UiDynamicScrollBoxComponentUuid, AZ::Component);

    UiDynamicScrollBoxComponent();
    ~UiDynamicScrollBoxComponent() override;

    // UiDynamicScrollBoxInterface
    void RefreshContent() override;
    void AddElementsToEnd(int numElementsToAdd, bool scrollToEndIfWasAtEnd) override;
    void RemoveElementsFromFront(int numElementsToRemove) override;
    void ScrollToEnd() override;
    int GetElementIndexOfChild(AZ::EntityId childElement) override;
    int GetSectionIndexOfChild(AZ::EntityId childElement) override;
    AZ::EntityId GetChildAtElementIndex(int index) override;
    AZ::EntityId GetChildAtSectionAndElementIndex(int sectionIndex, int index) override;
    bool GetAutoRefreshOnPostActivate() override;
    void SetAutoRefreshOnPostActivate(bool autoRefresh) override;
    AZ::EntityId GetPrototypeElement() override;
    void SetPrototypeElement(AZ::EntityId prototypeElement) override;
    bool GetElementsVaryInSize() override;
    void SetElementsVaryInSize(bool varyInSize) override;
    bool GetAutoCalculateVariableElementSize() override;
    void SetAutoCalculateVariableElementSize(bool autoCalculateSize) override;
    float GetEstimatedVariableElementSize() override;
    void SetEstimatedVariableElementSize(float estimatedSize) override;
    bool GetSectionsEnabled() override;
    void SetSectionsEnabled(bool enabled) override;
    AZ::EntityId GetPrototypeHeader() override;
    void SetPrototypeHeader(AZ::EntityId prototypeHeader) override;
    bool GetHeadersSticky() override;
    void SetHeadersSticky(bool stickyHeaders) override;
    bool GetHeadersVaryInSize() override;
    void SetHeadersVaryInSize(bool varyInSize) override;
    bool GetAutoCalculateVariableHeaderSize() override;
    void SetAutoCalculateVariableHeaderSize(bool autoCalculateSize) override;
    float GetEstimatedVariableHeaderSize() override;
    void SetEstimatedVariableHeaderSize(float estimatedSize) override;
    // ~UiDynamicScrollBoxInterface

    // UiScrollBoxNotifications
    void OnScrollOffsetChanging(AZ::Vector2 newScrollOffset) override;
    void OnScrollOffsetChanged(AZ::Vector2 newScrollOffset) override;
    // ~UiScrollBoxNotifications

    // UiInitializationInterface
    void InGamePostActivate() override;
    // ~UiInitializationInterface

    // UiTransformChangeNotification
    void OnCanvasSpaceRectChanged(AZ::EntityId entityId, const UiTransformInterface::Rect& oldRect, const UiTransformInterface::Rect& newRect) override;
    // ~UiTransformChangeNotification

    // UiElementNotifications
    void OnUiElementBeingDestroyed() override;
    // ~UiElementNotifications

public:  // static member functions

    static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("UiDynamicScrollBoxService"));
        provided.push_back(AZ_CRC_CE("UiDynamicContentService"));
    }

    static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("UiDynamicContentService"));
    }

    static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC_CE("UiElementService"));
        required.push_back(AZ_CRC_CE("UiTransformService"));
        required.push_back(AZ_CRC_CE("UiScrollBoxService"));
    }

    static void Reflect(AZ::ReflectContext* context);

protected: //types

    // The types of elements that the list may contain
    enum ElementType
    {
        SectionHeader,  // element that appears at the start of each section
        Item,           // all other elements

        NumElementTypes
    };

    // The list can be divided into sections that have a header followed by a list of items
    struct Section
    {
        int m_index;                // the section index
        int m_numItems;             // the number of items in this section
        int m_headerElementIndex;   // the element index of the section header
    };

    // Used when the list is divided into sections
    struct ElementIndexInfo
    {
        int m_sectionIndex;         // the section index that the element belongs to. -1 if the list is not divided into sections
        int m_itemIndexInSection;   // the index of the item relative to the section. -1 if the element is a section header
    };

    // An element that is currently being displayed
    struct DisplayedElement
    {
        AZ::EntityId m_element;
        int m_elementIndex;             // the absolute index of the element in the list
        ElementIndexInfo m_indexInfo;   // the index info for the element including section index and item index relative to the section
        ElementType m_type;
    };

    // Used when elements vary in size
    struct CachedElementInfo
    {
        CachedElementInfo();

        float m_size;
        float m_accumulatedSize;
    };

protected: // member functions

    // AZ::Component
    void Activate() override;
    void Deactivate() override;
    // ~AZ::Component

    AZ_DISABLE_COPY_MOVE(UiDynamicScrollBoxComponent);

    // Initialization
    void PrepareListForDisplay();

    // Get the content entity of the scrollbox
    AZ::Entity* GetContentEntity() const;

    // Clone a prototype element. The parent defaults to the content entity
    AZ::EntityId ClonePrototypeElement(ElementType elementType, AZ::EntityId parentEntityId = AZ::EntityId()) const;

    // Check if the specified entity is a prototype element
    bool IsPrototypeElement(AZ::EntityId entityId) const;

    // Check if all needed prototype elements are valid
    bool AllPrototypeElementsValid() const;

    // Return whether any of the prototype elements are navigable
    bool AnyPrototypeElementsNavigable() const;

    // Return whether any element types vary in size
    bool AnyElementTypesHaveVariableSize() const;

    // Return whether any element types have an estimated size that's greater than 0
    bool AnyElementTypesHaveEstimatedSizes() const;

    // Return whether all element types have an estimated size that's greater than 0
    bool AllElementTypesHaveEstimatedSizes() const;

    // Return whether headers should stick to the top or left of the visible area
    bool StickyHeadersEnabled() const;

    // Resize the content entity to fit all the elements
    void ResizeContentToFitElements();

    // Resize the content entity to the specified height or width
    void ResizeContentElement(float newSize) const;

    // Adjust the size of the content entity and the scroll offset by the specified amount.
    // This is used after adding, removing or resizing elements
    void AdjustContentSizeAndScrollOffsetByDelta(float sizeDelta, float scrollDelta) const;

    // Calculate and cache the size of the element at the specified index
    float CalculateVariableElementSize(int index);

    // Get the size of the element at the specified index.
    // If the size is in the cache, return that size.
    // Otherwise, if there is an estimated size, return the estimated size.
    // Otherwise, calculate and cache the size
    float GetAndCacheVariableElementSize(int index);

    // Get the current size of the element at the specified index.
    // The returned size matches the size used to determine the content size.
    // Should only be called when the size is already cached or the element has an estimated size
    float GetVariableElementSize(int index) const;

    // Get the last element index, before the specified element index, with a known accumulated size
    int GetLastKnownAccumulatedSizeIndex(int index, int numElementsWithUnknownSizeOut[ElementType::NumElementTypes]) const;

    // Get the offset of the element at the specified index
    float GetElementOffsetAtIndex(int index) const;

    // Get the offset of the element at the specified index when all element types have a fixed size
    float GetFixedSizeElementOffset(int index) const;

    // Get the offset of the element at the specified index when some element types have variable size
    float GetVariableSizeElementOffset(int index) const;

    // Calculate the average element size
    void UpdateAverageElementSize(int numAddedElements, float sizeDelta);

    // Mark all elements as not displayed
    void ClearDisplayedElements();

    // Find the displayed element with a matching index
    AZ::EntityId FindDisplayedElementWithIndex(int index) const;

    // Get the size of the content's parent (visible area)
    float GetVisibleAreaSize() const;

    // Check if any elements are visible and set the visible content bounds if they are
    bool AreAnyElementsVisible(AZ::Vector2& visibleContentBoundsOut) const;

    // Update which elements are visible, and set up for display
    void UpdateElementVisibility(bool keepAtEndIfWasAtEnd = false);

    // Calculate the first and last visible element indices. Elements that have come into view will
    // get their real size calculated if only their estimated size was known.
    // Returns the total change in element size, and the scroll delta needed to keep the previously
    // visible elements at the same position after the content size is changed to accomodate the new
    // element sizes
    void CalculateVisibleElementIndices(bool keepAtEndIfWasAtEnd,
        const AZ::Vector2& visibleContentBounds,
        int& firstVisibleElementIndexOut,
        int& lastVisibleElementIndexOut,
        int& firstDisplayedElementIndexOut,
        int& lastDisplayedElementIndexOut,
        int& firstDisplayedElementIndexWithSizeChangeOut,
        float& totalElementSizeChangeOut,
        float& scrollChangeOut);

    // Update the header that is currently sticky
    void UpdateStickyHeader(int firstVisibleElementIndex, int lastVisibleElementIndex, float visibleContentBeginning);

    // Find the first visible header element index excluding the specified header element index
    int FindFirstVisibleHeaderIndex(int firstVisibleElementIndex, int lastVisibleElementIndex, int excludeIndex);

    // Find the first and last visible element indices when all element types have a fixed size
    void FindVisibleElementIndicesForFixedSizes(const AZ::Vector2& visibleContentBounds,
        int& firstVisibleElementIndexOut,
        int& lastVisibleElementIndexOut) const;

    // Find the visible element index that should remain in the same position after calculating
    // visible element indices for lists that may not know the sizes of all their elements
    int FindVisibleElementIndexToRemainInPlace(const AZ::Vector2& visibleContentBounds) const;

    // Add extra elements to the beginning and end for keyboard/gamepad navigation
    void AddExtraElementsForNavigation(int& firstDisplayedElementIndexOut, int& lastDisplayedElementIndexOut) const;

    // Estimate the index of the first visible element. This is used when elements can vary in size
    int EstimateFirstVisibleElementIndex(const AZ::Vector2& visibleContentBounds) const;

    // Find the real first visible element index based on an estimated first visible element index.
    // Also returns the bottom or right of the first visible element
    int FindFirstVisibleElementIndex(int estimatedIndex, const AZ::Vector2& visibleContentBounds, float& firstVisibleElementEndOut) const;

    // Calculate the visible space available before and after the specified visible element index
    void CalculateVisibleSpaceBeforeAndAfterElement(int visibleElementIndex,
        bool keepAtEnd,
        float visibleAreaBeginning,
        float& spaceLeftBeforeOut,
        float& spaceLeftAfterOut) const;

    // Calculate the first and last visible element indices based on a known visible element index.
    // Elements that have come into view will get their real size calculated if only their estimated
    // size was known. Returns the total change in element size, and the scroll delta needed to keep
    // the top or left of the passed in visible element index at the same position after the content
    // size is changed to accomodate the new element sizes
    void CalculateVisibleElementIndicesFromVisibleElementIndex(int visibleElementIndex,
        const AZ::Vector2& visibleContentBound,
        bool keepAtEnd,
        int& firstVisibleElementIndexOut,
        int& lastVisibleElementIndexOut,
        int& firstDisplayedElementIndexOut,
        int& lastDisplayedElementIndexOut,
        int& firstDisplayedElementIndexWithSizeChangeOut,
        float& totalElementSizeChangeOut,
        float& scrollChangeOut);

    // Calculate the change in the content's top or left when resizing content with the specified delta.
    // Used to get the scroll delta needed to keep the beginning (top or left) of the content element 
    // at the same position after a content size change
    float CalculateContentBeginningDeltaAfterSizeChange(float contentSizeDelta) const;

    // Calculate the change in the content's bottom or right when resizing content with the specified delta.
    // Used to get the scroll delta needed to keep the end (bottom or right) of the content element
    // at the same position after a content size change
    float CalculateContentEndDeltaAfterSizeChange(float contentSizeDelta) const;

    // Return whether the list is scrolled to the end
    bool IsScrolledToEnd() const;

    // Return whether the element at the specified index is being displayed
    bool IsElementDisplayedAtIndex(int index) const;

    // Get a recycled entity or a new entity for display
    AZ::EntityId GetElementForDisplay(ElementType elementType);

    // Get an entity to use to auto calculate sizes
    AZ::EntityId GetElementForAutoSizeCalculation(ElementType elementType);

    // Disable entities used to auto calculate sizes
    void DisableElementsForAutoSizeCalculation() const;

    // Calculate an element size with the layout cell interface
    float AutoCalculateElementSize(AZ::EntityId elementForAutoSizeCalculation) const;

    // Set an element's size based on index
    void SizeVariableElementAtIndex(AZ::EntityId element, int index) const;

    // Set an element's position based on index
    void PositionElementAtIndex(AZ::EntityId element, int index) const;

    // Set an element's anchors to the top or left
    void SetElementAnchors(AZ::EntityId element) const;

    // Set an element's offsets based on the specified offset
    void SetElementOffsets(AZ::EntityId element, float offset) const;

    // Get element type at the specified element index
    ElementType GetElementTypeAtIndex(int index) const;

    // Get index info of an element from the index of the element in the list
    ElementIndexInfo GetElementIndexInfoFromIndex(int index) const;

    // Get an index of an element in the list from the index info of that element
    int GetIndexFromElementIndexInfo(const ElementIndexInfo& elementIndexInfo) const;

    // Get the child of the content entity that contains the specified descendant
    AZ::EntityId GetImmediateContentChildFromDescendant(AZ::EntityId childElement) const;

    // Used for visibility attribute in reflection
    bool HeadersHaveVariableSizes() const;

    // Used to check if our item/header prototype elements contain this scrollbox which spawns it.
    // Prototype elements are spawned dynamically, and we need to avoid recursively spawning elements
    bool IsValidPrototype(AZ::EntityId entityId) const;

protected: // data

    //! Whether the list should refresh automatically on post activate
    bool m_autoRefreshOnPostActivate;

    //! Number of elements by default. Overridden by UiDynamicListDataBus::GetNumElements
    int m_defaultNumElements;

    //! The prototype element for the items in the list.
    //! Used when sections are enabled
    AZ::EntityId m_itemPrototypeElement;

    //! Whether items in the list can vary in size (height for vertical lists and width for horizontal lists)
    //! If false, the item size is fixed and is determined by the prototype element
    bool m_variableItemElementSize;

    //! Whether item sizes should be auto calculated or whether they should be requested.
    //! Applies when items can vary in size
    bool m_autoCalculateItemElementSize;

    //! An estimated height or width for the item size. If set to greater than 0, this size will be used
    //! until an item goes into view and its real size is calculated.
    //! Applies when items can vary in size
    float m_estimatedItemElementSize;

    //! Whether the list is divided into sections with headers
    bool m_hasSections;

    //! Number of sections by default. Overridden by UiDynamicListDataBus::GetNumSections
    int m_defaultNumSections;

    //! The prototype element for the headers in the list.
    //! Used when sections are enabled
    AZ::EntityId m_headerPrototypeElement;

    //! Whether headers should stick to the beginning of the visible area
    //! Used when sections are enabled
    bool m_stickyHeaders;

    //! Whether headers in the list can vary in size (height for vertical lists and width for horizontal lists)
    //! If false, the header size is fixed and is determined by the prototype element
    bool m_variableHeaderElementSize;

    //! Whether header sizes should be auto calculated or whether they should be requested.
    //! Applies when headers can vary in size
    bool m_autoCalculateHeaderElementSize;

    //! An estimated height or width for the header size. If set to greater than 0, this size will be used
    //! until a header goes into view and its real size is calculated.
    //! Applies when headers can vary in size
    float m_estimatedHeaderElementSize;

    //! The entity Ids of the prototype elements
    AZ::EntityId m_prototypeElement[ElementType::NumElementTypes];

    //! Stores the size of the prototype elements before they are removed from the content element's
    //! child list. Used to calculate the content size and element offsets when there is no variation in element size
    float m_prototypeElementSize[ElementType::NumElementTypes];

    //! Whether elements in the list can vary in size (height for vertical lists and width for horizontal lists)
    //! If false, the element size is fixed and is determined by the prototype element
    bool m_variableElementSize[ElementType::NumElementTypes];

    //! Whether element sizes should be auto calculated or whether they should be requested.
    //! Applies when elements can vary in size
    bool m_autoCalculateElementSize[ElementType::NumElementTypes];

    //! An estimated height or width for the element size. If set to greater than 0, this size will be used
    //! until an element goes into view and its real size is calculated.
    //! Applies when elements can vary in size
    float m_estimatedElementSize[ElementType::NumElementTypes];

    //! Whether elements in the list are navigable
    bool m_isPrototypeElementNavigable[ElementType::NumElementTypes];

    //! Average element size of the elements with known sizes
    float m_averageElementSize;

    //! The number of elements used to calculate the current average
    int m_numElementsUsedForAverage;

    //! The last calculated offset from the top or left of the content's parent (the visible area) to the top or left of the content.
    //! Used to estimate a first visible element index
    float m_lastCalculatedVisibleContentOffset;

    //! Whether the list is vertical or horizontal. Determined by the scroll box
    bool m_isVertical;

    //! A list of currently displayed elements
    AZStd::list<DisplayedElement> m_displayedElements;

    //! A list of unused entities
    AZStd::list<AZ::EntityId> m_recycledElements[ElementType::NumElementTypes];

    //! Cloned entities used to auto calculate sizes
    AZ::EntityId m_clonedElementForAutoSizeCalculation[ElementType::NumElementTypes];

    //! The header that is currently sticky at the top or left of the visible area
    DisplayedElement m_currentStickyHeader;

    //! First and last element indices that are being displayed (Not necessarily visible in viewport since we add an extra element for gamepad/keyboard navigation purposes)
    int m_firstDisplayedElementIndex;
    int m_lastDisplayedElementIndex;

    //! First and last element indices that are visible in the viewport
    int m_firstVisibleElementIndex;
    int m_lastVisibleElementIndex;

    //! Cached element sizes. Used when elements can vary in size
    AZStd::vector<CachedElementInfo> m_cachedElementInfo;

    //! The virtual number of elements in the list. Includes both headers and items
    int m_numElements;

    //! List of the sections in the list
    AZStd::vector<Section> m_sections;

    //! Whether the list has been set up for display
    bool m_listPreparedForDisplay;
};
