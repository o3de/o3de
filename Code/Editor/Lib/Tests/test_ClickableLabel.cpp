/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "EditorDefs.h"
#include <AzTest/AzTest.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <QtUI/ClickableLabel.h>

#include <QApplication>

using namespace AZ;
using namespace ::testing;

namespace UnitTest
{
    class TestingClickableLabel
        : public testing::Test
    {
    public:
        ClickableLabel m_clickableLabel;
    };

    TEST_F(TestingClickableLabel, CursorDoesNotUpdateWhileDisabled)
    {
        m_clickableLabel.setEnabled(false);

        QApplication::setOverrideCursor(QCursor(Qt::BlankCursor));
        QEnterEvent enterEvent{ QPointF(), QPointF(), QPointF() };
        QApplication::sendEvent(&m_clickableLabel, &enterEvent);

        const Qt::CursorShape cursorShape = QApplication::overrideCursor()->shape();
        EXPECT_THAT(cursorShape, Ne(Qt::PointingHandCursor));
        EXPECT_THAT(cursorShape, Eq(Qt::BlankCursor));
    }

    TEST_F(TestingClickableLabel, DoesNotRespondToDblClickWhileDisabled)
    {
        m_clickableLabel.setEnabled(false);

        bool linkActivated = false;
        QObject::connect(&m_clickableLabel, &QLabel::linkActivated, [&linkActivated]()
        {
            linkActivated = true;
        });

        QMouseEvent mouseEvent {
            QEvent::MouseButtonDblClick, QPointF(),
            Qt::LeftButton, Qt::LeftButton, Qt::NoModifier };
        QApplication::sendEvent(&m_clickableLabel, &mouseEvent);

        EXPECT_THAT(linkActivated, Eq(false));
    }
} // namespace UnitTest
