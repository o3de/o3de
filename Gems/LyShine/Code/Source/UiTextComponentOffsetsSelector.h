/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
#pragma once

#include "UiTextComponent.h"
#include <AzCore/std/containers/stack.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
//! \brief Helper class for calculating offsets for visualizing multi-line selection.
struct UiTextComponentOffsetsSelector
{
    UiTextComponentOffsetsSelector(
        const UiTextComponent::DrawBatchLines& drawBatchLines,
        const STextDrawContext& fontContext,
        float fontSize,
        int firstIndex,
        int lastIndex,
        int lastIndexLineNumber,
        int lineNumHint)
        : m_drawBatchLines(drawBatchLines)
        , m_fontContext(fontContext)
        , m_fontSize(fontSize)
        , m_maxLineHeight(0.0f)
        , m_firstIndex(firstIndex)
        , m_lastIndex(lastIndex)
        , m_lastIndexLineNumber(lastIndexLineNumber)
        , m_numLines(m_drawBatchLines.batchLines.size())
        , m_indexIter(0)
        , m_numCharsSelected(0)
        , m_lineCounter(0)
        , m_lineNumHint(lineNumHint)
    {
    }

    //! \brief Parses all the DrawBatch string content of a DrawBatchLine for offsets calculation.
    void ParseBatchLine(const UiTextComponent::DrawBatchLine& batchLine, float& curLineWidth);

    //! \brief Handles top and middle offset section cases.
    void HandleTopAndMiddleOffsets();

    //! \brief Condition y-offset incrementing for whatever is on the top of the stack.
    void IncrementYOffsets();

    //! \brief Parses entirety of DrawBatchLines of text and assigns values to top, middle, and bottom offsets accordingly.
    void CalculateOffsets(UiTextComponent::LineOffsets& top, UiTextComponent::LineOffsets& middle, UiTextComponent::LineOffsets& bottom);

    AZStd::stack<UiTextComponent::LineOffsets*> lineOffsetsStack;   //!< A multi-line selection can be divided into three offsets: the first
                                                                    //!< line (top), the last line (bottom), and a multi-line middle section
                                                                    //!< that is basically a rect.
                                                                    //!<
                                                                    //!< Each LineOffset contains a Vec2 for left and right offsets. The left
                                                                    //!< offset is "absolute" for the element rect whereas the right offset is
                                                                    //!< relative to the left offset.
    const UiTextComponent::DrawBatchLines& m_drawBatchLines;
    const STextDrawContext& m_fontContext;
    const float m_fontSize;
    float m_maxLineHeight;
    const int m_firstIndex;
    const int m_lastIndex;
    const int m_lastIndexLineNumber;                                //!< Used to determine location within "middle" section
    const int m_numLines;
    int m_indexIter;                                                //!< Index for iterating over displaying string (m_locText)
    int m_numCharsSelected;
    int m_lineCounter;
    int m_lineNumHint;
    bool firstLine = true;
    bool firstIndexFound = false;
    bool lastIndexFound = false;
    bool firstAndLastIndexOccurOnDifferentLines = false;
};
