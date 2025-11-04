/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/UnitTest/AzToolsFrameworkTestHelpers.h>
#include <AzQtComponents/Components/Widgets/SpinBox.h>

#include <QApplication>
#include <QLineEdit>
#include <QWheelEvent>

namespace UnitTest
{
    using namespace AzToolsFramework;

    // Expose the LineEdit functionality so selection behavior can be more easily tested.
    class DoubleSpinBoxWithLineEdit
        : public AzQtComponents::DoubleSpinBox
    {
    public:
        // const required as lineEdit() is const
        QLineEdit* GetLineEdit() const { return lineEdit(); }
    };

    // A fixture to help test the int and double spin boxes.
    class SpinBoxFixture
        : public ToolsApplicationFixture<>
    {
    public:
        void SetUpEditorFixtureImpl() override
        {
            // note: must set a widget as the active window and add widgets
            // as children to ensure focus in/out events fire correctly
            m_dummyWidget = AZStd::make_unique<QWidget>();
            // Give the test window a valid windowHandle. SpinBox code uses this to access the QScreen
            m_dummyWidget->winId();
            QApplication::setActiveWindow(m_dummyWidget.get());

            m_intSpinBox = AZStd::make_unique<AzQtComponents::SpinBox>();
            m_doubleSpinBox = AZStd::make_unique<AzQtComponents::DoubleSpinBox>();
            m_doubleSpinBoxWithLineEdit = AZStd::make_unique<DoubleSpinBoxWithLineEdit>();

            m_spinBoxes = { m_intSpinBox.get(), m_doubleSpinBox.get(), m_doubleSpinBoxWithLineEdit.get() };
            for (auto spinBox : m_spinBoxes)
            {
                // Polish is required to set up the SpinBoxWatcher event filter
                spinBox->ensurePolished();
                spinBox->setParent(m_dummyWidget.get());
                spinBox->setKeyboardTracking(false);
                spinBox->setFocusPolicy(Qt::StrongFocus);
                spinBox->clearFocus();
            }
        }

        void TearDownEditorFixtureImpl() override
        {
            QApplication::setActiveWindow(nullptr);

            // Regenerate this list in case any of them were deleted during the test
            m_spinBoxes = { m_intSpinBox.get(), m_doubleSpinBox.get(), m_doubleSpinBoxWithLineEdit.get() };

            for (auto spinBox : m_spinBoxes)
            {
                if (spinBox)
                {
                    spinBox->setParent(nullptr);
                }
            }

            m_dummyWidget.reset();
            m_doubleSpinBoxWithLineEdit.reset();
            m_doubleSpinBox.reset();
            m_intSpinBox.reset();
        }

        QString setupTruncationTest(QString textValue)
        {
            QString retval;
            m_doubleSpinBoxWithLineEdit->setDecimals(7);
            m_doubleSpinBoxWithLineEdit->setDisplayDecimals(3);
            m_doubleSpinBoxWithLineEdit->setFocus();
            m_doubleSpinBoxWithLineEdit->GetLineEdit()->setText(textValue);
            m_doubleSpinBoxWithLineEdit->clearFocus();

            return m_doubleSpinBoxWithLineEdit->textFromValue(m_doubleSpinBoxWithLineEdit->value());
        }


        AZStd::unique_ptr<QWidget> m_dummyWidget;
        AZStd::unique_ptr<AzQtComponents::SpinBox> m_intSpinBox;
        AZStd::unique_ptr<AzQtComponents::DoubleSpinBox> m_doubleSpinBox;
        AZStd::unique_ptr<DoubleSpinBoxWithLineEdit> m_doubleSpinBoxWithLineEdit;

        AZStd::vector<QAbstractSpinBox*> m_spinBoxes;
    };

    TEST_F(SpinBoxFixture, SpinBoxesCreated)
    {
        using ::testing::Ne;

        EXPECT_THAT(m_intSpinBox, Ne(nullptr));
        EXPECT_THAT(m_doubleSpinBox, Ne(nullptr));
        EXPECT_THAT(m_doubleSpinBoxWithLineEdit, Ne(nullptr));
    }

    TEST_F(SpinBoxFixture, SpinBoxMousePressAndMoveRightScrollsValue)
    {
        m_doubleSpinBox->setValue(10.0);

        const int halfWidgetHeight = m_doubleSpinBox->height() / 2;
        const QPoint widgetCenterLeftBorder = m_doubleSpinBox->pos() + QPoint(1, halfWidgetHeight);

        // Check we have a valid window setup before moving the cursor
        EXPECT_TRUE(m_doubleSpinBox->window()->windowHandle() != nullptr);

        // Right in screen space
        MousePressAndMove(m_doubleSpinBox.get(), widgetCenterLeftBorder, QPoint(11, 0));

        // AzQtComponents::SpinBox::Config.pixelsPerStep is 10
        EXPECT_NEAR(m_doubleSpinBox->value(), 11.0, 0.001);
    }

