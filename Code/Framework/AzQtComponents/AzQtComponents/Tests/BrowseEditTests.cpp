/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzTest/AzTest.h>
#include <AzQtComponents/Components/Style.h>
#include <AzQtComponents/Components/StyleManager.h>
#include <AzQtComponents/Components/Widgets/BrowseEdit.h>

#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>

#include <QApplication>
#include <QIntValidator>
#include <QLineEdit>
#include <QStyleFactory>
#include <QtGlobal>

namespace AzQtComponents
{
    // ----------------------------------------------------------------------------------
    // Fixture: the flattened style chain (Style -> Fusion) under an application
    // ----------------------------------------------------------------------------------

    class BrowseEditTests
        : public ::UnitTest::LeakDetectionFixture
    {
    public:
        BrowseEditTests()
        {
            m_application = AZStd::make_unique<QApplication>(m_argc, m_argv);

            // As StyleManager::initialize builds it (StyleManager.cpp:244-246); the application owns it
            QApplication::setStyle(new Style(QStyleFactory::create(QStringLiteral("Fusion"))));
        }

        ~BrowseEditTests() override
        {
            m_application.reset();
        }

    protected:
        int m_argc = 1;                                     // QApplication keeps a reference to it
        char m_programName[5] = "test";
        char* m_argv[2] = { m_programName, nullptr };
        AZStd::unique_ptr<QApplication> m_application;
    };

    // ----------------------------------------------------------------------------------
    // T1: the palette of the line edit inside a BrowseEdit, state by state. Each value is the
    // one the legacy GUI finally rendered (the Native dumps of Gallery 02_browse-edit, Editor
    // 02_main_entity_selected and the Atom tools' Viewport Settings docks).
    // ----------------------------------------------------------------------------------

    TEST_F(BrowseEditTests, FieldPalette_EnabledIsBlackTextOnLightGrey)
    {
        ASSERT_TRUE(StyleManager::stylesheetsDisabled());

        BrowseEdit browseEdit;
        const QPalette palette = browseEdit.lineEdit()->palette();

        // BaseStyleSheet.qss:76-77; the placeholder is the text at half alpha
        for (const auto group : { QPalette::Active, QPalette::Inactive })
        {
            EXPECT_TRUE(palette.color(group, QPalette::Base) == QColor(0xCC, 0xCC, 0xCC));
            EXPECT_TRUE(palette.color(group, QPalette::Text) == QColor(0x00, 0x00, 0x00));
            EXPECT_TRUE(palette.color(group, QPalette::PlaceholderText) == QColor(0x00, 0x00, 0x00, 0x80));
        }
    }

    TEST_F(BrowseEditTests, FieldPalette_DisabledIsGreyTextOnDarkGrey)
    {
        ASSERT_TRUE(StyleManager::stylesheetsDisabled());

        BrowseEdit browseEdit;
        const QPalette palette = browseEdit.lineEdit()->palette();

        // LineEdit.qss:33-34; the placeholder is the text at half alpha
        EXPECT_TRUE(palette.color(QPalette::Disabled, QPalette::Base) == QColor(0x66, 0x66, 0x66));
        EXPECT_TRUE(palette.color(QPalette::Disabled, QPalette::Text) == QColor(0x99, 0x99, 0x99));
        EXPECT_TRUE(palette.color(QPalette::Disabled, QPalette::PlaceholderText) == QColor(0x99, 0x99, 0x99, 0x80));
    }

    TEST_F(BrowseEditTests, FieldPalette_DisabledWidgetPaintsFromTheDisabledGroup)
    {
        ASSERT_TRUE(StyleManager::stylesheetsDisabled());

        BrowseEdit browseEdit;
        browseEdit.setEnabled(false);

        // QLineEdit paints the field from Base and the text from Text of the group the widget is in
        const QPalette palette = browseEdit.lineEdit()->palette();
        EXPECT_EQ(palette.currentColorGroup(), QPalette::Disabled);
        EXPECT_TRUE(palette.color(QPalette::Base) == QColor(0x66, 0x66, 0x66));
        EXPECT_TRUE(palette.color(QPalette::Text) == QColor(0x99, 0x99, 0x99));
    }

