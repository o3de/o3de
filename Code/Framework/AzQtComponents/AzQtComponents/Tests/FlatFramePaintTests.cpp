/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzTest/AzTest.h>
#include <AzQtComponents/Components/Elements/FrameElement.h>
#include <AzQtComponents/Components/Elements/TextElement.h>
#include <AzQtComponents/Components/Style.h>
#include <AzQtComponents/Components/StyleManager.h>

#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>

#include <QApplication>
#include <QFrame>
#include <QImage>
#include <QLabel>
#include <QPainter>
#include <QScrollArea>
#include <QStyleFactory>
#include <QStyleOptionFrame>
#include <QWidget>

namespace AzQtComponents
{
    namespace
    {
        constexpr QRgb DialogColor = 0x444444;  // the window colour the picker frames sit on
        constexpr QRgb CardColor = 0x555555;    // a parent that is not the universal frame fill (#444444)
        constexpr QRgb LineColor = 0x999999;    // a WindowText a widget has from code (a .ui palette): not the one a frame draws in

        // The values below are Native's, stated again here: the tests prove the draw, not that it reads the sheets.
        constexpr QRgb FramePanelFill = 0x444444;   // BaseStyleSheet.qss:47
        constexpr QRgb FrameStarWhite = 0xFFFFFF;   // the '*' rule's color, BaseStyleSheet.qss:18
        constexpr QRgb FrameBevelDark = 0x2D2D2D;   // #444444.darker(150), qstylesheetstyle.cpp:1402
        constexpr QRgb FrameBevelLight = 0x4E4E4E;  // #444444.lighter(115), qstylesheetstyle.cpp:1400
        constexpr QRgb FrameOutline = 0x343434;     // Fusion's outline of a panel on #444444: Window.darker(140).lighter(108)

        //! The colour of one device pixel, without alpha
        QRgb pixelAt(const QImage& image, int x, int y)
        {
            return image.pixelColor(x, y).rgb() & 0xFFFFFF;
        }

        //! The element a frame has, by name ("" for none)
        const char* elementNameOf(const QWidget* widget)
        {
            const FrameElement* element = FrameElement::forWidget(widget);
            return element ? element->m_name : "";
        }

        // Module elements registered by the tests (a registration lasts for the process, so each names a widget of its own)
        const FrameElement FrameTestTimer{ .m_name = "Test.Frame.Timer", .m_foreground = QColor(0x88, 0x88, 0x88) };      // AssetProcessor.qss:20-22
        const FrameElement FrameTestLate{ .m_name = "Test.Frame.Late", .m_foreground = QColor(0x66, 0x66, 0x66) };

        bool isFrameTestTimer(const QWidget* widget)
        {
            return Selectors::named(widget, "frameElementTestsTimer");
        }

        bool isFrameTestLate(const QWidget* widget)
        {
            return Selectors::named(widget, "frameElementTestsLate");
        }
    } // namespace

    // ----------------------------------------------------------------------------------
    // Fixture: the flattened Style as the application's style over Fusion, and frames painted the way a widget paints them
    // ----------------------------------------------------------------------------------