    TEST_F(SpinBoxFixture, SpinBoxMousePressAndMoveLeftScrollsValue)
    {
        m_doubleSpinBox->setValue(10.0);

        const int halfWidgetHeight = m_doubleSpinBox->height() / 2;
        const QPoint widgetCenterLeftBorder = m_doubleSpinBox->pos() + QPoint(1, halfWidgetHeight);

        // Check we have a valid window setup before moving the cursor
        EXPECT_TRUE(m_doubleSpinBox->window()->windowHandle() != nullptr);

        // Left in screen space
        MousePressAndMove(m_doubleSpinBox.get(), widgetCenterLeftBorder, QPoint(-11, 0));

        // AzQtComponents::SpinBox::Config.pixelsPerStep is 10
        EXPECT_NEAR(m_doubleSpinBox->value(), 9.0, 0.001);
    }

    TEST_F(SpinBoxFixture, SpinBoxKeyboardUpAndDownArrowsChangeValue)
    {
        m_intSpinBox->setValue(5);
        m_intSpinBox->setFocus();

        QTest::keyClick(m_intSpinBox.get(), Qt::Key_Up, Qt::NoModifier);

        EXPECT_EQ(m_intSpinBox->value(), 6);

        QTest::keyClick(m_intSpinBox.get(), Qt::Key_Down, Qt::NoModifier);
        QTest::keyClick(m_intSpinBox.get(), Qt::Key_Down, Qt::NoModifier);

        EXPECT_EQ(m_intSpinBox->value(), 4);
    }

    TEST_F(SpinBoxFixture, SpinBoxChangeContentsAndEnterCommitsNewValue)
    {
        m_doubleSpinBoxWithLineEdit->setValue(10.0);
        m_doubleSpinBoxWithLineEdit->setFocus();
        m_doubleSpinBoxWithLineEdit->GetLineEdit()->setText(QString("15"));

        QTest::keyClick(m_doubleSpinBoxWithLineEdit.get(), Qt::Key_Enter, Qt::NoModifier);

        EXPECT_NEAR(m_doubleSpinBoxWithLineEdit->value(), 15.0, 0.001);
    }

    TEST_F(SpinBoxFixture, SpinBoxChangeContentsAndLoseFocusCommitsNewValue)
    {
        m_doubleSpinBoxWithLineEdit->setValue(10.0);
        m_doubleSpinBoxWithLineEdit->setFocus();
        m_doubleSpinBoxWithLineEdit->GetLineEdit()->setText(QString("15"));

        m_doubleSpinBoxWithLineEdit->clearFocus();

        EXPECT_NEAR(m_doubleSpinBoxWithLineEdit->value(), 15.0, 0.001);
    }

    TEST_F(SpinBoxFixture, SpinBoxClearContentsAndEscapeReturnsToPreviousValue)
    {
        m_doubleSpinBoxWithLineEdit->setValue(10.0);
        m_doubleSpinBoxWithLineEdit->setFocus();
        m_doubleSpinBoxWithLineEdit->GetLineEdit()->clear();

        QTest::keyClick(m_doubleSpinBoxWithLineEdit.get(), Qt::Key_Escape, Qt::NoModifier);

        EXPECT_NEAR(m_doubleSpinBoxWithLineEdit->value(), 10.0, 0.001);
    }

    TEST_F(SpinBoxFixture, SpinBoxChangeContentsAndEscapeReturnsToPreviousValue)
    {
        m_doubleSpinBoxWithLineEdit->setValue(10.0);
        m_doubleSpinBoxWithLineEdit->setFocus();
        m_doubleSpinBoxWithLineEdit->GetLineEdit()->setText(QString("15"));

        QTest::keyClick(m_doubleSpinBoxWithLineEdit.get(), Qt::Key_Escape, Qt::NoModifier);

        EXPECT_NEAR(m_doubleSpinBoxWithLineEdit->value(), 10.0, 0.001);
        EXPECT_TRUE(m_doubleSpinBoxWithLineEdit->GetLineEdit()->hasSelectedText());
    }

    TEST_F(SpinBoxFixture, SpinBoxSelectContentsAndEscapeKeepsFocus)
    {
        m_doubleSpinBox->setValue(10.0);
        m_doubleSpinBox->setFocus();
        m_doubleSpinBox->selectAll();

        QTest::keyClick(m_doubleSpinBox.get(), Qt::Key_Escape, Qt::NoModifier);

        EXPECT_TRUE(m_doubleSpinBox->hasFocus());

        QTest::keyClick(m_doubleSpinBox.get(), Qt::Key_Escape, Qt::NoModifier);

        EXPECT_TRUE(m_doubleSpinBox->hasFocus());
    }

