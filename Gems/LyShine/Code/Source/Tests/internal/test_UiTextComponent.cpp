/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

////////////////////////////////////////////////////////////////////////////////////////////////////
#if defined(LYSHINE_INTERNAL_UNIT_TEST)

#include <LyShine/Bus/UiCanvasBus.h>
#include <regex>

namespace
{
    bool IsClose(float value1, float value2, float epsilon = 0.0001f)
    {
        return fabsf(value1 - value2) < epsilon;
    }

    using FontList = AZStd::list < IFFont * >;

    void AssertTextNotEmpty(const AZStd::list<UiTextComponent::DrawBatch>& drawBatches)
    {
        for (const UiTextComponent::DrawBatch& drawBatch : drawBatches)
        {
            AZ_Assert(!drawBatch.text.empty(), "Test failed");
        }
    }

    void AssertDrawBatchFontOrder(
        const AZStd::list<UiTextComponent::DrawBatch>& drawBatches,
        const FontList& fontList)
    {
        AZ_Assert(drawBatches.size() == fontList.size(), "Test failed");

        auto drawBatchIt = drawBatches.begin();
        auto fontsIt = fontList.begin();
        for (;
            drawBatchIt != drawBatches.end();
            ++drawBatchIt, ++fontsIt)
        {
            const UiTextComponent::DrawBatch& drawBatch = *drawBatchIt;
            const IFFont* font = *fontsIt;
            AZ_Assert(drawBatch.font == font, "Test failed");
        }
    }

    void AssertDrawBatchSingleColor(
        const AZStd::list<UiTextComponent::DrawBatch>& drawBatches,
        const AZ::Vector3& color)
    {
        auto drawBatchIt = drawBatches.begin();
        for (;
            drawBatchIt != drawBatches.end();
            ++drawBatchIt)
        {
            const UiTextComponent::DrawBatch& drawBatch = *drawBatchIt;
            AZ_Assert(drawBatch.color == color, "Test failed");
        }
    }

    using ColorList = AZStd::list < AZ::Vector3 >;

    void AssertDrawBatchMultiColor(
        const AZStd::list<UiTextComponent::DrawBatch>& drawBatches,
        const ColorList& colorList)
    {
        auto drawBatchIt = drawBatches.begin();
        auto colorIt = colorList.begin();
        for (;
            drawBatchIt != drawBatches.end();
            ++drawBatchIt, ++colorIt)
        {
            const UiTextComponent::DrawBatch& drawBatch = *drawBatchIt;
            const AZ::Vector3& color(*colorIt);
            AZ_Assert(drawBatch.color == color, "Test failed");
        }
    }

    using StringList = AZStd::list < LyShine::StringType >;

    void AssertDrawBatchTextContent(
        const AZStd::list<UiTextComponent::DrawBatch>& drawBatches,
        const StringList& stringList)
    {
        AZ_Assert(drawBatches.size() == stringList.size(), "Test failed");

        auto drawBatchIt = drawBatches.begin();
        auto stringIt = stringList.begin();
        for (;
            drawBatchIt != drawBatches.end();
            ++drawBatchIt, ++stringIt)
        {
            const UiTextComponent::DrawBatch& drawBatch = *drawBatchIt;
            const LyShine::StringType& text = *stringIt;
            AZ_Assert(drawBatch.text == text, "Test failed");
        }
    }

    void AssertDrawBatchTextNumNewlines(
        const AZStd::list<UiTextComponent::DrawBatch>& drawBatches,
        const int numNewlines)
    {
        int numNewlinesFound = 0;
        auto drawBatchIt = drawBatches.begin();
        for (;
            drawBatchIt != drawBatches.end();
            ++drawBatchIt)
        {
            const UiTextComponent::DrawBatch& drawBatch = *drawBatchIt;
            numNewlinesFound += static_cast<int>(AZStd::count_if(drawBatch.text.begin(), drawBatch.text.end(),
            [](char c) -> bool
            {
                return c == '\n';
            }));
        }
        AZ_Assert(numNewlines == numNewlinesFound, "Test failed");
    }

    FontFamilyPtr FontFamilyLoad(const char* fontFamilyFilename)
    {
        FontFamilyPtr fontFamily = gEnv->pCryFont->GetFontFamily(fontFamilyFilename);
        if (!fontFamily)
        {
            fontFamily = gEnv->pCryFont->LoadFontFamily(fontFamilyFilename);
            AZ_Assert(gEnv->pCryFont->GetFontFamily(fontFamilyFilename).get(), "Test failed");
        }

        // We need the font family to load correctly in order to test properly
        AZ_Assert(fontFamily.get(), "Test failed");

        return fontFamily;
    }

    //! \brief Verify fonts that ship with Open 3D Engine load correctly.
    //!
    //! This test depends on the LyShineExamples and UiBasics gems being
    //! included in the project.
    //!
    //! There are other fonts that ship in other projects (SamplesProject,
    //! FeatureTests), but that would call for project-specific unit-tests
    //! which don't belong here.
    void VerifyShippingFonts()
    {
        FontFamilyLoad("ui/fonts/lyshineexamples/notosans/notosans.fontfamily");
        FontFamilyLoad("ui/fonts/lyshineexamples/notoserif/notoserif.fontfamily");
        FontFamilyLoad("fonts/vera.fontfamily");
    }

    void NewlineSanitizeTests()
    {
        {
            AZStd::string inputString("Test\\nHi");
            SanitizeUserEnteredNewlineChar(inputString);

            const AZStd::string expectedOutput("Test\nHi");
            AZ_Assert(expectedOutput == inputString, "Test failed");

            // Sanity check that AZStd::regex and std::regex are functionally equivalent.
            {
                const std::string NewlineDelimiter("\n");
                const std::regex UserInputNewlineDelimiter("\\\\n");
                std::string inputStringCopy("Test\\nHi");
                inputStringCopy = std::regex_replace(inputStringCopy, UserInputNewlineDelimiter, NewlineDelimiter);
                AZ_Assert(inputStringCopy == std::string(inputString.c_str()), "Test failed");
                AZ_Assert(AZStd::string(inputStringCopy.c_str()) == inputString, "Test failed");
            }
        }

    }

