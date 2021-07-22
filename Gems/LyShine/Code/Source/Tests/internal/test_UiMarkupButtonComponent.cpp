/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#if defined(LYSHINE_INTERNAL_UNIT_TEST)

namespace
{
    void FindClickableTextRectIndexFromCanvasSpacePointTests()
    {
        // Empty case (no clickable rects)
        {
            AZ::Vector2 mousePos = AZ::Vector2::CreateZero();
            UiClickableTextInterface::ClickableTextRects clickableTextRects;

            int index = FindClickableTextRectIndexFromCanvasSpacePoint(mousePos, clickableTextRects);
            AZ_Assert(index < 0, "Test failed");
        }

        // Zero-sized rect, mouse pos exactly on "rect"
        {
            UiClickableTextInterface::ClickableTextRects clickableTextRects;
            UiClickableTextInterface::ClickableTextRect textRect;
            textRect.rect.top = textRect.rect.left = textRect.rect.bottom = textRect.rect.right = 0.0f;
            clickableTextRects.push_back(textRect);
            AZ::Vector2 mousePos = AZ::Vector2::CreateZero();
            int index = FindClickableTextRectIndexFromCanvasSpacePoint(mousePos, clickableTextRects);
            AZ_Assert(index == 0, "Test failed");
        }

        // Mouse pos: single rect
        {
            UiClickableTextInterface::ClickableTextRects clickableTextRects;

            UiClickableTextInterface::ClickableTextRect textRect;
            textRect.rect.top = textRect.rect.left = 1.0f;
            textRect.rect.bottom = textRect.rect.right = 100.0f;
            clickableTextRects.push_back(textRect);

            AZ::Vector2 mousePos = AZ::Vector2(2.0f, 2.0f);
            int index = FindClickableTextRectIndexFromCanvasSpacePoint(mousePos, clickableTextRects);
            AZ_Assert(index == 0, "Test failed");

            mousePos = AZ::Vector2(1.0f, 1.0f);
            index = FindClickableTextRectIndexFromCanvasSpacePoint(mousePos, clickableTextRects);
            AZ_Assert(index == 0, "Test failed");

            mousePos = AZ::Vector2(100.0f, 100.0f);
            index = FindClickableTextRectIndexFromCanvasSpacePoint(mousePos, clickableTextRects);
            AZ_Assert(index == 0, "Test failed");

            mousePos = AZ::Vector2::CreateZero();
            index = FindClickableTextRectIndexFromCanvasSpacePoint(mousePos, clickableTextRects);
            AZ_Assert(index < 0, "Test failed");

            mousePos = AZ::Vector2(2.0f, 101.0f);
            index = FindClickableTextRectIndexFromCanvasSpacePoint(mousePos, clickableTextRects);
            AZ_Assert(index < 0, "Test failed");

            mousePos = AZ::Vector2(101.0f, 2.0f);
            index = FindClickableTextRectIndexFromCanvasSpacePoint(mousePos, clickableTextRects);
            AZ_Assert(index < 0, "Test failed");

            mousePos = AZ::Vector2(101.0f, 101.0f);
            index = FindClickableTextRectIndexFromCanvasSpacePoint(mousePos, clickableTextRects);
            AZ_Assert(index < 0, "Test failed");
        }

        // Mouse pos: multiple rects
        {
            UiClickableTextInterface::ClickableTextRects clickableTextRects;

            UiClickableTextInterface::ClickableTextRect textRect;
            textRect.rect.top = textRect.rect.left = 1.0f;
            textRect.rect.bottom = textRect.rect.right = 100.0f;
            clickableTextRects.push_back(textRect);

            textRect.rect.top = textRect.rect.left = 101.0f;
            textRect.rect.bottom = textRect.rect.right = 200.0f;
            clickableTextRects.push_back(textRect);

            AZ::Vector2 mousePos = AZ::Vector2(2.0f, 2.0f);
            int index = FindClickableTextRectIndexFromCanvasSpacePoint(mousePos, clickableTextRects);
            AZ_Assert(index == 0, "Test failed");

            mousePos = AZ::Vector2(1.0f, 1.0f);
            index = FindClickableTextRectIndexFromCanvasSpacePoint(mousePos, clickableTextRects);
            AZ_Assert(index == 0, "Test failed");

            mousePos = AZ::Vector2(100.0f, 100.0f);
            index = FindClickableTextRectIndexFromCanvasSpacePoint(mousePos, clickableTextRects);
            AZ_Assert(index == 0, "Test failed");

            mousePos = AZ::Vector2(102.0f, 102.0f);
            index = FindClickableTextRectIndexFromCanvasSpacePoint(mousePos, clickableTextRects);
            AZ_Assert(index == 1, "Test failed");

            mousePos = AZ::Vector2(101.0f, 101.0f);
            index = FindClickableTextRectIndexFromCanvasSpacePoint(mousePos, clickableTextRects);
            AZ_Assert(index == 1, "Test failed");

            mousePos = AZ::Vector2(200.0f, 200.0f);
            index = FindClickableTextRectIndexFromCanvasSpacePoint(mousePos, clickableTextRects);
            AZ_Assert(index == 1, "Test failed");

            mousePos = AZ::Vector2::CreateZero();
            index = FindClickableTextRectIndexFromCanvasSpacePoint(mousePos, clickableTextRects);
            AZ_Assert(index < 0, "Test failed");

            mousePos = AZ::Vector2(1.0f, 101.0f);
            index = FindClickableTextRectIndexFromCanvasSpacePoint(mousePos, clickableTextRects);
            AZ_Assert(index < 0, "Test failed");

            mousePos = AZ::Vector2(101.0f, 1.0f);
            index = FindClickableTextRectIndexFromCanvasSpacePoint(mousePos, clickableTextRects);
            AZ_Assert(index < 0, "Test failed");

            mousePos = AZ::Vector2(101.0f, 201.0f);
            index = FindClickableTextRectIndexFromCanvasSpacePoint(mousePos, clickableTextRects);
            AZ_Assert(index < 0, "Test failed");

            mousePos = AZ::Vector2(201.0f, 101.0f);
            index = FindClickableTextRectIndexFromCanvasSpacePoint(mousePos, clickableTextRects);
            AZ_Assert(index < 0, "Test failed");
        }
    }
}

void UiMarkupButtonComponent::UnitTest(CLyShine* /* lyshine */, IConsoleCmdArgs* /* cmdArgs */)
{
    FindClickableTextRectIndexFromCanvasSpacePointTests();
}
#endif
