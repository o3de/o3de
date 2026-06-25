/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/UnitTest/AzToolsFrameworkTestHelpers.h>
#include <AzToolsFramework/UI/PropertyEditor/GrowTextEdit.h>

#include <QApplication>
#include <QTest>

namespace UnitTest
{
    using namespace AzToolsFramework;

    // =========================================================================
    // Fixture
    //
    // GrowTextEdit is a QTextEdit subclass used for multi-line string fields
    // in the property editor. It implements its own Esc / Ctrl+Z revert-and-exit
    // behavior in event() and keyPressEvent() (not via the shared
    // LineEditRevertHandler, because QTextEdit is not a QLineEdit).
    // =========================================================================
    class GrowTextEditFixture
        : public ToolsApplicationFixture<>
    {
    public:
        void SetUpEditorFixtureImpl() override
        {
            // Qt6 will not deliver QEvent::FocusIn to a widget whose top-level
            // has never been shown, even on the offscreen platform. Show before
            // activating so focus events reach the child correctly.
            m_dummyWidget = AZStd::make_unique<QWidget>();
            m_dummyWidget->winId();

            m_textEdit = AZStd::make_unique<GrowTextEdit>(m_dummyWidget.get());
            m_textEdit->setFocusPolicy(Qt::StrongFocus);
            // ensurePolished() forces full internal-state initialization on the
            // child. Without it, Linux offscreen QPA does not deliver
            // QEvent::FocusIn to a freshly created QTextEdit subclass.
            m_textEdit->ensurePolished();

            m_dummyWidget->show();
            m_dummyWidget->activateWindow();
            // qWaitForWindowExposed is the canonical Qt synchronization for
            // "window is ready for input". Required on Linux offscreen where
            // processEvents() alone is not enough for show() to fully expose.
            // Return is [[nodiscard]] in Qt6; offscreen QPA may legitimately
            // return false, so the value is discarded rather than asserted.
            (void)QTest::qWaitForWindowExposed(m_dummyWidget.get());
            // Some QPA plugins (including Linux offscreen) auto-focus the
            // first focusable child when the top-level is shown, firing
            // FocusIn before the test body can set the value it expects to
            // capture. Clear focus so each test body's setFocus() delivers a
            // fresh FocusIn against the just-set text.
            m_textEdit->clearFocus();
            QApplication::processEvents();
        }

        void TearDownEditorFixtureImpl() override
        {
            m_textEdit.reset();
            m_dummyWidget.reset();
        }

        // Revert path defers clearFocus via QTimer::singleShot(0); drain the
        // event loop before asserting focus state.
        static void FlushDeferred()
        {
            QApplication::processEvents();
        }

        AZStd::unique_ptr<QWidget> m_dummyWidget;
        AZStd::unique_ptr<GrowTextEdit> m_textEdit;
    };

    // =========================================================================
    // Escape reverts to focus-in text and exits
    // =========================================================================
    TEST_F(GrowTextEditFixture, EscapeRevertsAndExitsField)
    {
        m_textEdit->setPlainText(QStringLiteral("initial"));
        m_textEdit->setFocus();
        m_textEdit->setPlainText(QStringLiteral("changed"));

        QTest::keyClick(m_textEdit.get(), Qt::Key_Escape, Qt::NoModifier);
        FlushDeferred();

        EXPECT_EQ(m_textEdit->toPlainText(), QStringLiteral("initial"));
        EXPECT_FALSE(m_textEdit->hasFocus());
    }

    // =========================================================================
    // Revert path must not report an edit to the owner
    //
    // GrowTextEdit emits EditCompleted on focusOutEvent when the text changed.
    // A revert that causes focus-out must reset the dirty flag so no spurious
    // "edit finished" signal fires.
    // =========================================================================
    TEST_F(GrowTextEditFixture, EscapeDoesNotEmitEditCompleted)
    {
        m_textEdit->setPlainText(QStringLiteral("initial"));
        m_textEdit->setFocus();
        m_textEdit->setPlainText(QStringLiteral("changed"));

        int editCompletedCount = 0;
        QObject::connect(m_textEdit.get(), &GrowTextEdit::EditCompleted, m_textEdit.get(),
            [&editCompletedCount]() { ++editCompletedCount; });

        QTest::keyClick(m_textEdit.get(), Qt::Key_Escape, Qt::NoModifier);
        FlushDeferred();

        EXPECT_EQ(editCompletedCount, 0);
    }

    // =========================================================================
    // Ctrl+Z reverts and exits only when text was changed
    // =========================================================================
    TEST_F(GrowTextEditFixture, CtrlZRevertsAndExitsFieldWhenTextChanged)
    {
        m_textEdit->setPlainText(QStringLiteral("initial"));
        m_textEdit->setFocus();
        m_textEdit->setPlainText(QStringLiteral("changed"));

        QTest::keyClick(m_textEdit.get(), Qt::Key_Z, Qt::ControlModifier);
        FlushDeferred();

        EXPECT_EQ(m_textEdit->toPlainText(), QStringLiteral("initial"));
        EXPECT_FALSE(m_textEdit->hasFocus());
    }

    TEST_F(GrowTextEditFixture, CtrlZDoesNotExitFieldWhenTextUnchanged)
    {
        m_textEdit->setPlainText(QStringLiteral("initial"));
        m_textEdit->setFocus();

        QTest::keyClick(m_textEdit.get(), Qt::Key_Z, Qt::ControlModifier);
        FlushDeferred();

        EXPECT_EQ(m_textEdit->toPlainText(), QStringLiteral("initial"));
        EXPECT_TRUE(m_textEdit->hasFocus());
    }

} // namespace UnitTest
