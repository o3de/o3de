/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "UiTextComponent.h"

#include <AzCore/Math/Crc.h>
#include <AzCore/Math/MathUtils.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/std/string/conversions.h>
#include <AzCore/std/string/regex.h>

#include <AzFramework/API/ApplicationAPI.h>

#include <LyShine/Bus/UiTransform2dBus.h>
#include <LyShine/Bus/UiElementBus.h>
#include <LyShine/Bus/UiLayoutManagerBus.h>
#include <LyShine/Bus/UiCanvasBus.h>
#include <LyShine/UiSerializeHelpers.h>
#include <LyShine/IRenderGraph.h>
#include <LyShine/Draw2d.h>

#include <ILocalizationManager.h>

#include "UiSerialize.h"
#include "TextMarkup.h"
#include "UiTextComponentOffsetsSelector.h"
#include "StringUtfUtils.h"
#include "UiLayoutHelpers.h"
#include "RenderGraph.h"

#include <AtomLyIntegration/AtomFont/FFont.h>
#include <Atom/RPI.Public/Image/ImageSystemInterface.h>

namespace
{
    AZStd::string DefaultDisplayedTextFunction(const AZStd::string& originalText)
    {
        // By default, the text component renders the string contents as-is
        return originalText;
    }

    bool RemoveV4MarkupFlag(
        [[maybe_unused]] AZ::SerializeContext& context,
        AZ::SerializeContext::DataElementNode& classElement)
    {
        int index = classElement.FindElement(AZ_CRC("SupportMarkup", 0x5e81a9c7));
        if (index != -1)
        {
            classElement.RemoveElement(index);
        }

        return true;
    }

    bool AddV8EnableMarkupFlag(
        AZ::SerializeContext& context,
        AZ::SerializeContext::DataElementNode& classElement)
    {
        // This element is a pre-version-8 text component. Prior to version 8 there was no MarkupEnabled
        // flag and markup was always enabled. Going forward, for new components we want to default to
        // markupEnabled = false because of the performance hit of parsing text strings for XML.
        // However, we want to be backward compatible with old data so for pre-version-8 components
        // we set the flag to true.

        // We considered searching the text string for characters such as "<&@" and only turning it on
        // if they were found. But the problem is that data patches do not come through version conversion
        // currently. So there could be markup in the text string in the data patch but we would not turn
        // the flag on. So the markup would stop working.

        // Just for safety check that the flag doesn't already exist
        int index = classElement.FindElement(AZ_CRC("MarkupEnabled"));
        if (index == -1)
        {
            // The element does not exist (it really never should at this version)
            // Add a data element, setting the flag to true
            int newElementIndex = classElement.AddElementWithData<bool>(context, "MarkupEnabled", true);
            if (newElementIndex == -1)
            {
                // Error adding the new data element
                AZ_Error("Serialization", false, "AddElement failed for MarkupEnabled element");
                return false;
            }
        }

        return true;
    }

    bool ConvertV3FontFileNameIfDefault(
        AZ::SerializeContext& context,
        AZ::SerializeContext::DataElementNode& classElement)
    {
        int index = classElement.FindElement(AZ_CRC("FontFileName", 0x44defd6f));
        if (index != -1)
        {
            AZ::SerializeContext::DataElementNode& fontFileNameNode = classElement.GetSubElement(index);
            index = fontFileNameNode.FindElement(AZ_CRC("BaseClass1", 0xd4925735));

            if (index != -1)
            {
                AZ::SerializeContext::DataElementNode& baseClassNode = fontFileNameNode.GetSubElement(index);
                index = baseClassNode.FindElement(AZ_CRC("AssetPath", 0x2c355179));

                if (index != -1)
                {
                    AZ::SerializeContext::DataElementNode& assetPathNode = baseClassNode.GetSubElement(index);
                    AZStd::string oldData;

                    if (!assetPathNode.GetData(oldData))
                    {
                        AZ_Error("Serialization", false, "Element AssetPath is not a AZStd::string.");
                        return false;
                    }

                    if (oldData == "default")
                    {
                        if (!assetPathNode.SetData(context, AZStd::string("default-ui")))
                        {
                            AZ_Error("Serialization", false, "Unable to set AssetPath data.");
                            return false;
                        }

                        // The effect indicies have flip-flopped between the "default" and "default-ui"
                        // fonts. Handle the conversion here.
                        index = classElement.FindElement(AZ_CRC("EffectIndex", 0x4d3320e3));
                        if (index != -1)
                        {
                            AZ::SerializeContext::DataElementNode& effectIndexNode = classElement.GetSubElement(index);
                            uint32 effectIndex = 0;

                            if (!effectIndexNode.GetData(effectIndex))
                            {
                                AZ_Error("Serialization", false, "Element EffectIndex is not an int.");
                                return false;
                            }

                            uint32 newEffectIndex = effectIndex;

                            // Only handle converting indices 1 and 0 in the rare (?) case that the user added
                            // their own effects to the default font.
                            if (newEffectIndex == 1)
                            {
                                newEffectIndex = 0;
                            }
                            else if (newEffectIndex == 0)
                            {
                                newEffectIndex = 1;
                            }

                            if (!effectIndexNode.SetData(context, newEffectIndex))
                            {
                                AZ_Error("Serialization", false, "Unable to set EffectIndex data.");
                                return false;
                            }
                        }
                    }
                }
            }
        }

        return true;
    }

    //! Migrate legacy shrink-to-fit setting to new ShrinkToFit enum.
    //!
    //! As of V8 of text component, the "shrink to fit" setting was a value of
    //! the WrapTextSetting enum. With V9, a new ShrinkToFit enum was introduced
    //! and offered an additional "width-only" option (previously, shrink-to-fit
    //! only performed uniform scaling along both axes).
    bool ConvertV8ShrinkToFitSetting(
        AZ::SerializeContext& context,
        AZ::SerializeContext::DataElementNode& classElement)
    {
        int index = classElement.FindElement(AZ_CRC("WrapTextSetting"));
        if (index != -1)
        {
            AZ::SerializeContext::DataElementNode& wrapTextSettingNode = classElement.GetSubElement(index);
            int oldWrapTextValue = 0;

            if (!wrapTextSettingNode.GetData<int>(oldWrapTextValue))
            {
                AZ_Error("Serialization", false, "Element WrapTextSetting is not an int.");
                return false;
            }

            // Check if WrapTextSetting is set to the legacy "ShrinkToFit" enum value.
            static const int shrinkToFitValue = 2;
            const bool shrinkToFitSettingNeedsUpdating = oldWrapTextValue == shrinkToFitValue;
            if (shrinkToFitSettingNeedsUpdating)
            {
                // It wasn't possible to word-wrap and have shrink-to-fit before, so we just
                // reset the wrap text setting to NoWrap to maintain backwards compatibilty.
                if (!wrapTextSettingNode.SetData<int>(context, static_cast<int>(UiTextInterface::WrapTextSetting::NoWrap)))
                {
                    AZ_Error("Serialization", false, "Unable to set WrapTextSetting to NoWrap (%d).", static_cast<int>(UiTextInterface::WrapTextSetting::NoWrap));
                    return false;
                }

                // If ShrinkToFit doesn't exist yet, add it
                index = classElement.FindElement(AZ_CRC("ShrinkToFit"));
                if (index == -1)
                {
                    index = classElement.AddElement<int>(context, "ShrinkToFit");

                    if (index == -1)
                    {
                        // Error adding the new sub element
                        AZ_Error("Serialization", false, "Failed to create ShrinkToFit node");
                        return false;
                    }
                }

                // Legacy shrink-to-fit only applied uniform scaling along both axes. So here we use
                // the Uniform setting of ShrinkToFit to maintain backwards compatibility.
                AZ::SerializeContext::DataElementNode& shrinkToFitNode = classElement.GetSubElement(index);
                if (!shrinkToFitNode.SetData<int>(context, static_cast<int>(UiTextInterface::ShrinkToFit::Uniform)))
                {
                    AZ_Error("Serialization", false, "Unable to set ShrinkToFit to Uniform (%d).", static_cast<int>(UiTextInterface::ShrinkToFit::Uniform));
                    return false;
                }
            }
        }

        return true;
    }

    //! Remove an older OverflowMode setting that no longer has any effect.
    //!
    //! There used to be an overflow mode setting called "ResizeToText". It
    //! was removed, but some canvases still have the enum value set to it,
    //! which would now set those text fields to ellipsis, which isn't intended.
    //!
    //! Reset the field back to zero (overflow) since the property hasn't had any
    //! effect since ResizeToText was removed anyways.
    bool ConvertV8LegacyOverflowModeSetting(
        AZ::SerializeContext& context,
        AZ::SerializeContext::DataElementNode& classElement)
    {
        int index = classElement.FindElement(AZ_CRC("OverflowMode"));
        if (index != -1)
        {
            AZ::SerializeContext::DataElementNode& overflowModeSettingNode = classElement.GetSubElement(index);
            int oldOverflowModeValue = 0;

            if (!overflowModeSettingNode.GetData<int>(oldOverflowModeValue))
            {
                AZ_Error("Serialization", false, "Element OverflowMode is not an int.");
                return false;
            }

            // Check if OverflowMode is set to the legacy "ResizeToText" enum value.
            static const int legacyResizeToTextValue = 2;
            const bool overflowModeSettingNeedsUpdating = oldOverflowModeValue == legacyResizeToTextValue;
            if (overflowModeSettingNeedsUpdating)
            {
                // This value enum was removed without version conversion. Since it hasn't had any effect
                // up to this point, we just reset the OverflowMode back to default (overflow).
                if (!overflowModeSettingNode.SetData<int>(context, static_cast<int>(UiTextInterface::OverflowMode::OverflowText)))
                {
                    AZ_Error("Serialization", false, "Unable to set OverflowMode to OverflowText (%d).", static_cast<int>(UiTextInterface::OverflowMode::OverflowText));
                    return false;
                }
            }
        }

        return true;
    }

    void SanitizeUserEnteredNewlineChar(AZStd::string& stringToSanitize)
    {
        // Convert user-entered newline delimiters to proper ones before wrapping
        // the text so they can be correctly accounted for.
        static const AZStd::string NewlineDelimiter("\n");
        static const AZStd::regex UserInputNewlineDelimiter("\\\\n");
        stringToSanitize = AZStd::regex_replace(stringToSanitize, UserInputNewlineDelimiter, NewlineDelimiter);
    }
    //! Builds a list of DrawBatch objects from a XML tag tree.
    //!
    //! A DrawBatch is essentially render "state" for text. This method tries
    //! to determine what the current state is that should be applied based
    //! on the tag tree traversal. Once all of a tag's children are
    //! traversed, and a new DrawBatch was created, the batch is popped off
    //! the batch stack and moved into the DrawBatch output list.
    //!
    //! Example usage:
    //!
    //! TextMarkup::Tag markupRoot;
    //! if (TextMarkup::ParseMarkupBuffer(markupText, markupRoot))
    //! {
    //!   AZStd::stack<UiTextComponent::DrawBatch> batchStack;
    //!   AZStd::stack<FontFamily*> fontFamilyStack;
    //!   fontFamilyStack.push(m_fontFamily.get());
    //!   BuildDrawBatches(drawBatches, batchStack, fontFamilyStack, &markupRoot);
    //! }
    //!
    //! \param output List of DrawBatch objects built based on tag tree contents
    //! \param fontFamilyRefs List of Font Family's that output (strongly) references.
    //! \param inlineImages List of Inline Images that are created while building the draw batches
    //! \param fontHeight The height of the font
    //! \param fontAscent The ascent of the font
    //! \param batchStack The DrawBatch on "top" of the stack is the state that is currently active.
    //! \param fontFamilyStack The FontFamily on top of the stack is the font family that's currently active.
    //! The font family can change when the font tag is encountered.
    //! \param currentTag Current tag being visited in the parsed tag tree.
    void BuildDrawBatches(
        UiTextComponent::DrawBatchContainer& output,
        UiTextComponent::FontFamilyRefSet& fontFamilyRefs,
        UiTextComponent::InlineImageContainer& inlineImages,
        float fontHeight,
        float fontAscent,
        AZStd::stack<UiTextComponent::DrawBatch>& batchStack,
        AZStd::stack<FontFamily*>& fontFamilyStack,
        const TextMarkup::Tag* currentTag,
        int& clickableId)
    {
        TextMarkup::TagType type = currentTag->GetType();

        const bool isRoot = type == TextMarkup::TagType::Root;

        bool newBatchStackPushed = false;

        // Root tag doesn't push any new state
        if (!isRoot)
        {
            if (batchStack.empty())
            {
                batchStack.push(UiTextComponent::DrawBatch());
                newBatchStackPushed = true;

                // For new batches, use the Font Family's "normal" font by default
                batchStack.top().font = fontFamilyStack.top()->normal;
            }

            // Prevent creating a new DrawBatch if the "current" batch has
            // no text associated with it yet.
            else if (!batchStack.top().text.empty())
            {
                // Create a copy of the top
                batchStack.push(batchStack.top());
                newBatchStackPushed = true;

                // We assume that a DrawBatch will eventually get its own
                // text assigned, but in case no character was specified
                // in the markup, we explicitly clear the text here to avoid
                // showing duplicate text.
                batchStack.top().text.clear();
            }
        }

        // We need the previous batch for all cases except the root case
        // (where there is no previous batch). To streamline handling this
        // case, we just create an unused default-constructed DrawBatch
        // for the root case.
        const UiTextComponent::DrawBatch& prevBatch = batchStack.empty() ? UiTextComponent::DrawBatch() : batchStack.top();

        bool newFontFamilyPushed = false;
        switch (type)
        {
        case TextMarkup::TagType::Text:
        {
            batchStack.top().text = (static_cast<const TextMarkup::TextTag*>(currentTag))->text;

            // Replace escaped newlines with actual newlines
            batchStack.top().text = AZStd::regex_replace(batchStack.top().text, AZStd::regex("\\\\n"), "\n");

            break;
        }
        case TextMarkup::TagType::Image:
        {
            const TextMarkup::ImageTag* pImageTag = static_cast<const TextMarkup::ImageTag*>(currentTag);

            // Image tag isn't affected by any other tag so add a new draw batch directly to the output

            UiTextComponent::InlineImage::VAlign vAlign = UiTextComponent::InlineImage::VAlign::Baseline;
            if (pImageTag->m_vAlign == "baseline")
            {
                vAlign = UiTextComponent::InlineImage::VAlign::Baseline;
            }
            else if (pImageTag->m_vAlign == "top")
            {
                vAlign = UiTextComponent::InlineImage::VAlign::Top;
            }
            else if (pImageTag->m_vAlign == "center")
            {
                vAlign = UiTextComponent::InlineImage::VAlign::Center;
            }
            else if (pImageTag->m_vAlign == "bottom")
            {
                vAlign = UiTextComponent::InlineImage::VAlign::Bottom;
            }

            float imageHeight = fontAscent;

            if (pImageTag->m_height == "fontHeight")
            {
                imageHeight = fontHeight;
            }
            else if (pImageTag->m_height == "fontAscent")
            {
                imageHeight = fontAscent;
            }
            else if (!pImageTag->m_height.empty())
            {
                imageHeight = AZ::GetMax(0.0f, AZStd::stof(pImageTag->m_height));
            }

            UiTextComponent::InlineImage* inlineImage = new UiTextComponent::InlineImage(pImageTag->m_imagePathname,
                imageHeight,
                pImageTag->m_scale,
                vAlign,
                pImageTag->m_yOffset,
                pImageTag->m_leftPadding,
                pImageTag->m_rightPadding);
            inlineImages.push_back(inlineImage);

            UiTextComponent::DrawBatch drawBatch;
            drawBatch.image = inlineImages.back();
            output.push_back(drawBatch);

            break;
        }
        case TextMarkup::TagType::Bold:
        {
            if (prevBatch.font == fontFamilyStack.top()->bold)
            {
                // adjacent bold tags, no need to push a new batch
                break;
            }
            else if (prevBatch.font == fontFamilyStack.top()->italic)
            {
                // We're on a bold tag, but current font applied is
                // italic, so we apply the bold-italic font.
                batchStack.top().font = fontFamilyStack.top()->boldItalic;
            }
            else
            {
                batchStack.top().font = fontFamilyStack.top()->bold;
            }
            break;
        }
        case TextMarkup::TagType::Italic:
        {
            if (prevBatch.font == fontFamilyStack.top()->italic)
            {
                // adjacent italic tags, no need to push a new batch
                break;
            }
            else if (prevBatch.font == fontFamilyStack.top()->bold)
            {
                // We're on an italic tag, but current font applied is
                // bold, so we apply the bold-italic font.
                batchStack.top().font = fontFamilyStack.top()->boldItalic;
            }
            else
            {
                batchStack.top().font = fontFamilyStack.top()->italic;
            }
            break;
        }
        case TextMarkup::TagType::Anchor:
        {
            const TextMarkup::AnchorTag* pAnchorTag = static_cast<const TextMarkup::AnchorTag*>(currentTag);
            batchStack.top().action = pAnchorTag->action;
            batchStack.top().data = pAnchorTag->data;
            batchStack.top().clickableId = ++clickableId;
            break;
        }
        case TextMarkup::TagType::Font:
        {
            const TextMarkup::FontTag* pFontTag = static_cast<const TextMarkup::FontTag*>(currentTag);
            if (!(pFontTag->face.empty()))
            {
                FontFamilyPtr pFontFamily = gEnv->pCryFont->GetFontFamily(pFontTag->face.c_str());

                if (!pFontFamily)
                {
                    pFontFamily = gEnv->pCryFont->LoadFontFamily(pFontTag->face.c_str());
                }

                // Still need to check for pFontFamily validity since
                // Font Family load could have failed.
                if (pFontFamily)
                {
                    // Important to strongly reference the Font Family
                    // here otherwise it will de-ref once we go out of
                    // scope (and possibly unload).
                    fontFamilyRefs.insert(pFontFamily);

                    if (fontFamilyStack.top() != pFontFamily.get())
                    {
                        fontFamilyStack.push(pFontFamily.get());
                        newFontFamilyPushed = true;

                        // Reset font to default face for new font family
                        batchStack.top().font = pFontFamily->normal;
                    }
                }
                else
                {
                    AZ_Warning("UiTextComponent", false, "Failed to find font family referenced in markup (BuildDrawBatches): %s", pFontTag->face.c_str());
                }
            }
            const bool newColorNeeded = pFontTag->color != prevBatch.color;
            const bool tagHasValidColor = pFontTag->color != TextMarkup::ColorInvalid;
            if (newColorNeeded && tagHasValidColor)
            {
                batchStack.top().color = pFontTag->color;
            }
            break;
        }
        default:
        {
            break;
        }
        }

        // We only want to push a DrawBatch when it has text to display. We
        // store character data in separate tags. So when a bold tag is
        // traversed, we haven't yet visited its child character data:
        // <b> <!-- Bold tag DrawBatch created, no text yet -->
        //   <ch>Child character data here</ch>
        // </b>
        if (!batchStack.empty() && !batchStack.top().text.empty())
        {
            output.push_back(batchStack.top());
        }

        // Depth-first traversal of children tags.
        auto iter = currentTag->children.begin();
        for (; iter != currentTag->children.end(); ++iter)
        {
            BuildDrawBatches(output, fontFamilyRefs, inlineImages, fontHeight, fontAscent, batchStack, fontFamilyStack, *iter, clickableId);
        }

        // Children visited, clear newly created DrawBatch state.
        if (newBatchStackPushed)
        {
            batchStack.pop();
        }

        // Clear FontFamily state also.
        if (newFontFamilyPushed)
        {
            fontFamilyStack.pop();
        }
    }

    void BuildDrawBatchesAndAssignClickableIds(
        UiTextComponent::DrawBatchContainer& output,
        UiTextComponent::FontFamilyRefSet& fontFamilyRefs,
        UiTextComponent::InlineImageContainer& inlineImages,
        float fontHeight,
        float fontAscent,
        AZStd::stack<UiTextComponent::DrawBatch>& batchStack,
        AZStd::stack<FontFamily*>& fontFamilyStack,
        const TextMarkup::Tag* currentTag)
    {
        int clickableId = -1;
        BuildDrawBatches(
            output,
            fontFamilyRefs,
            inlineImages,
            fontHeight,
            fontAscent,
            batchStack,
            fontFamilyStack,
            currentTag,
            clickableId
            );
    }

    //! Use the given width and font context to insert newline breaks in the given DrawBatchList.
    //! This code is largely adapted from FFont::WrapText to work with DrawBatch objects.
    void InsertNewlinesToWrapText(
        UiTextComponent::DrawBatchContainer& drawBatches,
        const STextDrawContext& ctx,
        float elementWidth)
    {
        if (drawBatches.empty())
        {
            return;
        }

        // Keep track of the last space char we encountered as ideal
        // locations for inserting newlines for word-wrapping. We also need
        // to track which DrawBatch contained the last-encountered space.
        const char* pLastSpace = NULL;
        UiTextComponent::DrawBatch* lastSpaceBatch = nullptr;
        int lastSpace = -1;
        int lastSpaceIndexInBatch = -1;
        float lastSpaceWidth = 0.0f;

        int curChar = 0;
        float curLineWidth = 0.0f;
        float biggestLineWidth = 0.0f;
        float widthSum = 0.0f;

        // When iterating over batches, we need to know the previous
        // character, which we can only obtain if we keep track of the last
        // batch we iterated on.
        UiTextComponent::DrawBatch* prevBatch = &drawBatches.front();

        // Map draw batches to text indices where spaces should be restored
        // (more details below); this happens after we've inserted newlines
        // for word-wrapping.
        using SpaceIndices = AZStd::vector < int >;
        AZStd::unordered_map<UiTextComponent::DrawBatch*, SpaceIndices> batchSpaceIndices;

        // Iterate over all drawbatches, calculating line length and add newlines
        // when element width is exceeded. Reset line length when a newline is added
        // or a newline is encountered.
        for (UiTextComponent::DrawBatch& drawBatch : drawBatches)
        {
            // If this entry ultimately ends up not having any space char
            // indices associated with it, we will simply skip iterating over
            // it later.
            batchSpaceIndices.insert(&drawBatch);

            int batchCurChar = 0;

            Utf8::Unchecked::octet_iterator pChar(drawBatch.text.data());
            uint32_t prevCh = 0;
            while (uint32_t ch = *pChar)
            {
                size_t maxSize = 5;
                char codepoint[5] = { 0 };
                char* codepointPtr = codepoint;
                Utf8::Unchecked::octet_iterator<AZStd::string::iterator>::to_utf8_sequence(ch, codepointPtr, maxSize);

                float curCharWidth = drawBatch.font->GetTextSize(codepoint, true, ctx).x;

                if (prevCh && ctx.m_kerningEnabled)
                {
                    curCharWidth += drawBatch.font->GetKerning(prevCh, ch, ctx).x;
                }

                if (prevCh)
                {
                    curCharWidth += ctx.m_tracking;
                }

                prevCh = ch;

                // keep track of spaces
                // they are good for splitting the string
                if (ch == ' ')
                {
                    lastSpace = curChar;
                    lastSpaceIndexInBatch = batchCurChar;
                    lastSpaceBatch = &drawBatch;
                    lastSpaceWidth = curLineWidth + curCharWidth;
                    pLastSpace = pChar.base();
                    assert(*pLastSpace == ' ');
                }

                bool prevCharWasNewline = false;
                const bool isFirstChar = pChar.base() == drawBatch.text.c_str();
                if (ch && !isFirstChar)
                {
                    const char* pPrevCharStr = pChar.base() - 1;
                    prevCharWasNewline = pPrevCharStr[0] == '\n';
                }
                else if (isFirstChar)
                {
                    // Since prevBatch is initialized to front of drawBatches,
                    // check to ensure there was a previous batch.
                    const bool prevBatchValid = prevBatch != &drawBatch;

                    if (prevBatchValid && !prevBatch->text.empty())
                    {
                        prevCharWasNewline = prevBatch->text.at(prevBatch->text.length() - 1) == '\n';
                    }
                }

                // line must contain some content, otherwise single characters larger than
                // the element width would wrap themselves.
                const bool lineContainsContent = curLineWidth > 0.0f;

                // if line exceed allowed width, split it
                const bool lineWidthExceeded = lineContainsContent && (curLineWidth + curCharWidth) > elementWidth;

                if (prevCharWasNewline || (lineWidthExceeded && ch))
                {
                    if (prevCharWasNewline)
                    {
                        // Reset the current line width to account for newline
                        curLineWidth = curCharWidth;
                        widthSum += curLineWidth;
                    }
                    else if ((lastSpace > 0) && ((curChar - lastSpace) < 16) && (curChar - lastSpace >= 0)) // 16 is the default threshold
                    {
                        *(char*)pLastSpace = '\n';  // This is safe inside UTF-8 because space is single-byte codepoint
                        batchSpaceIndices.at(lastSpaceBatch).push_back(lastSpaceIndexInBatch);

                        if (lastSpaceWidth > biggestLineWidth)
                        {
                            biggestLineWidth = lastSpaceWidth;
                        }

                        curLineWidth = curLineWidth - lastSpaceWidth + curCharWidth;
                        widthSum += curLineWidth;
                    }
                    else
                    {
                        char* pBuf = pChar.base();
                        AZStd::string::size_type bytesProcessed = pBuf - drawBatch.text.c_str();
                        drawBatch.text.insert(bytesProcessed, "\n"); // Insert the newline, this invalidates the iterator
                        pBuf = drawBatch.text.data() + bytesProcessed; // In case reallocation occurs, we ensure we are inside the new buffer
                        assert(*pBuf == '\n');
                        pChar = Utf8::Unchecked::octet_iterator(pBuf); // pChar once again points inside the target string, at the current character
                        assert(*pChar == ch);
                        ++pChar;
                        ++curChar;
                        ++batchCurChar;

                        if (curLineWidth > biggestLineWidth)
                        {
                            biggestLineWidth = curLineWidth;
                        }

                        widthSum += curLineWidth;
                        curLineWidth = curCharWidth;
                    }

                    lastSpaceWidth = 0;
                    lastSpace = 0;
                }
                else
                {
                    curLineWidth += curCharWidth;
                }

                curChar += LyShine::GetMultiByteCharSize(ch);
                batchCurChar += LyShine::GetMultiByteCharSize(ch);
                ++pChar;
            }

            prevBatch = &drawBatch;
        }

        // We insert newline breaks (perform word-wrapping) in-place for
        // formatting purposes, but we restore the original delimiting
        // space characters now. This resolves a lot of issues with indices
        // mismatching between the rendered string content and the original
        // string.
        //
        // This seems unintuitive since (above) we simply (in some cases)
        // replace the space character with newline, so inserting an additional
        // space now would mismatch the original string contents even further.
        // However, since draw batch "lines" are delimited by newline, the
        // newline character will eventually be removed (because it will be
        // implied). So at this part in the pipeline, the strings will not
        // match in content or length, but eventually will.
        for (auto& batchSpaceList : batchSpaceIndices)
        {
            UiTextComponent::DrawBatch* drawBatch = batchSpaceList.first;
            const SpaceIndices& spaceIndices = batchSpaceList.second;

            int insertOffset = 0;
            for (const int spaceIndex : spaceIndices)
            {
                drawBatch->text.insert(spaceIndex + insertOffset, 1, ' ');

                // Each time we insert, our indices our off by one.
                ++insertOffset;
            }
        }
    }

