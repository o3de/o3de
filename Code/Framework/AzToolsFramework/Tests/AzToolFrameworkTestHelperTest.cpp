/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/UnitTest/AzToolsFrameworkTestHelpers.h>

#include <QWidget>

namespace UnitTest
{
    class AzToolsFrameworkTestHelpersFixture : public AllocatorsTestFixture
    {
    public:
        void SetUp() override
        {
            m_rootWidget = AZStd::make_unique<QWidget>();
        }

        void TearDown() override
        {
            m_rootWidget.reset();
        }

        AZStd::unique_ptr<QWidget> m_rootWidget;
    };

    struct MouseMoveParams
    {
        QPoint widgetPosition;
        QSize widgetSize;
        QPoint localCursorPosition;
        QPoint deltaPosition;
    };

    class MouseMoveAzToolsFrameworkTestHelperFixture
        : public AzToolsFrameworkTestHelpersFixture
        , public ::testing::WithParamInterface<MouseMoveParams>
    {
    };

    TEST_P(MouseMoveAzToolsFrameworkTestHelperFixture, MouseMove_AzToolsFrameworkTestHelpers)
    {
        // setup
        const MouseMoveParams mouseMoveParams = GetParam();
        m_rootWidget->setFixedSize(mouseMoveParams.widgetSize);
        m_rootWidget->move(mouseMoveParams.widgetPosition);

        MouseMove(m_rootWidget.get(), mouseMoveParams.localCursorPosition, mouseMoveParams.deltaPosition);

        const QPoint mouseLocalPos = m_rootWidget->mapFromGlobal(QCursor::pos());
        const QPoint expectedPosition = mouseMoveParams.localCursorPosition + mouseMoveParams.deltaPosition;
        EXPECT_NEAR(mouseLocalPos.x(), expectedPosition.x(), 1.0f);
        EXPECT_NEAR(mouseLocalPos.y(), expectedPosition.y(), 1.0f);
    }

    INSTANTIATE_TEST_CASE_P(
        All,
        MouseMoveAzToolsFrameworkTestHelperFixture,
        testing::Values(
            MouseMoveParams{ QPoint(0, 0), QSize(100, 100), QPoint(0, 0), QPoint(10, 10) },
            MouseMoveParams{ QPoint(100, 100), QSize(100, 100), QPoint(0, 0), QPoint(10, 10) },
            MouseMoveParams{ QPoint(20, 20), QSize(100, 100), QPoint(50, 50), QPoint(50, 50) },
            MouseMoveParams{ QPoint(20, 20), QSize(100, 100), QPoint(50, 50), QPoint(0, 10) }));
} // namespace UnitTest