    void BuildDrawBatchesTests(FontFamily* fontFamily)
    {
        UiTextComponent::InlineImageContainer inlineImages;
        const float defaultImageHeight = 32.0f;

        STextDrawContext fontContext;
        fontContext.SetEffect(0);
        fontContext.SetSizeIn800x600(false);
        fontContext.SetSize(Vec2(32.0f, 32.0f));
        const float defaultAscent = fontFamily->normal->GetAscender(fontContext);

        // Plain string
        {
            const LyShine::StringType markupTestString("this is a test!");

            {
                TextMarkup::Tag markupRoot;
                AZ_Assert(TextMarkup::ParseMarkupBuffer(markupTestString, markupRoot), "Test failed");
                AZStd::list<UiTextComponent::DrawBatch> drawBatches;
                AZStd::stack<UiTextComponent::DrawBatch> batchStack;

                AZStd::stack<FontFamily*> fontFamilyStack;
                fontFamilyStack.push(fontFamily);

                UiTextComponent::FontFamilyRefSet fontFamilyRefs;
                BuildDrawBatchesAndAssignClickableIds(drawBatches, fontFamilyRefs, inlineImages, defaultImageHeight, defaultAscent, batchStack, fontFamilyStack, &markupRoot);
                AZ_Assert(0 == fontFamilyRefs.size(), "Test failed");
                AZ_Assert(1 == drawBatches.size(), "Test failed");
                AssertTextNotEmpty(drawBatches);

                FontList fontList;
                fontList.push_back(fontFamily->normal);
                AssertDrawBatchFontOrder(drawBatches, fontList);

                AssertDrawBatchSingleColor(drawBatches, TextMarkup::ColorInvalid);

            }
        }

        // Plain string: newline
        {
            const LyShine::StringType markupTestString("Regular Bold Italic\n");

            {
                TextMarkup::Tag markupRoot;
                AZ_Assert(TextMarkup::ParseMarkupBuffer(markupTestString, markupRoot), "Test failed");

                AZStd::list<UiTextComponent::DrawBatch> drawBatches;
                AZStd::stack<UiTextComponent::DrawBatch> batchStack;

                AZStd::stack<FontFamily*> fontFamilyStack;
                fontFamilyStack.push(fontFamily);

                UiTextComponent::FontFamilyRefSet fontFamilyRefs;
                BuildDrawBatchesAndAssignClickableIds(drawBatches, fontFamilyRefs, inlineImages, defaultImageHeight, defaultAscent, batchStack, fontFamilyStack, &markupRoot);
                AZ_Assert(0 == fontFamilyRefs.size(), "Test failed");
                AZ_Assert(1 == drawBatches.size(), "Test failed");
                AssertTextNotEmpty(drawBatches);

                FontList fontList;
                fontList.push_back(fontFamily->normal);
                AssertDrawBatchFontOrder(drawBatches, fontList);

                AssertDrawBatchSingleColor(drawBatches, TextMarkup::ColorInvalid);

            }
        }

        // Single bold
        {
            const LyShine::StringType markupTestString("<b>this</b> is a test!");

            {
                TextMarkup::Tag markupRoot;
                AZ_Assert(TextMarkup::ParseMarkupBuffer(markupTestString, markupRoot), "Test failed");
                AZStd::list<UiTextComponent::DrawBatch> drawBatches;
                AZStd::stack<UiTextComponent::DrawBatch> batchStack;

                AZStd::stack<FontFamily*> fontFamilyStack;
                fontFamilyStack.push(fontFamily);

                UiTextComponent::FontFamilyRefSet fontFamilyRefs;
                BuildDrawBatchesAndAssignClickableIds(drawBatches, fontFamilyRefs, inlineImages, defaultImageHeight, defaultAscent, batchStack, fontFamilyStack, &markupRoot);
                AZ_Assert(0 == fontFamilyRefs.size(), "Test failed");
                AZ_Assert(2 == drawBatches.size(), "Test failed");
                AssertTextNotEmpty(drawBatches);

                FontList fontList;
                fontList.push_back(fontFamily->bold);
                fontList.push_back(fontFamily->normal);
                AssertDrawBatchFontOrder(drawBatches, fontList);

                AssertDrawBatchSingleColor(drawBatches, TextMarkup::ColorInvalid);
            }
        }

        // Single italic
        {
            const LyShine::StringType markupTestString("<i>this</i> is a test!");

            {
                TextMarkup::Tag markupRoot;
                AZ_Assert(TextMarkup::ParseMarkupBuffer(markupTestString, markupRoot), "Test failed");
                AZStd::list<UiTextComponent::DrawBatch> drawBatches;
                AZStd::stack<UiTextComponent::DrawBatch> batchStack;

                AZStd::stack<FontFamily*> fontFamilyStack;
                fontFamilyStack.push(fontFamily);

                UiTextComponent::FontFamilyRefSet fontFamilyRefs;
                BuildDrawBatchesAndAssignClickableIds(drawBatches, fontFamilyRefs, inlineImages, defaultImageHeight, defaultAscent, batchStack, fontFamilyStack, &markupRoot);
                AZ_Assert(0 == fontFamilyRefs.size(), "Test failed");
                AZ_Assert(2 == drawBatches.size(), "Test failed");
                AssertTextNotEmpty(drawBatches);

                FontList fontList;
                fontList.push_back(fontFamily->italic);
                fontList.push_back(fontFamily->normal);
                AssertDrawBatchFontOrder(drawBatches, fontList);

                AssertDrawBatchSingleColor(drawBatches, TextMarkup::ColorInvalid);
            }
        }

        // Bold-italic
        {
            const LyShine::StringType markupTestString("<b><i>this</i></b> is a test!");

            {
                TextMarkup::Tag markupRoot;
                AZ_Assert(TextMarkup::ParseMarkupBuffer(markupTestString, markupRoot), "Test failed");
                AZStd::list<UiTextComponent::DrawBatch> drawBatches;
                AZStd::stack<UiTextComponent::DrawBatch> batchStack;

                AZStd::stack<FontFamily*> fontFamilyStack;
                fontFamilyStack.push(fontFamily);

                UiTextComponent::FontFamilyRefSet fontFamilyRefs;
                BuildDrawBatchesAndAssignClickableIds(drawBatches, fontFamilyRefs, inlineImages, defaultImageHeight, defaultAscent, batchStack, fontFamilyStack, &markupRoot);
                AZ_Assert(0 == fontFamilyRefs.size(), "Test failed");
                AZ_Assert(2 == drawBatches.size(), "Test failed");
                AssertTextNotEmpty(drawBatches);

                FontList fontList;
                fontList.push_back(fontFamily->boldItalic);
                fontList.push_back(fontFamily->normal);
                AssertDrawBatchFontOrder(drawBatches, fontList);

                AssertDrawBatchSingleColor(drawBatches, TextMarkup::ColorInvalid);
            }
        }

        // Anchor tag
        {
            const LyShine::StringType markupTestString("<a action=\"action\" data=\"data\">this</a> is a test!");

            {
                TextMarkup::Tag markupRoot;
                AZ_Assert(TextMarkup::ParseMarkupBuffer(markupTestString, markupRoot), "Test failed");
                AZStd::list<UiTextComponent::DrawBatch> drawBatches;
                AZStd::stack<UiTextComponent::DrawBatch> batchStack;

                AZStd::stack<FontFamily*> fontFamilyStack;
                fontFamilyStack.push(fontFamily);

                UiTextComponent::FontFamilyRefSet fontFamilyRefs;
                BuildDrawBatchesAndAssignClickableIds(drawBatches, fontFamilyRefs, inlineImages, defaultImageHeight, defaultAscent, batchStack, fontFamilyStack, &markupRoot);
                AZ_Assert(0 == fontFamilyRefs.size(), "Test failed");
                AZ_Assert(2 == drawBatches.size(), "Test failed");
                AssertTextNotEmpty(drawBatches);

                auto drawBatchIter = drawBatches.begin();
                const auto& drawBatch = *drawBatchIter;
                AZ_Assert(drawBatch.IsClickable(), "Test failed");
                AZ_Assert(UiTextComponent::DrawBatch::Type::Text == drawBatch.GetType(), "Test failed");
                AZ_Assert(drawBatch.action == "action", "Test failed");
                AZ_Assert(drawBatch.data == "data", "Test failed");
                AZ_Assert(drawBatch.clickableId == 0, "Test failed");

                ++drawBatchIter;
                const auto& nextDrawBatch = *drawBatchIter;
                AZ_Assert(!nextDrawBatch.IsClickable(), "Test failed");
                AZ_Assert(UiTextComponent::DrawBatch::Type::Text == drawBatch.GetType(), "Test failed");
                AZ_Assert(nextDrawBatch.action.empty(), "Test failed");
                AZ_Assert(nextDrawBatch.data.empty(), "Test failed");
                AZ_Assert(nextDrawBatch.clickableId == -1, "Test failed");

                StringList stringList;
                stringList.push_back("this");
                stringList.push_back(" is a test!");
                AssertDrawBatchTextContent(drawBatches, stringList);

                FontList fontList;
                fontList.push_back(fontFamily->normal);
                fontList.push_back(fontFamily->normal);
                AssertDrawBatchFontOrder(drawBatches, fontList);

                ColorList colorList;
                colorList.push_back(TextMarkup::ColorInvalid);
                colorList.push_back(TextMarkup::ColorInvalid);
                AssertDrawBatchMultiColor(drawBatches, colorList);
            }
        }

        // Anchor tag: multiple anchor tags
        {
            const LyShine::StringType markupTestString(
                "<a action=\"action1\" data=\"data1\">this</a>"
                " is a <a action=\"action2\" data=\"data2\">test</a>!");

            {
                TextMarkup::Tag markupRoot;
                AZ_Assert(TextMarkup::ParseMarkupBuffer(markupTestString, markupRoot), "Test failed");
                AZStd::list<UiTextComponent::DrawBatch> drawBatches;
                AZStd::stack<UiTextComponent::DrawBatch> batchStack;

                AZStd::stack<FontFamily*> fontFamilyStack;
                fontFamilyStack.push(fontFamily);

                UiTextComponent::FontFamilyRefSet fontFamilyRefs;
                BuildDrawBatchesAndAssignClickableIds(drawBatches, fontFamilyRefs, inlineImages, defaultImageHeight, defaultAscent, batchStack, fontFamilyStack, &markupRoot);
                AZ_Assert(0 == fontFamilyRefs.size(), "Test failed");
                AZ_Assert(4 == drawBatches.size(), "Test failed");
                AssertTextNotEmpty(drawBatches);

                auto batchIter = drawBatches.begin();
                {
                    const auto& drawBatch = *(batchIter++);
                    AZ_Assert(drawBatch.IsClickable(), "Test failed");
                    AZ_Assert(UiTextComponent::DrawBatch::Type::Text == drawBatch.GetType(), "Test failed");
                    AZ_Assert(drawBatch.action == "action1", "Test failed");
                    AZ_Assert(drawBatch.data == "data1", "Test failed");
                    AZ_Assert(drawBatch.clickableId == 0, "Test failed");
                }

                {
                    const auto& drawBatch = *(batchIter++);
                    AZ_Assert(!drawBatch.IsClickable(), "Test failed");
                    AZ_Assert(UiTextComponent::DrawBatch::Type::Text == drawBatch.GetType(), "Test failed");
                    AZ_Assert(drawBatch.action.empty(), "Test failed");
                    AZ_Assert(drawBatch.data.empty(), "Test failed");
                    AZ_Assert(drawBatch.clickableId == -1, "Test failed");
                }

                {
                    const auto& drawBatch = *(batchIter++);
                    AZ_Assert(drawBatch.IsClickable(), "Test failed");
                    AZ_Assert(UiTextComponent::DrawBatch::Type::Text == drawBatch.GetType(), "Test failed");
                    AZ_Assert(drawBatch.action == "action2", "Test failed");
                    AZ_Assert(drawBatch.data == "data2", "Test failed");
                    AZ_Assert(drawBatch.clickableId == 1, "Test failed");
                }

                {
                    const auto& drawBatch = *(batchIter++);
                    AZ_Assert(!drawBatch.IsClickable(), "Test failed");
                    AZ_Assert(UiTextComponent::DrawBatch::Type::Text == drawBatch.GetType(), "Test failed");
                    AZ_Assert(drawBatch.action.empty(), "Test failed");
                    AZ_Assert(drawBatch.data.empty(), "Test failed");
                    AZ_Assert(drawBatch.clickableId == -1, "Test failed");
                }

                StringList stringList;
                stringList.push_back("this");
                stringList.push_back(" is a ");
                stringList.push_back("test");
                stringList.push_back("!");
                AssertDrawBatchTextContent(drawBatches, stringList);

                FontList fontList;
                fontList.push_back(fontFamily->normal);
                fontList.push_back(fontFamily->normal);
                fontList.push_back(fontFamily->normal);
                fontList.push_back(fontFamily->normal);
                AssertDrawBatchFontOrder(drawBatches, fontList);

                ColorList colorList;
                colorList.push_back(TextMarkup::ColorInvalid);
                colorList.push_back(TextMarkup::ColorInvalid);
                colorList.push_back(TextMarkup::ColorInvalid);
                colorList.push_back(TextMarkup::ColorInvalid);
                AssertDrawBatchMultiColor(drawBatches, colorList);
            }
        }

        {
            const LyShine::StringType markupTestString(
                "<a action=\"action1\" data=\"data1\">this</a>"
                "<a action=\"action2\" data=\"data2\"> is a test!</a>");

            {
                TextMarkup::Tag markupRoot;
                AZ_Assert(TextMarkup::ParseMarkupBuffer(markupTestString, markupRoot), "Test failed");
                AZStd::list<UiTextComponent::DrawBatch> drawBatches;
                AZStd::stack<UiTextComponent::DrawBatch> batchStack;

                AZStd::stack<FontFamily*> fontFamilyStack;
                fontFamilyStack.push(fontFamily);

                UiTextComponent::FontFamilyRefSet fontFamilyRefs;
                BuildDrawBatchesAndAssignClickableIds(drawBatches, fontFamilyRefs, inlineImages, defaultImageHeight, defaultAscent, batchStack, fontFamilyStack, &markupRoot);
                AZ_Assert(0 == fontFamilyRefs.size(), "Test failed");
                AZ_Assert(2 == drawBatches.size(), "Test failed");
                AssertTextNotEmpty(drawBatches);

                auto batchIter = drawBatches.begin();
                {
                    const auto& drawBatch = *(batchIter++);
                    AZ_Assert(drawBatch.IsClickable(), "Test failed");
                    AZ_Assert(UiTextComponent::DrawBatch::Type::Text == drawBatch.GetType(), "Test failed");
                    AZ_Assert(drawBatch.action == "action1", "Test failed");
                    AZ_Assert(drawBatch.data == "data1", "Test failed");
                    AZ_Assert(drawBatch.clickableId == 0, "Test failed");
                }

                {
                    const auto& drawBatch = *(batchIter++);
                    AZ_Assert(drawBatch.IsClickable(), "Test failed");
                    AZ_Assert(UiTextComponent::DrawBatch::Type::Text == drawBatch.GetType(), "Test failed");
                    AZ_Assert(drawBatch.action == "action2", "Test failed");
                    AZ_Assert(drawBatch.data == "data2", "Test failed");
                    AZ_Assert(drawBatch.clickableId == 1, "Test failed");
                }

                StringList stringList;
                stringList.push_back("this");
                stringList.push_back(" is a test!");
                AssertDrawBatchTextContent(drawBatches, stringList);

                FontList fontList;
                fontList.push_back(fontFamily->normal);
                fontList.push_back(fontFamily->normal);
                AssertDrawBatchFontOrder(drawBatches, fontList);

                ColorList colorList;
                colorList.push_back(TextMarkup::ColorInvalid);
                colorList.push_back(TextMarkup::ColorInvalid);
                AssertDrawBatchMultiColor(drawBatches, colorList);
            }
        }

        {
            const LyShine::StringType markupTestString(
                "<b><a action=\"action1\" data=\"data1\">this</a></b> is "
                "<a action=\"action2\" data=\"data2\">a test!</a>");

            {
                TextMarkup::Tag markupRoot;
                AZ_Assert(TextMarkup::ParseMarkupBuffer(markupTestString, markupRoot), "Test failed");
                AZStd::list<UiTextComponent::DrawBatch> drawBatches;
                AZStd::stack<UiTextComponent::DrawBatch> batchStack;

                AZStd::stack<FontFamily*> fontFamilyStack;
                fontFamilyStack.push(fontFamily);

                UiTextComponent::FontFamilyRefSet fontFamilyRefs;
                BuildDrawBatchesAndAssignClickableIds(drawBatches, fontFamilyRefs, inlineImages, defaultImageHeight, defaultAscent, batchStack, fontFamilyStack, &markupRoot);
                AZ_Assert(0 == fontFamilyRefs.size(), "Test failed");
                AZ_Assert(3 == drawBatches.size(), "Test failed");
                AssertTextNotEmpty(drawBatches);

                auto batchIter = drawBatches.begin();
                {
                    const auto& drawBatch = *(batchIter++);
                    AZ_Assert(drawBatch.IsClickable(), "Test failed");
                    AZ_Assert(UiTextComponent::DrawBatch::Type::Text == drawBatch.GetType(), "Test failed");
                    AZ_Assert(drawBatch.action == "action1", "Test failed");
                    AZ_Assert(drawBatch.data == "data1", "Test failed");
                    AZ_Assert(drawBatch.clickableId == 0, "Test failed");
                }

                {
                    const auto& drawBatch = *(batchIter++);
                    AZ_Assert(!drawBatch.IsClickable(), "Test failed");
                    AZ_Assert(UiTextComponent::DrawBatch::Type::Text == drawBatch.GetType(), "Test failed");
                    AZ_Assert(drawBatch.action.empty(), "Test failed");
                    AZ_Assert(drawBatch.data.empty(), "Test failed");
                    AZ_Assert(drawBatch.clickableId == -1, "Test failed");
                }

                {
                    const auto& drawBatch = *(batchIter++);
                    AZ_Assert(drawBatch.IsClickable(), "Test failed");
                    AZ_Assert(UiTextComponent::DrawBatch::Type::Text == drawBatch.GetType(), "Test failed");
                    AZ_Assert(drawBatch.action == "action2", "Test failed");
                    AZ_Assert(drawBatch.data == "data2", "Test failed");
                    AZ_Assert(drawBatch.clickableId == 1, "Test failed");
                }

                StringList stringList;
                stringList.push_back("this");
                stringList.push_back(" is ");
                stringList.push_back("a test!");
                AssertDrawBatchTextContent(drawBatches, stringList);

                FontList fontList;
                fontList.push_back(fontFamily->bold);
                fontList.push_back(fontFamily->normal);
                fontList.push_back(fontFamily->normal);
                AssertDrawBatchFontOrder(drawBatches, fontList);

                ColorList colorList;
                colorList.push_back(TextMarkup::ColorInvalid);
                colorList.push_back(TextMarkup::ColorInvalid);
                colorList.push_back(TextMarkup::ColorInvalid);
                AssertDrawBatchMultiColor(drawBatches, colorList);
            }
        }

        // Anchor tag with link color applied via markup
        {
            const LyShine::StringType markupTestString("<font color=\"#ff0000\"><a action=\"action\" data=\"data\">this</a></font> is a test!");

            {
                TextMarkup::Tag markupRoot;
                AZ_Assert(TextMarkup::ParseMarkupBuffer(markupTestString, markupRoot), "Test failed");
                AZStd::list<UiTextComponent::DrawBatch> drawBatches;
                AZStd::stack<UiTextComponent::DrawBatch> batchStack;

                AZStd::stack<FontFamily*> fontFamilyStack;
                fontFamilyStack.push(fontFamily);

                UiTextComponent::FontFamilyRefSet fontFamilyRefs;
                BuildDrawBatchesAndAssignClickableIds(drawBatches, fontFamilyRefs, inlineImages, defaultImageHeight, defaultAscent, batchStack, fontFamilyStack, &markupRoot);
                AZ_Assert(0 == fontFamilyRefs.size(), "Test failed");
                AZ_Assert(2 == drawBatches.size(), "Test failed");
                AssertTextNotEmpty(drawBatches);

                auto batchIter = drawBatches.begin();
                {
                    const auto& drawBatch = *(batchIter++);
                    AZ_Assert(drawBatch.IsClickable(), "Test failed");
                    AZ_Assert(UiTextComponent::DrawBatch::Type::Text == drawBatch.GetType(), "Test failed");
                    AZ_Assert(drawBatch.action == "action", "Test failed");
                    AZ_Assert(drawBatch.data == "data", "Test failed");
                    AZ_Assert(drawBatch.clickableId == 0, "Test failed");
                }

                {
                    const auto& drawBatch = *(batchIter++);
                    AZ_Assert(!drawBatch.IsClickable(), "Test failed");
                    AZ_Assert(UiTextComponent::DrawBatch::Type::Text == drawBatch.GetType(), "Test failed");
                    AZ_Assert(drawBatch.action.empty(), "Test failed");
                    AZ_Assert(drawBatch.data.empty(), "Test failed");
                    AZ_Assert(drawBatch.clickableId == -1, "Test failed");
                }

                StringList stringList;
                stringList.push_back("this");
                stringList.push_back(" is a test!");
                AssertDrawBatchTextContent(drawBatches, stringList);

                FontList fontList;
                fontList.push_back(fontFamily->normal);
                fontList.push_back(fontFamily->normal);
                AssertDrawBatchFontOrder(drawBatches, fontList);

                ColorList colorList;
                colorList.push_back(AZ::Vector3(1.0f, 0.0f, 0.0f));
                colorList.push_back(TextMarkup::ColorInvalid);
                AssertDrawBatchMultiColor(drawBatches, colorList);
            }
        }

        // Anchor tag with multiple colors within link
        {
            const LyShine::StringType markupTestString("<a action=\"action\" data=\"data\">this <font color=\"#ff0000\">is</font> a test!</a>");

            {
                TextMarkup::Tag markupRoot;
                AZ_Assert(TextMarkup::ParseMarkupBuffer(markupTestString, markupRoot), "Test failed");
                AZStd::list<UiTextComponent::DrawBatch> drawBatches;
                AZStd::stack<UiTextComponent::DrawBatch> batchStack;

                AZStd::stack<FontFamily*> fontFamilyStack;
                fontFamilyStack.push(fontFamily);

                UiTextComponent::FontFamilyRefSet fontFamilyRefs;
                BuildDrawBatchesAndAssignClickableIds(drawBatches, fontFamilyRefs, inlineImages, defaultImageHeight, defaultAscent, batchStack, fontFamilyStack, &markupRoot);
                AZ_Assert(0 == fontFamilyRefs.size(), "Test failed");
                AZ_Assert(3 == drawBatches.size(), "Test failed");
                AssertTextNotEmpty(drawBatches);

                // All drawbatches should have the same clickable ID since there's only one link that
                // encompasses all of the text.
                for (auto& drawBatch : drawBatches)
                {
                    AZ_Assert(drawBatch.IsClickable(), "Test failed");
                    AZ_Assert(UiTextComponent::DrawBatch::Type::Text == drawBatch.GetType(), "Test failed");
                    AZ_Assert(drawBatch.action == "action", "Test failed");
                    AZ_Assert(drawBatch.data == "data", "Test failed");
                    AZ_Assert(drawBatch.clickableId == 0, "Test failed");
                }

                StringList stringList;
                stringList.push_back("this ");
                stringList.push_back("is");
                stringList.push_back(" a test!");
                AssertDrawBatchTextContent(drawBatches, stringList);

                FontList fontList;
                fontList.push_back(fontFamily->normal);
                fontList.push_back(fontFamily->normal);
                fontList.push_back(fontFamily->normal);
                AssertDrawBatchFontOrder(drawBatches, fontList);

                ColorList colorList;
                colorList.push_back(TextMarkup::ColorInvalid);
                colorList.push_back(AZ::Vector3(1.0f, 0.0f, 0.0f));
                colorList.push_back(TextMarkup::ColorInvalid);
                AssertDrawBatchMultiColor(drawBatches, colorList);
            }
        }

        // Multiple anchor tags with link colors applied within markup
        {
            const LyShine::StringType markupTestString("<a action=\"action1\" data=\"data1\">this <font color=\"#ff0000\">is</font></a> a <a action=\"action2\" data=\"data2\">te<font color=\"#ff0000\">st!</font></a>");

            {
                TextMarkup::Tag markupRoot;
                AZ_Assert(TextMarkup::ParseMarkupBuffer(markupTestString, markupRoot), "Test failed");
                AZStd::list<UiTextComponent::DrawBatch> drawBatches;
                AZStd::stack<UiTextComponent::DrawBatch> batchStack;

                AZStd::stack<FontFamily*> fontFamilyStack;
                fontFamilyStack.push(fontFamily);

                UiTextComponent::FontFamilyRefSet fontFamilyRefs;
                BuildDrawBatchesAndAssignClickableIds(drawBatches, fontFamilyRefs, inlineImages, defaultImageHeight, defaultAscent, batchStack, fontFamilyStack, &markupRoot);
                AZ_Assert(0 == fontFamilyRefs.size(), "Test failed");
                AZ_Assert(5 == drawBatches.size(), "Test failed");
                AssertTextNotEmpty(drawBatches);

                auto batchIter = drawBatches.begin();
                {
                    const auto& drawBatch = *(batchIter++);
                    AZ_Assert(drawBatch.IsClickable(), "Test failed");
                    AZ_Assert(UiTextComponent::DrawBatch::Type::Text == drawBatch.GetType(), "Test failed");
                    AZ_Assert(drawBatch.action == "action1", "Test failed");
                    AZ_Assert(drawBatch.data == "data1", "Test failed");
                    AZ_Assert(drawBatch.clickableId == 0, "Test failed");
                }

                {
                    const auto& drawBatch = *(batchIter++);
                    AZ_Assert(drawBatch.IsClickable(), "Test failed");
                    AZ_Assert(UiTextComponent::DrawBatch::Type::Text == drawBatch.GetType(), "Test failed");
                    AZ_Assert(drawBatch.action == "action1", "Test failed");
                    AZ_Assert(drawBatch.data == "data1", "Test failed");
                    AZ_Assert(drawBatch.clickableId == 0, "Test failed");
                }

                {
                    const auto& drawBatch = *(batchIter++);
                    AZ_Assert(!drawBatch.IsClickable(), "Test failed");
                    AZ_Assert(UiTextComponent::DrawBatch::Type::Text == drawBatch.GetType(), "Test failed");
                    AZ_Assert(drawBatch.action.empty(), "Test failed");
                    AZ_Assert(drawBatch.data.empty(), "Test failed");
                    AZ_Assert(drawBatch.clickableId == -1, "Test failed");
                }

                {
                    const auto& drawBatch = *(batchIter++);
                    AZ_Assert(drawBatch.IsClickable(), "Test failed");
                    AZ_Assert(UiTextComponent::DrawBatch::Type::Text == drawBatch.GetType(), "Test failed");
                    AZ_Assert(drawBatch.action == "action2", "Test failed");
                    AZ_Assert(drawBatch.data == "data2", "Test failed");
                    AZ_Assert(drawBatch.clickableId == 1, "Test failed");
                }

                {
                    const auto& drawBatch = *(batchIter++);
                    AZ_Assert(drawBatch.IsClickable(), "Test failed");
                    AZ_Assert(UiTextComponent::DrawBatch::Type::Text == drawBatch.GetType(), "Test failed");
                    AZ_Assert(drawBatch.action == "action2", "Test failed");
                    AZ_Assert(drawBatch.data == "data2", "Test failed");
                    AZ_Assert(drawBatch.clickableId == 1, "Test failed");
                }

                StringList stringList;
                stringList.push_back("this ");
                stringList.push_back("is");
                stringList.push_back(" a ");
                stringList.push_back("te");
                stringList.push_back("st!");
                AssertDrawBatchTextContent(drawBatches, stringList);

                FontList fontList;
                fontList.push_back(fontFamily->normal);
                fontList.push_back(fontFamily->normal);
                fontList.push_back(fontFamily->normal);
                fontList.push_back(fontFamily->normal);
                fontList.push_back(fontFamily->normal);
                AssertDrawBatchFontOrder(drawBatches, fontList);

                ColorList colorList;
                colorList.push_back(TextMarkup::ColorInvalid);
                colorList.push_back(AZ::Vector3(1.0f, 0.0f, 0.0f));
                colorList.push_back(TextMarkup::ColorInvalid);
                colorList.push_back(TextMarkup::ColorInvalid);
                colorList.push_back(AZ::Vector3(1.0f, 0.0f, 0.0f));
                AssertDrawBatchMultiColor(drawBatches, colorList);
            }
        }

        // Font tag: font face
        {
            const LyShine::StringType markupTestString("<font face=\"notoserif\">this</font> is a test!");

            {
                TextMarkup::Tag markupRoot;
                AZ_Assert(TextMarkup::ParseMarkupBuffer(markupTestString, markupRoot), "Test failed");
                AZStd::list<UiTextComponent::DrawBatch> drawBatches;
                AZStd::stack<UiTextComponent::DrawBatch> batchStack;

                AZStd::stack<FontFamily*> fontFamilyStack;
                fontFamilyStack.push(fontFamily);

                UiTextComponent::FontFamilyRefSet fontFamilyRefs;
                BuildDrawBatchesAndAssignClickableIds(drawBatches, fontFamilyRefs, inlineImages, defaultImageHeight, defaultAscent, batchStack, fontFamilyStack, &markupRoot);
                AZ_Assert(1 == fontFamilyRefs.size(), "Test failed");
                AZ_Assert(2 == drawBatches.size(), "Test failed");
                AssertTextNotEmpty(drawBatches);

                StringList stringList;
                stringList.push_back("this");
                stringList.push_back(" is a test!");
                AssertDrawBatchTextContent(drawBatches, stringList);

                FontFamilyPtr notoSerifFamily = gEnv->pCryFont->GetFontFamily("notoserif");
                AZ_Assert(notoSerifFamily, "Test failed");

                FontList fontList;
                fontList.push_back(notoSerifFamily->normal);
                fontList.push_back(fontFamily->normal);
                AssertDrawBatchFontOrder(drawBatches, fontList);

                ColorList colorList;
                colorList.push_back(TextMarkup::ColorInvalid);
                colorList.push_back(TextMarkup::ColorInvalid);
                AssertDrawBatchMultiColor(drawBatches, colorList);
            }
        }

        // Font tag: font face (different font)
        {
            const LyShine::StringType markupTestString("<font face=\"notosans\">this</font> is a test!");

            {
                TextMarkup::Tag markupRoot;
                AZ_Assert(TextMarkup::ParseMarkupBuffer(markupTestString, markupRoot), "Test failed");
                AZStd::list<UiTextComponent::DrawBatch> drawBatches;
                AZStd::stack<UiTextComponent::DrawBatch> batchStack;

                AZStd::stack<FontFamily*> fontFamilyStack;
                fontFamilyStack.push(fontFamily);

                UiTextComponent::FontFamilyRefSet fontFamilyRefs;
                BuildDrawBatchesAndAssignClickableIds(drawBatches, fontFamilyRefs, inlineImages, defaultImageHeight, defaultAscent, batchStack, fontFamilyStack, &markupRoot);
                AZ_Assert(1 == fontFamilyRefs.size(), "Test failed", "Test failed");
                AZ_Assert(2 == drawBatches.size(), "Test failed", "Test failed");
                AssertTextNotEmpty(drawBatches);

                StringList stringList;
                stringList.push_back("this");
                stringList.push_back(" is a test!");
                AssertDrawBatchTextContent(drawBatches, stringList);

                FontFamilyPtr notoSansFamily = gEnv->pCryFont->GetFontFamily("notosans");
                AZ_Assert(notoSansFamily, "Test failed");

                FontList fontList;
                fontList.push_back(notoSansFamily->normal);
                fontList.push_back(fontFamily->normal);
                AssertDrawBatchFontOrder(drawBatches, fontList);

                ColorList colorList;
                colorList.push_back(TextMarkup::ColorInvalid);
                colorList.push_back(TextMarkup::ColorInvalid);
                AssertDrawBatchMultiColor(drawBatches, colorList);
            }
        }

        // Font tag: font face (leading space)
        {
            const LyShine::StringType markupTestString("<font face=\"   notosans\">this</font> is a test!");

            {
                TextMarkup::Tag markupRoot;
                AZ_Assert(TextMarkup::ParseMarkupBuffer(markupTestString, markupRoot), "Test failed");
                AZStd::list<UiTextComponent::DrawBatch> drawBatches;
                AZStd::stack<UiTextComponent::DrawBatch> batchStack;

                AZStd::stack<FontFamily*> fontFamilyStack;
                fontFamilyStack.push(fontFamily);

                UiTextComponent::FontFamilyRefSet fontFamilyRefs;
                BuildDrawBatchesAndAssignClickableIds(drawBatches, fontFamilyRefs, inlineImages, defaultImageHeight, defaultAscent, batchStack, fontFamilyStack, &markupRoot);
                AZ_Assert(1 == fontFamilyRefs.size(), "Test failed");
                AZ_Assert(2 == drawBatches.size(), "Test failed");
                AssertTextNotEmpty(drawBatches);

                StringList stringList;
                stringList.push_back("this");
                stringList.push_back(" is a test!");
                AssertDrawBatchTextContent(drawBatches, stringList);

                FontFamilyPtr notoSansFamily = gEnv->pCryFont->GetFontFamily("notosans");
                AZ_Assert(notoSansFamily, "Test failed");

                FontList fontList;
                fontList.push_back(notoSansFamily->normal);
                fontList.push_back(fontFamily->normal);
                AssertDrawBatchFontOrder(drawBatches, fontList);

                ColorList colorList;
                colorList.push_back(TextMarkup::ColorInvalid);
                colorList.push_back(TextMarkup::ColorInvalid);
                AssertDrawBatchMultiColor(drawBatches, colorList);
            }
        }

        // Font tag: font face (trailing space)
        {
            const LyShine::StringType markupTestString("<font face=\"notosans   \">this</font> is a test!");

            {
                TextMarkup::Tag markupRoot;
                AZ_Assert(TextMarkup::ParseMarkupBuffer(markupTestString, markupRoot), "Test failed");
                AZStd::list<UiTextComponent::DrawBatch> drawBatches;
                AZStd::stack<UiTextComponent::DrawBatch> batchStack;

                AZStd::stack<FontFamily*> fontFamilyStack;
                fontFamilyStack.push(fontFamily);

                UiTextComponent::FontFamilyRefSet fontFamilyRefs;
                BuildDrawBatchesAndAssignClickableIds(drawBatches, fontFamilyRefs, inlineImages, defaultImageHeight, defaultAscent, batchStack, fontFamilyStack, &markupRoot);
                AZ_Assert(1 == fontFamilyRefs.size(), "Test failed");
                AZ_Assert(2 == drawBatches.size(), "Test failed");
                AssertTextNotEmpty(drawBatches);

                StringList stringList;
                stringList.push_back("this");
                stringList.push_back(" is a test!");
                AssertDrawBatchTextContent(drawBatches, stringList);

                FontFamilyPtr notoSansFamily = gEnv->pCryFont->GetFontFamily("notosans");
                AZ_Assert(notoSansFamily, "Test failed");

                FontList fontList;
                fontList.push_back(notoSansFamily->normal);
                fontList.push_back(fontFamily->normal);
                AssertDrawBatchFontOrder(drawBatches, fontList);

                ColorList colorList;
                colorList.push_back(TextMarkup::ColorInvalid);
                colorList.push_back(TextMarkup::ColorInvalid);
                AssertDrawBatchMultiColor(drawBatches, colorList);
            }
        }

        // Font tag: font face (leading and trailing space)
        {
            const LyShine::StringType markupTestString("<font face=\"    notosans   \">this</font> is a test!");

            {
                TextMarkup::Tag markupRoot;
                AZ_Assert(TextMarkup::ParseMarkupBuffer(markupTestString, markupRoot), "Test failed");
                AZStd::list<UiTextComponent::DrawBatch> drawBatches;
                AZStd::stack<UiTextComponent::DrawBatch> batchStack;

                AZStd::stack<FontFamily*> fontFamilyStack;
                fontFamilyStack.push(fontFamily);

                UiTextComponent::FontFamilyRefSet fontFamilyRefs;
                BuildDrawBatchesAndAssignClickableIds(drawBatches, fontFamilyRefs, inlineImages, defaultImageHeight, defaultAscent, batchStack, fontFamilyStack, &markupRoot);
                AZ_Assert(1 == fontFamilyRefs.size(), "Test failed");
                AZ_Assert(2 == drawBatches.size(), "Test failed");
                AssertTextNotEmpty(drawBatches);

                StringList stringList;
                stringList.push_back("this");
                stringList.push_back(" is a test!");
                AssertDrawBatchTextContent(drawBatches, stringList);

                FontFamilyPtr notoSansFamily = gEnv->pCryFont->GetFontFamily("notosans");
                AZ_Assert(notoSansFamily, "Test failed");

                FontList fontList;
                fontList.push_back(notoSansFamily->normal);
                fontList.push_back(fontFamily->normal);
                AssertDrawBatchFontOrder(drawBatches, fontList);

                ColorList colorList;
                colorList.push_back(TextMarkup::ColorInvalid);
                colorList.push_back(TextMarkup::ColorInvalid);
                AssertDrawBatchMultiColor(drawBatches, colorList);
            }
        }

        // Font tag: font face ("pass-through" font)
        {
            const LyShine::StringType markupTestString("<font face=\"default-ui\">this</font> is a test!");

            {
                TextMarkup::Tag markupRoot;
                AZ_Assert(TextMarkup::ParseMarkupBuffer(markupTestString, markupRoot), "Test failed");
                AZStd::list<UiTextComponent::DrawBatch> drawBatches;
                AZStd::stack<UiTextComponent::DrawBatch> batchStack;

                AZStd::stack<FontFamily*> fontFamilyStack;
                fontFamilyStack.push(fontFamily);

                UiTextComponent::FontFamilyRefSet fontFamilyRefs;
                BuildDrawBatchesAndAssignClickableIds(drawBatches, fontFamilyRefs, inlineImages, defaultImageHeight, defaultAscent, batchStack, fontFamilyStack, &markupRoot);
                AZ_Assert(1 == fontFamilyRefs.size(), "Test failed");
                AZ_Assert(2 == drawBatches.size(), "Test failed");
                AssertTextNotEmpty(drawBatches);

                StringList stringList;
                stringList.push_back("this");
                stringList.push_back(" is a test!");
                AssertDrawBatchTextContent(drawBatches, stringList);

                FontFamilyPtr defaultUiFamily = gEnv->pCryFont->GetFontFamily("default-ui");
                AZ_Assert(defaultUiFamily, "Test failed");

                FontList fontList;
                fontList.push_back(defaultUiFamily->normal);
                fontList.push_back(fontFamily->normal);
                AssertDrawBatchFontOrder(drawBatches, fontList);

                ColorList colorList;
                colorList.push_back(TextMarkup::ColorInvalid);
                colorList.push_back(TextMarkup::ColorInvalid);
                AssertDrawBatchMultiColor(drawBatches, colorList);
            }
        }

        // Font tag: font face (invalid font)
        {
            const LyShine::StringType markupTestString("<font face=\"invalidFontName\">this</font> is a test!");

            {
                TextMarkup::Tag markupRoot;
                AZ_Assert(TextMarkup::ParseMarkupBuffer(markupTestString, markupRoot), "Test failed");
                AZStd::list<UiTextComponent::DrawBatch> drawBatches;
                AZStd::stack<UiTextComponent::DrawBatch> batchStack;

                AZStd::stack<FontFamily*> fontFamilyStack;
                fontFamilyStack.push(fontFamily);

                UiTextComponent::FontFamilyRefSet fontFamilyRefs;
                BuildDrawBatchesAndAssignClickableIds(drawBatches, fontFamilyRefs, inlineImages, defaultImageHeight, defaultAscent, batchStack, fontFamilyStack, &markupRoot);
                AZ_Assert(0 == fontFamilyRefs.size(), "Test failed");
                AZ_Assert(2 == drawBatches.size(), "Test failed");
                AssertTextNotEmpty(drawBatches);

                StringList stringList;
                stringList.push_back("this");
                stringList.push_back(" is a test!");
                AssertDrawBatchTextContent(drawBatches, stringList);

                FontList fontList;
                fontList.push_back(fontFamily->normal);
                fontList.push_back(fontFamily->normal);
                AssertDrawBatchFontOrder(drawBatches, fontList);

                ColorList colorList;
                colorList.push_back(TextMarkup::ColorInvalid);
                colorList.push_back(TextMarkup::ColorInvalid);
                AssertDrawBatchMultiColor(drawBatches, colorList);
            }
        }

        // Font tag: font face (invalid empty string)
        {
            const LyShine::StringType markupTestString("<font face=\"\">this</font> is a test!");

            {
                TextMarkup::Tag markupRoot;
                AZ_Assert(TextMarkup::ParseMarkupBuffer(markupTestString, markupRoot), "Test failed");
                AZStd::list<UiTextComponent::DrawBatch> drawBatches;
                AZStd::stack<UiTextComponent::DrawBatch> batchStack;

                AZStd::stack<FontFamily*> fontFamilyStack;
                fontFamilyStack.push(fontFamily);

                UiTextComponent::FontFamilyRefSet fontFamilyRefs;
                BuildDrawBatchesAndAssignClickableIds(drawBatches, fontFamilyRefs, inlineImages, defaultImageHeight, defaultAscent, batchStack, fontFamilyStack, &markupRoot);
                AZ_Assert(0 == fontFamilyRefs.size(), "Test failed");
                AZ_Assert(2 == drawBatches.size(), "Test failed");
                AssertTextNotEmpty(drawBatches);

                StringList stringList;
                stringList.push_back("this");
                stringList.push_back(" is a test!");
                AssertDrawBatchTextContent(drawBatches, stringList);

                FontList fontList;
                fontList.push_back(fontFamily->normal);
                fontList.push_back(fontFamily->normal);
                AssertDrawBatchFontOrder(drawBatches, fontList);

                ColorList colorList;
                colorList.push_back(TextMarkup::ColorInvalid);
                colorList.push_back(TextMarkup::ColorInvalid);
                AssertDrawBatchMultiColor(drawBatches, colorList);
            }
        }

        // Font tag: font color (red, lower case)
        {
            const LyShine::StringType markupTestString("<font color=\"#ff0000\">this</font> is a test!");

            {
                TextMarkup::Tag markupRoot;
                AZ_Assert(TextMarkup::ParseMarkupBuffer(markupTestString, markupRoot), "Test failed");
                AZStd::list<UiTextComponent::DrawBatch> drawBatches;
                AZStd::stack<UiTextComponent::DrawBatch> batchStack;

                AZStd::stack<FontFamily*> fontFamilyStack;
                fontFamilyStack.push(fontFamily);

                UiTextComponent::FontFamilyRefSet fontFamilyRefs;
                BuildDrawBatchesAndAssignClickableIds(drawBatches, fontFamilyRefs, inlineImages, defaultImageHeight, defaultAscent, batchStack, fontFamilyStack, &markupRoot);
                AZ_Assert(0 == fontFamilyRefs.size(), "Test failed");
                AZ_Assert(2 == drawBatches.size(), "Test failed");
                AssertTextNotEmpty(drawBatches);

                StringList stringList;
                stringList.push_back("this");
                stringList.push_back(" is a test!");
                AssertDrawBatchTextContent(drawBatches, stringList);

                FontList fontList;
                fontList.push_back(fontFamily->normal);
                fontList.push_back(fontFamily->normal);
                AssertDrawBatchFontOrder(drawBatches, fontList);

                ColorList colorList;
                colorList.push_back(AZ::Vector3(1.0f, 0.0f, 0.0f));
                colorList.push_back(TextMarkup::ColorInvalid);
                AssertDrawBatchMultiColor(drawBatches, colorList);
            }
        }

        // Font tag: font color (red, upper case)
        {
            const LyShine::StringType markupTestString("<font color=\"#FF0000\">this</font> is a test!");

            {
                TextMarkup::Tag markupRoot;
                AZ_Assert(TextMarkup::ParseMarkupBuffer(markupTestString, markupRoot), "Test failed");
                AZStd::list<UiTextComponent::DrawBatch> drawBatches;
                AZStd::stack<UiTextComponent::DrawBatch> batchStack;

                AZStd::stack<FontFamily*> fontFamilyStack;
                fontFamilyStack.push(fontFamily);

                UiTextComponent::FontFamilyRefSet fontFamilyRefs;
                BuildDrawBatchesAndAssignClickableIds(drawBatches, fontFamilyRefs, inlineImages, defaultImageHeight, defaultAscent, batchStack, fontFamilyStack, &markupRoot);
                AZ_Assert(0 == fontFamilyRefs.size(), "Test failed");
                AZ_Assert(2 == drawBatches.size(), "Test failed");
                AssertTextNotEmpty(drawBatches);

                StringList stringList;
                stringList.push_back("this");
                stringList.push_back(" is a test!");
                AssertDrawBatchTextContent(drawBatches, stringList);

                FontList fontList;
                fontList.push_back(fontFamily->normal);
                fontList.push_back(fontFamily->normal);
                AssertDrawBatchFontOrder(drawBatches, fontList);

                ColorList colorList;
                colorList.push_back(AZ::Vector3(1.0f, 0.0f, 0.0f));
                colorList.push_back(TextMarkup::ColorInvalid);
                AssertDrawBatchMultiColor(drawBatches, colorList);
            }
        }

        // Font tag: font color (red, mixed case) 1
        {
            const LyShine::StringType markupTestString("<font color=\"#fF0000\">this</font> is a test!");

            {
                TextMarkup::Tag markupRoot;
                AZ_Assert(TextMarkup::ParseMarkupBuffer(markupTestString, markupRoot), "Test failed");
                AZStd::list<UiTextComponent::DrawBatch> drawBatches;
                AZStd::stack<UiTextComponent::DrawBatch> batchStack;

                AZStd::stack<FontFamily*> fontFamilyStack;
                fontFamilyStack.push(fontFamily);

                UiTextComponent::FontFamilyRefSet fontFamilyRefs;
                BuildDrawBatchesAndAssignClickableIds(drawBatches, fontFamilyRefs, inlineImages, defaultImageHeight, defaultAscent, batchStack, fontFamilyStack, &markupRoot);
                AZ_Assert(0 == fontFamilyRefs.size(), "Test failed");
                AZ_Assert(2 == drawBatches.size(), "Test failed");
                AssertTextNotEmpty(drawBatches);

                StringList stringList;
                stringList.push_back("this");
                stringList.push_back(" is a test!");
                AssertDrawBatchTextContent(drawBatches, stringList);

                FontList fontList;
                fontList.push_back(fontFamily->normal);
                fontList.push_back(fontFamily->normal);
                AssertDrawBatchFontOrder(drawBatches, fontList);

                ColorList colorList;
                colorList.push_back(AZ::Vector3(1.0f, 0.0f, 0.0f));
                colorList.push_back(TextMarkup::ColorInvalid);
                AssertDrawBatchMultiColor(drawBatches, colorList);
            }
        }

        // Font tag: font color (red, mixed case) 2
        {
            const LyShine::StringType markupTestString("<font color=\"#Ff0000\">this</font> is a test!");

            {
                TextMarkup::Tag markupRoot;
                AZ_Assert(TextMarkup::ParseMarkupBuffer(markupTestString, markupRoot), "Test failed");
                AZStd::list<UiTextComponent::DrawBatch> drawBatches;
                AZStd::stack<UiTextComponent::DrawBatch> batchStack;

                AZStd::stack<FontFamily*> fontFamilyStack;
                fontFamilyStack.push(fontFamily);

                UiTextComponent::FontFamilyRefSet fontFamilyRefs;
                BuildDrawBatchesAndAssignClickableIds(drawBatches, fontFamilyRefs, inlineImages, defaultImageHeight, defaultAscent, batchStack, fontFamilyStack, &markupRoot);
                AZ_Assert(0 == fontFamilyRefs.size(), "Test failed");
                AZ_Assert(2 == drawBatches.size(), "Test failed");
                AssertTextNotEmpty(drawBatches);

                StringList stringList;
                stringList.push_back("this");
                stringList.push_back(" is a test!");
                AssertDrawBatchTextContent(drawBatches, stringList);

                FontList fontList;
                fontList.push_back(fontFamily->normal);
                fontList.push_back(fontFamily->normal);
                AssertDrawBatchFontOrder(drawBatches, fontList);

                ColorList colorList;
                colorList.push_back(AZ::Vector3(1.0f, 0.0f, 0.0f));
                colorList.push_back(TextMarkup::ColorInvalid);
                AssertDrawBatchMultiColor(drawBatches, colorList);
            }
        }

        // Font tag: font color (red, upper case, leading space)
        {
            const LyShine::StringType markupTestString("<font color=\"   #FF0000\">this</font> is a test!");

            {
                TextMarkup::Tag markupRoot;
                AZ_Assert(TextMarkup::ParseMarkupBuffer(markupTestString, markupRoot), "Test failed");
                AZStd::list<UiTextComponent::DrawBatch> drawBatches;
                AZStd::stack<UiTextComponent::DrawBatch> batchStack;

                AZStd::stack<FontFamily*> fontFamilyStack;
                fontFamilyStack.push(fontFamily);

                UiTextComponent::FontFamilyRefSet fontFamilyRefs;
                BuildDrawBatchesAndAssignClickableIds(drawBatches, fontFamilyRefs, inlineImages, defaultImageHeight, defaultAscent, batchStack, fontFamilyStack, &markupRoot);
                AZ_Assert(0 == fontFamilyRefs.size(), "Test failed");
                AZ_Assert(2 == drawBatches.size(), "Test failed");
                AssertTextNotEmpty(drawBatches);

                StringList stringList;
                stringList.push_back("this");
                stringList.push_back(" is a test!");
                AssertDrawBatchTextContent(drawBatches, stringList);

                FontList fontList;
                fontList.push_back(fontFamily->normal);
                fontList.push_back(fontFamily->normal);
                AssertDrawBatchFontOrder(drawBatches, fontList);

                ColorList colorList;
                colorList.push_back(AZ::Vector3(1.0f, 0.0f, 0.0f));
                colorList.push_back(TextMarkup::ColorInvalid);
                AssertDrawBatchMultiColor(drawBatches, colorList);
            }
        }

        // Font tag: font color (red, upper case, trailing space)
        {
            const LyShine::StringType markupTestString("<font color=\"#FF0000   \">this</font> is a test!");

            {
                TextMarkup::Tag markupRoot;
                AZ_Assert(TextMarkup::ParseMarkupBuffer(markupTestString, markupRoot), "Test failed");
                AZStd::list<UiTextComponent::DrawBatch> drawBatches;
                AZStd::stack<UiTextComponent::DrawBatch> batchStack;

                AZStd::stack<FontFamily*> fontFamilyStack;
                fontFamilyStack.push(fontFamily);

                UiTextComponent::FontFamilyRefSet fontFamilyRefs;
                BuildDrawBatchesAndAssignClickableIds(drawBatches, fontFamilyRefs, inlineImages, defaultImageHeight, defaultAscent, batchStack, fontFamilyStack, &markupRoot);
                AZ_Assert(0 == fontFamilyRefs.size(), "Test failed");
                AZ_Assert(2 == drawBatches.size(), "Test failed");
                AssertTextNotEmpty(drawBatches);

                StringList stringList;
                stringList.push_back("this");
                stringList.push_back(" is a test!");
                AssertDrawBatchTextContent(drawBatches, stringList);

                FontList fontList;
                fontList.push_back(fontFamily->normal);
                fontList.push_back(fontFamily->normal);
                AssertDrawBatchFontOrder(drawBatches, fontList);

                ColorList colorList;
                colorList.push_back(AZ::Vector3(1.0f, 0.0f, 0.0f));
                colorList.push_back(TextMarkup::ColorInvalid);
                AssertDrawBatchMultiColor(drawBatches, colorList);
            }
        }

        // Font tag: font color (red, upper case, leading and trailing space)
        {
            const LyShine::StringType markupTestString("<font color=\"   #FF0000   \">this</font> is a test!");

            {
                TextMarkup::Tag markupRoot;
                AZ_Assert(TextMarkup::ParseMarkupBuffer(markupTestString, markupRoot), "Test failed");
                AZStd::list<UiTextComponent::DrawBatch> drawBatches;
                AZStd::stack<UiTextComponent::DrawBatch> batchStack;

                AZStd::stack<FontFamily*> fontFamilyStack;
                fontFamilyStack.push(fontFamily);

                UiTextComponent::FontFamilyRefSet fontFamilyRefs;
                BuildDrawBatchesAndAssignClickableIds(drawBatches, fontFamilyRefs, inlineImages, defaultImageHeight, defaultAscent, batchStack, fontFamilyStack, &markupRoot);
                AZ_Assert(0 == fontFamilyRefs.size(), "Test failed");
                AZ_Assert(2 == drawBatches.size(), "Test failed");
                AssertTextNotEmpty(drawBatches);

                StringList stringList;
                stringList.push_back("this");
                stringList.push_back(" is a test!");
                AssertDrawBatchTextContent(drawBatches, stringList);

                FontList fontList;
                fontList.push_back(fontFamily->normal);
                fontList.push_back(fontFamily->normal);
                AssertDrawBatchFontOrder(drawBatches, fontList);

                ColorList colorList;
                colorList.push_back(AZ::Vector3(1.0f, 0.0f, 0.0f));
                colorList.push_back(TextMarkup::ColorInvalid);
                AssertDrawBatchMultiColor(drawBatches, colorList);
            }
        }

        // Font tag: font color (green, upper case)
        {
            const LyShine::StringType markupTestString("<font color=\"#00FF00\">this</font> is a test!");

            {
                TextMarkup::Tag markupRoot;
                AZ_Assert(TextMarkup::ParseMarkupBuffer(markupTestString, markupRoot), "Test failed");
                AZStd::list<UiTextComponent::DrawBatch> drawBatches;
                AZStd::stack<UiTextComponent::DrawBatch> batchStack;

                AZStd::stack<FontFamily*> fontFamilyStack;
                fontFamilyStack.push(fontFamily);

                UiTextComponent::FontFamilyRefSet fontFamilyRefs;
                BuildDrawBatchesAndAssignClickableIds(drawBatches, fontFamilyRefs, inlineImages, defaultImageHeight, defaultAscent, batchStack, fontFamilyStack, &markupRoot);
                AZ_Assert(0 == fontFamilyRefs.size(), "Test failed");
                AZ_Assert(2 == drawBatches.size(), "Test failed");
                AssertTextNotEmpty(drawBatches);

                StringList stringList;
                stringList.push_back("this");
                stringList.push_back(" is a test!");
                AssertDrawBatchTextContent(drawBatches, stringList);

                FontList fontList;
                fontList.push_back(fontFamily->normal);
                fontList.push_back(fontFamily->normal);
                AssertDrawBatchFontOrder(drawBatches, fontList);

                ColorList colorList;
                colorList.push_back(AZ::Vector3(0.0f, 1.0f, 0.0f));
                colorList.push_back(TextMarkup::ColorInvalid);
                AssertDrawBatchMultiColor(drawBatches, colorList);
            }
        }

        // Font tag: font color (blue, upper case)
        {
            const LyShine::StringType markupTestString("<font color=\"#0000FF\">this</font> is a test!");

            {
                TextMarkup::Tag markupRoot;
                AZ_Assert(TextMarkup::ParseMarkupBuffer(markupTestString, markupRoot), "Test failed");
                AZStd::list<UiTextComponent::DrawBatch> drawBatches;
                AZStd::stack<UiTextComponent::DrawBatch> batchStack;

                AZStd::stack<FontFamily*> fontFamilyStack;
                fontFamilyStack.push(fontFamily);

                UiTextComponent::FontFamilyRefSet fontFamilyRefs;
                BuildDrawBatchesAndAssignClickableIds(drawBatches, fontFamilyRefs, inlineImages, defaultImageHeight, defaultAscent, batchStack, fontFamilyStack, &markupRoot);
                AZ_Assert(0 == fontFamilyRefs.size(), "Test failed");
                AZ_Assert(2 == drawBatches.size(), "Test failed");
                AssertTextNotEmpty(drawBatches);

                StringList stringList;
                stringList.push_back("this");
                stringList.push_back(" is a test!");
                AssertDrawBatchTextContent(drawBatches, stringList);

                FontList fontList;
                fontList.push_back(fontFamily->normal);
                fontList.push_back(fontFamily->normal);
                AssertDrawBatchFontOrder(drawBatches, fontList);

                ColorList colorList;
                colorList.push_back(AZ::Vector3(0.0f, 0.0f, 1.0f));
                colorList.push_back(TextMarkup::ColorInvalid);
                AssertDrawBatchMultiColor(drawBatches, colorList);
            }
        }

        // Font tag: font color (invalid hex value)
        {
            const LyShine::StringType markupTestString("<font color=\"#GGGGGG\">this</font> is a test!");

            {
                TextMarkup::Tag markupRoot;
                AZ_Assert(TextMarkup::ParseMarkupBuffer(markupTestString, markupRoot), "Test failed");
                AZStd::list<UiTextComponent::DrawBatch> drawBatches;
                AZStd::stack<UiTextComponent::DrawBatch> batchStack;

                AZStd::stack<FontFamily*> fontFamilyStack;
                fontFamilyStack.push(fontFamily);

                UiTextComponent::FontFamilyRefSet fontFamilyRefs;
                BuildDrawBatchesAndAssignClickableIds(drawBatches, fontFamilyRefs, inlineImages, defaultImageHeight, defaultAscent, batchStack, fontFamilyStack, &markupRoot);
                AZ_Assert(0 == fontFamilyRefs.size(), "Test failed");
                AZ_Assert(2 == drawBatches.size(), "Test failed");
                AssertTextNotEmpty(drawBatches);

                StringList stringList;
                stringList.push_back("this");
                stringList.push_back(" is a test!");
                AssertDrawBatchTextContent(drawBatches, stringList);

                FontList fontList;
                fontList.push_back(fontFamily->normal);
                fontList.push_back(fontFamily->normal);
                AssertDrawBatchFontOrder(drawBatches, fontList);

                ColorList colorList;
                colorList.push_back(AZ::Vector3(0.0f, 0.0f, 0.0f));
                colorList.push_back(TextMarkup::ColorInvalid);
                AssertDrawBatchMultiColor(drawBatches, colorList);
            }
        }

        // Font tag: font color (invalid hex value)
        {
            const LyShine::StringType markupTestString("<font color=\"#FF\">this</font> is a test!");

            {
                TextMarkup::Tag markupRoot;
                AZ_Assert(TextMarkup::ParseMarkupBuffer(markupTestString, markupRoot), "Test failed");
                AZStd::list<UiTextComponent::DrawBatch> drawBatches;
                AZStd::stack<UiTextComponent::DrawBatch> batchStack;

                AZStd::stack<FontFamily*> fontFamilyStack;
                fontFamilyStack.push(fontFamily);

                UiTextComponent::FontFamilyRefSet fontFamilyRefs;
                BuildDrawBatchesAndAssignClickableIds(drawBatches, fontFamilyRefs, inlineImages, defaultImageHeight, defaultAscent, batchStack, fontFamilyStack, &markupRoot);
                AZ_Assert(0 == fontFamilyRefs.size(), "Test failed");
                AZ_Assert(2 == drawBatches.size(), "Test failed");
                AssertTextNotEmpty(drawBatches);

                StringList stringList;
                stringList.push_back("this");
                stringList.push_back(" is a test!");
                AssertDrawBatchTextContent(drawBatches, stringList);

                FontList fontList;
                fontList.push_back(fontFamily->normal);
                fontList.push_back(fontFamily->normal);
                AssertDrawBatchFontOrder(drawBatches, fontList);

                ColorList colorList;
                colorList.push_back(TextMarkup::ColorInvalid);
                colorList.push_back(TextMarkup::ColorInvalid);
                AssertDrawBatchMultiColor(drawBatches, colorList);
            }
        }

        // Font tag: font color (invalid formatting)
        {
            const LyShine::StringType markupTestString("<font color=\"FF0000\">this</font> is a test!");

            {
                TextMarkup::Tag markupRoot;
                AZ_Assert(TextMarkup::ParseMarkupBuffer(markupTestString, markupRoot), "Test failed");
                AZStd::list<UiTextComponent::DrawBatch> drawBatches;
                AZStd::stack<UiTextComponent::DrawBatch> batchStack;

                AZStd::stack<FontFamily*> fontFamilyStack;
                fontFamilyStack.push(fontFamily);

                UiTextComponent::FontFamilyRefSet fontFamilyRefs;
                BuildDrawBatchesAndAssignClickableIds(drawBatches, fontFamilyRefs, inlineImages, defaultImageHeight, defaultAscent, batchStack, fontFamilyStack, &markupRoot);
                AZ_Assert(0 == fontFamilyRefs.size(), "Test failed");
                AZ_Assert(2 == drawBatches.size(), "Test failed");
                AssertTextNotEmpty(drawBatches);

                StringList stringList;
                stringList.push_back("this");
                stringList.push_back(" is a test!");
                AssertDrawBatchTextContent(drawBatches, stringList);

                FontList fontList;
                fontList.push_back(fontFamily->normal);
                fontList.push_back(fontFamily->normal);
                AssertDrawBatchFontOrder(drawBatches, fontList);

                ColorList colorList;
                colorList.push_back(TextMarkup::ColorInvalid);
                colorList.push_back(TextMarkup::ColorInvalid);
                AssertDrawBatchMultiColor(drawBatches, colorList);
            }
        }

        // Font tag: font color (invalid formatting)
        {
            const LyShine::StringType markupTestString("<font color=\"gluten\">this</font> is a test!");

            {
                TextMarkup::Tag markupRoot;
                AZ_Assert(TextMarkup::ParseMarkupBuffer(markupTestString, markupRoot), "Test failed");
                AZStd::list<UiTextComponent::DrawBatch> drawBatches;
                AZStd::stack<UiTextComponent::DrawBatch> batchStack;

                AZStd::stack<FontFamily*> fontFamilyStack;
                fontFamilyStack.push(fontFamily);

                UiTextComponent::FontFamilyRefSet fontFamilyRefs;
                BuildDrawBatchesAndAssignClickableIds(drawBatches, fontFamilyRefs, inlineImages, defaultImageHeight, defaultAscent, batchStack, fontFamilyStack, &markupRoot);
                AZ_Assert(0 == fontFamilyRefs.size(), "Test failed");
                AZ_Assert(2 == drawBatches.size(), "Test failed");
                AssertTextNotEmpty(drawBatches);

                StringList stringList;
                stringList.push_back("this");
                stringList.push_back(" is a test!");
                AssertDrawBatchTextContent(drawBatches, stringList);

                FontList fontList;
                fontList.push_back(fontFamily->normal);
                fontList.push_back(fontFamily->normal);
                AssertDrawBatchFontOrder(drawBatches, fontList);

                ColorList colorList;
                colorList.push_back(TextMarkup::ColorInvalid);
                colorList.push_back(TextMarkup::ColorInvalid);
                AssertDrawBatchMultiColor(drawBatches, colorList);
            }
        }

        // Font tag: font color (invalid value, empty string)
        {
            const LyShine::StringType markupTestString("<font color=\"\">this</font> is a test!");

            {
                TextMarkup::Tag markupRoot;
                AZ_Assert(TextMarkup::ParseMarkupBuffer(markupTestString, markupRoot), "Test failed");
                AZStd::list<UiTextComponent::DrawBatch> drawBatches;
                AZStd::stack<UiTextComponent::DrawBatch> batchStack;

                AZStd::stack<FontFamily*> fontFamilyStack;
                fontFamilyStack.push(fontFamily);

                UiTextComponent::FontFamilyRefSet fontFamilyRefs;
                BuildDrawBatchesAndAssignClickableIds(drawBatches, fontFamilyRefs, inlineImages, defaultImageHeight, defaultAscent, batchStack, fontFamilyStack, &markupRoot);
                AZ_Assert(0 == fontFamilyRefs.size(), "Test failed");
                AZ_Assert(2 == drawBatches.size(), "Test failed");
                AssertTextNotEmpty(drawBatches);

                StringList stringList;
                stringList.push_back("this");
                stringList.push_back(" is a test!");
                AssertDrawBatchTextContent(drawBatches, stringList);

                FontList fontList;
                fontList.push_back(fontFamily->normal);
                fontList.push_back(fontFamily->normal);
                AssertDrawBatchFontOrder(drawBatches, fontList);

                ColorList colorList;
                colorList.push_back(TextMarkup::ColorInvalid);
                colorList.push_back(TextMarkup::ColorInvalid);
                AssertDrawBatchMultiColor(drawBatches, colorList);
            }
        }

        // Font tag: font color (invalid value, empty string, spaces)
        {
            const LyShine::StringType markupTestString("<font color=\"   \">this</font> is a test!");

            {
                TextMarkup::Tag markupRoot;
                AZ_Assert(TextMarkup::ParseMarkupBuffer(markupTestString, markupRoot), "Test failed");
                AZStd::list<UiTextComponent::DrawBatch> drawBatches;
                AZStd::stack<UiTextComponent::DrawBatch> batchStack;

                AZStd::stack<FontFamily*> fontFamilyStack;
                fontFamilyStack.push(fontFamily);

                UiTextComponent::FontFamilyRefSet fontFamilyRefs;
                BuildDrawBatchesAndAssignClickableIds(drawBatches, fontFamilyRefs, inlineImages, defaultImageHeight, defaultAscent, batchStack, fontFamilyStack, &markupRoot);
                AZ_Assert(0 == fontFamilyRefs.size(), "Test failed");
                AZ_Assert(2 == drawBatches.size(), "Test failed");
                AssertTextNotEmpty(drawBatches);

                StringList stringList;
                stringList.push_back("this");
                stringList.push_back(" is a test!");
                AssertDrawBatchTextContent(drawBatches, stringList);

                FontList fontList;
                fontList.push_back(fontFamily->normal);
                fontList.push_back(fontFamily->normal);
                AssertDrawBatchFontOrder(drawBatches, fontList);

                ColorList colorList;
                colorList.push_back(TextMarkup::ColorInvalid);
                colorList.push_back(TextMarkup::ColorInvalid);
                AssertDrawBatchMultiColor(drawBatches, colorList);
            }
        }

        // Font tag: font color (invalid value, leading hash, empty following)
        {
            const LyShine::StringType markupTestString("<font color=\"#\">this</font> is a test!");

            {
                TextMarkup::Tag markupRoot;
                AZ_Assert(TextMarkup::ParseMarkupBuffer(markupTestString, markupRoot), "Test failed");
                AZStd::list<UiTextComponent::DrawBatch> drawBatches;
                AZStd::stack<UiTextComponent::DrawBatch> batchStack;

                AZStd::stack<FontFamily*> fontFamilyStack;
                fontFamilyStack.push(fontFamily);

                UiTextComponent::FontFamilyRefSet fontFamilyRefs;
                BuildDrawBatchesAndAssignClickableIds(drawBatches, fontFamilyRefs, inlineImages, defaultImageHeight, defaultAscent, batchStack, fontFamilyStack, &markupRoot);
                AZ_Assert(0 == fontFamilyRefs.size(), "Test failed");
                AZ_Assert(2 == drawBatches.size(), "Test failed");
                AssertTextNotEmpty(drawBatches);

                StringList stringList;
                stringList.push_back("this");
                stringList.push_back(" is a test!");
                AssertDrawBatchTextContent(drawBatches, stringList);

                FontList fontList;
                fontList.push_back(fontFamily->normal);
                fontList.push_back(fontFamily->normal);
                AssertDrawBatchFontOrder(drawBatches, fontList);

                ColorList colorList;
                colorList.push_back(TextMarkup::ColorInvalid);
                colorList.push_back(TextMarkup::ColorInvalid);
                AssertDrawBatchMultiColor(drawBatches, colorList);
            }
        }

        // Font tag: font color (invalid value, leading spaces with hash)
        {
            const LyShine::StringType markupTestString("<font color=\"  #\">this</font> is a test!");

            {
                TextMarkup::Tag markupRoot;
                AZ_Assert(TextMarkup::ParseMarkupBuffer(markupTestString, markupRoot), "Test failed");
                AZStd::list<UiTextComponent::DrawBatch> drawBatches;
                AZStd::stack<UiTextComponent::DrawBatch> batchStack;

                AZStd::stack<FontFamily*> fontFamilyStack;
                fontFamilyStack.push(fontFamily);

                UiTextComponent::FontFamilyRefSet fontFamilyRefs;
                BuildDrawBatchesAndAssignClickableIds(drawBatches, fontFamilyRefs, inlineImages, defaultImageHeight, defaultAscent, batchStack, fontFamilyStack, &markupRoot);
                AZ_Assert(0 == fontFamilyRefs.size(), "Test failed");
                AZ_Assert(2 == drawBatches.size(), "Test failed");
                AssertTextNotEmpty(drawBatches);

                StringList stringList;
                stringList.push_back("this");
                stringList.push_back(" is a test!");
                AssertDrawBatchTextContent(drawBatches, stringList);

                FontList fontList;
                fontList.push_back(fontFamily->normal);
                fontList.push_back(fontFamily->normal);
                AssertDrawBatchFontOrder(drawBatches, fontList);

                ColorList colorList;
                colorList.push_back(TextMarkup::ColorInvalid);
                colorList.push_back(TextMarkup::ColorInvalid);
                AssertDrawBatchMultiColor(drawBatches, colorList);
            }
        }

        // Font tag: font color (invalid value, trailing spaces with hash)
        {
            const LyShine::StringType markupTestString("<font color=\"#  \">this</font> is a test!");

            {
                TextMarkup::Tag markupRoot;
                AZ_Assert(TextMarkup::ParseMarkupBuffer(markupTestString, markupRoot), "Test failed");
                AZStd::list<UiTextComponent::DrawBatch> drawBatches;
                AZStd::stack<UiTextComponent::DrawBatch> batchStack;

                AZStd::stack<FontFamily*> fontFamilyStack;
                fontFamilyStack.push(fontFamily);

                UiTextComponent::FontFamilyRefSet fontFamilyRefs;
                BuildDrawBatchesAndAssignClickableIds(drawBatches, fontFamilyRefs, inlineImages, defaultImageHeight, defaultAscent, batchStack, fontFamilyStack, &markupRoot);
                AZ_Assert(0 == fontFamilyRefs.size(), "Test failed");
                AZ_Assert(2 == drawBatches.size(), "Test failed");
                AssertTextNotEmpty(drawBatches);

                StringList stringList;
                stringList.push_back("this");
                stringList.push_back(" is a test!");
                AssertDrawBatchTextContent(drawBatches, stringList);

                FontList fontList;
                fontList.push_back(fontFamily->normal);
                fontList.push_back(fontFamily->normal);
                AssertDrawBatchFontOrder(drawBatches, fontList);

                ColorList colorList;
                colorList.push_back(TextMarkup::ColorInvalid);
                colorList.push_back(TextMarkup::ColorInvalid);
                AssertDrawBatchMultiColor(drawBatches, colorList);
            }
        }

        // Font tag: font color (invalid value, leading and trailing spaces with hash)
        {
            const LyShine::StringType markupTestString("<font color=\"  #  \">this</font> is a test!");

            {
                TextMarkup::Tag markupRoot;
                AZ_Assert(TextMarkup::ParseMarkupBuffer(markupTestString, markupRoot), "Test failed");
                AZStd::list<UiTextComponent::DrawBatch> drawBatches;
                AZStd::stack<UiTextComponent::DrawBatch> batchStack;

                AZStd::stack<FontFamily*> fontFamilyStack;
                fontFamilyStack.push(fontFamily);

                UiTextComponent::FontFamilyRefSet fontFamilyRefs;
                BuildDrawBatchesAndAssignClickableIds(drawBatches, fontFamilyRefs, inlineImages, defaultImageHeight, defaultAscent, batchStack, fontFamilyStack, &markupRoot);
                AZ_Assert(0 == fontFamilyRefs.size(), "Test failed");
                AZ_Assert(2 == drawBatches.size(), "Test failed");
                AssertTextNotEmpty(drawBatches);

                StringList stringList;
                stringList.push_back("this");
                stringList.push_back(" is a test!");
                AssertDrawBatchTextContent(drawBatches, stringList);

                FontList fontList;
                fontList.push_back(fontFamily->normal);
                fontList.push_back(fontFamily->normal);
                AssertDrawBatchFontOrder(drawBatches, fontList);

                ColorList colorList;
                colorList.push_back(TextMarkup::ColorInvalid);
                colorList.push_back(TextMarkup::ColorInvalid);
                AssertDrawBatchMultiColor(drawBatches, colorList);
            }
        }

        // Font tag: font face and color
        {
            const LyShine::StringType markupTestString("<font face=\"notoserif\" color=\"#FF0000\">this</font> is a test!");

            {
                TextMarkup::Tag markupRoot;
                AZ_Assert(TextMarkup::ParseMarkupBuffer(markupTestString, markupRoot), "Test failed");
                AZStd::list<UiTextComponent::DrawBatch> drawBatches;
                AZStd::stack<UiTextComponent::DrawBatch> batchStack;

                AZStd::stack<FontFamily*> fontFamilyStack;
                fontFamilyStack.push(fontFamily);

                UiTextComponent::FontFamilyRefSet fontFamilyRefs;
                BuildDrawBatchesAndAssignClickableIds(drawBatches, fontFamilyRefs, inlineImages, defaultImageHeight, defaultAscent, batchStack, fontFamilyStack, &markupRoot);
                AZ_Assert(1 == fontFamilyRefs.size(), "Test failed");
                AZ_Assert(2 == drawBatches.size(), "Test failed");
                AssertTextNotEmpty(drawBatches);

                StringList stringList;
                stringList.push_back("this");
                stringList.push_back(" is a test!");
                AssertDrawBatchTextContent(drawBatches, stringList);

                FontFamilyPtr notoSerifFamily = gEnv->pCryFont->GetFontFamily("notoserif");
                AZ_Assert(notoSerifFamily, "Test failed");

                FontList fontList;
                fontList.push_back(notoSerifFamily->normal);
                fontList.push_back(fontFamily->normal);
                AssertDrawBatchFontOrder(drawBatches, fontList);

                ColorList colorList;
                colorList.push_back(AZ::Vector3(1.0f, 0.0f, 0.0f));
                colorList.push_back(TextMarkup::ColorInvalid);
                AssertDrawBatchMultiColor(drawBatches, colorList);
            }
        }

        // Font tag: font color and face
        {
            const LyShine::StringType markupTestString("<font color=\"#FF0000\" face=\"notoserif\">this</font> is a test!");

            {
                TextMarkup::Tag markupRoot;
                AZ_Assert(TextMarkup::ParseMarkupBuffer(markupTestString, markupRoot), "Test failed");
                AZStd::list<UiTextComponent::DrawBatch> drawBatches;
                AZStd::stack<UiTextComponent::DrawBatch> batchStack;

                AZStd::stack<FontFamily*> fontFamilyStack;
                fontFamilyStack.push(fontFamily);

                UiTextComponent::FontFamilyRefSet fontFamilyRefs;
                BuildDrawBatchesAndAssignClickableIds(drawBatches, fontFamilyRefs, inlineImages, defaultImageHeight, defaultAscent, batchStack, fontFamilyStack, &markupRoot);
                AZ_Assert(1 == fontFamilyRefs.size(), "Test failed");
                AZ_Assert(2 == drawBatches.size(), "Test failed");
                AssertTextNotEmpty(drawBatches);

                StringList stringList;
                stringList.push_back("this");
                stringList.push_back(" is a test!");
                AssertDrawBatchTextContent(drawBatches, stringList);

                FontFamilyPtr notoSerifFamily = gEnv->pCryFont->GetFontFamily("notoserif");
                AZ_Assert(notoSerifFamily, "Test failed");

                FontList fontList;
                fontList.push_back(notoSerifFamily->normal);
                fontList.push_back(fontFamily->normal);
                AssertDrawBatchFontOrder(drawBatches, fontList);

                ColorList colorList;
                colorList.push_back(AZ::Vector3(1.0f, 0.0f, 0.0f));
                colorList.push_back(TextMarkup::ColorInvalid);
                AssertDrawBatchMultiColor(drawBatches, colorList);
            }
        }

        // Font tag: invalid attribute
        {
            const LyShine::StringType markupTestString("<font cllor=\"#FF0000\">this</font> is a test!");

            {
                TextMarkup::Tag markupRoot;
                AZ_Assert(false == TextMarkup::ParseMarkupBuffer(markupTestString, markupRoot), "Test failed");
            }
        }

        // Mixed test: Bold, italic, bold-italic
        {
            const LyShine::StringType markupTestString("Regular <b>Bold</b> <i>Italic\n<b>Bold-Italic</b></i>");

            {
                TextMarkup::Tag markupRoot;
                AZ_Assert(TextMarkup::ParseMarkupBuffer(markupTestString, markupRoot), "Test failed");
                AZStd::list<UiTextComponent::DrawBatch> drawBatches;
                AZStd::stack<UiTextComponent::DrawBatch> batchStack;

                AZStd::stack<FontFamily*> fontFamilyStack;
                fontFamilyStack.push(fontFamily);

                UiTextComponent::FontFamilyRefSet fontFamilyRefs;
                BuildDrawBatchesAndAssignClickableIds(drawBatches, fontFamilyRefs, inlineImages, defaultImageHeight, defaultAscent, batchStack, fontFamilyStack, &markupRoot);
                AZ_Assert(0 == fontFamilyRefs.size(), "Test failed");
                AZ_Assert(5 == drawBatches.size(), "Test failed");
                AssertTextNotEmpty(drawBatches);

                StringList stringList;
                stringList.push_back("Regular ");
                stringList.push_back("Bold");
                stringList.push_back(" ");
                stringList.push_back("Italic\n");
                stringList.push_back("Bold-Italic");
                AssertDrawBatchTextContent(drawBatches, stringList);

                FontList fontList;
                fontList.push_back(fontFamily->normal);
                fontList.push_back(fontFamily->bold);
                fontList.push_back(fontFamily->normal);
                fontList.push_back(fontFamily->italic);
                fontList.push_back(fontFamily->boldItalic);
                AssertDrawBatchFontOrder(drawBatches, fontList);

                AssertDrawBatchSingleColor(drawBatches, TextMarkup::ColorInvalid);
            }
        }

        // Mixed test: Font color, font face, bold
        {
            const LyShine::StringType markupTestString("<font color=\"#00ff00\">Regular <font face=\"notoserif\"><b>Bold</b></font></font>");

            {
                TextMarkup::Tag markupRoot;
                AZ_Assert(TextMarkup::ParseMarkupBuffer(markupTestString, markupRoot), "Test failed");
                AZStd::list<UiTextComponent::DrawBatch> drawBatches;
                AZStd::stack<UiTextComponent::DrawBatch> batchStack;

                AZStd::stack<FontFamily*> fontFamilyStack;
                fontFamilyStack.push(fontFamily);

                UiTextComponent::FontFamilyRefSet fontFamilyRefs;
                BuildDrawBatchesAndAssignClickableIds(drawBatches, fontFamilyRefs, inlineImages, defaultImageHeight, defaultAscent, batchStack, fontFamilyStack, &markupRoot);
                AZ_Assert(1 == fontFamilyRefs.size(), "Test failed");
                AZ_Assert(2 == drawBatches.size(), "Test failed");
                AssertTextNotEmpty(drawBatches);

                StringList stringList;
                stringList.push_back("Regular ");
                stringList.push_back("Bold");
                AssertDrawBatchTextContent(drawBatches, stringList);

                FontFamilyPtr notoSerifFamily = gEnv->pCryFont->GetFontFamily("notoserif");
                AZ_Assert(notoSerifFamily, "Test failed");

                FontList fontList;
                fontList.push_back(fontFamily->normal);
                fontList.push_back(notoSerifFamily->bold);
                AssertDrawBatchFontOrder(drawBatches, fontList);

                ColorList colorList;
                colorList.push_back(AZ::Vector3(0.0f, 1.0f, 0.0f));
                colorList.push_back(AZ::Vector3(0.0f, 1.0f, 0.0f));
                AssertDrawBatchMultiColor(drawBatches, colorList);
            }
        }

        // Mixed test: Multiple font faces, color, bold
        {
            const LyShine::StringType markupTestString("<font color=\"#00ff00\">Regular </font><font face=\"notoserif\"><b>Bold</b></font> <i>Italic<b> Bold-Italic</b></i>\nHere is <font face=\"default-ui\">default-ui</font>");

            {
                TextMarkup::Tag markupRoot;
                AZ_Assert(TextMarkup::ParseMarkupBuffer(markupTestString, markupRoot), "Test failed");
                AZStd::list<UiTextComponent::DrawBatch> drawBatches;
                AZStd::stack<UiTextComponent::DrawBatch> batchStack;

                AZStd::stack<FontFamily*> fontFamilyStack;
                fontFamilyStack.push(fontFamily);

                UiTextComponent::FontFamilyRefSet fontFamilyRefs;
                BuildDrawBatchesAndAssignClickableIds(drawBatches, fontFamilyRefs, inlineImages, defaultImageHeight, defaultAscent, batchStack, fontFamilyStack, &markupRoot);
                AZ_Assert(2 == fontFamilyRefs.size(), "Test failed");
                AZ_Assert(7 == drawBatches.size(), "Test failed");
                AssertTextNotEmpty(drawBatches);

                StringList stringList;
                stringList.push_back("Regular ");
                stringList.push_back("Bold");
                stringList.push_back(" ");
                stringList.push_back("Italic");
                stringList.push_back(" Bold-Italic");
                stringList.push_back("\nHere is ");
                stringList.push_back("default-ui");
                AssertDrawBatchTextContent(drawBatches, stringList);

                FontFamilyPtr notoSerifFamily = gEnv->pCryFont->GetFontFamily("notoserif");
                AZ_Assert(notoSerifFamily, "Test failed");
                FontFamilyPtr defaultUiFamily = gEnv->pCryFont->GetFontFamily("default-ui");
                AZ_Assert(notoSerifFamily, "Test failed");

                FontList fontList;
                fontList.push_back(fontFamily->normal);
                fontList.push_back(notoSerifFamily->bold);
                fontList.push_back(fontFamily->normal);
                fontList.push_back(fontFamily->italic);
                fontList.push_back(fontFamily->boldItalic);
                fontList.push_back(fontFamily->normal);
                fontList.push_back(defaultUiFamily->normal);
                AssertDrawBatchFontOrder(drawBatches, fontList);

                ColorList colorList;
                colorList.push_back(AZ::Vector3(0.0f, 1.0f, 0.0f));
                colorList.push_back(TextMarkup::ColorInvalid);
                colorList.push_back(TextMarkup::ColorInvalid);
                colorList.push_back(TextMarkup::ColorInvalid);
                colorList.push_back(TextMarkup::ColorInvalid);
                colorList.push_back(TextMarkup::ColorInvalid);
                colorList.push_back(TextMarkup::ColorInvalid);
                AssertDrawBatchMultiColor(drawBatches, colorList);
            }
        }
    }