    TEST_F(BrowseEditTests, FieldPalette_DropTargetFieldHoldsInEveryState)
    {
        ASSERT_TRUE(StyleManager::stylesheetsDisabled());

        BrowseEdit browseEdit;
        QLineEdit* lineEdit = browseEdit.lineEdit();

        // BrowseEdit.qss:35 and :41 outrank QLineEdit:disabled
        Style::addClass(&browseEdit, ValidDropTarget);
        for (const auto group : { QPalette::Active, QPalette::Inactive, QPalette::Disabled })
        {
            EXPECT_TRUE(lineEdit->palette().color(group, QPalette::Base) == QColor(Qt::transparent));
        }
        EXPECT_TRUE(lineEdit->palette().color(QPalette::Active, QPalette::Text) == QColor(0x00, 0x00, 0x00));
        Style::removeClass(&browseEdit, ValidDropTarget);

        Style::addClass(&browseEdit, InvalidDropTarget);
        for (const auto group : { QPalette::Active, QPalette::Inactive, QPalette::Disabled })
        {
            EXPECT_TRUE(lineEdit->palette().color(group, QPalette::Base) == QColor(0xFF, 0xFF, 0xFF));
        }
        EXPECT_TRUE(lineEdit->palette().color(QPalette::Active, QPalette::Text) == QColor(0x00, 0x00, 0x00));

        // Without the class the plain field is back
        Style::removeClass(&browseEdit, InvalidDropTarget);
        EXPECT_TRUE(lineEdit->palette().color(QPalette::Active, QPalette::Base) == QColor(0xCC, 0xCC, 0xCC));
        EXPECT_TRUE(lineEdit->palette().color(QPalette::Disabled, QPalette::Base) == QColor(0x66, 0x66, 0x66));
    }

    TEST_F(BrowseEditTests, FieldPalette_ReadOnlyAndErrorKeepTheEnabledField)
    {
        ASSERT_TRUE(StyleManager::stylesheetsDisabled());

        // No rule colors a read-only or an invalid line edit: the error is the frame BrowseEdit draws
        BrowseEdit browseEdit;
        browseEdit.setValidator(new QIntValidator(&browseEdit));
        browseEdit.setLineEditReadOnly(true);
        browseEdit.setText(QStringLiteral("Nan"));
        browseEdit.style()->polish(&browseEdit);

        ASSERT_FALSE(browseEdit.hasAcceptableInput());
        const QPalette palette = browseEdit.lineEdit()->palette();
        EXPECT_TRUE(palette.color(QPalette::Active, QPalette::Base) == QColor(0xCC, 0xCC, 0xCC));
        EXPECT_TRUE(palette.color(QPalette::Active, QPalette::Text) == QColor(0x00, 0x00, 0x00));
    }

    // ----------------------------------------------------------------------------------
    // T2: polish order. The widget's polish runs LineEdit::polish (its #888888 placeholder)
    // before the field palette; the line edit's own polish comes after the widget's.
    // ----------------------------------------------------------------------------------

    TEST_F(BrowseEditTests, FieldPalette_RepolishAndTheLineEditsOwnPolishKeepIt)
    {
        ASSERT_TRUE(StyleManager::stylesheetsDisabled());

        BrowseEdit browseEdit;
        QLineEdit* lineEdit = browseEdit.lineEdit();
        Style::addClass(&browseEdit, ValidDropTarget);

        browseEdit.style()->polish(&browseEdit);
        browseEdit.ensurePolished();
        lineEdit->style()->polish(lineEdit);

        EXPECT_TRUE(lineEdit->palette().color(QPalette::Active, QPalette::Base) == QColor(Qt::transparent));
        EXPECT_TRUE(lineEdit->palette().color(QPalette::Active, QPalette::PlaceholderText) == QColor(0x00, 0x00, 0x00, 0x80));
    }
} // namespace AzQtComponents