    class FlatFramePaintTests
        : public ::UnitTest::LeakDetectionFixture
    {
    public:
        FlatFramePaintTests()
        {
            m_application = AZStd::make_unique<QApplication>(m_argc, m_argv);

            // The application's style, as in the running GUI: Qt reaches it through widget->style() for the outline of a
            // panel (QCommonStyle, qcommonstyle.cpp:2368-2376), and a widget under a style sheet has it as its base style
            m_style = new Style(QStyleFactory::create(QStringLiteral("fusion")));
            QApplication::setStyle(m_style);
        }

        ~FlatFramePaintTests() override
        {
            m_application.reset();      // the application takes the style with it
        }

        //! The option QFrame::paintEvent builds for the frame (QFrame::initStyleOption), with a WindowText the widget
        //! has from code: the '*' rule's white reached every frame over it in the legacy GUI
        static QStyleOptionFrame frameOption(const QFrame& frame, QRgb windowText)
        {
            QStyleOptionFrame option = frameOption(frame);
            option.palette.setColor(QPalette::WindowText, QColor::fromRgb(windowText));
            return option;
        }

        //! The option QFrame::paintEvent builds for the frame (QFrame::initStyleOption)
        static QStyleOptionFrame frameOption(const QFrame& frame)
        {
            QStyleOptionFrame option;
            option.initFrom(&frame);
            option.frameShape = frame.frameShape();
            option.lineWidth = frame.lineWidth();
            option.midLineWidth = frame.midLineWidth();
            if (frame.frameShadow() == QFrame::Sunken)
            {
                option.state |= QStyle::State_Sunken;
            }
            else if (frame.frameShadow() == QFrame::Raised)
            {
                option.state |= QStyle::State_Raised;
            }
            return option;
        }

        //! The frame painted as QFrame::paintEvent paints it (a CE_ShapedFrame through the style), at a logical offset on
        //! a parent-coloured image of the given device ratio, inside the clip of the widget rect
        QImage paintFrame(const QFrame& frame, const QPoint& offset, qreal ratio, const QSize& deviceSize, QRgb parent,
            QRgb windowText = LineColor)
        {
            const QStyleOptionFrame option = frameOption(frame, windowText);

            QImage image(deviceSize, QImage::Format_ARGB32_Premultiplied);
            image.setDevicePixelRatio(ratio);
            image.fill(QColor::fromRgb(parent));

            QPainter painter(&image);
            painter.translate(offset);
            painter.setClipRect(option.rect);
            m_style->drawControl(QStyle::CE_ShapedFrame, &option, &painter, &frame);
            painter.end();
            return image;
        }

        //! One exact QFrame of a shape and shadow, with a class tag if given
        QImage paintFrame(QFrame::Shape shape, QFrame::Shadow shadow, int lineWidth, const QString& classTag, const QPoint& offset,
            const QSize& size, qreal ratio, const QSize& deviceSize, QRgb parent)
        {
            QFrame frame;
            frame.setFrameShape(shape);
            frame.setFrameShadow(shadow);
            frame.setLineWidth(lineWidth);
            if (!classTag.isEmpty())
            {
                frame.setProperty("class", classTag);
            }
            frame.resize(size);
            return paintFrame(frame, offset, ratio, deviceSize, parent);
        }

        //! The same, Plain and one line wide
        QImage paintFrame(QFrame::Shape shape, const QString& classTag, const QPoint& offset, const QSize& size,
            qreal ratio, const QSize& deviceSize, QRgb parent)
        {
            return paintFrame(shape, QFrame::Plain, 1, classTag, offset, size, ratio, deviceSize, parent);
        }

        //! The frame painted as a window paints it (QWidgetPrivate::drawWidget), through whatever style it has (a sheet's
        //! style over this one): its styled background (PE_Widget, QWidgetPrivate::paintBackground), then its frame
        //! (CE_ShapedFrame, QFrame::paintEvent), at a logical offset on a parent-coloured image of the given device ratio.
        //! The image is the painter's device, so the style scales the lines by the image's ratio (QStyleHelper::getDpr);
        //! QWidget::render would hand the style the widget instead, and the ratio of the screen the test runs on.
        static QImage paintThroughItsStyle(const QFrame& frame, const QPoint& offset, qreal ratio, const QSize& deviceSize, QRgb parent)
        {
            QImage image(deviceSize, QImage::Format_ARGB32_Premultiplied);
            image.setDevicePixelRatio(ratio);
            image.fill(QColor::fromRgb(parent));

            QPainter painter(&image);
            painter.translate(offset);
            painter.setClipRect(frame.rect());
            if (frame.testAttribute(Qt::WA_StyledBackground))
            {
                QStyleOption background;
                background.initFrom(&frame);
                frame.style()->drawPrimitive(QStyle::PE_Widget, &background, &painter, &frame);
            }
            const QStyleOptionFrame option = frameOption(frame);
            frame.style()->drawControl(QStyle::CE_ShapedFrame, &option, &painter, &frame);
            painter.end();
            return image;
        }

    protected:
        int m_argc = 1;                                     // QApplication keeps a reference to it
        char m_programName[5] = "test";
        char* m_argv[2] = { m_programName, nullptr };
        AZStd::unique_ptr<QApplication> m_application;
        Style* m_style = nullptr;                           // the application's
    };

