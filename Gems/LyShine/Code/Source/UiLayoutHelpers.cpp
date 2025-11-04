/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "UiLayoutHelpers.h"

#include <LyShine/Bus/UiElementBus.h>
#include <LyShine/Bus/UiLayoutManagerBus.h>
#include <LyShine/Bus/UiLayoutCellBus.h>
#include <LyShine/Bus/UiLayoutCellDefaultBus.h>
#include <LyShine/Bus/UiEditorChangeNotificationBus.h>
#include <LyShine/Bus/UiLayoutFitterBus.h>

namespace
{
    float GetLargestFloat(const AZStd::vector<float>& values)
    {
        float largestValue = 0.0f;

        for (auto value : values)
        {
            if (largestValue < value)
            {
                largestValue = value;
            }
        }

        return largestValue;
    }

    float GetElementDefaultMinWidth(AZ::EntityId elementId, float defaultValue)
    {
        AZ::EBusAggregateResults<float> results;
        UiLayoutCellDefaultBus::EventResult(results, elementId, &UiLayoutCellDefaultBus::Events::GetMinWidth);

        if (results.values.empty())
        {
            return defaultValue;
        }

        return GetLargestFloat(results.values);
    }

    float GetElementDefaultTargetWidth(AZ::EntityId elementId, float defaultValue, float maxValue)
    {
        AZ::EBusAggregateResults<float> results;
        UiLayoutCellDefaultBus::EventResult(results, elementId, &UiLayoutCellDefaultBus::Events::GetTargetWidth, maxValue);

        if (results.values.empty())
        {
            return defaultValue;
        }

        return GetLargestFloat(results.values);
    }

    float GetElementDefaultExtraWidthRatio(AZ::EntityId elementId, float defaultValue)
    {
        AZ::EBusAggregateResults<float> results;
        UiLayoutCellDefaultBus::EventResult(results, elementId, &UiLayoutCellDefaultBus::Events::GetExtraWidthRatio);

        if (results.values.empty())
        {
            return defaultValue;
        }

        return GetLargestFloat(results.values);
    }

    float GetElementDefaultMinHeight(AZ::EntityId elementId, float defaultValue)
    {
        AZ::EBusAggregateResults<float> results;
        UiLayoutCellDefaultBus::EventResult(results, elementId, &UiLayoutCellDefaultBus::Events::GetMinHeight);

        if (results.values.empty())
        {
            return defaultValue;
        }

        return GetLargestFloat(results.values);
    }

    float GetElementDefaultTargetHeight(AZ::EntityId elementId, float defaultValue, float maxValue)
    {
        AZ::EBusAggregateResults<float> results;
        UiLayoutCellDefaultBus::EventResult(results, elementId, &UiLayoutCellDefaultBus::Events::GetTargetHeight, maxValue);

        if (results.values.empty())
        {
            return defaultValue;
        }

        return GetLargestFloat(results.values);
    }

    float GetElementDefaultExtraHeightRatio(AZ::EntityId elementId, float defaultValue)
    {
        AZ::EBusAggregateResults<float> results;
        UiLayoutCellDefaultBus::EventResult(results, elementId, &UiLayoutCellDefaultBus::Events::GetExtraHeightRatio);

        if (results.values.empty())
        {
            return defaultValue;
        }

        return GetLargestFloat(results.values);
    }

