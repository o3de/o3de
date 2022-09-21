/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/UnitTest/AzToolsFrameworkTestHelpers.h>

#include <QObject>
#include <QWidget>

namespace UnitTest
{
    class AzToolsFrameworkTestHelpersFixture : public AllocatorsTestFixture
    {
    public:
        void SetUp() override
        {
            AllocatorsTestFixture::SetUp();

            m_rootWidget = new QWidget();
            m_rootWidget->setFixedSize(0, 0);
            m_rootWidget->setMouseTracking(true);
            m_rootWidget->move(0, 0); // explicitly set the widget to be in the upper left corner

            m_mouseMoveDetector = new MouseMoveDetector();
            m_rootWidget->installEventFilter(m_mouseMoveDetector);
        }

        void TearDown() override
        {
            m_rootWidget->removeEventFilter(m_mouseMoveDetector);
            delete m_rootWidget;
            delete m_mouseMoveDetector;

            AllocatorsTestFixture::TearDown();
        }

        QWidget* m_rootWidget = nullptr;
        MouseMoveDetector* m_mouseMoveDetector = nullptr;
    };

    struct MouseMoveParams
    {
        QSize m_widgetSize;
        QPoint m_widgetPosition;
        QPoint m_localCursorPosition;
        QPoint m_cursorDelta;
    };

    class MouseMoveAzToolsFrameworkTestHelperFixture
        : public AzToolsFrameworkTestHelpersFixture
        , public ::testing::WithParamInterface<MouseMoveParams>
    {
    };

    TEST_P(MouseMoveAzToolsFrameworkTestHelperFixture, MouseMoveCorrectlyTransformsCursorPositionInGlobalAndLocalSpace)
    {
        // given
        const MouseMoveParams mouseMoveParams = GetParam();
        printf("reached #1");
        m_rootWidget->move(mouseMoveParams.m_widgetPosition);
        printf("reached #2");
        m_rootWidget->setFixedSize(mouseMoveParams.m_widgetSize);
        printf("reached #3");

        // when
        MouseMove(m_rootWidget, mouseMoveParams.m_localCursorPosition, mouseMoveParams.m_cursorDelta);
        printf("reached #4");

        // then
        const QPoint mouseLocalPosition = m_mouseMoveDetector->m_mouseLocalPosition;
        printf("reached #5");
        const QPoint mouseLocalPositionFromGlobal = m_rootWidget->mapFromGlobal(m_mouseMoveDetector->m_mouseGlobalPosition);
        printf("reached #6");
        const QPoint expectedPosition = mouseMoveParams.m_localCursorPosition + mouseMoveParams.m_cursorDelta;
        printf("reached #7");

        using ::testing::Eq;
        EXPECT_THAT(mouseLocalPosition.x(), Eq(expectedPosition.x()));
        EXPECT_THAT(mouseLocalPosition.y(), Eq(expectedPosition.y()));
        EXPECT_THAT(mouseLocalPositionFromGlobal.x(), Eq(expectedPosition.x()));
        EXPECT_THAT(mouseLocalPositionFromGlobal.y(), Eq(expectedPosition.y()));
        printf("reached #1");
    }

    INSTANTIATE_TEST_CASE_P(
        All,
        MouseMoveAzToolsFrameworkTestHelperFixture,
        testing::Values(
            MouseMoveParams{ QSize(100, 100), QPoint(0, 0), QPoint(0, 0), QPoint(10, 10) },
            MouseMoveParams{ QSize(100, 100), QPoint(100, 100), QPoint(0, 0), QPoint(10, 10) },
            MouseMoveParams{ QSize(100, 100), QPoint(20, 20), QPoint(50, 50), QPoint(20, 20) }));

} // namespace UnitTest