    // ----------------------------------------------------------------------------------
    // T-FE.1: the universal rule's frames (BaseStyleSheet.qss:41-48): the fill, then the base style's lines over a
    // palette derived from it, as QStyleSheetStyle drew them
    // ----------------------------------------------------------------------------------

    TEST_F(FlatFramePaintTests, ExactQFrameLine_IsTheUniversalFillAndTheStarRulesWhiteLine)
    {
        ASSERT_TRUE(StyleManager::stylesheetsDisabled());

        // An untagged exact QFrame, HLine, Plain: `.QFrame[frameShape="4"] { background-color: #444444 }` and a line in
        // the '*' rule's white (BaseStyleSheet.qss:18) - not in the WindowText the widget has from code (LineColor)
        const QImage image = paintFrame(QFrame::HLine, QString(), QPoint(17, 10), QSize(270, 3), 1.0, QSize(456, 40), CardColor);

        EXPECT_EQ(pixelAt(image, 100, 9), CardColor);
        EXPECT_EQ(pixelAt(image, 100, 10), FramePanelFill);
        EXPECT_EQ(pixelAt(image, 100, 11), FrameStarWhite);
        EXPECT_EQ(pixelAt(image, 100, 12), FramePanelFill);
        EXPECT_EQ(pixelAt(image, 100, 13), CardColor);
    }

    TEST_F(FlatFramePaintTests, HorizontalSeparatorTag_IsAWholeWordNotAPrefix)
    {
        ASSERT_TRUE(StyleManager::stylesheetsDisabled());

        // The picker's container tag starts with the separator tag: a frame carrying it is a QFrame with a class tag, which
        // `.QFrame[frameShape]` does not match (the tag replaces the class name, QStyleSheetStyleSelector::attributeValue): it
        // has no fill, and the base style draws its Plain line in the '*' rule's white over the parent. (The picker's own
        // separators are its element's: ColorPickerElementTests, T-CP.4.)
        const QImage image = paintFrame(QFrame::HLine, QStringLiteral("HorizontalSeparatorContainer"),
            QPoint(17, 10), QSize(270, 3), 1.0, QSize(456, 40), CardColor);

        EXPECT_EQ(pixelAt(image, 100, 10), CardColor);
        EXPECT_EQ(pixelAt(image, 100, 11), FrameStarWhite);
        EXPECT_EQ(pixelAt(image, 100, 12), CardColor);
    }

    TEST_F(FlatFramePaintTests, ExactQFrameVLineAndPanel_KeepTheUniversalFill)
    {
        ASSERT_TRUE(StyleManager::stylesheetsDisabled());

        const QImage vline = paintFrame(QFrame::VLine, QString(), QPoint(17, 10), QSize(3, 20), 1.0, QSize(456, 40), CardColor);
        EXPECT_EQ(pixelAt(vline, 16, 20), CardColor);
        EXPECT_EQ(pixelAt(vline, 17, 20), FramePanelFill);
        EXPECT_EQ(pixelAt(vline, 18, 20), FrameStarWhite);
        EXPECT_EQ(pixelAt(vline, 19, 20), FramePanelFill);

        // A StyledPanel: the fill, and Fusion's outline of it (Native MaterialCanvas dock-Bookmarks 'frame_2', n2: the
        // ring of the panel reads #343434 on its #444444). The frame the old code left without a line.
        const QImage panel = paintFrame(QFrame::StyledPanel, QString(), QPoint(17, 10), QSize(30, 20), 1.0, QSize(456, 40), CardColor);
        EXPECT_EQ(pixelAt(panel, 30, 20), FramePanelFill);
        EXPECT_EQ(pixelAt(panel, 16, 20), CardColor);
        EXPECT_EQ(pixelAt(panel, 17, 20), FrameOutline);
        EXPECT_EQ(pixelAt(panel, 46, 20), FrameOutline);
        EXPECT_EQ(pixelAt(panel, 30, 10), FrameOutline);
        EXPECT_EQ(pixelAt(panel, 30, 29), FrameOutline);
        EXPECT_EQ(pixelAt(panel, 30, 30), CardColor);
    }

