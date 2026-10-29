/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzTest/AzTest.h>
#include <AzQtComponents/Components/FilteredSearchWidget.h>
#include <AzQtComponents/Components/Style.h>
#include <AzQtComponents/Components/StyleManager.h>
#include <AzQtComponents/Components/Widgets/LineEdit.h>
#include <AzQtComponents/Components/Widgets/SpinBox.h>

#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>

#include <QApplication>
#include <QComboBox>
#include <QImage>
#include <QLineEdit>
#include <QPainter>
#include <QStyleFactory>
#include <QStyleOption>
#include <QToolButton>

#include <string>

namespace AzQtComponents
{
    namespace
    {
        constexpr QRgb FieldRing = 0xE9E9E9;            // LineEdit.qss:16-17 the 2px border, BaseStyleSheet.qss:81
        constexpr QRgb FieldFill = 0xCCCCCC;            // BaseStyleSheet.qss:76 the field
        constexpr QRgb FieldDisabledFill = 0x666666;    // LineEdit.qss:33 the field when disabled
        constexpr QRgb FieldParent = 0xFF00FF;          // what the ring is painted over: magenta shows every pixel it misses

        //! The colour of one pixel, without alpha
        QRgb lineEditPixel(const QImage& image, int x, int y)
        {
            return image.pixelColor(x, y).rgb() & 0xFFFFFF;
        }

        //! A rect as text, for the failure message of a comparison
        std::string lineEditRectText(const QRect& rect)
        {
            return QStringLiteral("(%1, %2, %3 x %4)").arg(rect.x()).arg(rect.y()).arg(rect.width()).arg(rect.height()).toStdString();
        }

        //! A size as text, for the failure message of a comparison
        std::string lineEditSizeText(const QSize& size)
        {
            return QStringLiteral("(%1 x %2)").arg(size.width()).arg(size.height()).toStdString();
        }
    } // namespace

    // ----------------------------------------------------------------------------------
    // Fixture: the flattened style chain (Style -> Fusion) under an application
    // ----------------------------------------------------------------------------------

    class LineEditFieldTests
        : public ::UnitTest::LeakDetectionFixture
    {
    public:
        LineEditFieldTests()
        {
            m_application = AZStd::make_unique<QApplication>(m_argc, m_argv);

            // As StyleManager::initialize builds it (StyleManager.cpp:244-246); the application owns it
            m_style = new Style(QStyleFactory::create(QStringLiteral("Fusion")));
            QApplication::setStyle(m_style);
        }

        ~LineEditFieldTests() override
        {
            m_application.reset();
        }

        //! The size the style gives a line edit whose contents are 100x14
        QSize styledHint(const QLineEdit* lineEdit) const
        {
            QStyleOptionFrame option;
            option.initFrom(lineEdit);
            return m_style->sizeFromContents(QStyle::CT_LineEdit, &option, QSize(100, 14), lineEdit);
        }

        //! The size the style under it would give the same line edit
        QSize baseHint(const QLineEdit* lineEdit) const
        {
            QStyleOptionFrame option;
            option.initFrom(lineEdit);
            return m_style->baseStyle()->sizeFromContents(QStyle::CT_LineEdit, &option, QSize(100, 14), lineEdit);
        }

        //! The text rect of a framed line edit whose rect is 100x20
        QRect contentsOf(const QLineEdit* lineEdit) const
        {
            QStyleOptionFrame option;
            option.initFrom(lineEdit);
            option.rect = QRect(0, 0, 100, 20);
            return m_style->subElementRect(QStyle::SE_LineEditContents, &option, lineEdit);
        }

        //! The ring and field of a line edit, painted the way QLineEdit paints its panel, over magenta
        QImage paintPanel(const QLineEdit* lineEdit, QStyle::State state) const
        {
            QStyleOptionFrame option;
            option.initFrom(lineEdit);
            option.rect = QRect(0, 0, 60, 20);
            option.state = state;

            QImage image(option.rect.size(), QImage::Format_ARGB32_Premultiplied);
            image.fill(QColor::fromRgb(FieldParent));
            QPainter painter(&image);
            m_style->drawPrimitive(QStyle::PE_PanelLineEdit, &option, &painter, lineEdit);
            painter.end();
            return image;
        }

    protected:
        int m_argc = 1;                                     // QApplication keeps a reference to it
        char m_programName[5] = "test";
        char* m_argv[2] = { m_programName, nullptr };
        AZStd::unique_ptr<QApplication> m_application;
        Style* m_style = nullptr;                           // owned by the application
    };

