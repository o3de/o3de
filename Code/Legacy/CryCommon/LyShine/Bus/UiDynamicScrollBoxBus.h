/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Component/ComponentBus.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
//! Interface class that a dynamic scrollbox component needs to implement. A dynamic scrollbox
//! component sets up scrollbox content as a horizontal or vertical list of elements that are
//! cloned from prototype entities. Only the minimum number of entities are created for efficient
//! scrolling
class UiDynamicScrollBoxInterface
    : public AZ::ComponentBus
{
public: // member functions

    virtual ~UiDynamicScrollBoxInterface() {}

    //! Refresh the content. Should be called when list size or element content has changed.
    //! This will reset any cached information such as element sizes, so it is recommended
    //! to use AddElementsToEnd and RemoveElementsFromFront if possible when elements vary
    //! in size. AddElementsToEnd and RemoveElementsFromFront will also ensure that the
    //! scroll offset is adjusted to keep the visible elements in place
    virtual void RefreshContent() = 0;

    //! Add elements to the end of the list.
    //! Used with lists that are not divided into sections
    virtual void AddElementsToEnd(int numElementsToAdd, bool scrollToEndIfWasAtEnd) = 0;

    //! Remove elements from the front of the list.
    //! Used with lists that are not divided into sections
    virtual void RemoveElementsFromFront(int numElementsToRemove) = 0;

    //! Scroll to the end of the list
    virtual void ScrollToEnd() = 0;

    //! Get the element index of the specified child element. Returns -1 if not found.
    //! If the list is divided into sections, the index is local to the section
    virtual int GetElementIndexOfChild(AZ::EntityId childElement) = 0;

    //! Get the section index of the specified child element. Returns -1 if not found.
    //! Used with lists that are divided into sections
    virtual int GetSectionIndexOfChild(AZ::EntityId childElement) = 0;

    //! Get the child element at the specified element index.
    //! Used with lists that are not divided into sections
    virtual AZ::EntityId GetChildAtElementIndex(int index) = 0;

    //! Get the child element at the specified section index and element index.
    //! Used with lists that are divided into sections
    virtual AZ::EntityId GetChildAtSectionAndElementIndex(int sectionIndex, int index) = 0;

    //! Get whether the list should automatically prepare and refresh its content post activation
    virtual bool GetAutoRefreshOnPostActivate() = 0;

    //! Set whether the list should automatically prepare and refresh its content post activation
    virtual void SetAutoRefreshOnPostActivate(bool autoRefresh) = 0;

    //! Get the prototype entity used for the elements
    virtual AZ::EntityId GetPrototypeElement() = 0;

    //! Set the prototype entity used for the elements
    virtual void SetPrototypeElement(AZ::EntityId prototypeElement) = 0;

    //! Get whether the elements vary in size
    virtual bool GetElementsVaryInSize() = 0;

    //! Set whether the elements vary in size
    virtual void SetElementsVaryInSize(bool varyInSize) = 0;

    //! Get whether to auto calculate the elements when they vary in size
    virtual bool GetAutoCalculateVariableElementSize() = 0;

    //! Set whether to auto calculate the elements when they vary in size
    virtual void SetAutoCalculateVariableElementSize(bool autoCalculateSize) = 0;

    //! Get the estimated size for the variable elements. If set to 0, then element sizes
    //! are calculated up front rather than when becoming visible
    virtual float GetEstimatedVariableElementSize() = 0;

    //! Set the estimated size for the variable elements. If set to 0, then element sizes
    //! are calculated up front rather than when becoming visible
    virtual void SetEstimatedVariableElementSize(float estimatedSize) = 0;

    //! Get whether the list is divided into sections with headers
    virtual bool GetSectionsEnabled() = 0;

    //! Set whether the list is divided into sections with headers
    virtual void SetSectionsEnabled(bool enabled) = 0;

    //! Get the prototype entity used for the headers
    virtual AZ::EntityId GetPrototypeHeader() = 0;

    //! Set the prototype entity used for the headers
    virtual void SetPrototypeHeader(AZ::EntityId prototypeHeader) = 0;

    //! Get whether headers stick to the beginning of the visible list area
    virtual bool GetHeadersSticky() = 0;

    //! Set whether headers stick to the beginning of the visible list area
    virtual void SetHeadersSticky(bool stickyHeaders) = 0;

    //! Get whether the headers vary in size
    virtual bool GetHeadersVaryInSize() = 0;

    //! Set whether the headers vary in size
    virtual void SetHeadersVaryInSize(bool varyInSize) = 0;

    //! Get whether to auto calculate the headers when they vary in size
    virtual bool GetAutoCalculateVariableHeaderSize() = 0;

    //! Set whether to auto calculate the headers when they vary in size
    virtual void SetAutoCalculateVariableHeaderSize(bool autoCalculateSize) = 0;

    //! Get the estimated size for the variable headers. If set to 0, then header sizes
    //! are calculated up front rather than when becoming visible
    virtual float GetEstimatedVariableHeaderSize() = 0;

    //! Set the estimated size for the variable headers. If set to 0, then header sizes
    //! are calculated up front rather than when becoming visible
    virtual void SetEstimatedVariableHeaderSize(float estimatedSize) = 0;

public: // static member data

    //! Only one component on a entity can implement the events
    static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
};