    TEST_F(FlatFramePaintTests, BoxFrame_HasTheStarRulesWhiteOutlineOverTheFill)
    {
        ASSERT_TRUE(StyleManager::stylesheetsDisabled());

        // Box, Plain: qDrawPlainRect in the foreground role (qcommonstyle.cpp:2354-2358), over the fill
        const QImage image = paintFrame(QFrame::Box, QString(), QPoint(17, 10), QSize(30, 20), 1.0, QSize(456, 40), CardColor);
        EXPECT_EQ(pixelAt(image, 17, 20), FrameStarWhite);
        EXPECT_EQ(pixelAt(image, 30, 10), FrameStarWhite);
        EXPECT_EQ(pixelAt(image, 30, 20), FramePanelFill);
        EXPECT_EQ(pixelAt(image, 16, 20), CardColor);
    }

    TEST_F(FlatFramePaintTests, SunkenDivider_HasTheBevelsDarkAndLightLines)
    {
        ASSERT_TRUE(StyleManager::stylesheetsDisabled());

        // Editor Inspector 'line' (02_main_entity_selected, n2): HLine, Sunken, line width 1, 348x3 at logical y 249
        // (device 373.5), ratio 1.5. Device rows 374-377 at one column: #2d2d2d #2d2d2d #4e4e4e #4e4e4e, the bevel
        // of #444444 (Dark, Light). The test sits at y 5 (device 7.5): the same phase, so rows 8-11.
        const QImage image = paintFrame(QFrame::HLine, QFrame::Sunken, 1, QString(), QPoint(10, 5), QSize(60, 3), 1.5, QSize(120, 30), CardColor);

        EXPECT_EQ(pixelAt(image, 60, 7), CardColor);
        EXPECT_EQ(pixelAt(image, 60, 8), FrameBevelDark);
        EXPECT_EQ(pixelAt(image, 60, 9), FrameBevelDark);
        EXPECT_EQ(pixelAt(image, 60, 10), FrameBevelLight);
        EXPECT_EQ(pixelAt(image, 60, 11), FrameBevelLight);
        EXPECT_EQ(pixelAt(image, 60, 12), CardColor);
    }

    TEST_F(FlatFramePaintTests, SunkenVerticalDivider_HasTheBevelsDarkAndLightLines)
    {
        ASSERT_TRUE(StyleManager::stylesheetsDisabled());

        // MaterialCanvas 'line' (n2): VLine, Sunken, 3x20 at logical x 408 (device 612.0), ratio 1.5. Device columns
        // 612-615 at mid height: #2d2d2d #2d2d2d #4e4e4e #4e4e4e. The test sits at x 10 (device 15.0): columns 15-18,
        // then the fill of the 4.5 device pixels the widget covers.
        const QImage image = paintFrame(QFrame::VLine, QFrame::Sunken, 1, QString(), QPoint(10, 5), QSize(3, 20), 1.5, QSize(60, 60), CardColor);

        EXPECT_EQ(pixelAt(image, 14, 22), CardColor);
        EXPECT_EQ(pixelAt(image, 15, 22), FrameBevelDark);
        EXPECT_EQ(pixelAt(image, 16, 22), FrameBevelDark);
        EXPECT_EQ(pixelAt(image, 17, 22), FrameBevelLight);
        EXPECT_EQ(pixelAt(image, 18, 22), FrameBevelLight);
        EXPECT_EQ(pixelAt(image, 19, 22), FramePanelFill);
        EXPECT_EQ(pixelAt(image, 20, 22), CardColor);
    }

    TEST_F(FlatFramePaintTests, RaisedPanel_IsOutlinedByFusionFromTheFill)
    {
        ASSERT_TRUE(StyleManager::stylesheetsDisabled());

        // The panels of the Script Canvas and Material Canvas docks (StyledPanel, Raised): Fusion draws one outline
        // whatever the shadow, from Window.darker(140) of the rule's #444444
        const QImage image = paintFrame(QFrame::StyledPanel, QFrame::Raised, 1, QString(), QPoint(10, 5), QSize(50, 30), 1.0, QSize(100, 60), CardColor);

        EXPECT_EQ(pixelAt(image, 10, 20), FrameOutline);
        EXPECT_EQ(pixelAt(image, 59, 20), FrameOutline);
        EXPECT_EQ(pixelAt(image, 30, 20), FramePanelFill);
    }

