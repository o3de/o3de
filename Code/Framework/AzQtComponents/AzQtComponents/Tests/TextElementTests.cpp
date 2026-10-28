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
#include <QComboBox>
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
            TextElement::resolve(widget);
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

    TEST_F(TextElementTests, InstallClassFonts_EveryClassARuleSizesHasAFontOfItsOwn)
    {
        // Each class keeps the element of its own rule, whatever the platform supplies: a font of its own, not the base
        TextElement::installClassFonts();

        const QFont base = QApplication::font("QWidget");
        for (const char* className : { "QLabel", "QTipLabel", "QPushButton", "QCheckBox", "QRadioButton", "QTextEdit", "QPlainTextEdit",
                 "QLineEdit", "QSpinBox", "QDoubleSpinBox", "QComboBox" })
        {
            const QFont font = QApplication::font(className);
            EXPECT_FALSE(font.isCopyOf(QApplication::font())) << className;
            EXPECT_FALSE(font.isCopyOf(base)) << className;
            EXPECT_TRUE(font.family() == QStringLiteral("Open Sans")) << className;
            EXPECT_EQ(font.pixelSize(), 12) << className;
            EXPECT_EQ(font.weight(), QFont::Normal) << className;
        }
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
        TextElement::resolve(&panel);
        TextElement::resolve(label);
        TextElement::resolve(plain);

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
            return Selectors::named(widget, "textElementTestsModuleTitle");
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

    // ----------------------------------------------------------------------------------
    // T4: the colour of a module's element, as the sheet put it into the palette
    // ----------------------------------------------------------------------------------

    namespace
    {
        // The looks of ProjectManager.qss:997-1001 (#CountLabel, #94d2ff) and :299-306 with :262 (a #formFrame field:
        // white, #aaaaaa disabled), under names of the tests' own
        const TextElement TestModuleLink{ .m_name = "Test.Text.ModuleLink", .m_color = QColor(0x94, 0xD2, 0xFF) };
        const TextElement TestModuleField{
            .m_name = "Test.Text.ModuleField", .m_color = QColor(0xFF, 0xFF, 0xFF), .m_disabledColor = QColor(0xAA, 0xAA, 0xAA) };

        bool isTestModuleLink(const QWidget* widget)
        {
            return Selectors::named(widget, "textElementTestsModuleLink");
        }

        bool isTestModuleField(const QWidget* widget)
        {
            return Selectors::named(widget, "textElementTestsModuleField");
        }

        //! The text roles a `color` sets in one group, and the placeholder at half the alpha (qstylesheetstyle.cpp:1454-1464)
        void expectForeground(const QPalette& palette, QPalette::ColorGroup group, const QColor& color)
        {
            EXPECT_TRUE(palette.color(group, QPalette::WindowText) == color);
            EXPECT_TRUE(palette.color(group, QPalette::Text) == color);
            EXPECT_TRUE(palette.color(group, QPalette::ButtonText) == color);
            QColor placeholder(color);
            placeholder.setAlpha(128);
            EXPECT_TRUE(palette.color(group, QPalette::PlaceholderText) == placeholder);
        }
    } // namespace

    TEST_F(TextElementTests, Color_EveryGroupTakesTheColourOfARuleWithoutDisabled)
    {
        TextElement::registerElement(&TestModuleLink, &QLabel::staticMetaObject, &isTestModuleLink);

        QLabel label;
        label.setObjectName(QStringLiteral("textElementTestsModuleLink"));
        polishAndApply(&label);
        TextElement::applyColor(&label);

        const QColor link(0x94, 0xD2, 0xFF);
        expectForeground(label.palette(), QPalette::Active, link);
        expectForeground(label.palette(), QPalette::Inactive, link);
        expectForeground(label.palette(), QPalette::Disabled, link);
    }

    TEST_F(TextElementTests, Color_DisabledColourIsTheDisabledGroupsOnly)
    {
        TextElement::registerElement(&TestModuleField, &QLineEdit::staticMetaObject, &isTestModuleField);

        QLineEdit field;
        field.setObjectName(QStringLiteral("textElementTestsModuleField"));
        polishAndApply(&field);
        TextElement::applyColor(&field);

        expectForeground(field.palette(), QPalette::Active, QColor(0xFF, 0xFF, 0xFF));
        expectForeground(field.palette(), QPalette::Inactive, QColor(0xFF, 0xFF, 0xFF));
        expectForeground(field.palette(), QPalette::Disabled, QColor(0xAA, 0xAA, 0xAA));
    }

    TEST_F(TextElementTests, Color_AnElementWithoutOneLeavesThePalette)
    {
        TextElement::installClassFonts();
        TextElement::registerElement(&TestModuleTitle, &QLabel::staticMetaObject, &isTestModuleTitle);

        QLabel label;
        label.setObjectName(QStringLiteral("textElementTestsModuleTitle"));
        const QPalette before = label.palette();
        polishAndApply(&label);
        TextElement::applyColor(&label);

        EXPECT_FALSE(label.testAttribute(Qt::WA_SetPalette));
        EXPECT_TRUE(label.palette() == before);
    }

    TEST_F(TextElementTests, Color_WiredLastOverTheGlobalPaletteOfTheFlattenedPolish)
    {
        ASSERT_TRUE(StyleManager::stylesheetsDisabled()); // the test environment sets it (AzQtComponentTests.cpp)
        QApplication::setStyle(new Style(QStyleFactory::create(QStringLiteral("Fusion"))));
        TextElement::registerElement(&TestModuleField, &QLineEdit::staticMetaObject, &isTestModuleField);

        // The flattened polish gives an entry the black text of LineEdit.qss; the module's rule won over it in the sheet
        QLineEdit plain;
        plain.ensurePolished();
        ASSERT_TRUE(plain.palette().color(QPalette::Active, QPalette::Text) == QColor(Qt::black));

        QLineEdit field;
        field.setObjectName(QStringLiteral("textElementTestsModuleField"));
        field.ensurePolished();
        expectForeground(field.palette(), QPalette::Active, QColor(0xFF, 0xFF, 0xFF));
        expectForeground(field.palette(), QPalette::Disabled, QColor(0xAA, 0xAA, 0xAA));
    }

    TEST_F(TextElementTests, Color_TheEditableComboBoxsLineEditTakesTheSamePalette)
    {
        TextElement::registerElement(&TestModuleField, &QComboBox::staticMetaObject, &isTestModuleField);

        QComboBox comboBox;
        comboBox.setEditable(true);
        comboBox.setObjectName(QStringLiteral("textElementTestsModuleField"));
        polishAndApply(&comboBox);
        TextElement::applyColor(&comboBox);

        ASSERT_TRUE(comboBox.lineEdit() != nullptr);
        expectForeground(comboBox.lineEdit()->palette(), QPalette::Active, QColor(0xFF, 0xFF, 0xFF));
        expectForeground(comboBox.lineEdit()->palette(), QPalette::Disabled, QColor(0xAA, 0xAA, 0xAA));
    }
} // namespace AzQtComponents
