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
#include <AzQtComponents/Components/Titlebar.h>
#include <AzQtComponents/Components/WindowDecorationWrapper.h>

#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>

#include <QApplication>
#include <QImage>
#include <QLabel>
#include <QStyleFactory>

#include <string>

namespace AzQtComponents
{
    namespace
    {
        //! The colour of one pixel, without alpha
        QRgb windowFramePixel(const QImage& image, int x, int y)
        {
            return image.pixelColor(x, y).rgb() & 0xFFFFFF;
        }

        //! The number of black pixels in a row, from a pixel on in a direction
        int windowFrameBlackRun(const QImage& image, int x, int y, int stepX, int stepY)
        {
            int run = 0;
            while (x >= 0 && y >= 0 && x < image.width() && y < image.height() && windowFramePixel(image, x, y) == 0x000000u)
            {
                ++run;
                x += stepX;
                y += stepY;
            }
            return run;
        }

        //! Margins as text, for the failure message of a comparison
        std::string windowFrameMarginsText(const QMargins& margins)
        {
            return QStringLiteral("(%1, %2, %3, %4)").arg(margins.left()).arg(margins.top()).arg(margins.right()).arg(margins.bottom()).toStdString();
        }
    } // namespace

    // ----------------------------------------------------------------------------------
    // Fixture: the flattened style chain (Style -> Fusion) under an application
    // ----------------------------------------------------------------------------------

    class WindowFrameTests
        : public ::UnitTest::LeakDetectionFixture
    {
    public:
        WindowFrameTests()
        {
            m_application = AZStd::make_unique<QApplication>(m_argc, m_argv);

            // As StyleManager::initialize builds it (StyleManager.cpp:244-246); the application owns it
            QApplication::setStyle(new Style(QStyleFactory::create(QStringLiteral("Fusion"))));
        }

        ~WindowFrameTests() override
        {
            m_application.reset();
        }

        //! A window frame of the given logical size, painted by render() the way a window is painted, into an image of
        //! the given device ratio. Its interior is the window color (not black), so the black of the frame shows.
        static QImage paintFrame(const QSize& size, qreal ratio)
        {
            WindowDecorationWrapper wrapper(WindowDecorationWrapper::Option::OptionDisabled);
            wrapper.resize(size);

            QImage image((QSizeF(size) * ratio).toSize(), QImage::Format_ARGB32_Premultiplied);
            image.setDevicePixelRatio(ratio);
            image.fill(Qt::magenta);
            wrapper.render(&image);
            return image;
        }

    protected:
        int m_argc = 1;                                     // QApplication keeps a reference to it
        char m_programName[5] = "test";
        char* m_argv[2] = { m_programName, nullptr };
        AZStd::unique_ptr<QApplication> m_application;
    };

    // ----------------------------------------------------------------------------------
    // T-WF.1: the frame box. WindowDecorationWrapper.qss:9-11 `border: 1px solid black` is a 1px box inside which
    // the window's contents lie, and which paintEvent draws.
    // ----------------------------------------------------------------------------------

    TEST_F(WindowFrameTests, Frame_BoxIsTheOnePixelBorder)
    {
        ASSERT_TRUE(StyleManager::stylesheetsDisabled());

        // The frame reads its box while it is constructed, before the style knows its class: the polish reads it again
        WindowDecorationWrapper wrapper(WindowDecorationWrapper::Option::OptionDisabled);
        wrapper.ensurePolished();

        const QMargins margins = wrapper.contentsMargins();
        EXPECT_TRUE(margins == QMargins(1, 1, 1, 1)) << windowFrameMarginsText(margins);
    }

    // ----------------------------------------------------------------------------------
    // T-WF.2: the line. The gutter is painted as fills of its logical rectangles, so that at a fractional device
    // ratio the line is as wide as the gutter left to the guest and the title bar: their edges round the same way
    // ----------------------------------------------------------------------------------

    TEST_F(WindowFrameTests, Paint_LineIsOneDevicePixelAtTheWholeRatio)
    {
        ASSERT_TRUE(StyleManager::stylesheetsDisabled());

        const QImage image = paintFrame(QSize(100, 40), 1.0);
        ASSERT_EQ(image.width(), 100);
        ASSERT_EQ(image.height(), 40);
        ASSERT_NE(windowFramePixel(image, 50, 20), 0x000000u);

        EXPECT_EQ(windowFrameBlackRun(image, 0, 20, 1, 0), 1);
        EXPECT_EQ(windowFrameBlackRun(image, 99, 20, -1, 0), 1);
        EXPECT_EQ(windowFrameBlackRun(image, 50, 0, 0, 1), 1);
        EXPECT_EQ(windowFrameBlackRun(image, 50, 39, 0, -1), 1);
    }

    TEST_F(WindowFrameTests, Paint_LineFillsTheGutterAtAFractionalRatio)
    {
        ASSERT_TRUE(StyleManager::stylesheetsDisabled());

        // 100x40 logical is 150x60 device at 150%. The 1px gutter on the left and the top ends at 1.5, which rounds to
        // 2 device pixels; the one on the right and the bottom starts at 148.5 and 58.5, which round to 149 and 59:
        // 1 device pixel, reaching the edge of the window. (A stroked rectangle left 1px, one pixel inside the edge.)
        const QImage image = paintFrame(QSize(100, 40), 1.5);
        ASSERT_EQ(image.width(), 150);
        ASSERT_EQ(image.height(), 60);
        ASSERT_NE(windowFramePixel(image, 75, 30), 0x000000u);

        EXPECT_EQ(windowFrameBlackRun(image, 0, 30, 1, 0), 2);
        EXPECT_EQ(windowFrameBlackRun(image, 149, 30, -1, 0), 1);
        EXPECT_EQ(windowFrameBlackRun(image, 75, 0, 0, 1), 2);
        EXPECT_EQ(windowFrameBlackRun(image, 75, 59, 0, -1), 1);
    }

    // ----------------------------------------------------------------------------------
    // T-WF.3: the title of the title bar. TitleBar.qss:31-37 `AzQtComponents--TitleBar #title { margin-left: 10px;
    // margin-right: 16px }` matches at any depth (the label lies in a container of the bar, Titlebar.cpp:150,
    // 161-162), and Text.qss:93-94 gives every QLabel a margin of 1px above and below.
    // ----------------------------------------------------------------------------------

    TEST_F(WindowFrameTests, TitleLabel_BoxIsTheMarginsOfTheSheet)
    {
        ASSERT_TRUE(StyleManager::stylesheetsDisabled());

        TitleBar titleBar;
        titleBar.ensurePolished();

        const QLabel* title = titleBar.findChild<QLabel*>(QStringLiteral("title"));
        ASSERT_TRUE(title != nullptr);
        ASSERT_TRUE(title->parentWidget() != &titleBar);

        const QMargins margins = title->contentsMargins();
        EXPECT_TRUE(margins == QMargins(10, 1, 16, 1)) << windowFrameMarginsText(margins);
    }
} // namespace AzQtComponents