    // ----------------------------------------------------------------------------------
    // T-FE.2: frames the rules gave no background of their own are transparent, so what derives from it is not seen
    // ----------------------------------------------------------------------------------

    TEST_F(FlatFramePaintTests, ScrollArea_ShowsNoFrameLine)
    {
        ASSERT_TRUE(StyleManager::stylesheetsDisabled());

        // '*' gave the scroll area a transparent background: Fusion's outline of it (Window.darker(140)) is transparent,
        // and Native shows the parent at the edge (QGraphicsView and QScrollArea rings in n2, no #343434)
        QScrollArea scrollArea;
        scrollArea.resize(50, 30);
        ASSERT_EQ(scrollArea.frameShape(), QFrame::StyledPanel);

        const QImage image = paintFrame(scrollArea, QPoint(17, 10), 1.0, QSize(456, 40), CardColor);
        EXPECT_EQ(pixelAt(image, 17, 25), CardColor);
        EXPECT_EQ(pixelAt(image, 66, 25), CardColor);
        EXPECT_EQ(pixelAt(image, 40, 10), CardColor);
        EXPECT_EQ(pixelAt(image, 40, 39), CardColor);

        // and at the ratio of the captures
        const QImage scaled = paintFrame(scrollArea, QPoint(10, 5), 1.5, QSize(120, 80), CardColor);
        EXPECT_EQ(pixelAt(scaled, 15, 30), CardColor);
        EXPECT_EQ(pixelAt(scaled, 16, 30), CardColor);
        EXPECT_EQ(pixelAt(scaled, 50, 8), CardColor);
        EXPECT_EQ(pixelAt(scaled, 50, 9), CardColor);
    }

    TEST_F(FlatFramePaintTests, ClassTaggedSunkenDivider_IsNotSeen)
    {
        ASSERT_TRUE(StyleManager::stylesheetsDisabled());

        // A tagged QFrame is not matched by `.QFrame[frameShape]`: transparent, and so is the bevel derived from it
        const QImage image = paintFrame(QFrame::HLine, QFrame::Sunken, 1, QStringLiteral("frameElementTestsOther"),
            QPoint(17, 10), QSize(50, 3), 1.0, QSize(456, 40), CardColor);

        for (int y = 9; y <= 13; ++y)
        {
            EXPECT_EQ(pixelAt(image, 30, y), CardColor);
        }
    }

    TEST_F(FlatFramePaintTests, CardSeparator_IsAPlainLineInItsOwnColour)
    {
        ASSERT_TRUE(StyleManager::stylesheetsDisabled());

        // `.separator { color: #999999 }` (Card.qss:259-264) on the Card's QFrame::HLine (Gallery 04_card, n2: 1075x3 at
        // logical y 331, device 496.5: rows 497-498 read #999999 on the card's #555555, no #444444 under the line).
        // The test sits at y 5 (device 7.5): rows 8-9.
        const QImage scaled = paintFrame(QFrame::HLine, QFrame::Plain, 1, QStringLiteral("separator"),
            QPoint(10, 5), QSize(60, 3), 1.5, QSize(120, 30), CardColor);
        EXPECT_EQ(pixelAt(scaled, 60, 7), CardColor);
        EXPECT_EQ(pixelAt(scaled, 60, 8), 0x999999u);
        EXPECT_EQ(pixelAt(scaled, 60, 9), 0x999999u);
        EXPECT_EQ(pixelAt(scaled, 60, 10), CardColor);

        const QImage image = paintFrame(QFrame::HLine, QFrame::Plain, 1, QStringLiteral("small separator"),
            QPoint(17, 10), QSize(270, 3), 1.0, QSize(456, 40), CardColor);
        EXPECT_EQ(pixelAt(image, 100, 10), CardColor);
        EXPECT_EQ(pixelAt(image, 100, 11), 0x999999u);
        EXPECT_EQ(pixelAt(image, 100, 12), CardColor);
    }