    // ----------------------------------------------------------------------------------
    // T-LE.1: the size of a field. LineEdit.qss gave a line edit a 2px border, no padding but a vertical
    // one of -1 above and 1 below, and a height of 16px (the box is 20px high): the legacy box is 2px wider than
    // Fusion's, 20px high, and 6px narrower with a search action (padding-left -6px, LineEdit.qss:22-26).
    // ----------------------------------------------------------------------------------

    TEST_F(LineEditFieldTests, Hint_PlainFieldIsTwoWiderThanTheBaseBoxAndTwentyHigh)
    {
        ASSERT_TRUE(StyleManager::stylesheetsDisabled());

        QLineEdit lineEdit;
        const QSize hint = styledHint(&lineEdit);
        EXPECT_EQ(hint.width(), baseHint(&lineEdit).width() + 2) << lineEditSizeText(hint);
        EXPECT_EQ(hint.height(), 20) << lineEditSizeText(hint);
    }

    TEST_F(LineEditFieldTests, Hint_SearchFieldIsSixNarrowerThanAPlainOne)
    {
        ASSERT_TRUE(StyleManager::stylesheetsDisabled());

        QLineEdit lineEdit;
        LineEdit::applySearchStyle(&lineEdit);
        const QSize hint = styledHint(&lineEdit);
        EXPECT_EQ(hint.width(), baseHint(&lineEdit).width() + 2 - 6) << lineEditSizeText(hint);
        EXPECT_EQ(hint.height(), 20) << lineEditSizeText(hint);
    }

    TEST_F(LineEditFieldTests, Hint_FieldOfAnEditableComboBoxIsSixteenHigh)
    {
        ASSERT_TRUE(StyleManager::stylesheetsDisabled());

        QComboBox combo;
        combo.setEditable(true);
        const QLineEdit* lineEdit = combo.lineEdit();
        ASSERT_TRUE(lineEdit != nullptr);

        const QSize hint = styledHint(lineEdit);
        EXPECT_EQ(hint.width(), baseHint(lineEdit).width() + 2) << lineEditSizeText(hint);
        EXPECT_EQ(hint.height(), 16) << lineEditSizeText(hint);
    }

    // ----------------------------------------------------------------------------------
    // T-LE.2: the text rect of a framed field is the rect inside the sheet's border and padding
    // (qstylesheetstyle.cpp:6176-6181): border 2 (LineEdit.qss:16), padding-top -1 (:19), padding-bottom 1 (:18)
    // ----------------------------------------------------------------------------------

    TEST_F(LineEditFieldTests, Contents_FramedFieldIsInsideItsBorderAndPadding)
    {
        ASSERT_TRUE(StyleManager::stylesheetsDisabled());

        QLineEdit lineEdit;
        const QRect contents = contentsOf(&lineEdit);
        EXPECT_EQ(contents, QRect(2, 1, 96, 16)) << lineEditRectText(contents);
    }

    TEST_F(LineEditFieldTests, Contents_SearchFieldReachesFourPixelsLeftOfItsBox)
    {
        ASSERT_TRUE(StyleManager::stylesheetsDisabled());

        // padding-left -6px under the 2px border: the contents start at -4 (the search button lies over that)
        QLineEdit lineEdit;
        LineEdit::applySearchStyle(&lineEdit);
        const QRect contents = contentsOf(&lineEdit);
        EXPECT_EQ(contents, QRect(-4, 1, 102, 16)) << lineEditRectText(contents);
    }

