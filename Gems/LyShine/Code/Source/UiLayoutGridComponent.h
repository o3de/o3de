/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <LyShine/Bus/UiLayoutBus.h>
#include <LyShine/Bus/UiLayoutControllerBus.h>
#include <LyShine/Bus/UiLayoutGridBus.h>
#include <LyShine/Bus/UiTransform2dBus.h>
#include <LyShine/Bus/UiLayoutCellDefaultBus.h>
#include <LyShine/UiComponentTypes.h>

#include <AzCore/Component/Component.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Math/Vector2.h>

#include <UiLayoutHelpers.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
//! This component overrides the transforms of immediate children to organize them into a grid
class UiLayoutGridComponent
    : public AZ::Component
    , public UiLayoutControllerBus::Handler
    , public UiLayoutBus::Handler
    , public UiLayoutGridBus::Handler
    , public UiLayoutCellDefaultBus::Handler
    , public UiTransformChangeNotificationBus::Handler
{
public: // member functions

    AZ_COMPONENT(UiLayoutGridComponent, LyShine::UiLayoutGridComponentUuid, AZ::Component);

    UiLayoutGridComponent();
    ~UiLayoutGridComponent() override;

    // UiLayoutControllerInterface
    virtual void ApplyLayoutWidth() override;
    virtual void ApplyLayoutHeight() override;
    // ~UiLayoutControllerInterface

    // UiLayoutInterface
    virtual bool IsUsingLayoutCellsToCalculateLayout() override;
    virtual bool GetIgnoreDefaultLayoutCells() override;
    virtual void SetIgnoreDefaultLayoutCells(bool ignoreDefaultLayoutCells) override;
    virtual IDraw2d::HAlign GetHorizontalChildAlignment() override;
    virtual void SetHorizontalChildAlignment(IDraw2d::HAlign alignment) override;
    virtual IDraw2d::VAlign GetVerticalChildAlignment() override;
    virtual void SetVerticalChildAlignment(IDraw2d::VAlign alignment) override;
    virtual bool IsControllingChild(AZ::EntityId childId) override;
    virtual AZ::Vector2 GetSizeToFitChildElements(const AZ::Vector2& childElementSize, int numChildElements) override;
    // ~UiLayoutInterface

    // UiLayoutGridInterface
    virtual UiLayoutInterface::Padding GetPadding() override;
    virtual void SetPadding(UiLayoutInterface::Padding padding) override;
    virtual AZ::Vector2 GetSpacing() override;
    virtual void SetSpacing(AZ::Vector2 spacing) override;
    virtual AZ::Vector2 GetCellSize() override;
    virtual void SetCellSize(AZ::Vector2 size) override;
    virtual UiLayoutInterface::HorizontalOrder GetHorizontalOrder() override;
    virtual void SetHorizontalOrder(UiLayoutInterface::HorizontalOrder order) override;
    virtual UiLayoutInterface::VerticalOrder GetVerticalOrder() override;
    virtual void SetVerticalOrder(UiLayoutInterface::VerticalOrder order) override;
    virtual StartingDirection GetStartingDirection() override;
    virtual void SetStartingDirection(StartingDirection direction) override;
    // ~UiLayoutGridInterface

    // UiLayoutCellDefaultInterface
    float GetMinWidth() override;
    float GetMinHeight() override;
    float GetTargetWidth(float maxWidth) override;
    float GetTargetHeight(float maxHeight) override;
    float GetExtraWidthRatio() override;
    float GetExtraHeightRatio() override;
    // ~UiLayoutCellDefaultInterface

    // UiTransformChangeNotificationBus
    void OnCanvasSpaceRectChanged(AZ::EntityId entityId, const UiTransformInterface::Rect& oldRect, const UiTransformInterface::Rect& newRect) override;
    // ~UiTransformChangeNotificationBus

public:  // static member functions

    static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("UiLayoutService"));
    }

    static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("UiLayoutService"));
    }

    static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC_CE("UiElementService"));
        required.push_back(AZ_CRC_CE("UiTransformService"));
    }

    static void Reflect(AZ::ReflectContext* context);

protected: // member functions

    // AZ::Component
    void Activate() override;
    void Deactivate() override;
    // ~AZ::Component

    AZ_DISABLE_COPY_MOVE(UiLayoutGridComponent);

    //! Get the bounding rect size of the children
    AZ::Vector2 GetChildrenBoundingRectSize(const AZ::Vector2 childElementSize, int numChildElements);

    //! Called on a property change that has caused this element's layout to be invalid
    void InvalidateLayout();

    //! Called when a property that is used to calculate default layout cell values has changed
    void InvalidateParentLayout();

    //! Refresh the transform properties in the editor's properties pane
    void CheckLayoutFitterAndRefreshEditorTransformProperties() const;

private: // static member functions

    static bool VersionConverter(AZ::SerializeContext& context,
        AZ::SerializeContext::DataElementNode& classElement);

protected: // data

    //! the padding (in pixels) inside the edges of this element
    UiLayoutInterface::Padding m_padding;

    //! the vertical and horizontal spacing between child elements in pixels
    AZ::Vector2 m_spacing;

    //! the width and height of child elements in pixels
    AZ::Vector2 m_cellSize;

    //! the order that the child elements are placed in
    UiLayoutInterface::HorizontalOrder m_horizontalOrder;
    UiLayoutInterface::VerticalOrder m_verticalOrder;
    StartingDirection m_startingDirection;

    //! Child alignment
    IDraw2d::HAlign m_childHAlignment;
    IDraw2d::VAlign m_childVAlignment;

    //! The original offsets. Used to get a bounding size that is used to calculate
    //! the number of rows or columns that fit within the bounds
    UiTransform2dInterface::Offsets m_origOffsets;
    bool m_origOffsetsInitialized;
};