    TEST_F(FlatFramePaintTests, NoFrame_DrawsNothing)
    {
        ASSERT_TRUE(StyleManager::stylesheetsDisabled());

        // A frame with no shape has no frame to fill: what a box gives such a widget is the box element's
        const QImage image = paintFrame(QFrame::NoFrame, QString(), QPoint(17, 10), QSize(30, 20), 1.0, QSize(456, 40), CardColor);
        EXPECT_EQ(pixelAt(image, 30, 20), CardColor);
        EXPECT_EQ(pixelAt(image, 17, 20), CardColor);
    }

    // ----------------------------------------------------------------------------------
    // T-FE.3: widgets that are not the universal rule's frames keep what their code gave them
    // ----------------------------------------------------------------------------------

    TEST_F(FlatFramePaintTests, SubclassWithACodeSetPalette_IsLeftAlone)
    {
        ASSERT_TRUE(StyleManager::stylesheetsDisabled());

        // ComponentPaletteWidget's frame: Box | Plain, line width 1, its border colour carried by a WindowText the code
        // set (ComponentPaletteWidget.cpp:93-98). A QFrame subclass has no element (`.QFrame` never matched one), so the
        // base style draws the outline from the palette as before.
        QLabel subclass(QStringLiteral("x"));
        subclass.setFrameStyle(QFrame::Box | QFrame::Plain);
        subclass.setLineWidth(1);
        subclass.resize(30, 20);
        EXPECT_STREQ(elementNameOf(&subclass), "");

        const QImage image = paintFrame(subclass, QPoint(17, 10), 1.0, QSize(456, 40), CardColor, 0x4E4E4E);
        EXPECT_EQ(pixelAt(image, 17, 20), 0x4E4E4Eu);
        EXPECT_EQ(pixelAt(image, 46, 20), 0x4E4E4Eu);
        EXPECT_EQ(pixelAt(image, 30, 10), 0x4E4E4Eu);
        EXPECT_EQ(pixelAt(image, 30, 29), 0x4E4E4Eu);
        EXPECT_EQ(pixelAt(image, 30, 20), CardColor);
    }

    // ----------------------------------------------------------------------------------
    // T-FE.4: resolution and registration
    // ----------------------------------------------------------------------------------

    TEST_F(FlatFramePaintTests, Resolve_GlobalElementsFollowTheWidgetsIdentity)
    {
        QFrame plain;
        EXPECT_STREQ(elementNameOf(&plain), "Global.Frame.Panel");

        // `.QFrame` is [class~=QFrame]: any class tag replaces the class name
        QFrame tagged;
        tagged.setProperty("class", QStringLiteral("anything"));
        EXPECT_STREQ(elementNameOf(&tagged), "Global.Frame.Clear");

        // `.separator` is [class~=separator]: a whole word, compared with case (qcssparser.cpp:2168-2180)
        QFrame separator;
        separator.setProperty("class", QStringLiteral("small separator"));
        EXPECT_STREQ(elementNameOf(&separator), "Global.Frame.Separator");

        QFrame notSeparator;
        notSeparator.setProperty("class", QStringLiteral("Separator separators"));
        EXPECT_STREQ(elementNameOf(&notSeparator), "Global.Frame.Clear");

        QScrollArea scrollArea;
        EXPECT_STREQ(elementNameOf(&scrollArea), "Global.Frame.Clear");

        QLabel label;
        EXPECT_STREQ(elementNameOf(&label), "");
    }

    TEST_F(FlatFramePaintTests, Resolve_PolishKeepsTheElementOnTheWidgetAndATagChangesItOnRepolish)
    {
        ASSERT_TRUE(StyleManager::stylesheetsDisabled());

        QFrame frame;
        frame.ensurePolished();                         // Style::polish resolves it
        EXPECT_STREQ(elementNameOf(&frame), "Global.Frame.Panel");

        // Style::addClass tags the widget and polishes it again
        Style::addClass(&frame, QStringLiteral("separator"));
        EXPECT_STREQ(elementNameOf(&frame), "Global.Frame.Separator");
    }