    float GetLayoutCellTargetWidth(AZ::EntityId elementId, bool ignoreDefaultLayoutCells)
    {
        float value = LyShine::UiLayoutCellUnspecifiedSize;

        // First check for overridden cell values
        UiLayoutCellBus::EventResult(value, elementId, &UiLayoutCellBus::Events::GetTargetWidth);

        // Get max value
        float maxValue = LyShine::UiLayoutCellUnspecifiedSize;
        UiLayoutCellBus::EventResult(maxValue, elementId, &UiLayoutCellBus::Events::GetMaxWidth);

        // If not overridden, get the default cell values
        if (!LyShine::IsUiLayoutCellSizeSpecified(value))
        {
            value = 0.0f;
            if (!ignoreDefaultLayoutCells)
            {
                value = GetElementDefaultTargetWidth(elementId, value, maxValue);
            }
        }

        // Make sure that min width isn't greater than target width
        float minValue = LyShine::UiLayoutCellUnspecifiedSize;
        UiLayoutCellBus::EventResult(minValue, elementId, &UiLayoutCellBus::Events::GetMinWidth);
        if (!LyShine::IsUiLayoutCellSizeSpecified(minValue))
        {
            minValue = 0.0f;
            if (!ignoreDefaultLayoutCells)
            {
                minValue = GetElementDefaultMinWidth(elementId, minValue);
            }
        }
        value = AZ::GetMax(value, minValue);

        // Make sure that max width isn't less than target width
        if (LyShine::IsUiLayoutCellSizeSpecified(maxValue) && maxValue < value)
        {
            value = maxValue;
        }

        return value;
    }

    float GetLayoutCellTargetHeight(AZ::EntityId elementId, bool ignoreDefaultLayoutCells)
    {
        float value = LyShine::UiLayoutCellUnspecifiedSize;

        // First check for overridden cell values
        UiLayoutCellBus::EventResult(value, elementId, &UiLayoutCellBus::Events::GetTargetHeight);

        // Get max value
        float maxValue = LyShine::UiLayoutCellUnspecifiedSize;
        UiLayoutCellBus::EventResult(maxValue, elementId, &UiLayoutCellBus::Events::GetMaxHeight);

        // If not overridden, get the default cell values
        if (!LyShine::IsUiLayoutCellSizeSpecified(value))
        {
            value = 0.0f;
            if (!ignoreDefaultLayoutCells)
            {
                value = GetElementDefaultTargetHeight(elementId, value, maxValue);
            }
        }

        // Make sure that min height isn't greater than target height
        float minValue = LyShine::UiLayoutCellUnspecifiedSize;
        UiLayoutCellBus::EventResult(minValue, elementId, &UiLayoutCellBus::Events::GetMinHeight);
        if (!LyShine::IsUiLayoutCellSizeSpecified(minValue))
        {
            minValue = 0.0f;
            if (!ignoreDefaultLayoutCells)
            {
                minValue = GetElementDefaultMinHeight(elementId, minValue);
            }
        }
        value = AZ::GetMax(value, minValue);

        // Make sure that max height isn't less than target height
        if (LyShine::IsUiLayoutCellSizeSpecified(maxValue) && maxValue < value)
        {
            value = maxValue;
        }

        return value;
    }
}