    TEST_F(SpinBoxFixture, SpinBoxSuffixRemovedAndAppliedWithFocusChange)
    {
        using testing::StrEq;

        QLocale testLocale{ QLocale() };
        QString testString = "10" + QString(testLocale.decimalPoint()) + "0";

        m_doubleSpinBox->setSuffix("m");
        m_doubleSpinBox->setValue(10.0);

        // test internal logic (textFromValue() calls private StringValue())
        QString value = m_doubleSpinBox->textFromValue(10.0);
        EXPECT_THAT(value.toUtf8().constData(), testString);

        m_doubleSpinBox->setFocus();
        EXPECT_THAT(m_doubleSpinBox->suffix().toUtf8().constData(), StrEq(""));

        m_doubleSpinBox->clearFocus();
        EXPECT_THAT(m_doubleSpinBox->suffix().toUtf8().constData(), StrEq("m"));
    }

    // There is logic in our AzQtComponents::SpinBoxWatcher that delays processing of the end of wheel
    // events by 100msec, which used to result in a crash if the SpinBox happened to be deleted after
    // the timer was started and before it was triggered. This test was added to ensure the new handling
    // works correctly by no longer crashing in this scenario.
    TEST_F(SpinBoxFixture, SpinBoxClearDelayedWheelTimeoutAfterDelete)
    {
        // The wheel movement logic won't be triggered unless the SpinBox is focused at the start
        m_intSpinBox->setFocus();

        // Simulate the mouse wheel scrolling
        // The delta for the wheel changing doesn't matter, it just needs to be different
        auto delta = QPoint(10, 10);
        auto spinBox = m_intSpinBox.get();
        QWheelEvent wheelEventBegin(QPoint(), QPoint(), QPoint(), QPoint(), Qt::NoButton, Qt::NoModifier, Qt::ScrollBegin, false);
        QWheelEvent wheelEventUpdate(delta, delta, delta, delta, Qt::NoButton, Qt::NoModifier, Qt::ScrollUpdate, false);
        QWheelEvent wheelEventEnd(QPoint(), QPoint(), QPoint(), QPoint(), Qt::NoButton, Qt::NoModifier, Qt::ScrollEnd, false);
        QApplication::sendEvent(spinBox, &wheelEventBegin);
        QApplication::sendEvent(spinBox, &wheelEventUpdate);
        QApplication::sendEvent(spinBox, &wheelEventEnd);

        // Delete the SpinBox after triggering the mouse wheel scroll
        m_intSpinBox.reset();

        // The timeout in question is triggered 100msec after the mouse wheel has been moved
        // Waiting 200msec here to make sure it has been triggered
        QTest::qWait(200);

        // Verifying the SpinBox was deleted, although the true verification is that before the fix this
        // test would result in a crash
        EXPECT_TRUE(m_intSpinBox.get() == nullptr);
    }

    TEST_F(SpinBoxFixture, SpinBoxCheckHighValueTruncatesCorrectly)
    {
        QLocale testLocale{ QLocale() };
        QString testString = "0" + QString(testLocale.decimalPoint()) + "9999999";
        QString value = setupTruncationTest(testString);

        testString = "1" + QString(testLocale.decimalPoint()) + "0";
        EXPECT_TRUE(value == testString);
    }

    TEST_F(SpinBoxFixture, SpinBoxCheckLowValueTruncatesCorrectly)
    {
        QLocale testLocale{ QLocale() };
        QString testString = "0" + QString(testLocale.decimalPoint()) + "0000001";
        QString value = setupTruncationTest(testString);

        testString = "0" + QString(testLocale.decimalPoint()) + "0";
        EXPECT_TRUE(value == testString);
    }

    TEST_F(SpinBoxFixture, SpinBoxCheckBugValuesTruncatesCorrectly)
    {
        QLocale testLocale{ QLocale() };
        QString testString = "0" + QString(testLocale.decimalPoint()) + "12395";
        QString value = setupTruncationTest(testString);

        testString = "0" + QString(testLocale.decimalPoint()) + "124";
        EXPECT_TRUE(value == testString);

        testString = "0" + QString(testLocale.decimalPoint()) + "94496";
        value = setupTruncationTest(testString);

        testString = "0" + QString(testLocale.decimalPoint()) + "945";
        EXPECT_TRUE(value == testString);

        testString = "0" + QString(testLocale.decimalPoint()) + "0009999";
        value = setupTruncationTest(testString);

        testString = "0" + QString(testLocale.decimalPoint()) + "001";
        EXPECT_TRUE(value == testString);
    }

} // namespace UnitTest