    TEST_F(FlatFramePaintTests, Register_ModuleElementBeforeTheGlobalOnes)
    {
        ASSERT_TRUE(StyleManager::stylesheetsDisabled());

        FrameElement::registerElement(&FrameTestTimer, &QFrame::staticMetaObject, &isFrameTestTimer);

        // The AssetProcessor timer row (n2 AssetProcessor_00, 'lastScanSeparator'): a VLine, Plain, line width 2, 3x25 at
        // logical x 839 (device 1258.5), ratio 1.5, with a class tag, so no fill; its line is three device columns of
        // #888888 over the parent. The test sits at x 5 (device 7.5): columns 8-10.
        QFrame frame;
        frame.setObjectName(QStringLiteral("frameElementTestsTimer"));
        frame.setProperty("class", QStringLiteral("TimerLine"));
        frame.setFrameShape(QFrame::VLine);
        frame.setFrameShadow(QFrame::Plain);
        frame.setLineWidth(2);
        frame.resize(3, 25);
        EXPECT_STREQ(elementNameOf(&frame), "Test.Frame.Timer");

        const QImage image = paintFrame(frame, QPoint(5, 5), 1.5, QSize(60, 60), DialogColor);
        EXPECT_EQ(pixelAt(image, 7, 20), DialogColor);
        EXPECT_EQ(pixelAt(image, 8, 20), 0x888888u);
        EXPECT_EQ(pixelAt(image, 9, 20), 0x888888u);
        EXPECT_EQ(pixelAt(image, 10, 20), 0x888888u);
        EXPECT_EQ(pixelAt(image, 11, 20), DialogColor);
    }

    TEST_F(FlatFramePaintTests, Register_ResolvesTheWidgetsPolishedBeforeTheCall)
    {
        ASSERT_TRUE(StyleManager::stylesheetsDisabled());

        // A module builds some windows before it sets up its styles
        QFrame frame;
        frame.setObjectName(QStringLiteral("frameElementTestsLate"));
        frame.ensurePolished();
        EXPECT_STREQ(elementNameOf(&frame), "Global.Frame.Panel");

        FrameElement::registerElement(&FrameTestLate, &QFrame::staticMetaObject, &isFrameTestLate);
        EXPECT_STREQ(elementNameOf(&frame), "Test.Frame.Late");
    }

    // ----------------------------------------------------------------------------------
    // T-FE.5: a widget still drawn through a style sheet keeps what the sheet gave it (transitional)
    // ----------------------------------------------------------------------------------

    TEST_F(FlatFramePaintTests, Sheet_GivingTheFrameABackground_OwnsItsFillAndShades)
    {
        ASSERT_TRUE(StyleManager::stylesheetsDisabled());

        // AssetProcessor's Settings page: each divider is a Line (a QFrame, HLine, Sunken, 3 high) with an inline
        // `background-color: #616161;` of its own (MainWindow.ui:1619, 1767, 1900, 2051, 2202), and QStyleSheetStyle draws
        // it over this style: the bevel of the sheet's fill, not of the universal #444444. Native at ratio 1.5 (n2
        // AssetProcessor_00 SettingsPage6), down the divider from its first device row:
        //   at a whole device pixel ('line_2', logical y 192): #414141 #414141 #707070 #707070 #616161
        //   at a half one ('line', logical y 273): the parent, then #414141 #414141 #707070 #707070, then the parent
        QWidget page;
        QFrame* divider = new QFrame(&page);
        divider->setStyleSheet(QStringLiteral("background-color: #616161;"));
        divider->setFrameShape(QFrame::HLine);
        divider->setFrameShadow(QFrame::Sunken);
        divider->resize(50, 3);
        page.ensurePolished();
        ASSERT_TRUE(divider->style()->inherits("QStyleSheetStyle"));

        // Logical y 2 is device row 3
        const QImage whole = paintThroughItsStyle(*divider, QPoint(0, 2), 1.5, QSize(75, 12), CardColor);
        EXPECT_EQ(pixelAt(whole, 30, 2), CardColor);
        EXPECT_EQ(pixelAt(whole, 30, 3), 0x414141u);
        EXPECT_EQ(pixelAt(whole, 30, 4), 0x414141u);
        EXPECT_EQ(pixelAt(whole, 30, 5), 0x707070u);
        EXPECT_EQ(pixelAt(whole, 30, 6), 0x707070u);
        EXPECT_EQ(pixelAt(whole, 30, 7), 0x616161u);
        EXPECT_EQ(pixelAt(whole, 30, 8), CardColor);

        // Logical y 1 is device row 1.5
        const QImage half = paintThroughItsStyle(*divider, QPoint(0, 1), 1.5, QSize(75, 12), CardColor);
        EXPECT_EQ(pixelAt(half, 30, 1), CardColor);
        EXPECT_EQ(pixelAt(half, 30, 2), 0x414141u);
        EXPECT_EQ(pixelAt(half, 30, 3), 0x414141u);
        EXPECT_EQ(pixelAt(half, 30, 4), 0x707070u);
        EXPECT_EQ(pixelAt(half, 30, 5), 0x707070u);
        EXPECT_EQ(pixelAt(half, 30, 6), CardColor);
    }