    //! Given a "flat" list of DrawBatches, separate them by newline and place in output.
    void CreateBatchLines(
        UiTextComponent::DrawBatchLines& output,
        UiTextComponent::DrawBatchContainer& drawBatches,
        FontFamily* defaultFontFamily)
    {
        UiTextComponent::DrawBatchLineContainer& lineList = output.batchLines;
        lineList.push_back(UiTextComponent::DrawBatchLine());

        for (UiTextComponent::DrawBatch& drawBatch : drawBatches)
        {
            AZStd::string::size_type newlineIndex = drawBatch.text.find('\n');

            if (newlineIndex == AZStd::string::npos)
            {
                lineList.back().drawBatchList.push_back(drawBatch);
                continue;
            }
            while (newlineIndex != AZStd::string::npos)
            {
                // Found a newline within a single drawbatch, so split
                // into two batches, one for the current line, and one
                // for the following.
                UiTextComponent::DrawBatchContainer& currentLine = lineList.back().drawBatchList;
                lineList.push_back(UiTextComponent::DrawBatchLine());
                UiTextComponent::DrawBatchContainer& newLine = lineList.back().drawBatchList;

                const bool moreCharactersAfterNewline = drawBatch.text.length() - 1 > newlineIndex;

                // Note that we purposely build the string such that the newline
                // character is truncated from the string. If it were included,
                // it would be doubly-accounted for in the renderer.
                UiTextComponent::DrawBatch splitBatch(drawBatch);
                splitBatch.text = drawBatch.text.substr(0, newlineIndex);
                currentLine.push_back(splitBatch);

                // Start a new newline
                if (moreCharactersAfterNewline)
                {
                    const AZStd::string::size_type endOfStringIndex = drawBatch.text.length() - newlineIndex - 1;
                    drawBatch.text = drawBatch.text.substr(newlineIndex + 1, endOfStringIndex);
                    newlineIndex = drawBatch.text.find('\n');

                    if (newlineIndex == AZStd::string::npos)
                    {
                        newLine.push_back(drawBatch);
                    }
                }
                else
                {
                    break;
                }
            }
        }

        // Push an empty DrawBatch if the string happened to end with a
        // newline but no following text (e.g. "Hello\n").
        // :TODO: is this still needed? Can the final DrawBatchLine be removed
        // altogether if it has no content?
        if (lineList.back().drawBatchList.empty())
        {
            lineList.back().drawBatchList.push_back(UiTextComponent::DrawBatch());
            lineList.back().drawBatchList.front().font = defaultFontFamily->normal;
        }
    }

    void AssignLineSizes(
        UiTextComponent::DrawBatchLines& output,
        [[maybe_unused]] FontFamily* fontFamily,
        const STextDrawContext& ctx,
        bool excludeTrailingSpace = true)
    {
        output.height = 0.0f;

        for (UiTextComponent::DrawBatchLine& drawBatchLine : output.batchLines)
        {
            // First calculate the batch sizes
            for (auto drawBatchIterator = drawBatchLine.drawBatchList.begin(); drawBatchIterator != drawBatchLine.drawBatchList.end(); ++drawBatchIterator)
            {
                // Exclude trailing space from the last batch in the line
                bool excludeTrailingSpaceFromLine = excludeTrailingSpace ? (AZStd::next(drawBatchIterator) == drawBatchLine.drawBatchList.end()) : false;
                drawBatchIterator->CalculateSize(ctx, excludeTrailingSpaceFromLine);
            }

            // Calculate the batch y offsets from the text y position based on the text's baseline
            for (UiTextComponent::DrawBatch& drawBatch : drawBatchLine.drawBatchList)
            {
                drawBatch.CalculateYOffset(ctx.m_size.y, output.baseline);
            }

            // Figure out the highest batch offset above the text y position
            float minYOffset = 0.0f;
            for (UiTextComponent::DrawBatch& drawBatch : drawBatchLine.drawBatchList)
            {
                minYOffset = AZ::GetMin<float>(minYOffset, drawBatch.yOffset);
            }
            // Update the batch y offsets so they all become a positive offset from the y draw position of the batch line
            for (UiTextComponent::DrawBatch& drawBatch : drawBatchLine.drawBatchList)
            {
                drawBatch.yOffset -= minYOffset;
            }

            // Now calculate the size of the line
            float width = 0.0f;
            float height = 0.0f;
            for (const UiTextComponent::DrawBatch& drawBatch : drawBatchLine.drawBatchList)
            {
                width += drawBatch.size.GetX();
                height = AZ::GetMax<float>(height, drawBatch.yOffset + drawBatch.size.GetY());
            }

            drawBatchLine.lineSize.SetX(width);
            drawBatchLine.lineSize.SetY(height);

            output.height += height;
        }
    }

    //! Takes a flat list of draw batches (created by the Draw Batch Builder) and groups them
    //! by line, taking the element width into account, and also taking any newline characters
    //! that may already exist within the character data of the DrawBatch objects
    void BatchAwareWrapText(
        UiTextComponent::DrawBatchLines& output,
        UiTextComponent::DrawBatchContainer& drawBatches,
        FontFamily* fontFamily,
        const STextDrawContext& ctx,
        float elementWidth,
        bool excludeTrailingSpaceWidth = true)
    {
        InsertNewlinesToWrapText(drawBatches, ctx, elementWidth);
        CreateBatchLines(output, drawBatches, fontFamily);
        AssignLineSizes(output, fontFamily, ctx, excludeTrailingSpaceWidth);
    }

    //! Takes a flat list of draw batches (created by the Draw Batch Builder) that may contain
    //! non-text elements (such as images) and groups them by line, taking the element width into
    //! account, and also taking any newline characters that may already exist within the character
    //! data of the DrawBatch objects
    void BatchAwareWrapTextWithImages(
        UiTextComponent::DrawBatchLines& output,
        UiTextComponent::DrawBatchContainer& drawBatches,
        FontFamily* fontFamily,
        const STextDrawContext& ctx,
        float elementWidth,
        bool excludeTrailingSpaceWidth = true
    )
    {
        // Create batch lines based on existing newline characters
        CreateBatchLines(output, drawBatches, fontFamily);

        // Iterate over each line and split the line if it runs over the allowed width
        for (auto batchLinesIterator = output.batchLines.begin(); batchLinesIterator != output.batchLines.end(); batchLinesIterator++)
        {
            UiTextComponent::DrawBatchLine newBatchLineOut;

            // Check whether the line exceeds the allowed width and split the line if needed
            bool split = batchLinesIterator->CheckAndSplitLine(ctx, elementWidth, newBatchLineOut);
            if (split && !newBatchLineOut.drawBatchList.empty())
            {
                // Insert new line
                output.batchLines.insert_after(batchLinesIterator, newBatchLineOut);
            }
        }

        AssignLineSizes(output, fontFamily, ctx, excludeTrailingSpaceWidth);
    }

    //! Returns the maximum scale value along the X and Y axes for the given entity's transform.
    float GetMax2dTransformScale(AZ::EntityId entityId)
    {
        AZ::Matrix4x4 elementTransform = AZ::Matrix4x4::CreateIdentity();
        EBUS_EVENT_ID(entityId, UiTransformBus, GetTransformToCanvasSpace, elementTransform);
        const AZ::Vector3 elementScale = elementTransform.RetrieveScale();
        return AZ::GetMax<float>(elementScale.GetX(), elementScale.GetY());
    }

    //! Returns the size of the given font after scale-to-device and entity transform scales have been applied.
    int CalcRequestFontSize(float fontSize, AZ::EntityId entityId)
    {
        const float max2dTransformScale = GetMax2dTransformScale(entityId);
        return static_cast<int>(fontSize * max2dTransformScale);
    }

    //! Clips an inline image markup quad and UVs to the defined region
    //!
    //! \param imageQuad Array of 4 vertices defining the image quad
    //! \param uvs Array of 4 UV coordinates for the textured quad
    //! \param points Region to clip quad and UVs to
    //! \param drawBatch The DrawBatch containing the inline image
    //! \param imageStartPos Upper-left coordinate of unclipped image
    //! \param imageEndPos Bottom-right coordinate of unclipped image
    void ClipImageQuadAndUvs(
        AZ::Vector3* imageQuad,
        AZ::Vector2* uvs,
        const UiTransformInterface::RectPoints& points,
        const UiTextComponent::DrawBatch& drawBatch,
        const AZ::Vector2& imageStartPos,
        const AZ::Vector2& imageEndPos)
    {
        const bool imageLeftOfElement = imageStartPos.GetX() < points.TopLeft().GetX();
        const bool imageRightOfElement = imageEndPos.GetX() > points.TopRight().GetX();
        const bool imageTopOfElement = imageStartPos.GetY() < points.TopLeft().GetY();
        const bool imageBottomOfElement = imageEndPos.GetY() > points.BottomLeft().GetY();

        if (imageLeftOfElement)
        {
            imageQuad[0].SetX(AZStd::GetMin<float>(points.TopLeft().GetX(), imageEndPos.GetX()));
            imageQuad[3].SetX(imageQuad[0].GetX());
            const float diff = points.TopLeft().GetX() - imageStartPos.GetX();
            const float uvScale = diff / drawBatch.image->m_size.GetX();
            uvs[0].SetX(uvScale);
            uvs[3].SetX(uvScale);
        }

        if (imageRightOfElement)
        {
            imageQuad[1].SetX(AZStd::GetMax<float>(points.TopRight().GetX(), imageStartPos.GetX()));
            imageQuad[2].SetX(imageQuad[1].GetX());
            const float diff = imageEndPos.GetX() - points.TopRight().GetX();
            const float uvScale = diff / drawBatch.image->m_size.GetX();
            uvs[1].SetX(1.0f - uvScale);
            uvs[2].SetX(1.0f - uvScale);
        }

        if (imageTopOfElement)
        {
            imageQuad[0].SetY(AZStd::GetMin<float>(points.TopLeft().GetY(), imageEndPos.GetY()));
            imageQuad[1].SetY(imageQuad[0].GetY());
            const float diff = points.TopLeft().GetY() - imageStartPos.GetY();
            const float uvScale = diff / drawBatch.image->m_size.GetY();
            uvs[0].SetY(uvScale);
            uvs[1].SetY(uvScale);
        }

        if (imageBottomOfElement)
        {
            imageQuad[2].SetY(AZStd::GetMax<float>(points.BottomLeft().GetY(), imageStartPos.GetY()));
            imageQuad[3].SetY(imageQuad[2].GetY());
            const float diff = imageEndPos.GetY() - points.BottomLeft().GetY();
            const float uvScale = diff / drawBatch.image->m_size.GetY();
            uvs[2].SetY(1.0f - uvScale);
            uvs[3].SetY(1.0f - uvScale);
        }
    }

    //! Returns the maximum number of non-overflowing lines the given element can display.
    //!
    //! Note that this assumes the lines have been word-wrapped and don't overflow horizontally.
    int GetNumNonOverflowingLinesForElement(
        const UiTextComponent::DrawBatchLineContainer& batchLines,
        const AZ::Vector2& currentElementSize,
        float lineSpacing)
    {
        int maxLinesElementCanHold = 0;
        float nonOverflowingLineHeight = 0.0f;
        for (const auto& batchLine : batchLines)
        {
            float lineHeight = batchLine.lineSize.GetY();

            // Only consider line spacing when there are multiple lines (this
            // also handles the case when there is only one line).
            if (maxLinesElementCanHold >= 1)
            {
                lineHeight += lineSpacing;
            }

            // Add up the lines that fit vertically within the element
            if (nonOverflowingLineHeight + lineHeight < currentElementSize.GetY())
            {
                maxLinesElementCanHold++;
                nonOverflowingLineHeight += lineHeight;
            }
            else
            {
                break;
            }
        }

        // It's possible the element can't accommodate a single line of text (too small for text),
        // so in this case we just say the element can accommodate one line anyways to avoid
        // div by zero checks etc.
        maxLinesElementCanHold = AZStd::GetMax<int>(maxLinesElementCanHold, 1);
        return maxLinesElementCanHold;
    }

}   // anonymous namespace

