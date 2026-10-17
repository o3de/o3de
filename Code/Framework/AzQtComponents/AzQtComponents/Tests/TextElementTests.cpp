/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzTest/AzTest.h>
#include <AzQtComponents/Components/Elements/TextElement.h>
#include <AzQtComponents/Components/Style.h>
#include <AzQtComponents/Components/StyleManager.h>

#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>

#include <QApplication>
#include <QDateTimeEdit>
#include <QLabel>
#include <QLineEdit>
#include <QSpinBox>
#include <QStatusBar>
#include <QStyleFactory>

namespace AzQtComponents
{
    // ----------------------------------------------------------------------------------
    // Fixture: an application with the fonts it has when StyleManager calls setStyle
    // ----------------------------------------------------------------------------------

    class TextElementTests
        : public ::UnitTest::LeakDetectionFixture
    {
    public:
        TextElementTests()
        {
            m_application = AZStd::make_unique<QApplication>(m_argc, m_argv);

            // The application font (StyleManager.cpp:220-222)
            QFont applicationFont(QStringLiteral("Open Sans"));
            applicationFont.setPixelSize(12);
            QApplication::setFont(applicationFont);

            // A class font the platform supplies, as Windows does for QStatusBar
            QApplication::setFont(QFont(QStringLiteral("Segoe UI"), 9), "QStatusBar");
        }

        ~TextElementTests() override
        {
            m_application.reset();
        }

        //! Polished, then the element over it (Style::polish applies it during the polish: the same font)
        static void polishAndApply(QWidget* widget)
        {
            widget->ensurePolished();
            TextElement::apply(widget);
        }

    protected:
        int m_argc = 1;                                     // QApplication keeps a reference to it
        char m_programName[5] = "test";
        char* m_argv[2] = { m_programName, nullptr };
        AZStd::unique_ptr<QApplication> m_application;
    };

    // ----------------------------------------------------------------------------------
    // T1: class fonts, and the Qt rule they rest on
    // ----------------------------------------------------------------------------------

    TEST_F(TextElementTests, InstallClassFonts_BaseEqualsTheApplicationFontButIsNotACopy)
    {
        TextElement::installClassFonts();

        const QFont base = QApplication::font("QWidget");
        EXPECT_TRUE(base == QApplication::font());
        EXPECT_FALSE(base.isCopyOf(QApplication::font()));
    }

    TEST_F(TextElementTests, InstallClassFonts_PlatformClassFontKeepsItsSizeUnderStar)
    {
        TextElement::installClassFonts();

        const QFont statusBar = QApplication::font("QStatusBar");
        EXPECT_TRUE(statusBar.family() == QStringLiteral("Open Sans"));
        EXPECT_EQ(statusBar.pointSize(), 9);
        EXPECT_EQ(statusBar.weight(), QFont::Normal);
    }

    TEST_F(TextElementTests, InstallClassFonts_MenuBarAndProgressBarWhateverThePlatformSupplies)
    {
        TextElement::installClassFonts();

        const QFont menuBar = QApplication::font("QMenuBar");
        EXPECT_TRUE(menuBar.family() == QStringLiteral("Amazon Ember"));
        EXPECT_EQ(menuBar.pixelSize(), 12);
        EXPECT_EQ(QApplication::font("QProgressBar").pixelSize(), 1);
    }

    TEST_F(TextElementTests, InstallClassFonts_LeavesTheKeysQtReadsItself)
    {
        QFont titleBarFont(QStringLiteral("Open Sans"));
        titleBarFont.setPixelSize(18);
        QApplication::setFont(titleBarFont, "QMdiSubWindowTitleBar"); // as StyleManager.cpp:343-345
        const QFont before = QApplication::font("QMdiSubWindowTitleBar");

        TextElement::installClassFonts();

        EXPECT_TRUE(QApplication::font("QMdiSubWindowTitleBar").isCopyOf(before));
    }

    TEST_F(TextElementTests, InstallClassFonts_AddsNoFontForAClassThePlatformLeavesOut)
    {
        const bool supplied = !QApplication::font("QHeaderView").isCopyOf(QApplication::font());

        TextElement::installClassFonts();

        EXPECT_EQ(!QApplication::font("QHeaderView").isCopyOf(QApplication::font()), supplied);
    }

    TEST_F(TextElementTests, IdentityRule_ChildOfAClassFontParentTakesTheBase)
    {
        // The base equals the application font in value: only identity tells it apart
        // (QFont::isCopyOf, qwidget.cpp:4758-4766). If this fails, Qt no longer compares by identity.
        TextElement::installClassFonts();

        QStatusBar statusBar;
        QLabel* label = new QLabel(&statusBar);
        statusBar.ensurePolished();

        EXPECT_EQ(statusBar.font().pointSize(), 9);
        EXPECT_EQ(label->font().pixelSize(), 12);
        EXPECT_TRUE(label->font().family() == QStringLiteral("Open Sans"));
    }

    TEST_F(TextElementTests, IdentityRule_ACopyOfTheApplicationFontIsNoClassFont)
    {
        // Why the base is a font of its own: as a copy, the child takes its parent's whole font
        QApplication::setFont(QApplication::font(), "QWidget");

        QStatusBar statusBar;
        QLabel* label = new QLabel(&statusBar);
        statusBar.ensurePolished();

        EXPECT_EQ(label->font().pointSize(), 9);
        EXPECT_TRUE(label->font().family() == QStringLiteral("Segoe UI"));
    }