    TEST_F(LineEditFieldTests, Contents_FieldInsideASpinBoxIsLeftToTheSpinBox)
    {
        ASSERT_TRUE(StyleManager::stylesheetsDisabled());

        // The line edit of a spin box has the box of its container (SpinBox::editFieldRect), no field of its own:
        // its text rect is the one the base style gives, with no border and padding taken off
        SpinBox spinBox;
        const QLineEdit* lineEdit = spinBox.findChild<QLineEdit*>();
        ASSERT_TRUE(lineEdit != nullptr);

        QStyleOptionFrame option;
        option.initFrom(lineEdit);
        option.rect = QRect(0, 0, 100, 20);
        const QRect contents = m_style->subElementRect(QStyle::SE_LineEditContents, &option, lineEdit);
        const QRect base = m_style->baseStyle()->subElementRect(QStyle::SE_LineEditContents, &option, lineEdit);
        EXPECT_EQ(contents, base) << lineEditRectText(contents) << " of " << lineEditRectText(base);
    }

    // ----------------------------------------------------------------------------------
    // T-LE.3: the ring. A 2px #E9E9E9 border (LineEdit.qss:16-17) whose outer corners the sheet's radius of 2px
    // rounds (BaseStyleSheet.qss:79) and whose inner corners are square (2 - 2 = 0), around the field.
    // ----------------------------------------------------------------------------------

    TEST_F(LineEditFieldTests, Ring_IsTwoPixelsWideAroundTheField)
    {
        ASSERT_TRUE(StyleManager::stylesheetsDisabled());

        QLineEdit lineEdit;
        const QImage image = paintPanel(&lineEdit, QStyle::State_Enabled);

        for (const int x : { 0, 1 })
        {
            EXPECT_EQ(lineEditPixel(image, x, 10), FieldRing);
            EXPECT_EQ(lineEditPixel(image, 59 - x, 10), FieldRing);
        }
        for (const int y : { 0, 1 })
        {
            EXPECT_EQ(lineEditPixel(image, 30, y), FieldRing);
            EXPECT_EQ(lineEditPixel(image, 30, 19 - y), FieldRing);
        }

        // The field: from (2, 2) to (57, 17), square at its corners
        EXPECT_EQ(lineEditPixel(image, 2, 10), FieldFill);
        EXPECT_EQ(lineEditPixel(image, 30, 2), FieldFill);
        EXPECT_EQ(lineEditPixel(image, 2, 2), FieldFill);
        EXPECT_EQ(lineEditPixel(image, 57, 17), FieldFill);
    }

    TEST_F(LineEditFieldTests, Ring_OuterCornersAreRounded)
    {
        ASSERT_TRUE(StyleManager::stylesheetsDisabled());

        QLineEdit lineEdit;
        const QImage image = paintPanel(&lineEdit, QStyle::State_Enabled);

        // The corner pixel of the box is only partly covered by the rounded border: neither the ring nor what is under it
        for (const QPoint corner : { QPoint(0, 0), QPoint(59, 0), QPoint(0, 19), QPoint(59, 19) })
        {
            EXPECT_NE(lineEditPixel(image, corner.x(), corner.y()), FieldRing);
            EXPECT_NE(lineEditPixel(image, corner.x(), corner.y()), FieldParent);
        }
    }

    TEST_F(LineEditFieldTests, Ring_DisabledFieldIsDarkAndKeepsTheRing)
    {
        ASSERT_TRUE(StyleManager::stylesheetsDisabled());

        QLineEdit lineEdit;
        const QImage image = paintPanel(&lineEdit, QStyle::State_None);

        EXPECT_EQ(lineEditPixel(image, 0, 10), FieldRing);
        EXPECT_EQ(lineEditPixel(image, 2, 2), FieldDisabledFill);
        EXPECT_EQ(lineEditPixel(image, 30, 10), FieldDisabledFill);
    }

    // ----------------------------------------------------------------------------------
    // T-LE.4: the placeholder. `color: black` gave it the text color at half alpha (qstylesheetstyle.cpp:1459-1463);
    // a line edit inside a spin box or a combo box kept the color of the config.
    // ----------------------------------------------------------------------------------

