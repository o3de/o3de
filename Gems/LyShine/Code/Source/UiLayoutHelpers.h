/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <LyShine/Bus/UiLayoutBus.h>
#include <LyShine/Bus/UiTransformBus.h>

namespace UiLayoutHelpers
{
    struct LayoutCellSize
    {
        LayoutCellSize();

        float m_minSize;
        float m_targetSize;
        float m_maxSize;
        float m_extraSizeRatio;
    };

    using LayoutCellSizes = AZStd::vector<LayoutCellSize>;

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    //! Get a list of layout cell widths corresponding to the children of the layout element
    void GetLayoutCellWidths(AZ::EntityId elementId, bool ignoreDefaultLayoutCells, LayoutCellSizes& layoutCellsOut);

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    //! Get a list of layout cell heights corresponding to the children of the layout element
    void GetLayoutCellHeights(AZ::EntityId elementId, bool ignoreDefaultLayoutCells, LayoutCellSizes& layoutCellsOut);

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    //! Get a list of layout cell min widths corresponding to the children of the layout element
    AZStd::vector<float> GetLayoutCellMinWidths(AZ::EntityId elementId, bool ignoreDefaultLayoutCells);

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    //! Get a list of layout cell target widths corresponding to the children of the layout element
    AZStd::vector<float> GetLayoutCellTargetWidths(AZ::EntityId elementId, bool ignoreDefaultLayoutCells);

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    //! Get a list of layout cell min heights corresponding to the children of the layout element
    AZStd::vector<float> GetLayoutCellMinHeights(AZ::EntityId elementId, bool ignoreDefaultLayoutCells);

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    //! Get a list of layout cell target heights corresponding to the children of the layout element
    AZStd::vector<float> GetLayoutCellTargetHeights(AZ::EntityId elementId, bool ignoreDefaultLayoutCells);

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    //! Calculate the sizes of the elements that will occupy the available space
    void CalculateElementSizes(const LayoutCellSizes& layoutCells, float availableSize, float spacing, AZStd::vector<float>& sizesOut);

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    //! Calculate the size of a single element that will occupy the available space
    float CalculateSingleElementSize(const LayoutCellSize& layoutCell, float availableSize);

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    //! Calculate the horizontal offset for alignment
    float GetHorizontalAlignmentOffset(IDraw2d::HAlign hAlignment, float availableSpace, float occupiedSpace);

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    //! Calculate the vertical offset for alignment
    float GetVerticalAlignmentOffset(IDraw2d::VAlign vAlignment, float availableSpace, float occupiedSpace);

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    //! Check whether a parent layout element is controlling a child element
    bool IsControllingChild(AZ::EntityId parentId, AZ::EntityId childId);

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    //! Calculate the rectangle described by the padding inside the element's borders
    void GetSizeInsidePadding(AZ::EntityId elementId, const UiLayoutInterface::Padding& padding, AZ::Vector2& size);

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    //! Get the width to apply to an element based on the layout cell properties on that element
    float GetLayoutElementTargetWidth(AZ::EntityId elementId);

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    //! Get the height to apply to an element based on the layout cell properties on that element
    float GetLayoutElementTargetHeight(AZ::EntityId elementId);

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    //! Called on a property change that has caused an element's layout to be invalid.
    //! Marks the element as needing to recompute its layout
    void InvalidateLayout(AZ::EntityId elementId);

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    //! Called when a property that is used to calculate default layout cell values has changed.
    //! Marks the element's parent as needing to recompute its layout
    void InvalidateParentLayout(AZ::EntityId elementId);

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    //! Check whether the element's width is being controlled by a layout fitter
    bool IsControlledByHorizontalFit(AZ::EntityId elementId);

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    //! Check whether the element's height is being controlled by a layout fitter
    bool IsControlledByVerticalFit(AZ::EntityId elementId);

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    //! Called on a property change in the UI editor that has caused an element's layout to be invalid.
    //! Sets up a refresh of the UI editor's transform properties in the properties pane if
    //! the transform is controlled by a layout fitter
    void CheckFitterAndRefreshEditorTransformProperties(AZ::EntityId elementId);
} // namespace UiLayoutHelpers