typedef AZ::EBus<UiDynamicScrollBoxInterface> UiDynamicScrollBoxBus;

////////////////////////////////////////////////////////////////////////////////////////////////////
//! Interface class that provides data needed to display a list of elements
class UiDynamicScrollBoxDataInterface
    : public AZ::ComponentBus
{
public: // member functions

    virtual ~UiDynamicScrollBoxDataInterface() {}

    //! Returns the number of elements in the list.
    //! Called when the list is being constructed (in the component's InGamePostActivate or when RefreshContent is being called explicitely).
    //! Used with lists that are not divided into sections
    virtual int GetNumElements() { return 0; }

    //! Returns the width of an element at the specified index.
    //! Called when a horizontal list contains elements of varying size, and the element's "auto calculate size" option is disabled.
    //! Used with lists that are not divided into sections
    virtual float GetElementWidth([[maybe_unused]] int index) { return 0.0f; }

    //! Returns the height of an element at the specified index.
    //! Called when a vertical list contains elements of varying size, and the element's "auto calculate size" option is disabled.
    //! Used with lists that are not divided into sections
    virtual float GetElementHeight([[maybe_unused]] int index) { return 0.0f; }

    //! Returns the number of sections in the list.
    //! Called when the list is being constructed (in the component's InGamePostActivate or when RefreshContent is being called explicitely).
    //! Used with lists that are divided into sections
    virtual int GetNumSections() { return 0; }

    //! Returns the number of elements in the specified section.
    //! Called when the list is being constructed (in the component's InGamePostActivate or when RefreshContent is being called explicitely).
    //! Used with lists that are divided into sections
    virtual int GetNumElementsInSection([[maybe_unused]] int sectionIndex) { return 0; }

    //! Returns the width of an element at the specified section.
    //! Called when a horizontal list contains elements of varying size, and the element's "auto calculate size" option is disabled.
    //! Used with lists that are divided into sections
    virtual float GetElementInSectionWidth([[maybe_unused]] int sectionIndex, [[maybe_unused]] int elementindex) { return 0.0f; }

    //! Returns the height of an element at the specified section.
    //! Called when a vertical list contains elements of varying size, and the element's "auto calculate size" option is disabled.
    //! Used with lists that are divided into sections
    virtual float GetElementInSectionHeight([[maybe_unused]] int sectionIndex, [[maybe_unused]] int elementindex) { return 0.0f; }

    //! Returns the width of a header at the specified section.
    //! Called when a horizontal list contains headers of varying size, and the header's "auto calculate size" option is disabled.
    //! Used with lists that are divided into sections
    virtual float GetSectionHeaderWidth([[maybe_unused]] int sectionIndex) { return 0.0f; }

    //! Returns the height of a header at the specified section.
    //! Called when a vertical list contains elements of varying size, and the header's "auto calculate size" option is disabled.
    //! Used with lists that are divided into sections
    virtual float GetSectionHeaderHeight([[maybe_unused]] int sectionIndex) { return 0.0f; }

public: // static member data

    //! Only one component on a entity can implement the events
    static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
};

typedef AZ::EBus<UiDynamicScrollBoxDataInterface> UiDynamicScrollBoxDataBus;

////////////////////////////////////////////////////////////////////////////////////////////////////
//! Interface class that listeners need to implement to receive notifications of element state
//! changes, such as when an element is about to scroll into view
class UiDynamicScrollBoxElementNotifications
    : public AZ::ComponentBus
{
public: // member functions

    virtual ~UiDynamicScrollBoxElementNotifications(){}

    //! Called when an element is about to become visible. Used to populate the element with data for display.
    //! Used with lists that are not divided into sections
    virtual void OnElementBecomingVisible([[maybe_unused]] AZ::EntityId entityId, [[maybe_unused]] int index) {}

    //! Called when elements have variable sizes and are set to auto calculate.
    //! Used with lists that are not divided into sections
    virtual void OnPrepareElementForSizeCalculation([[maybe_unused]] AZ::EntityId entityId, [[maybe_unused]] int index) {}

    //! Called when an element in a section is about to become visible. Used to populate the element with data for display
    //! Used with lists that are divided into sections
    virtual void OnElementInSectionBecomingVisible([[maybe_unused]] AZ::EntityId entityId, [[maybe_unused]] int sectionIndex, [[maybe_unused]] int index) {}

    //! Called when elements in sections have variable sizes and are set to auto calculate
    //! Used with lists that are divided into sections
    virtual void OnPrepareElementInSectionForSizeCalculation([[maybe_unused]] AZ::EntityId entityId, [[maybe_unused]] int sectionIndex, [[maybe_unused]] int index) {}

    //! Called when a header is about to become visible. Used to populate the header with data for display.
    //! Used with lists that are divided into sections
    virtual void OnSectionHeaderBecomingVisible([[maybe_unused]] AZ::EntityId entityId, [[maybe_unused]] int sectionIndex) {}

    //! Called when headers have variable sizes and are set to auto calculate.
    //! Used with lists that are divided into sections
    virtual void OnPrepareSectionHeaderForSizeCalculation([[maybe_unused]] AZ::EntityId entityId, [[maybe_unused]] int sectionIndex) {}
};

typedef AZ::EBus<UiDynamicScrollBoxElementNotifications> UiDynamicScrollBoxElementNotificationBus;