    TEST_F(LineEditFieldTests, Placeholder_OfAPlainFieldIsItsTextAtHalfAlpha)
    {
        ASSERT_TRUE(StyleManager::stylesheetsDisabled());

        QLineEdit lineEdit;
        lineEdit.ensurePolished();

        const QPalette palette = lineEdit.palette();
        for (const auto group : { QPalette::Active, QPalette::Inactive })
        {
            EXPECT_TRUE(palette.color(group, QPalette::PlaceholderText) == QColor(0x00, 0x00, 0x00, 0x80));
        }
        EXPECT_TRUE(palette.color(QPalette::Disabled, QPalette::PlaceholderText) == QColor(0x99, 0x99, 0x99, 0x80));
    }

    TEST_F(LineEditFieldTests, Placeholder_InsideASpinBoxKeepsTheColorOfTheConfig)
    {
        ASSERT_TRUE(StyleManager::stylesheetsDisabled());

#if !defined(AZ_PLATFORM_LINUX)
        SpinBox spinBox;
        spinBox.ensurePolished();
        const QLineEdit* lineEdit = spinBox.findChild<QLineEdit*>();
        ASSERT_TRUE(lineEdit != nullptr);

        // LineEdit::defaultConfig (LineEditConfig.ini sets nothing)
        const QPalette palette = lineEdit->palette();
        for (const auto group : { QPalette::Active, QPalette::Inactive, QPalette::Disabled })
        {
            EXPECT_TRUE(palette.color(group, QPalette::PlaceholderText) == QColor(0x88, 0x88, 0x88));
        }
#endif
    }

    // ----------------------------------------------------------------------------------
    // T-LE.5: the search field of a FilteredSearchWidget. FilteredSearchWidget.qss:9-13 gave it a frame (:11) and a
    // width of 260px (:12, the contents width), and LineEdit.qss:42-48 gave its action buttons a size of 16px.
    // ----------------------------------------------------------------------------------

    TEST_F(LineEditFieldTests, SearchField_HasAFrameAndTheWidthOfTheSheet)
    {
        ASSERT_TRUE(StyleManager::stylesheetsDisabled());

        FilteredSearchWidget search;
        search.ensurePolished();
        QLineEdit* field = search.findChild<QLineEdit*>(QStringLiteral("textSearch"));
        ASSERT_TRUE(field != nullptr);

        // The .ui leaves the frame off; the sheet turned it on
        EXPECT_TRUE(field->hasFrame());

        // 260px of contents plus the box of the field: the 2px border, less the 6px of padding-left of a search action
        // (-4 on the left, 2 on the right), 20px high
        const QSize hint = field->sizeHint();
        const QSize minimumHint = field->minimumSizeHint();
        EXPECT_EQ(hint, QSize(258, 20)) << lineEditSizeText(hint);
        EXPECT_EQ(minimumHint, QSize(258, 20)) << lineEditSizeText(minimumHint);
    }

    TEST_F(LineEditFieldTests, SearchField_ActionButtonsAreEighteenSquare)
    {
        ASSERT_TRUE(StyleManager::stylesheetsDisabled());

        FilteredSearchWidget search;
        search.ensurePolished();
        QLineEdit* field = search.findChild<QLineEdit*>(QStringLiteral("textSearch"));
        ASSERT_TRUE(field != nullptr);
        ASSERT_TRUE(field->findChild<QToolButton*>(SearchToolButton) != nullptr);

        // 16px (LineEdit.qss:42-48) and the 1px native border of a tool button on each side
        // (qstylesheetstyle.cpp:1124-1160, with PM_DefaultFrameWidth of the base style)
        int buttons = 0;
        for (QToolButton* button : field->findChildren<QToolButton*>())
        {
            if (!button->inherits("QLineEditIconButton"))
            {
                continue;
            }
            button->ensurePolished();
            EXPECT_TRUE(button->minimumSize() == QSize(18, 18));
            EXPECT_TRUE(button->maximumSize() == QSize(18, 18));
            ++buttons;
        }

        // The search action and the clear action
        EXPECT_GE(buttons, 2);
    }
} // namespace AzQtComponents
