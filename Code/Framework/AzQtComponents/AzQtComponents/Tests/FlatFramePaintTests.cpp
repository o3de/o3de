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

#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>

#include <QApplication>
#include <QFrame>
#include <QImage>
#include <QPainter>
#include <QStyleFactory>
#include <QStyleOptionFrame>

namespace AzQtComponents
{
    namespace
    {
        constexpr QRgb DialogColor = 0x444444;  // the window colour the picker frames sit on
        constexpr QRgb CardColor = 0x555555;    // a parent that is not the universal frame fill (#444444)
        constexpr QRgb LineColor = 0x999999;    // the WindowText the universal rule hands a frame line

        //! The colour of one device pixel, without alpha
        QRgb pixelAt(const QImage& image, int x, int y)
        {
            return image.pixelColor(x, y).rgb() & 0xFFFFFF;
        }
    } // namespace

    // ----------------------------------------------------------------------------------
    // Fixture: the flattened Style over Fusion, and one exact QFrame painted the way a widget paints it
    // ----------------------------------------------------------------------------------

    class FlatFramePaintTests
        : public ::UnitTest::LeakDetectionFixture
    {
    public:
        FlatFramePaintTests()
        {
            m_application = AZStd::make_unique<QApplication>(m_argc, m_argv);
            m_style = AZStd::make_unique<Style>(QStyleFactory::create(QStringLiteral("fusion")));
        }

        ~FlatFramePaintTests() override
        {
            m_style.reset();
            m_application.reset();
        }

        //! One exact QFrame at a logical offset on a parent-coloured image of the given device ratio:
        //! QFrame::paintEvent's call (a CE_ShapedFrame through the style) inside the clip of the widget rect.
        QImage paintFrame(QFrame::Shape shape, const QString& classTag, const QPoint& offset, const QSize& size,
            qreal ratio, const QSize& deviceSize, QRgb parent)
        {
            QFrame frame;
            frame.setFrameShape(shape);
            frame.setFrameShadow(QFrame::Plain);
            if (!classTag.isEmpty())
            {
                frame.setProperty("class", classTag);
            }
            frame.resize(size);

            QStyleOptionFrame option;
            option.initFrom(&frame);
            option.frameShape = shape;
            option.lineWidth = 1;
            option.palette.setColor(QPalette::WindowText, QColor::fromRgb(LineColor));

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

    protected:
        int m_argc = 1;                                     // QApplication keeps a reference to it
        char m_programName[5] = "test";
        char* m_argv[2] = { m_programName, nullptr };
        AZStd::unique_ptr<QApplication> m_application;
        AZStd::unique_ptr<Style> m_style;
    };

    // ----------------------------------------------------------------------------------
    // T-CPS.1: the picker separator is Native's band (ColorPicker.qss:83-90), not the universal frame line
    // ----------------------------------------------------------------------------------

    TEST_F(FlatFramePaintTests, HorizontalSeparator_IsTheNativeBandAtAWholeDevicePixelOffset)
    {
        ASSERT_TRUE(StyleManager::stylesheetsDisabled());

        // Gallery 07_color-picker__dialog0 (n1 and m0 qss), first separator: 270x3 at logical y 236, ratio 1.5.
        // Device rows at x 228: 354 #393a3c, 355 #2d2e2f, 356 #222222, 357 #393a3c, 358 #2d2e2f
        const QImage image = paintFrame(QFrame::HLine, QStringLiteral("HorizontalSeparator"),
            QPoint(17, 236), QSize(270, 3), 1.5, QSize(456, 400), DialogColor);

        EXPECT_EQ(pixelAt(image, 228, 353), DialogColor);
        EXPECT_EQ(pixelAt(image, 228, 354), 0x393A3Cu);
        EXPECT_EQ(pixelAt(image, 228, 355), 0x2D2E2Fu);
        EXPECT_EQ(pixelAt(image, 228, 356), 0x222222u);
        EXPECT_EQ(pixelAt(image, 228, 357), 0x393A3Cu);
        EXPECT_EQ(pixelAt(image, 228, 358), 0x2D2E2Fu);
        EXPECT_EQ(pixelAt(image, 228, 359), DialogColor);

        // The ends: the clip takes the 25.5 and 430.5 device edges to columns 26 and 430, and the antialiased top
        // border blends there horizontally too (Native n1: row 356 reads #444444 #222222 ... #222222 #444444 at
        // columns 25 26 ... 430 431, and row 354 reads #2d2e2f at column 430)
        EXPECT_EQ(pixelAt(image, 25, 356), DialogColor);
        EXPECT_EQ(pixelAt(image, 26, 356), 0x222222u);
        EXPECT_EQ(pixelAt(image, 430, 356), 0x222222u);
        EXPECT_EQ(pixelAt(image, 431, 356), DialogColor);
        EXPECT_EQ(pixelAt(image, 430, 354), 0x2D2E2Fu);
    }

    TEST_F(FlatFramePaintTests, HorizontalSeparator_IsTheNativeBandAtAHalfDevicePixelOffset)
    {
        ASSERT_TRUE(StyleManager::stylesheetsDisabled());

        // The fourth separator of the same capture, logical y 535 (device 802.5). The band stays inside the
        // device rect of the widget: 802 #444444, 803 #393a3c, 804 #222222, 805 #2d2e2f, 806 #393a3c, 807 #444444
        const QImage image = paintFrame(QFrame::HLine, QStringLiteral("HorizontalSeparator"),
            QPoint(17, 535), QSize(270, 3), 1.5, QSize(456, 900), DialogColor);

        EXPECT_EQ(pixelAt(image, 228, 802), DialogColor);
        EXPECT_EQ(pixelAt(image, 228, 803), 0x393A3Cu);
        EXPECT_EQ(pixelAt(image, 228, 804), 0x222222u);
        EXPECT_EQ(pixelAt(image, 228, 805), 0x2D2E2Fu);
        EXPECT_EQ(pixelAt(image, 228, 806), 0x393A3Cu);
        EXPECT_EQ(pixelAt(image, 228, 807), DialogColor);
    }

    TEST_F(FlatFramePaintTests, HorizontalSeparator_IsOneLogicalPixelBordersAndNoSideBorders)
    {
        ASSERT_TRUE(StyleManager::stylesheetsDisabled());

        // ColorPicker.qss:85-88: border-top and border-bottom 1px solid #393a3c, border-left and border-right none
        const QImage image = paintFrame(QFrame::HLine, QStringLiteral("HorizontalSeparator"),
            QPoint(17, 10), QSize(270, 3), 1.0, QSize(456, 40), DialogColor);

        EXPECT_EQ(pixelAt(image, 100, 9), DialogColor);
        EXPECT_EQ(pixelAt(image, 100, 10), 0x393A3Cu);
        EXPECT_EQ(pixelAt(image, 100, 11), 0x222222u);
        EXPECT_EQ(pixelAt(image, 100, 12), 0x393A3Cu);
        EXPECT_EQ(pixelAt(image, 100, 13), DialogColor);

        // The fill reaches both ends of the middle row; the border rows run the whole width
        EXPECT_EQ(pixelAt(image, 16, 11), DialogColor);
        EXPECT_EQ(pixelAt(image, 17, 11), 0x222222u);
        EXPECT_EQ(pixelAt(image, 286, 11), 0x222222u);
        EXPECT_EQ(pixelAt(image, 287, 11), DialogColor);
        EXPECT_EQ(pixelAt(image, 17, 10), 0x393A3Cu);
        EXPECT_EQ(pixelAt(image, 286, 12), 0x393A3Cu);
    }

    // ----------------------------------------------------------------------------------
    // T-CPS.2: the exact-QFrame rule still serves the frames it served (BaseStyleSheet.qss:41-48)
    // ----------------------------------------------------------------------------------

    TEST_F(FlatFramePaintTests, ExactQFrameLine_KeepsTheUniversalFillAndLine)
    {
        ASSERT_TRUE(StyleManager::stylesheetsDisabled());

        // An untagged exact QFrame: `.QFrame[frameShape="4"] { background-color: #444444 }` and a 1px WindowText line
        const QImage image = paintFrame(QFrame::HLine, QString(), QPoint(17, 10), QSize(270, 3), 1.0, QSize(456, 40), CardColor);

        EXPECT_EQ(pixelAt(image, 100, 9), CardColor);
        EXPECT_EQ(pixelAt(image, 100, 10), 0x444444u);
        EXPECT_EQ(pixelAt(image, 100, 11), LineColor);
        EXPECT_EQ(pixelAt(image, 100, 12), 0x444444u);
        EXPECT_EQ(pixelAt(image, 100, 13), CardColor);
    }

    TEST_F(FlatFramePaintTests, HorizontalSeparatorTag_IsAWholeWordNotAPrefix)
    {
        ASSERT_TRUE(StyleManager::stylesheetsDisabled());

        // The picker's container tag starts with the separator tag; Style::hasClass compares whole words, so a
        // frame carrying it must not take the picker band. What such a class-tagged exact QFrame does draw is not
        // pinned here: Native gives it no BaseStyleSheet fill (the open Card separator / TimerLine gap).
        const QImage image = paintFrame(QFrame::HLine, QStringLiteral("HorizontalSeparatorContainer"),
            QPoint(17, 10), QSize(270, 3), 1.0, QSize(456, 40), CardColor);

        EXPECT_NE(pixelAt(image, 100, 10), 0x393A3Cu);
        EXPECT_NE(pixelAt(image, 100, 11), 0x222222u);
        EXPECT_NE(pixelAt(image, 100, 12), 0x393A3Cu);
    }

    TEST_F(FlatFramePaintTests, ExactQFrameVLineAndPanel_KeepTheUniversalFill)
    {
        ASSERT_TRUE(StyleManager::stylesheetsDisabled());

        const QImage vline = paintFrame(QFrame::VLine, QString(), QPoint(17, 10), QSize(3, 20), 1.0, QSize(456, 40), CardColor);
        EXPECT_EQ(pixelAt(vline, 16, 20), CardColor);
        EXPECT_EQ(pixelAt(vline, 17, 20), 0x444444u);
        EXPECT_EQ(pixelAt(vline, 18, 20), LineColor);
        EXPECT_EQ(pixelAt(vline, 19, 20), 0x444444u);

        const QImage panel = paintFrame(QFrame::StyledPanel, QString(), QPoint(17, 10), QSize(30, 20), 1.0, QSize(456, 40), CardColor);
        EXPECT_EQ(pixelAt(panel, 30, 20), 0x444444u);
        EXPECT_EQ(pixelAt(panel, 16, 20), CardColor);
        EXPECT_EQ(pixelAt(panel, 30, 31), CardColor);
    }
} // namespace AzQtComponents