    using SizeList = AZStd::list < size_t >;

    void AssertBatchLineSizes(
        const UiTextComponent::DrawBatchLines& batchLines,
        const SizeList& batchSizes)
    {
        AZ_Assert(batchLines.batchLines.size() == batchSizes.size(), "Test failed");

        auto linesIt = batchLines.batchLines.begin();
        auto sizesIt = batchSizes.begin();
        for (;
            linesIt != batchLines.batchLines.end();
            ++linesIt, ++sizesIt)
        {
            const UiTextComponent::DrawBatchContainer& batchLine = (*linesIt).drawBatchList;
            const size_t batchSize = *sizesIt;
            AZ_Assert(batchLine.size() == batchSize, "Test failed");
        }
    }

    using DrawBatchLines = UiTextComponent::DrawBatchLines;
    using DrawBatchContainer = UiTextComponent::DrawBatchContainer;
    using DrawBatch = UiTextComponent::DrawBatch;

    void WrapTextTests(FontFamily* fontFamily)
    {
        UiTextComponent::InlineImageContainer inlineImages;
        const float defaultImageHeight = 32.0f;

        STextDrawContext fontContext;
        fontContext.SetEffect(0);
        fontContext.SetSizeIn800x600(false);
        fontContext.SetSize(Vec2(32.0f, 32.0f));
        const float defaultAscent = fontFamily->normal->GetAscender(fontContext);

        {
            const LyShine::StringType testMarkup("Regular Bold Italic\n");
            DrawBatchContainer drawBatches;
            DrawBatch b1;
            b1.font = fontFamily->normal;
            b1.text = testMarkup;
            drawBatches.push_back(b1);

            InsertNewlinesToWrapText(drawBatches, fontContext, 1000.0f);
            AZ_Assert(drawBatches.front().text == testMarkup, "Test failed");
        }

        {
            // "Regular Bold   v          .<i>Italic\n</i>Bold-Italic"

            StringList stringList;
            stringList.push_back("Regular Bold   v          .");
            stringList.push_back("Italic\n");
            stringList.push_back("Bold-Italic");
            StringList::const_iterator citer = stringList.begin();

            DrawBatchContainer drawBatches;
            DrawBatch b1;
            b1.font = fontFamily->normal;
            b1.text = *citer; ++citer;
            drawBatches.push_back(b1);
            DrawBatch b2;
            b2.font = fontFamily->italic;
            b2.text = *citer; ++citer;
            drawBatches.push_back(b2);
            DrawBatch b3;
            b3.font = fontFamily->normal;
            b3.text = *citer; ++citer;
            drawBatches.push_back(b3);

            InsertNewlinesToWrapText(drawBatches, fontContext, 1000.0f);
            AssertDrawBatchTextContent(drawBatches, stringList);
        }

        // Anchor tag: single line, no wrapping
        {
            const LyShine::StringType textNoMarkup("this is a test!");
            const LyShine::StringType markupTestString("<a action=\"action\" data=\"data\">this</a> is a test!");

            {
                TextMarkup::Tag markupRoot;
                AZ_Assert(TextMarkup::ParseMarkupBuffer(markupTestString, markupRoot), "Test failed");
                AZStd::list<UiTextComponent::DrawBatch> drawBatches;
                AZStd::stack<UiTextComponent::DrawBatch> batchStack;

                AZStd::stack<FontFamily*> fontFamilyStack;
                fontFamilyStack.push(fontFamily);

                UiTextComponent::FontFamilyRefSet fontFamilyRefs;
                BuildDrawBatchesAndAssignClickableIds(drawBatches, fontFamilyRefs, inlineImages, defaultImageHeight, defaultAscent, batchStack, fontFamilyStack, &markupRoot);
                AZ_Assert(0 == fontFamilyRefs.size(), "Test failed");
                AZ_Assert(2 == drawBatches.size(), "Test failed");
                AssertTextNotEmpty(drawBatches);

                StringList stringList;
                stringList.push_back("this");
                stringList.push_back(" is a test!");

                // 1000.0f should be too big to cause any newlines to be inserted
                const float wrapWidth = 1000.0f;
                InsertNewlinesToWrapText(drawBatches, fontContext, wrapWidth);
                AssertDrawBatchTextContent(drawBatches, stringList);
            }

            // Anchor tag: word-wrap cases
            {
                TextMarkup::Tag markupRoot;
                AZ_Assert(TextMarkup::ParseMarkupBuffer(markupTestString, markupRoot), "Test failed");
                AZStd::list<UiTextComponent::DrawBatch> drawBatches;
                AZStd::stack<UiTextComponent::DrawBatch> batchStack;

                AZStd::stack<FontFamily*> fontFamilyStack;
                fontFamilyStack.push(fontFamily);

                UiTextComponent::FontFamilyRefSet fontFamilyRefs;
                BuildDrawBatchesAndAssignClickableIds(drawBatches, fontFamilyRefs, inlineImages, defaultImageHeight, defaultAscent, batchStack, fontFamilyStack, &markupRoot);
                AZ_Assert(0 == fontFamilyRefs.size(), "Test failed");
                AZ_Assert(2 == drawBatches.size(), "Test failed");
                AssertTextNotEmpty(drawBatches);

                // Element size 75% of text length should insert one newline
                const float textWidth = fontFamily->normal->GetTextSize(textNoMarkup.c_str(), true, fontContext).x;
                const float wrapWidth = textWidth * 0.75f;
                InsertNewlinesToWrapText(drawBatches, fontContext, wrapWidth);

                const int numNewlines = 1;
                AssertDrawBatchTextNumNewlines(drawBatches, numNewlines);
            }

            {
                TextMarkup::Tag markupRoot;
                AZ_Assert(TextMarkup::ParseMarkupBuffer(markupTestString, markupRoot), "Test failed");
                AZStd::list<UiTextComponent::DrawBatch> drawBatches;
                AZStd::stack<UiTextComponent::DrawBatch> batchStack;

                AZStd::stack<FontFamily*> fontFamilyStack;
                fontFamilyStack.push(fontFamily);

                UiTextComponent::FontFamilyRefSet fontFamilyRefs;
                BuildDrawBatchesAndAssignClickableIds(drawBatches, fontFamilyRefs, inlineImages, defaultImageHeight, defaultAscent, batchStack, fontFamilyStack, &markupRoot);
                AZ_Assert(0 == fontFamilyRefs.size(), "Test failed");
                AZ_Assert(2 == drawBatches.size(), "Test failed");
                AssertTextNotEmpty(drawBatches);

                // Element size 45% of text length should insert two newlines
                const float textWidth = fontFamily->normal->GetTextSize(textNoMarkup.c_str(), true, fontContext).x;
                const float wrapWidth = textWidth * 0.45f;
                InsertNewlinesToWrapText(drawBatches, fontContext, wrapWidth);

                const int numNewlines = 2;
                AssertDrawBatchTextNumNewlines(drawBatches, numNewlines);
            }
        }
    }

