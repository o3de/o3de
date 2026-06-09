/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/UnitTest/AzToolsFrameworkTestHelpers.h>
#include <AzQtComponents/Components/Widgets/LineEditRevertHandler.h>

#include <QApplication>
#include <QLineEdit>
#include <QTest>

namespace UnitTest
{
    using namespace AzToolsFramework;

    // =========================================================================
    // Fixture
    //
    // LineEditRevertHandler is the shared utility that gives any QLineEdit
    // Esc / Ctrl+Z revert-and-exit behavior. The four widgets that install it
    // (BrowseEdit, PropertyStringLineEditCtrl, PropertyColorCtrl, and the
    // EntityPropertyEditor name field) all share this exact code path, so the
    // behavior only needs to be verified once at the utility level.
    // =========================================================================
    class LineEditRevertHandlerFixture
        : public ToolsApplicationFixture<>
    {
    public:
        void SetUpEditorFixtureImpl() override
        {
            // Must have an active top-level window for focus events to fire.
            m_dummyWidget = AZStd::make_unique<QWidget>();
            m_dummyWidget->winId();
            QApplication::setActiveWindow(m_dummyWidget.get());

            m_lineEdit = new QLineEdit(m_dummyWidget.get());
            m_lineEdit->setFocusPolicy(Qt::StrongFocus);

            // Default the clear-focus target to the line edit itself. In real
            // consumers a parent widget can be passed when it owns the focus
            // (typically via setFocusProxy), but in this fixture the line edit
            // is what actually holds focus, so clearing it is what unfocuses.
            new AzQtComponents::LineEditRevertHandler(m_lineEdit);
        }

        void TearDownEditorFixtureImpl() override
        {
            QApplication::setActiveWindow(nullptr);
            m_lineEdit = nullptr; // owned by m_dummyWidget
            m_dummyWidget.reset();
        }

        // LineEditRevertHandler defers its clearFocus() via QTimer::singleShot(0)
        // so the event stack can unwind before focus moves. Tests must drain the
        // event loop to observe the final focus state.
        static void FlushDeferred()
        {
            QApplication::processEvents();
        }

        AZStd::unique_ptr<QWidget> m_dummyWidget;
        QLineEdit* m_lineEdit = nullptr;
    };

    // =========================================================================
    // Focus-In capture
    // =========================================================================
    TEST_F(LineEditRevertHandlerFixture, FocusInCapturesTextForLaterRevert)
    {
        m_lineEdit->setText(QStringLiteral("initial"));
        m_lineEdit->setFocus();
        m_lineEdit->setText(QStringLiteral("changed"));

        QTest::keyClick(m_lineEdit, Qt::Key_Escape, Qt::NoModifier);
        FlushDeferred();

        EXPECT_EQ(m_lineEdit->text(), QStringLiteral("initial"));
    }

    // =========================================================================
    // Escape always reverts and exits
    // =========================================================================
    TEST_F(LineEditRevertHandlerFixture, EscapeRevertsAndExitsField)
    {
        m_lineEdit->setText(QStringLiteral("initial"));
        m_lineEdit->setFocus();
        m_lineEdit->setText(QStringLiteral("changed"));

        QTest::keyClick(m_lineEdit, Qt::Key_Escape, Qt::NoModifier);
        FlushDeferred();

        EXPECT_EQ(m_lineEdit->text(), QStringLiteral("initial"));
        EXPECT_FALSE(m_lineEdit->hasFocus());
    }

    TEST_F(LineEditRevertHandlerFixture, EscapeExitsFieldEvenWithoutInProgressEdit)
    {
        m_lineEdit->setText(QStringLiteral("initial"));
        m_lineEdit->setFocus();

        QTest::keyClick(m_lineEdit, Qt::Key_Escape, Qt::NoModifier);
        FlushDeferred();

        EXPECT_EQ(m_lineEdit->text(), QStringLiteral("initial"));
        EXPECT_FALSE(m_lineEdit->hasFocus());
    }

    // =========================================================================
    // Ctrl+Z reverts and exits only when there is a local edit
    // =========================================================================
    TEST_F(LineEditRevertHandlerFixture, CtrlZRevertsAndExitsFieldWhenTextChanged)
    {
        m_lineEdit->setText(QStringLiteral("initial"));
        m_lineEdit->setFocus();
        m_lineEdit->setText(QStringLiteral("changed"));

        QTest::keyClick(m_lineEdit, Qt::Key_Z, Qt::ControlModifier);
        FlushDeferred();

        EXPECT_EQ(m_lineEdit->text(), QStringLiteral("initial"));
        EXPECT_FALSE(m_lineEdit->hasFocus());
    }

    TEST_F(LineEditRevertHandlerFixture, CtrlZDoesNotExitFieldWhenTextUnchanged)
    {
        // When there is no local edit, Ctrl+Z must bubble up to the global undo
        // stack instead of being swallowed. The handler stays hands-off.
        m_lineEdit->setText(QStringLiteral("initial"));
        m_lineEdit->setFocus();

        QTest::keyClick(m_lineEdit, Qt::Key_Z, Qt::ControlModifier);
        FlushDeferred();

        EXPECT_EQ(m_lineEdit->text(), QStringLiteral("initial"));
        EXPECT_TRUE(m_lineEdit->hasFocus());
    }

    // =========================================================================
    // Read-only lines bypass the handler entirely
    // =========================================================================
    TEST_F(LineEditRevertHandlerFixture, ReadOnlyLineEditDoesNotExitOnEscape)
    {
        m_lineEdit->setText(QStringLiteral("initial"));
        m_lineEdit->setReadOnly(true);
        m_lineEdit->setFocus();

        QTest::keyClick(m_lineEdit, Qt::Key_Escape, Qt::NoModifier);
        FlushDeferred();

        // Handler must not claim Escape on a read-only line edit; the field
        // keeps focus so the outer context (dialog, panel) can react.
        EXPECT_TRUE(m_lineEdit->hasFocus());
    }

} // namespace UnitTest
