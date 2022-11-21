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
    class AzToolsFrameworkTestHelpersFixture : public LeakDetectionFixture
    {
    public:
        void SetUp() override
        {
            LeakDetectionFixture::SetUp();

            m_rootWidget = AZStd::make_unique<QWidget>();
            m_rootWidget->setFixedSize(0, 0);
            m_rootWidget->setMouseTracking(true);
            m_rootWidget->move(0, 0); // explicitly set the widget to be in the upper left corner

            m_mouseMoveDetector = AZStd::make_unique<MouseMoveDetector>();
            m_rootWidget->installEventFilter(m_mouseMoveDetector.get());
        }

        void TearDown() override
        {
            m_rootWidget->removeEventFilter(m_mouseMoveDetector.get());
            m_rootWidget.reset();
            m_mouseMoveDetector.reset();

            LeakDetectionFixture::TearDown();
        }

        AZStd::unique_ptr<QWidget> m_rootWidget;
        AZStd::unique_ptr<MouseMoveDetector> m_mouseMoveDetector;
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
        m_rootWidget->move(mouseMoveParams.m_widgetPosition);
        m_rootWidget->setFixedSize(mouseMoveParams.m_widgetSize);

        // when
        MouseMove(m_rootWidget.get(), mouseMoveParams.m_localCursorPosition, mouseMoveParams.m_cursorDelta);

        // then
        const QPoint mouseLocalPosition = m_mouseMoveDetector->m_mouseLocalPosition;
        const QPoint mouseLocalPositionFromGlobal = m_rootWidget->mapFromGlobal(m_mouseMoveDetector->m_mouseGlobalPosition);
        const QPoint expectedPosition = mouseMoveParams.m_localCursorPosition + mouseMoveParams.m_cursorDelta;

        using ::testing::Eq;
        EXPECT_THAT(mouseLocalPosition.x(), Eq(expectedPosition.x()));
        EXPECT_THAT(mouseLocalPosition.y(), Eq(expectedPosition.y()));
        EXPECT_THAT(mouseLocalPositionFromGlobal.x(), Eq(expectedPosition.x()));
        EXPECT_THAT(mouseLocalPositionFromGlobal.y(), Eq(expectedPosition.y()));
    }

    INSTANTIATE_TEST_CASE_P(
        All,
        MouseMoveAzToolsFrameworkTestHelperFixture,
        testing::Values(
            MouseMoveParams{ QSize(100, 100), QPoint(0, 0), QPoint(0, 0), QPoint(10, 10) },
            MouseMoveParams{ QSize(100, 100), QPoint(100, 100), QPoint(0, 0), QPoint(10, 10) },
            MouseMoveParams{ QSize(100, 100), QPoint(20, 20), QPoint(50, 50), QPoint(20, 20) }));
} // namespace UnitTest
