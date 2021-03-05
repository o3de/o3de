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