    void BatchLinesTests(FontFamily* fontFamily)
    {
        STextDrawContext fontContext;
        fontContext.SetEffect(0);
        fontContext.SetSizeIn800x600(false);
        fontContext.SetSize(Vec2(32.0f, 32.0f));

        UiTextComponent::InlineImageContainer inlineImages;
        float defaultImageHeight = 32.0f;
        const float defaultAscent = fontFamily->normal->GetAscender(fontContext);

        UiTextInterface::DisplayedTextFunction displayedTextFunction(DefaultDisplayedTextFunction);

        {
            DrawBatchLines batchLines;
            DrawBatchContainer drawBatches;
            DrawBatch b1;
            b1.font = fontFamily->normal;
            b1.text = "a";
            drawBatches.push_back(b1);

            CreateBatchLines(batchLines, drawBatches, fontFamily);
            AZ_Assert(1 == batchLines.batchLines.size(), "Test failed");

            SizeList sizeList;
            sizeList.push_back(1);
            AssertBatchLineSizes(batchLines, sizeList);
        }

        {
            DrawBatchLines batchLines;
            DrawBatchContainer drawBatches;
            DrawBatch b1;
            b1.font = fontFamily->normal;
            b1.text = "a\n";
            drawBatches.push_back(b1);

            CreateBatchLines(batchLines, drawBatches, fontFamily);
            AZ_Assert(2 == batchLines.batchLines.size(), "Test failed");

            SizeList sizeList;
            sizeList.push_back(1);
            sizeList.push_back(1);
            AssertBatchLineSizes(batchLines, sizeList);
        }

        {
            DrawBatchLines batchLines;
            DrawBatchContainer drawBatches;
            DrawBatch b1;
            b1.font = fontFamily->normal;
            b1.text = "a\nb";
            drawBatches.push_back(b1);

            CreateBatchLines(batchLines, drawBatches, fontFamily);
            AZ_Assert(2 == batchLines.batchLines.size(), "Test failed");

            SizeList sizeList;
            sizeList.push_back(1);
            sizeList.push_back(1);
            AssertBatchLineSizes(batchLines, sizeList);
        }

        {
            DrawBatchLines batchLines;
            DrawBatchContainer drawBatches;
            DrawBatch b1;
            b1.font = fontFamily->normal;
            b1.text = "a\n\nb";
            drawBatches.push_back(b1);

            CreateBatchLines(batchLines, drawBatches, fontFamily);
            AZ_Assert(3 == batchLines.batchLines.size(), "Test failed");

            SizeList sizeList;
            sizeList.push_back(1);
            sizeList.push_back(1);
            sizeList.push_back(1);
            AssertBatchLineSizes(batchLines, sizeList);
        }

        {
            DrawBatchLines batchLines;
            DrawBatchContainer drawBatches;
            DrawBatch b1;
            b1.font = fontFamily->normal;
            b1.text = "a\n\n\nb";
            drawBatches.push_back(b1);

            CreateBatchLines(batchLines, drawBatches, fontFamily);
            AZ_Assert(4 == batchLines.batchLines.size(), "Test failed");

            SizeList sizeList;
            sizeList.push_back(1);
            sizeList.push_back(1);
            sizeList.push_back(1);
            sizeList.push_back(1);
            AssertBatchLineSizes(batchLines, sizeList);
        }

        {
            DrawBatchLines batchLines;
            DrawBatchContainer drawBatches;
            DrawBatch b1;
            b1.font = fontFamily->normal;
            b1.text = "Regular Bold Italic\n";
            drawBatches.push_back(b1);

            CreateBatchLines(batchLines, drawBatches, fontFamily);
            AZ_Assert(2 == batchLines.batchLines.size(), "Test failed");

            SizeList sizeList;
            sizeList.push_back(1);
            sizeList.push_back(1);
            AssertBatchLineSizes(batchLines, sizeList);
        }

        {
            const LyShine::StringType markupTestString("Regular Bold     <i>Italic</i>Bold-Italic");
            TextMarkup::Tag markupRoot;

            AZ_Assert(TextMarkup::ParseMarkupBuffer(markupTestString, markupRoot), "Test failed");
            AZStd::list<UiTextComponent::DrawBatch> drawBatches;
            AZStd::stack<UiTextComponent::DrawBatch> batchStack;

            AZStd::stack<FontFamily*> fontFamilyStack;
            fontFamilyStack.push(fontFamily);

            UiTextComponent::FontFamilyRefSet fontFamilyRefs;
            BuildDrawBatchesAndAssignClickableIds(drawBatches, fontFamilyRefs, inlineImages, defaultImageHeight, defaultAscent, batchStack, fontFamilyStack, &markupRoot);

            DrawBatchLines batchLines;
            BatchAwareWrapText(batchLines, drawBatches, fontFamily, fontContext, 290.0f);
            AZ_Assert(2 == batchLines.batchLines.size(), "Test failed");

            SizeList sizeList;
            sizeList.push_back(1);
            sizeList.push_back(2);
            AssertBatchLineSizes(batchLines, sizeList);
        }

        {
            const LyShine::StringType markupTestString("Regular <b>Bold</b> <i>Italic\n<b>Bold-Italic</b></i>");
            TextMarkup::Tag markupRoot;

            AZ_Assert(TextMarkup::ParseMarkupBuffer(markupTestString, markupRoot), "Test failed");
            AZStd::list<UiTextComponent::DrawBatch> drawBatches;
            AZStd::stack<UiTextComponent::DrawBatch> batchStack;

            AZStd::stack<FontFamily*> fontFamilyStack;
            fontFamilyStack.push(fontFamily);

            UiTextComponent::FontFamilyRefSet fontFamilyRefs;
            BuildDrawBatchesAndAssignClickableIds(drawBatches, fontFamilyRefs, inlineImages, defaultImageHeight, defaultAscent, batchStack, fontFamilyStack, &markupRoot);
            DrawBatchLines batchLines;

            CreateBatchLines(batchLines, drawBatches, fontFamily);
            AZ_Assert(2 == batchLines.batchLines.size(), "Test failed");

            SizeList sizeList;
            sizeList.push_back(4);
            sizeList.push_back(1);
            AssertBatchLineSizes(batchLines, sizeList);
        }

        // Anchor tag: word-wrap, anchor doesn't span multiple lines
        {
            const LyShine::StringType textNoMarkup("this is a test!");
            const LyShine::StringType markupTestString("<a action=\"action\" data=\"data\">this</a> is a test!");

            {
                TextMarkup::Tag markupRoot;
                AZ_Assert(TextMarkup::ParseMarkupBuffer(markupTestString, markupRoot), "Test failed");
                AZStd::list<UiTextComponent::DrawBatch> drawBatches;
                AZStd::stack<UiTextComponent::DrawBatch> batchStack;

                AZStd::stack<FontFamily*> fontFamilyStack;
                fontFamilyStack.push(fontFamily);

                UiTextComponent::FontFamilyRefSet fontFamilyRefs;
                BuildDrawBatchesAndAssignClickableIds(drawBatches, fontFamilyRefs, inlineImages, defaultImageHeight, defaultAscent, batchStack, fontFamilyStack, &markupRoot);

                // 1000.0f should be too big to cause any newlines to be inserted
                const float wrapWidth = 1000.0f;
                InsertNewlinesToWrapText(drawBatches, fontContext, wrapWidth);

                DrawBatchLines batchLines;
                CreateBatchLines(batchLines, drawBatches, fontFamily);
                AZ_Assert(1 == batchLines.batchLines.size(), "Test failed");

                SizeList sizeList;
                sizeList.push_back(2);
                AssertBatchLineSizes(batchLines, sizeList);
            }

            {
                TextMarkup::Tag markupRoot;
                AZ_Assert(TextMarkup::ParseMarkupBuffer(markupTestString, markupRoot), "Test failed");
                AZStd::list<UiTextComponent::DrawBatch> drawBatches;
                AZStd::stack<UiTextComponent::DrawBatch> batchStack;

                AZStd::stack<FontFamily*> fontFamilyStack;
                fontFamilyStack.push(fontFamily);

                UiTextComponent::FontFamilyRefSet fontFamilyRefs;
                BuildDrawBatchesAndAssignClickableIds(drawBatches, fontFamilyRefs, inlineImages, defaultImageHeight, defaultAscent, batchStack, fontFamilyStack, &markupRoot);

                // Element size 75% of text length should insert one newline
                const float textWidth = fontFamily->normal->GetTextSize(textNoMarkup.c_str(), true, fontContext).x;
                const float wrapWidth = textWidth * 0.75f;
                InsertNewlinesToWrapText(drawBatches, fontContext, wrapWidth);

                DrawBatchLines batchLines;
                CreateBatchLines(batchLines, drawBatches, fontFamily);
                AZ_Assert(2 == batchLines.batchLines.size(), "Test failed");

                SizeList sizeList;
                sizeList.push_back(2);
                sizeList.push_back(1);
                AssertBatchLineSizes(batchLines, sizeList);
            }

            {
                TextMarkup::Tag markupRoot;
                AZ_Assert(TextMarkup::ParseMarkupBuffer(markupTestString, markupRoot), "Test failed");
                AZStd::list<UiTextComponent::DrawBatch> drawBatches;
                AZStd::stack<UiTextComponent::DrawBatch> batchStack;

                AZStd::stack<FontFamily*> fontFamilyStack;
                fontFamilyStack.push(fontFamily);

                UiTextComponent::FontFamilyRefSet fontFamilyRefs;
                BuildDrawBatchesAndAssignClickableIds(drawBatches, fontFamilyRefs, inlineImages, defaultImageHeight, defaultAscent, batchStack, fontFamilyStack, &markupRoot);

                // Element size 45% of text length should insert two newlines
                const float textWidth = fontFamily->normal->GetTextSize(textNoMarkup.c_str(), true, fontContext).x;
                const float wrapWidth = textWidth * 0.45f;
                InsertNewlinesToWrapText(drawBatches, fontContext, wrapWidth);

                DrawBatchLines batchLines;
                CreateBatchLines(batchLines, drawBatches, fontFamily);
                AZ_Assert(3 == batchLines.batchLines.size(), "Test failed");

                SizeList sizeList;
                sizeList.push_back(2);
                sizeList.push_back(1);
                sizeList.push_back(1);
                AssertBatchLineSizes(batchLines, sizeList);
            }
        }

        // Anchor tag: word-wrap, single anchor spans multiple lines
        {
            const LyShine::StringType textNoMarkup("this is a test!");
            const LyShine::StringType markupTestString("<a action=\"action\" data=\"data\">this is a test!</a>");

            // Sanity check: single-line case
            {
                TextMarkup::Tag markupRoot;
                AZ_Assert(TextMarkup::ParseMarkupBuffer(markupTestString, markupRoot), "Test failed");
                AZStd::list<UiTextComponent::DrawBatch> drawBatches;
                AZStd::stack<UiTextComponent::DrawBatch> batchStack;

                AZStd::stack<FontFamily*> fontFamilyStack;
                fontFamilyStack.push(fontFamily);

                UiTextComponent::FontFamilyRefSet fontFamilyRefs;
                BuildDrawBatchesAndAssignClickableIds(drawBatches, fontFamilyRefs, inlineImages, defaultImageHeight, defaultAscent, batchStack, fontFamilyStack, &markupRoot);

                // 1000.0f should be too big to cause any newlines to be inserted
                const float wrapWidth = 1000.0f;
                InsertNewlinesToWrapText(drawBatches, fontContext, wrapWidth);

                DrawBatchLines batchLines;
                CreateBatchLines(batchLines, drawBatches, fontFamily);
                AZ_Assert(1 == batchLines.batchLines.size(), "Test failed");

                SizeList sizeList;
                sizeList.push_back(1);
                AssertBatchLineSizes(batchLines, sizeList);

                // Since a single anchor tag spans the entirety of the text,
                // we can just iterate over all drawbatches for all lines
                // and verify that the anchor tag information exists across
                // all drawbatch lines.
                for (auto& batchLine : batchLines.batchLines)
                {
                    for (auto& drawBatch : batchLine.drawBatchList)
                    {
                        AZ_Assert(drawBatch.IsClickable(), "Test failed");
                        AZ_Assert(UiTextComponent::DrawBatch::Type::Text == drawBatch.GetType(), "Test failed");
                        AZ_Assert(drawBatch.action == "action", "Test failed");
                        AZ_Assert(drawBatch.data == "data", "Test failed");
                        AZ_Assert(drawBatch.clickableId == 0, "Test failed");
                    }
                }
            }

            // Verify that anchor tag on word-wrapped text expands to both lines
            {
                TextMarkup::Tag markupRoot;
                AZ_Assert(TextMarkup::ParseMarkupBuffer(markupTestString, markupRoot), "Test failed");
                AZStd::list<UiTextComponent::DrawBatch> drawBatches;
                AZStd::stack<UiTextComponent::DrawBatch> batchStack;

                AZStd::stack<FontFamily*> fontFamilyStack;
                fontFamilyStack.push(fontFamily);

                UiTextComponent::FontFamilyRefSet fontFamilyRefs;
                BuildDrawBatchesAndAssignClickableIds(drawBatches, fontFamilyRefs, inlineImages, defaultImageHeight, defaultAscent, batchStack, fontFamilyStack, &markupRoot);

                // Element size 75% of text length should insert one newline
                const float textWidth = fontFamily->normal->GetTextSize(textNoMarkup.c_str(), true, fontContext).x;
                const float wrapWidth = textWidth * 0.75f;
                InsertNewlinesToWrapText(drawBatches, fontContext, wrapWidth);

                DrawBatchLines batchLines;
                CreateBatchLines(batchLines, drawBatches, fontFamily);
                AZ_Assert(2 == batchLines.batchLines.size(), "Test failed");

                SizeList sizeList;
                sizeList.push_back(1);
                sizeList.push_back(1);
                AssertBatchLineSizes(batchLines, sizeList);

                // Since a single anchor tag spans the entirety of the text,
                // we can just iterate over all drawbatches for all lines
                // and verify that the anchor tag information exists across
                // all drawbatch lines.
                for (auto& batchLine : batchLines.batchLines)
                {
                    for (auto& drawBatch : batchLine.drawBatchList)
                    {
                        AZ_Assert(drawBatch.IsClickable(), "Test failed");
                        AZ_Assert(UiTextComponent::DrawBatch::Type::Text == drawBatch.GetType(), "Test failed");
                        AZ_Assert(drawBatch.action == "action", "Test failed");
                        AZ_Assert(drawBatch.data == "data", "Test failed");
                        AZ_Assert(drawBatch.clickableId == 0, "Test failed");
                    }
                }
            }

            {
                TextMarkup::Tag markupRoot;
                AZ_Assert(TextMarkup::ParseMarkupBuffer(markupTestString, markupRoot), "Test failed");
                AZStd::list<UiTextComponent::DrawBatch> drawBatches;
                AZStd::stack<UiTextComponent::DrawBatch> batchStack;

                AZStd::stack<FontFamily*> fontFamilyStack;
                fontFamilyStack.push(fontFamily);

                UiTextComponent::FontFamilyRefSet fontFamilyRefs;
                BuildDrawBatchesAndAssignClickableIds(drawBatches, fontFamilyRefs, inlineImages, defaultImageHeight, defaultAscent, batchStack, fontFamilyStack, &markupRoot);

                // Element size 45% of text length should insert two newlines
                const float textWidth = fontFamily->normal->GetTextSize(textNoMarkup.c_str(), true, fontContext).x;
                const float wrapWidth = textWidth * 0.45f;
                InsertNewlinesToWrapText(drawBatches, fontContext, wrapWidth);

                DrawBatchLines batchLines;
                CreateBatchLines(batchLines, drawBatches, fontFamily);
                AZ_Assert(3 == batchLines.batchLines.size(), "Test failed");

                SizeList sizeList;
                sizeList.push_back(1);
                sizeList.push_back(1);
                sizeList.push_back(1);
                AssertBatchLineSizes(batchLines, sizeList);

                // Since a single anchor tag spans the entirety of the text,
                // we can just iterate over all drawbatches for all lines
                // and verify that the anchor tag information exists across
                // all drawbatch lines.
                for (auto& batchLine : batchLines.batchLines)
                {
                    for (auto& drawBatch : batchLine.drawBatchList)
                    {
                        AZ_Assert(drawBatch.IsClickable(), "Test failed");
                        AZ_Assert(UiTextComponent::DrawBatch::Type::Text == drawBatch.GetType(), "Test failed");
                        AZ_Assert(drawBatch.action == "action", "Test failed");
                        AZ_Assert(drawBatch.data == "data", "Test failed");
                        AZ_Assert(drawBatch.clickableId == 0, "Test failed");
                    }
                }
            }
        }

        /////////////////////////////////////////////////////////////

        // Anchor tag: word-wrap, multiple anchor spans multiple lines
        {
            {
                const LyShine::StringType textNoMarkup("this is a test!");
                const LyShine::StringType markupTestString(
                    "<a action=\"action1\" data=\"data1\">this is a test</a>"
                    "<a action=\"action2\" data=\"data2\">!</a>"
                );

                TextMarkup::Tag markupRoot;
                AZ_Assert(TextMarkup::ParseMarkupBuffer(markupTestString, markupRoot), "Test failed");
                AZStd::list<UiTextComponent::DrawBatch> drawBatches;
                AZStd::stack<UiTextComponent::DrawBatch> batchStack;

                AZStd::stack<FontFamily*> fontFamilyStack;
                fontFamilyStack.push(fontFamily);

                UiTextComponent::FontFamilyRefSet fontFamilyRefs;
                BuildDrawBatchesAndAssignClickableIds(drawBatches, fontFamilyRefs, inlineImages, defaultImageHeight, defaultAscent, batchStack, fontFamilyStack, &markupRoot);

                // Element size 75% of text length should insert one newline
                const float textWidth = fontFamily->normal->GetTextSize(textNoMarkup.c_str(), true, fontContext).x;
                const float wrapWidth = textWidth * 0.75f;
                InsertNewlinesToWrapText(drawBatches, fontContext, wrapWidth);

                DrawBatchLines batchLines;
                CreateBatchLines(batchLines, drawBatches, fontFamily);
                AZ_Assert(2 == batchLines.batchLines.size(), "Test failed");

                SizeList sizeList;
                sizeList.push_back(1);
                sizeList.push_back(2);
                AssertBatchLineSizes(batchLines, sizeList);
                auto batchLineIter = batchLines.batchLines.begin();

                {
                    const auto& batchLine = *batchLineIter;
                    auto drawBatchIter = batchLine.drawBatchList.begin();
                    {
                        const DrawBatch& drawBatch = *drawBatchIter;
                        AZ_Assert(drawBatch.IsClickable(), "Test failed");
                        AZ_Assert(UiTextComponent::DrawBatch::Type::Text == drawBatch.GetType(), "Test failed");
                        AZ_Assert(drawBatch.action == "action1", "Test failed");
                        AZ_Assert(drawBatch.data == "data1", "Test failed");
                        AZ_Assert(drawBatch.clickableId == 0, "Test failed");
                    }
                }

                // Next line
                ++batchLineIter;
                {
                    const auto& batchLine = *batchLineIter;
                    auto drawBatchIter = batchLine.drawBatchList.begin();
                    {
                        const DrawBatch& drawBatch = *drawBatchIter;
                        AZ_Assert(drawBatch.IsClickable(), "Test failed");
                        AZ_Assert(UiTextComponent::DrawBatch::Type::Text == drawBatch.GetType(), "Test failed");
                        AZ_Assert(drawBatch.action == "action1", "Test failed");
                        AZ_Assert(drawBatch.data == "data1", "Test failed");
                        AZ_Assert(drawBatch.clickableId == 0, "Test failed");
                    }

                    // Next batch
                    ++drawBatchIter;
                    {
                        const DrawBatch& drawBatch = *drawBatchIter;
                        AZ_Assert(drawBatch.text == "!", "Test failed");
                        AZ_Assert(drawBatch.IsClickable(), "Test failed");
                        AZ_Assert(UiTextComponent::DrawBatch::Type::Text == drawBatch.GetType(), "Test failed");
                        AZ_Assert(drawBatch.action == "action2", "Test failed");
                        AZ_Assert(drawBatch.data == "data2", "Test failed");
                        AZ_Assert(drawBatch.clickableId == 1, "Test failed");
                    }
                }
            }

            {
                const LyShine::StringType textNoMarkup("this is a test!");
                const LyShine::StringType markupTestString(
                    "<a action=\"action1\" data=\"data1\">t</a>"
                    "<a action=\"action2\" data=\"data2\">his is a test!</a>"
                );

                TextMarkup::Tag markupRoot;
                AZ_Assert(TextMarkup::ParseMarkupBuffer(markupTestString, markupRoot), "Test failed");
                AZStd::list<UiTextComponent::DrawBatch> drawBatches;
                AZStd::stack<UiTextComponent::DrawBatch> batchStack;

                AZStd::stack<FontFamily*> fontFamilyStack;
                fontFamilyStack.push(fontFamily);

                UiTextComponent::FontFamilyRefSet fontFamilyRefs;
                BuildDrawBatchesAndAssignClickableIds(drawBatches, fontFamilyRefs, inlineImages, defaultImageHeight, defaultAscent, batchStack, fontFamilyStack, &markupRoot);

                // Element size 75% of text length should insert one newline
                const float textWidth = fontFamily->normal->GetTextSize(textNoMarkup.c_str(), true, fontContext).x;
                const float wrapWidth = textWidth * 0.75f;
                InsertNewlinesToWrapText(drawBatches, fontContext, wrapWidth);

                DrawBatchLines batchLines;
                CreateBatchLines(batchLines, drawBatches, fontFamily);
                AZ_Assert(2 == batchLines.batchLines.size(), "Test failed");

                SizeList sizeList;
                sizeList.push_back(2);
                sizeList.push_back(1);
                AssertBatchLineSizes(batchLines, sizeList);
                auto batchLineIter = batchLines.batchLines.begin();

                {
                    const auto& batchLine = *batchLineIter;
                    auto drawBatchIter = batchLine.drawBatchList.begin();
                    {
                        const DrawBatch& drawBatch = *drawBatchIter;
                        AZ_Assert(drawBatch.text == "t", "Test failed");
                        AZ_Assert(drawBatch.IsClickable(), "Test failed");
                        AZ_Assert(UiTextComponent::DrawBatch::Type::Text == drawBatch.GetType(), "Test failed");
                        AZ_Assert(drawBatch.action == "action1", "Test failed");
                        AZ_Assert(drawBatch.data == "data1", "Test failed");
                        AZ_Assert(drawBatch.clickableId == 0, "Test failed");
                    }

                    // Next batch
                    ++drawBatchIter;
                    {
                        const DrawBatch& drawBatch = *drawBatchIter;
                        AZ_Assert(drawBatch.IsClickable(), "Test failed");
                        AZ_Assert(UiTextComponent::DrawBatch::Type::Text == drawBatch.GetType(), "Test failed");
                        AZ_Assert(drawBatch.action == "action2", "Test failed");
                        AZ_Assert(drawBatch.data == "data2", "Test failed");
                        AZ_Assert(drawBatch.clickableId == 1, "Test failed");
                    }
                }

                // Next line
                ++batchLineIter;
                {
                    const auto& batchLine = *batchLineIter;
                    auto drawBatchIter = batchLine.drawBatchList.begin();
                    {
                        const DrawBatch& drawBatch = *drawBatchIter;
                        AZ_Assert(drawBatch.IsClickable(), "Test failed");
                        AZ_Assert(UiTextComponent::DrawBatch::Type::Text == drawBatch.GetType(), "Test failed");
                        AZ_Assert(drawBatch.action == "action2", "Test failed");
                        AZ_Assert(drawBatch.data == "data2", "Test failed");
                        AZ_Assert(drawBatch.clickableId == 1, "Test failed");
                    }
                }
            }
        }
    }