    // ----------------------------------------------------------------------------------
    // T2: widget elements
    // ----------------------------------------------------------------------------------

    TEST_F(TextElementTests, Apply_LeavesAWidgetNothingReaches)
    {
        TextElement::installClassFonts();

        QLabel label;
        polishAndApply(&label);

        EXPECT_FALSE(label.testAttribute(Qt::WA_SetFont));
        EXPECT_EQ(label.font().pixelSize(), 12);
    }

    TEST_F(TextElementTests, Apply_TierTagIsAWholeWordComparedWithCase)
    {
        TextElement::installClassFonts();

        QLabel title;
        title.setProperty("class", QStringLiteral("Title"));
        polishAndApply(&title);
        EXPECT_EQ(title.font().pixelSize(), 18);

        QLabel lowerCase;
        lowerCase.setProperty("class", QStringLiteral("title")); // Style::hasClass took it for .Title
        polishAndApply(&lowerCase);
        EXPECT_EQ(lowerCase.font().pixelSize(), 12);

        QLabel twoTags;
        twoTags.setProperty("class", QStringLiteral("Title Paragraph")); // the later declaration (Text.qss:100)
        polishAndApply(&twoTags);
        EXPECT_EQ(twoTags.font().pixelSize(), 12);
    }

    TEST_F(TextElementTests, Apply_RulesOverACodeFont)
    {
        TextElement::installClassFonts();
        const QFont codeFont(QStringLiteral("Consolas"), 14);

        QLineEdit lineEdit; // BaseStyleSheet.qss:82 gives it a size
        lineEdit.setFont(codeFont);
        polishAndApply(&lineEdit);
        EXPECT_TRUE(lineEdit.font().family() == QStringLiteral("Open Sans"));
        EXPECT_EQ(lineEdit.font().pixelSize(), 12);

        QSpinBox spinBox;
        spinBox.setFont(codeFont);
        polishAndApply(&spinBox);
        EXPECT_EQ(spinBox.font().pixelSize(), 12);

        QDateTimeEdit dateTimeEdit; // a QAbstractSpinBox no rule gives a size: the code's stays
        dateTimeEdit.setFont(codeFont);
        polishAndApply(&dateTimeEdit);
        EXPECT_TRUE(dateTimeEdit.font().family() == QStringLiteral("Open Sans"));
        EXPECT_EQ(dateTimeEdit.font().pointSize(), 14);
    }

    TEST_F(TextElementTests, Apply_RulesOverTheCodeFontOfAnAncestor)
    {
        TextElement::installClassFonts();

        QWidget panel;
        panel.setFont(QFont(QStringLiteral("Consolas"), 14));
        QLabel* label = new QLabel(&panel);
        QWidget* plain = new QWidget(&panel);
        panel.ensurePolished();
        TextElement::apply(&panel);
        TextElement::apply(label);
        TextElement::apply(plain);

        EXPECT_EQ(label->font().pixelSize(), 12);   // the QLabel rule (Text.qss:92)
        EXPECT_TRUE(plain->font().family() == QStringLiteral("Open Sans"));
        EXPECT_EQ(plain->font().pointSize(), 14);   // no size rule: the inherited size stays
    }

    // ----------------------------------------------------------------------------------
    // T3: wired as the flattened GUI runs it, through Style
    // ----------------------------------------------------------------------------------

    TEST_F(TextElementTests, Wiring_StylePolishAppliesTheElementOfEveryWidget)
    {
        ASSERT_TRUE(StyleManager::stylesheetsDisabled()); // the test environment sets it (AzQtComponentTests.cpp)

        // Style::polish(QApplication*) installs the class fonts; Style::polish(QWidget*) applies the elements
        QApplication::setStyle(new Style(QStyleFactory::create(QStringLiteral("Fusion"))));

        QLabel title;
        title.setProperty("class", QStringLiteral("Title"));
        title.ensurePolished();
        EXPECT_EQ(title.font().pixelSize(), 18);

        QLabel plain;
        plain.ensurePolished();
        EXPECT_EQ(plain.font().pixelSize(), 12);
        EXPECT_FALSE(plain.testAttribute(Qt::WA_SetFont));
    }

    namespace
    {
        const TextElement TestModuleTitle{ .m_name = "Test.Text.ModuleTitle", .m_pixelSize = 20 };

        bool isTestModuleTitle(const QWidget* widget)
        {
            return TextElement::named(widget, "textElementTestsModuleTitle");
        }
    } // namespace

    TEST_F(TextElementTests, Apply_ModuleElementBeforeTheGlobalOnes)
    {
        TextElement::installClassFonts();
        TextElement::registerElement(&TestModuleTitle, &QLabel::staticMetaObject, &isTestModuleTitle);

        QLabel label;
        label.setObjectName(QStringLiteral("textElementTestsModuleTitle"));
        label.setProperty("class", QStringLiteral("Title"));
        polishAndApply(&label);

        EXPECT_EQ(label.font().pixelSize(), 20);
    }
} // namespace AzQtComponents
