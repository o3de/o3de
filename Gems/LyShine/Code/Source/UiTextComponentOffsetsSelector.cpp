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
#include "LyShine_precompiled.h"
#include "UiTextComponentOffsetsSelector.h"
#include "StringUtfUtils.h"

void UiTextComponentOffsetsSelector::ParseBatchLine(const UiTextComponent::DrawBatchLine& batchLine, float& curLineWidth)
{
    // Knowing the length of the line helps with alignment calculations
    lineOffsetsStack.top()->batchLineLength = batchLine.lineSize.GetX();

    // The "current line index" resets to zero with each new line. This
    // index allows us to index relative to the current line of text
    // we're iterating on.
    int curLineIndexIter = 0;

    // Keep track of where m_firstIndex occurs relative to the current line.
    // This is needed when m_firstIndex and m_lastIndex occur on the same line
    // to obtain the selection range for that line.
    int firstIndexLineIndex = 0;

    // For input text, we could safely assume one DrawBatch per line,
    // since we don't support marked-up input (at least for now). But
    // it's easy enough to iterate through the list anyways.
    for (const UiTextComponent::DrawBatch& drawBatch : batchLine.drawBatchList)
    {
        // Iterate character by character over DrawBatch string contents,
        // looking for m_firstIndex and m_lastIndex.
        Unicode::CIterator<const char*, false> pChar(drawBatch.text.c_str());
        while (uint32_t ch = *pChar)
        {
            ++pChar;
            if (m_indexIter == m_firstIndex)
            {
                firstIndexFound = true;
                firstIndexLineIndex = curLineIndexIter;

                // Get the width of the string of characters prior to the
                // selection string. This will be used to offset the
                // cursor position from the left of the start of the line.
                AZStd::string unselectedPrecedingString(drawBatch.text.substr(0, firstIndexLineIndex));
                lineOffsetsStack.top()->left.SetX(curLineWidth + drawBatch.font->GetTextSize(unselectedPrecedingString.c_str(), false, m_fontContext).x);

                if (m_firstIndex == m_lastIndex)
                {
                    lastIndexFound = true;
                    lineOffsetsStack.top()->right = AZ::Vector2::CreateZero();

                    break;
                }
            }
            else if (m_indexIter == m_lastIndex)
            {
                lastIndexFound = true;

                // The number of chars selected (selection length) for this
                // line depends on whether the selection is split across multiple lines.
                const int selectionLength = firstAndLastIndexOccurOnDifferentLines ? curLineIndexIter : curLineIndexIter - firstIndexLineIndex;

                AZStd::string selectionString(drawBatch.text.substr(firstIndexLineIndex, selectionLength));
                Vec2 rightSize = drawBatch.font->GetTextSize(selectionString.c_str(), true, m_fontContext);
                lineOffsetsStack.top()->right.SetX(rightSize.x);
                m_numCharsSelected += LyShine::GetUtf8StringLength(selectionString);

                break;
            }

            // Iterate both curLineIndexIter (the index relative to this
            // line) and m_indexIter (the 'global' index for iterating across
            // the entire rendered string).
            curLineIndexIter += LyShine::GetMultiByteCharSize(ch);
            ++m_indexIter;
        }

        // We're done iterating through the string contents of this DrawBatch
        // for this line and we still haven't found m_firstIndex. In this case,
        // we can add the entire width of the DrawBatch contents to the current
        // line width.
        if (!firstIndexFound)
        {
            curLineWidth += drawBatch.font->GetTextSize(drawBatch.text.c_str(), false, m_fontContext).x;
        }
        // If m_firstIndex has been found, but we haven't found m_lastIndex, we
        // calculate curLineWidth relative to firstIndexLineIndex (the m_firstIndex
        // position relative to the current line). Note that firstIndexLineIndex
        // is reset to zero with each line we iterate on. This allows us to
        // select the substring for the current line whether m_firstIndex occurs
        // on the same line or not.
        else if (!lastIndexFound)
        {
            int substrLength = drawBatch.text.length() - firstIndexLineIndex;
            AZStd::string curSubstring(drawBatch.text.substr(firstIndexLineIndex, substrLength));
            curLineWidth += drawBatch.font->GetTextSize(curSubstring.c_str(), false, m_fontContext).x;
            lineOffsetsStack.top()->right.SetX(AZStd::GetMax<float>(lineOffsetsStack.top()->right.GetX(), curLineWidth));
            m_numCharsSelected += LyShine::GetUtf8StringLength(curSubstring);
        }
    }
}