    void CreateComponent(AZ::Entity* entity, const AZ::Uuid& componentTypeId)
    {
        entity->Deactivate();
        entity->CreateComponent(componentTypeId);
        entity->Activate();
    }

    void TestCharacterSpacing(CLyShine* lyshine, const AZStd::string& fontPath, float fontSize, const AZStd::string& testString, float characterSpacing, const char* testName)
    {
        AZ::EntityId canvasEntityId = lyshine->CreateCanvas();
        UiCanvasInterface* canvas = UiCanvasBus::FindFirstHandler(canvasEntityId);
        AZ_Assert(canvas, "Test failed");

        AZ::Entity* testElem = canvas->CreateChildElement("TrackingTestElement");
        AZ_Assert(testElem, "Test failed");
        CreateComponent(testElem, LyShine::UiTransform2dComponentUuid);
        CreateComponent(testElem, LyShine::UiTextComponentUuid);
        AZ::EntityId testElemId = testElem->GetId();

        UiTextBus::Event(testElemId, &UiTextBus::Events::SetText, testString);

        UiTextBus::Event(testElemId, &UiTextBus::Events::SetFont, fontPath);
        UiTextBus::Event(testElemId, &UiTextBus::Events::SetFontSize, fontSize);

        float baseWidth;
        UiLayoutCellDefaultBus::EventResult(
            baseWidth, testElemId, &UiLayoutCellDefaultBus::Events::GetTargetWidth, LyShine::UiLayoutCellUnspecifiedSize);

        UiTextBus::Event(testElemId, &UiTextBus::Events::SetCharacterSpacing, characterSpacing);
        float newWidth;
        UiLayoutCellDefaultBus::EventResult(newWidth, testElemId, &UiLayoutCellDefaultBus::Events::GetTargetWidth, LyShine::UiLayoutCellUnspecifiedSize);

        const int testStringLength = static_cast<int>(testString.length());
        const int numGapsBetweenCharacters = testStringLength >= 1 ? testStringLength - 1 : 0;
        const float ems = characterSpacing * 0.001f;
        float expectedWidth = baseWidth + numGapsBetweenCharacters * ems * fontSize;

        if (expectedWidth < 0.0f)
        {
            expectedWidth = 0.0f;
        }

        AZ_Assert(IsClose(newWidth, expectedWidth), "Test failed: Character Spacing, %s. Expected: %f, actual: %f", testName, expectedWidth, newWidth);

        lyshine->ReleaseCanvas(canvasEntityId, false);
    }

