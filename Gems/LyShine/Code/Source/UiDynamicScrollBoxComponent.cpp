/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "UiDynamicScrollBoxComponent.h"

#include "UiElementComponent.h"
#include "UiNavigationHelpers.h"
#include "UiLayoutHelpers.h"

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Component/ComponentApplicationBus.h>

#include <LyShine/Bus/UiTransform2dBus.h>
#include <LyShine/Bus/UiCanvasBus.h>
#include <LyShine/Bus/UiLayoutCellBus.h>
#include <LyShine/Bus/UiLayoutCellDefaultBus.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
//! UiDynamicScrollBoxDataBus Behavior context handler class
class BehaviorUiDynamicScrollBoxDataBusHandler
    : public UiDynamicScrollBoxDataBus::Handler
    , public AZ::BehaviorEBusHandler
{
public:
    AZ_EBUS_BEHAVIOR_BINDER(BehaviorUiDynamicScrollBoxDataBusHandler, "{74FA95AB-D4C2-40B8-8568-1B174BF577C0}", AZ::SystemAllocator,
        GetNumElements, GetElementWidth, GetElementHeight, GetNumSections, GetNumElementsInSection,
        GetElementInSectionWidth, GetElementInSectionHeight, GetSectionHeaderWidth, GetSectionHeaderHeight);

    int GetNumElements() override
    {
        int numElements = 0;
        CallResult(numElements, FN_GetNumElements);
        return numElements;
    }

    float GetElementWidth(int index) override
    {
        float width = 0.0f;
        CallResult(width, FN_GetElementWidth, index);
        return width;
    }

    float GetElementHeight(int index) override
    {
        float height = 0.0f;
        CallResult(height, FN_GetElementHeight, index);
        return height;
    }

    int GetNumSections() override
    {
        int numSections = 0;
        CallResult(numSections, FN_GetNumSections);
        return numSections;
    }

    int GetNumElementsInSection(int sectionIndex) override
    {
        int numElementsInSection = 0;
        CallResult(numElementsInSection, FN_GetNumElementsInSection, sectionIndex);
        return numElementsInSection;
    }

    float GetElementInSectionWidth(int sectionIndex, int index) override
    {
        float width = 0.0f;
        CallResult(width, FN_GetElementInSectionWidth, sectionIndex, index);
        return width;
    }

    float GetElementInSectionHeight(int sectionIndex, int index) override
    {
        float height = 0.0f;
        CallResult(height, FN_GetElementInSectionHeight, sectionIndex, index);
        return height;
    }

    float GetSectionHeaderWidth(int sectionIndex) override
    {
        float width = 0.0f;
        CallResult(width, FN_GetSectionHeaderWidth, sectionIndex);
        return width;
    }

    float GetSectionHeaderHeight(int sectionIndex) override
    {
        float height = 0.0f;
        CallResult(height, FN_GetSectionHeaderHeight, sectionIndex);
        return height;
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////
//! UiDynamicScrollBoxElementNotificationBus Behavior context handler class
class BehaviorUiDynamicScrollBoxElementNotificationBusHandler
    : public UiDynamicScrollBoxElementNotificationBus::Handler
    , public AZ::BehaviorEBusHandler
{
public:
    AZ_EBUS_BEHAVIOR_BINDER(BehaviorUiDynamicScrollBoxElementNotificationBusHandler, "{4D166273-4D12-45A4-BC42-A7FF59A2092E}", AZ::SystemAllocator,
        OnElementBecomingVisible, OnPrepareElementForSizeCalculation,
        OnElementInSectionBecomingVisible, OnPrepareElementInSectionForSizeCalculation,
        OnSectionHeaderBecomingVisible, OnPrepareSectionHeaderForSizeCalculation);

    void OnElementBecomingVisible(AZ::EntityId entityId, int index) override
    {
        Call(FN_OnElementBecomingVisible, entityId, index);
    }

    void OnPrepareElementForSizeCalculation(AZ::EntityId entityId, int index) override
    {
        Call(FN_OnPrepareElementForSizeCalculation, entityId, index);
    }

    void OnElementInSectionBecomingVisible(AZ::EntityId entityId, int sectionIndex, int index) override
    {
        Call(FN_OnElementInSectionBecomingVisible, entityId, sectionIndex, index);
    }

    void OnPrepareElementInSectionForSizeCalculation(AZ::EntityId entityId, int sectionIndex, int index) override
    {
        Call(FN_OnPrepareElementInSectionForSizeCalculation, entityId, sectionIndex, index);
    }

    void OnSectionHeaderBecomingVisible(AZ::EntityId entityId, int sectionIndex) override
    {
        Call(FN_OnSectionHeaderBecomingVisible, entityId, sectionIndex);
    }

    void OnPrepareSectionHeaderForSizeCalculation(AZ::EntityId entityId, int sectionIndex) override
    {
        Call(FN_OnPrepareSectionHeaderForSizeCalculation, entityId, sectionIndex);
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////
// PUBLIC MEMBER FUNCTIONS
////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////
UiDynamicScrollBoxComponent::CachedElementInfo::CachedElementInfo()
    : m_size(-1.0f)
    , m_accumulatedSize(-1.0f)
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
UiDynamicScrollBoxComponent::UiDynamicScrollBoxComponent()
    : m_autoRefreshOnPostActivate(true)
    , m_defaultNumElements(0)
    , m_variableItemElementSize(false)
    , m_autoCalculateItemElementSize(true)
    , m_estimatedItemElementSize(0.0f)
    , m_hasSections(false)
    , m_defaultNumSections(1)
    , m_stickyHeaders(false)
    , m_variableHeaderElementSize(false)
    , m_autoCalculateHeaderElementSize(true)
    , m_estimatedHeaderElementSize(0.0f)
    , m_averageElementSize(0.0f)
    , m_numElementsUsedForAverage(0)
    , m_lastCalculatedVisibleContentOffset(0.0f)
    , m_isVertical(true)
    , m_firstDisplayedElementIndex(-1)
    , m_lastDisplayedElementIndex(-1)
    , m_firstVisibleElementIndex(-1)
    , m_lastVisibleElementIndex(-1)
    , m_numElements(0)
    , m_listPreparedForDisplay(false)
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
UiDynamicScrollBoxComponent::~UiDynamicScrollBoxComponent()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiDynamicScrollBoxComponent::RefreshContent()
{
    if (!m_listPreparedForDisplay)
    {
        PrepareListForDisplay();
    }

    ResizeContentToFitElements();

    ClearDisplayedElements();

    bool keepAtEndIfWasAtEnd = false;
    if (AnyElementTypesHaveEstimatedSizes())
    {
        // Check if the content's pivot is at the end (bottom or right)
        AZ::EntityId contentEntityId;
        UiScrollBoxBus::EventResult(contentEntityId, GetEntityId(), &UiScrollBoxBus::Events::GetContentEntity);
        if (contentEntityId.IsValid())
        {
            AZ::Vector2 pivot(0.0f, 0.0f);
            UiTransformBus::EventResult(pivot, contentEntityId, &UiTransformBus::Events::GetPivot);

            if (m_isVertical)
            {
                keepAtEndIfWasAtEnd = pivot.GetY() == 1.0f;
            }
            else
            {
                keepAtEndIfWasAtEnd = pivot.GetX() == 1.0f;
            }
        }
    }

    UpdateElementVisibility(keepAtEndIfWasAtEnd);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiDynamicScrollBoxComponent::AddElementsToEnd(int numElementsToAdd, bool scrollToEndIfWasAtEnd)
{
    AZ_Warning("UiDynamicScrollBoxComponent", m_listPreparedForDisplay, "AddElementsToEnd() is only supported after the first content refresh");
    if (!m_listPreparedForDisplay)
    {
        return;
    }

    AZ_Warning("UiDynamicScrollBoxComponent", !m_hasSections, "AddElementsToEnd() can only be used on lists that are not divided into sections");

    if (numElementsToAdd > 0 && !m_hasSections)
    {
        m_numElements += numElementsToAdd;

        // Calculate new content size
        float sizeDiff = 0.0f;
        if (!m_variableElementSize[ElementType::Item])
        {
            sizeDiff = numElementsToAdd * m_prototypeElementSize[ElementType::Item];
        }
        else
        {
            // Add cache entries for the new elements
            m_cachedElementInfo.insert(m_cachedElementInfo.end(), numElementsToAdd, CachedElementInfo());

            for (int i = m_numElements - numElementsToAdd; i < m_numElements; i++)
            {
                sizeDiff += GetAndCacheVariableElementSize(i);
            }

            if (m_autoCalculateElementSize[ElementType::Item])
            {
                DisableElementsForAutoSizeCalculation();
            }

            UpdateAverageElementSize(numElementsToAdd, sizeDiff);
        }

        bool scrollToEnd = scrollToEndIfWasAtEnd && IsScrolledToEnd();
        if (scrollToEnd)
        {
            float scrollDiff = CalculateContentEndDeltaAfterSizeChange(sizeDiff);
            AdjustContentSizeAndScrollOffsetByDelta(sizeDiff, scrollDiff);

            if (!IsScrolledToEnd())
            {
                ScrollToEnd();
            }
            else
            {
                UpdateElementVisibility(true);
            }
        }
        else
        {
            float scrollDiff = CalculateContentBeginningDeltaAfterSizeChange(sizeDiff);
            AdjustContentSizeAndScrollOffsetByDelta(sizeDiff, scrollDiff);

            UpdateElementVisibility();
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiDynamicScrollBoxComponent::RemoveElementsFromFront(int numElementsToRemove)
{
    AZ_Warning("UiDynamicScrollBoxComponent", m_listPreparedForDisplay, "RemoveElementsFromFront() is only supported after the first content refresh");
    if (!m_listPreparedForDisplay)
    {
        return;
    }

    AZ_Warning("UiDynamicScrollBoxComponent", !m_hasSections, "RemoveElementsFromFront() can only be used on lists that are not divided into sections");

    if (numElementsToRemove > 0 && !m_hasSections)
    {
        AZ_Warning("UiDynamicScrollBoxComponent", numElementsToRemove <= m_numElements, "attempting to remove more elements than are in the list");

        numElementsToRemove = AZ::GetClamp(numElementsToRemove, 0, m_numElements);

        float sizeDiff = 0.0f;
        if (!m_variableElementSize[ElementType::Item])
        {
            sizeDiff = numElementsToRemove * m_prototypeElementSize[ElementType::Item];
        }
        else
        {
            // Get accumulated size being removed
            sizeDiff = GetVariableSizeElementOffset(numElementsToRemove - 1) + GetVariableElementSize(numElementsToRemove - 1);

            // Update cached element info
            m_cachedElementInfo.erase(m_cachedElementInfo.begin(), m_cachedElementInfo.begin() + numElementsToRemove);

            // Update accumulated sizes
            int newElementCount = m_numElements - numElementsToRemove;
            for (int i = 0; i < newElementCount; i++)
            {
                if (m_cachedElementInfo[i].m_accumulatedSize >= 0.0f)
                {
                    m_cachedElementInfo[i].m_accumulatedSize -= sizeDiff;
                }
            }
        }
        sizeDiff = -sizeDiff;

        m_numElements -= numElementsToRemove;

        if (numElementsToRemove > 0)
        {
            ClearDisplayedElements();

            float scrollDiff = CalculateContentBeginningDeltaAfterSizeChange(sizeDiff) - sizeDiff;
            AdjustContentSizeAndScrollOffsetByDelta(sizeDiff, scrollDiff);

            UpdateElementVisibility();
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiDynamicScrollBoxComponent::ScrollToEnd()
{
    AZ_Warning("UiDynamicScrollBoxComponent", m_listPreparedForDisplay, "ScrollToEnd() is only supported after the first content refresh");
    if (!m_listPreparedForDisplay)
    {
        return;
    }

    // Find the content element
    AZ::EntityId contentEntityId;
    UiScrollBoxBus::EventResult(contentEntityId, GetEntityId(), &UiScrollBoxBus::Events::GetContentEntity);

    if (!contentEntityId.IsValid())
    {
        return;
    }

    // Get content's parent
    AZ::EntityId contentParentEntityId;
    UiElementBus::EventResult(contentParentEntityId, contentEntityId, &UiElementBus::Events::GetParentEntityId);
    if (!contentParentEntityId.IsValid())
    {
        return;
    }

    // Get content's rect in canvas space
    UiTransformInterface::Rect contentRect;
    UiTransformBus::Event(contentEntityId, &UiTransformBus::Events::GetCanvasSpaceRectNoScaleRotate, contentRect);

    // Get content parent's rect in canvas space
    UiTransformInterface::Rect parentRect;
    UiTransformBus::Event(contentParentEntityId, &UiTransformBus::Events::GetCanvasSpaceRectNoScaleRotate, parentRect);

    float scrollDelta = 0.0f;
    if (m_isVertical)
    {
        if (contentRect.bottom > parentRect.bottom)
        {
            scrollDelta = parentRect.bottom - contentRect.bottom;
        }
    }
    else
    {
        if (contentRect.right > parentRect.right)
        {
            scrollDelta = parentRect.right - contentRect.right;
        }
    }

    if (scrollDelta != 0.0f)
    {
        AdjustContentSizeAndScrollOffsetByDelta(0.0f, scrollDelta);

        UpdateElementVisibility(true);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
int UiDynamicScrollBoxComponent::GetElementIndexOfChild(AZ::EntityId childElement)
{
    AZ::EntityId immediateChild = GetImmediateContentChildFromDescendant(childElement);

    for (const auto& e : m_displayedElements)
    {
        if (e.m_element == immediateChild)
        {
            if (!m_hasSections)
            {
                return e.m_elementIndex;
            }
            else
            {
                return e.m_indexInfo.m_itemIndexInSection;
            }
        }
    }

    return -1;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
int UiDynamicScrollBoxComponent::GetSectionIndexOfChild(AZ::EntityId childElement)
{
    AZ_Warning("UiDynamicScrollBoxComponent", m_hasSections, "GetSectionIndexOfChild() can only be used on lists that are divided into sections");

    if (m_hasSections)
    {
        AZ::EntityId immediateChild = GetImmediateContentChildFromDescendant(childElement);

        for (const auto& e : m_displayedElements)
        {
            if (e.m_element == immediateChild)
            {
                return e.m_indexInfo.m_sectionIndex;
            }
        }
    }

    return -1;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZ::EntityId UiDynamicScrollBoxComponent::GetChildAtElementIndex(int index)
{
    AZ::EntityId elementId;

    AZ_Warning("UiDynamicScrollBoxComponent", !m_hasSections, "GetChildAtElementIndex() can only be used on lists that are not divided into sections");

    if (!m_hasSections)
    {
        elementId = FindDisplayedElementWithIndex(index);
    }

    return elementId;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZ::EntityId  UiDynamicScrollBoxComponent::GetChildAtSectionAndElementIndex(int sectionIndex, int index)
{
    AZ::EntityId elementId;

    AZ_Warning("UiDynamicScrollBoxComponent", m_hasSections, "GetChildElementAtSectionAndLocationIndex() can only be used on lists that are divided into sections");

    if (m_hasSections)
    {
        for (const auto& e : m_displayedElements)
        {
            if (e.m_indexInfo.m_sectionIndex == sectionIndex && e.m_indexInfo.m_itemIndexInSection == index)
            {
                elementId = e.m_element;
                break;
            }
        }
    }

    return elementId;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiDynamicScrollBoxComponent::GetAutoRefreshOnPostActivate()
{
    return m_autoRefreshOnPostActivate;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiDynamicScrollBoxComponent::SetAutoRefreshOnPostActivate(bool autoRefresh)
{
    m_autoRefreshOnPostActivate = autoRefresh;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZ::EntityId UiDynamicScrollBoxComponent::GetPrototypeElement()
{
    return m_itemPrototypeElement;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiDynamicScrollBoxComponent::SetPrototypeElement(AZ::EntityId prototypeElement)
{
    AZ_Warning("UiDynamicScrollBoxComponent", !m_listPreparedForDisplay, "Changing properties is only supported before the first content refresh");

    if (!m_listPreparedForDisplay)
    {
        m_itemPrototypeElement = prototypeElement;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiDynamicScrollBoxComponent::GetElementsVaryInSize()
{
    return m_variableItemElementSize;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiDynamicScrollBoxComponent::SetElementsVaryInSize(bool varyInSize)
{
    AZ_Warning("UiDynamicScrollBoxComponent", !m_listPreparedForDisplay, "Changing properties is only supported before the first content refresh");

    if (!m_listPreparedForDisplay)
    {
        m_variableItemElementSize = varyInSize;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiDynamicScrollBoxComponent::GetAutoCalculateVariableElementSize()
{
    return m_autoCalculateItemElementSize;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiDynamicScrollBoxComponent::SetAutoCalculateVariableElementSize(bool autoCalculateSize)
{
    AZ_Warning("UiDynamicScrollBoxComponent", !m_listPreparedForDisplay, "Changing properties is only supported before the first content refresh");

    if (!m_listPreparedForDisplay)
    {
        m_autoCalculateItemElementSize = autoCalculateSize;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
float UiDynamicScrollBoxComponent::GetEstimatedVariableElementSize()
{
    return m_estimatedItemElementSize;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiDynamicScrollBoxComponent::SetEstimatedVariableElementSize(float estimatedSize)
{
    AZ_Warning("UiDynamicScrollBoxComponent", !m_listPreparedForDisplay, "Changing properties is only supported before the first content refresh");

    if (!m_listPreparedForDisplay)
    {
        m_estimatedItemElementSize = AZ::GetMax(estimatedSize, 0.0f);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiDynamicScrollBoxComponent::GetSectionsEnabled()
{
    return m_hasSections;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiDynamicScrollBoxComponent::SetSectionsEnabled(bool sectionsEnabled)
{
    AZ_Warning("UiDynamicScrollBoxComponent", !m_listPreparedForDisplay, "Changing properties is only supported before the first content refresh");

    if (!m_listPreparedForDisplay)
    {
        m_hasSections = sectionsEnabled;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZ::EntityId UiDynamicScrollBoxComponent::GetPrototypeHeader()
{
    return m_headerPrototypeElement;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiDynamicScrollBoxComponent::SetPrototypeHeader(AZ::EntityId prototypeHeader)
{
    AZ_Warning("UiDynamicScrollBoxComponent", !m_listPreparedForDisplay, "Changing properties is only supported before the first content refresh");

    if (!m_listPreparedForDisplay)
    {
        m_headerPrototypeElement = prototypeHeader;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiDynamicScrollBoxComponent::GetHeadersSticky()
{
    return m_stickyHeaders;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiDynamicScrollBoxComponent::SetHeadersSticky(bool stickyHeaders)
{
    AZ_Warning("UiDynamicScrollBoxComponent", !m_listPreparedForDisplay, "Changing properties is only supported before the first content refresh");

    if (!m_listPreparedForDisplay)
    {
        m_stickyHeaders = stickyHeaders;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiDynamicScrollBoxComponent::GetHeadersVaryInSize()
{
    return m_variableHeaderElementSize;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiDynamicScrollBoxComponent::SetHeadersVaryInSize(bool varyInSize)
{
    AZ_Warning("UiDynamicScrollBoxComponent", !m_listPreparedForDisplay, "Changing properties is only supported before the first content refresh");

    if (!m_listPreparedForDisplay)
    {
        m_variableHeaderElementSize = varyInSize;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiDynamicScrollBoxComponent::GetAutoCalculateVariableHeaderSize()
{
    return m_autoCalculateHeaderElementSize;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiDynamicScrollBoxComponent::SetAutoCalculateVariableHeaderSize(bool autoCalculateSize)
{
    AZ_Warning("UiDynamicScrollBoxComponent", !m_listPreparedForDisplay, "Changing properties is only supported before the first content refresh");

    if (!m_listPreparedForDisplay)
    {
        m_autoCalculateHeaderElementSize = autoCalculateSize;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
float UiDynamicScrollBoxComponent::GetEstimatedVariableHeaderSize()
{
    return m_estimatedHeaderElementSize;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiDynamicScrollBoxComponent::SetEstimatedVariableHeaderSize(float estimatedSize)
{
    AZ_Warning("UiDynamicScrollBoxComponent", !m_listPreparedForDisplay, "Changing properties is only supported before the first content refresh");

    if (!m_listPreparedForDisplay)
    {
        m_estimatedHeaderElementSize = AZ::GetMax(estimatedSize, 0.0f);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiDynamicScrollBoxComponent::OnScrollOffsetChanging([[maybe_unused]] AZ::Vector2 newScrollOffset)
{
    UpdateElementVisibility();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiDynamicScrollBoxComponent::OnScrollOffsetChanged([[maybe_unused]] AZ::Vector2 newScrollOffset)
{
    UpdateElementVisibility();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiDynamicScrollBoxComponent::InGamePostActivate()
{
    if (m_autoRefreshOnPostActivate)
    {
        RefreshContent();
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiDynamicScrollBoxComponent::OnCanvasSpaceRectChanged([[maybe_unused]] AZ::EntityId entityId, const UiTransformInterface::Rect& oldRect, const UiTransformInterface::Rect& newRect)
{
    // If old rect equals new rect, size changed due to initialization
    bool sizeChanged = (oldRect == newRect) || (!oldRect.GetSize().IsClose(newRect.GetSize(), 0.05f));

    if (sizeChanged)
    {
        UpdateElementVisibility();
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiDynamicScrollBoxComponent::OnUiElementBeingDestroyed()
{
    for (int i = 0; i < ElementType::NumElementTypes; i++)
    {
        if (m_prototypeElement[i].IsValid())
        {
            UiElementBus::Event(m_prototypeElement[i], &UiElementBus::Events::DestroyElement);
            m_prototypeElement[i].SetInvalid();
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// PUBLIC STATIC MEMBER FUNCTIONS
////////////////////////////////////////////////////////////////////////////////////////////////////

void UiDynamicScrollBoxComponent::Reflect(AZ::ReflectContext* context)
{
    AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);

    if (serializeContext)
    {
        serializeContext->Class<UiDynamicScrollBoxComponent, AZ::Component>()
            ->Version(1)
            ->Field("AutoRefreshOnPostActivate", &UiDynamicScrollBoxComponent::m_autoRefreshOnPostActivate)
            ->Field("PrototypeElement", &UiDynamicScrollBoxComponent::m_itemPrototypeElement)
            ->Field("VariableElementSize", &UiDynamicScrollBoxComponent::m_variableItemElementSize)
            ->Field("AutoCalcElementSize", &UiDynamicScrollBoxComponent::m_autoCalculateItemElementSize)
            ->Field("EstimatedElementSize", &UiDynamicScrollBoxComponent::m_estimatedItemElementSize)
            ->Field("DefaultNumElements", &UiDynamicScrollBoxComponent::m_defaultNumElements)
            ->Field("HasSections", &UiDynamicScrollBoxComponent::m_hasSections)
            ->Field("HeaderPrototypeElement", &UiDynamicScrollBoxComponent::m_headerPrototypeElement)
            ->Field("StickyHeaders", &UiDynamicScrollBoxComponent::m_stickyHeaders)
            ->Field("VariableHeaderSize", &UiDynamicScrollBoxComponent::m_variableHeaderElementSize)
            ->Field("AutoCalcHeaderSize", &UiDynamicScrollBoxComponent::m_autoCalculateHeaderElementSize)
            ->Field("EstimatedHeaderSize", &UiDynamicScrollBoxComponent::m_estimatedHeaderElementSize)
            ->Field("DefaultNumSections", &UiDynamicScrollBoxComponent::m_defaultNumSections);

        AZ::EditContext* ec = serializeContext->GetEditContext();
        if (ec)
        {
            auto editInfo = ec->Class<UiDynamicScrollBoxComponent>("DynamicScrollBox",
                    "A component that dynamically sets up scroll box content as a horizontal or vertical list of elements that\n"
                    "are cloned from a prototype element. Only the minimum number of elements are created for efficient scrolling.\n"
                    "The scroll box's content element's first child acts as the prototype element.");

            editInfo->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                ->Attribute(AZ::Edit::Attributes::Category, "UI")
                ->Attribute(AZ::Edit::Attributes::Icon, "Editor/Icons/Components/UiDynamicScrollBox.png")
                ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Editor/Icons/Components/Viewport/UiDynamicScrollBox.png")
                ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("UI"))
                ->Attribute(AZ::Edit::Attributes::AutoExpand, true);

            editInfo->DataElement(0, &UiDynamicScrollBoxComponent::m_autoRefreshOnPostActivate, "Refresh on activate",
                "Whether the list should automatically prepare and refresh its content post activation.");

            editInfo->DataElement(0, &UiDynamicScrollBoxComponent::m_itemPrototypeElement, "Prototype element",
                "The prototype element to be used for the elements in the list. If empty, the prototype element will default to the first child of the content element.");

            editInfo->DataElement(0, &UiDynamicScrollBoxComponent::m_variableItemElementSize, "Variable element size",
                "Whether elements in the list can vary in size. If not, the element size is fixed and is determined by the prototype element.")
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ_CRC_CE("RefreshEntireTree"));

            editInfo->DataElement(0, &UiDynamicScrollBoxComponent::m_autoCalculateItemElementSize, "Auto calc element size",
                "Whether element sizes should be auto calculated or whether they should be requested.")
                ->Attribute(AZ::Edit::Attributes::Visibility, &UiDynamicScrollBoxComponent::m_variableItemElementSize);

            editInfo->DataElement(0, &UiDynamicScrollBoxComponent::m_estimatedItemElementSize, "Estimated element size",
                "The element size to use as an estimate before the element appears in the view. If set to 0, sizes will be calculated up front.")
                ->Attribute(AZ::Edit::Attributes::Visibility, &UiDynamicScrollBoxComponent::m_variableItemElementSize)
                ->Attribute(AZ::Edit::Attributes::Min, 0.0f);

            editInfo->DataElement(AZ::Edit::UIHandlers::SpinBox, &UiDynamicScrollBoxComponent::m_defaultNumElements, "Default num elements",
                "The default number of elements in the list.")
                ->Attribute(AZ::Edit::Attributes::Min, 0);

            editInfo->ClassElement(AZ::Edit::ClassElements::Group, "Sections")
                ->Attribute(AZ::Edit::Attributes::AutoExpand, true);

            editInfo->DataElement(0, &UiDynamicScrollBoxComponent::m_hasSections, "Enabled",
                "Whether the list should be divided into sections with headers.")
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ_CRC_CE("RefreshEntireTree"));

            editInfo->DataElement(0, &UiDynamicScrollBoxComponent::m_headerPrototypeElement, "Prototype header",
                "The prototype element to be used for the section headers in the list.")
                ->Attribute(AZ::Edit::Attributes::Visibility, &UiDynamicScrollBoxComponent::m_hasSections);

            editInfo->DataElement(0, &UiDynamicScrollBoxComponent::m_stickyHeaders, "Sticky headers",
                "Whether headers should stick to the beginning of the visible list area.")
                ->Attribute(AZ::Edit::Attributes::Visibility, &UiDynamicScrollBoxComponent::m_hasSections);

            editInfo->DataElement(0, &UiDynamicScrollBoxComponent::m_variableHeaderElementSize, "Variable header size",
                "Whether headers in the list can vary in size. If not, the header size is fixed and is determined by the prototype element.")
                ->Attribute(AZ::Edit::Attributes::Visibility, &UiDynamicScrollBoxComponent::m_hasSections)
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ_CRC_CE("RefreshEntireTree"));

            editInfo->DataElement(0, &UiDynamicScrollBoxComponent::m_autoCalculateHeaderElementSize, "Auto calc header size",
                "Whether header sizes should be auto calculated or whether they should be requested.")
                ->Attribute(AZ::Edit::Attributes::Visibility, &UiDynamicScrollBoxComponent::HeadersHaveVariableSizes);

            editInfo->DataElement(0, &UiDynamicScrollBoxComponent::m_estimatedHeaderElementSize, "Estimated header size",
                "The header size to use as an estimate before the header appears in the view. If set to 0, sizes will be calculated up front.")
                ->Attribute(AZ::Edit::Attributes::Visibility, &UiDynamicScrollBoxComponent::HeadersHaveVariableSizes)
                ->Attribute(AZ::Edit::Attributes::Min, 0.0f);

            editInfo->DataElement(AZ::Edit::UIHandlers::SpinBox, &UiDynamicScrollBoxComponent::m_defaultNumSections, "Default num sections",
                "The default number of sections in the list.")
                ->Attribute(AZ::Edit::Attributes::Visibility, &UiDynamicScrollBoxComponent::m_hasSections)
                ->Attribute(AZ::Edit::Attributes::Min, 1);
        }
    }

    AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context);
    if (behaviorContext)
    {
        behaviorContext->EBus<UiDynamicScrollBoxBus>("UiDynamicScrollBoxBus")
            ->Event("RefreshContent", &UiDynamicScrollBoxBus::Events::RefreshContent)
            ->Event("AddElementsToEnd", &UiDynamicScrollBoxBus::Events::AddElementsToEnd)
            ->Event("RemoveElementsFromFront", &UiDynamicScrollBoxBus::Events::RemoveElementsFromFront)
            ->Event("ScrollToEnd", &UiDynamicScrollBoxBus::Events::ScrollToEnd)
            ->Event("GetElementIndexOfChild", &UiDynamicScrollBoxBus::Events::GetElementIndexOfChild)
            ->Event("GetSectionIndexOfChild", &UiDynamicScrollBoxBus::Events::GetSectionIndexOfChild)
            ->Event("GetChildAtElementIndex", &UiDynamicScrollBoxBus::Events::GetChildAtElementIndex)
            ->Event("GetChildAtSectionAndElementIndex", &UiDynamicScrollBoxBus::Events::GetChildAtSectionAndElementIndex)
            ->Event("GetAutoRefreshOnPostActivate", &UiDynamicScrollBoxBus::Events::GetAutoRefreshOnPostActivate)
            ->Event("SetAutoRefreshOnPostActivate", &UiDynamicScrollBoxBus::Events::SetAutoRefreshOnPostActivate)
            ->Event("GetPrototypeElement", &UiDynamicScrollBoxBus::Events::GetPrototypeElement)
            ->Event("SetPrototypeElement", &UiDynamicScrollBoxBus::Events::SetPrototypeElement)
            ->Event("GetElementsVaryInSize", &UiDynamicScrollBoxBus::Events::GetElementsVaryInSize)
            ->Event("SetElementsVaryInSize", &UiDynamicScrollBoxBus::Events::SetElementsVaryInSize)
            ->Event("GetAutoCalculateVariableElementSize", &UiDynamicScrollBoxBus::Events::GetAutoCalculateVariableElementSize)
            ->Event("SetAutoCalculateVariableElementSize", &UiDynamicScrollBoxBus::Events::SetAutoCalculateVariableElementSize)
            ->Event("GetEstimatedVariableElementSize", &UiDynamicScrollBoxBus::Events::GetEstimatedVariableElementSize)
            ->Event("SetEstimatedVariableElementSize", &UiDynamicScrollBoxBus::Events::SetEstimatedVariableElementSize)
            ->Event("GetSectionsEnabled", &UiDynamicScrollBoxBus::Events::GetSectionsEnabled)
            ->Event("SetSectionsEnabled", &UiDynamicScrollBoxBus::Events::SetSectionsEnabled)
            ->Event("GetPrototypeHeader", &UiDynamicScrollBoxBus::Events::GetPrototypeHeader)
            ->Event("SetPrototypeHeader", &UiDynamicScrollBoxBus::Events::SetPrototypeHeader)
            ->Event("GetHeadersSticky", &UiDynamicScrollBoxBus::Events::GetHeadersSticky)
            ->Event("SetHeadersSticky", &UiDynamicScrollBoxBus::Events::SetHeadersSticky)
            ->Event("GetHeadersVaryInSize", &UiDynamicScrollBoxBus::Events::GetHeadersVaryInSize)
            ->Event("SetHeadersVaryInSize", &UiDynamicScrollBoxBus::Events::SetHeadersVaryInSize)
            ->Event("GetAutoCalculateVariableHeaderSize", &UiDynamicScrollBoxBus::Events::GetAutoCalculateVariableHeaderSize)
            ->Event("SetAutoCalculateVariableHeaderSize", &UiDynamicScrollBoxBus::Events::SetAutoCalculateVariableHeaderSize)
            ->Event("GetEstimatedVariableHeaderSize", &UiDynamicScrollBoxBus::Events::GetEstimatedVariableHeaderSize)
            ->Event("SetEstimatedVariableHeaderSize", &UiDynamicScrollBoxBus::Events::SetEstimatedVariableHeaderSize)
            ;

        behaviorContext->EBus<UiDynamicScrollBoxDataBus>("UiDynamicScrollBoxDataBus")
            ->Handler<BehaviorUiDynamicScrollBoxDataBusHandler>();

        behaviorContext->EBus<UiDynamicScrollBoxElementNotificationBus>("UiDynamicScrollBoxElementNotificationBus")
            ->Handler<BehaviorUiDynamicScrollBoxElementNotificationBusHandler>();
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// PROTECTED MEMBER FUNCTIONS
////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiDynamicScrollBoxComponent::Activate()
{
    UiDynamicScrollBoxBus::Handler::BusConnect(GetEntityId());
    UiInitializationBus::Handler::BusConnect(GetEntityId());
    UiElementNotificationBus::Handler::BusConnect(GetEntityId());
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiDynamicScrollBoxComponent::Deactivate()
{
    UiDynamicScrollBoxBus::Handler::BusDisconnect();
    UiInitializationBus::Handler::BusDisconnect();
    if (UiTransformChangeNotificationBus::Handler::BusIsConnected())
    {
        UiTransformChangeNotificationBus::Handler::BusDisconnect();
    }
    if (UiScrollBoxNotificationBus::Handler::BusIsConnected())
    {
        UiScrollBoxNotificationBus::Handler::BusDisconnect();
    }
    UiElementNotificationBus::Handler::BusDisconnect();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiDynamicScrollBoxComponent::PrepareListForDisplay()
{
    if (m_listPreparedForDisplay)
    {
        return;
    }

    // Set whether list is vertical or horizontal
    m_isVertical = true;
    UiScrollBoxBus::EventResult(m_isVertical, GetEntityId(), &UiScrollBoxBus::Events::GetIsVerticalScrollingEnabled);

    m_variableElementSize[ElementType::Item] = m_variableItemElementSize;
    m_autoCalculateElementSize[ElementType::Item] = m_variableItemElementSize ? m_autoCalculateItemElementSize : false;
    m_estimatedElementSize[ElementType::Item] = m_variableItemElementSize ? m_estimatedItemElementSize : 0.0f;
    m_variableElementSize[ElementType::SectionHeader] = m_hasSections ? m_variableHeaderElementSize : false;
    m_autoCalculateElementSize[ElementType::SectionHeader] = (m_hasSections && m_variableHeaderElementSize) ? m_autoCalculateHeaderElementSize : false;
    m_estimatedElementSize[ElementType::SectionHeader] = (m_hasSections && m_variableHeaderElementSize) ? m_estimatedHeaderElementSize : 0.0f;

    for (int i = 0; i < ElementType::NumElementTypes; i++)
    {
        m_prototypeElement[i].SetInvalid();
    }

    // Find the content element
    AZ::EntityId contentEntityId;
    UiScrollBoxBus::EventResult(contentEntityId, GetEntityId(), &UiScrollBoxBus::Events::GetContentEntity);

    int numChildren = 0;
    UiElementBus::EventResult(numChildren, contentEntityId, &UiElementBus::Events::GetNumChildElements);

    // Make sure the item prototype element isn't pointing to itself (the dynamic scroll box) or an ancestor,
    // otherwise this scroll box will spawn scroll boxes recursively ad infinitum.
    if (IsValidPrototype(m_itemPrototypeElement))
    {
        m_prototypeElement[ElementType::Item] = m_itemPrototypeElement;
    }
    else
    {
        if (m_itemPrototypeElement.IsValid())
        {
            AZ_Warning("UiDynamicScrollBoxComponent", false,
                "The prototype element is not safe for cloning. "
                "This scroll box's prototype element contains the scroll box itself which can result in recursively spawning scroll boxes. "
                "Please change the prototype element to a nonancestral entity.");
        }

        // Find the prototype element as the first child of the content element
        if (numChildren > 0)
        {
            AZ::EntityId prototypeEntityId;
            UiElementBus::EventResult(prototypeEntityId, contentEntityId, &UiElementBus::Events::GetChildEntityId, 0);
            m_prototypeElement[ElementType::Item] = prototypeEntityId;
        }
    }

    if (m_hasSections)
    {
        if (IsValidPrototype(m_headerPrototypeElement))
        {
            // Prototype header element is defined in properties
            m_prototypeElement[ElementType::SectionHeader] = m_headerPrototypeElement;
        }
        else if(m_headerPrototypeElement.IsValid())
        {
            AZ_Warning("UiDynamicScrollBoxComponent", false,
                "The selected prototype header is not safe for cloning. "
                "This scroll box's prototype header contains the scroll box itself which can result in recursively spawning scroll boxes. "
                "Please change the header to a nonancestral entity.");
        }
    }

    for (int i = 0; i < ElementType::NumElementTypes; i++)
    {
        m_isPrototypeElementNavigable[i] = false;
        m_prototypeElementSize[i] = 0.0f;

        if (m_prototypeElement[i].IsValid())
        {
            m_isPrototypeElementNavigable[i] = UiNavigationHelpers::IsElementInteractableAndNavigable(m_prototypeElement[i]);

            // Store the size of the item prototype element for future content element size calculations
            AZ::Vector2 prototypeElementSize(0.0f, 0.0f);
            UiTransformBus::EventResult(
                prototypeElementSize, m_prototypeElement[i], &UiTransformBus::Events::GetCanvasSpaceSizeNoScaleRotate);

            m_prototypeElementSize[i] = (m_isVertical ? prototypeElementSize.GetY() : prototypeElementSize.GetX());

            // Set anchors to top or left
            SetElementAnchors(m_prototypeElement[i]);
        }
    }

    AZ::Entity* contentEntity = GetContentEntity();
    if (contentEntity)
    {
        // Get the content entity's element component
        UiElementComponent* elementComponent = contentEntity->FindComponent<UiElementComponent>();
        AZ_Assert(elementComponent, "entity has no UiElementComponent");

        if (elementComponent)
        {
            // Remove any extra elements
            for (int i = numChildren - 1; i >= 0; i--)
            {
                AZ::EntityId entityId;
                UiElementBus::EventResult(entityId, contentEntityId, &UiElementBus::Events::GetChildEntityId, i);

                // Remove the child element
                elementComponent->RemoveChild(entityId);

                if (!IsPrototypeElement(entityId))
                {
                    UiElementBus::Event(entityId, &UiElementBus::Events::DestroyElement);
                }
            }
        }

        // Get the content's parent
        AZ::EntityId contentParentEntityId;
        UiElementBus::EventResult(contentParentEntityId, contentEntityId, &UiElementBus::Events::GetParentEntityId);

        // Create an entity that will be used as the sticky header
        m_currentStickyHeader.m_elementIndex = -1;
        m_currentStickyHeader.m_indexInfo.m_sectionIndex = -1;
        m_currentStickyHeader.m_indexInfo.m_itemIndexInSection = -1;
        m_currentStickyHeader.m_type = ElementType::SectionHeader;
        if (m_hasSections && m_stickyHeaders && contentParentEntityId.IsValid())
        {
            m_currentStickyHeader.m_element = ClonePrototypeElement(ElementType::SectionHeader, contentParentEntityId);
            UiElementBus::Event(m_currentStickyHeader.m_element, &UiElementBus::Events::SetIsEnabled, false);
        }

        // Listen for canvas space rect changes of the content's parent
        if (contentParentEntityId.IsValid())
        {
            UiTransformChangeNotificationBus::Handler::BusConnect(contentParentEntityId);
        }

        // Listen to scrollbox scrolling events
        UiScrollBoxNotificationBus::Handler::BusConnect(GetEntityId());
    }

    m_listPreparedForDisplay = true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZ::Entity* UiDynamicScrollBoxComponent::GetContentEntity() const
{
    AZ::Entity* contentEntity = nullptr;

    // Find the content element
    AZ::EntityId contentEntityId;
    UiScrollBoxBus::EventResult(contentEntityId, GetEntityId(), &UiScrollBoxBus::Events::GetContentEntity);

    if (contentEntityId.IsValid())
    {
        AZ::ComponentApplicationBus::BroadcastResult(contentEntity, &AZ::ComponentApplicationBus::Events::FindEntity, contentEntityId);
    }

    return contentEntity;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZ::EntityId UiDynamicScrollBoxComponent::ClonePrototypeElement(ElementType elementType, AZ::EntityId parentEntityId) const
{
    AZ::EntityId element;

    // Clone the prototype element and add it as a child of the specified parent (defaults to content entity)
    AZ::Entity* prototypeEntity = nullptr;
    AZ::ComponentApplicationBus::BroadcastResult(
        prototypeEntity, &AZ::ComponentApplicationBus::Events::FindEntity, m_prototypeElement[elementType]);
    if (prototypeEntity)
    {
        if (!parentEntityId.IsValid())
        {
            AZ::EntityId contentEntityId;
            UiScrollBoxBus::EventResult(contentEntityId, GetEntityId(), &UiScrollBoxBus::Events::GetContentEntity);

            parentEntityId = contentEntityId;
        }

        // Find the parent entity
        AZ::Entity* parentEntity = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(parentEntity, &AZ::ComponentApplicationBus::Events::FindEntity, parentEntityId);

        if (parentEntity)
        {
            AZ::EntityId canvasEntityId;
            UiElementBus::EventResult(canvasEntityId, GetEntityId(), &UiElementBus::Events::GetCanvasEntityId);

            AZ::Entity* clonedElement = nullptr;
            UiCanvasBus::EventResult(clonedElement, canvasEntityId, &UiCanvasBus::Events::CloneElement, prototypeEntity, parentEntity);

            if (clonedElement)
            {
                element = clonedElement->GetId();
            }
        }
    }

    return element;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiDynamicScrollBoxComponent::IsPrototypeElement(AZ::EntityId entityId) const
{
    for (int i = 0; i < ElementType::NumElementTypes; i++)
    {
        if (m_prototypeElement[i] == entityId)
        {
            return true;
        }
    }

    return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiDynamicScrollBoxComponent::AllPrototypeElementsValid() const
{
    return (m_prototypeElement[ElementType::Item].IsValid() && (!m_hasSections || m_prototypeElement[ElementType::SectionHeader].IsValid()));
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiDynamicScrollBoxComponent::AnyPrototypeElementsNavigable() const
{
    return (m_isPrototypeElementNavigable[ElementType::Item] || (m_hasSections && m_isPrototypeElementNavigable[ElementType::SectionHeader]));
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiDynamicScrollBoxComponent::AnyElementTypesHaveVariableSize() const
{
    return (m_variableElementSize[ElementType::Item] || (m_hasSections && m_variableElementSize[ElementType::SectionHeader]));
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiDynamicScrollBoxComponent::AnyElementTypesHaveEstimatedSizes() const
{
    return (m_estimatedElementSize[ElementType::Item] > 0.0f || (m_hasSections && m_estimatedElementSize[ElementType::SectionHeader] > 0.0f));
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiDynamicScrollBoxComponent::AllElementTypesHaveEstimatedSizes() const
{
    return (m_estimatedElementSize[ElementType::Item] > 0.0f && (!m_hasSections || m_estimatedElementSize[ElementType::SectionHeader] > 0.0f));
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiDynamicScrollBoxComponent::StickyHeadersEnabled() const
{
    return (m_hasSections && m_stickyHeaders && m_currentStickyHeader.m_element.IsValid());
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiDynamicScrollBoxComponent::ResizeContentToFitElements()
{
    if (!AllPrototypeElementsValid())
    {
        return;
    }

    // Get the number of elements in the list
    if (!m_hasSections)
    {
        m_sections.clear();

        m_numElements = m_defaultNumElements;
        UiDynamicScrollBoxDataBus::EventResult(m_numElements, GetEntityId(), &UiDynamicScrollBoxDataBus::Events::GetNumElements);
    }
    else
    {
        int numSections = m_defaultNumSections;
        UiDynamicScrollBoxDataBus::EventResult(numSections, GetEntityId(), &UiDynamicScrollBoxDataBus::Events::GetNumSections);
        numSections = AZ::GetMax(numSections, 1);

        m_sections.clear();
        m_sections.reserve(numSections);
        m_numElements = 0;
        for (int i = 0; i < numSections; i++)
        {
            int numItems = m_defaultNumElements;
            UiDynamicScrollBoxDataBus::EventResult(numItems, GetEntityId(), &UiDynamicScrollBoxDataBus::Events::GetNumElementsInSection, i);

            Section section;
            section.m_index = i;
            section.m_numItems = numItems;
            section.m_headerElementIndex = m_numElements;
            m_sections.push_back(section);

            m_numElements += 1 + section.m_numItems;
        }
    }

    // Calculate new content size
    float newSize = 0.0f;
    if (!AnyElementTypesHaveVariableSize())
    {
        if (!m_hasSections)
        {
            newSize = m_numElements * m_prototypeElementSize[ElementType::Item];
        }
        else
        {
            int numHeaders = static_cast<int>(m_sections.size());
            int numItems = m_numElements - numHeaders;
            newSize = numHeaders * m_prototypeElementSize[ElementType::SectionHeader] + numItems * m_prototypeElementSize[ElementType::Item];
        }
    }
    else
    {
        // Some element types have variable element sizes

        // Reset cached element info
        m_cachedElementInfo.clear();
        m_cachedElementInfo.reserve(m_numElements);
        m_cachedElementInfo.insert(m_cachedElementInfo.end(), m_numElements, CachedElementInfo());

        if (AllElementTypesHaveEstimatedSizes())
        {
            if (!m_hasSections)
            {
                newSize = m_numElements * m_estimatedElementSize[ElementType::Item];
            }
            else
            {
                int numHeaders = static_cast<int>(m_sections.size());
                int numItems = m_numElements - numHeaders;
                newSize = numHeaders * m_estimatedElementSize[ElementType::SectionHeader] + numItems * m_estimatedElementSize[ElementType::Item];
            }
        }
        else
        {
            for (int i = 0; i < m_numElements; i++)
            {
                newSize += GetAndCacheVariableElementSize(i);
            }

            DisableElementsForAutoSizeCalculation();
        }

        m_averageElementSize = 0.0f;
        m_numElementsUsedForAverage = 0;

        UpdateAverageElementSize(m_numElements, newSize);
    }

    // Resize content element
    ResizeContentElement(newSize);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiDynamicScrollBoxComponent::ResizeContentElement(float newSize) const
{
    // Find the content element
    AZ::EntityId contentEntityId;
    UiScrollBoxBus::EventResult(contentEntityId, GetEntityId(), &UiScrollBoxBus::Events::GetContentEntity);

    if (!contentEntityId.IsValid())
    {
        return;
    }

    // Get current content size
    AZ::Vector2 curContentSize(0.0f, 0.0f);
    UiTransformBus::EventResult(curContentSize, contentEntityId, &UiTransformBus::Events::GetCanvasSpaceSizeNoScaleRotate);

    float curSize = m_isVertical ? curContentSize.GetY() : curContentSize.GetX();

    if (newSize != curSize)
    {
        // Resize content element

        UiTransform2dInterface::Offsets offsets;
        UiTransform2dBus::EventResult(offsets, contentEntityId, &UiTransform2dBus::Events::GetOffsets);

        AZ::Vector2 pivot;
        UiTransformBus::EventResult(pivot, contentEntityId, &UiTransformBus::Events::GetPivot);

        float sizeDiff = newSize - curSize;

        if (m_isVertical)
        {
            offsets.m_top -= sizeDiff * pivot.GetY();
            offsets.m_bottom += sizeDiff * (1.0f - pivot.GetY());
        }
        else
        {
            offsets.m_left -= sizeDiff * pivot.GetX();
            offsets.m_right += sizeDiff * (1.0f - pivot.GetX());
        }

        UiTransform2dBus::Event(contentEntityId, &UiTransform2dBus::Events::SetOffsets, offsets);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiDynamicScrollBoxComponent::AdjustContentSizeAndScrollOffsetByDelta(float sizeDelta, float scrollDelta) const
{
    // Find the content element
    AZ::EntityId contentEntityId;
    UiScrollBoxBus::EventResult(contentEntityId, GetEntityId(), &UiScrollBoxBus::Events::GetContentEntity);

    if (!contentEntityId.IsValid())
    {
        return;
    }

    // Get content size
    AZ::Vector2 contentSize(0.0f, 0.0f);
    UiTransformBus::EventResult(contentSize, contentEntityId, &UiTransformBus::Events::GetCanvasSpaceSizeNoScaleRotate);

    if (sizeDelta != 0.0f)
    {
        if (m_isVertical)
        {
            contentSize.SetY(contentSize.GetY() + sizeDelta);
        }
        else
        {
            contentSize.SetX(contentSize.GetX() + sizeDelta);
        }
    }

    // Get scroll offset
    AZ::Vector2 scrollOffset(0.0f, 0.0f);
    UiScrollBoxBus::EventResult(scrollOffset, GetEntityId(), &UiScrollBoxBus::Events::GetScrollOffset);

    if (scrollDelta != 0.0f)
    {
        if (m_isVertical)
        {
            scrollOffset.SetY(scrollOffset.GetY() + scrollDelta);
        }
        else
        {
            scrollOffset.SetX(scrollOffset.GetX() + scrollDelta);
        }
    }

    UiScrollBoxBus::Event(GetEntityId(), &UiScrollBoxBus::Events::ChangeContentSizeAndScrollOffset, contentSize, scrollOffset);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
float UiDynamicScrollBoxComponent::CalculateVariableElementSize(int index)
{
    float size = 0.0f;

    AZ_Assert(index >= 0 && index < m_numElements, "index %d out of range", index);

    if (index < 0 || index >= m_numElements)
    {
        return size;
    }

    ElementType elementType = GetElementTypeAtIndex(index);

    if (!m_autoCalculateElementSize[elementType])
    {
        if (m_isVertical)
        {
            if (!m_hasSections)
            {
                UiDynamicScrollBoxDataBus::EventResult(size, GetEntityId(), &UiDynamicScrollBoxDataBus::Events::GetElementHeight, index);
            }
            else
            {
                ElementIndexInfo elementIndexInfo = GetElementIndexInfoFromIndex(index);
                if (elementType == ElementType::Item)
                {
                    UiDynamicScrollBoxDataBus::EventResult(
                        size,
                        GetEntityId(),
                        &UiDynamicScrollBoxDataBus::Events::GetElementInSectionHeight,
                        elementIndexInfo.m_sectionIndex,
                        elementIndexInfo.m_itemIndexInSection);
                }
                else if (elementType == ElementType::SectionHeader)
                {
                    UiDynamicScrollBoxDataBus::EventResult(
                        size, GetEntityId(), &UiDynamicScrollBoxDataBus::Events::GetSectionHeaderHeight, elementIndexInfo.m_sectionIndex);
                }
                else
                {
                    AZ_Assert(false, "unknown element type");
                }
            }
        }
        else
        {
            if (!m_hasSections)
            {
                UiDynamicScrollBoxDataBus::EventResult(size, GetEntityId(), &UiDynamicScrollBoxDataBus::Events::GetElementWidth, index);
            }
            else
            {
                ElementIndexInfo elementIndexInfo = GetElementIndexInfoFromIndex(index);
                if (elementType == ElementType::Item)
                {
                    UiDynamicScrollBoxDataBus::EventResult(
                        size,
                        GetEntityId(),
                        &UiDynamicScrollBoxDataBus::Events::GetElementInSectionWidth,
                        elementIndexInfo.m_sectionIndex,
                        elementIndexInfo.m_itemIndexInSection);
                }
                else if (elementType == ElementType::SectionHeader)
                {
                    UiDynamicScrollBoxDataBus::EventResult(
                        size, GetEntityId(), &UiDynamicScrollBoxDataBus::Events::GetSectionHeaderWidth, elementIndexInfo.m_sectionIndex);
                }
                else
                {
                    AZ_Assert(false, "unknown element type");
                }
            }
        }
    }
    else
    {
        AZ::EntityId elementForAutoSizeCalculation = GetElementForAutoSizeCalculation(elementType);

        // Auto calculate the size of the element
        AZ_Assert(elementForAutoSizeCalculation.IsValid(), "elementForAutoSizeCalculation is invalid");

        // Notify listeners to setup this element for auto calculation
        if (!m_hasSections)
        {
            UiDynamicScrollBoxElementNotificationBus::Event(
                GetEntityId(),
                &UiDynamicScrollBoxElementNotificationBus::Events::OnPrepareElementForSizeCalculation,
                elementForAutoSizeCalculation,
                index);
        }
        else
        {
            ElementIndexInfo elementIndexInfo = GetElementIndexInfoFromIndex(index);
            if (elementType == ElementType::Item)
            {
                UiDynamicScrollBoxElementNotificationBus::Event(
                    GetEntityId(),
                    &UiDynamicScrollBoxElementNotificationBus::Events::OnPrepareElementInSectionForSizeCalculation,
                    elementForAutoSizeCalculation,
                    elementIndexInfo.m_sectionIndex,
                    elementIndexInfo.m_itemIndexInSection);
            }
            else if (elementType == ElementType::SectionHeader)
            {
                UiDynamicScrollBoxElementNotificationBus::Event(
                    GetEntityId(),
                    &UiDynamicScrollBoxElementNotificationBus::Events::OnPrepareSectionHeaderForSizeCalculation,
                    elementForAutoSizeCalculation,
                    elementIndexInfo.m_sectionIndex);
            }
            else
            {
                AZ_Assert(false, "unknown element type");
            }
        }
        size = AutoCalculateElementSize(elementForAutoSizeCalculation);
    }

    // Cache the calculated size
    m_cachedElementInfo[index].m_size = size;

    return size;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
float UiDynamicScrollBoxComponent::GetAndCacheVariableElementSize(int index)
{
    float size = 0.0f;

    AZ_Assert(index >= 0 && index < m_numElements, "index %d out of range", index);

    if (index < 0 || index >= m_numElements)
    {
        return size;
    }

    if (m_cachedElementInfo[index].m_size >= 0.0f)
    {
        // Use the cached size
        size = m_cachedElementInfo[index].m_size;
    }
    else
    {
        ElementType elementType = GetElementTypeAtIndex(index);

        if (!m_variableElementSize[elementType])
        {
            // Use the prototype element size
            size = m_prototypeElementSize[elementType];

            // Cache the calculated size
            m_cachedElementInfo[index].m_size = size;

            // Cache the accumulated size
            m_cachedElementInfo[index].m_accumulatedSize = GetVariableSizeElementOffset(index) + size;
        }
        else if (m_estimatedElementSize[elementType] > 0.0f)
        {
            // Uses the estimated element size
            size = m_estimatedElementSize[elementType];
        }
        else
        {
            size = CalculateVariableElementSize(index);

            // Cache the accumulated size
            m_cachedElementInfo[index].m_accumulatedSize = GetVariableSizeElementOffset(index) + size;
        }
    }

    return size;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
float UiDynamicScrollBoxComponent::GetVariableElementSize(int index) const
{
    float size = 0.0f;

    AZ_Assert(index >= 0 && index < m_numElements, "index %d out of range", index);

    if (index < 0 || index >= m_numElements)
    {
        return size;
    }

    if (m_cachedElementInfo[index].m_size >= 0.0f)
    {
        // Use the cached size
        size = m_cachedElementInfo[index].m_size;
    }
    else
    {
        ElementType elementType = GetElementTypeAtIndex(index);

        if (m_estimatedElementSize[elementType] > 0.0f)
        {
            // Uses the estimated element size
            size = m_estimatedElementSize[elementType];
        }
        else
        {
            AZ_Assert(false, "GetVariableElementSize is being called before size is known");
        }
    }

    return size;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
int UiDynamicScrollBoxComponent::GetLastKnownAccumulatedSizeIndex(int index, int numElementsWithUnknownSizeOut[ElementType::NumElementTypes]) const
{
    for (int i = 0; i < ElementType::NumElementTypes; i++)
    {
        numElementsWithUnknownSizeOut[i] = 0;
    }

    for (int i = index - 1; i >= 0; i--)
    {
        if (m_cachedElementInfo[i].m_accumulatedSize >= 0.0f)
        {
            return i;
        }

        ElementType elementType = GetElementTypeAtIndex(i);
        ++numElementsWithUnknownSizeOut[elementType];
    }

    return -1;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
float UiDynamicScrollBoxComponent::GetElementOffsetAtIndex(int index) const
{
    float offset = 0.0f;

    if (!AnyElementTypesHaveVariableSize())
    {
        offset = GetFixedSizeElementOffset(index);
    }
    else
    {
        offset = GetVariableSizeElementOffset(index);
    }

    return offset;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
float UiDynamicScrollBoxComponent::GetFixedSizeElementOffset(int index) const
{
    float offset = 0.0f;

    if (!m_hasSections)
    {
        offset = m_prototypeElementSize[ElementType::Item] * index;
    }
    else
    {
        int numHeaders = 0;
        int numItems = 0;

        int numSections = static_cast<int>(m_sections.size());
        if (numSections > 0)
        {
            if (index > m_sections[numSections - 1].m_headerElementIndex)
            {
                numHeaders = numSections;
            }
            else
            {
                for (int i = 0; i < numSections; i++)
                {
                    if (index <= m_sections[i].m_headerElementIndex)
                    {
                        numHeaders = i;
                        break;
                    }
                }
            }

            numItems = index - numHeaders;
        }

        offset = numHeaders * m_prototypeElementSize[ElementType::SectionHeader] + numItems * m_prototypeElementSize[ElementType::Item];
    }

    return offset;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
float UiDynamicScrollBoxComponent::GetVariableSizeElementOffset(int index) const
{
    float offset = 0.0f;

    AZ_Assert(index >= 0 && index < m_numElements, "index %d out of range", index);

    if (index < 0 || index >= m_numElements)
    {
        return offset;
    }

    if (index > 0)
    {
        if (m_cachedElementInfo[index - 1].m_accumulatedSize >= 0.0f)
        {
            offset = m_cachedElementInfo[index - 1].m_accumulatedSize;
        }
        else
        {
            // Calculate the accumulated size
            int numElementsWithUnknownSizeOut[ElementType::NumElementTypes];
            int lastKnownIndex = GetLastKnownAccumulatedSizeIndex(index, numElementsWithUnknownSizeOut);

            offset = lastKnownIndex >= 0 ? m_cachedElementInfo[lastKnownIndex].m_accumulatedSize : 0.0f;
            for (int i = 0; i < ElementType::NumElementTypes; i++)
            {
                offset += numElementsWithUnknownSizeOut[i] * m_estimatedElementSize[i];
            }
        }
    }

    return offset;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiDynamicScrollBoxComponent::UpdateAverageElementSize(int numAddedElements, float sizeDelta)
{
    float curTotalSize = m_averageElementSize * m_numElementsUsedForAverage;

    m_numElementsUsedForAverage += numAddedElements;
    m_averageElementSize = (m_numElementsUsedForAverage > 0) ? (AZ::GetMax(curTotalSize + sizeDelta, 0.0f) / m_numElementsUsedForAverage) : 0.0f;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiDynamicScrollBoxComponent::ClearDisplayedElements()
{
    for (const auto& e : m_displayedElements)
    {
        ElementType elementType = e.m_type;
        m_recycledElements[elementType].push_front(e.m_element);

        // Disable element
        UiElementBus::Event(e.m_element, &UiElementBus::Events::SetIsEnabled, false);
    }

    m_displayedElements.clear();

    m_firstDisplayedElementIndex = -1;
    m_lastDisplayedElementIndex = -1;
    m_firstVisibleElementIndex = -1;
    m_lastVisibleElementIndex = -1;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZ::EntityId UiDynamicScrollBoxComponent::FindDisplayedElementWithIndex(int index) const
{
    AZ::EntityId elementId;

    for (const auto& e : m_displayedElements)
    {
        if (e.m_elementIndex == index)
        {
            elementId = e.m_element;
            break;
        }
    }

    return elementId;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
float UiDynamicScrollBoxComponent::GetVisibleAreaSize() const
{
    float visibleAreaSize = 0.0f;

    // Find the content element
    AZ::EntityId contentEntityId;
    UiScrollBoxBus::EventResult(contentEntityId, GetEntityId(), &UiScrollBoxBus::Events::GetContentEntity);

    if (!contentEntityId.IsValid())
    {
        return visibleAreaSize;
    }

    // Get content's parent
    AZ::EntityId contentParentEntityId;
    UiElementBus::EventResult(contentParentEntityId, contentEntityId, &UiElementBus::Events::GetParentEntityId);
    if (!contentParentEntityId.IsValid())
    {
        return visibleAreaSize;
    }

    // Get content parent's size in canvas space
    AZ::Vector2 contentParentSize(0.0f, 0.0f);
    UiTransformBus::EventResult(contentParentSize, contentParentEntityId, &UiTransformBus::Events::GetCanvasSpaceSizeNoScaleRotate);

    if (m_isVertical)
    {
        visibleAreaSize = contentParentSize.GetY();
    }
    else
    {
        visibleAreaSize = contentParentSize.GetX();
    }

    return visibleAreaSize;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiDynamicScrollBoxComponent::AreAnyElementsVisible(AZ::Vector2& visibleContentBoundsOut) const
{
    if (m_numElements == 0)
    {
        return false;
    }

    // Find the content element
    AZ::EntityId contentEntityId;
    UiScrollBoxBus::EventResult(contentEntityId, GetEntityId(), &UiScrollBoxBus::Events::GetContentEntity);

    if (!contentEntityId.IsValid())
    {
        return false;
    }

    // Get content's parent
    AZ::EntityId contentParentEntityId;
    UiElementBus::EventResult(contentParentEntityId, contentEntityId, &UiElementBus::Events::GetParentEntityId);
    if (!contentParentEntityId.IsValid())
    {
        return false;
    }

    // Get content's rect in canvas space
    UiTransformInterface::Rect contentRect;
    UiTransformBus::Event(contentEntityId, &UiTransformBus::Events::GetCanvasSpaceRectNoScaleRotate, contentRect);

    // Get content parent's rect in canvas space
    UiTransformInterface::Rect parentRect;
    UiTransformBus::Event(contentParentEntityId, &UiTransformBus::Events::GetCanvasSpaceRectNoScaleRotate, parentRect);

    // Check if any items are visible
    AZ::Vector2 minA(contentRect.left, contentRect.top);
    AZ::Vector2 maxA(contentRect.right, contentRect.bottom);
    AZ::Vector2 minB(parentRect.left, parentRect.top);
    AZ::Vector2 maxB(parentRect.right, parentRect.bottom);

    bool boxesIntersect = true;

    if (maxA.GetX() < minB.GetX() || // a is left of b
        minA.GetX() > maxB.GetX() || // a is right of b
        maxA.GetY() < minB.GetY() || // a is above b
        minA.GetY() > maxB.GetY())   // a is below b
    {
        boxesIntersect = false;   // no overlap
    }

    if (boxesIntersect)
    {
        // Set visible content bounds
        if (m_isVertical)
        {
            // Set top offset
            visibleContentBoundsOut.SetX(AZ::GetMax(parentRect.top - contentRect.top, 0.0f));

            // Set bottom offset
            visibleContentBoundsOut.SetY(AZ::GetMin(parentRect.bottom, contentRect.bottom) - contentRect.top);
        }
        else
        {
            // Set left offset
            visibleContentBoundsOut.SetX(AZ::GetMax(parentRect.left - contentRect.left, 0.0f));

            // Set right offset
            visibleContentBoundsOut.SetY(AZ::GetMin(parentRect.right, contentRect.right) - contentRect.left);
        }
    }

    return boxesIntersect;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiDynamicScrollBoxComponent::UpdateElementVisibility(bool keepAtEndIfWasAtEnd)
{
    // Calculate which elements are visible
    int firstVisibleElementIndex = -1;
    int lastVisibleElementIndex = -1;
    int firstDisplayedElementIndex = -1;
    int lastDisplayedElementIndex = -1;
    int firstDisplayedElementIndexWithSizeChange = -1;
    float totalElementSizeChange = 0.0f;
    float scrollChange = 0.0f;

    AZ::Vector2 visibleContentBounds(0.0f, 0.0f);
    bool elementsVisible = AreAnyElementsVisible(visibleContentBounds);

    if (elementsVisible)
    {
        CalculateVisibleElementIndices(keepAtEndIfWasAtEnd,
            visibleContentBounds,
            firstVisibleElementIndex,
            lastVisibleElementIndex,
            firstDisplayedElementIndex,
            lastDisplayedElementIndex,
            firstDisplayedElementIndexWithSizeChange,
            totalElementSizeChange,
            scrollChange);
    }

    m_lastCalculatedVisibleContentOffset = visibleContentBounds.GetX();
    if (totalElementSizeChange != 0.0f)
    {
        m_lastCalculatedVisibleContentOffset += CalculateContentBeginningDeltaAfterSizeChange(totalElementSizeChange);
    }

    if (StickyHeadersEnabled())
    {
        UpdateStickyHeader(firstVisibleElementIndex, lastVisibleElementIndex, m_lastCalculatedVisibleContentOffset);
    }

    // Remove the elements that are no longer being displayed
    m_displayedElements.remove_if(
        [this, firstDisplayedElementIndex, lastDisplayedElementIndex](const DisplayedElement& e)
    {
        if ((firstDisplayedElementIndex < 0) || (e.m_elementIndex < firstDisplayedElementIndex) || (e.m_elementIndex > lastDisplayedElementIndex))
        {
            // This element is no longer being displayed, move it to the recycled elements list
            m_recycledElements[e.m_type].push_front(e.m_element);

            // Disable element
            UiElementBus::Event(e.m_element, &UiElementBus::Events::SetIsEnabled, false);

            // Remove element from the displayed element list
            return true;
        }
        else
        {
            return false;
        }
    }
    );

    // Add the newly displayed elements
    if (firstDisplayedElementIndex >= 0)
    {
        for (int i = firstDisplayedElementIndex; i <= lastDisplayedElementIndex; i++)
        {
            if (!IsElementDisplayedAtIndex(i))
            {
                ElementType elementType = GetElementTypeAtIndex(i);
                ElementIndexInfo elementIndexInfo = GetElementIndexInfoFromIndex(i);

                AZ::EntityId element = GetElementForDisplay(elementType);
                DisplayedElement elementEntry;
                elementEntry.m_element = element;
                elementEntry.m_elementIndex = i;
                elementEntry.m_indexInfo = elementIndexInfo;
                elementEntry.m_type = elementType;

                m_displayedElements.push_front(elementEntry);

                if (m_variableElementSize[elementType])
                {
                    SizeVariableElementAtIndex(element, i);
                }

                PositionElementAtIndex(element, i);

                // Notify listeners that this element is about to be displayed
                if (!m_hasSections)
                {
                    UiDynamicScrollBoxElementNotificationBus::Event(
                        GetEntityId(), &UiDynamicScrollBoxElementNotificationBus::Events::OnElementBecomingVisible, element, i);
                }
                else
                {
                    if (elementType == ElementType::Item)
                    {
                        UiDynamicScrollBoxElementNotificationBus::Event(
                            GetEntityId(),
                            &UiDynamicScrollBoxElementNotificationBus::Events::OnElementInSectionBecomingVisible,
                            element,
                            elementIndexInfo.m_sectionIndex,
                            elementIndexInfo.m_itemIndexInSection);
                    }
                    else if (elementType == ElementType::SectionHeader)
                    {
                        UiDynamicScrollBoxElementNotificationBus::Event(
                            GetEntityId(),
                            &UiDynamicScrollBoxElementNotificationBus::Events::OnSectionHeaderBecomingVisible,
                            element,
                            elementIndexInfo.m_sectionIndex);
                    }
                    else
                    {
                        AZ_Assert(false, "unknown element type");
                    }
                }
            }
            else
            {
                if (firstDisplayedElementIndexWithSizeChange >= 0 && firstDisplayedElementIndexWithSizeChange <= i)
                {
                    AZ::EntityId element = FindDisplayedElementWithIndex(i);
                    PositionElementAtIndex(element, i);
                }
            }
        }
    }

    m_firstVisibleElementIndex = firstVisibleElementIndex;
    m_lastVisibleElementIndex = lastVisibleElementIndex;
    m_firstDisplayedElementIndex = firstDisplayedElementIndex;
    m_lastDisplayedElementIndex = lastDisplayedElementIndex;

    if (totalElementSizeChange != 0.0f || scrollChange != 0.0f)
    {
        AdjustContentSizeAndScrollOffsetByDelta(totalElementSizeChange, scrollChange);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiDynamicScrollBoxComponent::CalculateVisibleElementIndices(bool keepAtEndIfWasAtEnd,
    const AZ::Vector2& visibleContentBounds,
    int& firstVisibleElementIndexOut,
    int& lastVisibleElementIndexOut,
    int& firstDisplayedElementIndexOut,
    int& lastDisplayedElementIndexOut,
    int& firstDisplayedElementIndexWithSizeChangeOut,
    float& totalElementSizeChangeOut,
    float& scrollChangeOut)
{
    firstVisibleElementIndexOut = -1;
    lastVisibleElementIndexOut = -1;
    firstDisplayedElementIndexOut = -1;
    lastDisplayedElementIndexOut = -1;
    firstDisplayedElementIndexWithSizeChangeOut = -1;
    totalElementSizeChangeOut = 0.0f;
    scrollChangeOut = 0.0f;

    if (!AllPrototypeElementsValid())
    {
        return;
    }

    bool addedExtraElementsForNavigation = false;

    if (!AnyElementTypesHaveVariableSize())
    {
        // All elements are the same size
        FindVisibleElementIndicesForFixedSizes(visibleContentBounds, firstVisibleElementIndexOut, lastVisibleElementIndexOut);
    }
    else
    {
        // Elements vary in size

        if (AnyElementTypesHaveEstimatedSizes())
        {
            // We may not have the real sizes of all the elements yet

            // Find the first elment index that's visible and that will remain in the same position
            int visibleElementIndex = -1;

            bool keepAtEnd = keepAtEndIfWasAtEnd && IsScrolledToEnd();
            if (keepAtEnd)
            {
                visibleElementIndex = m_numElements - 1;
            }
            else
            {
                visibleElementIndex = FindVisibleElementIndexToRemainInPlace(visibleContentBounds);
            }

            // Calculate the first and last visible elements without moving the beginning (top or left) of the specified visible element index
            CalculateVisibleElementIndicesFromVisibleElementIndex(visibleElementIndex,
                visibleContentBounds,
                keepAtEnd,
                firstVisibleElementIndexOut,
                lastVisibleElementIndexOut,
                firstDisplayedElementIndexOut,
                lastDisplayedElementIndexOut,
                firstDisplayedElementIndexWithSizeChangeOut,
                totalElementSizeChangeOut,
                scrollChangeOut);

            addedExtraElementsForNavigation = true;
        }
        else
        {
            // We have the real sizes of all the elements

            // Estimate a first visible element index
            int estimatedFirstVisibleElementIndex = EstimateFirstVisibleElementIndex(visibleContentBounds);

            // Look for the real new first visible element index
            float curElementEnd = 0.0f;
            firstVisibleElementIndexOut = FindFirstVisibleElementIndex(estimatedFirstVisibleElementIndex, visibleContentBounds, curElementEnd);

            // Now find the last visible element index
            lastVisibleElementIndexOut = firstVisibleElementIndexOut;
            while (curElementEnd < visibleContentBounds.GetY() && lastVisibleElementIndexOut < m_numElements - 1)
            {
                ++lastVisibleElementIndexOut;
                curElementEnd += GetVariableElementSize(lastVisibleElementIndexOut);
            }
        }
    }

    if (!addedExtraElementsForNavigation)
    {
        firstDisplayedElementIndexOut = firstVisibleElementIndexOut;
        lastDisplayedElementIndexOut = lastVisibleElementIndexOut;

        AddExtraElementsForNavigation(firstDisplayedElementIndexOut, lastDisplayedElementIndexOut);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiDynamicScrollBoxComponent::UpdateStickyHeader(int firstVisibleElementIndex, int lastVisibleElementIndex, float visibleContentBeginning)
{
    // Find which header should currently be sticky
    if (firstVisibleElementIndex >= 0)
    {
        ElementIndexInfo firstVisibleElementIndexInfo = GetElementIndexInfoFromIndex(firstVisibleElementIndex);
        int newStickyHeaderElementIndex = m_sections[firstVisibleElementIndexInfo.m_sectionIndex].m_headerElementIndex;
        if (newStickyHeaderElementIndex != m_currentStickyHeader.m_elementIndex)
        {
            if (m_currentStickyHeader.m_elementIndex < 0)
            {
                UiElementBus::Event(m_currentStickyHeader.m_element, &UiElementBus::Events::SetIsEnabled, true);
            }

            m_currentStickyHeader.m_elementIndex = newStickyHeaderElementIndex;
            m_currentStickyHeader.m_indexInfo.m_sectionIndex = firstVisibleElementIndexInfo.m_sectionIndex;

            if (m_variableElementSize[ElementType::SectionHeader])
            {
                SizeVariableElementAtIndex(m_currentStickyHeader.m_element, m_currentStickyHeader.m_elementIndex);
            }

            UiDynamicScrollBoxElementNotificationBus::Event(
                GetEntityId(),
                &UiDynamicScrollBoxElementNotificationBus::Events::OnSectionHeaderBecomingVisible,
                m_currentStickyHeader.m_element,
                m_currentStickyHeader.m_indexInfo.m_sectionIndex);
        }

        float stickyHeaderOffset = 0.0f;

        // Check if the current sticky header is being pushed out of the way by another visible header
        int firstVisibleHeaderIndex = FindFirstVisibleHeaderIndex(firstVisibleElementIndex, lastVisibleElementIndex, m_currentStickyHeader.m_elementIndex);
        if (firstVisibleHeaderIndex >= 0)
        {
            // Get the beginning of the first visible header
            float firstVisibleHeaderBeginning = GetElementOffsetAtIndex(firstVisibleHeaderIndex);

            // Get the end of the current sticky header
            float stickyHeaderSize = !m_variableElementSize[ElementType::SectionHeader] ? m_prototypeElementSize[ElementType::SectionHeader] : GetVariableElementSize(m_currentStickyHeader.m_elementIndex);
            float stickyHeaderEnd = visibleContentBeginning + stickyHeaderSize;

            // Adjust sticky header offset
            if (firstVisibleHeaderBeginning < stickyHeaderEnd)
            {
                stickyHeaderOffset = firstVisibleHeaderBeginning - stickyHeaderEnd;
            }
        }

        SetElementOffsets(m_currentStickyHeader.m_element, stickyHeaderOffset);
    }
    else
    {
        m_currentStickyHeader.m_elementIndex = -1;
        m_currentStickyHeader.m_indexInfo.m_sectionIndex = -1;

        // Hide the sticky header
        UiElementBus::Event(m_currentStickyHeader.m_element, &UiElementBus::Events::SetIsEnabled, false);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
int UiDynamicScrollBoxComponent::FindFirstVisibleHeaderIndex(int firstVisibleElementIndex, int lastVisibleElementIndex, int excludeIndex)
{
    int firstVisibleHeaderIndex = -1;

    for (int i = firstVisibleElementIndex; i <= lastVisibleElementIndex; i++)
    {
        if (i != excludeIndex && GetElementTypeAtIndex(i) == ElementType::SectionHeader)
        {
            firstVisibleHeaderIndex = i;
            break;
        }
    }

    return firstVisibleHeaderIndex;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiDynamicScrollBoxComponent::FindVisibleElementIndicesForFixedSizes(const AZ::Vector2& visibleContentBounds,
    int& firstVisibleElementIndexOut,
    int& lastVisibleElementIndexOut) const
{
    float itemSize = m_prototypeElementSize[ElementType::Item];
    float beginningVisibleOffset = visibleContentBounds.GetX();
    float endVisibleOffset = visibleContentBounds.GetY();

    if (!m_hasSections)
    {
        if (itemSize > 0.0f)
        {
            // Calculate first visible element index
            firstVisibleElementIndexOut = max(static_cast<int>(ceil(beginningVisibleOffset / itemSize)) - 1, 0);

            // Calculate last visible element index
            lastVisibleElementIndexOut = static_cast<int>(ceil(endVisibleOffset / itemSize)) - 1;
            int lastElementIndex = max(m_numElements - 1, 0);
            Limit(lastVisibleElementIndexOut, 0, lastElementIndex);
        }
    }
    else
    {
        float headerSize = m_prototypeElementSize[ElementType::SectionHeader];

        if (itemSize > 0.0f || headerSize > 0.0f)
        {
            // Calculate first and last visible element indices
            float curElementOffset = 0.0f;
            int curSectionIndex = 0;
            for (int i = 0; i < 2; i++)
            {
                int& visibleElementIndex = (i == 0) ? firstVisibleElementIndexOut : lastVisibleElementIndexOut;
                float visibleOffset = (i == 0) ? beginningVisibleOffset : endVisibleOffset;

                for (; curSectionIndex < m_sections.size(); curSectionIndex++)
                {
                    float headerElementEnd = curElementOffset + headerSize;
                    if (headerElementEnd >= visibleOffset)
                    {
                        visibleElementIndex = m_sections[curSectionIndex].m_headerElementIndex;
                        break;
                    }
                    else
                    {
                        float sectionEnd = headerElementEnd + itemSize * m_sections[curSectionIndex].m_numItems;
                        if (sectionEnd >= visibleOffset)
                        {
                            int numItems = 0;
                            if (itemSize > 0.0f)
                            {
                                numItems = static_cast<int>(ceil((visibleOffset - headerElementEnd) / itemSize));
                            }
                            visibleElementIndex = m_sections[curSectionIndex].m_headerElementIndex + numItems;
                            break;
                        }
                        else if (curSectionIndex == m_sections.size() - 1)
                        {
                            visibleElementIndex = m_sections[curSectionIndex].m_headerElementIndex + m_sections[curSectionIndex].m_numItems;
                            break;
                        }

                        curElementOffset = sectionEnd;
                    }
                }
            }
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
int UiDynamicScrollBoxComponent::FindVisibleElementIndexToRemainInPlace(const AZ::Vector2& visibleContentBounds) const
{
    int visibleElementIndex = -1;

    // Try and find the first previously visible element that's still visible
    int firstPrevVisibleIndexStillVisible = -1;
    if (m_firstVisibleElementIndex >= 0)
    {
        // Check if any of the previously visible elements are still visible
        float prevFirstVisibleBeginning = GetVariableSizeElementOffset(m_firstVisibleElementIndex);
        float prevLastVisibleEnd = GetVariableSizeElementOffset(m_lastVisibleElementIndex) + GetVariableElementSize(m_lastVisibleElementIndex);

        if (!(prevFirstVisibleBeginning > visibleContentBounds.GetY() || prevLastVisibleEnd < visibleContentBounds.GetX()))
        {
            // Find the first previously visible element that's still visible
            for (int index = m_firstVisibleElementIndex; index <= m_lastVisibleElementIndex; index++)
            {
                if (GetVariableSizeElementOffset(index) + GetVariableElementSize(index) >= visibleContentBounds.GetX())
                {
                    firstPrevVisibleIndexStillVisible = index;
                    break;
                }
            }
        }
    }

    if (firstPrevVisibleIndexStillVisible >= 0)
    {
        visibleElementIndex = firstPrevVisibleIndexStillVisible;
    }
    else
    {
        // No previously visible elements are still visible, so find the first element that's about to become visible

        // Estimate a first visible element index
        int estimatedFirstVisibleElementIndex = EstimateFirstVisibleElementIndex(visibleContentBounds);

        // Look for the real new first visible element index
        float firstVisibleElementEnd = 0.0f;
        visibleElementIndex = FindFirstVisibleElementIndex(estimatedFirstVisibleElementIndex, visibleContentBounds, firstVisibleElementEnd);

        // We actually want the first visible element who's beginning (top or left) is visible if we don't know the first visible element's real size.
        // This is so that we don't end up having to calculate the size of more elements if the real size of the first visible
        // element ends up being smaller than the estimated size
        if (m_cachedElementInfo[visibleElementIndex].m_size < 0.0f && visibleElementIndex < m_numElements - 1)
        {
            float firstVisibleElementBeginning = firstVisibleElementEnd - GetVariableElementSize(visibleElementIndex);
            if (firstVisibleElementBeginning < visibleContentBounds.GetX() && firstVisibleElementEnd < visibleContentBounds.GetY())
            {
                ++visibleElementIndex;
            }
        }
    }

    return visibleElementIndex;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiDynamicScrollBoxComponent::AddExtraElementsForNavigation(int& firstDisplayedElementIndexOut, int& lastDisplayedElementIndexOut) const
{
    if (AnyPrototypeElementsNavigable())
    {
        if (firstDisplayedElementIndexOut > 0)
        {
            --firstDisplayedElementIndexOut;
            if (m_hasSections)
            {
                int newFirstDisplayedElementIndex = firstDisplayedElementIndexOut;
                while (!m_isPrototypeElementNavigable[GetElementTypeAtIndex(newFirstDisplayedElementIndex)] && newFirstDisplayedElementIndex >= 0)
                {
                    --newFirstDisplayedElementIndex;
                }
                if (newFirstDisplayedElementIndex >= 0)
                {
                    firstDisplayedElementIndexOut = newFirstDisplayedElementIndex;
                }
            }
        }

        if (lastDisplayedElementIndexOut > -1 && lastDisplayedElementIndexOut < m_numElements - 1)
        {
            ++lastDisplayedElementIndexOut;
            if (m_hasSections)
            {
                int newLastDisplayedElementIndex = lastDisplayedElementIndexOut;
                while (!m_isPrototypeElementNavigable[GetElementTypeAtIndex(newLastDisplayedElementIndex)] && newLastDisplayedElementIndex < m_numElements)
                {
                    ++newLastDisplayedElementIndex;
                }
                if (newLastDisplayedElementIndex < m_numElements)
                {
                    lastDisplayedElementIndexOut = newLastDisplayedElementIndex;
                }
            }
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
int UiDynamicScrollBoxComponent::EstimateFirstVisibleElementIndex(const AZ::Vector2& visibleContentBounds) const
{
    // Estimate an index size that will be close to the new first visible element index
    int estimatedElementIndex = 0;

    if (m_averageElementSize > 0.0f)
    {
        if (m_firstVisibleElementIndex >= 0)
        {
            // Check how much scrolling has occurred
            float scrollDelta = visibleContentBounds.GetX() - m_lastCalculatedVisibleContentOffset;
            // Estimate the number of elements within the scroll delta
            int estimatedElementIndexOffset = max(static_cast<int>(ceil(fabs(scrollDelta / m_averageElementSize))) - 1, 0);
            estimatedElementIndex = m_firstVisibleElementIndex + (scrollDelta > 0.0f ? estimatedElementIndexOffset : -estimatedElementIndexOffset);
        }
        else
        {
            estimatedElementIndex = max(static_cast<int>(ceil(visibleContentBounds.GetX() / m_averageElementSize)) - 1, 0);
        }
    }

    estimatedElementIndex = AZ::GetClamp(estimatedElementIndex, 0, m_numElements - 1);

    return estimatedElementIndex;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
int UiDynamicScrollBoxComponent::FindFirstVisibleElementIndex(int estimatedIndex, const AZ::Vector2& visibleContentBounds, float& firstVisibleElementEndOut) const
{
    int curElementIndex = estimatedIndex;
    float curElementPos = GetVariableSizeElementOffset(curElementIndex);
    if (curElementPos <= visibleContentBounds.GetX())
    {
        // Traverse down to find the real new first visible element index
        curElementPos += GetVariableElementSize(curElementIndex);
        while (curElementPos < visibleContentBounds.GetX() && curElementIndex < m_numElements - 1)
        {
            ++curElementIndex;
            curElementPos += GetVariableElementSize(curElementIndex);
        }
    }
    else
    {
        // Traverse up to find the real new first visible element index
        while (curElementPos > visibleContentBounds.GetX() && curElementIndex > 0)
        {
            --curElementIndex;
            curElementPos -= GetVariableElementSize(curElementIndex);
        }
        curElementPos += GetVariableElementSize(curElementIndex);
    }

    firstVisibleElementEndOut = curElementPos;

    return curElementIndex;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiDynamicScrollBoxComponent::CalculateVisibleSpaceBeforeAndAfterElement(int visibleElementIndex,
    bool keepAtEnd,
    float visibleAreaBeginning,
    float& spaceLeftBeforeOut,
    float& spaceLeftAfterOut) const
{
    float visibleAreaSize = GetVisibleAreaSize();

    // Calculate space left in the visible area
    spaceLeftBeforeOut = 0.0f;
    spaceLeftAfterOut = 0.0f;
    if (keepAtEnd)
    {
        spaceLeftAfterOut = 0.0f;
        spaceLeftBeforeOut = AZ::GetMax(visibleAreaSize - GetVariableElementSize(visibleElementIndex), 0.0f);
    }
    else
    {
        float visibleElementBeginning = GetVariableSizeElementOffset(visibleElementIndex);
        float visibleElementEnd = visibleElementBeginning + GetVariableElementSize(visibleElementIndex);
        spaceLeftBeforeOut = AZ::GetMax(visibleElementBeginning - visibleAreaBeginning, 0.0f);
        spaceLeftAfterOut = AZ::GetMax(visibleAreaSize - (visibleElementEnd - visibleAreaBeginning), 0.0f);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiDynamicScrollBoxComponent::CalculateVisibleElementIndicesFromVisibleElementIndex(int visibleElementIndex,
    const AZ::Vector2& visibleContentBound,
    bool keepAtEnd,
    int& firstVisibleElementIndexOut,
    int& lastVisibleElementIndexOut,
    int& firstDisplayedElementIndexOut,
    int& lastDisplayedElementIndexOut,
    int& firstDisplayedElementIndexWithSizeChangeOut,
    float& totalElementSizechangeOut,
    float& scrollChangeOut)
{
    // From the current element index that we know is going to be at least partly visible,
    // traverse up and down to find the real first and last visible element indices

    // Track the total change in element size
    float totalSizeChange = 0.0f;

    // Track the total change in size of elements that are positioned before the passed in
    // visible element index who's beginning (top or left) will remain in the same position
    float totalSizeChangeBeforeFixedVisibleElement = 0.0f;

    // Keep track of the index of the first element who's size changed
    firstDisplayedElementIndexWithSizeChangeOut = -1;

    // Check if we need to calculate the real size for the known visible element index
    if (m_cachedElementInfo[visibleElementIndex].m_size < 0.0f)
    {
        float prevSize = GetVariableElementSize(visibleElementIndex);
        float newSize = CalculateVariableElementSize(visibleElementIndex);

        totalSizeChange = newSize - prevSize;
        firstDisplayedElementIndexWithSizeChangeOut = visibleElementIndex;
    }

    // Calculate visible space remaining
    float spaceLeftBefore = 0.0f;
    float spaceLeftAfter = 0.0f;
    CalculateVisibleSpaceBeforeAndAfterElement(visibleElementIndex, keepAtEnd, visibleContentBound.GetX(), spaceLeftBefore, spaceLeftAfter);

    firstVisibleElementIndexOut = visibleElementIndex;
    lastVisibleElementIndexOut = visibleElementIndex;
    firstDisplayedElementIndexOut = firstVisibleElementIndexOut;
    lastDisplayedElementIndexOut = lastVisibleElementIndexOut;

    bool extraElementsNeededForNavigation = AnyPrototypeElementsNavigable();

    // Traverse up or left
    bool hadSpaceLeft = true;
    bool addedExtraElements = false;
    while ((spaceLeftBefore > 0.0f || !addedExtraElements) && firstDisplayedElementIndexOut > 0)
    {
        if (spaceLeftBefore <= 0.0f)
        {
            if (hadSpaceLeft)
            {
                firstVisibleElementIndexOut = firstDisplayedElementIndexOut;
                hadSpaceLeft = false;
            }

            if (!extraElementsNeededForNavigation)
            {
                break;
            }

            addedExtraElements = !m_hasSections || m_isPrototypeElementNavigable[GetElementTypeAtIndex(firstDisplayedElementIndexOut - 1)];
        }

        --firstDisplayedElementIndexOut;
        if (m_cachedElementInfo[firstDisplayedElementIndexOut].m_size >= 0.0f)
        {
            spaceLeftBefore -= m_cachedElementInfo[firstDisplayedElementIndexOut].m_size;
        }
        else
        {
            // Calculate this element's size
            float prevSize = GetVariableElementSize(firstDisplayedElementIndexOut);
            float newSize = CalculateVariableElementSize(firstDisplayedElementIndexOut);

            float sizeChange = newSize - prevSize;
            totalSizeChange += sizeChange;

            if (firstDisplayedElementIndexOut <= visibleElementIndex)
            {
                totalSizeChangeBeforeFixedVisibleElement += sizeChange;
            }

            spaceLeftBefore -= newSize;

            if (firstDisplayedElementIndexWithSizeChangeOut < 0 || firstDisplayedElementIndexOut < firstDisplayedElementIndexWithSizeChangeOut)
            {
                firstDisplayedElementIndexWithSizeChangeOut = firstDisplayedElementIndexOut;
            }
        }
    }

    if (hadSpaceLeft)
    {
        firstVisibleElementIndexOut = firstDisplayedElementIndexOut;
    }

    // Traverse down or right
    hadSpaceLeft = true;
    addedExtraElements = false;
    while ((spaceLeftAfter > 0.0f || !addedExtraElements) && lastDisplayedElementIndexOut < m_numElements - 1)
    {
        if (spaceLeftAfter <= 0.0f)
        {
            if (hadSpaceLeft)
            {
                lastVisibleElementIndexOut = lastDisplayedElementIndexOut;
                hadSpaceLeft = false;
            }

            if (!extraElementsNeededForNavigation)
            {
                break;
            }

            addedExtraElements = !m_hasSections || m_isPrototypeElementNavigable[GetElementTypeAtIndex(lastDisplayedElementIndexOut + 1)];
        }

        ++lastDisplayedElementIndexOut;
        if (m_cachedElementInfo[lastDisplayedElementIndexOut].m_size >= 0.0f)
        {
            spaceLeftAfter -= m_cachedElementInfo[lastDisplayedElementIndexOut].m_size;
        }
        else
        {
            // Calculate this element's size
            float prevSize = GetVariableElementSize(lastDisplayedElementIndexOut);
            float newSize = CalculateVariableElementSize(lastDisplayedElementIndexOut);

            float sizeChange = newSize - prevSize;
            totalSizeChange += sizeChange;

            if (lastDisplayedElementIndexOut <= visibleElementIndex)
            {
                totalSizeChangeBeforeFixedVisibleElement += sizeChange;
            }

            spaceLeftAfter -= newSize;

            if (firstDisplayedElementIndexWithSizeChangeOut < 0 || lastDisplayedElementIndexOut < firstDisplayedElementIndexWithSizeChangeOut)
            {
                firstDisplayedElementIndexWithSizeChangeOut = lastDisplayedElementIndexOut;
            }
        }
    }

    if (hadSpaceLeft)
    {
        lastVisibleElementIndexOut = lastDisplayedElementIndexOut;
    }

    if (StickyHeadersEnabled())
    {
        // Check which header should currently be sticky and calculate its size if needed
        if (firstVisibleElementIndexOut >= 0)
        {
            ElementIndexInfo firstVisibleElementIndexInfo = GetElementIndexInfoFromIndex(firstVisibleElementIndexOut);
            int stickyHeaderElementIndex = m_sections[firstVisibleElementIndexInfo.m_sectionIndex].m_headerElementIndex;

            if (m_cachedElementInfo[stickyHeaderElementIndex].m_size < 0.0f)
            {
                // Calculate this element's size    
                float prevSize = GetVariableElementSize(stickyHeaderElementIndex);
                float newSize = CalculateVariableElementSize(stickyHeaderElementIndex);

                float sizeChange = newSize - prevSize;
                totalSizeChange += sizeChange;

                // Cache the accumulated size
                m_cachedElementInfo[stickyHeaderElementIndex].m_accumulatedSize = GetVariableSizeElementOffset(stickyHeaderElementIndex) + newSize;

                // Update accumulated sizes for all elements after the sticky header and before the first displayed element who's size changed.
                // The rest of the cache updates for the displayed elements who's size changed will be handled below
                for (int index = stickyHeaderElementIndex + 1; index < AZ::GetMax(firstDisplayedElementIndexWithSizeChangeOut, firstDisplayedElementIndexOut); index++)
                {
                    if (m_cachedElementInfo[index].m_accumulatedSize >= 0.0f)
                    {
                        m_cachedElementInfo[index].m_accumulatedSize += sizeChange;
                    }
                }

                if (stickyHeaderElementIndex <= visibleElementIndex)
                {
                    totalSizeChangeBeforeFixedVisibleElement += sizeChange;
                }

                if (firstDisplayedElementIndexWithSizeChangeOut < 0 || stickyHeaderElementIndex < firstDisplayedElementIndexWithSizeChangeOut)
                {
                    firstDisplayedElementIndexWithSizeChangeOut = stickyHeaderElementIndex;
                }
            }
        }
    }

    DisableElementsForAutoSizeCalculation();

    // Update the cache info
    if (firstDisplayedElementIndexWithSizeChangeOut >= 0)
    {
        // Cache the accumulated sizes for the displayed elements who's sizes were just calculated and cached
        int startIndex = AZ::GetMax(firstDisplayedElementIndexWithSizeChangeOut, firstDisplayedElementIndexOut);
        float curPos = GetVariableSizeElementOffset(startIndex);
        for (int index = startIndex; index <= lastDisplayedElementIndexOut; index++)
        {
            curPos += m_cachedElementInfo[index].m_size;
            m_cachedElementInfo[index].m_accumulatedSize = curPos;
        }

        // Update accumulated sizes for all elements after the last displayed element
        for (int index = lastDisplayedElementIndexOut + 1; index < m_numElements; index++)
        {
            if (m_cachedElementInfo[index].m_accumulatedSize >= 0.0f)
            {
                m_cachedElementInfo[index].m_accumulatedSize += totalSizeChange;
            }
        }
    }

    UpdateAverageElementSize(0, totalSizeChange);
    
    scrollChangeOut = 0.0f;
    if (totalSizeChange != 0.0f)
    {
        if (keepAtEnd)
        {
            scrollChangeOut = CalculateContentEndDeltaAfterSizeChange(totalSizeChange);
        }
        else
        {
            scrollChangeOut = CalculateContentBeginningDeltaAfterSizeChange(totalSizeChange);
        }
    }
    if (!keepAtEnd)
    {
        scrollChangeOut -= totalSizeChangeBeforeFixedVisibleElement;
    }

    totalElementSizechangeOut = totalSizeChange;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
float UiDynamicScrollBoxComponent::CalculateContentBeginningDeltaAfterSizeChange(float contentSizeDelta) const
{
    // Find the content element
    AZ::EntityId contentEntityId;
    UiScrollBoxBus::EventResult(contentEntityId, GetEntityId(), &UiScrollBoxBus::Events::GetContentEntity);

    if (!contentEntityId.IsValid())
    {
        return 0.0f;
    }

    // Get current content size
    AZ::Vector2 curContentSize(0.0f, 0.0f);
    UiTransformBus::EventResult(curContentSize, contentEntityId, &UiTransformBus::Events::GetCanvasSpaceSizeNoScaleRotate);

    UiTransform2dInterface::Offsets offsets;
    UiTransform2dBus::EventResult(offsets, contentEntityId, &UiTransform2dBus::Events::GetOffsets);

    AZ::Vector2 pivot;
    UiTransformBus::EventResult(pivot, contentEntityId, &UiTransformBus::Events::GetPivot);

    float beginningDelta = 0.0f;
    if (m_isVertical)
    {
        beginningDelta = contentSizeDelta * pivot.GetY();
    }
    else
    {
        beginningDelta = contentSizeDelta * pivot.GetX();
    }

    return beginningDelta;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
float UiDynamicScrollBoxComponent::CalculateContentEndDeltaAfterSizeChange(float contentSizeDelta) const
{
    // Find the content element
    AZ::EntityId contentEntityId;
    UiScrollBoxBus::EventResult(contentEntityId, GetEntityId(), &UiScrollBoxBus::Events::GetContentEntity);

    if (!contentEntityId.IsValid())
    {
        return 0.0f;
    }

    // Get current content size
    AZ::Vector2 curContentSize(0.0f, 0.0f);
    UiTransformBus::EventResult(curContentSize, contentEntityId, &UiTransformBus::Events::GetCanvasSpaceSizeNoScaleRotate);

    UiTransform2dInterface::Offsets offsets;
    UiTransform2dBus::EventResult(offsets, contentEntityId, &UiTransform2dBus::Events::GetOffsets);

    AZ::Vector2 pivot;
    UiTransformBus::EventResult(pivot, contentEntityId, &UiTransformBus::Events::GetPivot);

    float endDelta = 0.0f;
    if (m_isVertical)
    {
        // Restore end
        endDelta = -contentSizeDelta * (1.0f - pivot.GetY());
    }
    else
    {
        // Restore end
        endDelta = -contentSizeDelta * (1.0f - pivot.GetX());
    }

    return endDelta;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiDynamicScrollBoxComponent::IsScrolledToEnd() const
{
    // Find the content element
    AZ::EntityId contentEntityId;
    UiScrollBoxBus::EventResult(contentEntityId, GetEntityId(), &UiScrollBoxBus::Events::GetContentEntity);

    if (!contentEntityId.IsValid())
    {
        return false;
    }

    // Get content's parent
    AZ::EntityId contentParentEntityId;
    UiElementBus::EventResult(contentParentEntityId, contentEntityId, &UiElementBus::Events::GetParentEntityId);
    if (!contentParentEntityId.IsValid())
    {
        return false;
    }

    // Get content's rect in canvas space
    UiTransformInterface::Rect contentRect;
    UiTransformBus::Event(contentEntityId, &UiTransformBus::Events::GetCanvasSpaceRectNoScaleRotate, contentRect);

    // Get content parent's rect in canvas space
    UiTransformInterface::Rect parentRect;
    UiTransformBus::Event(contentParentEntityId, &UiTransformBus::Events::GetCanvasSpaceRectNoScaleRotate, parentRect);

    bool scrolledToEnd = false;
    if (m_isVertical)
    {
        scrolledToEnd = parentRect.bottom >= contentRect.bottom;
    }
    else
    {
        scrolledToEnd = parentRect.right >= contentRect.right;
    }

    return scrolledToEnd;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiDynamicScrollBoxComponent::IsElementDisplayedAtIndex(int index) const
{
    if (m_firstDisplayedElementIndex < 0)
    {
        return false;
    }

    return ((index >= m_firstDisplayedElementIndex) && (index <= m_lastDisplayedElementIndex));
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZ::EntityId UiDynamicScrollBoxComponent::GetElementForDisplay(ElementType elementType)
{
    AZ::EntityId element;

    // Check if there is an existing element
    if (!m_recycledElements[elementType].empty())
    {
        element = m_recycledElements[elementType].front();
        m_recycledElements[elementType].pop_front();

        // Enable element
        UiElementBus::Event(element, &UiElementBus::Events::SetIsEnabled, true);
    }
    else
    {
        element = ClonePrototypeElement(elementType);
    }

    return element;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZ::EntityId UiDynamicScrollBoxComponent::GetElementForAutoSizeCalculation(ElementType elementType)
{
    if (!m_clonedElementForAutoSizeCalculation[elementType].IsValid())
    {
        m_clonedElementForAutoSizeCalculation[elementType] = ClonePrototypeElement(elementType);
    }
    else
    {
        // Enable element
        UiElementBus::Event(m_clonedElementForAutoSizeCalculation[elementType], &UiElementBus::Events::SetIsEnabled, true);
    }

    return m_clonedElementForAutoSizeCalculation[elementType];
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiDynamicScrollBoxComponent::DisableElementsForAutoSizeCalculation() const
{
    for (int i = 0; i < ElementType::NumElementTypes; i++)
    {
        if (m_clonedElementForAutoSizeCalculation[i].IsValid())
        {
            // Disable element
            UiElementBus::Event(m_clonedElementForAutoSizeCalculation[i], &UiElementBus::Events::SetIsEnabled, false);
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
float UiDynamicScrollBoxComponent::AutoCalculateElementSize(AZ::EntityId elementForAutoSizeCalculation) const
{
    float size = 0.0f;

    if (m_isVertical)
    {
        size = UiLayoutHelpers::GetLayoutElementTargetHeight(elementForAutoSizeCalculation);
    }
    else
    {
        size = UiLayoutHelpers::GetLayoutElementTargetWidth(elementForAutoSizeCalculation);
    }

    return size;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiDynamicScrollBoxComponent::SizeVariableElementAtIndex(AZ::EntityId element, int index) const
{
    // Get current element size
    AZ::Vector2 curElementSize(0.0f, 0.0f);
    UiTransformBus::EventResult(curElementSize, element, &UiTransformBus::Events::GetCanvasSpaceSizeNoScaleRotate);

    float curSize = m_isVertical ? curElementSize.GetY() : curElementSize.GetX();

    // Get new element size
    float newSize = GetVariableElementSize(index);

    if (newSize != curSize)
    {
        // Resize the element

        UiTransform2dInterface::Offsets offsets;
        UiTransform2dBus::EventResult(offsets, element, &UiTransform2dBus::Events::GetOffsets);

        AZ::Vector2 pivot;
        UiTransformBus::EventResult(pivot, element, &UiTransformBus::Events::GetPivot);

        float sizeDiff = newSize - curSize;

        if (m_isVertical)
        {
            offsets.m_top -= sizeDiff * pivot.GetY();
            offsets.m_bottom += sizeDiff * (1.0f - pivot.GetY());
        }
        else
        {
            offsets.m_left -= sizeDiff * pivot.GetX();
            offsets.m_right += sizeDiff * (1.0f - pivot.GetX());
        }

        UiTransform2dBus::Event(element, &UiTransform2dBus::Events::SetOffsets, offsets);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiDynamicScrollBoxComponent::PositionElementAtIndex(AZ::EntityId element, int index) const
{
    // Position offsets based on index
    float offset = GetElementOffsetAtIndex(index);

    SetElementOffsets(element, offset);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiDynamicScrollBoxComponent::SetElementAnchors(AZ::EntityId element) const
{
    // Get the element anchors
    UiTransform2dInterface::Anchors anchors;
    UiTransform2dBus::EventResult(anchors, element, &UiTransform2dBus::Events::GetAnchors);

    if (m_isVertical)
    {
        // Set anchors to top of parent
        anchors.m_top = 0.0f;
        anchors.m_bottom = 0.0f;
    }
    else
    {
        // Set anchors to left of parent
        anchors.m_left = 0.0f;
        anchors.m_right = 0.0f;
    }

    UiTransform2dBus::Event(element, &UiTransform2dBus::Events::SetAnchors, anchors, false, false);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiDynamicScrollBoxComponent::SetElementOffsets(AZ::EntityId element, float offset) const
{
    // Get the element offsets
    UiTransform2dInterface::Offsets offsets;
    UiTransform2dBus::EventResult(offsets, element, &UiTransform2dBus::Events::GetOffsets);

    if ((m_isVertical && offsets.m_top != offset) || (!m_isVertical && offsets.m_left != offset))
    {
        if (m_isVertical)
        {
            float height = offsets.m_bottom - offsets.m_top;
            offsets.m_top = offset;
            offsets.m_bottom = offsets.m_top + height;
        }
        else
        {
            float width = offsets.m_right - offsets.m_left;
            offsets.m_left = offset;
            offsets.m_right = offsets.m_left + width;
        }

        UiTransform2dBus::Event(element, &UiTransform2dBus::Events::SetOffsets, offsets);
    }
}



////////////////////////////////////////////////////////////////////////////////////////////////////
UiDynamicScrollBoxComponent::ElementType UiDynamicScrollBoxComponent::GetElementTypeAtIndex(int index) const
{
    ElementType elementType = ElementType::Item;

    if (m_hasSections)
    {
        for (int i = 0; i < m_sections.size(); i++)
        {
            if (m_sections[i].m_headerElementIndex == index)
            {
                elementType = ElementType::SectionHeader;
                break;
            }
        }
    }

    return elementType;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
UiDynamicScrollBoxComponent::ElementIndexInfo UiDynamicScrollBoxComponent::GetElementIndexInfoFromIndex(int index) const
{
    ElementIndexInfo elementIndexInfo;
    elementIndexInfo.m_sectionIndex = -1;
    elementIndexInfo.m_itemIndexInSection = index;

    if (m_hasSections)
    {
        for (int i = 0; i < m_sections.size(); i++)
        {
            if (index <= m_sections[i].m_headerElementIndex + m_sections[i].m_numItems)
            {
                elementIndexInfo.m_sectionIndex = i;
                elementIndexInfo.m_itemIndexInSection = (index - m_sections[i].m_headerElementIndex) - 1; // for headers, this will be set to -1
                break;
            }
        }
    }

    return elementIndexInfo;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
int UiDynamicScrollBoxComponent::GetIndexFromElementIndexInfo(const ElementIndexInfo& elementIndexInfo) const
{
    int index = elementIndexInfo.m_itemIndexInSection;

    if (m_hasSections)
    {
        index += m_sections[elementIndexInfo.m_sectionIndex].m_headerElementIndex + 1;
    }

    return index;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZ::EntityId UiDynamicScrollBoxComponent::GetImmediateContentChildFromDescendant(AZ::EntityId childElement) const
{
    AZ::EntityId immediateChild;

    AZ::Entity* contentEntity = GetContentEntity();
    if (contentEntity)
    {
        immediateChild = childElement;
        AZ::Entity* parent = nullptr;
        UiElementBus::EventResult(parent, immediateChild, &UiElementBus::Events::GetParent);
        while (parent && parent != contentEntity)
        {
            immediateChild = parent->GetId();
            UiElementBus::EventResult(parent, immediateChild, &UiElementBus::Events::GetParent);
        }

        if (parent != contentEntity)
        {
            immediateChild.SetInvalid();
        }
    }

    return immediateChild;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiDynamicScrollBoxComponent::HeadersHaveVariableSizes() const
{
    return m_hasSections && m_variableHeaderElementSize;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiDynamicScrollBoxComponent::IsValidPrototype(AZ::EntityId entityId) const
{
    // Entities containing the scroll box itself are not safe to clone as they will respawn this
    // scroll box and result in infinite recursive spawning.
    if (!entityId.IsValid() || entityId == GetEntityId())
    {
        return false;
    }

    bool isEntityAncestor;
    UiElementBus::EventResult(isEntityAncestor, GetEntityId(), &UiElementBus::Events::IsAncestor, entityId);
    return !isEntityAncestor;
}