namespace UiLayoutHelpers
{
    ////////////////////////////////////////////////////////////////////////////////////////////////////
    LayoutCellSize::LayoutCellSize()
        : m_minSize(LyShine::UiLayoutCellUnspecifiedSize)
        , m_targetSize(LyShine::UiLayoutCellUnspecifiedSize)
        , m_maxSize(LyShine::UiLayoutCellUnspecifiedSize)
        , m_extraSizeRatio(LyShine::UiLayoutCellUnspecifiedSize)
    {
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    void GetLayoutCellWidths(AZ::EntityId elementId, bool ignoreDefaultLayoutCells, LayoutCellSizes& layoutCellsOut)
    {
        // Helper for ApplyLayoutWidth handler in LayoutRow and LayoutColumn components

        AZStd::vector<AZ::EntityId> childEntityIds;
        UiElementBus::EventResult(childEntityIds, elementId, &UiElementBus::Events::GetChildEntityIds);

        layoutCellsOut.reserve(childEntityIds.size());

        for (auto childEntityId : childEntityIds)
        {
            LayoutCellSize layoutCell;

            // First check for overridden cell values
            if (UiLayoutCellBus::FindFirstHandler(childEntityId))
            {
                UiLayoutCellBus::EventResult(layoutCell.m_minSize, childEntityId, &UiLayoutCellBus::Events::GetMinWidth);
                UiLayoutCellBus::EventResult(layoutCell.m_targetSize, childEntityId, &UiLayoutCellBus::Events::GetTargetWidth);
                UiLayoutCellBus::EventResult(layoutCell.m_maxSize, childEntityId, &UiLayoutCellBus::Events::GetMaxWidth);
                UiLayoutCellBus::EventResult(layoutCell.m_extraSizeRatio, childEntityId, &UiLayoutCellBus::Events::GetExtraWidthRatio);
            }

            // If not overridden, get the default cell values
            if (!LyShine::IsUiLayoutCellSizeSpecified(layoutCell.m_minSize))
            {
                layoutCell.m_minSize = 0.0f;
                if (!ignoreDefaultLayoutCells)
                {
                    layoutCell.m_minSize = GetElementDefaultMinWidth(childEntityId, layoutCell.m_minSize);
                }
            }
            if (!LyShine::IsUiLayoutCellSizeSpecified(layoutCell.m_targetSize))
            {
                layoutCell.m_targetSize = 0.0f;
                if (!ignoreDefaultLayoutCells)
                {
                    layoutCell.m_targetSize = GetElementDefaultTargetWidth(childEntityId, layoutCell.m_targetSize, layoutCell.m_maxSize);
                }
            }
            if (!LyShine::IsUiLayoutCellSizeSpecified(layoutCell.m_extraSizeRatio))
            {
                layoutCell.m_extraSizeRatio = 1.0f;
                if (!ignoreDefaultLayoutCells)
                {
                    layoutCell.m_extraSizeRatio = GetElementDefaultExtraWidthRatio(childEntityId, layoutCell.m_extraSizeRatio);
                }
            }

            layoutCell.m_targetSize = AZ::GetMax(layoutCell.m_targetSize, layoutCell.m_minSize);
            if (LyShine::IsUiLayoutCellSizeSpecified(layoutCell.m_maxSize) && layoutCell.m_maxSize < layoutCell.m_targetSize)
            {
                layoutCell.m_targetSize = layoutCell.m_maxSize;
            }

            layoutCellsOut.push_back(layoutCell);
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    void GetLayoutCellHeights(AZ::EntityId elementId, bool ignoreDefaultLayoutCells, LayoutCellSizes& layoutCellsOut)
    {
        // Helper for ApplyLayoutHeight handler in LayoutRow and LayoutColumn components

        AZStd::vector<AZ::EntityId> childEntityIds;
        UiElementBus::EventResult(childEntityIds, elementId, &UiElementBus::Events::GetChildEntityIds);

        layoutCellsOut.reserve(childEntityIds.size());

        for (auto childEntityId : childEntityIds)
        {
            LayoutCellSize layoutCell;

            // First check for overridden cell values
            if (UiLayoutCellBus::FindFirstHandler(childEntityId))
            {
                UiLayoutCellBus::EventResult(layoutCell.m_minSize, childEntityId, &UiLayoutCellBus::Events::GetMinHeight);
                UiLayoutCellBus::EventResult(layoutCell.m_targetSize, childEntityId, &UiLayoutCellBus::Events::GetTargetHeight);
                UiLayoutCellBus::EventResult(layoutCell.m_maxSize, childEntityId, &UiLayoutCellBus::Events::GetMaxHeight);
                UiLayoutCellBus::EventResult(layoutCell.m_extraSizeRatio, childEntityId, &UiLayoutCellBus::Events::GetExtraHeightRatio);
            }

            // If not overridden, get the default cell values
            if (!LyShine::IsUiLayoutCellSizeSpecified(layoutCell.m_minSize))
            {
                layoutCell.m_minSize = 0.0f;
                if (!ignoreDefaultLayoutCells)
                {
                    layoutCell.m_minSize = GetElementDefaultMinHeight(childEntityId, layoutCell.m_minSize);
                }
            }
            if (!LyShine::IsUiLayoutCellSizeSpecified(layoutCell.m_targetSize))
            {
                layoutCell.m_targetSize = 0.0f;
                if (!ignoreDefaultLayoutCells)
                {
                    layoutCell.m_targetSize = GetElementDefaultTargetHeight(childEntityId, layoutCell.m_targetSize, layoutCell.m_maxSize);
                }
            }
            if (!LyShine::IsUiLayoutCellSizeSpecified(layoutCell.m_extraSizeRatio))
            {
                layoutCell.m_extraSizeRatio = 1.0f;
                if (!ignoreDefaultLayoutCells)
                {
                    layoutCell.m_extraSizeRatio = GetElementDefaultExtraHeightRatio(childEntityId, layoutCell.m_extraSizeRatio);
                }
            }

            layoutCell.m_targetSize = AZ::GetMax(layoutCell.m_targetSize, layoutCell.m_minSize);
            if (LyShine::IsUiLayoutCellSizeSpecified(layoutCell.m_maxSize) && layoutCell.m_maxSize < layoutCell.m_targetSize)
            {
                layoutCell.m_targetSize = layoutCell.m_maxSize;
            }

            layoutCellsOut.push_back(layoutCell);
        }
    }

    AZStd::vector<float> GetLayoutCellMinWidths(AZ::EntityId elementId, bool ignoreDefaultLayoutCells)
    {
        AZStd::vector<AZ::EntityId> childEntityIds;
        UiElementBus::EventResult(childEntityIds, elementId, &UiElementBus::Events::GetChildEntityIds);

        AZStd::vector<float> values(childEntityIds.size(), 0.0f);

        int i = 0;
        for (auto childEntityId : childEntityIds)
        {
            float value = LyShine::UiLayoutCellUnspecifiedSize;

            // First check for overridden cell values
            UiLayoutCellBus::EventResult(value, childEntityId, &UiLayoutCellBus::Events::GetMinWidth);

            // If not overridden, get the default cell values
            if (!LyShine::IsUiLayoutCellSizeSpecified(value))
            {
                value = 0.0f;
                if (!ignoreDefaultLayoutCells)
                {
                    value = GetElementDefaultMinWidth(childEntityId, value);
                }
            }

            values[i] = value;
            i++;
        }

        return values;
    }

    AZStd::vector<float> GetLayoutCellTargetWidths(AZ::EntityId elementId, bool ignoreDefaultLayoutCells)
    {
        // Helper for GetTargetWidth handler in LayoutRow and LayoutColumn components.
        // Used when a LayoutRow/Column wants to know its target size (ex. when a layout element has a LayoutFitterComponent or when layouts are nested)

        AZStd::vector<AZ::EntityId> childEntityIds;
        UiElementBus::EventResult(childEntityIds, elementId, &UiElementBus::Events::GetChildEntityIds);

        AZStd::vector<float> values(childEntityIds.size(), 0.0f);

        int i = 0;
        for (auto childEntityId : childEntityIds)
        {
            float value = GetLayoutCellTargetWidth(childEntityId, ignoreDefaultLayoutCells);
            values[i] = value;
            i++;
        }

        return values;
    }

    AZStd::vector<float> GetLayoutCellMinHeights(AZ::EntityId elementId, bool ignoreDefaultLayoutCells)
    {
        AZStd::vector<AZ::EntityId> childEntityIds;
        UiElementBus::EventResult(childEntityIds, elementId, &UiElementBus::Events::GetChildEntityIds);

        AZStd::vector<float> values(childEntityIds.size(), 0.0f);

        int i = 0;
        for (auto childEntityId : childEntityIds)
        {
            float value = LyShine::UiLayoutCellUnspecifiedSize;

            // First check for overridden cell values
            UiLayoutCellBus::EventResult(value, childEntityId, &UiLayoutCellBus::Events::GetMinHeight);

            // If not overridden, get the default cell values
            if (!LyShine::IsUiLayoutCellSizeSpecified(value))
            {
                value = 0.0f;
                if (!ignoreDefaultLayoutCells)
                {
                    value = GetElementDefaultMinHeight(childEntityId, value);
                }
            }

            values[i] = value;
            i++;
        }

        return values;
    }

    AZStd::vector<float> GetLayoutCellTargetHeights(AZ::EntityId elementId, bool ignoreDefaultLayoutCells)
    {
        // Helper for GetTargetHeight handler in LayoutRow and LayoutColumn components.
        // Used when a LayoutRow/Column wants to know its target size (ex. when a layout element has a LayoutFitterComponent or when layouts are nested)

        AZStd::vector<AZ::EntityId> childEntityIds;
        UiElementBus::EventResult(childEntityIds, elementId, &UiElementBus::Events::GetChildEntityIds);

        AZStd::vector<float> values(childEntityIds.size(), 0.0f);

        int i = 0;
        for (auto childEntityId : childEntityIds)
        {
            float value = GetLayoutCellTargetHeight(childEntityId, ignoreDefaultLayoutCells);
            values[i] = value;
            i++;
        }

        return values;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    void CalculateElementSizes(const LayoutCellSizes& layoutCells, float availableSize, float spacing, AZStd::vector<float>& sizesOut)
    {
        int numElements = static_cast<int>(layoutCells.size());

        availableSize -= (numElements - 1) * spacing;

        // Check if there's enough space for all target sizes
        float totalTargetSize = 0.0f;
        for (int i = 0; i < numElements; i++)
        {
            totalTargetSize += layoutCells[i].m_targetSize;
        }

        if (totalTargetSize <= availableSize)
        {
            // Enough space for all target sizes, allocate target size
            // Target size is always greater than or equal to min size
            for (int i = 0; i < numElements; i++)
            {
                sizesOut[i] = layoutCells[i].m_targetSize;
                availableSize -= layoutCells[i].m_targetSize;
            }
        }
        else
        {
            // Not enough space for all target sizes

            // Allocate min size
            for (int i = 0; i < numElements; i++)
            {
                sizesOut[i] = layoutCells[i].m_minSize;
                availableSize -= layoutCells[i].m_minSize;
            }

            // If there is space left, try to allocate target size
            if (availableSize > 0.0f)
            {
                // Make a list of element indexes that have not met their target sizes
                AZStd::vector<int> needTargetSizeIndexes;
                AZStd::vector<float> neededAmounts;
                float totalNeededAmount = 0.0f;
                for (int i = 0; i < numElements; i++)
                {
                    if (sizesOut[i] < layoutCells[i].m_targetSize)
                    {
                        needTargetSizeIndexes.push_back(i);
                        float neededAmount = layoutCells[i].m_targetSize - sizesOut[i];
                        neededAmounts.push_back(neededAmount);
                        totalNeededAmount += neededAmount;
                    }
                }

                if (!needTargetSizeIndexes.empty())
                {
                    // Give each element part of the available space to get as close to target size as possible
                    int neededAmountIndex = 0;
                    for (auto index : needTargetSizeIndexes)
                    {
                        sizesOut[index] += (neededAmounts[neededAmountIndex] / totalNeededAmount) * availableSize;

                        neededAmountIndex++;
                    }
                }

                availableSize = 0.0f;
            }
        }

        // If there is still space left, allocate extra size based on ratios
        if (availableSize > 0.0f)
        {
            struct CellExtraSizeInfo
            {
                int m_cellIndex;
                float m_normalizedExtraSizeRatio;
                bool m_reachedMax;

                CellExtraSizeInfo(int cellIndex)
                {
                    m_cellIndex = cellIndex;
                    m_normalizedExtraSizeRatio = 0.0f;
                    m_reachedMax = false;
                }
            };

            // Make a list of cells that accept extra size
            AZStd::vector<CellExtraSizeInfo> cellsAcceptingExtraSize;
            cellsAcceptingExtraSize.reserve(numElements);
            for (int i = 0; i < numElements; i++)
            {
                if (layoutCells[i].m_extraSizeRatio > 0.0f)
                {
                    cellsAcceptingExtraSize.push_back(CellExtraSizeInfo(i));
                }
            }

            // Add extra size to each element
            while (!cellsAcceptingExtraSize.empty())
            {
                // Find smallest extra size ratio
                float smallestRatio = -1.0f;
                for (const auto& cellExtraSizeInfo : cellsAcceptingExtraSize)
                {
                    const LayoutCellSize& layoutCell = layoutCells[cellExtraSizeInfo.m_cellIndex];
                    if (smallestRatio < 0.0f || smallestRatio > layoutCell.m_extraSizeRatio)
                    {
                        smallestRatio = layoutCell.m_extraSizeRatio;
                    }
                }

                // Normalize ratios so that the smallest ratio has a value of one
                float totalUnits = 0.0f;
                for (auto& cellExtraSizeInfo : cellsAcceptingExtraSize)
                {
                    const LayoutCellSize& layoutCell = layoutCells[cellExtraSizeInfo.m_cellIndex];
                    cellExtraSizeInfo.m_normalizedExtraSizeRatio = layoutCell.m_extraSizeRatio / smallestRatio;
                    totalUnits += cellExtraSizeInfo.m_normalizedExtraSizeRatio;
                }

                // Track any unused space by a cell due to reaching its maximum size
                float unusedSpace = 0.0f;

                const float sizePerUnit = availableSize / totalUnits;
                for (auto& cellExtraSizeInfo : cellsAcceptingExtraSize)
                {
                    const LayoutCellSize& layoutCell = layoutCells[cellExtraSizeInfo.m_cellIndex];
                    const float sizeToAdd = cellExtraSizeInfo.m_normalizedExtraSizeRatio * sizePerUnit;
                    const float curSize = sizesOut[cellExtraSizeInfo.m_cellIndex];
                    const float newSize = curSize + sizeToAdd;
                    if (LyShine::IsUiLayoutCellSizeSpecified(layoutCell.m_maxSize) && layoutCell.m_maxSize < newSize)
                    {
                        sizesOut[cellExtraSizeInfo.m_cellIndex] = layoutCell.m_maxSize;
                        cellExtraSizeInfo.m_reachedMax = true;
                        unusedSpace += newSize - layoutCell.m_maxSize;
                    }
                    else
                    {
                        sizesOut[cellExtraSizeInfo.m_cellIndex] += sizeToAdd;
                    }
                }

                if (unusedSpace >= 1.0f)
                {
                    // Remove any cells that have reached their max size
                    cellsAcceptingExtraSize.erase(
                        AZStd::remove_if(
                            cellsAcceptingExtraSize.begin(), cellsAcceptingExtraSize.end(),
                            [](const CellExtraSizeInfo& cellExtraSizeInfo)
                    {
                        return cellExtraSizeInfo.m_reachedMax;
                    }),
                    cellsAcceptingExtraSize.end());

                    availableSize = unusedSpace;
                }
                else
                {
                    break;
                }
            }
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    float CalculateSingleElementSize(const LayoutCellSize& layoutCell, float availableSize)
    {
        float size = 0.0f;

        if (layoutCell.m_minSize > availableSize)
        {
            size = layoutCell.m_minSize;
        }
        else
        {
            if (layoutCell.m_extraSizeRatio > 0.0f)
            {
                size = availableSize;
            }
            else
            {
                size = AZ::GetMin(availableSize, layoutCell.m_targetSize);
            }

            if (LyShine::IsUiLayoutCellSizeSpecified(layoutCell.m_maxSize) && layoutCell.m_maxSize < size)
            {
                size = layoutCell.m_maxSize;
            }
        }

        return size;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    float GetHorizontalAlignmentOffset(IDraw2d::HAlign hAlignment, float availableSpace, float occupiedSpace)
    {
        float alignmentOffset = 0.0f;

        switch (hAlignment)
        {
        case IDraw2d::HAlign::Left:
            alignmentOffset = 0.0f;
            break;

        case IDraw2d::HAlign::Center:
            alignmentOffset = (availableSpace - occupiedSpace) * 0.5f;
            break;

        case IDraw2d::HAlign::Right:
            alignmentOffset = availableSpace - occupiedSpace;
            break;

        default:
            AZ_Assert(0, "Unrecognized HAlign type in UiLayout");
            alignmentOffset = 0.0f;
            break;
        }

        return alignmentOffset;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    float GetVerticalAlignmentOffset(IDraw2d::VAlign vAlignment, float availableSpace, float occupiedSpace)
    {
        float alignmentOffset = 0.0f;

        switch (vAlignment)
        {
        case IDraw2d::VAlign::Top:
            alignmentOffset = 0.0f;
            break;

        case IDraw2d::VAlign::Center:
            alignmentOffset = (availableSpace - occupiedSpace) * 0.5f;
            break;

        case IDraw2d::VAlign::Bottom:
            alignmentOffset = availableSpace - occupiedSpace;
            break;

        default:
            AZ_Assert(0, "Unrecognized VAlign type in UiLayout Component");
            alignmentOffset = 0.0f;
            break;
        }

        return alignmentOffset;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    bool IsControllingChild(AZ::EntityId parentId, AZ::EntityId childId)
    {
        AZ::Entity* element = nullptr;
        UiElementBus::EventResult(element, parentId, &UiElementBus::Events::FindChildByEntityId, childId);

        if (element == nullptr)
        {
            return false;
        }

        return true;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    void GetSizeInsidePadding(AZ::EntityId elementId, const UiLayoutInterface::Padding& padding, AZ::Vector2& size)
    {
        UiTransformBus::EventResult(size, elementId, &UiTransformBus::Events::GetCanvasSpaceSizeNoScaleRotate);

        float width = size.GetX() - (padding.m_left + padding.m_right);
        float height = size.GetY() - (padding.m_top + padding.m_bottom);

        // Add a small value to accommodate for rounding errors
        const float epsilon = 0.01f;
        width += epsilon;
        height += epsilon;

        size.Set(width, height);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    float GetLayoutElementTargetWidth(AZ::EntityId elementId)
    {
        return GetLayoutCellTargetWidth(elementId, false);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    float GetLayoutElementTargetHeight(AZ::EntityId elementId)
    {
        return GetLayoutCellTargetHeight(elementId, false);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    void InvalidateLayout(AZ::EntityId elementId)
    {
        AZ::EntityId canvasEntityId;
        UiElementBus::EventResult(canvasEntityId, elementId, &UiElementBus::Events::GetCanvasEntityId);
        UiLayoutManagerBus::Event(canvasEntityId, &UiLayoutManagerBus::Events::MarkToRecomputeLayout, elementId);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    void InvalidateParentLayout(AZ::EntityId elementId)
    {
        AZ::EntityId canvasEntityId;
        UiElementBus::EventResult(canvasEntityId, elementId, &UiElementBus::Events::GetCanvasEntityId);
        UiLayoutManagerBus::Event(
            canvasEntityId, &UiLayoutManagerBus::Events::MarkToRecomputeLayoutsAffectedByLayoutCellChange, elementId, true);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    bool IsControlledByHorizontalFit(AZ::EntityId elementId)
    {
        bool isHorizontallyFit = false;
        UiLayoutFitterBus::EventResult(isHorizontallyFit, elementId, &UiLayoutFitterBus::Events::GetHorizontalFit);
        return isHorizontallyFit;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    bool IsControlledByVerticalFit(AZ::EntityId elementId)
    {
        bool isVerticallyFit = false;
        UiLayoutFitterBus::EventResult(isVerticallyFit, elementId, &UiLayoutFitterBus::Events::GetVerticalFit);
        return isVerticallyFit;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    void CheckFitterAndRefreshEditorTransformProperties(AZ::EntityId elementId)
    {
        if (IsControlledByHorizontalFit(elementId) || (IsControlledByVerticalFit(elementId)))
        {
            UiEditorChangeNotificationBus::Broadcast(&UiEditorChangeNotificationBus::Events::OnEditorTransformPropertiesNeedRefresh);
        }
    }
} // namespace UiLayoutHelpers