    void TestLineSpacing(CLyShine* lyshine, const AZStd::string& fontPath, float fontSize, const AZStd::string& testString, int numNewlines, float lineSpacing, const char* testName)
    {
        AZ::EntityId canvasEntityId = lyshine->CreateCanvas();
        UiCanvasInterface* canvas = UiCanvasBus::FindFirstHandler(canvasEntityId);
        AZ_Assert(canvas, "Test failed");

        AZ::Entity* testElem = canvas->CreateChildElement("LeadingTestElement");
        AZ_Assert(testElem, "Test failed");
        CreateComponent(testElem, LyShine::UiTransform2dComponentUuid);
        CreateComponent(testElem, LyShine::UiTextComponentUuid);
        AZ::EntityId testElemId = testElem->GetId();

        UiTextBus::Event(testElemId, &UiTextBus::Events::SetText, testString);

        UiTextBus::Event(testElemId, &UiTextBus::Events::SetFont, fontPath);
        UiTextBus::Event(testElemId, &UiTextBus::Events::SetFontSize, fontSize);

        float baseHeight;
        UiLayoutCellDefaultBus::EventResult(
            baseHeight, testElemId, &UiLayoutCellDefaultBus::Events::GetTargetHeight, LyShine::UiLayoutCellUnspecifiedSize);

        UiTextBus::Event(testElemId, &UiTextBus::Events::SetLineSpacing, lineSpacing);
        float newHeight;
        UiLayoutCellDefaultBus::EventResult(
            newHeight, testElemId, &UiLayoutCellDefaultBus::Events::GetTargetHeight, LyShine::UiLayoutCellUnspecifiedSize);

        float expectedHeight = baseHeight + numNewlines * lineSpacing;

        if (expectedHeight < 0.0f)
        {
            expectedHeight = 0.0f;
        }

        AZ_Assert(IsClose(newHeight, expectedHeight), "Test failed: Line Spacing, %s. Expected: %f, actual: %f", testName, expectedHeight, newHeight);

        lyshine->ReleaseCanvas(canvasEntityId, false);
    }