void UiTextComponentOffsetsSelector::HandleTopAndMiddleOffsets()
{
    const bool topOffsetNeedsPopping = 3 == lineOffsetsStack.size();
    const bool middleOffsetNeedsPopping = m_lineCounter + 1 == m_lastIndexLineNumber;
    if (topOffsetNeedsPopping)
    {
        const float curHeightOffset = lineOffsetsStack.top()->left.GetY() + lineOffsetsStack.top()->right.GetY();
        lineOffsetsStack.pop();

        // We take the max here in case the top offset occurs on
        // the first line (in which case the height offset would be zero).
        // This either pushes the cursor to the following line
        // (m_fontSize) or following lines if an offset is applied (curHeightOffset).
        lineOffsetsStack.top()->left.SetY(AZStd::GetMax<float>(curHeightOffset, m_fontSize));

        // Always reset right (relative) y-offset when a new left
        // ("absolute") y-offset is assigned.
        lineOffsetsStack.top()->right.SetY(0.0f);
    }
    else if (middleOffsetNeedsPopping)
    {
        const float curHeightOffset = lineOffsetsStack.top()->left.GetY() + lineOffsetsStack.top()->right.GetY();
        lineOffsetsStack.pop();

        // We need to substract m_fontSize here to "prime" for the
        // fact that we'll be adding it back in, below.
        lineOffsetsStack.top()->left.SetY(lineOffsetsStack.top()->left.GetY() + (curHeightOffset - m_fontSize));

        // Always reset right (relative) y-offset when a new left
        // ("absolute") y-offset is assigned.
        lineOffsetsStack.top()->right.SetY(0.0f);
    }
}

void UiTextComponentOffsetsSelector::IncrementYOffsets()
{
    // We increment the left (absolute) y-offset only when we are NOT
    // iterating through a "middle" section. Once we hit a middle
    // section, we want to lock/freeze the left (absolute) y-offset
    // position and only increment the right (relative) y-offset
    // position. This allows the rendered rect to span the entirety
    // of the selection.
    const bool iteratingOnMiddleSection = 2 == lineOffsetsStack.size() && m_lineCounter < m_numLines;
    if (!iteratingOnMiddleSection)
    {
        lineOffsetsStack.top()->left.SetY(lineOffsetsStack.top()->left.GetY() + m_fontSize);

        // Always reset right (relative) y-offset when a new left
        // ("absolute") y-offset is assigned.
        lineOffsetsStack.top()->right.SetY(0.0f);
    }

    lineOffsetsStack.top()->right.SetY(lineOffsetsStack.top()->right.GetY() + m_fontSize);
}

void UiTextComponentOffsetsSelector::CalculateOffsets(UiTextComponent::LineOffsets& top, UiTextComponent::LineOffsets& middle, UiTextComponent::LineOffsets& bottom)
{
    lineOffsetsStack.push(&bottom);
    lineOffsetsStack.push(&middle);
    lineOffsetsStack.push(&top);

    // Iterate over each rendered line of text, operating on the top of the
    // line offsets stack. The stack is popped as each section is completed.
    // Since the bottom section is the last section, there's no need to pop
    // it off the stack.
    for (const UiTextComponent::DrawBatchLine& batchLine : m_drawBatchLines.batchLines)
    {
        m_lineCounter++;
        float curLineWidth = 0.0f;

        // X offset gets reset for every new line we iterate
        lineOffsetsStack.top()->left.SetX(0.0f);

        ParseBatchLine(batchLine, curLineWidth);

        // Handle the special case where the index is at the end of the string
        // (1 beyond the string index, technically) and there is no selection.
        // For this case we want to display the cursor at the end of the string,
        // so we assign the curLineWidth to the left X offset.
        const bool cursorAtEndOfString = lineOffsetsStack.top()->left.GetX() == 0.0f;
        if (cursorAtEndOfString && m_firstIndex == m_lastIndex)
        {
            lineOffsetsStack.top()->left.SetX(curLineWidth);
        }

        const bool noSelection = m_firstIndex == m_lastIndex;
        const bool onLineHint = m_lineCounter == m_lineNumHint;
        const bool onIndex = m_indexIter == m_firstIndex;
        const bool shouldPlaceIndexOnThisLine = noSelection && onLineHint && onIndex;
        if (shouldPlaceIndexOnThisLine)
        {
            firstIndexFound = lastIndexFound = true;
        }

        // If we still haven't found m_firstIndex, we can skip additional
        // early-out, stack-popping logic, etc.
        if (firstIndexFound)
        {
            // It's possible to have all the characters selected but never found
            // m_lastIndex because m_lastIndex could be 1-beyond the string array.
            // For example, if all characters are selected, then m_lastIndex is
            // actually 1-beyond the array extents, so we account for that here.
            const bool allCharsSelected = m_numCharsSelected > 0 && m_numCharsSelected == m_lastIndex - m_firstIndex;

            if (lastIndexFound || allCharsSelected)
            {
                // Nothing left to do
                break;
            }
            else
            {
                HandleTopAndMiddleOffsets();
                firstAndLastIndexOccurOnDifferentLines = true;
            }
        }

        // When cursor is at end of text, last and first index booleans technically
        // aren't found because the cursor is one past the end of the string
        // array, so execution comes to this point.
        const bool cursorAtEndOfText = cursorAtEndOfString && m_lineCounter == m_numLines;
        if (!cursorAtEndOfText)
        {
            IncrementYOffsets();
        }
    }
}