////////////////////////////////////////////////////////////////////////////////////////////////////
// PUBLIC MEMBER FUNCTIONS
////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////
UiTextComponent::InlineImage::InlineImage(const AZStd::string& texturePathname,
    float height,
    float scale,
    VAlign vAlign,
    float yOffset,
    float leftPadding,
    float rightPadding)
{
    m_filepath = texturePathname;
    AzFramework::ApplicationRequests::Bus::Broadcast(&AzFramework::ApplicationRequests::NormalizePath, m_filepath);
    m_texture.reset();
    m_size = AZ::Vector2(0.0f, 0.0f);
    m_vAlign = vAlign;
    m_yOffset = yOffset;
    m_leftPadding = leftPadding;
    m_rightPadding = rightPadding;
    m_atlas = nullptr;

    TextureAtlasNamespace::TextureAtlasRequestBus::BroadcastResult(m_atlas, &TextureAtlasNamespace::TextureAtlasRequests::FindAtlasContainingImage, m_filepath);
    if (m_atlas)
    {
        m_texture = m_atlas->GetTexture();
        m_coordinates = m_atlas->GetAtlasCoordinates(m_filepath);
        m_size = AZ::Vector2(static_cast<float>(m_coordinates.GetWidth()), static_cast<float>(m_coordinates.GetHeight()));
    }
    else
    {
        // Load the texture
        m_texture = CDraw2d::LoadTexture(m_filepath);
        if (m_texture)
        {
            AZ::RHI::Size size = m_texture->GetDescriptor().m_size;
            m_size = AZ::Vector2(static_cast<float>(size.m_width), static_cast<float>(size.m_height));
        }
    }

    // Adjust size to the specified height while keeping the aspect ratio
    float aspectRatio = m_size.GetY() != 0.0f ? m_size.GetX() / m_size.GetY() : 1.0f;
    m_size.SetY(height);
    m_size.SetX(m_size.GetY() * aspectRatio);

    // Apply specified scale
    m_size *= scale;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
UiTextComponent::InlineImage::~InlineImage()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiTextComponent::InlineImage::OnAtlasLoaded(const TextureAtlasNamespace::TextureAtlas* atlas)
{
    if (!m_atlas)
    {
        m_coordinates = atlas->GetAtlasCoordinates(m_filepath);
        if (m_coordinates.GetWidth() > 0)
        {
            m_atlas = atlas;
            m_texture = m_atlas->GetTexture();
            return true;
        }
    }
    return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiTextComponent::InlineImage::OnAtlasUnloaded(const TextureAtlasNamespace::TextureAtlas* atlas)
{
    if (m_atlas == atlas)
    {
        TextureAtlasNamespace::TextureAtlasRequestBus::BroadcastResult(m_atlas, &TextureAtlasNamespace::TextureAtlasRequests::FindAtlasContainingImage, m_filepath);
        if (m_atlas)
        {
            m_texture = m_atlas->GetTexture();
            m_coordinates = m_atlas->GetAtlasCoordinates(m_filepath);
        }
        else
        {
            // Load the texture
            m_texture = CDraw2d::LoadTexture(m_filepath);
        }
        return true;
    }
    return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
UiTextComponent::DrawBatch::DrawBatch()
    : color(TextMarkup::ColorInvalid)
    , font(nullptr)
    , image(nullptr)
    , size(0.0f, 0.0f)
    , yOffset(0.0f)
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
UiTextComponent::DrawBatch::Type UiTextComponent::DrawBatch::GetType() const
{
    if (image)
    {
        return UiTextComponent::DrawBatch::Type::Image;
    }

    return UiTextComponent::DrawBatch::Type::Text;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTextComponent::DrawBatch::CalculateSize(const STextDrawContext& ctx, bool excludeTrailingSpace)
{
    if (GetType() == UiTextComponent::DrawBatch::Type::Text)
    {
        AZStd::string displayString(text);

        // For now, we only use batch text size for rendering purposes,
        // so we don't account for trailing spaces to avoid alignment
        // and formatting issues. In the future, we may need to
        // calculate batch size by use case (rendering, "true" size,
        // etc.). rather than assume one-size-fits-all.

        // Trim right
        if (excludeTrailingSpace)
        {
            if (displayString.length() > 0)
            {
                AZStd::string::size_type endpos = displayString.find_last_not_of(" \t\n\v\f\r");
                if ((endpos != AZStd::string::npos) && (endpos != displayString.length() - 1))
                {
                    displayString.erase(endpos + 1);
                }
            }
        }

        Vec2 textSize = font->GetTextSize(displayString.c_str(), true, ctx);
        size = AZ::Vector2(textSize.x, textSize.y);
    }
    else if (GetType() == UiTextComponent::DrawBatch::Type::Image)
    {
        size = image->m_size;
        size.SetX(size.GetX() + image->m_leftPadding + image->m_rightPadding);
    }
    else
    {
        AZ_Assert(false, "Unknown DrawBatch Type");
        size = AZ::Vector2(0.0f, 0.0f);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTextComponent::DrawBatch::CalculateYOffset(float fontSize, float baseline)
{
    if (GetType() == UiTextComponent::DrawBatch::Type::Text)
    {
        yOffset = 0.0f;
    }
    else if (GetType() == UiTextComponent::DrawBatch::Type::Image)
    {
        float imageHeight = size.GetY();

        switch (image->m_vAlign)
        {
        case InlineImage::VAlign::Baseline:
        {
            yOffset = (baseline - imageHeight);
        }
        break;

        case InlineImage::VAlign::Top:
        {
            yOffset = 0.0f;
        }
        break;

        case InlineImage::VAlign::Center:
        {
            yOffset = (fontSize - imageHeight) / 2.0f;
        }
        break;

        case InlineImage::VAlign::Bottom:
        {
            yOffset = fontSize - imageHeight;
        }
        break;
        }

        yOffset += image->m_yOffset;
    }
    else
    {
        AZ_Assert(false, "Unknown DrawBatch Type");
        yOffset = 0.0f;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
int UiTextComponent::DrawBatch::GetNumChars() const
{
    if (GetType() == UiTextComponent::DrawBatch::Type::Text)
    {
        return LyShine::GetUtf8StringLength(text);
    }
    else if (GetType() == UiTextComponent::DrawBatch::Type::Image)
    {
        return 1;
    }
    else
    {
        AZ_Assert(false, "Unknown DrawBatch Type");
        return 0;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiTextComponent::DrawBatch::GetOverflowInfo(const STextDrawContext& ctx,
    float availableWidth, bool skipFirstChar, OverflowInfo& overflowInfoOut)
{
    overflowInfoOut.overflowIndex = -1;
    overflowInfoOut.overflowCharIsSpace = false;
    overflowInfoOut.widthUntilOverflowOrTotalWidth = -1.0f;
    overflowInfoOut.overflowCharWidth = -1.0f;
    overflowInfoOut.lastSpaceIndex = -1;
    overflowInfoOut.isSpaceAtEnd = false;

    if (GetType() == UiTextComponent::DrawBatch::Type::Text)
    {
        int batchCurChar = 0;

        float width = 0.0f;

        float maxEffectOffsetX = font->GetMaxEffectOffset(ctx.m_fxIdx).x;

        Utf8::Unchecked::octet_iterator pChar(text.data());
        uint32_t prevCh = 0;
        while (uint32_t ch = *pChar)
        {
            size_t maxSize = 5;
            char codepoint[5] = { 0 };
            char* codepointPtr = codepoint;
            Utf8::Unchecked::octet_iterator<AZStd::string::iterator>::to_utf8_sequence(ch, codepointPtr, maxSize);

            float curCharWidth = font->GetTextSize(codepoint, true, ctx).x;
            if (prevCh)
            {
                curCharWidth -= maxEffectOffsetX;
            }

            if (prevCh && ctx.m_kerningEnabled)
            {
                curCharWidth += font->GetKerning(prevCh, ch, ctx).x;
            }

            if (prevCh)
            {
                curCharWidth += ctx.m_tracking;
            }

            prevCh = ch;

            const bool lineWidthExceeded = (width + curCharWidth) > availableWidth;
            if (lineWidthExceeded)
            {
                if (!skipFirstChar || batchCurChar != 0)
                {
                    overflowInfoOut.overflowIndex = batchCurChar;
                    overflowInfoOut.overflowCharIsSpace = (ch == ' ');
                    overflowInfoOut.widthUntilOverflowOrTotalWidth = width;
                    overflowInfoOut.overflowCharWidth = curCharWidth;

                    return true;
                }
            }

            // keep track of spaces
            // they are good for splitting the string
            if (ch == ' ')
            {
                overflowInfoOut.lastSpaceIndex = batchCurChar;
            }

            width += curCharWidth;

            batchCurChar += 1;
            ++pChar;

            if (ch == ' ' && !*pChar)
            {
                overflowInfoOut.isSpaceAtEnd = true;
            }
        }

        overflowInfoOut.widthUntilOverflowOrTotalWidth = width;

        return false;
    }
    else if (GetType() == UiTextComponent::DrawBatch::Type::Image)
    {
        float totalImageSize = image->m_size.GetX() + image->m_leftPadding + image->m_rightPadding;
        if (!skipFirstChar && totalImageSize > availableWidth)
        {
            overflowInfoOut.overflowIndex = 0;
            overflowInfoOut.overflowCharIsSpace = false;
            overflowInfoOut.widthUntilOverflowOrTotalWidth = 0.0f;
            overflowInfoOut.overflowCharWidth = totalImageSize;

            return true;
        }
        else
        {
            overflowInfoOut.widthUntilOverflowOrTotalWidth = totalImageSize;
            return false;
        }
    }
    else
    {
        AZ_Assert(false, "Unknown DrawBatch Type");
        return false;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTextComponent::DrawBatch::Split(int atCharIndex, DrawBatch& newDrawBatchOut)
{
    newDrawBatchOut = *this;

    if (GetType() == UiTextComponent::DrawBatch::Type::Text)
    {
        AZ_Assert(atCharIndex >= 0 && atCharIndex < LyShine::GetUtf8StringLength(text), "Text index out of range. Can't split batch");

        // Set text for new batch
        int numBytesToSplit = LyShine::GetByteLengthOfUtf8Chars(text.c_str(), atCharIndex);
        newDrawBatchOut.text = text.substr(numBytesToSplit);

        // Update this batch's text
        text = atCharIndex > 0 ? text.substr(0, numBytesToSplit) : "";
    }
    else if (GetType() == UiTextComponent::DrawBatch::Type::Image)
    {
        AZ_Assert(atCharIndex == 0, "Image index out of range. Can't split batch");

        // Update this batch's image
        image = nullptr;
    }
    else
    {
        AZ_Assert(false, "Unknown DrawBatch Type");
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
UiTextComponent::DrawBatchLine::DrawBatchLine()
    : lineSize(0.0f, 0.0f)
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiTextComponent::DrawBatchLine::CheckAndSplitLine(const STextDrawContext& ctx,
    float maxWidth,
    DrawBatchLine& newDrawBatchLineOut)
{
    bool lineSplit = false;

    // Allow a space at the end of the line to overflow. This is to remain consistent with the non-image
    // line split implementation. If the space at the end of the line was simply removed, the character
    // indexes wouldn't match the localized text character indexes, and would cause issues with cursor positioning
    const bool allowSpaceToOverflow = true;

    // Keep track of available width left
    float availableWidth = maxWidth;

    // Keep track of the last good place to split the line, such as a space
    UiTextComponent::DrawBatchContainer::iterator lastBatchWithSpaceIterator = drawBatchList.end();
    int lastSpaceIndexInBatch = -1;
    int isLastSpaceAtEndOfBatch = false;
    int numCharsSinceLastSpace = -1;

    // Iterate over the line's draw batches and split the line if they run over the allowed width
    UiTextComponent::DrawBatchContainer::iterator drawBatchIterator = drawBatchList.begin();
    while (drawBatchIterator != drawBatchList.end())
    {
        int numCharsInBatch = drawBatchIterator->GetNumChars();

        // Can't split the first char of the first batch in the line even if it is wider than the available width
        bool skipFirstChar = (drawBatchIterator == drawBatchList.begin());

        // Check whether current batch is overflowing and get overflow info
        UiTextComponent::DrawBatch::OverflowInfo overflowInfoOut;
        bool overflowing = drawBatchIterator->GetOverflowInfo(ctx, availableWidth, skipFirstChar, overflowInfoOut);

        // Check if this batch has a space and remember for later
        if (overflowInfoOut.lastSpaceIndex >= 0)
        {
            // Remember the space unless it's the first character in the line (we don't want to end up with a line consisting of just one space)
            if (overflowInfoOut.lastSpaceIndex > 0 || drawBatchIterator != drawBatchList.begin())
            {
                lastBatchWithSpaceIterator = drawBatchIterator;
                lastSpaceIndexInBatch = overflowInfoOut.lastSpaceIndex;
                isLastSpaceAtEndOfBatch = overflowInfoOut.isSpaceAtEnd;
                numCharsSinceLastSpace = (overflowing ? overflowInfoOut.overflowIndex : numCharsInBatch - 1) - lastSpaceIndexInBatch;
            }
        }
        else
        {
            if (lastBatchWithSpaceIterator != drawBatchList.end())
            {
                numCharsSinceLastSpace += (overflowing ? overflowInfoOut.overflowIndex : numCharsInBatch);
            }
        }

        const int maxCharsSinceLastSpace = 16;
        if (numCharsSinceLastSpace > maxCharsSinceLastSpace)
        {
            // Space is now too far away
            lastBatchWithSpaceIterator = drawBatchList.end();
            lastSpaceIndexInBatch = -1;
            isLastSpaceAtEndOfBatch = false;
            numCharsSinceLastSpace = -1;
        }

        if (overflowing)
        {
            // Find a batch to split
            UiTextComponent::DrawBatchContainer::iterator splitBatchIterator = drawBatchList.end();
            int splitBatchAtIndex = -1;

            // First check whether the overflow character is a space that we should allow to overflow
            if (allowSpaceToOverflow && overflowInfoOut.overflowCharIsSpace)
            {
                // Allow this space to overflow

                // Check if the space is the last character in the batch
                if (overflowInfoOut.overflowIndex == numCharsInBatch - 1)
                {
                    // Just move on to the next batch
                    availableWidth -= overflowInfoOut.widthUntilOverflowOrTotalWidth + overflowInfoOut.overflowCharWidth;
                }
                else
                {
                    // Split one character after the space
                    splitBatchIterator = drawBatchIterator;
                    splitBatchAtIndex = overflowInfoOut.overflowIndex + 1;
                }
            }
            // Next check if there's a batch that contains a space for splitting
            else if (lastBatchWithSpaceIterator != drawBatchList.end())
            {
                // Split the last batch that has a space
                if (isLastSpaceAtEndOfBatch && lastBatchWithSpaceIterator != drawBatchIterator)
                {
                    // The space is at the end of the batch but there is a batch after it so move the next batch to a new line
                    splitBatchIterator = lastBatchWithSpaceIterator;
                    ++splitBatchIterator;
                    splitBatchAtIndex = 0;
                }
                else
                {
                    // Split the batch that has the space
                    // We know there's another character after the space because either overflow occurred in the last batch
                    // or the space wasn't the last character in a previous batch
                    splitBatchIterator = lastBatchWithSpaceIterator;
                    splitBatchAtIndex = lastSpaceIndexInBatch + 1;
                }
            }
            else
            {
                // Must split the current batch
                splitBatchIterator = drawBatchIterator;
                splitBatchAtIndex = overflowInfoOut.overflowIndex;
            }

            if (splitBatchIterator != drawBatchList.end())
            {
                UiTextComponent::DrawBatch newDrawBatchOut;

                // Create a new line
                newDrawBatchLineOut.drawBatchList.clear();

                if (splitBatchAtIndex > 0)
                {
                    // Split the batch
                    splitBatchIterator->Split(splitBatchAtIndex, newDrawBatchOut);

                    // Add the new draw batch to the new batch line
                    newDrawBatchLineOut.drawBatchList.push_back(newDrawBatchOut);

                    // Keep the current batch in its own line
                    ++splitBatchIterator;
                }

                // Add the remaining draw batches to the new batch line
                if (splitBatchIterator != drawBatchList.end())
                {
                    newDrawBatchLineOut.drawBatchList.splice(newDrawBatchLineOut.drawBatchList.end(), drawBatchList, splitBatchIterator, drawBatchList.end());
                }

                lineSplit = true;

                break;
            }
        }
        else
        {
            availableWidth -= overflowInfoOut.widthUntilOverflowOrTotalWidth; // subtract total width
        }

        ++drawBatchIterator;
    }

    return lineSplit;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
UiTextComponent::DrawBatchLines::~DrawBatchLines()
{
    Clear();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTextComponent::DrawBatchLines::Clear()
{
    batchLines.clear();
    fontFamilyRefs.clear();
    for (auto image : inlineImages)
    {
        delete image;
    }
    inlineImages.clear();
    height = 0.0f;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
UiTextComponent::UiTextComponent()
    : m_text("My string")
    , m_color(1.0f, 1.0f, 1.0f, 1.0f)
    , m_alpha(1.0f)
    , m_fontSize(32)
    , m_requestFontSize(static_cast<int>(m_fontSize))
    , m_textHAlignment(IDraw2d::HAlign::Center)
    , m_textVAlignment(IDraw2d::VAlign::Center)
    , m_charSpacing(0.0f)
    , m_lineSpacing(0.0f)
    , m_currFontSize(m_fontSize)
    , m_currCharSpacing(m_charSpacing)
    , m_font(nullptr)
    , m_fontFamily(nullptr)
    , m_fontEffectIndex(0)
    , m_displayedTextFunction(DefaultDisplayedTextFunction)
    , m_overrideColor(m_color)
    , m_overrideAlpha(m_alpha)
    , m_overrideFontFamily(nullptr)
    , m_overrideFontEffectIndex(m_fontEffectIndex)
    , m_isColorOverridden(false)
    , m_isAlphaOverridden(false)
    , m_isFontFamilyOverridden(false)
    , m_isFontEffectOverridden(false)
    , m_textSelectionColor(0.0f, 0.0f, 0.0f, 1.0f)
    , m_selectionStart(-1)
    , m_selectionEnd(-1)
    , m_cursorLineNumHint(-1)
    , m_overflowMode(OverflowMode::OverflowText)
    , m_wrapTextSetting(WrapTextSetting::NoWrap)
    , m_clipOffset(0.0f)
    , m_clipOffsetMultiplier(1.0f)
    , m_isMarkupEnabled(false)
{
    static const AZStd::string DefaultUi("default-ui");
    m_fontFilename.SetAssetPath(DefaultUi.c_str());

    if (gEnv && gEnv->pCryFont) // these will be null in RC.exe
    {
        m_fontFamily = gEnv->pCryFont->GetFontFamily(DefaultUi.c_str());

        if (!m_fontFamily)
        {
            m_fontFamily = gEnv->pCryFont->LoadFontFamily(DefaultUi.c_str());
        }
    }

    if (m_fontFamily)
    {
        m_font = m_fontFamily->normal;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
UiTextComponent::~UiTextComponent()
{
    FreeRenderCacheMemory();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTextComponent::ResetOverrides()
{
    bool fontChanged = false;
    bool colorChanged = false;
    bool alphaChanged = false;

    if (m_overrideColor != m_color)
    {
        m_overrideColor = m_color;
        colorChanged = true;
    }

    if (m_overrideAlpha != m_alpha)
    {
        m_overrideAlpha = m_alpha;
        alphaChanged = true;
    }

    if (m_overrideFontFamily != m_fontFamily)
    {
        m_overrideFontFamily = m_fontFamily;
        fontChanged = true;
    }

    if (m_overrideFontEffectIndex != m_fontEffectIndex)
    {
        m_overrideFontEffectIndex = m_fontEffectIndex;
        fontChanged = true;
    }

    m_isColorOverridden = false;
    m_isAlphaOverridden = false;
    m_isFontFamilyOverridden = false;
    m_isFontEffectOverridden = false;

    if (fontChanged)
    {
        MarkDrawBatchLinesDirty(true);
    }
    else if (colorChanged)
    {
        MarkRenderCacheDirty();
    }
    else if (alphaChanged)
    {
        if (m_drawBatchLines.m_fontEffectHasTransparency)
        {
            MarkRenderCacheDirty();
        }
        else
        {
            // alpha changed but there is no transparency in font effect so we need RenderGraph to be rebuilt but not render cache
            MarkRenderGraphDirty();
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTextComponent::SetOverrideColor(const AZ::Color& color)
{
    m_overrideColor.Set(color.GetAsVector3());

    m_isColorOverridden = true;
    MarkRenderCacheDirty();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTextComponent::SetOverrideAlpha(float alpha)
{
    float oldOverrideAlpha = m_overrideAlpha;
    m_overrideAlpha = alpha;
    m_isAlphaOverridden = true;

    if (m_overrideAlpha != oldOverrideAlpha)
    {
        if (m_drawBatchLines.m_fontEffectHasTransparency)
        {
            MarkRenderCacheDirty();
        }
        else
        {
            // alpha changed but there is no transparency in font effect so we need RenderGraph to be rebuilt but not render cache
            MarkRenderGraphDirty();
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTextComponent::SetOverrideFont(FontFamilyPtr fontFamily)
{
    m_overrideFontFamily = fontFamily;

    m_isFontFamilyOverridden = true;

    MarkDrawBatchLinesDirty(true);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTextComponent::SetOverrideFontEffect(unsigned int fontEffectIndex)
{
    m_overrideFontEffectIndex = fontEffectIndex;

    m_isFontEffectOverridden = true;

    MarkDrawBatchLinesDirty(true);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTextComponent::Render(LyShine::IRenderGraph* renderGraph)
{
    // get fade value (tracked by UiRenderer) and compute alpha for text
    float fade = renderGraph->GetAlphaFade();
    float finalAlpha = fade * m_overrideAlpha;
    uint8 finalAlphaByte = static_cast<uint8>(finalAlpha * 255.0f);

    // if we have any cached text batches that have transparency in their font effects then we need to
    // regenerate the render cache if alpha has changed. This is fairly unusual so it still
    // makes sense to not mark the render cache dirty on most fades or alpha changes.
    if (m_drawBatchLines.m_fontEffectHasTransparency)
    {
        if (!m_renderCache.m_batches.empty() && m_renderCache.m_batches[0]->m_color.a != finalAlphaByte)
        {
            MarkRenderCacheDirty();
        }
    }

    // If the cache is out of date then regenerate it
    if (m_renderCache.m_isDirty)
    {
        RenderToCache(finalAlpha);
        m_renderCache.m_isDirty = false;
    }
    else
    {
        // Check font texture version for each cached batch and update batch if out of date.
        // This will happen if the quads for a text string are generated and a required glyph is not in the texture.
        // The font texture is then updated. This means that any existing cached quads could be invalid since one
        // or more glyphs they are using could have been removed from the font texture.
        // The CanvasManager listens for the OnFontTextureUpdated event and will cause all
        // render graphs to be rebuilt when any font texture has changed.
        UpdateTextRenderBatchesForFontTextureChange();
    }

    if (finalAlphaByte == 0)
    {
        // do not render anything if alpha is zero (alpha cannot be overridden by markup)
        // NOTE: this test needs to be done after regenerating the cache. Otherwise m_renderCache.m_isDirty
        // can stay true, which means that the rendergraph doesn't get marked dirty on changes to this
        // component.
        return;
    }

    // these settings are the same for background rect, inline images and text
    bool isTextureSRGB = false;
    bool isTexturePremultipliedAlpha = false;
    LyShine::BlendMode blendMode = LyShine::BlendMode::Normal;

    // if there is a background rect (not typical - used for text selection) then draw it
    // this is not optimized by caching since it is typically only visible on one text component at a time
    if (m_selectionStart != -1)
    {
        UiTransformInterface::RectPointsArray rectPoints;
        GetTextBoundingBoxPrivate(GetDrawBatchLines(), m_selectionStart, m_selectionEnd, rectPoints);

        auto systemImage = AZ::RPI::ImageSystemInterface::Get()->GetSystemImage(AZ::RPI::SystemImage::White);
        bool isClampTextureMode = true;

        uint32 packedColor = (m_textSelectionColor.GetA8() << 24) | (m_textSelectionColor.GetR8() << 16) | (m_textSelectionColor.GetG8() << 8) | m_textSelectionColor.GetB8();

        for (UiTransformInterface::RectPoints& rect : rectPoints)
        {
            DynUiPrimitive* primitive = renderGraph->GetDynamicQuadPrimitive(rect.pt, packedColor);
            primitive->m_next = nullptr;

            LyShine::RenderGraph* lyRenderGraph = static_cast<LyShine::RenderGraph*>(renderGraph); // LYSHINE_ATOM_TODO - find a different solution from downcasting - GHI #3570
            if (lyRenderGraph)
            {
                lyRenderGraph->AddPrimitiveAtom(primitive, systemImage, isClampTextureMode, isTextureSRGB, isTexturePremultipliedAlpha, blendMode);
            }
        }
    }

    // Render the image batches
    if (!m_renderCache.m_imageBatches.empty())
    {
        for (auto batch : m_renderCache.m_imageBatches)
        {
            AZ::Data::Instance<AZ::RPI::Image> texture = batch->m_texture;

            // If the fade value has changed we need to update the alpha values in the vertex colors but we do
            // not want to touch or recompute the RGB values
            if (batch->m_cachedPrimitive.m_vertices[0].color.a != finalAlphaByte)
            {
                for (int i = 0; i < 4; ++i)
                {
                    batch->m_cachedPrimitive.m_vertices[i].color.a = finalAlphaByte;
                }
            }

            bool isClampTextureMode = true;
            LyShine::RenderGraph* lyRenderGraph = static_cast<LyShine::RenderGraph*>(renderGraph); // LYSHINE_ATOM_TODO - find a different solution from downcasting - GHI #3570
            if (lyRenderGraph)
            {
                lyRenderGraph->AddPrimitiveAtom(&batch->m_cachedPrimitive, texture,
                    isClampTextureMode, isTextureSRGB, isTexturePremultipliedAlpha, blendMode);
            }
        }
    }

    // Render the text batches

    STextDrawContext fontContext(m_renderCache.m_fontContext);

    for (RenderCacheBatch* batch : m_renderCache.m_batches)
    {
        AZ::FFont* font = static_cast<AZ::FFont*>(batch->m_font); // LYSHINE_ATOM_TODO - find a different solution from downcasting - GHI #3570
        AZ::Data::Instance<AZ::RPI::Image> fontImage = font->GetFontImage();
        if (fontImage)
        {
            // update alpha values in the verts if alpha has changed (due to fader or SetAlpha).
            // We never do this if any font effect used has transparency since in that case
            // not all of the verts will have the same alpha. We handle that case above
            // by regenerating the render cache in that case.
            if (!m_drawBatchLines.m_fontEffectHasTransparency && batch->m_color.a != finalAlphaByte)
            {
                for (int i=0; i < batch->m_cachedPrimitive.m_numVertices; ++i)
                {
                    batch->m_cachedPrimitive.m_vertices[i].color.a = finalAlphaByte;
                }

                batch->m_color.a = finalAlphaByte;
            }

            // We always use wrap mode for text (isClamp false). This is historically what was done
            // in CryFont and without it characters that are on the left of the font texture look bad
            // because there is no padding on the left of the glyphs.
            bool isClampTextureMode = false;

            LyShine::RenderGraph* lyRenderGraph = static_cast<LyShine::RenderGraph*>(renderGraph); // LYSHINE_ATOM_TODO - find a different solution from downcasting - GHI #3570
            if (lyRenderGraph)
            {
                lyRenderGraph->AddPrimitiveAtom(&batch->m_cachedPrimitive, fontImage,
                    isClampTextureMode, isTextureSRGB, isTexturePremultipliedAlpha, blendMode);
            }
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZStd::string UiTextComponent::GetText()
{
    return m_text;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTextComponent::SetText(const AZStd::string& text)
{
    if (m_text != text)
    {
        m_text = text;

        // This method is used by text input so it has historically always avoided localization
        m_locText = m_text;

        // the text changed so if markup is enabled the XML parsing should report warnings on next parse
        if (m_isMarkupEnabled)
        {
            m_textNeedsXmlValidation = true;
        }

        MarkDrawBatchLinesDirty(true);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZStd::string UiTextComponent::GetTextWithFlags(GetTextFlags flags)
{
    if (flags == UiTextInterface::GetLocalized)
    {
        return m_locText;
    }
    else
    {
        return m_text;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTextComponent::SetTextWithFlags(const AZStd::string& text, SetTextFlags flags)
{
    bool changed = false;

    if (m_text != text)
    {
        m_text = text;
        changed = true;
    }

    AZStd::string locText;
    if ((flags & UiTextInterface::SetLocalized) == UiTextInterface::SetLocalized)
    {
        locText = GetLocalizedText(m_text);
    }
    else
    {
        locText = m_text;
    }

    // a previous call could have had a different value for UiTextInterface::SetLocalized flag but same text
    if (changed || m_locText != locText)
    {
        m_locText = locText;
        changed = true;
    }

    // supported for backward compatibility, now we have the isMarkupEnabled flag the caller could just set that to false
    if ((flags& UiTextInterface::SetEscapeMarkup) == UiTextInterface::SetEscapeMarkup)
    {
        if (m_isMarkupEnabled)
        {
            m_isMarkupEnabled = false;
            changed = true;
        }
    }

    if (changed)
    {
        // The text changed so draw batches will need recalculation
        MarkDrawBatchLinesDirty(true);

        // the text changed so if markup is enabled the XML parsing should report warnings on next parse
        if (m_isMarkupEnabled)
        {
            m_textNeedsXmlValidation = true;
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZ::Color UiTextComponent::GetColor()
{
    return AZ::Color::CreateFromVector3AndFloat(m_color.GetAsVector3(), m_alpha);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTextComponent::SetColor(const AZ::Color& color)
{
    m_color.Set(color.GetAsVector3());
    m_alpha = color.GetA();

    AZ::Color oldOverrideColor = m_overrideColor;
    float oldOverrideAlpha = m_overrideAlpha;

    if (!m_isColorOverridden)
    {
        m_overrideColor = m_color;
    }
    if (!m_isAlphaOverridden)
    {
        m_overrideAlpha = m_alpha;
    }

    // Usually, only a color change requires regenerating render cache.
    // The exception is if we have font effects with separate alpha in which case the
    // m_fontEffectHasTransparency flag is set.
    if (m_overrideColor != oldOverrideColor)
    {
        MarkRenderCacheDirty();
    }
    else if (m_overrideAlpha != oldOverrideAlpha)
    {
        if (m_drawBatchLines.m_fontEffectHasTransparency)
        {
            MarkRenderCacheDirty();
        }
        else
        {
            // alpha changed so we need RenderGraph to be rebuilt but not render cache
            MarkRenderGraphDirty();
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
LyShine::PathnameType UiTextComponent::GetFont()
{
    return m_fontFilename.GetAssetPath().c_str();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTextComponent::SetFont(const LyShine::PathnameType& fontPath)
{
    // the input string could be in any form but must be a game path - not a full path.
    // Make it normalized
    AZStd::string newPath = fontPath;
    EBUS_EVENT(AzFramework::ApplicationRequests::Bus, NormalizePath, newPath);

    if (m_fontFilename.GetAssetPath() != newPath)
    {
        ChangeFont(newPath);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
int UiTextComponent::GetFontEffect()
{
    return m_fontEffectIndex;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTextComponent::SetFontEffect(int effectIndex)
{
    if (m_fontEffectIndex != static_cast<unsigned int>(effectIndex))
    {
        m_fontEffectIndex = effectIndex;

        m_overrideFontEffectIndex = effectIndex;

        MarkDrawBatchLinesDirty(true);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZStd::string UiTextComponent::GetFontEffectName(int effectIndex)
{
    const char* effectName = m_font->GetEffectName(effectIndex);
    return AZStd::string(effectName);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTextComponent::SetFontEffectByName(const AZStd::string& effectName)
{
    unsigned int effectId = m_font->GetEffectId(effectName.c_str());
    SetFontEffect(static_cast<int>(effectId));
}

////////////////////////////////////////////////////////////////////////////////////////////////////
float UiTextComponent::GetFontSize()
{
    return m_fontSize;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTextComponent::SetFontSize(float fontSize)
{
    if (m_fontSize != fontSize)
    {
        m_fontSize = fontSize;
        m_isRequestFontSizeDirty = true;
        m_currFontSize = m_fontSize;

        MarkDrawBatchLinesDirty(true);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTextComponent::GetTextAlignment(IDraw2d::HAlign& horizontalAlignment,
    IDraw2d::VAlign& verticalAlignment)
{
    horizontalAlignment = m_textHAlignment;
    verticalAlignment = m_textVAlignment;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTextComponent::SetTextAlignment(IDraw2d::HAlign horizontalAlignment,
    IDraw2d::VAlign verticalAlignment)
{
    m_textHAlignment = horizontalAlignment;
    m_textVAlignment = verticalAlignment;

    MarkRenderCacheDirty();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
IDraw2d::HAlign UiTextComponent::GetHorizontalTextAlignment()
{
    return m_textHAlignment;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTextComponent::SetHorizontalTextAlignment(IDraw2d::HAlign alignment)
{
    m_textHAlignment = alignment;
    MarkRenderCacheDirty();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
IDraw2d::VAlign UiTextComponent::GetVerticalTextAlignment()
{
    return m_textVAlignment;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTextComponent::SetVerticalTextAlignment(IDraw2d::VAlign alignment)
{
    m_textVAlignment = alignment;
    MarkRenderCacheDirty();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
float UiTextComponent::GetCharacterSpacing()
{
    return m_charSpacing;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTextComponent::SetCharacterSpacing(float characterSpacing)
{
    m_charSpacing = characterSpacing;
    m_currCharSpacing = characterSpacing;

    // Recompute the text since we might have more lines to draw now (for word wrap)
    OnTextWidthPropertyChanged();

    InvalidateLayout();
    MarkRenderCacheDirty();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
float UiTextComponent::GetLineSpacing()
{
    return m_lineSpacing;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTextComponent::SetLineSpacing(float lineSpacing)
{
    m_lineSpacing = lineSpacing;

    InvalidateLayout();
    MarkRenderCacheDirty();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
int UiTextComponent::GetCharIndexFromPoint(AZ::Vector2 point, bool mustBeInBoundingBox)
{
    // get the input point into untransformed canvas space
    AZ::Vector3 point3(point.GetX(), point.GetY(), 0.0f);
    AZ::Matrix4x4 transform;
    EBUS_EVENT_ID(GetEntityId(), UiTransformBus, GetTransformFromViewport, transform);
    point3 = transform * point3;
    AZ::Vector2 pointInCanvasSpace(point3.GetX(), point3.GetY());

    return GetCharIndexFromCanvasSpacePoint(pointInCanvasSpace, mustBeInBoundingBox);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
int UiTextComponent::GetCharIndexFromCanvasSpacePoint(AZ::Vector2 point, bool mustBeInBoundingBox)
{
    // get the bounding rectangle of the text itself in untransformed canvas space
    UiTransformInterface::RectPoints rect;
    GetTextRect(rect);

    // Since the text rect differs from the clipping rect, we have to adjust
    // the user's input by the clipping offset to match the selection with
    // the contents on-screen.
    point.SetX(point.GetX() + CalculateHorizontalClipOffset());

    // first test if the point is in the bounding box
    // point is in rect if it is within rect or exactly on edge
    bool isInRect = false;
    if (point.GetX() >= rect.TopLeft().GetX() &&
        point.GetX() <= rect.BottomRight().GetX() &&
        point.GetY() >= rect.TopLeft().GetY() &&
        point.GetY() <= rect.BottomRight().GetY())
    {
        isInRect = true;
    }

    if (mustBeInBoundingBox && !isInRect)
    {
        return -1;
    }

    // Get point relative to this element's TopLeft() rect. We use this offset
    // to see how far along we've iterated over the rendered string size and
    // whether or not the index has been found.
    AZ::Vector2 pickOffset = AZ::Vector2(point.GetX() - rect.TopLeft().GetX(),
            point.GetY() - rect.TopLeft().GetY());

    int requestFontSize = GetRequestFontSize();
    const DrawBatchLines& drawBatchLines = GetDrawBatchLines();
    STextDrawContext fontContext(GetTextDrawContextPrototype(requestFontSize, drawBatchLines.fontSizeScale));

    int indexIter = 0;
    float lastSubstrX = 0;
    float accumulatedHeight = m_fontSize;
    const bool multiLineText = drawBatchLines.batchLines.size() > 1;
    uint32_t lineCounter = 0;

    // Iterate over each rendered line of text
    for (const DrawBatchLine& batchLine : drawBatchLines.batchLines)
    {
        ++lineCounter;

        // Iterate to the line containing the point
        if (multiLineText && pickOffset.GetY() >= accumulatedHeight)
        {
            // Increment indexIter by number of characters on this line
            for (const DrawBatch& drawBatch : batchLine.drawBatchList)
            {
                indexIter += LyShine::GetUtf8StringLength(drawBatch.text);
            }

            accumulatedHeight += m_fontSize;
            continue;
        }

        // In some cases, we may want the cursor to be displayed on the end
        // of a preceding line, and in others, we may want the cursor to be
        // displaying at the beginning of the following line. We resolve this
        // ambiguity by assigning a "hint" to the offsets calculator on where
        // to place the cursor.
        m_cursorLineNumHint = lineCounter;

        // This index allows us to index relative to the current line of text
        // we're iterating on.
        int curLineIndexIter = 0;

        // Iterate across the line
        for (const DrawBatch& drawBatch : batchLine.drawBatchList)
        {
            Utf8::Unchecked::octet_iterator pChar(drawBatch.text.data());
            while (uint32_t ch = *pChar)
            {
                ++pChar;
                curLineIndexIter += LyShine::GetMultiByteCharSize(ch);

                // Iterate across each character of text until the width
                // exceeds the X pick offset.
                AZStd::string subString(drawBatch.text.substr(0, curLineIndexIter));
                Vec2 sizeSoFar = m_font->GetTextSize(subString.c_str(), true, fontContext);
                float charWidth = sizeSoFar.x - lastSubstrX;

                // pickOffset is a screen-position and the text position changes
                // based on its alignment. We add an offset here to account for
                // the location of the text on-screen for different alignments.
                float alignedOffset = 0.0f;
                if (m_textHAlignment == IDraw2d::HAlign::Center)
                {
                    alignedOffset = 0.5f * (rect.GetAxisAlignedSize().GetX() - batchLine.lineSize.GetX());
                }
                else if (m_textHAlignment == IDraw2d::HAlign::Right)
                {
                    alignedOffset = rect.GetAxisAlignedSize().GetX() - batchLine.lineSize.GetX();
                }

                if (pickOffset.GetX() <= alignedOffset + lastSubstrX + (charWidth * 0.5f))
                {
                    return indexIter;
                }

                lastSubstrX = sizeSoFar.x;
                ++indexIter;
            }
        }

        return indexIter;
    }

    // We can reach here if the point is just on the boundary of the rect.
    // In this case, there are no more lines of text to iterate on, so just
    // assume the user is trying to get to the end of the string.
    return indexIter;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZ::Vector2 UiTextComponent::GetPointFromCharIndex(int index)
{
    // Left and right offsets for determining the position of the beginning
    // and end of the selection.
    LineOffsets top, middle, bottom;

    GetOffsetsFromSelectionInternal(top, middle, bottom, index, index);

    UiTransformInterface::RectPoints rect;
    GetTextRect(rect);

    // LineOffsets values don't take on-screen position with alignment
    // into account, so we adjust the offset here.
    float alignedOffset = 0.0f;
    if (m_textHAlignment == IDraw2d::HAlign::Center)
    {
        alignedOffset = 0.5f * (rect.GetAxisAlignedSize().GetX() - top.batchLineLength);
    }
    else if (m_textHAlignment == IDraw2d::HAlign::Right)
    {
        alignedOffset = rect.GetAxisAlignedSize().GetX() - top.batchLineLength;
    }

    // Calculate left and right rect positions for start and end selection
    rect.TopLeft().SetX(alignedOffset + rect.TopLeft().GetX() + top.left.GetX());

    // Finally, add the y-offset to position the cursor on the correct line
    // of text.
    rect.TopLeft().SetY(rect.TopLeft().GetY() + top.left.GetY());

    return rect.TopLeft();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZ::Color UiTextComponent::GetSelectionColor()
{
    return m_textSelectionColor;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTextComponent::GetSelectionRange(int& startIndex, int& endIndex)
{
    startIndex = m_selectionStart;
    endIndex = m_selectionEnd;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTextComponent::SetSelectionRange(int startIndex, int endIndex, const AZ::Color& textSelectionColor)
{
    m_selectionStart = startIndex;
    m_selectionEnd = endIndex;
    m_textSelectionColor = textSelectionColor;

    // The render cache stores positions based on these values so mark it dirty
    MarkRenderCacheDirty();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTextComponent::ClearSelectionRange()
{
    m_selectionStart = m_selectionEnd = -1;

    // The render cache stores positions based on these values so mark it dirty
    MarkRenderCacheDirty();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZ::Vector2 UiTextComponent::GetTextSize()
{
    // First ensure that the text wrapping is in sync with the element's width.
    // If the element's transform flag is dirty, then the text wrapping does not reflect the current
    // width of the element. Sync it up by checking and handling a change in canvas space size.
    // The notification handler will prepare the text again
    bool canvasSpaceSizeChanged = false;
    EBUS_EVENT_ID_RESULT(canvasSpaceSizeChanged, GetEntityId(), UiTransformBus, HasCanvasSpaceSizeChanged);
    if (canvasSpaceSizeChanged)
    {
        EBUS_EVENT_ID(GetEntityId(), UiTransformBus, NotifyAndResetCanvasSpaceRectChange);
    }

    return GetTextSizeFromDrawBatchLines(GetDrawBatchLines());
}

////////////////////////////////////////////////////////////////////////////////////////////////////
float UiTextComponent::GetTextWidth()
{
    return GetTextSize().GetX();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
float UiTextComponent::GetTextHeight()
{
    return GetTextSize().GetY();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTextComponent::GetTextBoundingBox(int startIndex, int endIndex, UiTransformInterface::RectPointsArray& rectPoints)
{
    // compute the bounding box of the specified area of text
    GetTextBoundingBoxPrivate(GetDrawBatchLines(), startIndex, endIndex, rectPoints);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTextComponent::GetTextBoundingBoxPrivate(const DrawBatchLines& drawBatchLines, int startIndex, int endIndex, UiTransformInterface::RectPointsArray& rectPoints)
{
    // Multi-line selection can be broken up into three pairs of offsets
    // representing the first (top) and last (bottom) lines, and everything
    // in-between (middle).
    LineOffsets top, middle, bottom;

    GetOffsetsFromSelectionInternal(top, middle, bottom, startIndex, endIndex);

    AZStd::stack<LineOffsets*> lineOffsetsStack;
    lineOffsetsStack.push(&bottom);
    lineOffsetsStack.push(&middle);
    lineOffsetsStack.push(&top);

    // Build rectPoints array depending on how many lines of text are selected
    rectPoints.push_back(UiTransformInterface::RectPoints());

    AZ::Vector2 zeroVector = AZ::Vector2::CreateZero();
    if (middle.left != zeroVector || middle.right != zeroVector)
    {
        rectPoints.push_back(UiTransformInterface::RectPoints());
    }

    if (bottom.left != zeroVector || bottom.right != zeroVector)
    {
        rectPoints.push_back(UiTransformInterface::RectPoints());
    }

    // Build RectPoints geometry based on selected lines of text
    for (UiTransformInterface::RectPoints& rect : rectPoints)
    {
        LineOffsets* lineOffsets = lineOffsetsStack.top();
        lineOffsetsStack.pop();
        AZ::Vector2& leftOffset = lineOffsets->left;
        AZ::Vector2& rightOffset = lineOffsets->right;

        // GetTextSize() returns the max width and height that this text component
        // occupies on-screen.
        AZ::Vector2 textSize(GetTextSizeFromDrawBatchLines(drawBatchLines));

        // For a multi-line selection, the width of our selection will span the
        // entire text element width. Otherwise, we need to adjust the text
        // size to only account for the current line width.
        const bool multiLineSection = leftOffset.GetY() < rightOffset.GetY();
        if (!multiLineSection)
        {
            textSize.SetX(lineOffsets->batchLineLength);
        }

        GetTextRect(rect, textSize);

        rect.TopLeft().SetX(rect.TopLeft().GetX() + leftOffset.GetX());
        rect.BottomLeft().SetX(rect.BottomLeft().GetX() + leftOffset.GetX());
        rect.TopRight().SetX(rect.TopLeft().GetX() + rightOffset.GetX());
        rect.BottomRight().SetX(rect.BottomLeft().GetX() + rightOffset.GetX());

        // Finally, add the y-offset to position the cursor on the correct line
        // of text.
        rect.TopLeft().SetY(rect.TopLeft().GetY() + leftOffset.GetY());
        rect.TopRight().SetY(rect.TopRight().GetY() + leftOffset.GetY());
        rightOffset.SetY(rightOffset.GetY() > 0.0f ? rightOffset.GetY() : m_fontSize);
        rect.BottomLeft().SetY(rect.TopLeft().GetY() + rightOffset.GetY());
        rect.BottomRight().SetY(rect.TopRight().GetY() + rightOffset.GetY());

        // Adjust cursor position to account for clipped text
        if (ShouldClip())
        {
            UiTransformInterface::RectPoints elemRect;
            EBUS_EVENT_ID(GetEntityId(), UiTransformBus, GetCanvasSpacePointsNoScaleRotate, elemRect);
            AZ::Vector2 elemSize = elemRect.GetAxisAlignedSize();
            const float displayedTextWidth = GetTextSize().GetX();

            // When we render clipped text, we offset its draw position in order to
            // clip the text properly and keep the visible text within the bounds of
            // the element. Here, we account for that offset in order to render the
            // cursor position at the correct location.
            const bool textOverflowing = displayedTextWidth > elemSize.GetX();
            if (textOverflowing)
            {
                const float rectOffset = CalculateHorizontalClipOffset();
                rect.TopLeft().SetX(rect.TopLeft().GetX() - rectOffset);
                rect.BottomLeft().SetX(rect.BottomLeft().GetX() - rectOffset);
                rect.TopRight().SetX(rect.TopRight().GetX() - rectOffset);
                rect.BottomRight().SetX(rect.BottomRight().GetX() - rectOffset);

                // For clipped selections we can end up with a rect that is too big
                // for the clipped boundaries. Here, we restrict the selection rect
                // size to match the boundaries of the element's size.
                rect.TopLeft().SetX(AZStd::GetMax<float>(elemRect.TopLeft().GetX(), rect.TopLeft().GetX()));
                rect.BottomLeft().SetX(AZStd::GetMax<float>(elemRect.BottomLeft().GetX(), rect.BottomLeft().GetX()));
                rect.TopRight().SetX(AZStd::GetMin<float>(elemRect.TopRight().GetX(), rect.TopRight().GetX()));
                rect.BottomRight().SetX(AZStd::GetMin<float>(elemRect.BottomRight().GetX(), rect.BottomRight().GetX()));
            }
        }

        // now we have the rect in untransformed canvas space, so transform it to viewport space
        EBUS_EVENT_ID(GetEntityId(), UiTransformBus, RotateAndScalePoints, rect);

        // if the start and end indices are the same we want to draw a cursor
        if (startIndex == endIndex)
        {
            // we want to make the rect one pixel wide in transformed space.
            // Get the transform to viewport for the text entity
            AZ::Matrix4x4 transformToViewport;
            EBUS_EVENT_ID(GetEntityId(), UiTransformBus, GetTransformToViewport, transformToViewport);

            // take a sample vector along X-axis and transform it then normalize it
            AZ::Vector3 offset(100.0f, 0.0f, 0.0f);
            AZ::Vector3 transformedOffset3 = transformToViewport.Multiply3x3(offset);
            transformedOffset3.NormalizeSafe();
            AZ::Vector2 transformedOffset(transformedOffset3.GetX(), transformedOffset3.GetY());

            // to help with scaled and rotated text round the offset to nearest pixels
            transformedOffset = Draw2dHelper::RoundXY(transformedOffset, IDraw2d::Rounding::Nearest);

            // before making it exactly one pixel wide, round the left edge to either the nearest pixel or round down
            // (nearest looks best for fonts smaller than 32 and down looks best for fonts larger than 32).
            // Really a better solution would be to draw a textured quad. In the 32 pt proportional font there is
            // usually exactly 2 pixels between characters so by picking one pixel to draw the line on we either make
            // it closer to one character or the other. If we had a text cursor texture we could draw a 4 pixel wide
            // quad and it would look better. A cursor would also look smoother when rotated than a one pixel line.
            // NOTE: The positions of text characters themselves will always be rounded DOWN to a pixel in the
            // font rendering
            IDraw2d::Rounding round = (m_fontSize < 32) ? IDraw2d::Rounding::Nearest : IDraw2d::Rounding::Down;
            rect.TopLeft() = Draw2dHelper::RoundXY(rect.TopLeft(), round);
            rect.BottomLeft() = Draw2dHelper::RoundXY(rect.BottomLeft(), round);

            // now add the unit vector to the two left hand corners of the transformed rect
            // to get the two right hand corners.
            // it will now be one pixel wide in transformed space
            rect.TopRight() = rect.TopLeft() + AZ::Vector2(transformedOffset);
            rect.BottomRight() = rect.BottomLeft() + AZ::Vector2(transformedOffset);
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTextComponent::SetDisplayedTextFunction(const DisplayedTextFunction& displayedTextFunction)
{
    if (displayedTextFunction)
    {
        m_displayedTextFunction = displayedTextFunction;
    }
    else
    {
        // For null function objects, we fall back on our default implementation
        m_displayedTextFunction = DefaultDisplayedTextFunction;
    }
    MarkRenderCacheDirty();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
UiTextInterface::OverflowMode UiTextComponent::GetOverflowMode()
{
    return m_overflowMode;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTextComponent::SetOverflowMode(OverflowMode overflowMode)
{
    if (m_overflowMode != overflowMode)
    {
        m_overflowMode = overflowMode;
        MarkDrawBatchLinesDirty(false);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
UiTextInterface::WrapTextSetting UiTextComponent::GetWrapText()
{
    return m_wrapTextSetting;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTextComponent::SetWrapText(WrapTextSetting wrapSetting)
{
    if (m_wrapTextSetting != wrapSetting)
    {
        m_wrapTextSetting = wrapSetting;
        MarkDrawBatchLinesDirty(false);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
UiTextInterface::ShrinkToFit UiTextComponent::GetShrinkToFit()
{
    return m_shrinkToFit;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTextComponent::SetShrinkToFit(ShrinkToFit shrinkToFit)
{
    if (m_shrinkToFit != shrinkToFit)
    {
        m_shrinkToFit = shrinkToFit;
        MarkDrawBatchLinesDirty(false);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiTextComponent::GetIsMarkupEnabled()
{
    return m_isMarkupEnabled;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTextComponent::SetIsMarkupEnabled(bool isEnabled)
{
    if (m_isMarkupEnabled != isEnabled)
    {
        m_isMarkupEnabled = isEnabled;
        OnMarkupEnabledChange();
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
float UiTextComponent::GetMinimumShrinkScale()
{
    return m_minShrinkScale;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTextComponent::SetMinimumShrinkScale(float minShrinkScale)
{
    // Guard against negative shrink scales
    m_minShrinkScale = AZ::GetMax<float>(0.0f, minShrinkScale);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTextComponent::GetClickableTextRects(UiClickableTextInterface::ClickableTextRects& clickableTextRects)
{
    UiTransformInterface::RectPoints points;
    EBUS_EVENT_ID(GetEntityId(), UiTransformBus, GetCanvasSpacePointsNoScaleRotate, points);
    AZ::Vector2 pos = CalculateAlignedPositionWithYOffset(points);

    const DrawBatchLines& drawBatchLines = GetDrawBatchLines();
    float newlinePosYIncrement = 0.0f;

    for (auto& drawBatchLine : drawBatchLines.batchLines)
    {
        float xDrawPosOffset = 0.0f;
        AZ::Vector2 alignedPosition;
        if (m_textHAlignment == IDraw2d::HAlign::Left && m_textVAlignment == IDraw2d::VAlign::Top)
        {
            alignedPosition = pos;
        }
        else
        {
            alignedPosition = CDraw2d::Align(pos, drawBatchLine.lineSize, m_textHAlignment, IDraw2d::VAlign::Top); // y is already aligned
        }

        alignedPosition.SetY(alignedPosition.GetY() + newlinePosYIncrement);

        for (auto& drawBatch : drawBatchLine.drawBatchList)
        {
            if (drawBatch.GetType() == UiTextComponent::DrawBatch::Type::Text)
            {
                if (ShouldClip())
                {
                    alignedPosition.SetX(alignedPosition.GetX() - CalculateHorizontalClipOffset());
                }

                alignedPosition.SetX(alignedPosition.GetX() + xDrawPosOffset);
                Vec2 textSize(drawBatch.size.GetX(), drawBatch.size.GetY());
                xDrawPosOffset = textSize.x;

                if (drawBatch.IsClickable())
                {
                    UiClickableTextInterface::ClickableTextRect clickableRect;
                    clickableRect.rect.left = alignedPosition.GetX();
                    clickableRect.rect.right = clickableRect.rect.left + drawBatch.size.GetX();
                    clickableRect.rect.top = alignedPosition.GetY();
                    clickableRect.rect.bottom = clickableRect.rect.top + drawBatchLine.lineSize.GetY();
                    clickableRect.action = drawBatch.action;
                    clickableRect.data = drawBatch.data;
                    clickableRect.id = drawBatch.clickableId;
                    clickableTextRects.push_back(clickableRect);
                }
            }
            else if (drawBatch.GetType() == UiTextComponent::DrawBatch::Type::Image)
            {
                xDrawPosOffset = drawBatch.size.GetX();
            }
            else
            {
                AZ_Assert(false, "Unknown DrawBatch Type");
            }
        }
        newlinePosYIncrement += drawBatchLine.lineSize.GetY() + m_lineSpacing;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTextComponent::SetClickableTextColor(int id, const AZ::Color& color)
{
    if (id < 0)
    {
        return;
    }

    bool clickableIdFound = false;
    for (auto& drawBatchLine : m_drawBatchLines.batchLines)
    {
        for (auto& drawBatch : drawBatchLine.drawBatchList)
        {
            if (drawBatch.IsClickable())
            {
                if (id == drawBatch.clickableId)
                {
                    // Don't return here. We purposely continue iterating in
                    // case there are subsequent draw batches (especially
                    // across multiple draw batch lines) with the same ID.
                    // This will occur with word-wrapped text.
                    drawBatch.color = color.GetAsVector3();
                    MarkRenderCacheDirty();
                    clickableIdFound = true;
                }
                else if (clickableIdFound)
                {
                    // However, we can end iteration if we found a matching
                    // ID but we've moved on to non-matching clickable IDs.
                    // Since IDs are unique to a text component, there are no
                    // other batches with the same ID.
                    return;
                }
            }
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTextComponent::PropertyValuesChanged()
{
    if (!m_isColorOverridden)
    {
        m_overrideColor = m_color;
    }
    if (!m_isAlphaOverridden)
    {
        m_overrideAlpha = m_alpha;
    }

    if (!m_isFontFamilyOverridden)
    {
        m_overrideFontFamily = m_fontFamily;
    }

    if (!m_isFontEffectOverridden)
    {
        m_overrideFontEffectIndex = m_fontEffectIndex;
    }

    // If any of the properties that affect line width changed
    if (m_currFontSize != m_fontSize || m_currCharSpacing != m_charSpacing)
    {
        OnTextWidthPropertyChanged();

        m_currFontSize = m_fontSize;
        m_currCharSpacing = m_charSpacing;
    }

    MarkRenderCacheDirty();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTextComponent::OnCanvasSpaceRectChanged([[maybe_unused]] AZ::EntityId entityId, const UiTransformInterface::Rect& oldRect, const UiTransformInterface::Rect& newRect)
{
    // If old rect equals new rect, size changed due to initialization
    const bool sizeChanged = (oldRect == newRect) || (!oldRect.GetSize().IsClose(newRect.GetSize(), 0.05f));

    if (sizeChanged)
    {
        // OnCanvasSpaceRectChanged (with a size change) is called on the first canvas update, any calculation of
        // the draw batches before the first call to OnCanvasSpaceRectChanged may be using the wrong size so we
        // call MarkDrawBatchLinesDirty on the initialization case..
        MarkDrawBatchLinesDirty(false);

        if (m_wrapTextSetting != UiTextInterface::WrapTextSetting::NoWrap)
        {
            // Invalidate the element's layout since element width affects text height (ex. text element has a layout fitter that is set to fit height)
            AZ::EntityId canvasEntityId;
            EBUS_EVENT_ID_RESULT(canvasEntityId, GetEntityId(), UiElementBus, GetCanvasEntityId);
            EBUS_EVENT_ID(canvasEntityId, UiLayoutManagerBus, MarkToRecomputeLayout, GetEntityId());
        }
    }

    // If size did not change, then the position must have changed for this method to be called, so notify listeners that
    // the clickable text rects have changed and invalidate the render cache.
    EBUS_EVENT_ID(GetEntityId(), UiClickableTextNotificationsBus, OnClickableTextChanged);
    MarkRenderCacheDirty();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTextComponent::OnTransformToViewportChanged()
{
    // Request size is correlated with transformation scale, so it must be
    // updated when the scale changes.
    m_isRequestFontSizeDirty = true;

    MarkRenderCacheDirty();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
float UiTextComponent::GetMinWidth()
{
    return 0.0f;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
float UiTextComponent::GetMinHeight()
{
    return 0.0f;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
float UiTextComponent::GetTargetWidth(float maxWidth)
{
    // Calculate draw batch lines based on max width. If unlimited, don't wrap text
    bool forceNoWrap = !LyShine::IsUiLayoutCellSizeSpecified(maxWidth);

    // Trailing space width needs to be included in the total line width so the element width is
    // assigned enough space to include the trailing space. Otherwise, when calculating batch lines
    // for rendering, a new empty line will be added to account for the newline that gets added
    // due to not having enough room for the trailing space
    bool excludeTrailingSpaceWidth = false;

    DrawBatchLines drawBatchLines;
    CalculateDrawBatchLines(drawBatchLines, forceNoWrap, maxWidth, excludeTrailingSpaceWidth);

    // Since we don't know about max height, we can't return an exact target width when overflow
    // handling is enabled because font scaling can change the max width of the draw batch lines.
    // However, the extra width should be minimal

    // Calculate the target width based on the draw batch line sizes
    float textWidth = 0.0f;
    for (const DrawBatchLine& drawBatchLine : drawBatchLines.batchLines)
    {
        textWidth = AZ::GetMax(drawBatchLine.lineSize.GetX(), textWidth);
    }

    if (m_wrapTextSetting != UiTextInterface::WrapTextSetting::NoWrap)
    {
        // In order for the wrapping to remain the same after the resize, the
        // text element width would need to match the string width exactly. To accommodate
        // for slight variation in size, add a small value to ensure that the string will fit
        // inside the text element's bounds. The downside to this is there may be extra space
        // at the bottom, but this is unlikely.
        const float epsilon = 0.01f;
        textWidth += epsilon;
    }

    return textWidth;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
float UiTextComponent::GetTargetHeight(float maxHeight)
{
    // Since target height is calculated after widths are assigned, it can rely on the element's width

    // Check if draw batch lines should be calculated to determine target height, or whether we can
    // use the existing draw batch lines. Overflow mode and shrink to fit mode are based on available height,
    // so we can't rely on current draw batch lines if max height is specified
    const bool haveMaxHeight = LyShine::IsUiLayoutCellSizeSpecified(maxHeight);
    const bool ellipsis = m_overflowMode == OverflowMode::Ellipsis;
    const bool shrinkToFit = m_shrinkToFit == ShrinkToFit::Uniform;

    const bool handleOverflow = haveMaxHeight && (ellipsis || shrinkToFit);
    const bool handleNoOverflow = !haveMaxHeight && (m_drawBatchLines.fontSizeScale.GetY() != 1.0f);

    bool calculateBatchLines = m_areDrawBatchLinesDirty || handleOverflow || handleNoOverflow;
    if (!calculateBatchLines)
    {
        // Check if the element's size has changed, but we haven't received a callback about it yet to mark
        // draw batches dirty (typically done by the Layout Manager after ApplyLayoutWidth and before ApplyLayoutHeight)
        bool canvasSpaceSizeChanged = false;
        EBUS_EVENT_ID_RESULT(canvasSpaceSizeChanged, GetEntityId(), UiTransformBus, HasCanvasSpaceSizeChanged);
        calculateBatchLines = canvasSpaceSizeChanged;
    }

    AZ::Vector2 textSize;
    if (calculateBatchLines)
    {
        // Calculate the draw batch lines
        DrawBatchLines drawBatchLines;
        CalculateDrawBatchLines(drawBatchLines);

        if (handleOverflow)
        {
            // Handle overflow to get an accurate height after the font scale has been determined.
            // The font scale is calculated with fixed increments and may end up being a little smaller
            // than necessary leaving extra height. This step could be eliminated if we can find a more
            // optimal font scale, but that could come with a performance cost.
            // Extra height may also be considered acceptable since the same side effect occurs with
            // fixed height text elements, and there may be extra width as well. However, since we're
            // calculating an optimal height here, we try to be as accurate as possible
            HandleShrinkToFit(drawBatchLines, maxHeight);
            HandleEllipsis(drawBatchLines, maxHeight);
        }

        textSize = GetTextSizeFromDrawBatchLines(drawBatchLines);
    }
    else
    {
        textSize = GetTextSizeFromDrawBatchLines(m_drawBatchLines);
    }

    float textHeight = textSize.GetY();

    if (handleOverflow && m_wrapTextSetting != UiTextInterface::WrapTextSetting::NoWrap)
    {
        // In order for the overflow handling to remain the same after the text element is resized to this
        // new height, the new height must match the height retrieved from GetCanvasSpacePointsNoScaleRotate
        // exactly. However, there is a slight variation in the value that is used to set the element height
        // and the height retrieved from GetCanvasSpacePointsNoScaleRotate. To accommodate for this, add a
        // small value to try and make the overflow handling as close to how it was calculated here as possible
        const float epsilon = 0.01f;
        textHeight += epsilon;
    }

    return textHeight;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
float UiTextComponent::GetExtraWidthRatio()
{
    return 1.0f;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
float UiTextComponent::GetExtraHeightRatio()
{
    return 1.0f;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTextComponent::OnFontsReloaded()
{
    // All old font pointers have been deleted and the old font family pointers have been removed from the CryFont list.
    // New fonts and font family objects have been created and added to the CryFont list.
    // However, the old font family objects are still around because we have a shared pointer to them.
    // Clear the font family shared pointers since they should no longer be used (their fonts have been deleted).
    // When the last one is cleared, the font family's custom deleter will be called and the object will be deleted.
    // This is OK because the custom deleter doesn't do anything if the font family is not in the CryFont's list (which it isn't)
    m_font = nullptr;
    m_fontFamily = nullptr;
    m_overrideFontFamily = nullptr;
    m_isFontFamilyOverridden = false;

    // the font family may have been deleted and reloaded so make sure we update m_fontFamily
    ChangeFont(m_fontFilename.GetAssetPath());

    // It's possible that the font failed to load. If it did, try to load and use the default font but leave the
    // assigned font path the same
    if (!m_fontFamily || !m_font)
    {
        AZStd::string assignedFontFilepath = m_fontFilename.GetAssetPath();
        ChangeFont("");
        m_fontFilename.SetAssetPath(assignedFontFilepath.c_str());
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTextComponent::LanguageChanged()
{
    OnTextChange();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTextComponent::OnCanvasTextPixelAlignmentChange()
{
    MarkDrawBatchLinesDirty(true);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTextComponent::OnAtlasLoaded(const TextureAtlasNamespace::TextureAtlas* atlas)
{
    bool atlasUsageChanged = false;
    for (auto image : m_drawBatchLines.inlineImages)
    {
        if (image->OnAtlasLoaded(atlas))
        {
            atlasUsageChanged = true;
        }
    }
    if (atlasUsageChanged)
    {
        MarkRenderCacheDirty();
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTextComponent::OnAtlasUnloaded(const TextureAtlasNamespace::TextureAtlas* atlas)
{
    bool atlasUsageChanged = false;
    for (auto image : m_drawBatchLines.inlineImages)
    {
        if (image->OnAtlasUnloaded(atlas))
        {
            atlasUsageChanged = true;
        }
    }
    if (atlasUsageChanged)
    {
        MarkRenderCacheDirty();
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// PUBLIC STATIC MEMBER FUNCTIONS
////////////////////////////////////////////////////////////////////////////////////////////////////

void UiTextComponent::Reflect(AZ::ReflectContext* context)
{
    AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);

    if (serializeContext)
    {
        serializeContext->Class<UiTextComponent, AZ::Component>()
            ->Version(9, &VersionConverter)
            ->Field("Text", &UiTextComponent::m_text)
            ->Field("MarkupEnabled", &UiTextComponent::m_isMarkupEnabled)
            ->Field("Color", &UiTextComponent::m_color)
            ->Field("Alpha", &UiTextComponent::m_alpha)
            ->Field("FontFileName", &UiTextComponent::m_fontFilename)
            ->Field("FontSize", &UiTextComponent::m_fontSize)
            ->Field("EffectIndex", &UiTextComponent::m_fontEffectIndex)
            ->Field("TextHAlignment", &UiTextComponent::m_textHAlignment)
            ->Field("TextVAlignment", &UiTextComponent::m_textVAlignment)
            ->Field("CharacterSpacing", &UiTextComponent::m_charSpacing)
            ->Field("LineSpacing", &UiTextComponent::m_lineSpacing)
            ->Field("OverflowMode", &UiTextComponent::m_overflowMode)
           ->Field("WrapTextSetting", &UiTextComponent::m_wrapTextSetting)
            ->Field("ShrinkToFit", &UiTextComponent::m_shrinkToFit)
            ->Field("MinShrinkScale", &UiTextComponent::m_minShrinkScale);

        AZ::EditContext* ec = serializeContext->GetEditContext();
        if (ec)
        {
            auto editInfo = ec->Class<UiTextComponent>("Text", "A visual component that draws a text string");

            editInfo->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                ->Attribute(AZ::Edit::Attributes::Category, "UI")
                ->Attribute(AZ::Edit::Attributes::Icon, "Editor/Icons/Components/UiText.png")
                ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Editor/Icons/Components/Viewport/UiText.png")
                ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("UI", 0x27ff46b0))
                ->Attribute(AZ::Edit::Attributes::AutoExpand, true);

            editInfo->DataElement(0, &UiTextComponent::m_text, "Text", "The text string")
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, &UiTextComponent::OnTextChange)
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, &UiTextComponent::CheckLayoutFitterAndRefreshEditorTransformProperties);
            editInfo->DataElement(AZ::Edit::UIHandlers::CheckBox, &UiTextComponent::m_isMarkupEnabled, "Enable markup", "Enable to support XML markup in the text string")
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, &UiTextComponent::OnMarkupEnabledChange);
            editInfo->DataElement(AZ::Edit::UIHandlers::Color, &UiTextComponent::m_color, "Color", "The color to draw the text string")
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, &UiTextComponent::OnColorChange);
            editInfo->DataElement(AZ::Edit::UIHandlers::Slider, &UiTextComponent::m_alpha, "Alpha", "The transparency of the text string")
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, &UiTextComponent::OnColorChange)
                ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                ->Attribute(AZ::Edit::Attributes::Max, 1.0f);
            editInfo->DataElement("SimpleAssetRef", &UiTextComponent::m_fontFilename, "Font path", "The pathname to the font")
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, &UiTextComponent::OnFontPathnameChange)
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, &UiTextComponent::CheckLayoutFitterAndRefreshEditorTransformProperties);
            editInfo->DataElement(AZ::Edit::UIHandlers::ComboBox, &UiTextComponent::m_fontEffectIndex, "Font effect", "The font effect (from font file)")
                ->Attribute("EnumValues", &UiTextComponent::PopulateFontEffectList)
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, &UiTextComponent::OnFontEffectChange)
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, &UiTextComponent::CheckLayoutFitterAndRefreshEditorTransformProperties);
            editInfo->DataElement(AZ::Edit::UIHandlers::SpinBox, &UiTextComponent::m_fontSize, "Font size", "The size of the font in points")
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, &UiTextComponent::OnFontSizeChange)
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, &UiTextComponent::CheckLayoutFitterAndRefreshEditorTransformProperties)
                ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                ->Attribute(AZ::Edit::Attributes::Step, 1.0f);
            editInfo->DataElement(AZ::Edit::UIHandlers::ComboBox, &UiTextComponent::m_textHAlignment, "Horizontal text alignment", "How to align the text within the rect")
                ->EnumAttribute(IDraw2d::HAlign::Left, "Left")
                ->EnumAttribute(IDraw2d::HAlign::Center, "Center")
                ->EnumAttribute(IDraw2d::HAlign::Right, "Right")
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, &UiTextComponent::OnAlignmentChange);
            editInfo->DataElement(AZ::Edit::UIHandlers::ComboBox, &UiTextComponent::m_textVAlignment, "Vertical text alignment", "How to align the text within the rect")
                ->EnumAttribute(IDraw2d::VAlign::Top, "Top")
                ->EnumAttribute(IDraw2d::VAlign::Center, "Center")
                ->EnumAttribute(IDraw2d::VAlign::Bottom, "Bottom")
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, &UiTextComponent::OnAlignmentChange);
            editInfo->DataElement(0, &UiTextComponent::m_charSpacing, "Character Spacing",
                "The spacing in 1/1000th of ems to add between each two consecutive characters.\n"
                "One em is equal to the currently specified font size.")
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, &UiTextComponent::OnCharSpacingChange)
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, &UiTextComponent::CheckLayoutFitterAndRefreshEditorTransformProperties);
            editInfo->DataElement(0, &UiTextComponent::m_lineSpacing, "Line Spacing", "The amount of pixels to add between each two consecutive lines.")
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, &UiTextComponent::OnLineSpacingChange)
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, &UiTextComponent::CheckLayoutFitterAndRefreshEditorTransformProperties);
            editInfo->DataElement(AZ::Edit::UIHandlers::ComboBox, &UiTextComponent::m_overflowMode, "Overflow mode", "How text should fit within the element")
                ->EnumAttribute(OverflowMode::OverflowText, "Overflow")
                ->EnumAttribute(OverflowMode::ClipText, "Clip text")
                ->EnumAttribute(OverflowMode::Ellipsis, "Ellipsis")
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, &UiTextComponent::OnOverflowChange);
            editInfo->DataElement(AZ::Edit::UIHandlers::ComboBox, &UiTextComponent::m_wrapTextSetting, "Wrap text", "Determines whether text is wrapped")
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, &UiTextComponent::OnWrapTextSettingChange)
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, &UiTextComponent::CheckLayoutFitterAndRefreshEditorTransformProperties)
                ->EnumAttribute(WrapTextSetting::NoWrap, "No wrap")
                ->EnumAttribute(WrapTextSetting::Wrap, "Wrap text");
            editInfo->DataElement(AZ::Edit::UIHandlers::ComboBox, &UiTextComponent::m_shrinkToFit, "Shrink to Fit", "Shrinks overflowing text to fit element bounds")
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, &UiTextComponent::OnShrinkToFitChange)
                ->EnumAttribute(ShrinkToFit::None, "None")
                ->EnumAttribute(ShrinkToFit::Uniform, "Uniform")
                ->EnumAttribute(ShrinkToFit::WidthOnly, "Width Only");
            editInfo->DataElement(AZ::Edit::UIHandlers::SpinBox, &UiTextComponent::m_minShrinkScale, "Minimum Shrink Scale", "Smallest scale that can be applied when 'Shrink to Fit' is specified")
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, &UiTextComponent::OnMinShrinkScaleChange)
                ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                ->Attribute(AZ::Edit::Attributes::Max, 1.0f);
        }
    }

    AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context);
    if (behaviorContext)
    {
        behaviorContext->EBus<UiTextBus>("UiTextBus")
            ->Event("GetText", &UiTextBus::Events::GetText)
            ->Event("SetText", &UiTextBus::Events::SetText)
            ->Event("GetColor", &UiTextBus::Events::GetColor)
            ->Event("SetColor", &UiTextBus::Events::SetColor)
            ->Event("GetFont", &UiTextBus::Events::GetFont)
            ->Event("SetFont", &UiTextBus::Events::SetFont)
            ->Event("GetFontEffect", &UiTextBus::Events::GetFontEffect)
            ->Event("SetFontEffect", &UiTextBus::Events::SetFontEffect)
            ->Event("GetFontEffectName", &UiTextBus::Events::GetFontEffectName)
            ->Event("SetFontEffectByName", &UiTextBus::Events::SetFontEffectByName)
            ->Event("GetFontSize", &UiTextBus::Events::GetFontSize)
            ->Event("SetFontSize", &UiTextBus::Events::SetFontSize)
            ->Event("GetHorizontalTextAlignment", &UiTextBus::Events::GetHorizontalTextAlignment)
            ->Event("SetHorizontalTextAlignment", &UiTextBus::Events::SetHorizontalTextAlignment)
            ->Event("GetVerticalTextAlignment", &UiTextBus::Events::GetVerticalTextAlignment)
            ->Event("SetVerticalTextAlignment", &UiTextBus::Events::SetVerticalTextAlignment)
            ->Event("GetCharacterSpacing", &UiTextBus::Events::GetCharacterSpacing)
            ->Event("SetCharacterSpacing", &UiTextBus::Events::SetCharacterSpacing)
            ->Event("GetLineSpacing", &UiTextBus::Events::GetLineSpacing)
            ->Event("SetLineSpacing", &UiTextBus::Events::SetLineSpacing)
            ->Event("GetOverflowMode", &UiTextBus::Events::GetOverflowMode)
            ->Event("SetOverflowMode", &UiTextBus::Events::SetOverflowMode)
            ->Event("GetWrapText", &UiTextBus::Events::GetWrapText)
            ->Event("SetWrapText", &UiTextBus::Events::SetWrapText)
            ->Event("GetShrinkToFit", &UiTextBus::Events::GetShrinkToFit)
            ->Event("SetShrinkToFit", &UiTextBus::Events::SetShrinkToFit)
            ->Event("GetIsMarkupEnabled", &UiTextBus::Events::GetIsMarkupEnabled)
            ->Event("SetIsMarkupEnabled", &UiTextBus::Events::SetIsMarkupEnabled)
            ->Event("GetTextWidth", &UiTextBus::Events::GetTextWidth)
            ->Event("GetTextHeight", &UiTextBus::Events::GetTextHeight)
            ->Event("GetTextSize", &UiTextBus::Events::GetTextSize)
            ->VirtualProperty("FontSize", "GetFontSize", "SetFontSize")
            ->VirtualProperty("Color", "GetColor", "SetColor")
            ->VirtualProperty("CharacterSpacing", "GetCharacterSpacing", "SetCharacterSpacing")
            ->VirtualProperty("LineSpacing", "GetLineSpacing", "SetLineSpacing");

        behaviorContext->Class<UiTextComponent>()->RequestBus("UiTextBus");

        behaviorContext->EBus<UiClickableTextBus>("UiClickableTextBus")
            ->Event("SetClickableTextColor", &UiClickableTextBus::Events::SetClickableTextColor);

        behaviorContext
            ->Enum<(int)UiTextInterface::OverflowMode::OverflowText>("eUiTextOverflowMode_OverflowText")
            ->Enum<(int)UiTextInterface::OverflowMode::ClipText>("eUiTextOverflowMode_ClipText")
            ->Enum<(int)UiTextInterface::OverflowMode::Ellipsis>("eUiTextOverflowMode_Ellipsis")

            ->Enum<(int)UiTextInterface::WrapTextSetting::NoWrap>("eUiTextWrapTextSetting_NoWrap")
            ->Enum<(int)UiTextInterface::WrapTextSetting::Wrap>("eUiTextWrapTextSetting_Wrap")

            ->Enum<(int)UiTextInterface::ShrinkToFit::None>("eUiTextShrinkToFit_None")
            ->Enum<(int)UiTextInterface::ShrinkToFit::Uniform>("eUiTextShrinkToFit_Uniform")
            ->Enum<(int)UiTextInterface::ShrinkToFit::WidthOnly>("eUiTextShrinkToFit_WidthOnly");
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// PROTECTED MEMBER FUNCTIONS
////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTextComponent::Init()
{
    m_overrideColor = m_color;
    m_overrideAlpha = m_alpha;
    m_overrideFontFamily = m_fontFamily;
    m_overrideFontEffectIndex = m_fontEffectIndex;
    m_requestFontSize = static_cast<int>(m_fontSize);

    // If this is called from RC.exe for example these pointers will not be set. In that case
    // we only need to be able to load, init and save the component. It will never be
    // activated.
    if (!gEnv || !gEnv->pCryFont || !gEnv->pSystem)
    {
        return;
    }

    // if the font is not the one specified by the path (e.g. after loading using serialization)
    if (gEnv->pCryFont->GetFontFamily(m_fontFilename.GetAssetPath().c_str()) != m_fontFamily)
    {
        ChangeFont(m_fontFilename.GetAssetPath());
    }

    // all saved UiTextComponents are assumed to want to try localization of the text string
    m_locText = GetLocalizedText(m_text);

    MarkDrawBatchLinesDirty(false);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTextComponent::Activate()
{
    UiVisualBus::Handler::BusConnect(GetEntityId());
    UiRenderBus::Handler::BusConnect(GetEntityId());
    UiTextBus::Handler::BusConnect(GetEntityId());
    UiClickableTextBus::Handler::BusConnect(GetEntityId());
    UiAnimateEntityBus::Handler::BusConnect(GetEntityId());
    UiTransformChangeNotificationBus::Handler::BusConnect(GetEntityId());
    UiLayoutCellDefaultBus::Handler::BusConnect(GetEntityId());
    FontNotificationBus::Handler::BusConnect();
    LanguageChangeNotificationBus::Handler::BusConnect();

    // When we are activated the transform could have changed so we will always need to recompute the
    // draw batch lines before they are used. Also, we pass true to invalidate the layout,
    // if this is the first time the entity has been activated this has no effect since the canvas
    // is not known. But if a Text component has just been added onto an existing entity
    // we need to invalidate the layout in case that affects things when there is a parent layout
    // component.
    MarkDrawBatchLinesDirty(true);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTextComponent::Deactivate()
{
    UiVisualBus::Handler::BusDisconnect();
    UiRenderBus::Handler::BusDisconnect();
    UiTextBus::Handler::BusDisconnect();
    UiClickableTextBus::Handler::BusDisconnect();
    UiAnimateEntityBus::Handler::BusDisconnect();
    UiTransformChangeNotificationBus::Handler::BusDisconnect();
    UiLayoutCellDefaultBus::Handler::BusDisconnect();
    FontNotificationBus::Handler::BusDisconnect();
    LanguageChangeNotificationBus::Handler::BusDisconnect();

    if (UiCanvasPixelAlignmentNotificationBus::Handler::BusIsConnected())
    {
        UiCanvasPixelAlignmentNotificationBus::Handler::BusDisconnect();
    }

    TextureAtlasNamespace::TextureAtlasNotificationBus::Handler::BusDisconnect();

    // We could be about to remove this component and then reactivate the entity
    // which could affect the layout if there is a parent layout component
    InvalidateLayout();

    // reduce memory use when deactivated
    ClearRenderCache();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTextComponent::ChangeFont(const AZStd::string& fontFileName)
{
    AZStd::string fileName = fontFileName;
    if (fileName.empty())
    {
        fileName = "default-ui";
    }

    FontFamilyPtr fontFamily = gEnv->pCryFont->GetFontFamily(fileName.c_str());
    if (!fontFamily)
    {
        fontFamily = gEnv->pCryFont->LoadFontFamily(fileName.c_str());
    }

    if (fontFamily)
    {
        m_fontFamily = fontFamily;
        m_font = m_fontFamily->normal;

        // we know that the input path is a root relative and normalized pathname
        m_fontFilename.SetAssetPath(fileName.c_str());

        // the font has changed so check that the font effect is valid
        unsigned int numEffects = m_font->GetNumEffects();
        if (m_fontEffectIndex >= numEffects)
        {
            m_fontEffectIndex = 0;
            AZ_Warning("UiTextComponent", false, "Font effect index is out of range for changed font, resetting index to 0");
        }

        if (!m_isFontFamilyOverridden)
        {
            m_overrideFontFamily = m_fontFamily;

            if (m_overrideFontEffectIndex >= numEffects)
            {
                m_overrideFontEffectIndex = m_fontEffectIndex;
            }
        }

        // When the font changes, we need to rebuild our draw batches
        MarkDrawBatchLinesDirty(true);
    }
    else
    {
        AZ_Warning("UiTextComponent", false, "Failed to find font family referenced in markup (ChangeFont): %s", fileName.c_str());
    }

    MarkRenderCacheDirty();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTextComponent::GetTextRect(UiTransformInterface::RectPoints& rect)
{
    GetTextRect(rect, GetTextSize());
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTextComponent::GetTextRect(UiTransformInterface::RectPoints& rect, const AZ::Vector2& textSize)
{
    // get the "no scale rotate" element box
    UiTransformInterface::RectPoints elemRect;
    EBUS_EVENT_ID(GetEntityId(), UiTransformBus, GetCanvasSpacePointsNoScaleRotate, elemRect);

    // given the text alignment work out the box of the actual text
    rect = elemRect;
    switch (m_textHAlignment)
    {
    case IDraw2d::HAlign::Left:
        rect.BottomRight().SetX(rect.TopLeft().GetX() + textSize.GetX());
        rect.TopRight().SetX(rect.BottomRight().GetX());
        break;
    case IDraw2d::HAlign::Center:
    {
        float centerX = (rect.TopLeft().GetX() + rect.TopRight().GetX()) * 0.5f;
        float halfWidth = textSize.GetX() * 0.5f;
        rect.BottomLeft().SetX(centerX - halfWidth);
        rect.TopLeft().SetX(rect.BottomLeft().GetX());
        rect.BottomRight().SetX(centerX + halfWidth);
        rect.TopRight().SetX(rect.BottomRight().GetX());
        break;
    }
    case IDraw2d::HAlign::Right:
        rect.BottomLeft().SetX(rect.TopRight().GetX() - textSize.GetX());
        rect.TopLeft().SetX(rect.BottomLeft().GetX());
        break;
    }
    switch (m_textVAlignment)
    {
    case IDraw2d::VAlign::Top:
        rect.BottomLeft().SetY(rect.TopLeft().GetY() + textSize.GetY());
        rect.BottomRight().SetY(rect.BottomLeft().GetY());
        break;
    case IDraw2d::VAlign::Center:
    {
        float centerY = (rect.TopLeft().GetY() + rect.BottomLeft().GetY()) * 0.5f;
        float halfHeight = textSize.GetY() * 0.5f;
        rect.TopLeft().SetY(centerY - halfHeight);
        rect.TopRight().SetY(rect.TopLeft().GetY());
        rect.BottomLeft().SetY(centerY + halfHeight);
        rect.BottomRight().SetY(rect.BottomLeft().GetY());
        break;
    }
    case IDraw2d::VAlign::Bottom:
        rect.TopLeft().SetY(rect.BottomLeft().GetY() - textSize.GetY());
        rect.TopRight().SetY(rect.TopLeft().GetY());
        break;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTextComponent::OnTextChange()
{
    // When text is changed in the editor we always try to localize it
    m_locText = GetLocalizedText(m_text);

    MarkDrawBatchLinesDirty(true);

    // the text changed so if markup is enabled the XML parsing should report warnings on next parse
    if (m_isMarkupEnabled)
    {
        m_textNeedsXmlValidation = true;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTextComponent::OnColorChange()
{
    m_overrideColor = m_color;
    m_overrideAlpha = m_alpha;
    MarkRenderCacheDirty();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTextComponent::OnAlignmentChange()
{
    MarkRenderCacheDirty();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTextComponent::OnOverflowChange()
{
    // Overflow modes like ellipsis actually change the contents of the draw batches,
    // so they need to be re-generated when the overflow setting changes.
    MarkDrawBatchLinesDirty(true);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTextComponent::OnFontSizeChange()
{
    m_isRequestFontSizeDirty = true;

    // We need to re-prepare the text for rendering, however this may not be
    // very efficient since completely re-preparing the text (parsing markup,
    // preparing batches, etc.) may not be necessary.
    MarkDrawBatchLinesDirty(true);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZ::u32 UiTextComponent::OnFontPathnameChange()
{
    // we should be guaranteed that the asset path in the simple asset ref is root relative and
    // normalized. But just to be safe we make sure is normalized
    AZStd::string fontPath = m_fontFilename.GetAssetPath();
    EBUS_EVENT(AzFramework::ApplicationRequests::Bus, NormalizePath, fontPath);
    m_fontFilename.SetAssetPath(fontPath.c_str());

    // if the font we have loaded has a different pathname to the one we want then change
    // the font (Release the old one and Load or AddRef the new one)
    if (gEnv->pCryFont->GetFontFamily(fontPath.c_str()) != m_fontFamily)
    {
        ChangeFont(m_fontFilename.GetAssetPath());
    }

    return AZ::Edit::PropertyRefreshLevels::AttributesAndValues;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTextComponent::OnFontEffectChange()
{
    m_overrideFontEffectIndex = m_fontEffectIndex;
    MarkDrawBatchLinesDirty(true);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTextComponent::OnWrapTextSettingChange()
{
    MarkDrawBatchLinesDirty(true);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTextComponent::OnShrinkToFitChange()
{
    // Batches need to be re-configured since shrink-to-fit affects
    // sizing information.
    MarkDrawBatchLinesDirty(true);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTextComponent::OnMinShrinkScaleChange()
{
    // Batches need to be re-configured since shrink-to-fit affects
    // sizing information.
    MarkDrawBatchLinesDirty(true);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTextComponent::OnCharSpacingChange()
{
    InvalidateLayout();

    OnTextWidthPropertyChanged();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTextComponent::OnLineSpacingChange()
{
    // If shrink-to-fit applies, we need to re-create draw batch lines in
    // order to ensure overflow conditions are properly applied.
    if (m_shrinkToFit != ShrinkToFit::None)
    {
        MarkDrawBatchLinesDirty(true);
    }
    else
    {
        InvalidateLayout();
        MarkRenderCacheDirty();
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTextComponent::OnMarkupEnabledChange()
{
    MarkDrawBatchLinesDirty(true);
    if (m_isMarkupEnabled)
    {
        m_textNeedsXmlValidation = true;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
UiTextComponent::FontEffectComboBoxVec UiTextComponent::PopulateFontEffectList()
{
    FontEffectComboBoxVec result;
    AZStd::vector<AZ::EntityId> entityIdList;

    if (m_font)
    {
        unsigned int numEffects = m_font->GetNumEffects();
        for (unsigned int i = 0; i < numEffects; ++i)
        {
            const char* name = m_font->GetEffectName(i);
            result.push_back(AZStd::make_pair(i, name));
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
float UiTextComponent::CalculateHorizontalClipOffset()
{
    const bool cursorIsValid = m_selectionStart >= 0;

    if (ShouldClip() && m_wrapTextSetting != WrapTextSetting::Wrap && cursorIsValid)
    {
        UiTransformInterface::RectPoints points;
        EBUS_EVENT_ID(GetEntityId(), UiTransformBus, GetCanvasSpacePointsNoScaleRotate, points);

        int requestFontSize = GetRequestFontSize();
        const DrawBatchLines& drawBatchLines = GetDrawBatchLines();
        STextDrawContext fontContext(GetTextDrawContextPrototype(requestFontSize, drawBatchLines.fontSizeScale));

        const AZStd::string displayedText(m_displayedTextFunction(m_locText));
        const AZ::Vector2 displayedTextSize(GetTextSize());
        const AZ::Vector2 elemSize(points.GetAxisAlignedSize());
        const bool textOverflowing = displayedTextSize.GetX() > elemSize.GetX();

        if (textOverflowing)
        {
            // Get size of text from beginning of the string to the end of
            // the text selection. This forms the basis of the assumptions
            // for the left and center-justified text cases, specifically for
            // calculating the following boolean variables for each case:
            // - cursorAtFirstChar
            // - cursorClippedRight
            // - cursorClippedLeft
            int bytesToSelectionEnd = LyShine::GetByteLengthOfUtf8Chars(displayedText.c_str(), m_selectionEnd);
            AZStd::string leftString(displayedText.substr(0, bytesToSelectionEnd));
            Vec2 leftSize(m_font->GetTextSize(leftString.c_str(), true, fontContext));

            if (m_textHAlignment == IDraw2d::HAlign::Left)
            {
                // Positive clip offset will scroll text left
                m_clipOffsetMultiplier = 1.0f;

                // Positive clip offsets scroll the text left, and negative
                // scrolls the text right. Zero is the minimum for left-
                // aligned since there is no text to scroll to before the first
                // character in the string.
                const float clipOffsetMin = 0.0f;

                // Width of the clipping area to the left of the visible text
                const float clipOffsetLeft = m_clipOffset;

                // We calculate the clip offset differently based on where
                // the cursor position is currently located.
                const bool cursorAtFirstChar = leftSize.x == 0.0f;
                const bool cursorClippedRight = leftSize.x > elemSize.GetX() + clipOffsetLeft;
                const bool cursorClippedLeft = leftSize.x < clipOffsetLeft;

                if (cursorAtFirstChar)
                {
                    m_clipOffset = clipOffsetMin;
                }
                else if (cursorClippedRight)
                {
                    // Scroll the text left to display characters to the
                    // right of the clipping area. The amount scrolled by is
                    // the clipped and non-clipped widths added together and
                    // subtracted from the string size to the left of the cursor.
                    m_clipOffset += leftSize.x - elemSize.GetX() - clipOffsetLeft;
                }
                else if (cursorClippedLeft)
                {
                    // Cursor is clipped to the left, so scroll the text
                    // right by decreasing the clip offset.
                    m_clipOffset = leftSize.x;
                }
            }

            else if (m_textHAlignment == IDraw2d::HAlign::Center)
            {
                // At zero offset, text is displayed centered. Negative
                // values scroll text to the right, so to display the first
                // char in the string, we would need to scroll by half of the
                // total clipped text.
                const float clipOffsetMin = -0.5f * (displayedTextSize.GetX() - elemSize.GetX());

                // Width of the clipped text to the left of the visible text. Adjusted
                // by the min clipping value when the offset becomes negative.
                const float clipOffsetLeft = m_clipOffset >= 0.0f ? m_clipOffset : m_clipOffset - clipOffsetMin;

                const bool cursorAtFirstChar = leftSize.x == 0.0f;
                const bool cursorClippedRight = leftSize.x > elemSize.GetX() + clipOffsetLeft;
                const bool cursorClippedLeft = leftSize.x < clipOffsetLeft;

                if (cursorAtFirstChar)
                {
                    m_clipOffset = clipOffsetMin;
                    m_clipOffsetMultiplier = 1.0f;
                }
                else if (cursorClippedRight)
                {
                    // Similar to left-aligned text, but we adjust our offset
                    // multiplier to account for half of the width already
                    // being accounted for in centered-alignment logic elsewhere.
                    m_clipOffset += leftSize.x - elemSize.GetX() - clipOffsetLeft;
                    m_clipOffsetMultiplier = 0.5f;
                }
                else if (cursorClippedLeft)
                {
                    const float prevClipOffset = m_clipOffset;
                    m_clipOffset = leftSize.x;

                    // Obtain a multiplier that, when multiplied by the new
                    // offset, returns the current offset value, minus the
                    // difference between the current and new offsets (to
                    // account for the clipped space).
                    const float clipOffsetInverse = 1.0f / m_clipOffset;
                    m_clipOffsetMultiplier = clipOffsetInverse * (prevClipOffset * (m_clipOffsetMultiplier - 1) + leftSize.x);
                }
            }

            // Handle right-alignment
            else
            {
                // Get the size of the text following the text selection. This
                // is in contrast to left and center-aligned text, simply
                // because it's more intuitive when dealing with right-
                // aligned text, for the following conditions:
                // - cursorAtFirstChar
                // - cursorClippedRight
                // - cursorClippedLeft
                AZStd::string rightString(displayedText.substr(bytesToSelectionEnd, displayedText.length() - bytesToSelectionEnd));
                Vec2 rightSize(m_font->GetTextSize(rightString.c_str(), true, fontContext));

                // Negative offset will scroll text to the right
                m_clipOffsetMultiplier = -1.0f;

                // Clip offset 0 means the text is text is furthest to the
                // right (for right-justified text).
                const float clipOffsetMin = 0.0f;

                // The difference between the total string size and element
                // size results in the total width that is clipped. When
                // the offset reaches this max value, the text is scrolled
                // furthest to the right (displaying the left-most character
                // in the string).
                const float clipOffsetMax = -1.0f * (displayedTextSize.GetX() - elemSize.GetX());

                // Amout of clipped text to the right of the non-clipped text
                const float clipOffsetRight = m_clipOffset;

                // Amout of clipped text to the left of the non-clipped text
                const float clipOffsetLeft = clipOffsetRight > 0.0f ? fabs(clipOffsetMax) - clipOffsetRight : 0.0f;

                const bool cursorAtFirstChar = rightSize.x == 0.0f;
                const bool cursorClippedRight = leftSize.x > elemSize.GetX() + clipOffsetLeft;
                const bool cursorClippedLeft = rightSize.x > elemSize.GetX() + clipOffsetRight;

                if (cursorAtFirstChar)
                {
                    m_clipOffset = clipOffsetMin;
                }
                else if (cursorClippedRight)
                {
                    // The way the math is setup, if clip offset is zero, we
                    // would subtract from the offset amount each frame.
                    if (m_clipOffset != 0.0f)
                    {
                        m_clipOffset -= leftSize.x - elemSize.GetX() - clipOffsetLeft;
                    }
                }
                else if (cursorClippedLeft)
                {
                    m_clipOffset += rightSize.x - elemSize.GetX() - clipOffsetRight;
                }
            }
        }
        else
        {
            m_clipOffset = 0.0f;
        }
    }

    return m_clipOffset * m_clipOffsetMultiplier;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTextComponent::MarkDrawBatchLinesDirty(bool invalidateLayout)
{
    m_areDrawBatchLinesDirty = true;
    m_drawBatchLines.Clear();

    // Setting this saves Render() from having to check multiple flags.
    MarkRenderCacheDirty();

    if (invalidateLayout)
    {
        InvalidateLayout();
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
const UiTextComponent::DrawBatchLines& UiTextComponent::GetDrawBatchLines()
{
    if (m_areDrawBatchLinesDirty)
    {
        // Reset the font size scale here so that the draw batches will be built at their original
        // (unaltered) size. Otherwise overflow handling could operate based on sizing info
        // that was calculated based on a previous overflow operation.
        m_drawBatchLines.fontSizeScale = AZ::Vector2(1.0f, 1.0f);

        CalculateDrawBatchLines(m_drawBatchLines);

        HandleOverflowText(m_drawBatchLines);

        m_areDrawBatchLinesDirty = false;

        // m_drawBatchLines has changed so render cache is invalid
        MarkRenderCacheDirty();

        EBUS_EVENT_ID(GetEntityId(), UiClickableTextNotificationsBus, OnClickableTextChanged);
    }

    return m_drawBatchLines;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTextComponent::CalculateDrawBatchLines(
    UiTextComponent::DrawBatchLines& drawBatchLinesOut,
    bool forceNoWrap,
    float availableWidth,
    bool excludeTrailingSpaceWidth
    )
{
    bool wrapText = !forceNoWrap && (m_wrapTextSetting == WrapTextSetting::Wrap);
    if (wrapText && availableWidth < 0.0f)
    {
        // Set available width to the width of the text element
        if (UiTransformBus::FindFirstHandler(GetEntityId()))
        {
            // Getting info from the TransformBus could trigger OnCanvasSpaceRectChanged,
            // which would cause this method to be called again. Call this first before
            // we start building our string content! Otherwise drawbatches etc. will end
            // up in a potentially undefined state.
            UiTransformInterface::RectPoints points;
            EBUS_EVENT_ID(GetEntityId(), UiTransformBus, GetCanvasSpacePointsNoScaleRotate, points);
            availableWidth = points.GetAxisAlignedSize().GetX();
        }
        else
        {
            availableWidth = 100.0f; // abritrary width to use in unlikely edge case where there is no active transform component
        }
    }

    // Clear the draw batch lines, but keep the images around until the new ones are created.
    // This is to prevent the same texture from being unloaded and then re-loaded right away
    InlineImageContainer prevInlineImages;
    prevInlineImages = drawBatchLinesOut.inlineImages;
    drawBatchLinesOut.inlineImages.clear();
    drawBatchLinesOut.Clear();

    int requestFontSize = GetRequestFontSize();
    STextDrawContext fontContext(GetTextDrawContextPrototype(requestFontSize, drawBatchLinesOut.fontSizeScale));
    // Set the baseline
    drawBatchLinesOut.baseline = m_font->GetBaseline(fontContext);

    UiTextComponent::DrawBatchContainer drawBatches;
    TextMarkup::Tag markupRoot;

    AZStd::string markupText(m_locText);

    SanitizeUserEnteredNewlineChar(m_locText);

    // Only attempt to parse the string for XML markup if the markup enabled flag is set (it is expensive)
    bool suppressXmlWarnings = !m_textNeedsXmlValidation;
    m_textNeedsXmlValidation = false;
    if (m_isMarkupEnabled && TextMarkup::ParseMarkupBuffer(markupText, markupRoot, suppressXmlWarnings))
    {
        AZStd::stack<UiTextComponent::DrawBatch> batchStack;
        AZStd::stack<FontFamily*> fontFamilyStack;
        fontFamilyStack.push(m_overrideFontFamily.get());

        float fontAscent = m_font->GetAscender(fontContext);

        BuildDrawBatchesAndAssignClickableIds(drawBatches, drawBatchLinesOut.fontFamilyRefs, drawBatchLinesOut.inlineImages, m_fontSize, fontAscent, batchStack, fontFamilyStack, &markupRoot);

        // go over the generated batches to scale empty space and look for font effects with transparency
        IFFont* prevFont = nullptr;
        drawBatchLinesOut.m_fontEffectHasTransparency = false;
        for (auto& drawBatch : drawBatches)
        {
            if (drawBatch.image)
            {
                // Scale empty space (created by horizontal and vertical padding/offset with markup),
                // otherwise element contents will scale unevenly with text.
                drawBatch.image->m_leftPadding *= drawBatchLinesOut.fontSizeScale.GetX();
                drawBatch.image->m_rightPadding *= drawBatchLinesOut.fontSizeScale.GetX();
                drawBatch.image->m_yOffset *= drawBatchLinesOut.fontSizeScale.GetY();

                // For uniform shrink-to-fit, the ascender (defaultImageHeight) gets assigned the
                // scaled Y axis value from the font context, but for width-only shrink-to-fit, we
                // need to apply the scale since the image is only scaled along the X-axis (and
                // not included in the ascender/default image height).
                if (m_shrinkToFit == ShrinkToFit::WidthOnly)
                {
                    drawBatch.image->m_size.SetX(drawBatch.image->m_size.GetX() * drawBatchLinesOut.fontSizeScale.GetX());
                }
            }
            else
            {
                // text batch, check for fonts with transparency in effects
                if (!drawBatchLinesOut.m_fontEffectHasTransparency && drawBatch.font != prevFont)
                {
                    IFFont* font = drawBatch.font;
                    // note that markup can change fonts but not the font effect index, the same
                    // font effect index is used for all batches (we may change this at some point).
                    if (font->DoesEffectHaveTransparency(fontContext.m_fxIdx))
                    {
                        drawBatchLinesOut.m_fontEffectHasTransparency = true;
                    }
                    prevFont = font;
                }
            }
        }
    }
    else
    {
        IFFont* font = m_overrideFontFamily->normal;
        drawBatches.push_back(DrawBatch());
        drawBatches.front().font = font;
        drawBatches.front().text = m_locText;

        // If the font effect we are using has any passes with alpha of less than 1 (not common) then
        // we set a flag in the batch lines since it affects how we can update the alpha in the cache
        drawBatchLinesOut.m_fontEffectHasTransparency = font->DoesEffectHaveTransparency(fontContext.m_fxIdx);
    }

    // Remove old images now. This is to prevent the same images from unloading and then re-loading right away
    for (auto image : prevInlineImages)
    {
        delete image;
    }
    prevInlineImages.clear();

    // Check if we have any inline images that require us to connect to the texture atlas bus
    if (drawBatchLinesOut.inlineImages.size() > 0)
    {
        if (!TextureAtlasNamespace::TextureAtlasNotificationBus::Handler::BusIsConnected())
        {
            TextureAtlasNamespace::TextureAtlasNotificationBus::Handler::BusConnect();
        }
    }
    else
    {
        TextureAtlasNamespace::TextureAtlasNotificationBus::Handler::BusDisconnect();
    }

    // Go through the drawBatchLines and apply the text transform
    for (DrawBatch& drawBatch : drawBatches)
    {
        if (drawBatch.GetType() == UiTextComponent::DrawBatch::Type::Text)
        {
            drawBatch.text = m_displayedTextFunction(drawBatch.text);

            // If the font changed recently, then the font texture is empty, and won't be
            // populated until the frame renders. If the glyphs aren't mapped to the
            // font texture, then their sizes will be reported as zero/missing, which
            // causes issues with alignment.
            gEnv->pCryFont->AddCharsToFontTextures(m_fontFamily, drawBatch.text.c_str(), requestFontSize, requestFontSize);
        }
    }

    if (wrapText)
    {
        if (drawBatchLinesOut.inlineImages.empty())
        {
            BatchAwareWrapText(drawBatchLinesOut, drawBatches, m_fontFamily.get(), fontContext, availableWidth, excludeTrailingSpaceWidth);
        }
        else
        {
            BatchAwareWrapTextWithImages(drawBatchLinesOut, drawBatches, m_fontFamily.get(), fontContext, availableWidth, excludeTrailingSpaceWidth);
        }
    }
    else
    {
        CreateBatchLines(drawBatchLinesOut, drawBatches, m_fontFamily.get());
        AssignLineSizes(drawBatchLinesOut, m_fontFamily.get(), fontContext, excludeTrailingSpaceWidth);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTextComponent::RenderToCache(float alpha)
{
    if (!m_overrideFontFamily)
    {
        return;
    }

    if (!UiCanvasPixelAlignmentNotificationBus::Handler::BusIsConnected())
    {
        AZ::EntityId canvasEntityId;
        EBUS_EVENT_ID_RESULT(canvasEntityId, GetEntityId(), UiElementBus, GetCanvasEntityId);
        UiCanvasPixelAlignmentNotificationBus::Handler::BusConnect(canvasEntityId);
    }

    int requestFontSize = GetRequestFontSize();
    const DrawBatchLines& drawBatchLines = GetDrawBatchLines();
    STextDrawContext fontContext(GetTextDrawContextPrototype(requestFontSize, drawBatchLines.fontSizeScale));
    fontContext.SetOverrideViewProjMatrices(false);

    ColorF color = LyShine::MakeColorF(m_overrideColor.GetAsVector3(), alpha);
    color.srgb2rgb();   // the colors are specified in sRGB but we want linear colors in the shader
    fontContext.SetColor(color);

    // FFont.cpp uses the alpha value of the color to decide whether to use the color, if the alpha value is zero
    // (in a ColorB format) then the color set via SetColor is ignored and it usually ends up drawing with an alpha of 1.
    // This is not what we want. In fact, if the alpha is zero we will not end up rendering anything (due to the early
    // out in UiTextComponent::Render()). So... we set the alpha to any non-zero value so that we do have something in
    // the render cache. This means that if a fader is at zero and then is faded up, we still have something in the
    // cache to modify the alpha values of.
    if (!fontContext.IsColorOverridden())
    {
        color.a = 1.0f;
        fontContext.SetColor(color);
    }

    // Tell the font system how to we are aligning the text
    // The font system uses these alignment flags to force text to be in the safe zone
    // depending on overscan etc
    int flags = 0;
    if (m_textHAlignment == IDraw2d::HAlign::Center)
    {
        flags |= eDrawText_Center;
    }
    else if (m_textHAlignment == IDraw2d::HAlign::Right)
    {
        flags |= eDrawText_Right;
    }

    if (m_textVAlignment == IDraw2d::VAlign::Center)
    {
        flags |= eDrawText_CenterV;
    }
    else if (m_textVAlignment == IDraw2d::VAlign::Bottom)
    {
        flags |= eDrawText_Bottom;
    }

    flags |= eDrawText_UseTransform;
    fontContext.SetFlags(flags);

    AZ::Matrix4x4 transform;
    EBUS_EVENT_ID(GetEntityId(), UiTransformBus, GetTransformToViewport, transform);

    float transFloats[16];
    transform.StoreToRowMajorFloat16(transFloats);
    Matrix34 transform34(transFloats[0], transFloats[1], transFloats[2], transFloats[3],
        transFloats[4], transFloats[5], transFloats[6], transFloats[7],
        transFloats[8], transFloats[9], transFloats[10], transFloats[11]);
    fontContext.SetTransform(transform34);

    // Get the rect that positions the text prior to scale and rotate. The scale and rotate transform
    // will be applied inside the font draw.
    UiTransformInterface::RectPoints points;
    EBUS_EVENT_ID(GetEntityId(), UiTransformBus, GetCanvasSpacePointsNoScaleRotate, points);

    if (ShouldClip())
    {
        fontContext.EnableClipping(true);
        const AZ::Vector2 elemSize(points.GetAxisAlignedSize());

        // Set the clipping rect to be the same size and position of this
        // element's rect.
        fontContext.SetClippingRect(
            points.TopLeft().GetX(),
            points.TopLeft().GetY(),
            elemSize.GetX(),
            elemSize.GetY());
    }

    m_renderCache.m_fontContext = fontContext;
    AZ::Vector2 pos = CalculateAlignedPositionWithYOffset(points);
    RenderDrawBatchLines(drawBatchLines, pos, points, transform, fontContext);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTextComponent::RenderDrawBatchLines(
    const DrawBatchLines& drawBatchLines,
    const AZ::Vector2& pos,
    const UiTransformInterface::RectPoints& points,
    const AZ::Matrix4x4& transformToViewport,
    STextDrawContext& fontContext)
{
    // For each newline-delimited string, we increment the draw call Y pos
    // by the font size
    float newlinePosYIncrement = 0.0f;

    const ColorB origColor(fontContext.m_colorOverride);

    for (const DrawBatchLine& drawBatchLine : drawBatchLines.batchLines)
    {
        float xDrawPosOffset = 0.0f;

        AZ::Vector2 alignedPosition;
        if (m_textHAlignment == IDraw2d::HAlign::Left && m_textVAlignment == IDraw2d::VAlign::Top)
        {
            alignedPosition = pos;
        }
        else
        {
            alignedPosition = CDraw2d::Align(pos, drawBatchLine.lineSize, m_textHAlignment, IDraw2d::VAlign::Top); // y is already aligned
        }

        alignedPosition.SetY(alignedPosition.GetY() + newlinePosYIncrement);

        for (const DrawBatch& drawBatch : drawBatchLine.drawBatchList)
        {
            if (drawBatch.GetType() == UiTextComponent::DrawBatch::Type::Text)
            {
                if (ShouldClip())
                {
                    alignedPosition.SetX(alignedPosition.GetX() - CalculateHorizontalClipOffset());
                }

                alignedPosition.SetX(alignedPosition.GetX() + xDrawPosOffset);

                Vec2 textSize(drawBatch.size.GetX(), drawBatch.size.GetY());
                xDrawPosOffset = textSize.x;

                ColorB batchColor = origColor;
                const bool drawBatchHasColorAssigned = drawBatch.color != TextMarkup::ColorInvalid;
                if (drawBatchHasColorAssigned)
                {
                    ColorF color = LyShine::MakeColorF(drawBatch.color, 1.0f);  // need ColorF to do srgb conversion
                    color.srgb2rgb();   // the colors are specified in markup in sRGB but we want linear colors in the shader
                    batchColor = color;
                }

                fontContext.m_colorOverride = batchColor;

                uint32 numQuads = drawBatch.font->GetNumQuadsForText(drawBatch.text.c_str(), true, fontContext);
                if (numQuads > 0)
                {
                    RenderCacheBatch* cacheBatch = new RenderCacheBatch;
                    cacheBatch->m_position = alignedPosition;
                    cacheBatch->m_position.SetY(cacheBatch->m_position.GetY() + drawBatch.yOffset);
                    cacheBatch->m_text = drawBatch.text;
                    cacheBatch->m_font = drawBatch.font;
                    cacheBatch->m_color = batchColor;

                    cacheBatch->m_cachedPrimitive.m_vertices = new SVF_P2F_C4B_T2F_F4B[numQuads * 4];
                    cacheBatch->m_cachedPrimitive.m_indices = new uint16[numQuads * 6];

                    uint32 numQuadsWritten = cacheBatch->m_font->WriteTextQuadsToBuffers(
                        cacheBatch->m_cachedPrimitive.m_vertices, cacheBatch->m_cachedPrimitive.m_indices, numQuads,
                        cacheBatch->m_position.GetX(), cacheBatch->m_position.GetY(), 1.0f, cacheBatch->m_text.c_str(), true, fontContext);

                    AZ_Assert(numQuadsWritten <= numQuads, "value returned from WriteTextQuadsToBuffers is larger than size allocated");

                    cacheBatch->m_cachedPrimitive.m_numVertices = numQuadsWritten * 4;
                    cacheBatch->m_cachedPrimitive.m_numIndices = numQuadsWritten * 6;

                    cacheBatch->m_fontTextureVersion = drawBatch.font->GetFontTextureVersion();

                    m_renderCache.m_batches.push_back(cacheBatch);
                }
            }
            else if (drawBatch.GetType() == UiTextComponent::DrawBatch::Type::Image)
            {
                alignedPosition.SetX(alignedPosition.GetX() + xDrawPosOffset);
                xDrawPosOffset = drawBatch.size.GetX();

                const AZ::Vector2 imageStartPos = AZ::Vector2(
                    alignedPosition.GetX() + drawBatch.image->m_leftPadding,
                    alignedPosition.GetY() + drawBatch.yOffset);

                const AZ::Vector2 imageEndPos = AZ::Vector2(
                    imageStartPos.GetX() + drawBatch.image->m_size.GetX(),
                    imageStartPos.GetY() + drawBatch.image->m_size.GetY());

                AZ::Vector3 imageQuad[4];
                imageQuad[0] = AZ::Vector3(imageStartPos.GetX(), imageStartPos.GetY(), 1.0f);
                imageQuad[1] = AZ::Vector3(imageEndPos.GetX(), imageQuad[0].GetY(), 1.0f);
                imageQuad[2] = AZ::Vector3(imageQuad[1].GetX(), imageEndPos.GetY(), 1.0f);
                imageQuad[3] = AZ::Vector3(imageQuad[0].GetX(), imageQuad[2].GetY(), 1.0f);

                AZ::Vector2 uvs[4];
                if (drawBatch.image->m_atlas)
                {
                    uvs[0] = AZ::Vector2(static_cast<float>(drawBatch.image->m_coordinates.GetLeft()) / drawBatch.image->m_atlas->GetWidth(),
                        static_cast<float>(drawBatch.image->m_coordinates.GetTop()) / drawBatch.image->m_atlas->GetHeight());
                    uvs[2] = AZ::Vector2(static_cast<float>(drawBatch.image->m_coordinates.GetRight()) / drawBatch.image->m_atlas->GetWidth(),
                        static_cast<float>(drawBatch.image->m_coordinates.GetBottom()) / drawBatch.image->m_atlas->GetHeight());
                    uvs[1] = AZ::Vector2(uvs[2].GetX(), uvs[0].GetY());
                    uvs[3] = AZ::Vector2(uvs[0].GetX(), uvs[2].GetY());
                }
                else
                {
                    uvs[0] = AZ::Vector2(0.0f, 0.0f);
                    uvs[1] = AZ::Vector2(1.0f, 0.0f);
                    uvs[2] = AZ::Vector2(1.0f, 1.0f);
                    uvs[3] = AZ::Vector2(0.0f, 1.0f);
                }

                if (ShouldClip())
                {
                    ClipImageQuadAndUvs(imageQuad, uvs, points, drawBatch, imageStartPos, imageEndPos);
                }

                for (int i = 0; i < 4; ++i)
                {
                    imageQuad[i] = transformToViewport * imageQuad[i];
                }

                static const uint32 packedColor = (255u << 24) | (255u << 16) | (255u << 8) | 255u;

                RenderCacheImageBatch* cacheImageBatch = new RenderCacheImageBatch;

                cacheImageBatch->m_texture = drawBatch.image->m_texture;

                cacheImageBatch->m_cachedPrimitive.m_vertices = new SVF_P2F_C4B_T2F_F4B[4];
                for (int i = 0; i < 4; ++i)
                {
                    cacheImageBatch->m_cachedPrimitive.m_vertices[i].xy = Vec2(imageQuad[i].GetX(), imageQuad[i].GetY());
                    cacheImageBatch->m_cachedPrimitive.m_vertices[i].color.dcolor = packedColor;
                    cacheImageBatch->m_cachedPrimitive.m_vertices[i].st = Vec2(uvs[i].GetX(), uvs[i].GetY());
                    cacheImageBatch->m_cachedPrimitive.m_vertices[i].texIndex = 0;
                    cacheImageBatch->m_cachedPrimitive.m_vertices[i].texHasColorChannel = 1;
                    cacheImageBatch->m_cachedPrimitive.m_vertices[i].texIndex2 = 0;
                    cacheImageBatch->m_cachedPrimitive.m_vertices[i].pad = 0;
                }

                cacheImageBatch->m_cachedPrimitive.m_numVertices = 4;
                static uint16 indices[6] = { 0, 1, 2, 2, 3, 0 };
                cacheImageBatch->m_cachedPrimitive.m_indices = indices;
                cacheImageBatch->m_cachedPrimitive.m_numIndices = 6;

                m_renderCache.m_imageBatches.push_back(cacheImageBatch);
            }
            else
            {
                AZ_Assert(false, "Unknown DrawBatch Type");
            }
        }

        newlinePosYIncrement += drawBatchLine.lineSize.GetY() + m_lineSpacing;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTextComponent::UpdateTextRenderBatchesForFontTextureChange()
{
    STextDrawContext fontContext = m_renderCache.m_fontContext;

    for (RenderCacheBatch* cacheBatch : m_renderCache.m_batches)
    {
        if (cacheBatch->m_fontTextureVersion != cacheBatch->m_font->GetFontTextureVersion())
        {
            uint32 numExistingQuads = cacheBatch->m_cachedPrimitive.m_numVertices / 4;

            fontContext.m_colorOverride = cacheBatch->m_color;

            uint32 numQuads = cacheBatch->m_font->GetNumQuadsForText(cacheBatch->m_text.c_str(), true, fontContext);

            if (numExistingQuads < numQuads)
            {
                delete [] cacheBatch->m_cachedPrimitive.m_vertices;
                delete [] cacheBatch->m_cachedPrimitive.m_indices;

                cacheBatch->m_cachedPrimitive.m_vertices = new SVF_P2F_C4B_T2F_F4B[numQuads * 4];
                cacheBatch->m_cachedPrimitive.m_indices = new uint16[numQuads * 6];
            }

            uint32 numQuadsWritten = cacheBatch->m_font->WriteTextQuadsToBuffers(
                cacheBatch->m_cachedPrimitive.m_vertices, cacheBatch->m_cachedPrimitive.m_indices, numQuads,
                cacheBatch->m_position.GetX(), cacheBatch->m_position.GetY(), 1.0f, cacheBatch->m_text.c_str(), true, fontContext);

            cacheBatch->m_cachedPrimitive.m_numVertices = numQuadsWritten * 4;
            cacheBatch->m_cachedPrimitive.m_numIndices = numQuadsWritten * 6;

            cacheBatch->m_fontTextureVersion = cacheBatch->m_font->GetFontTextureVersion();
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
STextDrawContext UiTextComponent::GetTextDrawContextPrototype(int requestFontSize, const AZ::Vector2& fontSizeScale) const
{
    STextDrawContext ctx;
    ctx.SetEffect(m_overrideFontEffectIndex);
    ctx.SetSizeIn800x600(false);

    // Shrink-to-fit scaling (fontSizeScale) gets applied to font size, but not request size.
    // This means that re-rendered fonts will not re-render characters that are scaled via
    // shrink-to-fit - a scale transformation is applied for these characters instead. For
    // higher quality font scaling with shrink-to-fit, consider taking m_fontSizeScale into
    // account.
    ctx.SetSize(Vec2(m_fontSize * fontSizeScale.GetX(), m_fontSize * fontSizeScale.GetY()));
    ctx.m_requestSize = Vec2i(requestFontSize, requestFontSize);
    ctx.m_processSpecialChars = false;
    ctx.m_tracking = (m_charSpacing * ctx.m_size.x) / 1000.0f; // m_charSpacing units are 1/1000th of ems, 1 em is equal to font size.
                                                               // It's important that we base the character spacing based on the
                                                               // the scaled font size since this is the size the characters will be
                                                               // rendered at. Because spacing is relative to font size, basing the
                                                               // the spacing on the unscaled font size (m_fontSize) would produce
                                                               // visually inaccurate results, such as when shrink-to-fit is being
                                                               // used.

    AZ::EntityId canvasId;
    EBUS_EVENT_ID_RESULT(canvasId, GetEntityId(), UiElementBus, GetCanvasEntityId);
    EBUS_EVENT_ID_RESULT(ctx.m_pixelAligned, canvasId, UiCanvasBus, GetIsTextPixelAligned);

    return ctx;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTextComponent::OnTextWidthPropertyChanged()
{
    if (m_wrapTextSetting == UiTextInterface::WrapTextSetting::NoWrap &&
        m_overflowMode != OverflowMode::Ellipsis &&
        m_shrinkToFit == ShrinkToFit::None &&
        !m_areDrawBatchLinesDirty)
    {
        // Recompute the line sizes so that they are aligned properly (else the sizes will be aligned
        // according to their original width)
        // NOTE:: The AssignLineSizes call modifies the draw batch lines in place so we don't use GetDrawBatchLines here.
        // We only get here if m_drawBatchLines is not dirty.
        int requestFontSize = GetRequestFontSize();
        STextDrawContext fontContext(GetTextDrawContextPrototype(requestFontSize, m_drawBatchLines.fontSizeScale));
        AssignLineSizes(m_drawBatchLines, m_fontFamily.get(), fontContext, true);
        MarkRenderCacheDirty();
    }
    else
    {
        // Recompute even the lines, because since we have new widths, we might have more lines due
        // to word wrap
        MarkDrawBatchLinesDirty(true);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTextComponent::HandleOverflowText(UiTextComponent::DrawBatchLines& drawBatchLinesOut)
{
    HandleShrinkToFit(drawBatchLinesOut);
    HandleEllipsis(drawBatchLinesOut);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTextComponent::HandleShrinkToFit(UiTextComponent::DrawBatchLines& drawBatchLinesOut, float availableHeight)
{
    const bool useShrinkToFit = m_shrinkToFit != ShrinkToFit::None;
    if (!useShrinkToFit)
    {
        return;
    }

    AZ::Vector2 textSize = GetTextSizeFromDrawBatchLines(drawBatchLinesOut);
    AZ::Vector2 currentElementSize;   //  This needs to be computed with the unscaled size. This is because scaling happens after the text is laid out.
    EBUS_EVENT_ID_RESULT(currentElementSize, GetEntityId(), UiTransformBus, GetCanvasSpaceSizeNoScaleRotate);
    if (availableHeight >= 0.0f)
    {
        currentElementSize.SetY(availableHeight);
    }
    const bool textOverflowsElementBounds = GetTextOverflowsBounds(textSize, currentElementSize);
    const bool textOverflowsElementBoundsX = textSize.GetX() > currentElementSize.GetX();
    const bool shrinkToFitNotNeeded = !textOverflowsElementBounds || (!textOverflowsElementBoundsX && m_shrinkToFit == ShrinkToFit::WidthOnly);
    if (shrinkToFitNotNeeded)
    {
        return;
    }

    // Calculate the scaling we need to apply to the font size scale to get
    // the text content to fit within the element. Note that this scale could
    // be limited by m_minShrinkScale.
    AZ::Vector2 scaleXy = AZ::Vector2(
        currentElementSize.GetX() / textSize.GetX(),
        currentElementSize.GetY() / textSize.GetY());

    if (m_shrinkToFit == ShrinkToFit::Uniform)
    {
        const bool textOverflowsElementBoundsY = textSize.GetY() > currentElementSize.GetY();
        const bool noWrap = m_wrapTextSetting == WrapTextSetting::NoWrap;
        const bool notMultiLine = drawBatchLinesOut.batchLines.size() <= 1;
        const bool wrappingNotNeeded = noWrap || notMultiLine;
        if (wrappingNotNeeded)
        {
            HandleUniformShrinkToFitWithScale(drawBatchLinesOut, scaleXy);
        }

        // Some cases produce small (fractional) amounts of overflow along X axis even
        // for word-wrapped cases. Here we check if shrink-to-fit is actually needed by
        // verifying that the text contents overflows the Y-axis bounds of the element.
        else if (textOverflowsElementBoundsY)
        {
            HandleShrinkToFitWithWrapping(drawBatchLinesOut, currentElementSize, textSize);
        }

        // Draw batches need to be re-populated with new font size scale applied
        CalculateDrawBatchLines(drawBatchLinesOut);
    }
    else if (m_shrinkToFit == ShrinkToFit::WidthOnly)
    {
        if (m_wrapTextSetting == WrapTextSetting::NoWrap)
        {
            drawBatchLinesOut.fontSizeScale.SetX(AZ::GetMax<float>(m_minShrinkScale, scaleXy.GetX()));

            // Draw batches need to be re-populated with new font size scale applied
            CalculateDrawBatchLines(drawBatchLinesOut);
        }
        else
        {
            AZ_Assert(m_wrapTextSetting == WrapTextSetting::Wrap, "A new WrapTextSetting other than Wrap has been added. We need to account for that new setting here.");

            HandleShrinkToFitWithWrapping(drawBatchLinesOut, currentElementSize, textSize);
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTextComponent::HandleUniformShrinkToFitWithScale(UiTextComponent::DrawBatchLines& drawBatchLinesOut, const AZ::Vector2& scaleVec)
{
    float minScale = AZ::GetMin<float>(scaleVec.GetX(), scaleVec.GetY());
    minScale = AZ::GetMax<float>(m_minShrinkScale, minScale);
    drawBatchLinesOut.fontSizeScale = AZ::Vector2(minScale, minScale);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTextComponent::HandleShrinkToFitWithWrapping(
    UiTextComponent::DrawBatchLines& drawBatchLinesOut, const AZ::Vector2& currentElementSize, const AZ::Vector2& textSize)
{
    if (m_shrinkToFit == ShrinkToFit::None)
    {
        return;
    }

    const float lineHeight = drawBatchLinesOut.batchLines.front().lineSize.GetY();

    // Sizing sanity checks
    {
        // Sizes less than one pixel are considered invalid
        const float minPixelSize = 1.0f;
        const bool textAndLineHeightsInvalid = lineHeight < minPixelSize || textSize.GetX() < minPixelSize;
        const bool elementSizeInvalid = currentElementSize.IsLessThan(AZ::Vector2::CreateOne());
        const bool invalidSizing = textAndLineHeightsInvalid || elementSizeInvalid;
        if (invalidSizing)
        {
            return;
        }
    }

    int maxLinesElementCanHold = GetNumNonOverflowingLinesForElement(drawBatchLinesOut.batchLines, currentElementSize, m_lineSpacing);

    if (maxLinesElementCanHold <= 0)
    {
        return;
    }

    if (m_shrinkToFit == ShrinkToFit::WidthOnly)
    {
        HandleWidthOnlyShrinkToFitWithWrapping(drawBatchLinesOut, currentElementSize, maxLinesElementCanHold);
    }
    else if (m_shrinkToFit == ShrinkToFit::Uniform)
    {
        HandleUniformShrinkToFitWithWrapping(drawBatchLinesOut, currentElementSize, maxLinesElementCanHold);
    }
    else
    {
        AZ_Assert(false, "Unexpected shrink-to-fit mode given.");
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTextComponent::HandleWidthOnlyShrinkToFitWithWrapping(
    UiTextComponent::DrawBatchLines& drawBatchLinesOut,
    const AZ::Vector2& currentElementSize,
    int maxLinesElementCanHold)
{
    bool textStillOverflows = true;
    while (textStillOverflows)
    {
        // Consider the sizes of all overflowing lines when calculating the
        // scale to reduce the number of times we need to iterate.
        int numOverflowingLines = static_cast<int>(drawBatchLinesOut.batchLines.size() - maxLinesElementCanHold);
        DrawBatchLineContainer::reverse_iterator riter;
        int overflowLineCount = 0;
        float overflowingLineSize = 0.0f;
        for (riter = drawBatchLinesOut.batchLines.rbegin();
            riter != drawBatchLinesOut.batchLines.rend() && overflowLineCount < numOverflowingLines;
            ++riter, ++overflowLineCount)
        {
            DrawBatchLine& batchLine = *riter;
            overflowingLineSize += batchLine.lineSize.GetX();
        }

        // If overflowing line size is empty (zero width), assume its an empty line and give
        // it the width of the element.
        const bool invalidLineSize = overflowingLineSize < 1.0f;
        overflowingLineSize = invalidLineSize ? currentElementSize.GetX() : overflowingLineSize;

        // Determine the total horizontal space the element can accommodate by adding up
        // the width of the total number of lines the element can hold
        const float nonOverflowingWidth = maxLinesElementCanHold * currentElementSize.GetX();

        // Get the scale necessary to fit all of the text within the element
        const float shrinkScale = nonOverflowingWidth / (nonOverflowingWidth + overflowingLineSize);

        // Limit the shrink scale by the minimum shrink scale
        const float newShrinkScale = AZ::GetMax<float>(drawBatchLinesOut.fontSizeScale.GetX() * shrinkScale, m_minShrinkScale);
        drawBatchLinesOut.fontSizeScale.SetX(newShrinkScale);

        // Rebuild the draw batches with the new font size scaling
        CalculateDrawBatchLines(drawBatchLinesOut);

        // Early out if minimum scale was reached or we're at a very small scale
        const bool minScaleThresholdReached = drawBatchLinesOut.fontSizeScale.GetX() < 0.05f;
        const bool useMinShrinkScale = m_minShrinkScale > 0.0f;
        const bool minShrinkScaleReached = drawBatchLinesOut.fontSizeScale.GetX() <= m_minShrinkScale;
        const bool exitDueToSmallScaleApplied = useMinShrinkScale ? minShrinkScaleReached : minScaleThresholdReached;
        if (exitDueToSmallScaleApplied)
        {
            break;
        }

        maxLinesElementCanHold = GetNumNonOverflowingLinesForElement(drawBatchLinesOut.batchLines, currentElementSize, m_lineSpacing);

        // Just because we applied a scale doesn't mean the text fits. This is due to word wrap.
        // Even though we calculate the exact scale to accmmodate all the characters for the
        // max number of lines the element can hold, word-wrap divides the characters unevenly
        // across the total space required by the text, because overflowing words/characters are
        // wrapped to the next line (and a character is "atomic" and can't be divided arbitrarily
        // to accommodate space).
        textStillOverflows = drawBatchLinesOut.batchLines.size() > maxLinesElementCanHold;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTextComponent::HandleUniformShrinkToFitWithWrapping(
    UiTextComponent::DrawBatchLines& drawBatchLinesOut,
    const AZ::Vector2& currentElementSize,
    int maxLinesElementCanHold)
{
    // First, the font scale is reduced by a fractional multiplier until the text no longer overflows.
    // Then, the font scale is incremented by a fixed amount until the largest font scale that
    // does not overflow the text is found

    // Font scale increment value for when the text no longer overflows
    const float fontScaleIncrement = 0.05f;

    float curFontScale = drawBatchLinesOut.fontSizeScale.GetX();

    // This keeps track of the last known largest scale that fits the text
    // to the element bounds with word wrap.
    float bestScaleFoundSoFar = curFontScale;

    // Calculate a default scale multiplier used to reduce the font scale by a percentage
    // until the text no longer overflows.
    // The default scale multiplier is the ratio of available height to the required height.
    // It is made a multiple of fontScaleIncrement so that the final font scale is consistent
    // with the element's height. Otherwise, the font scale could end up getting bigger when
    // the element's size becomes smaller
    const AZ::Vector2 curTextSize = GetTextSizeFromDrawBatchLines(drawBatchLinesOut);
    const float overflowFactor = curTextSize.GetY() > 0.0f ? (currentElementSize.GetY() / curTextSize.GetY()) : 1.0f;
    const float defaultScaleMultiplierUnclamped = floorf(overflowFactor / fontScaleIncrement) * fontScaleIncrement;
    const float defaultScaleMultiplier = AZ::GetClamp<float>(defaultScaleMultiplierUnclamped, fontScaleIncrement, 1.0f - fontScaleIncrement);

    // If min shrink scale applies, and it's bigger than the default scale multplier,
    // we set the scale to be half the difference between 1.0f (no scale) and the
    // min shrink scale (a "half step"). This gives a starting point that avoids
    // applying a scale that is too small too soon (esp for text that "almost fits"
    // the element bounds).
    const float minShrinkScaleHalfStep = (1.0f - m_minShrinkScale) * 0.5f + m_minShrinkScale;
    const bool useMinShrinkScale = m_minShrinkScale > 0.0f;

    const float scaleMultiplierUnclamped = useMinShrinkScale ? minShrinkScaleHalfStep : defaultScaleMultiplier;
    const float scaleMultiplier = AZStd::GetMax<float>(defaultScaleMultiplier, scaleMultiplierUnclamped);

    // Text always starts out overflowing
    bool textStillOverflows = true;

    // When we've reached a font scale that fits the text within the element
    // bounds, we enter an "adjust phase" where the scale gradually increases until
    // the text overflows once more. As the scale increases, we keep track of the
    // last scale to fit the text within bestScaleFoundSoFar.
    bool scaleAdjustPhase = false;

    while (textStillOverflows || scaleAdjustPhase)
    {
        if (textStillOverflows)
        {
            // Decrease current font scale value
            curFontScale *= scaleMultiplier;
        }

        // Rebuild the draw batches with the new font size scaling
        drawBatchLinesOut.fontSizeScale.Set(curFontScale, curFontScale);
        CalculateDrawBatchLines(drawBatchLinesOut);

        maxLinesElementCanHold = GetNumNonOverflowingLinesForElement(drawBatchLinesOut.batchLines, currentElementSize, m_lineSpacing);

        // Just because we applied a scale doesn't mean the text fits. This is due to word wrap.
        // Even though we calculate the exact scale to accmmodate all the characters for the
        // max number of lines the element can hold, word-wrap divides the characters unevenly
        // across the total space required by the text, because overflowing words/characters are
        // wrapped to the next line (and a character is "atomic" and can't be divided arbitrarily
        // to accommodate space).
        textStillOverflows = drawBatchLinesOut.batchLines.size() > maxLinesElementCanHold;

        if (textStillOverflows && !scaleAdjustPhase)
        {
            // Early out if minimum scale was reached or we're at a very small scale
            const bool minScaleThresholdReached = curFontScale < fontScaleIncrement;
            const bool minShrinkScaleReached = curFontScale <= m_minShrinkScale;
            const bool exitDueToSmallScaleApplied = useMinShrinkScale ? minShrinkScaleReached : minScaleThresholdReached;
            if (exitDueToSmallScaleApplied)
            {
                // Set final font scale
                float minFontScale = useMinShrinkScale ? m_minShrinkScale : fontScaleIncrement;
                drawBatchLinesOut.fontSizeScale.Set(minFontScale, minFontScale);
                break;
            }
        }

        // Text is no longer overflowing, begin scaling the text back up until we find
        // a better fit.
        if (!textStillOverflows)
        {
            bestScaleFoundSoFar = curFontScale;
            // Increment current font scale value by a small fixed amount
            curFontScale += fontScaleIncrement;
            scaleAdjustPhase = true;
        }
        // Text is overflowing. If we're in the "adjust phase", assume that the last known
        // scale that fits the text is the best fit and exit the loop.
        else if (scaleAdjustPhase)
        {
            // Make sure final font scale is within min/max
            float minFontScale = useMinShrinkScale ? m_minShrinkScale : fontScaleIncrement;
            bestScaleFoundSoFar = AZ::GetClamp<float>(bestScaleFoundSoFar, minFontScale, 1.0f);

            // Set final font scale
            drawBatchLinesOut.fontSizeScale.Set(bestScaleFoundSoFar, bestScaleFoundSoFar);
            break;
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTextComponent::HandleEllipsis(UiTextComponent::DrawBatchLines& drawBatchLinesOut, float availableHeight)
{
    if (m_overflowMode != OverflowMode::Ellipsis)
    {
        return;
    }

    AZ::Vector2 textSize = GetTextSizeFromDrawBatchLines(drawBatchLinesOut);
    AZ::Vector2 currentElementSize;   //  This needs to be computed with the unscaled size. This is because scaling happens after the text is laid out.
    EBUS_EVENT_ID_RESULT(currentElementSize, GetEntityId(), UiTransformBus, GetCanvasSpaceSizeNoScaleRotate);
    if (availableHeight >= 0.0f)
    {
        currentElementSize.SetY(availableHeight);
    }

    const bool textOverflowsElementBounds = GetTextOverflowsBounds(textSize, currentElementSize);
    const bool textOverflowsElementBoundsX = textSize.GetX() > currentElementSize.GetX();
    const bool onlyOneLine = drawBatchLinesOut.batchLines.size() == 1;
    const bool noEllipsisNeeded = !textOverflowsElementBoundsX && onlyOneLine;

    // No need to handle ellipsis if the text doesn't overflow, OR if the text is ONLY
    // overflowing vertically and there is only one line overflowing (in which case,
    // the content will start to clip). If we don't check for this condition, the
    // ellipsis text will unnecessarily be added to the end of the displayed text.
    if (!textOverflowsElementBounds || noEllipsisNeeded)
    {
        return;
    }

    // Iterate through batch lines until the first overflowing line is found. The
    // line that precedes the overflowing line is the line that will contain the
    // ellipsis. Also gather lines that overflow the element bounds so they can
    // be truncated.
    DrawBatchLineContainer::iterator lineToEllipsis = drawBatchLinesOut.batchLines.begin();
    DrawBatchLineIters linesToRemove;
    GetLineToEllipsisAndLinesToTruncate(drawBatchLinesOut, &lineToEllipsis, linesToRemove, currentElementSize);

    int requestFontSize = GetRequestFontSize();
    STextDrawContext ctx(GetTextDrawContextPrototype(requestFontSize, drawBatchLinesOut.fontSizeScale));

    DrawBatchLine* lineToEllipsisPtr = &(*lineToEllipsis);
    while (lineToEllipsisPtr)
    {
        // We need to know the starting position of each draw batch on this line
        // so that we can apply the ellipsis at the proper position in the text.
        DrawBatchStartPositions startPositions;
        GetDrawBatchStartPositions(startPositions, lineToEllipsisPtr, currentElementSize);
        SetBatchLineFontPointers(lineToEllipsisPtr);

        // Now that we have the line that we need to ellipse (esp in multi-line/word-wrap
        // situations), we need to get the draw batch on the line whose contents need to
        // be modified to include the ellipse.

        const char* ellipseText = "...";
        float drawBatchStartPos = 0.0f;
        float ellipsisPos = 0.0f;

        DrawBatch* drawBatchToEllipse = GetDrawBatchToEllipseAndPositions(ellipseText, ctx, currentElementSize, &startPositions, &drawBatchStartPos, &ellipsisPos);
        TruncateDrawBatches(lineToEllipsisPtr, drawBatchToEllipse);

        // Get the index of the draw batch text to insert the ellipsis text
        int ellipsisCharPos = GetStartEllipseIndexInDrawBatch(drawBatchToEllipse, ctx, drawBatchStartPos, ellipsisPos);
        InsertEllipsisText(ellipseText, ellipsisCharPos, drawBatchToEllipse);

        // Treat the drawbatch as text so ellipsis text renders
        drawBatchToEllipse->image = nullptr;

        // Remove all content if the only content being displayed is ellipsis text
        const bool batchContainsOnlyEllipsis = ellipseText == drawBatchToEllipse->text;
        const bool noOtherBatches = 1 == lineToEllipsisPtr->drawBatchList.size();
        const bool removeBatchContainingOnlyEllipsis = batchContainsOnlyEllipsis && noOtherBatches;
        if (removeBatchContainingOnlyEllipsis)
        {
            linesToRemove.push_back(lineToEllipsis);
        }
        else
        {
            // Otherwise, we found a line that contains content in addition to ellipsis
            break;
        }

        // Once we've reached the first line of text, we're done (since we're iterating backwards)
        if (lineToEllipsis == drawBatchLinesOut.batchLines.begin())
        {
            break;
        }

        // Continue iterating towards the top of text until we find a line that
        // can display the ellipsis
        --lineToEllipsis;
        lineToEllipsisPtr =  &(*lineToEllipsis);
    }

    // For the case when we've removed every possible line, we'll just clip instead
    // of truncate. Otherwise, we need to truncate lines follow ellipsis.
    const bool linesFollowingEllipsisNeedTruncating = drawBatchLinesOut.batchLines.size() > linesToRemove.size();
    if (linesFollowingEllipsisNeedTruncating)
    {
        // Truncate all lines following ellipsis
        for (const auto& drawBatchLineIter : linesToRemove)
        {
            drawBatchLinesOut.batchLines.erase(drawBatchLineIter);
        }
    }
    // Line sizes need to be updated to reflect ellipsis text insertion as well as batch
    // lines being truncated (past the ellipsis line).
    AssignLineSizes(drawBatchLinesOut, m_fontFamily.get(), ctx);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTextComponent::GetLineToEllipsisAndLinesToTruncate(UiTextComponent::DrawBatchLines& drawBatchLinesOut,
    DrawBatchLineContainer::iterator* lineToEllipsis, DrawBatchLineIters& linesToRemove, const AZ::Vector2& currentElementSize)
{
    // Keep track of height of all text as we iterate through the batch lines
    float totalTextHeight = 0.0f;
    DrawBatchLineContainer::iterator prevBatchLine = *lineToEllipsis;
    bool foundLineToEllipsis = false;

    for (auto iter = drawBatchLinesOut.batchLines.begin(); iter != drawBatchLinesOut.batchLines.end(); ++iter)
    {
        DrawBatchLine& batchLine = *iter;
        totalTextHeight += batchLine.lineSize.GetY();
        const bool lineOverflowsVertically = totalTextHeight > currentElementSize.GetY();
        const bool lineOverflowsHorizontally = batchLine.lineSize.GetX() > currentElementSize.GetX();
        const bool lineDoesntOverflow = !lineOverflowsVertically && !lineOverflowsHorizontally;

        if (foundLineToEllipsis)
        {
            // All other lines following the ellipsis are truncated.
            linesToRemove.push_back(iter);
            continue;
        }
        else if (lineDoesntOverflow)
        {
            prevBatchLine = iter;
            continue;
        }

        // Prevent the first line of text from being removed, even if the text
        // is overflowing. With ellipsis enabled, this content will be clipped.
        const bool firstLine = iter == drawBatchLinesOut.batchLines.begin();
        if (lineOverflowsVertically && !firstLine)
        {
            // The line we want to ellipse occurs prior to the
            // first overflowing line.
            *lineToEllipsis = prevBatchLine;
            linesToRemove.push_back(iter);
        }
        else if (lineOverflowsHorizontally)
        {
            // The first line to overflow horizontally gets ellipsis
            *lineToEllipsis = iter;
        }

        foundLineToEllipsis = true;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTextComponent::GetDrawBatchStartPositions(DrawBatchStartPositions& startPositions, DrawBatchLine* lineToEllipsis, [[maybe_unused]] const AZ::Vector2& currentElementSize)
{
    float currentLineSize = 0.0f;

    for (DrawBatch& drawBatch : lineToEllipsis->drawBatchList)
    {
        DrawBatchStartPosPair startPosPair(&drawBatch, currentLineSize);
        startPositions.emplace_back(startPosPair);
        currentLineSize += drawBatch.size.GetX();
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
UiTextComponent::DrawBatch* UiTextComponent::GetDrawBatchToEllipseAndPositions(const char* ellipseText,
    const STextDrawContext& ctx,
    const AZ::Vector2& currentElementSize,
    DrawBatchStartPositions* startPositions,
    float* drawBatchStartPos,
    float* ellipsisPos)
{
    // Iterate backwards through draw batches on this line, until we find a draw batch
    // that can contain the ellipsis text within the bounds of the element.
    auto drawBatchToEllipseIter = startPositions->rbegin();
    DrawBatch* drawBatchToEllipse = (*drawBatchToEllipseIter).first;

    float ellipsisSize = 0.0f;
    while (drawBatchToEllipse)
    {
        const bool prevBatchIsValid = AZStd::next(drawBatchToEllipseIter) != startPositions->rend();
        const bool prevBatchIsImage = prevBatchIsValid && (*AZStd::next(drawBatchToEllipseIter)).first->image != nullptr;
        const bool moreBatchesPriorToImage = prevBatchIsImage && startPositions->size() > 2;
        const bool moreTextBatches = !prevBatchIsImage && startPositions->size() > 1;
        const bool moreDrawBatchesAvailable = moreBatchesPriorToImage || moreTextBatches;

        // The size of the ellipsis text can change based on the font being used in the draw batch
        ellipsisSize = drawBatchToEllipse->font->GetTextSize(ellipseText, true, ctx).x;

        // Calculate where the ellipsis must start in order to be contained within the
        // element bounds. Also, guard against narrow elements that aren't wide enough
        // to accommodate ellipsis.
        *ellipsisPos = AZStd::GetMax<float>(0.0f, currentElementSize.GetX() - ellipsisSize);
        *drawBatchStartPos = startPositions->back().second;

        const bool drawBatchOccursAfterEllipsis = *ellipsisPos <= *drawBatchStartPos;
        const bool getPrevDrawBatch = drawBatchOccursAfterEllipsis && moreDrawBatchesAvailable;

        if (getPrevDrawBatch)
        {
            startPositions->pop_back();
            drawBatchToEllipseIter = startPositions->rbegin();
            drawBatchToEllipse = (*drawBatchToEllipseIter).first;
        }
        else
        {
            // Found a draw batch whose start position can contain the ellipsis
            // within the bounds of the element.
            break;
        }
    }

    return drawBatchToEllipse;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTextComponent::TruncateDrawBatches(UiTextComponent::DrawBatchLine* lineToTruncate, const UiTextComponent::DrawBatch* truncateAfterBatch)
{
    bool truncateBatchFound = false;
    lineToTruncate->drawBatchList.remove_if(
        [&truncateBatchFound, truncateAfterBatch](const UiTextComponent::DrawBatch& drawBatch)
        {
            if (truncateBatchFound)
            {
                return true;
            }
            else
            {
                truncateBatchFound = &drawBatch == truncateAfterBatch;
                return false;
            }
        });
}

////////////////////////////////////////////////////////////////////////////////////////////////////
int UiTextComponent::GetStartEllipseIndexInDrawBatch(const DrawBatch* drawBatchToEllipse,
    const STextDrawContext& ctx,
    const float drawBatchStartPos,
    const float ellipsePos)
{
    float overflowStringSize = 0.0f;
    int ellipsisCharPos = 0;
    Utf8::Unchecked::octet_iterator pChar(drawBatchToEllipse->text.data());
    uint32_t stringBufferIndex = 0;
    uint32_t prevCh = 0;
    while (uint32_t ch = *pChar)
    {
        ++pChar;
        size_t maxSize = 5;
        char codepoint[5] = { 0 };
        char* codepointPtr = codepoint;
        Utf8::Unchecked::octet_iterator<AZStd::string::iterator>::to_utf8_sequence(ch, codepointPtr, maxSize);

        overflowStringSize += drawBatchToEllipse->font->GetTextSize(codepoint, true, ctx).x;

        if (prevCh && ctx.m_kerningEnabled)
        {
            overflowStringSize += drawBatchToEllipse->font->GetKerning(prevCh, ch, ctx).x;
        }

        if (prevCh)
        {
            overflowStringSize += ctx.m_tracking;
        }
        prevCh = ch;

        const float overflowStartPos = drawBatchStartPos + overflowStringSize;
        const bool ellipseCharPosFound = overflowStartPos > ellipsePos;
        stringBufferIndex += LyShine::GetMultiByteCharSize(ch);
        if (ellipseCharPosFound)
        {
            const bool insertEllipsisFollowingFirstChar = ellipsisCharPos == 0;
            ellipsisCharPos = insertEllipsisFollowingFirstChar ? stringBufferIndex : ellipsisCharPos;
            break;
        }

        ellipsisCharPos = stringBufferIndex;

    }

    return ellipsisCharPos;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTextComponent::InsertEllipsisText(const char* ellipseText,
    const int ellipsisCharPos,
    DrawBatch* drawBatchToEllipse)
{
    drawBatchToEllipse->text = drawBatchToEllipse->text.substr(0, ellipsisCharPos) + ellipseText;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTextComponent::SetBatchLineFontPointers(DrawBatchLine* batchLine)
{
    IFFont* ellipsisFont = m_font;

    for (auto drawBatchIter = batchLine->drawBatchList.begin();
        drawBatchIter != batchLine->drawBatchList.end();
        drawBatchIter++)
    {
        DrawBatch* iterBatchPtr = &(*drawBatchIter);

        // Assign the last valid font ptr to this batch (note that batches
        // already containing valid font pointers will simply have that
        // font re-assigned back to them).
        ellipsisFont = iterBatchPtr->font ? iterBatchPtr->font : ellipsisFont;
        iterBatchPtr->font = ellipsisFont;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiTextComponent::GetTextOverflowsBounds(const AZ::Vector2& textSize, const AZ::Vector2& elementSize) const
{
    const bool textOverflowsElementBoundsX = textSize.GetX() > elementSize.GetX();
    const bool textOverflowsElementBoundsY = textSize.GetY() > elementSize.GetY();
    return textOverflowsElementBoundsX || textOverflowsElementBoundsY;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZ::Vector2 UiTextComponent::GetTextSizeFromDrawBatchLines(const UiTextComponent::DrawBatchLines& drawBatchLines) const
{
    AZ::Vector2 size = AZ::Vector2(0.0f, 0.0f);

    for (const DrawBatchLine& drawBatchLine : drawBatchLines.batchLines)
    {
        size.SetX(AZ::GetMax(drawBatchLine.lineSize.GetX(), size.GetX()));

        size.SetY(size.GetY() + drawBatchLine.lineSize.GetY());
    }

    // Add the extra line spacing to the Y size
    if (drawBatchLines.batchLines.size() > 0)
    {
        size.SetY(size.GetY() + (drawBatchLines.batchLines.size() - 1) * m_lineSpacing);
    }

    return size;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZStd::string UiTextComponent::GetLocalizedText([[maybe_unused]] const AZStd::string& text)
{
    AZStd::string locText;
    LocalizationManagerRequestBus::Broadcast(&LocalizationManagerRequestBus::Events::LocalizeString_ch, m_text.c_str(), locText, false);
    return locText.c_str();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZ::Vector2 UiTextComponent::CalculateAlignedPositionWithYOffset(const UiTransformInterface::RectPoints& points)
{
    AZ::Vector2 pos;
    const DrawBatchLines& drawBatchLines = GetDrawBatchLines();
    size_t numLinesOfText = drawBatchLines.batchLines.size();

    switch (m_textHAlignment)
    {
    case IDraw2d::HAlign::Left:
        pos.SetX(points.TopLeft().GetX());
        break;
    case IDraw2d::HAlign::Center:
    {
        float width = points.TopRight().GetX() - points.TopLeft().GetX();
        pos.SetX(points.TopLeft().GetX() + width * 0.5f);
        break;
    }
    case IDraw2d::HAlign::Right:
        pos.SetX(points.TopRight().GetX());
        break;
    }

    switch (m_textVAlignment)
    {
    case IDraw2d::VAlign::Top:
        pos.SetY(points.TopLeft().GetY());
        break;
    case IDraw2d::VAlign::Center:
    {
        float height = points.BottomLeft().GetY() - points.TopLeft().GetY();
        pos.SetY(points.TopLeft().GetY() + height * 0.5f);
        break;
    }
    case IDraw2d::VAlign::Bottom:
        pos.SetY(points.BottomLeft().GetY());
        break;
    }

    // For bottom-aligned text, we need to offset the vertical draw position
    // such that the text never displays below the max Y position
    if (m_textVAlignment == IDraw2d::VAlign::Bottom)
    {
        pos.SetY(pos.GetY() - (drawBatchLines.height + m_lineSpacing * (numLinesOfText - 1)));
    }

    // Centered alignment is obtained by offsetting by half the height of the
    // entire text
    else if (m_textVAlignment == IDraw2d::VAlign::Center)
    {
        pos.SetY(pos.GetY() - ((drawBatchLines.height + m_lineSpacing * (numLinesOfText - 1)) * 0.5f));
    }

    return pos;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// PRIVATE STATIC MEMBER FUNCTIONS
////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiTextComponent::VersionConverter(AZ::SerializeContext& context,
    AZ::SerializeContext::DataElementNode& classElement)
{
    // conversion from version 1: Need to convert Color to Color and Alpha
    // conversion from version 1 or 2: Need to convert Text from CryString to AzString
    AZ_Assert(classElement.GetVersion() > 2, "Unsupported UiTextComponent version: %d", classElement.GetVersion());

    // Versions prior to v4: Change default font
    if (classElement.GetVersion() <= 3)
    {
        if (!ConvertV3FontFileNameIfDefault(context, classElement))
        {
            return false;
        }
    }

    // V4: remove deprecated "supports markup" flag
    if (classElement.GetVersion() == 4)
    {
        if (!RemoveV4MarkupFlag(context, classElement))
        {
            return false;
        }
    }

    // conversion from version 5 to current: Strip off any leading forward slashes from font path
    if (classElement.GetVersion() <= 5)
    {
        if (!LyShine::RemoveLeadingForwardSlashesFromAssetPath(context, classElement, "FontFileName"))
        {
            return false;
        }
    }

    // conversion from version 6 to current: Need to convert ColorF to AZ::Color
    if (classElement.GetVersion() <= 6)
    {
        if (!LyShine::ConvertSubElementFromVector3ToAzColor(context, classElement, "Color"))
        {
            return false;
        }
    }

    // conversion from version 7 to current: The m_isMarkupEnabled flag was added. It defaults to false for new components.
    // But if old data is loaded it should default to true for backward compatibility
    if (classElement.GetVersion() <= 7)
    {
        if (!AddV8EnableMarkupFlag(context, classElement))
        {
            return false;
        }
    }

    // conversion from version 8 to current:
    // - "shrink to fit" wrap text setting now becomes the "uniform" value of the new "shrink to fit" enum
    // - legacy "ResizeToText" overflow mode (enum value 2) gets reset back to zero (overflow)
    if (classElement.GetVersion() <= 8)
    {
        if (!ConvertV8ShrinkToFitSetting(context, classElement))
        {
            return false;
        }

        if (!ConvertV8LegacyOverflowModeSetting(context, classElement))
        {
            return false;
        }
    }

    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTextComponent::GetOffsetsFromSelectionInternal(LineOffsets& top, LineOffsets& middle, LineOffsets& bottom, int selectionStart, int selectionEnd)
{
    const int localLastIndex = AZStd::GetMax<int>(selectionStart, selectionEnd);

    int requestFontSize = GetRequestFontSize();
    const DrawBatchLines& drawBatchLines = GetDrawBatchLines();

    if (!drawBatchLines.inlineImages.empty())
    {
        // CalculateOffsets below does not work for draw batch lines with images in them. Images can never be entered
        // in a text input BUT they can be in the initial starting string entered in the UI Editor.
        // For now we just do not support selection (avoids a crash in CalculateOffsets).
        // Text input in general will not work correctly with any markup in the text and will disable markup as soon
        // as the text string is modified.
        return;
    }

    STextDrawContext fontContext(GetTextDrawContextPrototype(requestFontSize, drawBatchLines.fontSizeScale));

    UiTextComponentOffsetsSelector offsetsSelector(
        drawBatchLines,
        fontContext,
        m_fontSize,
        AZStd::GetMin<int>(selectionStart, selectionEnd),
        localLastIndex,
        GetLineNumberFromCharIndex(drawBatchLines, localLastIndex),
        m_cursorLineNumHint);

    offsetsSelector.CalculateOffsets(top, middle, bottom);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
int UiTextComponent::GetLineNumberFromCharIndex(const DrawBatchLines& drawBatchLines, const int soughtIndex) const
{
    int lineCounter = 0;
    int indexIter = 0;

    // Iterate across the lines of text until soughtIndex is found,
    // incrementing lineCounter along the way and ultimately returning its
    // value when the index is found.
    for (const DrawBatchLine& batchLine : drawBatchLines.batchLines)
    {
        lineCounter++;

        for (const DrawBatch& drawBatch : batchLine.drawBatchList)
        {
            Utf8::Unchecked::octet_iterator pChar(drawBatch.text.data());
            while (uint32_t ch = *pChar)
            {
                ++pChar;
                if (indexIter == soughtIndex)
                {
                    return lineCounter;
                }

                ++indexIter;
            }
        }
    }

    // Note that it's possible for sought index to be one past the end of
    // the line string, in which case we count the soughtIndex as being on
    // that line anyways.
    return lineCounter;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTextComponent::InvalidateLayout() const
{
    // Invalidate the parent's layout
    AZ::EntityId canvasEntityId;
    EBUS_EVENT_ID_RESULT(canvasEntityId, GetEntityId(), UiElementBus, GetCanvasEntityId);
    EBUS_EVENT_ID(canvasEntityId, UiLayoutManagerBus, MarkToRecomputeLayoutsAffectedByLayoutCellChange, GetEntityId(), true);

    // Invalidate the element's layout
    EBUS_EVENT_ID(canvasEntityId, UiLayoutManagerBus, MarkToRecomputeLayout, GetEntityId());
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTextComponent::CheckLayoutFitterAndRefreshEditorTransformProperties() const
{
    UiLayoutHelpers::CheckFitterAndRefreshEditorTransformProperties(GetEntityId());
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTextComponent::MarkRenderCacheDirty()
{
    if (!m_renderCache.m_isDirty)
    {
        ClearRenderCache();
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTextComponent::MarkRenderGraphDirty()
{
    // tell the canvas to invalidate the render graph
    AZ::EntityId canvasEntityId;
    EBUS_EVENT_ID_RESULT(canvasEntityId, GetEntityId(), UiElementBus, GetCanvasEntityId);
    EBUS_EVENT_ID(canvasEntityId, UiCanvasComponentImplementationBus, MarkRenderGraphDirty);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTextComponent::ClearRenderCache()
{
    // at the moment, any change to the render cache requires the graph is cleared because a render node
    // in the graph has a list of primitives, if a primitive is removed it breaks the graph.
    MarkRenderGraphDirty();

    // As mentioned above it is ONLY valid to clear this and delete the image batches when the render graph
    // has been cleared. Otherwise the graph intrusive lists will have pointers to deleted structures.
    FreeRenderCacheMemory();

    m_renderCache.m_isDirty = true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTextComponent::FreeRenderCacheMemory()
{
    for (RenderCacheImageBatch* imageBatch : m_renderCache.m_imageBatches)
    {
        delete [] imageBatch->m_cachedPrimitive.m_vertices;
        delete imageBatch;
    }
    for (RenderCacheBatch* textBatch : m_renderCache.m_batches)
    {
        delete [] textBatch->m_cachedPrimitive.m_vertices;
        delete [] textBatch->m_cachedPrimitive.m_indices;
        delete textBatch;
    }

    m_renderCache.m_batches.clear();
    m_renderCache.m_imageBatches.clear();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiTextComponent::ShouldClip()
{
    return m_overflowMode == OverflowMode::ClipText || m_overflowMode == OverflowMode::Ellipsis;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
int UiTextComponent::GetRequestFontSize()
{
    if (m_isRequestFontSizeDirty)
    {
        m_requestFontSize = CalcRequestFontSize(m_fontSize, GetEntityId());
        m_isRequestFontSizeDirty = false;
    }

    return m_requestFontSize;
}

#include "Tests/internal/test_UiTextComponent.cpp"