    void TrackingLeadingTests(CLyShine* lyshine)
    {
        // Character Spacing

        TestCharacterSpacing(lyshine, "default-ui", 32.0f, "Hi", 1000.0f, "one space");
        TestCharacterSpacing(lyshine, "default-ui", 32.0f, "W", 1000.0f, "no spaces");
        TestCharacterSpacing(lyshine, "default-ui", 32.0f, "", 1000.0f, "empty string");
        TestCharacterSpacing(lyshine, "default-ui", 32.0f, "Hi", 4500.0f, "bigger spacing");
        TestCharacterSpacing(lyshine, "default-ui", 32.0f, "abcde", 1000.0f, "four spaces");
        TestCharacterSpacing(lyshine, "default-ui", 32.0f, "abcde", 3500.0f, "four spaces, larger spacing");
        TestCharacterSpacing(lyshine, "default-ui", 32.0f, "12345678", 5432.1f, "seven spaces, non-round spacing");
        TestCharacterSpacing(lyshine, "default-ui", 32.0f, "12345678", 5432.1f, "seven spaces, non-round spacing, lots of kerning");
        TestCharacterSpacing(lyshine, "default-ui", 32.0f, "Hi", -1000.0f, "negative spacing");
        TestCharacterSpacing(lyshine, "default-ui", 32.0f, "abcde", -1000.0f, "negative spacing, 4 spaces");
        TestCharacterSpacing(lyshine, "default-ui", 16.0f, "Hi", 1000.0f, "smaller font size, one space");
        TestCharacterSpacing(lyshine, "default-ui", 16.0f, "abcdefghijk", 1000.0f, "smaller font size, ten spaces");
        TestCharacterSpacing(lyshine, "default-ui", 16.0f, "abcdefghijk", 3500.0f, "smaller font size, ten spaces, larger spacing");
        TestCharacterSpacing(lyshine, "default-ui", 64.0f, "Hi", 1000.0f, "larger font size, one space");
        TestCharacterSpacing(lyshine, "default-ui", 64.0f, "abcdefgh", 1000.0f, "larger font size, seven spaces");
        TestCharacterSpacing(lyshine, "default-ui", 64.0f, "abcdefgh", 5200.0f, "larger font size, seven spaces, larger spacing");
        TestCharacterSpacing(lyshine, "default", 32.0f, "abcdefgh", 1000.0f, "default (monospace) font, seven spaces");
        TestCharacterSpacing(lyshine, "notosans-regular", 32.0f, "WAW.AWA|WAW", 2500.0f, "noto sans font, 10 spaces, larger spacing");
        TestCharacterSpacing(lyshine, "notosans-regular", 32.0f, "WAW.AWA|WAW", 25.0f, "noto sans font, 10 spaces, smaller spacing");
        TestCharacterSpacing(lyshine, "notosans-regular", 32.0f, "WAW.AWA|WAW", -25.0f, "noto sans font, 10 spaces, smaller negative spacing");

        // Line Spacing

        TestLineSpacing(lyshine, "default-ui", 32.0f, "Hi\nHello", 1, 5.0f, "one newline");
        TestLineSpacing(lyshine, "default-ui", 32.0f, "1\n2\n3\n4\n5", 4, 5.0f, "four newlines");
        TestLineSpacing(lyshine, "default-ui", 32.0f, "1\n2\n3\n4\n5\n6\n7\n8", 7, 8.3f, "seven newlines, larger spacing");
        TestLineSpacing(lyshine, "default-ui", 32.0f, "1\n2", 1, -1.0f, "one newline, negative spacing");
        TestLineSpacing(lyshine, "default-ui", 32.0f, "1\n2\n3\n4", 3, -2.2f, "three newlines, negative spacing, larger spacing");
        TestLineSpacing(lyshine, "default-ui", 18.0f, "1\n2", 1, 1.0f, "one newlines, smaller font");
        TestLineSpacing(lyshine, "default-ui", 18.0f, "1\n2\n3\n4\n5", 4, 1.0f, "four newlines, smaller font");
        TestLineSpacing(lyshine, "default-ui", 64.0f, "1\n2", 1, 1.0f, "one newlines, larger font");
        TestLineSpacing(lyshine, "default-ui", 64.0f, "1\n2\n3\n4\n5", 4, 1.0f, "four newlines, larger font");
        TestLineSpacing(lyshine, "default", 16.0f, "1\n2\n3\n4\n5", 4, 1.0f, "four newlines, default (mono) font");
        TestLineSpacing(lyshine, "notosans-regular", 20.0f, "1\n2\n3\n4\n5", 4, 1.0f, "four newlines, notosans font");
    }

    void ComponentGetSetTextTests(CLyShine* lyshine)
    {
        {
            AZ::EntityId canvasEntityId = lyshine->CreateCanvas();
            UiCanvasInterface* canvas = UiCanvasBus::FindFirstHandler(canvasEntityId);
            AZ_Assert(canvas, "Test failed");

            AZ::Entity* testElem = canvas->CreateChildElement("Test1");
            AZ_Assert(testElem, "Test failed");
            CreateComponent(testElem, LyShine::UiTransform2dComponentUuid);
            CreateComponent(testElem, LyShine::UiTextComponentUuid);
            AZ::EntityId testElemId = testElem->GetId();

            const AZStd::string testString("Hi");
            UiTextBus::Event(testElemId, &UiTextBus::Events::SetText, testString);
            AZStd::string resultString;
            UiTextBus::EventResult(resultString, testElemId, &UiTextBus::Events::GetText);
            AZ_Assert(testString == resultString, "Test failed");
            resultString.clear();
            UiTextBus::EventResult(resultString, testElemId, &UiTextBus::Events::GetTextWithFlags, UiTextInterface::GetTextFlags::GetAsIs);
            AZ_Assert(testString == resultString, "Test failed");

            lyshine->ReleaseCanvas(canvasEntityId, false);
        }

        {
            AZ::EntityId canvasEntityId = lyshine->CreateCanvas();
            UiCanvasInterface* canvas = UiCanvasBus::FindFirstHandler(canvasEntityId);
            AZ_Assert(canvas, "Test failed");

            AZ::Entity* testElem = canvas->CreateChildElement("Test1");
            AZ_Assert(testElem, "Test failed");
            CreateComponent(testElem, LyShine::UiTransform2dComponentUuid);
            CreateComponent(testElem, LyShine::UiTextComponentUuid);
            AZ::EntityId testElemId = testElem->GetId();

            const AZStd::string testString("Hi");
            UiTextBus::Event(testElemId, &UiTextBus::Events::SetTextWithFlags, testString, UiTextInterface::SetAsIs);
            AZStd::string resultString;
            UiTextBus::EventResult(resultString, testElemId, &UiTextBus::Events::GetText);
            AZ_Assert(testString == resultString, "Test failed");
            resultString.clear();
            UiTextBus::EventResult(resultString, testElemId, &UiTextBus::Events::GetTextWithFlags, UiTextInterface::GetTextFlags::GetAsIs);
            AZ_Assert(testString == resultString, "Test failed");

            lyshine->ReleaseCanvas(canvasEntityId, false);
        }

        {
            AZ::EntityId canvasEntityId = lyshine->CreateCanvas();
            UiCanvasInterface* canvas = UiCanvasBus::FindFirstHandler(canvasEntityId);
            AZ_Assert(canvas, "Test failed");

            AZ::Entity* testElem = canvas->CreateChildElement("Test1");
            AZ_Assert(testElem, "Test failed");
            CreateComponent(testElem, LyShine::UiTransform2dComponentUuid);
            CreateComponent(testElem, LyShine::UiTextComponentUuid);
            AZ::EntityId testElemId = testElem->GetId();

            const AZStd::string testString("&<>%");
            UiTextBus::Event(testElemId, &UiTextBus::Events::SetTextWithFlags, testString, UiTextInterface::SetAsIs);
            AZStd::string resultString;
            UiTextBus::EventResult(resultString, testElemId, &UiTextBus::Events::GetText);
            AZ_Assert(testString == resultString, "Test failed");
            resultString.clear();
            UiTextBus::EventResult(resultString, testElemId, &UiTextBus::Events::GetTextWithFlags, UiTextInterface::GetTextFlags::GetAsIs);
            AZ_Assert(testString == resultString, "Test failed");

            lyshine->ReleaseCanvas(canvasEntityId, false);
        }

        {
            AZ::EntityId canvasEntityId = lyshine->CreateCanvas();
            UiCanvasInterface* canvas = UiCanvasBus::FindFirstHandler(canvasEntityId);
            AZ_Assert(canvas, "Test failed");

            AZ::Entity* testElem = canvas->CreateChildElement("Test1");
            AZ_Assert(testElem, "Test failed");
            CreateComponent(testElem, LyShine::UiTransform2dComponentUuid);
            CreateComponent(testElem, LyShine::UiTextComponentUuid);
            AZ::EntityId testElemId = testElem->GetId();

            const AZStd::string testString("&amp;&lt;&gt;&#37;");
            UiTextBus::Event(testElemId, &UiTextBus::Events::SetTextWithFlags, testString, UiTextInterface::SetAsIs);
            AZStd::string resultString;
            UiTextBus::EventResult(resultString, testElemId, &UiTextBus::Events::GetText);
            AZ_Assert(testString == resultString, "Test failed");
            resultString.clear();
            UiTextBus::EventResult(resultString, testElemId, &UiTextBus::Events::GetTextWithFlags, UiTextInterface::GetTextFlags::GetAsIs);
            AZ_Assert(testString == resultString, "Test failed");

            lyshine->ReleaseCanvas(canvasEntityId, false);
        }

        {
            AZ::EntityId canvasEntityId = lyshine->CreateCanvas();
            UiCanvasInterface* canvas = UiCanvasBus::FindFirstHandler(canvasEntityId);
            AZ_Assert(canvas, "Test failed");

            AZ::Entity* testElem = canvas->CreateChildElement("Test1");
            AZ_Assert(testElem, "Test failed");
            CreateComponent(testElem, LyShine::UiTransform2dComponentUuid);
            CreateComponent(testElem, LyShine::UiTextComponentUuid);
            AZ::EntityId testElemId = testElem->GetId();

            const AZStd::string testString("&<>%");
            UiTextBus::Event(testElemId, &UiTextBus::Events::SetTextWithFlags, testString, UiTextInterface::SetEscapeMarkup);
            AZStd::string resultString;
            UiTextBus::EventResult(resultString, testElemId, &UiTextBus::Events::GetText);
            AZ_Assert(testString == resultString, "Test failed");
            resultString.clear();
            UiTextBus::EventResult(resultString, testElemId, &UiTextBus::Events::GetTextWithFlags, UiTextInterface::GetTextFlags::GetAsIs);
            AZ_Assert(testString == resultString, "Test failed");

            lyshine->ReleaseCanvas(canvasEntityId, false);
        }
    }

    void ComponentGetSetTextTestsLoc(CLyShine* lyshine)
    {
        if (AZStd::string("korean") == GetISystem()->GetLocalizationManager()->GetLanguage())
        {
            static const LyShine::StringType koreanHello("\xEC\x95\x88\xEB\x85\x95\xED\x95\x98\xEC\x84\xB8\xEC\x9A\x94");

            // Tests: Get/SetText with localization
            {
                AZ::EntityId canvasEntityId = lyshine->CreateCanvas();
                UiCanvasInterface* canvas = UiCanvasBus::FindFirstHandler(canvasEntityId);
                AZ_Assert(canvas, "Test failed");

                AZ::Entity* testElem = canvas->CreateChildElement("Test1");
                AZ_Assert(testElem, "Test failed");
                CreateComponent(testElem, LyShine::UiTransform2dComponentUuid);
                CreateComponent(testElem, LyShine::UiTextComponentUuid);
                AZ::EntityId testElemId = testElem->GetId();

                // Verify that GetText and GetAsIs returns the unlocalized key "@ui_Hello"
                {
                    AZStd::string testString("@ui_Hello");
                    UiTextBus::Event(testElemId, &UiTextBus::Events::SetTextWithFlags, testString, UiTextInterface::SetLocalized);
                    AZStd::string resultString;
                    UiTextBus::EventResult(resultString, testElemId, &UiTextBus::Events::GetText);
                    AZ_Assert(testString == resultString, "Test failed");
                    resultString.clear();
                    UiTextBus::EventResult(
                        resultString, testElemId, &UiTextBus::Events::GetTextWithFlags, UiTextInterface::GetTextFlags::GetAsIs);
                    AZ_Assert(testString == resultString, "Test failed");
                    resultString.clear();
                }

                // Verify that passing GetLocalized to GetTextWithFlags returns the localized content of "@ui_Hello"
                {
                    AZStd::string testString = koreanHello;
                    AZStd::string resultString;
                    UiTextBus::EventResult(resultString, testElemId, &UiTextBus::Events::GetTextWithFlags, UiTextInterface::GetLocalized);
                    AZ_Assert(testString == resultString, "Test failed");
                    resultString.clear();
                }

                lyshine->ReleaseCanvas(canvasEntityId, false);
            }

            // Tests: Get/SetText with localization and escaping markup
            {
                AZ::EntityId canvasEntityId = lyshine->CreateCanvas();
                UiCanvasInterface* canvas = UiCanvasBus::FindFirstHandler(canvasEntityId);
                AZ_Assert(canvas, "Test failed");

                AZ::Entity* testElem = canvas->CreateChildElement("Test1");
                AZ_Assert(testElem, "Test failed");
                CreateComponent(testElem, LyShine::UiTransform2dComponentUuid);
                CreateComponent(testElem, LyShine::UiTextComponentUuid);
                AZ::EntityId testElemId = testElem->GetId();

                // Verify that GetText and GetAsIs returns the unlocalized key "@ui_Hello" along
                // with the original (escaped) markup characters
                {
                    AZStd::string testString("&<>% @ui_Hello");
                    UiTextInterface::SetTextFlags setTextFlags = static_cast<UiTextInterface::SetTextFlags>(UiTextInterface::SetEscapeMarkup | UiTextInterface::SetLocalized);
                    UiTextBus::Event(testElemId, &UiTextBus::Events::SetTextWithFlags, testString, setTextFlags);
                    AZStd::string resultString;
                    UiTextBus::EventResult(resultString, testElemId, &UiTextBus::Events::GetText);
                    AZ_Assert(testString == resultString, "Test failed");
                    resultString.clear();
                    UiTextBus::EventResult(
                        resultString, testElemId, &UiTextBus::Events::GetTextWithFlags, UiTextInterface::GetTextFlags::GetAsIs);
                    AZ_Assert(testString == resultString, "Test failed");
                    resultString.clear();
                }

                // Verify that passing GetLocalized to GetTextWithFlags returns the localized content of "@ui_Hello"
                // along with the original (escaped) markup characters in the string
                {
                    AZStd::string testString = LyShine::StringType("&<>% ") + koreanHello;
                    AZStd::string resultString;
                    UiTextBus::EventResult(resultString, testElemId, &UiTextBus::Events::GetTextWithFlags, UiTextInterface::GetLocalized);
                    AZ_Assert(testString == resultString, "Test failed");
                    resultString.clear();
                }

                lyshine->ReleaseCanvas(canvasEntityId, false);
            }

            // Tests: Setting localized text with abutting invalid localization key chars
            //
            // Purpose: localization tokens appear in strings surrounded by characters that
            // shouldn't be part of the localization key.
            //
            // For example:
            // "@ui_Hello, @ui_Welcome!"
            //
            // This should only consider "@ui_Hello" and "@ui_Hello" for localization. The
            // abutting punctuation characters - comma, exclamation point - should not be
            // considered as part of the localization key.
            //
            // Markup example:
            // "<font color="#FF0000">@ui_DeathStatus</font>"
            //
            // The end font-tag text ("</font>") following the loc key "@ui_DeathStatus" should
            // not be considered for localization.
            //
            // Abutting loc keys example:
            // "@ui_item1@ui_item2"
            //
            // There are two loc keys in the above example and should be localized independently
            // of each other..
            {
                AZ::EntityId canvasEntityId = lyshine->CreateCanvas();
                UiCanvasInterface* canvas = UiCanvasBus::FindFirstHandler(canvasEntityId);
                AZ_Assert(canvas, "Test failed");

                AZ::Entity* testElem = canvas->CreateChildElement("Test1");
                AZ_Assert(testElem, "Test failed");
                CreateComponent(testElem, LyShine::UiTransform2dComponentUuid);
                CreateComponent(testElem, LyShine::UiTextComponentUuid);
                AZ::EntityId testElemId = testElem->GetId();

                // Verify that localizing keys won't consider punctuation as part
                // of the localization key.
                {
                    AZStd::string testString("@ui_Hello, @ui_Hello!");
                    UiTextInterface::SetTextFlags setTextFlags = static_cast<UiTextInterface::SetTextFlags>(UiTextInterface::SetLocalized);
                    UiTextBus::Event(testElemId, &UiTextBus::Events::SetTextWithFlags, testString, setTextFlags);

                    testString = koreanHello + ", " + koreanHello + "!";
                    AZStd::string resultString;
                    UiTextBus::EventResult(resultString, testElemId, &UiTextBus::Events::GetTextWithFlags, UiTextInterface::GetLocalized);
                    AZ_Assert(testString == resultString, "Test failed");
                }

                // Verify that localizing keys won't consider markup as part
                // of the localization key.
                {
                    AZStd::string testString("<font color=\"#FF0000\">@ui_Hello</font>");
                    UiTextInterface::SetTextFlags setTextFlags = static_cast<UiTextInterface::SetTextFlags>(UiTextInterface::SetLocalized);
                    UiTextBus::Event(testElemId, &UiTextBus::Events::SetTextWithFlags, testString, setTextFlags);

                    testString = LyShine::StringType("<font color=\"#FF0000\">") + koreanHello + "</font>";
                    AZStd::string resultString;
                    UiTextBus::EventResult(resultString, testElemId, &UiTextBus::Events::GetTextWithFlags, UiTextInterface::GetLocalized);
                    AZ_Assert(testString == resultString, "Test failed");
                }

                // Verify that localizing adjacent keys will localize the keys separately
                // and not consider them to be one single key
                {
                    AZStd::string testString("@ui_Hello@ui_Hello");
                    UiTextInterface::SetTextFlags setTextFlags = static_cast<UiTextInterface::SetTextFlags>(UiTextInterface::SetLocalized);
                    UiTextBus::Event(testElemId, &UiTextBus::Events::SetTextWithFlags, testString, setTextFlags);

                    testString = koreanHello + koreanHello;
                    AZStd::string resultString;
                    UiTextBus::EventResult(resultString, testElemId, &UiTextBus::Events::GetTextWithFlags, UiTextInterface::GetLocalized);
                    AZ_Assert(testString == resultString, "Test failed");
                }

                lyshine->ReleaseCanvas(canvasEntityId, false);
            }
        }
    }

    // This tests for whether or not the MarkupFlag is functioning properly
    void MarkupFlagTest(CLyShine* lyshine)
    {
        AZ::EntityId canvasEntityId = lyshine->CreateCanvas();
        UiCanvasInterface* canvas = UiCanvasBus::FindFirstHandler(canvasEntityId);
        AZ_Assert(canvas, "Test failed");

        AZ::Entity* testElem = canvas->CreateChildElement("Test1");
        AZ_Assert(testElem, "Test failed");

        CreateComponent(testElem, LyShine::UiTransform2dComponentUuid);
        CreateComponent(testElem, LyShine::UiTextComponentUuid);
        AZ::EntityId testElemId = testElem->GetId();
        UiTextBus::Event(testElemId, &UiTextBus::Events::SetText, "<font color=\"red\"> </font>");

        bool enabled(true);
        AZ::Vector2 NewSize(0, 0);
        // Sizes expected based on the default font
        AZ::Vector2 MarkUpEnabledSize(8, 32);
        AZ::Vector2 MarkUpDisabledSize(354, 32);

        // Test that markup is disabled by default.
        UiTextBus::EventResult(enabled, testElemId, &UiTextBus::Events::GetIsMarkupEnabled);
        AZ_Assert(!enabled, "Test failed");

        // Test that setting it to false when it is already false, does not set it to true.
        UiTextBus::Event(testElemId, &UiTextBus::Events::SetIsMarkupEnabled, false);
        UiTextBus::EventResult(enabled, testElemId, &UiTextBus::Events::GetIsMarkupEnabled);
        AZ_Assert(!enabled, "Test failed");

        // Check that the flag is actually disabled by checking the size of the textbox
        UiTextBus::EventResult(NewSize, testElemId, &UiTextBus::Events::GetTextSize);
        AZ_Assert(NewSize == MarkUpDisabledSize, "Test failed");

        // Test that setting it to true when it is false, sets it to true
        UiTextBus::Event(testElemId, &UiTextBus::Events::SetIsMarkupEnabled, true);
        UiTextBus::EventResult(enabled, testElemId, &UiTextBus::Events::GetIsMarkupEnabled);
        AZ_Assert(enabled, "Test failed");

        // Check that the flag is actually enabled by checking the size of the textbox
        UiTextBus::EventResult(NewSize, testElemId, &UiTextBus::Events::GetTextSize);
        AZ_Assert(NewSize == MarkUpEnabledSize, "Test failed");

        // Test that setting it to true when it is true, does not set it to false
        UiTextBus::Event(testElemId, &UiTextBus::Events::SetIsMarkupEnabled, true);
        UiTextBus::EventResult(enabled, testElemId, &UiTextBus::Events::GetIsMarkupEnabled);
        AZ_Assert(enabled, "Test failed");

        // Check that the flag is actually enabled by checking the size of the textbox
        UiTextBus::EventResult(NewSize, testElemId, &UiTextBus::Events::GetTextSize);
        AZ_Assert(NewSize == MarkUpEnabledSize, "Test failed");

        // Test that setting it to false when it is true, properly sets it to false.
        UiTextBus::Event(testElemId, &UiTextBus::Events::SetIsMarkupEnabled, false);
        UiTextBus::EventResult(enabled, testElemId, &UiTextBus::Events::GetIsMarkupEnabled);
        AZ_Assert(!enabled, "Test failed");

        // Check that the flag is actually disabled by checking the size of the textbox
        UiTextBus::EventResult(NewSize, testElemId, &UiTextBus::Events::GetTextSize);
        AZ_Assert(NewSize == MarkUpDisabledSize, "Test failed");

        lyshine->ReleaseCanvas(canvasEntityId, false);
    }
}

