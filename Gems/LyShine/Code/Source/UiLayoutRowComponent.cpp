/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "UiLayoutRowComponent.h"

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/RTTI/BehaviorContext.h>

#include <LyShine/Bus/UiElementBus.h>
#include <LyShine/Bus/UiTransform2dBus.h>
#include <LyShine/Bus/UiLayoutManagerBus.h>

#include "UiSerialize.h"

////////////////////////////////////////////////////////////////////////////////////////////////////
// PUBLIC MEMBER FUNCTIONS
////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////
UiLayoutRowComponent::UiLayoutRowComponent()
    : m_padding(UiLayoutInterface::Padding())
    , m_spacing(5)
    , m_order(UiLayoutInterface::HorizontalOrder::LeftToRight)
    , m_childHAlignment(IDraw2d::HAlign::Left)
    , m_childVAlignment(IDraw2d::VAlign::Top)
    , m_ignoreDefaultLayoutCells(true)
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
UiLayoutRowComponent::~UiLayoutRowComponent()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiLayoutRowComponent::ApplyLayoutWidth()
{
    // Get the layout rect inside the padding
    AZ::Vector2 layoutRectSize(0.0f, 0.0f);
    UiLayoutHelpers::GetSizeInsidePadding(GetEntityId(), m_padding, layoutRectSize);

    // First, calculate and set the child elements' widths.
    // Min/target/extra heights may depend on element widths
    ApplyLayoutWidth(layoutRectSize.GetX());
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiLayoutRowComponent::ApplyLayoutHeight()
{
    // Get the layout rect inside the padding
    AZ::Vector2 layoutRectSize(0.0f, 0.0f);
    UiLayoutHelpers::GetSizeInsidePadding(GetEntityId(), m_padding, layoutRectSize);

    // Calculate and set the child elements' heights
    ApplyLayoutHeight(layoutRectSize.GetY());
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiLayoutRowComponent::IsUsingLayoutCellsToCalculateLayout()
{
    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiLayoutRowComponent::GetIgnoreDefaultLayoutCells()
{
    return m_ignoreDefaultLayoutCells;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiLayoutRowComponent::SetIgnoreDefaultLayoutCells(bool ignoreDefaultLayoutCells)
{
    m_ignoreDefaultLayoutCells = ignoreDefaultLayoutCells;
    InvalidateLayout();
    InvalidateParentLayout();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
IDraw2d::HAlign UiLayoutRowComponent::GetHorizontalChildAlignment()
{
    return m_childHAlignment;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiLayoutRowComponent::SetHorizontalChildAlignment(IDraw2d::HAlign alignment)
{
    m_childHAlignment = alignment;
    InvalidateLayout();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
IDraw2d::VAlign UiLayoutRowComponent::GetVerticalChildAlignment()
{
    return m_childVAlignment;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiLayoutRowComponent::SetVerticalChildAlignment(IDraw2d::VAlign alignment)
{
    m_childVAlignment = alignment;
    InvalidateLayout();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiLayoutRowComponent::IsControllingChild(AZ::EntityId childId)
{
    return UiLayoutHelpers::IsControllingChild(GetEntityId(), childId);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZ::Vector2 UiLayoutRowComponent::GetSizeToFitChildElements(const AZ::Vector2& childElementSize, int numChildElements)
{
    AZ::Vector2 size(0.0f, 0.0f);

    if (numChildElements > 0)
    {
        size.SetX((childElementSize.GetX() * numChildElements) + (m_spacing * (numChildElements - 1)) + m_padding.m_left + m_padding.m_right);
    }
    else
    {
        size.SetX(0.0f);
    }

    // Check if anchors are together
    UiTransform2dInterface::Anchors anchors;
    UiTransform2dBus::EventResult(anchors, GetEntityId(), &UiTransform2dBus::Events::GetAnchors);
    if (anchors.m_top == anchors.m_bottom)
    {
        size.SetY(numChildElements > 0 ? childElementSize.GetY() : 0.0f);
    }
    else
    {
        // Anchors are apart, so height remains untouched
        AZ::Vector2 curSize(0.0f, 0.0f);
        UiTransformBus::EventResult(curSize, GetEntityId(), &UiTransformBus::Events::GetCanvasSpaceSizeNoScaleRotate);
        size.SetY(curSize.GetY());
    }

    return size;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
UiLayoutInterface::Padding UiLayoutRowComponent::GetPadding()
{
    return m_padding;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiLayoutRowComponent::SetPadding(UiLayoutInterface::Padding padding)
{
    m_padding = padding;
    InvalidateLayout();
    InvalidateParentLayout();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
float UiLayoutRowComponent::GetSpacing()
{
    return m_spacing;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiLayoutRowComponent::SetSpacing(float spacing)
{
    m_spacing = spacing;
    InvalidateLayout();
    InvalidateParentLayout();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
UiLayoutInterface::HorizontalOrder UiLayoutRowComponent::GetOrder()
{
    return m_order;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiLayoutRowComponent::SetOrder(UiLayoutInterface::HorizontalOrder order)
{
    m_order = order;
    InvalidateLayout();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
float UiLayoutRowComponent::GetMinWidth()
{
    float width = 0.0f;

    // Minimum layout width is padding + spacing + sum of all child element min widths
    AZStd::vector<float> minWidths = UiLayoutHelpers::GetLayoutCellMinWidths(GetEntityId(), m_ignoreDefaultLayoutCells);
    if (minWidths.size() > 0)
    {
        width += m_padding.m_left + m_padding.m_right + (m_spacing * (minWidths.size() - 1));

        for (auto minWidth : minWidths)
        {
            width += minWidth;
        }
    }

    return width;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
float UiLayoutRowComponent::GetMinHeight()
{
    float height = 0.0f;

    // Minimum layout height is padding + maximum child element min height
    AZStd::vector<float> minHeights = UiLayoutHelpers::GetLayoutCellMinHeights(GetEntityId(), m_ignoreDefaultLayoutCells);
    if (minHeights.size() > 0)
    {
        height += m_padding.m_top + m_padding.m_bottom;

        float maxChildHeight = 0.0f;
        for (auto minHeight : minHeights)
        {
            if (maxChildHeight < minHeight)
            {
                maxChildHeight = minHeight;
            }
        }
        height += maxChildHeight;
    }

    return height;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
float UiLayoutRowComponent::GetTargetWidth(float /*maxWidth*/)
{
    float width = 0.0f;

    // Target layout width is padding + spacing + sum of all child element target widths
    AZStd::vector<float> targetWidths = UiLayoutHelpers::GetLayoutCellTargetWidths(GetEntityId(), m_ignoreDefaultLayoutCells);
    if (targetWidths.size() > 0)
    {
        width += m_padding.m_left + m_padding.m_right + (m_spacing * (targetWidths.size() - 1));

        for (auto targetWidth : targetWidths)
        {
            width += targetWidth;
        }
    }

    return width;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
float UiLayoutRowComponent::GetTargetHeight(float /*maxHeight*/)
{
    float height = 0.0f;

    // Target layout height is padding + maximum child element target height
    AZStd::vector<float> targetHeights = UiLayoutHelpers::GetLayoutCellTargetHeights(GetEntityId(), m_ignoreDefaultLayoutCells);
    if (targetHeights.size() > 0)
    {
        height += m_padding.m_top + m_padding.m_bottom;

        float maxChildHeight = 0.0f;
        for (auto targetHeight : targetHeights)
        {
            if (maxChildHeight < targetHeight)
            {
                maxChildHeight = targetHeight;
            }
        }
        height += maxChildHeight;
    }

    return height;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
float UiLayoutRowComponent::GetExtraWidthRatio()
{
    return 1.0f;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
float UiLayoutRowComponent::GetExtraHeightRatio()
{
    return 1.0f;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiLayoutRowComponent::OnCanvasSpaceRectChanged([[maybe_unused]] AZ::EntityId entityId, const UiTransformInterface::Rect& oldRect, const UiTransformInterface::Rect& newRect)
{
    // If old rect equals new rect, size changed due to initialization
    bool sizeChanged = (oldRect == newRect) || (!oldRect.GetSize().IsClose(newRect.GetSize(), 0.05f));

    if (sizeChanged)
    {
        InvalidateLayout();
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// PUBLIC STATIC MEMBER FUNCTIONS
////////////////////////////////////////////////////////////////////////////////////////////////////

void UiLayoutRowComponent::Reflect(AZ::ReflectContext* context)
{
    AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);

    if (serializeContext)
    {
        serializeContext->Class<UiLayoutRowComponent, AZ::Component>()
            ->Version(2, &VersionConverter)
            ->Field("Padding", &UiLayoutRowComponent::m_padding)
            ->Field("Spacing", &UiLayoutRowComponent::m_spacing)
            ->Field("Order", &UiLayoutRowComponent::m_order)
            ->Field("ChildHAlignment", &UiLayoutRowComponent::m_childHAlignment)
            ->Field("ChildVAlignment", &UiLayoutRowComponent::m_childVAlignment)
            ->Field("IgnoreDefaultLayoutCells", &UiLayoutRowComponent::m_ignoreDefaultLayoutCells);

        AZ::EditContext* ec = serializeContext->GetEditContext();
        if (ec)
        {
            auto editInfo = ec->Class<UiLayoutRowComponent>("LayoutRow", "A layout component that arranges its children in a row");

            editInfo->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                ->Attribute(AZ::Edit::Attributes::Category, "UI")
                ->Attribute(AZ::Edit::Attributes::Icon, "Editor/Icons/Components/UiLayoutRow.png")
                ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Editor/Icons/Components/Viewport/UiLayoutRow.png")
                ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("UI"))
                ->Attribute(AZ::Edit::Attributes::AutoExpand, true);

            editInfo->DataElement(AZ::Edit::UIHandlers::LayoutPadding, &UiLayoutRowComponent::m_padding, "Padding", "The layout padding")
                ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::Show) // needed because sub-elements are hidden
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, &UiLayoutRowComponent::InvalidateLayout)
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, &UiLayoutRowComponent::InvalidateParentLayout)
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, &UiLayoutRowComponent::CheckLayoutFitterAndRefreshEditorTransformProperties);

            editInfo->DataElement(AZ::Edit::UIHandlers::SpinBox, &UiLayoutRowComponent::m_spacing, "Spacing", "The spacing between children")
                ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                ->Attribute(AZ::Edit::Attributes::Step, 1.0f)
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, &UiLayoutRowComponent::InvalidateLayout)
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, &UiLayoutRowComponent::InvalidateParentLayout)
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, &UiLayoutRowComponent::CheckLayoutFitterAndRefreshEditorTransformProperties);

            editInfo->DataElement(AZ::Edit::UIHandlers::ComboBox, &UiLayoutRowComponent::m_order, "Order", "Which direction the row fills")
                ->EnumAttribute(UiLayoutInterface::HorizontalOrder::LeftToRight, "Left to right")
                ->EnumAttribute(UiLayoutInterface::HorizontalOrder::RightToLeft, "Right to left")
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, &UiLayoutRowComponent::InvalidateLayout);

            editInfo->DataElement(AZ::Edit::UIHandlers::CheckBox, &UiLayoutRowComponent::m_ignoreDefaultLayoutCells, "Ignore Default Cells",
                "When checked, fixed default layout cell values are used for child elements with no LayoutCell\n"
                "component rather than using defaults calculated by other components on the child.")
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, &UiLayoutRowComponent::InvalidateLayout)
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, &UiLayoutRowComponent::InvalidateParentLayout)
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, &UiLayoutRowComponent::CheckLayoutFitterAndRefreshEditorTransformProperties);

            // Alignment
            {
                editInfo->ClassElement(AZ::Edit::ClassElements::Group, "Child Alignment")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true);

                editInfo->DataElement(AZ::Edit::UIHandlers::ComboBox, &UiLayoutRowComponent::m_childHAlignment, "Horizontal",
                    "How to align the children if they don't take up all the available width")
                    ->EnumAttribute(IDraw2d::HAlign::Left, "Left")
                    ->EnumAttribute(IDraw2d::HAlign::Center, "Center")
                    ->EnumAttribute(IDraw2d::HAlign::Right, "Right")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &UiLayoutRowComponent::InvalidateLayout);
                editInfo->DataElement(AZ::Edit::UIHandlers::ComboBox, &UiLayoutRowComponent::m_childVAlignment, "Vertical",
                    "How to align the children if they don't take up all the available height")
                    ->EnumAttribute(IDraw2d::VAlign::Top, "Top")
                    ->EnumAttribute(IDraw2d::VAlign::Center, "Center")
                    ->EnumAttribute(IDraw2d::VAlign::Bottom, "Bottom")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &UiLayoutRowComponent::InvalidateLayout);
            }
        }
    }

    AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context);
    if (behaviorContext)
    {
        behaviorContext->EBus<UiLayoutRowBus>("UiLayoutRowBus")
            ->Event("GetPadding", &UiLayoutRowBus::Events::GetPadding)
            ->Event("SetPadding", &UiLayoutRowBus::Events::SetPadding)
            ->Event("GetSpacing", &UiLayoutRowBus::Events::GetSpacing)
            ->Event("SetSpacing", &UiLayoutRowBus::Events::SetSpacing)
            ->Event("GetOrder", &UiLayoutRowBus::Events::GetOrder)
            ->Event("SetOrder", &UiLayoutRowBus::Events::SetOrder)
            ->VirtualProperty("Padding", "GetPadding", "SetPadding")
            ->VirtualProperty("Spacing", "GetSpacing", "SetSpacing");

        behaviorContext->Class<UiLayoutRowComponent>()->RequestBus("UiLayoutRowBus");
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// PROTECTED MEMBER FUNCTIONS
////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiLayoutRowComponent::Activate()
{
    UiLayoutBus::Handler::BusConnect(m_entity->GetId());
    UiLayoutControllerBus::Handler::BusConnect(m_entity->GetId());
    UiLayoutRowBus::Handler::BusConnect(m_entity->GetId());
    UiLayoutCellDefaultBus::Handler::BusConnect(m_entity->GetId());
    UiTransformChangeNotificationBus::Handler::BusConnect(m_entity->GetId());

    // If this is the first time the entity has been activated this has no effect since the canvas
    // is not known. But if a LayoutRow component has just been pasted onto an existing entity
    // we need to invalidate the layout in case that affects things.
    InvalidateLayout();
    InvalidateParentLayout();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiLayoutRowComponent::Deactivate()
{
    UiLayoutBus::Handler::BusDisconnect();
    UiLayoutControllerBus::Handler::BusDisconnect();
    UiLayoutRowBus::Handler::BusDisconnect();
    UiLayoutCellDefaultBus::Handler::BusDisconnect();
    UiTransformChangeNotificationBus::Handler::BusDisconnect();

    // We could be about to remove this component and then reactivate the entity
    // which could affect the layout if there is a parent layout component
    InvalidateLayout();
    InvalidateParentLayout();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiLayoutRowComponent::InvalidateLayout()
{
    UiLayoutHelpers::InvalidateLayout(GetEntityId());
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiLayoutRowComponent::InvalidateParentLayout()
{
    UiLayoutHelpers::InvalidateParentLayout(GetEntityId());
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiLayoutRowComponent::CheckLayoutFitterAndRefreshEditorTransformProperties() const
{
    UiLayoutHelpers::CheckFitterAndRefreshEditorTransformProperties(GetEntityId());
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiLayoutRowComponent::ApplyLayoutWidth(float availableWidth)
{
    // Get the child element cell widths
    UiLayoutHelpers::LayoutCellSizes layoutCells;
    UiLayoutHelpers::GetLayoutCellWidths(GetEntityId(), m_ignoreDefaultLayoutCells, layoutCells);
    int numChildren = static_cast<int>(layoutCells.size());
    if (numChildren > 0)
    {
        // Calculate child widths
        AZStd::vector<float> finalWidths(numChildren, 0.0f);
        UiLayoutHelpers::CalculateElementSizes(layoutCells, availableWidth, m_spacing, finalWidths);

        // Calculate occupied width
        float childrenRectWidth = (numChildren - 1) * m_spacing;
        for (auto width : finalWidths)
        {
            childrenRectWidth += width;
        }

        // Calculate alignment
        float alignmentOffset = UiLayoutHelpers::GetHorizontalAlignmentOffset(m_childHAlignment, availableWidth, childrenRectWidth);

        // Set the child elements' transform properties based on the calculated child widths
        UiTransform2dInterface::Anchors anchors(0.0f, 0.0f, 0.0f, 0.0f);

        AZStd::vector<AZ::EntityId> childEntityIds;
        UiElementBus::EventResult(childEntityIds, GetEntityId(), &UiElementBus::Events::GetChildEntityIds);

        float curX = alignmentOffset;
        switch (m_order)
        {
        case UiLayoutInterface::HorizontalOrder::LeftToRight:
            curX += m_padding.m_left;
            break;
        case UiLayoutInterface::HorizontalOrder::RightToLeft:
            curX += m_padding.m_left + childrenRectWidth;
            break;
        default:
            AZ_Assert(0, "Unrecognized HorizontalOrder type in UiLayoutRowComponent");
            break;
        }

        int childIndex = 0;
        for (auto child : childEntityIds)
        {
            // Set the anchors
            UiTransform2dBus::Event(child, &UiTransform2dBus::Events::SetAnchors, anchors, false, false);

            // Set the offsets
            UiTransform2dInterface::Offsets offsets;
            UiTransform2dBus::EventResult(offsets, child, &UiTransform2dBus::Events::GetOffsets);
            switch (m_order)
            {
            case UiLayoutInterface::HorizontalOrder::LeftToRight:
                offsets.m_left = curX;
                curX += finalWidths[childIndex];
                offsets.m_right = curX;
                curX += m_spacing;
                break;
            case UiLayoutInterface::HorizontalOrder::RightToLeft:
                offsets.m_right = curX;
                curX -= finalWidths[childIndex];
                offsets.m_left = curX;
                curX -= m_spacing;
                break;
            default:
                AZ_Assert(0, "Unrecognized HorizontalOrder type in UiLayoutRowComponent");
                break;
            }
            UiTransform2dBus::Event(child, &UiTransform2dBus::Events::SetOffsets, offsets);

            childIndex++;
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiLayoutRowComponent::ApplyLayoutHeight(float availableHeight)
{
    // Get the child element cell heights
    UiLayoutHelpers::LayoutCellSizes layoutCells;
    UiLayoutHelpers::GetLayoutCellHeights(GetEntityId(), m_ignoreDefaultLayoutCells, layoutCells);
    int numChildren = static_cast<int>(layoutCells.size());
    if (numChildren > 0)
    {
        // Set the child elements' transform properties based on the calculated child heights
        AZStd::vector<AZ::EntityId> childEntityIds;
        UiElementBus::EventResult(childEntityIds, GetEntityId(), &UiElementBus::Events::GetChildEntityIds);

        int childIndex = 0;
        for (auto child : childEntityIds)
        {
            // Calculate occupied height
            float height = UiLayoutHelpers::CalculateSingleElementSize(layoutCells[childIndex], availableHeight);

            // Calculate alignment
            float alignmentOffset = UiLayoutHelpers::GetVerticalAlignmentOffset(m_childVAlignment, availableHeight, height);

            // Set the offsets
            UiTransform2dInterface::Offsets offsets;
            UiTransform2dBus::EventResult(offsets, child, &UiTransform2dBus::Events::GetOffsets);

            offsets.m_top = m_padding.m_top + alignmentOffset;
            offsets.m_bottom = offsets.m_top + height;

            UiTransform2dBus::Event(child, &UiTransform2dBus::Events::SetOffsets, offsets);

            childIndex++;
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// PRIVATE STATIC MEMBER FUNCTIONS
////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiLayoutRowComponent::VersionConverter(AZ::SerializeContext& context,
    AZ::SerializeContext::DataElementNode& classElement)
{
    // conversion from version 1 to 2:
    // - Need to convert default m_ignoreDefaultLayoutCells to true
    if (classElement.GetVersion() < 2)
    {
        // Add a flag and set it to a value that's different from the default value for new components
        const char* subElementName = "IgnoreDefaultLayoutCells";
        int newElementIndex = classElement.AddElement<bool>(context, subElementName);
        if (newElementIndex == -1)
        {
            // Error adding the new sub element
            AZ_Error("Serialization", false, "AddElement failed for element %s", subElementName);
            return false;
        }

        classElement.GetSubElement(newElementIndex).SetData(context, true);
    }

    return true;
}