    TEST_F(FlatFramePaintTests, Sheet_MatchingNoFrame_LeavesTheFrameToTheElement)
    {
        ASSERT_TRUE(StyleManager::stylesheetsDisabled());

        // A sheet over the divider that gives it nothing: the element fills and shades it as the universal
        // `.QFrame[frameShape]` rule did (BaseStyleSheet.qss:41-48), the bevel of #444444, in the rows of the divider above
        QWidget page;
        page.setStyleSheet(QStringLiteral("QPushButton { color: red; }"));
        QFrame* divider = new QFrame(&page);
        divider->setFrameShape(QFrame::HLine);
        divider->setFrameShadow(QFrame::Sunken);
        divider->resize(50, 3);
        page.ensurePolished();
        ASSERT_TRUE(divider->style()->inherits("QStyleSheetStyle"));

        const QImage image = paintThroughItsStyle(*divider, QPoint(0, 2), 1.5, QSize(75, 12), CardColor);
        EXPECT_EQ(pixelAt(image, 30, 2), CardColor);
        EXPECT_EQ(pixelAt(image, 30, 3), FrameBevelDark);
        EXPECT_EQ(pixelAt(image, 30, 4), FrameBevelDark);
        EXPECT_EQ(pixelAt(image, 30, 5), FrameBevelLight);
        EXPECT_EQ(pixelAt(image, 30, 6), FrameBevelLight);
        EXPECT_EQ(pixelAt(image, 30, 7), FramePanelFill);
        EXPECT_EQ(pixelAt(image, 30, 8), CardColor);
    }

    TEST_F(FlatFramePaintTests, Sheet_ColorOfItsOwn_IsTheFramesLine)
    {
        ASSERT_TRUE(StyleManager::stylesheetsDisabled());

        // AzAssetBrowserWindow.ui:130 gives the asset browser's frames `QFrame { color : rgb(34, 34, 34); }`: the sheet
        // closest to the widget wins over the '*' rule, and Native draws their Plain Box outline #222222 (n2, Editor
        // 07_main_window_final 'framePreCreate'). The universal fill stays.
        QFrame frame;
        frame.setStyleSheet(QStringLiteral("QFrame { color: rgb(34, 34, 34); }"));
        frame.setFrameShape(QFrame::Box);
        frame.setFrameShadow(QFrame::Plain);
        frame.setLineWidth(1);
        frame.resize(30, 20);
        frame.ensurePolished();
        ASSERT_TRUE(frame.style()->inherits("QStyleSheetStyle"));

        const QImage image = paintThroughItsStyle(frame, QPoint(0, 0), 1.0, QSize(30, 20), CardColor);
        EXPECT_EQ(pixelAt(image, 0, 10), 0x222222u);
        EXPECT_EQ(pixelAt(image, 15, 0), 0x222222u);
        EXPECT_EQ(pixelAt(image, 29, 10), 0x222222u);
        EXPECT_EQ(pixelAt(image, 15, 10), FramePanelFill);
    }
} // namespace AzQtComponents
