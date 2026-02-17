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
            m_dummyWidget = AZStd::make_unique<QWidget>();
            m_dummyWidget->winId();
            m_dummyWidget->activateWindow();

            m_textEdit = AZStd::make_unique<GrowTextEdit>(m_dummyWidget.get());
            m_textEdit->setFocusPolicy(Qt::StrongFocus);
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
