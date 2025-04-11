/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "UiLayoutColumnComponent.h"

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
UiLayoutColumnComponent::UiLayoutColumnComponent()
    : m_padding(UiLayoutInterface::Padding())
    , m_spacing(5)
    , m_order(UiLayoutInterface::VerticalOrder::TopToBottom)
    , m_childHAlignment(IDraw2d::HAlign::Left)
    , m_childVAlignment(IDraw2d::VAlign::Top)
    , m_ignoreDefaultLayoutCells(true)
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
UiLayoutColumnComponent::~UiLayoutColumnComponent()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiLayoutColumnComponent::ApplyLayoutWidth()
{
    // Get the layout rect inside the padding
    AZ::Vector2 layoutRectSize(0.0f, 0.0f);
    UiLayoutHelpers::GetSizeInsidePadding(GetEntityId(), m_padding, layoutRectSize);

    // First, calculate and set the child elements' widths.
    // Min/target/extra heights may depend on element widths
    ApplyLayoutWidth(layoutRectSize.GetX());
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiLayoutColumnComponent::ApplyLayoutHeight()
{
    // Get the layout rect inside the padding
    AZ::Vector2 layoutRectSize(0.0f, 0.0f);
    UiLayoutHelpers::GetSizeInsidePadding(GetEntityId(), m_padding, layoutRectSize);

    // Calculate and set the child elements' heights
    ApplyLayoutHeight(layoutRectSize.GetY());
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiLayoutColumnComponent::IsUsingLayoutCellsToCalculateLayout()
{
    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiLayoutColumnComponent::GetIgnoreDefaultLayoutCells()
{
    return m_ignoreDefaultLayoutCells;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiLayoutColumnComponent::SetIgnoreDefaultLayoutCells(bool ignoreDefaultLayoutCells)
{
    m_ignoreDefaultLayoutCells = ignoreDefaultLayoutCells;
    InvalidateLayout();
    InvalidateParentLayout();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
IDraw2d::HAlign UiLayoutColumnComponent::GetHorizontalChildAlignment()
{
    return m_childHAlignment;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiLayoutColumnComponent::SetHorizontalChildAlignment(IDraw2d::HAlign alignment)
{
    m_childHAlignment = alignment;
    InvalidateLayout();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
IDraw2d::VAlign UiLayoutColumnComponent::GetVerticalChildAlignment()
{
    return m_childVAlignment;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiLayoutColumnComponent::SetVerticalChildAlignment(IDraw2d::VAlign alignment)
{
    m_childVAlignment = alignment;
    InvalidateLayout();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiLayoutColumnComponent::IsControllingChild(AZ::EntityId childId)
{
    return UiLayoutHelpers::IsControllingChild(GetEntityId(), childId);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZ::Vector2 UiLayoutColumnComponent::GetSizeToFitChildElements(const AZ::Vector2& childElementSize, int numChildElements)
{
    AZ::Vector2 size(0.0f, 0.0f);

    if (numChildElements > 0)
    {
        size.SetY((childElementSize.GetY() * numChildElements) + (m_spacing * (numChildElements - 1)) + m_padding.m_top + m_padding.m_bottom);
    }
    else
    {
        size.SetY(0.0f);
    }

    // Check if anchors are together
    UiTransform2dInterface::Anchors anchors;
    UiTransform2dBus::EventResult(anchors, GetEntityId(), &UiTransform2dBus::Events::GetAnchors);
    if (anchors.m_left == anchors.m_right)
    {
        size.SetX(numChildElements > 0 ? childElementSize.GetX() : 0.0f);
    }
    else
    {
        // Anchors are apart, so width remains untouched
        AZ::Vector2 curSize(0.0f, 0.0f);
        UiTransformBus::EventResult(curSize, GetEntityId(), &UiTransformBus::Events::GetCanvasSpaceSizeNoScaleRotate);
        size.SetX(curSize.GetX());
    }

    return size;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
UiLayoutInterface::Padding UiLayoutColumnComponent::GetPadding()
{
    return m_padding;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiLayoutColumnComponent::SetPadding(UiLayoutInterface::Padding padding)
{
    m_padding = padding;
    InvalidateLayout();
    InvalidateParentLayout();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
float UiLayoutColumnComponent::GetSpacing()
{
    return m_spacing;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiLayoutColumnComponent::SetSpacing(float spacing)
{
    m_spacing = spacing;
    InvalidateLayout();
    InvalidateParentLayout();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
UiLayoutInterface::VerticalOrder UiLayoutColumnComponent::GetOrder()
{
    return m_order;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiLayoutColumnComponent::SetOrder(UiLayoutInterface::VerticalOrder order)
{
    m_order = order;
    InvalidateLayout();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
float UiLayoutColumnComponent::GetMinWidth()
{
    float width = 0.0f;

    // Minimum layout width is padding + maximum child element min width
    AZStd::vector<float> minWidths = UiLayoutHelpers::GetLayoutCellMinWidths(GetEntityId(), m_ignoreDefaultLayoutCells);
    if (minWidths.size() > 0)
    {
        width += m_padding.m_left + m_padding.m_right;

        float maxChildWidth = 0.0f;
        for (auto minWidth : minWidths)
        {
            if (maxChildWidth < minWidth)
            {
                maxChildWidth = minWidth;
            }
        }
        width += maxChildWidth;
    }

    return width;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
float UiLayoutColumnComponent::GetMinHeight()
{
    float height = 0.0f;

    // Minimum layout height is padding + spacing + sum of all child element min heights
    AZStd::vector<float> minHeights = UiLayoutHelpers::GetLayoutCellMinHeights(GetEntityId(), m_ignoreDefaultLayoutCells);
    if (minHeights.size() > 0)
    {
        height += m_padding.m_top + m_padding.m_bottom + (m_spacing * (minHeights.size() - 1));

        for (auto minHeight : minHeights)
        {
            height += minHeight;
        }
    }

    return height;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
float UiLayoutColumnComponent::GetTargetWidth(float /*maxWidth*/)
{
    float width = 0.0f;

    // Target layout width is padding + maximum child element target width
    AZStd::vector<float> targetWidths = UiLayoutHelpers::GetLayoutCellTargetWidths(GetEntityId(), m_ignoreDefaultLayoutCells);
    if (targetWidths.size() > 0)
    {
        width += m_padding.m_left + m_padding.m_right;

        float maxChildWidth = 0.0f;
        for (auto targetWidth : targetWidths)
        {
            if (maxChildWidth < targetWidth)
            {
                maxChildWidth = targetWidth;
            }
        }
        width += maxChildWidth;
    }

    return width;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
float UiLayoutColumnComponent::GetTargetHeight(float /*maxHeight*/)
{
    float height = 0.0f;

    // Target layout height is padding + spacing + sum of all child element target heights
    AZStd::vector<float> targetHeights = UiLayoutHelpers::GetLayoutCellTargetHeights(GetEntityId(), m_ignoreDefaultLayoutCells);
    if (targetHeights.size() > 0)
    {
        height += m_padding.m_top + m_padding.m_bottom + (m_spacing * (targetHeights.size() - 1));

        for (auto targetHeight : targetHeights)
        {
            height += targetHeight;
        }
    }

    return height;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
float UiLayoutColumnComponent::GetExtraWidthRatio()
{
    return 1.0f;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
float UiLayoutColumnComponent::GetExtraHeightRatio()
{
    return 1.0f;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiLayoutColumnComponent::OnCanvasSpaceRectChanged([[maybe_unused]] AZ::EntityId entityId, const UiTransformInterface::Rect& oldRect, const UiTransformInterface::Rect& newRect)
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

void UiLayoutColumnComponent::Reflect(AZ::ReflectContext* context)
{
    AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);

    if (serializeContext)
    {
        serializeContext->Class<UiLayoutColumnComponent, AZ::Component>()
            ->Version(2, &VersionConverter)
            ->Field("Padding", &UiLayoutColumnComponent::m_padding)
            ->Field("Spacing", &UiLayoutColumnComponent::m_spacing)
            ->Field("Order", &UiLayoutColumnComponent::m_order)
            ->Field("ChildHAlignment", &UiLayoutColumnComponent::m_childHAlignment)
            ->Field("ChildVAlignment", &UiLayoutColumnComponent::m_childVAlignment)
            ->Field("IgnoreDefaultLayoutCells", &UiLayoutColumnComponent::m_ignoreDefaultLayoutCells);

        AZ::EditContext* ec = serializeContext->GetEditContext();
        if (ec)
        {
            auto editInfo = ec->Class<UiLayoutColumnComponent>("LayoutColumn", "A layout component that arranges its children in a column");

            editInfo->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                ->Attribute(AZ::Edit::Attributes::Category, "UI")
                ->Attribute(AZ::Edit::Attributes::Icon, "Editor/Icons/Components/UiLayoutColumn.png")
                ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Editor/Icons/Components/Viewport/UiLayoutColumn.png")
                ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("UI"))
                ->Attribute(AZ::Edit::Attributes::AutoExpand, true);

            editInfo->DataElement(AZ::Edit::UIHandlers::LayoutPadding, &UiLayoutColumnComponent::m_padding, "Padding", "The layout padding")
                ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::Show) // needed because sub-elements are hidden
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, &UiLayoutColumnComponent::InvalidateLayout)
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, &UiLayoutColumnComponent::InvalidateParentLayout)
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, &UiLayoutColumnComponent::CheckLayoutFitterAndRefreshEditorTransformProperties);

            editInfo->DataElement(AZ::Edit::UIHandlers::SpinBox, &UiLayoutColumnComponent::m_spacing, "Spacing", "The spacing between children")
                ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                ->Attribute(AZ::Edit::Attributes::Step, 1.0f)
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, &UiLayoutColumnComponent::InvalidateLayout)
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, &UiLayoutColumnComponent::InvalidateParentLayout)
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, &UiLayoutColumnComponent::CheckLayoutFitterAndRefreshEditorTransformProperties);

            editInfo->DataElement(AZ::Edit::UIHandlers::ComboBox, &UiLayoutColumnComponent::m_order, "Order", "Which direction the column fills")
                ->EnumAttribute(UiLayoutInterface::VerticalOrder::TopToBottom, "Top to bottom")
                ->EnumAttribute(UiLayoutInterface::VerticalOrder::BottomToTop, "Bottom to top")
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, &UiLayoutColumnComponent::InvalidateLayout);

            editInfo->DataElement(AZ::Edit::UIHandlers::CheckBox, &UiLayoutColumnComponent::m_ignoreDefaultLayoutCells, "Ignore Default Cells",
                "When checked, fixed default layout cell values are used for child elements with no LayoutCell\n"
                "component rather than using defaults calculated by other components on the child.")
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, &UiLayoutColumnComponent::InvalidateLayout)
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, &UiLayoutColumnComponent::InvalidateParentLayout)
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, &UiLayoutColumnComponent::CheckLayoutFitterAndRefreshEditorTransformProperties);

            // Alignment
            {
                editInfo->ClassElement(AZ::Edit::ClassElements::Group, "Child Alignment")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true);

                editInfo->DataElement(AZ::Edit::UIHandlers::ComboBox, &UiLayoutColumnComponent::m_childHAlignment, "Horizontal",
                    "How to align the children if they don't take up all the available width")
                    ->EnumAttribute(IDraw2d::HAlign::Left, "Left")
                    ->EnumAttribute(IDraw2d::HAlign::Center, "Center")
                    ->EnumAttribute(IDraw2d::HAlign::Right, "Right")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &UiLayoutColumnComponent::InvalidateLayout);
                editInfo->DataElement(AZ::Edit::UIHandlers::ComboBox, &UiLayoutColumnComponent::m_childVAlignment, "Vertical",
                    "How to align the children if they don't take up all the available height")
                    ->EnumAttribute(IDraw2d::VAlign::Top, "Top")
                    ->EnumAttribute(IDraw2d::VAlign::Center, "Center")
                    ->EnumAttribute(IDraw2d::VAlign::Bottom, "Bottom")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &UiLayoutColumnComponent::InvalidateLayout);
            }
        }
    }

    AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context);
    if (behaviorContext)
    {
        behaviorContext->EBus<UiLayoutColumnBus>("UiLayoutColumnBus")
            ->Event("GetPadding", &UiLayoutColumnBus::Events::GetPadding)
            ->Event("SetPadding", &UiLayoutColumnBus::Events::SetPadding)
            ->Event("GetSpacing", &UiLayoutColumnBus::Events::GetSpacing)
            ->Event("SetSpacing", &UiLayoutColumnBus::Events::SetSpacing)
            ->Event("GetOrder", &UiLayoutColumnBus::Events::GetOrder)
            ->Event("SetOrder", &UiLayoutColumnBus::Events::SetOrder)
            ->VirtualProperty("Padding", "GetPadding", "SetPadding")
            ->VirtualProperty("Spacing", "GetSpacing", "SetSpacing");

        behaviorContext->Class<UiLayoutColumnComponent>()->RequestBus("UiLayoutColumnBus");
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// PROTECTED MEMBER FUNCTIONS
////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiLayoutColumnComponent::Activate()
{
    UiLayoutBus::Handler::BusConnect(m_entity->GetId());
    UiLayoutControllerBus::Handler::BusConnect(m_entity->GetId());
    UiLayoutColumnBus::Handler::BusConnect(m_entity->GetId());
    UiLayoutCellDefaultBus::Handler::BusConnect(m_entity->GetId());
    UiTransformChangeNotificationBus::Handler::BusConnect(m_entity->GetId());

    // If this is the first time the entity has been activated this has no effect since the canvas
    // is not known. But if a LayoutColumn component has just been pasted onto an existing entity
    // we need to invalidate the layout in case that affects things.
    InvalidateLayout();
    InvalidateParentLayout();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiLayoutColumnComponent::Deactivate()
{
    UiLayoutBus::Handler::BusDisconnect();
    UiLayoutControllerBus::Handler::BusDisconnect();
    UiLayoutColumnBus::Handler::BusDisconnect();
    UiLayoutCellDefaultBus::Handler::BusDisconnect();
    UiTransformChangeNotificationBus::Handler::BusDisconnect();

    // We could be about to remove this component and then reactivate the entity
    // which could affect the layout if there is a parent layout component
    InvalidateLayout();
    InvalidateParentLayout();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiLayoutColumnComponent::InvalidateLayout()
{
    UiLayoutHelpers::InvalidateLayout(GetEntityId());
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiLayoutColumnComponent::InvalidateParentLayout()
{
    UiLayoutHelpers::InvalidateParentLayout(GetEntityId());
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiLayoutColumnComponent::CheckLayoutFitterAndRefreshEditorTransformProperties() const
{
    UiLayoutHelpers::CheckFitterAndRefreshEditorTransformProperties(GetEntityId());
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiLayoutColumnComponent::ApplyLayoutWidth(float availableWidth)
{
    // Get the child element cell widths
    UiLayoutHelpers::LayoutCellSizes layoutCells;
    UiLayoutHelpers::GetLayoutCellWidths(GetEntityId(), m_ignoreDefaultLayoutCells, layoutCells);
    int numChildren = static_cast<int>(layoutCells.size());
    if (numChildren > 0)
    {
        // Set the child elements' transform properties based on the calculated child widths
        UiTransform2dInterface::Anchors anchors(0.0f, 0.0f, 0.0f, 0.0f);

        AZStd::vector<AZ::EntityId> childEntityIds;
        UiElementBus::EventResult(childEntityIds, GetEntityId(), &UiElementBus::Events::GetChildEntityIds);
        int childIndex = 0;
        for (auto child : childEntityIds)
        {
            // Set the anchors
            UiTransform2dBus::Event(child, &UiTransform2dBus::Events::SetAnchors, anchors, false, false);

            // Calculate occupied width
            float width = UiLayoutHelpers::CalculateSingleElementSize(layoutCells[childIndex], availableWidth);

            // Calculate alignment
            float alignmentOffset = UiLayoutHelpers::GetHorizontalAlignmentOffset(m_childHAlignment, availableWidth, width);

            // Set the offsets
            UiTransform2dInterface::Offsets offsets;
            UiTransform2dBus::EventResult(offsets, child, &UiTransform2dBus::Events::GetOffsets);

            offsets.m_left = m_padding.m_left + alignmentOffset;
            offsets.m_right = offsets.m_left + width;

            UiTransform2dBus::Event(child, &UiTransform2dBus::Events::SetOffsets, offsets);

            childIndex++;
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiLayoutColumnComponent::ApplyLayoutHeight(float availableHeight)
{
    // Get the child element cell heights
    UiLayoutHelpers::LayoutCellSizes layoutCells;
    UiLayoutHelpers::GetLayoutCellHeights(GetEntityId(), m_ignoreDefaultLayoutCells, layoutCells);
    int numChildren = static_cast<int>(layoutCells.size());
    if (numChildren > 0)
    {
        // Calculate child heights
        AZStd::vector<float> finalHeights(numChildren, 0.0f);
        UiLayoutHelpers::CalculateElementSizes(layoutCells, availableHeight, m_spacing, finalHeights);

        // Calculate occupied height
        float childrenRectHeight = (numChildren - 1) * m_spacing;
        for (auto height : finalHeights)
        {
            childrenRectHeight += height;
        }

        // Calculate alignment
        float alignmentOffset = UiLayoutHelpers::GetVerticalAlignmentOffset(m_childVAlignment, availableHeight, childrenRectHeight);

        // Set the child elements' transform properties based on the calculated child heights
        AZStd::vector<AZ::EntityId> childEntityIds;
        UiElementBus::EventResult(childEntityIds, GetEntityId(), &UiElementBus::Events::GetChildEntityIds);

        float curY = alignmentOffset;
        switch (m_order)
        {
        case UiLayoutInterface::VerticalOrder::TopToBottom:
            curY += m_padding.m_top;
            break;
        case UiLayoutInterface::VerticalOrder::BottomToTop:
            curY += m_padding.m_top + childrenRectHeight;
            break;
        default:
            AZ_Assert(0, "Unrecognized VerticalOrder type in UiLayoutColumnComponent");
            break;
        }

        int childIndex = 0;
        for (auto child : childEntityIds)
        {
            // Set the offsets
            UiTransform2dInterface::Offsets offsets;
            UiTransform2dBus::EventResult(offsets, child, &UiTransform2dBus::Events::GetOffsets);
            switch (m_order)
            {
            case UiLayoutInterface::VerticalOrder::TopToBottom:
                offsets.m_top = curY;
                curY += finalHeights[childIndex];
                offsets.m_bottom = curY;
                curY += m_spacing;
                break;
            case UiLayoutInterface::VerticalOrder::BottomToTop:
                offsets.m_bottom = curY;
                curY -= finalHeights[childIndex];
                offsets.m_top = curY;
                curY -= m_spacing;
                break;
            default:
                AZ_Assert(0, "Unrecognized VerticalOrder type in UiLayoutColumnComponent");
                break;
            }
            UiTransform2dBus::Event(child, &UiTransform2dBus::Events::SetOffsets, offsets);

            childIndex++;
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// PRIVATE STATIC MEMBER FUNCTIONS
////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiLayoutColumnComponent::VersionConverter(AZ::SerializeContext& context,
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
