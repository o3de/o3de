/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "UiLayoutGridComponent.h"

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/RTTI/BehaviorContext.h>

#include <LyShine/Bus/UiElementBus.h>
#include <LyShine/UiSerializeHelpers.h>

#include "UiSerialize.h"

////////////////////////////////////////////////////////////////////////////////////////////////////
// PUBLIC MEMBER FUNCTIONS
////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////
UiLayoutGridComponent::UiLayoutGridComponent()
    : m_padding(UiLayoutInterface::Padding())
    , m_spacing(5.0f, 5.0f)
    , m_cellSize(30.0f, 30.0f)
    , m_horizontalOrder(UiLayoutInterface::HorizontalOrder::LeftToRight)
    , m_verticalOrder(UiLayoutInterface::VerticalOrder::TopToBottom)
    , m_startingDirection(StartingDirection::HorizontalOrder)
    , m_childHAlignment(IDraw2d::HAlign::Left)
    , m_childVAlignment(IDraw2d::VAlign::Top)
    , m_origOffsetsInitialized(false)
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
UiLayoutGridComponent::~UiLayoutGridComponent()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiLayoutGridComponent::ApplyLayoutWidth()
{
    // Set the child widths. The positioning will be applied in ApplyLayoutHeight after the grid's
    // height is valid. This is because a grid needs to know its width or height to layout its
    // children depending on fill direction

    UiTransform2dInterface::Anchors anchors(0.0f, 0.0f, 0.0f, 0.0f);
    UiTransform2dInterface::Offsets offsets(0.0f, 0.0f, m_cellSize.GetX(), m_cellSize.GetY());

    AZStd::vector<AZ::EntityId> childEntityIds;
    UiElementBus::EventResult(childEntityIds, GetEntityId(), &UiElementBus::Events::GetChildEntityIds);
    for (auto child : childEntityIds)
    {
        // Set the anchors
        UiTransform2dBus::Event(child, &UiTransform2dBus::Events::SetAnchors, anchors, false, false);

        // Set the offsets
        UiTransform2dBus::Event(child, &UiTransform2dBus::Events::SetOffsets, offsets);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiLayoutGridComponent::ApplyLayoutHeight()
{
    int numChildren = 0;
    UiElementBus::EventResult(numChildren, GetEntityId(), &UiElementBus::Events::GetNumChildElements);
    if (numChildren > 0)
    {
        // Get the layout rect inside the padding
        AZ::Vector2 layoutRectSize(0.0f, 0.0f);
        UiLayoutHelpers::GetSizeInsidePadding(GetEntityId(), m_padding, layoutRectSize);

        // Calculate occupied width/height
        AZ::Vector2 childrenRectSize = GetChildrenBoundingRectSize(m_cellSize, numChildren);

        // Calculate alignment
        float hAlignmentOffset = UiLayoutHelpers::GetHorizontalAlignmentOffset(m_childHAlignment, layoutRectSize.GetX(), childrenRectSize.GetX());
        float vAlignmentOffset = UiLayoutHelpers::GetVerticalAlignmentOffset(m_childVAlignment, layoutRectSize.GetY(), childrenRectSize.GetY());

        int numColumns = 0, numRows = 0;

        switch (m_startingDirection)
        {
        case StartingDirection::HorizontalOrder:
            numColumns = static_cast<int>(floorf((layoutRectSize.GetX() + m_spacing.GetX()) / (m_cellSize.GetX() + m_spacing.GetX())));
            numColumns = max(numColumns, 1);
            break;
        case StartingDirection::VerticalOrder:
            numRows = static_cast<int>(floorf((layoutRectSize.GetY() + m_spacing.GetY()) / (m_cellSize.GetY() + m_spacing.GetY())));
            numRows = max(numRows, 1);
            break;
        default:
            AZ_Assert(0, "Unrecognized Direction type in UiLayoutGridComponent");
            break;
        }

        UiTransform2dInterface::Anchors anchors(0.0f, 0.0f, 0.0f, 0.0f);
        UiTransform2dInterface::Offsets offsets(0.0f, 0.0f, 0.0f, 0.0f);

        AZStd::vector<AZ::EntityId> childEntityIds;
        UiElementBus::EventResult(childEntityIds, GetEntityId(), &UiElementBus::Events::GetChildEntityIds);
        int childIndex = 0;
        int columnIndex = 0;
        int rowIndex = 0;
        for (auto child : childEntityIds)
        {
            // Set the anchors
            UiTransform2dBus::Event(child, &UiTransform2dBus::Events::SetAnchors, anchors, false, false);

            // Set the offsets

            switch (m_startingDirection)
            {
            case StartingDirection::HorizontalOrder:
                columnIndex = childIndex % numColumns;
                rowIndex = childIndex / numColumns;
                break;
            case StartingDirection::VerticalOrder:
                columnIndex = childIndex / numRows;
                rowIndex = childIndex % numRows;
                break;
            default:
                AZ_Assert(0, "Unrecognized Direction type in UiLayoutGridComponent");
                break;
            }

            switch (m_horizontalOrder)
            {
            case UiLayoutInterface::HorizontalOrder::LeftToRight:
                offsets.m_left = m_padding.m_left + columnIndex * (m_cellSize.GetX() + m_spacing.GetX());
                offsets.m_right = offsets.m_left + m_cellSize.GetX();
                break;
            case UiLayoutInterface::HorizontalOrder::RightToLeft:
                offsets.m_right = m_padding.m_left + childrenRectSize.GetX() - columnIndex * (m_cellSize.GetX() + m_spacing.GetX());
                offsets.m_left = offsets.m_right - m_cellSize.GetX();
                break;
            default:
                AZ_Assert(0, "Unrecognized HorizontalOrder type in UiLayoutGridComponent");
                break;
            }

            switch (m_verticalOrder)
            {
            case UiLayoutInterface::VerticalOrder::TopToBottom:
                offsets.m_top = m_padding.m_top + rowIndex * (m_cellSize.GetY() + m_spacing.GetY());
                offsets.m_bottom = offsets.m_top + m_cellSize.GetY();
                break;
            case UiLayoutInterface::VerticalOrder::BottomToTop:
                offsets.m_bottom = m_padding.m_top + childrenRectSize.GetY() - rowIndex * (m_cellSize.GetY() + m_spacing.GetY());
                offsets.m_top = offsets.m_bottom - m_cellSize.GetY();
                break;
            default:
                AZ_Assert(0, "Unrecognized VerticalOrder type in UiLayoutGridComponent");
                break;
            }

            offsets.m_left += hAlignmentOffset;
            offsets.m_right += hAlignmentOffset;
            offsets.m_top += vAlignmentOffset;
            offsets.m_bottom += vAlignmentOffset;

            UiTransform2dBus::Event(child, &UiTransform2dBus::Events::SetOffsets, offsets);

            childIndex++;
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiLayoutGridComponent::IsUsingLayoutCellsToCalculateLayout()
{
    return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiLayoutGridComponent::GetIgnoreDefaultLayoutCells()
{
    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiLayoutGridComponent::SetIgnoreDefaultLayoutCells([[maybe_unused]] bool ignoreDefaultLayoutCells)
{
    // Layout cells are not used by this layout component
}

////////////////////////////////////////////////////////////////////////////////////////////////////
IDraw2d::HAlign UiLayoutGridComponent::GetHorizontalChildAlignment()
{
    return m_childHAlignment;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiLayoutGridComponent::SetHorizontalChildAlignment(IDraw2d::HAlign alignment)
{
    m_childHAlignment = alignment;
    InvalidateLayout();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
IDraw2d::VAlign UiLayoutGridComponent::GetVerticalChildAlignment()
{
    return m_childVAlignment;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiLayoutGridComponent::SetVerticalChildAlignment(IDraw2d::VAlign alignment)
{
    m_childVAlignment = alignment;
    InvalidateLayout();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiLayoutGridComponent::IsControllingChild(AZ::EntityId childId)
{
    return UiLayoutHelpers::IsControllingChild(GetEntityId(), childId);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZ::Vector2 UiLayoutGridComponent::GetSizeToFitChildElements(const AZ::Vector2& childElementSize, int numChildElements)
{
    AZ::Vector2 size(0.0f, 0.0f);

    // Initialize original offsets
    if (!m_origOffsetsInitialized)
    {
        m_origOffsetsInitialized = true;
        UiTransform2dBus::EventResult(m_origOffsets, GetEntityId(), &UiTransform2dBus::Events::GetOffsets);
    }

    if (numChildElements > 0)
    {
        // Calculate a layout rect size that is used to determine the number of rows and columns.
        // Since the element size may change after this call, use the original offsets to get the layout rect size
        UiTransform2dInterface::Offsets realOffsets;
        UiTransform2dBus::EventResult(realOffsets, GetEntityId(), &UiTransform2dBus::Events::GetOffsets);
        UiTransform2dBus::Event(GetEntityId(), &UiTransform2dBus::Events::SetOffsets, m_origOffsets);

        size = GetChildrenBoundingRectSize(childElementSize, numChildElements);

        // Add padding
        size.SetX(size.GetX() + m_padding.m_left + m_padding.m_right);
        size.SetY(size.GetY() + m_padding.m_top + m_padding.m_bottom);

        // In order for the number of rows and columns to remain the same after resizing to this new size, the
        // new size must match the size retrieved from GetCanvasSpacePointsNoScaleRotate. To accommodate
        // for slight variations, add a small value to ensure that the child element positions won't change
        const float epsilon = 0.01f;
        size += AZ::Vector2(epsilon, epsilon);

        UiTransform2dBus::Event(GetEntityId(), &UiTransform2dBus::Events::SetOffsets, realOffsets);
    }

    return size;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
UiLayoutInterface::Padding UiLayoutGridComponent::GetPadding()
{
    return m_padding;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiLayoutGridComponent::SetPadding(UiLayoutInterface::Padding padding)
{
    m_padding = padding;
    InvalidateLayout();
    InvalidateParentLayout();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZ::Vector2 UiLayoutGridComponent::GetSpacing()
{
    return m_spacing;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiLayoutGridComponent::SetSpacing(AZ::Vector2 spacing)
{
    m_spacing = spacing;
    InvalidateLayout();
    InvalidateParentLayout();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZ::Vector2 UiLayoutGridComponent::GetCellSize()
{
    return m_cellSize;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiLayoutGridComponent::SetCellSize(AZ::Vector2 size)
{
    m_cellSize = size;
    InvalidateLayout();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
UiLayoutInterface::HorizontalOrder UiLayoutGridComponent::GetHorizontalOrder()
{
    return m_horizontalOrder;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiLayoutGridComponent::SetHorizontalOrder(UiLayoutInterface::HorizontalOrder order)
{
    m_horizontalOrder = order;
    InvalidateLayout();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
UiLayoutInterface::VerticalOrder UiLayoutGridComponent::GetVerticalOrder()
{
    return m_verticalOrder;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiLayoutGridComponent::SetVerticalOrder(UiLayoutInterface::VerticalOrder order)
{
    m_verticalOrder = order;
    InvalidateLayout();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
UiLayoutGridComponent::StartingDirection UiLayoutGridComponent::GetStartingDirection()
{
    return m_startingDirection;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiLayoutGridComponent::SetStartingDirection(UiLayoutGridComponent::StartingDirection direction)
{
    m_startingDirection = direction;
    InvalidateLayout();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
float UiLayoutGridComponent::GetMinWidth()
{
    return 0.0f;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
float UiLayoutGridComponent::GetMinHeight()
{
    return 0.0f;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
float UiLayoutGridComponent::GetTargetWidth(float maxWidth)
{
    int numChildElements = 0;
    UiElementBus::EventResult(numChildElements, GetEntityId(), &UiElementBus::Events::GetNumChildElements);

    if (numChildElements == 0)
    {
        return 0.0f;
    }

    // Calculate number of columns
    int numColumns = 0;
    if (LyShine::IsUiLayoutCellSizeSpecified(maxWidth))
    {
        const int paddingWidth = m_padding.m_left + m_padding.m_right;
        const float availableWidthForCells = maxWidth - paddingWidth;
        if (availableWidthForCells > 0.0f)
        {
            const float cellAndSpacingWidth = m_cellSize.GetX() + m_spacing.GetX();
            const int numAvailableColumns = cellAndSpacingWidth > 0.0f ? static_cast<int>((availableWidthForCells + m_spacing.GetX()) / cellAndSpacingWidth) : 1;
            numColumns = AZ::GetMin(numAvailableColumns, numChildElements);
        }

        if (numColumns == 0)
        {
            return 0.0f;
        }
    }
    else
    {
        // Since element width/height is unknown at this point, make target width resemble a square grid
        numColumns = static_cast<int>(ceil(sqrt(numChildElements)));
    }

    float width = m_padding.m_left + m_padding.m_right + (numColumns * m_cellSize.GetX()) + ((numColumns - 1) * m_spacing.GetX());

    // In order for the number of columns to remain the same after resizing to this new size, the
    // new size must match the size retrieved from GetCanvasSpacePointsNoScaleRotate. To accommodate
    // for slight variations, add a small value to ensure that the same number of cells fit per row
    // after the element has been resized to this target size
    const float epsilon = 0.01f;
    width += epsilon;

    return width;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
float UiLayoutGridComponent::GetTargetHeight(float /*maxHeight*/)
{
    int numChildElements = 0;
    UiElementBus::EventResult(numChildElements, GetEntityId(), &UiElementBus::Events::GetNumChildElements);

    if (numChildElements == 0)
    {
        return 0.0f;
    }

    // Check how many elements fit in a row
    AZ::Vector2 rectSize(0.0f, 0.0f);
    UiTransformBus::EventResult(rectSize, GetEntityId(), &UiTransformBus::Events::GetCanvasSpaceSizeNoScaleRotate);

    // At least one child must fit in each row
    int numElementsPerRow = 1;
    float additionalElementWidth = m_spacing.GetX() + m_cellSize.GetX();
    if (additionalElementWidth > 0.0f)
    {
        float availableWidthForAdditionalElements = AZ::GetMax(0.0f, rectSize.GetX() - (m_padding.m_left + m_padding.m_right + m_cellSize.GetX()));
        numElementsPerRow += static_cast<int>(availableWidthForAdditionalElements / additionalElementWidth);
    }
    else
    {
        numElementsPerRow = numChildElements;
    }

    // Calculate number of rows
    int numRows = static_cast<int>(ceil(static_cast<float>(numChildElements) / numElementsPerRow));

    float height = m_padding.m_top + m_padding.m_bottom + (numRows * m_cellSize.GetY()) + ((numRows - 1) * m_spacing.GetY());
    return height;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
float UiLayoutGridComponent::GetExtraWidthRatio()
{
    return 1.0f;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
float UiLayoutGridComponent::GetExtraHeightRatio()
{
    return 1.0f;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiLayoutGridComponent::OnCanvasSpaceRectChanged([[maybe_unused]] AZ::EntityId entityId, const UiTransformInterface::Rect& oldRect, const UiTransformInterface::Rect& newRect)
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

void UiLayoutGridComponent::Reflect(AZ::ReflectContext* context)
{
    AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);

    if (serializeContext)
    {
        serializeContext->Class<UiLayoutGridComponent, AZ::Component>()
            ->Version(2, &VersionConverter)
            ->Field("Padding", &UiLayoutGridComponent::m_padding)
            ->Field("Spacing", &UiLayoutGridComponent::m_spacing)
            ->Field("CellSize", &UiLayoutGridComponent::m_cellSize)
            ->Field("HorizontalOrder", &UiLayoutGridComponent::m_horizontalOrder)
            ->Field("VerticalOrder", &UiLayoutGridComponent::m_verticalOrder)
            ->Field("StartingWith", &UiLayoutGridComponent::m_startingDirection)
            ->Field("ChildHAlignment", &UiLayoutGridComponent::m_childHAlignment)
            ->Field("ChildVAlignment", &UiLayoutGridComponent::m_childVAlignment);

        AZ::EditContext* ec = serializeContext->GetEditContext();
        if (ec)
        {
            auto editInfo = ec->Class<UiLayoutGridComponent>("LayoutGrid", "A layout component that arranges its children in a grid");

            editInfo->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                ->Attribute(AZ::Edit::Attributes::Category, "UI")
                ->Attribute(AZ::Edit::Attributes::Icon, "Editor/Icons/Components/UiLayoutGrid.png")
                ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Editor/Icons/Components/Viewport/UiLayoutGrid.png")
                ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("UI"))
                ->Attribute(AZ::Edit::Attributes::AutoExpand, true);

            editInfo->DataElement(AZ::Edit::UIHandlers::LayoutPadding, &UiLayoutGridComponent::m_padding, "Padding", "The layout padding")
                ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::Show) // needed because sub-elements are hidden
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, &UiLayoutGridComponent::InvalidateLayout)
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, &UiLayoutGridComponent::InvalidateParentLayout)
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, &UiLayoutGridComponent::CheckLayoutFitterAndRefreshEditorTransformProperties);

            editInfo->DataElement(0, &UiLayoutGridComponent::m_spacing, "Spacing", "The spacing between children")
                ->Attribute(AZ::Edit::Attributes::LabelForX, "Horizontal")
                ->Attribute(AZ::Edit::Attributes::LabelForY, "Vertical")
                ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::Show) // needed because sub-elements are hidden
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, &UiLayoutGridComponent::InvalidateLayout)
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, &UiLayoutGridComponent::InvalidateParentLayout)
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, &UiLayoutGridComponent::CheckLayoutFitterAndRefreshEditorTransformProperties);

            editInfo->DataElement(0, &UiLayoutGridComponent::m_cellSize, "Cell size", "The size of the cells")
                ->Attribute(AZ::Edit::Attributes::LabelForX, "Width")
                ->Attribute(AZ::Edit::Attributes::LabelForY, "Height")
                ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::Show) // needed because sub-elements are hidden
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, &UiLayoutGridComponent::InvalidateLayout)
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, &UiLayoutGridComponent::InvalidateParentLayout)
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, &UiLayoutGridComponent::CheckLayoutFitterAndRefreshEditorTransformProperties);

            // Order group
            {
                editInfo->ClassElement(AZ::Edit::ClassElements::Group, "Order")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true);

                editInfo->DataElement(AZ::Edit::UIHandlers::ComboBox, &UiLayoutGridComponent::m_horizontalOrder, "Horizontal",
                    "Which direction the rows fill")
                    ->EnumAttribute(UiLayoutInterface::HorizontalOrder::LeftToRight, "Left to right")
                    ->EnumAttribute(UiLayoutInterface::HorizontalOrder::RightToLeft, "Right to left")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &UiLayoutGridComponent::InvalidateLayout);

                editInfo->DataElement(AZ::Edit::UIHandlers::ComboBox, &UiLayoutGridComponent::m_verticalOrder, "Vertical",
                    "Which direction the columns fill")
                    ->EnumAttribute(UiLayoutInterface::VerticalOrder::TopToBottom, "Top to bottom")
                    ->EnumAttribute(UiLayoutInterface::VerticalOrder::BottomToTop, "Bottom to top")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &UiLayoutGridComponent::InvalidateLayout);

                editInfo->DataElement(AZ::Edit::UIHandlers::ComboBox, &UiLayoutGridComponent::m_startingDirection, "Starting with",
                    "Start filling horizontally or vertically")
                    ->EnumAttribute(UiLayoutGridInterface::StartingDirection::HorizontalOrder, "Horizontal")
                    ->EnumAttribute(UiLayoutGridInterface::StartingDirection::VerticalOrder, "Vertical")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &UiLayoutGridComponent::InvalidateLayout);
            }

            // Alignment
            {
                editInfo->ClassElement(AZ::Edit::ClassElements::Group, "Child Alignment")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true);

                editInfo->DataElement(AZ::Edit::UIHandlers::ComboBox, &UiLayoutGridComponent::m_childHAlignment, "Horizontal",
                    "How to align the children if they don't take up all the available width")
                    ->EnumAttribute(IDraw2d::HAlign::Left, "Left")
                    ->EnumAttribute(IDraw2d::HAlign::Center, "Center")
                    ->EnumAttribute(IDraw2d::HAlign::Right, "Right")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &UiLayoutGridComponent::InvalidateLayout);
                editInfo->DataElement(AZ::Edit::UIHandlers::ComboBox, &UiLayoutGridComponent::m_childVAlignment, "Vertical",
                    "How to align the children if they don't take up all the available height")
                    ->EnumAttribute(IDraw2d::VAlign::Top, "Top")
                    ->EnumAttribute(IDraw2d::VAlign::Center, "Center")
                    ->EnumAttribute(IDraw2d::VAlign::Bottom, "Bottom")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &UiLayoutGridComponent::InvalidateLayout);
            }
        }
    }

    AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context);
    if (behaviorContext)
    {
        behaviorContext->Enum<(int)UiLayoutGridInterface::StartingDirection::HorizontalOrder>("eUiLayoutGridStartingDirection_HorizontalOrder")
            ->Enum<(int)UiLayoutGridInterface::StartingDirection::VerticalOrder>("eUiLayoutGridStartingDirection_VerticalOrder");

        behaviorContext->EBus<UiLayoutGridBus>("UiLayoutGridBus")
            ->Event("GetPadding", &UiLayoutGridBus::Events::GetPadding)
            ->Event("SetPadding", &UiLayoutGridBus::Events::SetPadding)
            ->Event("GetSpacing", &UiLayoutGridBus::Events::GetSpacing)
            ->Event("SetSpacing", &UiLayoutGridBus::Events::SetSpacing)
            ->Event("GetCellSize", &UiLayoutGridBus::Events::GetCellSize)
            ->Event("SetCellSize", &UiLayoutGridBus::Events::SetCellSize)
            ->Event("GetHorizontalOrder", &UiLayoutGridBus::Events::GetHorizontalOrder)
            ->Event("SetHorizontalOrder", &UiLayoutGridBus::Events::SetHorizontalOrder)
            ->Event("GetVerticalOrder", &UiLayoutGridBus::Events::GetVerticalOrder)
            ->Event("SetVerticalOrder", &UiLayoutGridBus::Events::SetVerticalOrder)
            ->Event("GetStartingDirection", &UiLayoutGridBus::Events::GetStartingDirection)
            ->Event("SetStartingDirection", &UiLayoutGridBus::Events::SetStartingDirection);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// PROTECTED MEMBER FUNCTIONS
////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiLayoutGridComponent::Activate()
{
    UiLayoutBus::Handler::BusConnect(m_entity->GetId());
    UiLayoutControllerBus::Handler::BusConnect(m_entity->GetId());
    UiLayoutGridBus::Handler::BusConnect(m_entity->GetId());
    UiLayoutCellDefaultBus::Handler::BusConnect(m_entity->GetId());
    UiTransformChangeNotificationBus::Handler::BusConnect(m_entity->GetId());

    // If this is the first time the entity has been activated this has no effect since the canvas
    // is not known. But if a LayoutGrid component has just been pasted onto an existing entity
    // we need to invalidate the layout in case that affects things.
    InvalidateLayout();
    InvalidateParentLayout();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiLayoutGridComponent::Deactivate()
{
    UiLayoutBus::Handler::BusDisconnect();
    UiLayoutControllerBus::Handler::BusDisconnect();
    UiLayoutGridBus::Handler::BusDisconnect();
    UiLayoutCellDefaultBus::Handler::BusDisconnect();
    UiTransformChangeNotificationBus::Handler::BusDisconnect();

    // We could be about to remove this component and then reactivate the entity
    // which could affect the layout if there is a parent layout component
    InvalidateLayout();
    InvalidateParentLayout();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZ::Vector2 UiLayoutGridComponent::GetChildrenBoundingRectSize(const AZ::Vector2 childElementSize, int numChildElements)
{
    AZ::Vector2 size(0.0f, 0.0f);

    if (numChildElements > 0)
    {
        // Get the layout rect inside the padding
        AZ::Vector2 layoutRectSize(0.0f, 0.0f);
        UiLayoutHelpers::GetSizeInsidePadding(GetEntityId(), m_padding, layoutRectSize);

        // Calculate number of rows and columns
        int numColumns = 0;
        int numRows = 0;
        switch (m_startingDirection)
        {
        case StartingDirection::HorizontalOrder:
            numColumns = static_cast<int>(floorf((layoutRectSize.GetX() + m_spacing.GetX()) / (childElementSize.GetX() + m_spacing.GetX())));
            Limit(numColumns, 1, numChildElements);
            numRows = static_cast<int>(ceil(static_cast<float>(numChildElements) / numColumns));
            numRows = max(numRows, 1);
            break;
        case StartingDirection::VerticalOrder:
            numRows = static_cast<int>(floorf((layoutRectSize.GetY() + m_spacing.GetY()) / (childElementSize.GetY() + m_spacing.GetY())));
            Limit(numRows, 1, numChildElements);
            numColumns = static_cast<int>(ceil(static_cast<float>(numChildElements) / numRows));
            numColumns = max(numColumns, 1);
            break;
        default:
            AZ_Assert(0, "Unrecognized Direction type in UiLayoutGridComponent");
            break;
        }

        // Calculate the minimum size to cover the rows and columns with spacing
        size.SetX((numColumns * childElementSize.GetX()) + (m_spacing.GetX() * (numColumns - 1)));
        size.SetY((numRows * childElementSize.GetY()) + (m_spacing.GetY() * (numRows - 1)));
    }

    return size;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiLayoutGridComponent::InvalidateLayout()
{
    UiLayoutHelpers::InvalidateLayout(GetEntityId());
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiLayoutGridComponent::InvalidateParentLayout()
{
    UiLayoutHelpers::InvalidateParentLayout(GetEntityId());
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiLayoutGridComponent::CheckLayoutFitterAndRefreshEditorTransformProperties() const
{
    UiLayoutHelpers::CheckFitterAndRefreshEditorTransformProperties(GetEntityId());
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// PRIVATE STATIC MEMBER FUNCTIONS
////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiLayoutGridComponent::VersionConverter(AZ::SerializeContext& context,
    AZ::SerializeContext::DataElementNode& classElement)
{
    // conversion from version 1:
    // - Need to convert Vec2 to AZ::Vector2
    if (classElement.GetVersion() <= 1)
    {
        if (!LyShine::ConvertSubElementFromVec2ToVector2(context, classElement, "Spacing"))
        {
            return false;
        }

        if (!LyShine::ConvertSubElementFromVec2ToVector2(context, classElement, "CellSize"))
        {
            return false;
        }
    }

    return true;
}