void FontSharedPtrTests()
{
    // Verify test font isn't loaded
    {
        const char* fontName = "notosans-regular";
        AZ_Assert(nullptr == GetISystem()->GetICryFont()->GetFont(fontName), "Test failed");
    }

    // Basic font load and unload
    {
        const char* fontName = "notosans-regular";

        IFFont* font = GetISystem()->GetICryFont()->NewFont(fontName);
        AZ_Assert(font, "Test failed");
        AZ_Assert(font == GetISystem()->GetICryFont()->GetFont(fontName), "Test failed");

        const bool loadSuccess = font->Load("ui/fonts/lyshineexamples/notosans/notosans-regular.font");
        AZ_Assert(loadSuccess, "Test failed");
        font->AddRef();
        AZ_Assert(1 == font->Release(), "Test failed");
        AZ_Assert(0 == font->Release(), "Test failed");
        AZ_Assert(nullptr == GetISystem()->GetICryFont()->GetFont(fontName), "Test failed");
    }

    // Font and font family case sensitivity checks
    {
        // IFFont case sensitivity checks
        {

            const char* fontName = "notosans-regular";
            const char* fontNameUpper = "NOTOSANS-REGULAR";
            const char* fontNameMixed1 = "Notosans-regular";
            const char* fontNameMixed2 = "Notosans-Regular";
            const char* fontNameMixed3 = "NoToSaNs-ReGuLaR";

            IFFont* const font = GetISystem()->GetICryFont()->NewFont(fontName);
            AZ_Assert(font, "Test failed");
            AZ_Assert(2 == font->AddRef(), "Test failed");
            AZ_Assert(1 == font->Release(), "Test failed");

            // Verify that creating a new font for a font that's already created returns
            // that same font
            AZ_Assert(font == GetISystem()->GetICryFont()->NewFont(fontName), "Test failed");
            AZ_Assert(font == GetISystem()->GetICryFont()->NewFont(fontNameUpper), "Test failed");
            AZ_Assert(font == GetISystem()->GetICryFont()->NewFont(fontNameMixed1), "Test failed");
            AZ_Assert(font == GetISystem()->GetICryFont()->NewFont(fontNameMixed2), "Test failed");
            AZ_Assert(font == GetISystem()->GetICryFont()->NewFont(fontNameMixed3), "Test failed");

            // Getting the font with the expected name returns the same font
            AZ_Assert(font == GetISystem()->GetICryFont()->GetFont(fontName), "Test failed");
            AZ_Assert(font == GetISystem()->GetICryFont()->GetFont(fontNameUpper), "Test failed");
            AZ_Assert(font == GetISystem()->GetICryFont()->GetFont(fontNameMixed1), "Test failed");
            AZ_Assert(font == GetISystem()->GetICryFont()->GetFont(fontNameMixed2), "Test failed");
            AZ_Assert(font == GetISystem()->GetICryFont()->GetFont(fontNameMixed3), "Test failed");

            // Release the font
            AZ_Assert(0 == font->Release(), "Test failed");
        }

        // FontFamily case sensitivity checks
        {
            const char* notoSansFontFamily = "ui/fonts/lyshineexamples/notosans/notosans.fontfamily";
            const char* notoSansName = "notosans";

            // Shouldn't be loaded yet
            FontFamilyPtr fontFamily = gEnv->pCryFont->GetFontFamily(notoSansFontFamily);
            AZ_Assert(!fontFamily.get(), "Test failed");
            fontFamily = gEnv->pCryFont->GetFontFamily(notoSansName);
            AZ_Assert(!fontFamily.get(), "Test failed");

            // Should load successfully
            fontFamily = gEnv->pCryFont->LoadFontFamily(notoSansFontFamily);
            AZ_Assert(fontFamily.get(), "Test failed");

            // GetFontFamily case-sensitivity tests by filepath
            {
                AZ_Assert(fontFamily == gEnv->pCryFont->GetFontFamily(notoSansFontFamily), "Test failed");

                const char* notoSansFontFamilyUpper = "UI/FONTS/LYSHINEEXAMPLES/NOTOSANS/NOTOSANS.FONTFAMILY";
                const char* notoSansFontFamilyMixed1 = "ui/fonts/lyshineexamples/notosans/Notosans.fontfamily";
                const char* notoSansFontFamilyMixed2 = "ui/fonts/lyshineexamples/notosans/Notosans.Fontfamily";
                const char* notoSansFontFamilyMixed3 = "ui/fonts/lyshineexamples/notosans/NotoSans.Fontfamily";
                const char* notoSansFontFamilyMixed4 = "ui/fonts/lyshineexamples/notosans/notosans.FONTFAMILY";
                const char* notoSansFontFamilyMixed5 = "ui/fonts/lyshineexamples/notosans/NOTOSANS.fontfamily";
                const char* notoSansFontFamilyMixed6 = "Ui/fonts/lyshineexamples/notosans/notosans.fontfamily";
                const char* notoSansFontFamilyMixed7 = "ui/fonts/lyshineexamples/Notosans/notosans.fontfamily";
                AZ_Assert(fontFamily == gEnv->pCryFont->GetFontFamily(notoSansFontFamilyUpper), "Test failed");
                AZ_Assert(fontFamily == gEnv->pCryFont->GetFontFamily(notoSansFontFamilyMixed1), "Test failed");
                AZ_Assert(fontFamily == gEnv->pCryFont->GetFontFamily(notoSansFontFamilyMixed2), "Test failed");
                AZ_Assert(fontFamily == gEnv->pCryFont->GetFontFamily(notoSansFontFamilyMixed3), "Test failed");
                AZ_Assert(fontFamily == gEnv->pCryFont->GetFontFamily(notoSansFontFamilyMixed4), "Test failed");
                AZ_Assert(fontFamily == gEnv->pCryFont->GetFontFamily(notoSansFontFamilyMixed5), "Test failed");
                AZ_Assert(fontFamily == gEnv->pCryFont->GetFontFamily(notoSansFontFamilyMixed6), "Test failed");
                AZ_Assert(fontFamily == gEnv->pCryFont->GetFontFamily(notoSansFontFamilyMixed7), "Test failed");
            }

            // GetFontFamily case-sensitivity tests by font name
            {
                AZ_Assert(fontFamily == gEnv->pCryFont->GetFontFamily(notoSansName), "Test failed");

                const char* notoSansNameUpper = "NOTOSANS";
                const char* notoSansNameMixed1 = "Notosans";
                const char* notoSansNameMixed2 = "NotoSans";
                AZ_Assert(fontFamily == gEnv->pCryFont->GetFontFamily(notoSansNameUpper), "Test failed");
                AZ_Assert(fontFamily == gEnv->pCryFont->GetFontFamily(notoSansNameMixed1), "Test failed");
                AZ_Assert(fontFamily == gEnv->pCryFont->GetFontFamily(notoSansNameMixed2), "Test failed");
            }
        }
    }

    // Font family ref count test
    {
        const char* notoSansFontFamily = "ui/fonts/lyshineexamples/notosans/notosans.fontfamily";
        const char* notoSansRegularPath = "ui/fonts/lyshineexamples/notosans/notosans-regular.font";
        const char* notoSansItalicPath = "ui/fonts/lyshineexamples/notosans/notosans-italic.font";
        const char* notoSansBoldPath = "ui/fonts/lyshineexamples/notosans/notosans-bold.font";
        const char* notoSansBoldItalicPath = "ui/fonts/lyshineexamples/notosans/notosans-bolditalic.font";

        const char* notoSansRegular = "notosans-regular";
        const char* notoSansBold = "notosans-bold";
        const char* notoSansItalic = "notosans-italic";
        const char* notoSansBoldItalic = "notosans-boldItalic";

        {
            FontFamilyPtr notoSans = FontFamilyLoad(notoSansFontFamily);
            AZ_Assert(2 == notoSans->normal->AddRef(), "Test failed");
            AZ_Assert(1 == notoSans->normal->Release(), "Test failed");
            AZ_Assert(2 == notoSans->bold->AddRef(), "Test failed");
            AZ_Assert(1 == notoSans->bold->Release(), "Test failed");
            AZ_Assert(2 == notoSans->italic->AddRef(), "Test failed");
            AZ_Assert(1 == notoSans->italic->Release(), "Test failed");
            AZ_Assert(2 == notoSans->boldItalic->AddRef(), "Test failed");
            AZ_Assert(1 == notoSans->boldItalic->Release(), "Test failed");

            // This is a negative test which is difficult to support currently.
            // Uncommenting this line should trigger an assert in CryFont because
            // the font was de-allocated while still being referenced by a
            // FontFamily
            //notoSans->normal->Release();

            // Attempt to load FontFamily already loaded
            {
                FontFamilyPtr dupeFamily = GetISystem()->GetICryFont()->LoadFontFamily(notoSansFontFamily);
                AZ_Assert(nullptr == dupeFamily, "Test failed");

                //Ref counts should remain the same
                AZ_Assert(2 == notoSans->normal->AddRef(), "Test failed");
                AZ_Assert(1 == notoSans->normal->Release(), "Test failed");
                AZ_Assert(2 == notoSans->bold->AddRef(), "Test failed");
                AZ_Assert(1 == notoSans->bold->Release(), "Test failed");
                AZ_Assert(2 == notoSans->italic->AddRef(), "Test failed");
                AZ_Assert(1 == notoSans->italic->Release(), "Test failed");
                AZ_Assert(2 == notoSans->boldItalic->AddRef(), "Test failed");
                AZ_Assert(1 == notoSans->boldItalic->Release(), "Test failed");
            }

            IFFont* fontRegular = GetISystem()->GetICryFont()->GetFont(notoSansRegularPath);
            AZ_Assert(fontRegular, "Test failed");
            AZ_Assert(fontRegular == notoSans->normal, "Test failed");

            // Verify that ref counts are handled properly when font family
            // fonts are referenced outside of the font family.
            {
                // NewFont shouldn't increment ref count
                IFFont* checkFont = GetISystem()->GetICryFont()->NewFont(notoSansRegularPath);
                AZ_Assert(fontRegular == checkFont, "Test failed");
                AZ_Assert(2 == checkFont->AddRef(), "Test failed");
                AZ_Assert(1 == checkFont->Release(), "Test failed");

                // Load also doesn't increment ref count
                AZ_Assert(checkFont->Load(notoSansRegularPath), "Test failed");
                AZ_Assert(2 == checkFont->AddRef(), "Test failed");
                AZ_Assert(1 == checkFont->Release(), "Test failed");

                // If font is loaded as a Font Family, then ref counts will increment
                FontFamilyPtr notoSansRegularFamily = FontFamilyLoad(notoSansRegularPath);

                // Verify that every font of the single-font font family are the same
                AZ_Assert(notoSansRegularFamily->normal == notoSansRegularFamily->bold, "Test failed");
                AZ_Assert(notoSansRegularFamily->bold == notoSansRegularFamily->italic, "Test failed");
                AZ_Assert(notoSansRegularFamily->italic == notoSansRegularFamily->boldItalic, "Test failed");

                // Verify that the single-font is the same font in the original font family
                AZ_Assert(notoSansRegularFamily->normal == notoSans->normal, "Test failed");

                // Check "single font as a font family" ref counts
                AZ_Assert(6 == notoSansRegularFamily->normal->AddRef(), "Test failed");
                AZ_Assert(5 == notoSansRegularFamily->normal->Release(), "Test failed");
                AZ_Assert(6 == notoSansRegularFamily->bold->AddRef(), "Test failed");
                AZ_Assert(5 == notoSansRegularFamily->bold->Release(), "Test failed");
                AZ_Assert(6 == notoSansRegularFamily->italic->AddRef(), "Test failed");
                AZ_Assert(5 == notoSansRegularFamily->italic->Release(), "Test failed");
                AZ_Assert(6 == notoSansRegularFamily->boldItalic->AddRef(), "Test failed");
                AZ_Assert(5 == notoSansRegularFamily->boldItalic->Release(), "Test failed");

                // Check ref counts of the original font family
                AZ_Assert(6 == notoSans->normal->AddRef(), "Test failed");
                AZ_Assert(5 == notoSans->normal->Release(), "Test failed");
                AZ_Assert(2 == notoSans->bold->AddRef(), "Test failed");
                AZ_Assert(1 == notoSans->bold->Release(), "Test failed");
                AZ_Assert(2 == notoSans->italic->AddRef(), "Test failed");
                AZ_Assert(1 == notoSans->italic->Release(), "Test failed");
                AZ_Assert(2 == notoSans->boldItalic->AddRef(), "Test failed");
                AZ_Assert(1 == notoSans->boldItalic->Release(), "Test failed");

                // Attempt to load single-font font-family again
                {
                    FontFamilyPtr dupeFamily = GetISystem()->GetICryFont()->LoadFontFamily(notoSansRegularPath);
                    AZ_Assert(nullptr == dupeFamily, "Test failed");

                    //Ref counts should remain the same
                    AZ_Assert(6 == notoSansRegularFamily->normal->AddRef(), "Test failed");
                    AZ_Assert(5 == notoSansRegularFamily->normal->Release(), "Test failed");
                    AZ_Assert(6 == notoSansRegularFamily->bold->AddRef(), "Test failed");
                    AZ_Assert(5 == notoSansRegularFamily->bold->Release(), "Test failed");
                    AZ_Assert(6 == notoSansRegularFamily->italic->AddRef(), "Test failed");
                    AZ_Assert(5 == notoSansRegularFamily->italic->Release(), "Test failed");
                    AZ_Assert(6 == notoSansRegularFamily->boldItalic->AddRef(), "Test failed");
                    AZ_Assert(5 == notoSansRegularFamily->boldItalic->Release(), "Test failed");
                }
            }

            // BEGIN JAV_LY_FORK: r_persistFontFamilies keeps font families loaded for lifetime of application.
            // In this case, the normal/regular font has already been loaded as a "pass through" font family,
            // so it has been persisted in memory. Even though the FontFamilyPtr used has gone out of scope.
            // notoSansRegularFamily should now be out of scope, so the original font family's
            // ref counts should return to their original values
            {
                AZ_Assert(6 == notoSans->normal->AddRef(), "Test failed");
                AZ_Assert(5 == notoSans->normal->Release(), "Test failed");
                AZ_Assert(2 == notoSans->bold->AddRef(), "Test failed");
                AZ_Assert(1 == notoSans->bold->Release(), "Test failed");
                AZ_Assert(2 == notoSans->italic->AddRef(), "Test failed");
                AZ_Assert(1 == notoSans->italic->Release(), "Test failed");
                AZ_Assert(2 == notoSans->boldItalic->AddRef(), "Test failed");
                AZ_Assert(1 == notoSans->boldItalic->Release(), "Test failed");
            }

            // Reference a FontFamily already loaded
            {
                FontFamilyPtr dupeFamily = GetISystem()->GetICryFont()->GetFontFamily("notosans");

                // Ref couts for underlying fonts should stay the same
                AZ_Assert(6 == notoSans->normal->AddRef(), "Test failed");
                AZ_Assert(5 == notoSans->normal->Release(), "Test failed");
                AZ_Assert(2 == notoSans->bold->AddRef(), "Test failed");
                AZ_Assert(1 == notoSans->bold->Release(), "Test failed");
                AZ_Assert(2 == notoSans->italic->AddRef(), "Test failed");
                AZ_Assert(1 == notoSans->italic->Release(), "Test failed");
                AZ_Assert(2 == notoSans->boldItalic->AddRef(), "Test failed");
                AZ_Assert(1 == notoSans->boldItalic->Release(), "Test failed");
            }
            // END JAV_LY_FORK

            IFFont* fontBold = GetISystem()->GetICryFont()->GetFont(notoSansBoldPath);
            AZ_Assert(fontBold, "Test failed");
            AZ_Assert(fontBold == notoSans->bold, "Test failed");

            IFFont* fontItalic = GetISystem()->GetICryFont()->GetFont(notoSansItalicPath);
            AZ_Assert(fontItalic, "Test failed");
            AZ_Assert(fontItalic == notoSans->italic, "Test failed");

            IFFont* fontBoldItalic = GetISystem()->GetICryFont()->GetFont(notoSansBoldItalicPath);
            AZ_Assert(fontBoldItalic, "Test failed");
            AZ_Assert(fontBoldItalic == notoSans->boldItalic, "Test failed");
        }

        // Once FontFamilyPtr goes out of scope, all associated font family
        // fonts should get unloaded too.
        IFFont* fontRegular = GetISystem()->GetICryFont()->GetFont(notoSansRegular);
        AZ_Assert(!fontRegular, "Test failed");

        IFFont* fontBold = GetISystem()->GetICryFont()->GetFont(notoSansBold);
        AZ_Assert(!fontBold, "Test failed");

        IFFont* fontItalic = GetISystem()->GetICryFont()->GetFont(notoSansItalic);
        AZ_Assert(!fontItalic, "Test failed");

        IFFont* fontBoldItalic = GetISystem()->GetICryFont()->GetFont(notoSansBoldItalic);
        AZ_Assert(!fontBoldItalic, "Test failed");
    }

    // Load Vera.font as a font family, then load Vera.fontfamily that also
    // uses Vera.font as a font
    {
        const char* veraFontFile = "fonts/vera.font";
        FontFamilyPtr veraFont = gEnv->pCryFont->LoadFontFamily(veraFontFile);
        AZ_Assert(veraFont.get(), "Test failed");

        // Verify that vera.font has been referenced 4 times for all four
        // font styles in the font family markup (single fonts loaded as
        // font families get re-used for each of the font styles)
        AZ_Assert(5 == veraFont->normal->AddRef(), "Test failed");
        AZ_Assert(4 == veraFont->normal->Release(), "Test failed");
        AZ_Assert(5 == veraFont->bold->AddRef(), "Test failed");
        AZ_Assert(4 == veraFont->bold->Release(), "Test failed");
        AZ_Assert(5 == veraFont->italic->AddRef(), "Test failed");
        AZ_Assert(4 == veraFont->italic->Release(), "Test failed");
        AZ_Assert(5 == veraFont->boldItalic->AddRef(), "Test failed");
        AZ_Assert(4 == veraFont->boldItalic->Release(), "Test failed");

        const char* veraFontFamilyFile = "fonts/vera.fontfamily";
        FontFamilyPtr veraFontFamily = gEnv->pCryFont->LoadFontFamily(veraFontFamilyFile);

        // BEGIN JAV_LY_FORK: The above "vera.font" is a pass-through font (not a font family)
        // and is now mapped by by its full filepath rather than just the filename.
        AZ_Assert(veraFontFamily.get(), "Test failed");

        // The vera font family uses vera.font for its regular-weighted font,
        // so the ref count for vera.font increases by one, from 4 to 5.
        AZ_Assert(6 == veraFont->normal->AddRef(), "Test failed");
        AZ_Assert(5 == veraFont->normal->Release(), "Test failed");
        AZ_Assert(6 == veraFont->bold->AddRef(), "Test failed");
        AZ_Assert(5 == veraFont->bold->Release(), "Test failed");
        AZ_Assert(6 == veraFont->italic->AddRef(), "Test failed");
        AZ_Assert(5 == veraFont->italic->Release(), "Test failed");
        AZ_Assert(6 == veraFont->boldItalic->AddRef(), "Test failed");
        AZ_Assert(5 == veraFont->boldItalic->Release(), "Test failed");
        // END JAV_LY_FORK
    }
}

void UiTextComponent::UnitTest(CLyShine* lyshine, IConsoleCmdArgs* cmdArgs)
{
    const bool testsRunningAtStartup = cmdArgs == nullptr;
    if (testsRunningAtStartup)
    {
        FontSharedPtrTests();
    }
    else
    {
        // These tests assume the unit-tests run at startup in order for ref count
        // values to make sense.
        AZ_Warning("LyShine", false,
            "Unit-tests: skipping FontSharedPtrTests due to tests running "
            "ad-hoc. Run unit tests at startup for full coverage. See ui_RunUnitTestsOnStartup.");
    }

    VerifyShippingFonts();

    // These fonts are required for subsequent unit-tests to work.
    FontFamilyPtr notoSans = FontFamilyLoad("ui/fonts/lyshineexamples/notosans/notosans.fontfamily");
    FontFamilyPtr notoSerif = FontFamilyLoad("ui/fonts/lyshineexamples/notoserif/notoserif.fontfamily");

    NewlineSanitizeTests();
    BuildDrawBatchesTests(notoSans.get());
    WrapTextTests(notoSans.get());
    BatchLinesTests(notoSans.get());
    TrackingLeadingTests(lyshine);
    ComponentGetSetTextTests(lyshine);
    MarkupFlagTest(lyshine);
}

void UiTextComponent::UnitTestLocalization(CLyShine* lyshine, IConsoleCmdArgs* /* cmdArgs */)
{
    ILocalizationManager* pLocMan = GetISystem()->GetLocalizationManager();

    AZStd::string localizationXml("libs/localization/localization.xml");

    if (!pLocMan || !pLocMan->InitLocalizationData(localizationXml.c_str()) || !pLocMan->LoadLocalizationDataByTag("init"))
    {
        AZ_Assert(false, "Failed to load localization");
    }

    ComponentGetSetTextTestsLoc(lyshine);
}

#endif
