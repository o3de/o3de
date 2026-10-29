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
#include <AzQtComponents/Components/Widgets/SegmentBar.h>

#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>

#include <QApplication>
#include <QDockWidget>
#include <QFile>
#include <QFont>
#include <QIcon>
#include <QImage>
#include <QList>
#include <QPainter>
#include <QPushButton>
#include <QScrollBar>
#include <QStyleFactory>
#include <QStyleOptionButton>
#include <QStyleOptionDockWidget>
#include <QStyleOptionToolButton>
#include <QTableView>

namespace AzQtComponents
{
    //! Named, not anonymous: a unity build joins this file with the other tests, where anonymous namespaces merge
    namespace FlatWidgetTestsInternal
    {
        // SegmentControl.qss:14-33 (the box) and :35-66 (the overlap), stated again with the source line that grants each
        constexpr int SegmentHeight = 30;                   // content 28 + border 1 + 1 (:19, :23)
        constexpr int SegmentHeightFollowingVertical = 29;  // margin-top -1px (:65)
        constexpr int SegmentWidth = 102;                   // min-width 68 + padding 16 + 16 + border 1 + 1 (:19, :29, :31-32), a short label
        constexpr int SegmentWidthFollowingHorizontal = 101;// margin-left -1px (:37, :54)
        constexpr int SegmentLabelLeft = 17;                // border 1 + padding 16: the text-align: left content box (:57-60)

        const QColor SegmentFill(0x33, 0x33, 0x33);         // :16
        const QColor SegmentBorder(0x22, 0x22, 0x22);       // :17
        const QColor DockTitleFill(0x33, 0x33, 0x33);       // QDockWidget.qss:22
        const QColor DockTitleText(0xCC, 0xCC, 0xCC);       // BaseStyleSheet.qss:60-66 QDockWidget { color: #cccccc }

        //! One of the segment buttons of a bar, in the order the tabs were added
        QPushButton* segmentButton(SegmentBar& bar, int index)
        {
            const QList<QPushButton*> buttons = bar.findChildren<QPushButton*>();
            return index < buttons.size() ? buttons.at(index) : nullptr;
        }

        //! The brightest channel of any pixel of the image
        int brightestChannel(const QImage& image)
        {
            int brightest = 0;
            for (int y = 0; y < image.height(); ++y)
            {
                for (int x = 0; x < image.width(); ++x)
                {
                    const QColor pixel = image.pixelColor(x, y);
                    brightest = qMax(brightest, qMax(pixel.red(), qMax(pixel.green(), pixel.blue())));
                }
            }
            return brightest;
        }

        //! The leftmost column holding a pixel that is not the background (-1: none)
        int firstInkColumn(const QImage& image, const QColor& background)
        {
            for (int x = 0; x < image.width(); ++x)
            {
                for (int y = 0; y < image.height(); ++y)
                {
                    if (image.pixelColor(x, y) != background)
                    {
                        return x;
                    }
                }
            }
            return -1;
        }

        //! How many pixels differ from the background
        int inkPixels(const QImage& image, const QColor& background)
        {
            int count = 0;
            for (int y = 0; y < image.height(); ++y)
            {
                for (int x = 0; x < image.width(); ++x)
                {
                    count += (image.pixelColor(x, y) != background) ? 1 : 0;
                }
            }
            return count;
        }
    } // namespace FlatWidgetTestsInternal

    // ----------------------------------------------------------------------------------
    // Fixture: the flattened style chain (Style -> Fusion) under an application
    // ----------------------------------------------------------------------------------

    class FlatWidgetTests
        : public ::UnitTest::LeakDetectionFixture
    {
    public:
        FlatWidgetTests()
        {
            m_application = AZStd::make_unique<QApplication>(m_argc, m_argv);

            // As StyleManager::initialize builds it (StyleManager.cpp:244-246); the application owns it
            QApplication::setStyle(new Style(QStyleFactory::create(QStringLiteral("Fusion"))));
        }

        ~FlatWidgetTests() override
        {
            m_application.reset();
        }

        //! A segment bar with three short tabs, polished, the buttons at the size they ask for
        static void fillBar(SegmentBar& bar)
        {
            bar.addTab(QStringLiteral("Red"));
            bar.addTab(QStringLiteral("Green"));
            bar.addTab(QStringLiteral("Blue"));
            bar.ensurePolished();
            for (QPushButton* button : bar.findChildren<QPushButton*>())
            {
                button->resize(button->sizeHint());
            }
        }

        //! One control element of a button painted through the style over the given background
        static QImage paintButton(QPushButton* button, QStyle::ControlElement element, const QColor& background)
        {
            QStyleOptionButton option;
            option.initFrom(button);
            option.state |= QStyle::State_Enabled;
            if (button->isChecked())
            {
                option.state |= QStyle::State_On;
            }
            option.text = button->text();
            option.palette.setColor(QPalette::ButtonText, Qt::white);

            QImage image(button->size(), QImage::Format_ARGB32_Premultiplied);
            image.fill(background);
            QPainter painter(&image);
            painter.setFont(button->font());
            QApplication::style()->drawControl(element, &option, &painter, button);
            painter.end();
            return image;
        }

    protected:
        int m_argc = 1;                                     // QApplication keeps a reference to it
        char m_programName[5] = "test";
        char* m_argv[2] = { m_programName, nullptr };
        AZStd::unique_ptr<QApplication> m_application;
    };

    // ----------------------------------------------------------------------------------
    // T-SB: SegmentBar buttons, horizontal and vertical (SegmentControl.qss; Native AP navigation 30, 29 x 8)
    // ----------------------------------------------------------------------------------

    TEST_F(FlatWidgetTests, SegmentBar_VerticalButtonsAreThirtyThenTwentyNineHigh)
    {
        using namespace FlatWidgetTestsInternal;
        ASSERT_TRUE(StyleManager::stylesheetsDisabled()); // the test environment sets it (AzQtComponentTests.cpp)

        SegmentBar bar(Qt::Vertical);
        fillBar(bar);

        // The buttons sit in the bar's inner widget: the orientation is found on an ancestor, not on the parent
        ASSERT_TRUE(segmentButton(bar, 2) != nullptr);
        EXPECT_EQ(segmentButton(bar, 0)->sizeHint().height(), SegmentHeight);
        EXPECT_EQ(segmentButton(bar, 1)->sizeHint().height(), SegmentHeightFollowingVertical);
        EXPECT_EQ(segmentButton(bar, 2)->sizeHint().height(), SegmentHeightFollowingVertical);

        // No margin-left on a vertical bar: every button is as wide as the first
        for (int index = 0; index < 3; ++index)
        {
            EXPECT_EQ(segmentButton(bar, index)->sizeHint().width(), SegmentWidth);
        }
    }

    TEST_F(FlatWidgetTests, SegmentBar_HorizontalButtonsOverlapTheNeighbourOnTheLeft)
    {
        using namespace FlatWidgetTestsInternal;
        ASSERT_TRUE(StyleManager::stylesheetsDisabled());

        SegmentBar bar(Qt::Horizontal);
        fillBar(bar);

        ASSERT_TRUE(segmentButton(bar, 2) != nullptr);
        for (int index = 0; index < 3; ++index)
        {
            EXPECT_EQ(segmentButton(bar, index)->sizeHint().height(), SegmentHeight);
        }
        EXPECT_EQ(segmentButton(bar, 0)->sizeHint().width(), SegmentWidth);
        EXPECT_EQ(segmentButton(bar, 1)->sizeHint().width(), SegmentWidthFollowingHorizontal);
        EXPECT_EQ(segmentButton(bar, 2)->sizeHint().width(), SegmentWidthFollowingHorizontal);
    }

    TEST_F(FlatWidgetTests, SegmentBar_LongLabelWidensTheButtonByTheBoxOnly)
    {
        using namespace FlatWidgetTestsInternal;
        ASSERT_TRUE(StyleManager::stylesheetsDisabled());

        SegmentBar bar(Qt::Vertical);
        bar.addTab(QStringLiteral("A segment label that is much wider than the minimum content width of the rule"));
        bar.ensurePolished();
        QPushButton* button = segmentButton(bar, 0);
        ASSERT_TRUE(button != nullptr);

        // content = the label (QPushButton::sizeHint hands it to the style), box = border + padding on both sides
        const int labelWidth = button->fontMetrics().size(Qt::TextShowMnemonic, button->text()).width();
        ASSERT_GT(labelWidth, 68);
        EXPECT_EQ(button->sizeHint().width(), labelWidth + 2 * (1 + 16));
    }

    TEST_F(FlatWidgetTests, SegmentBar_VerticalFollowingButtonOverlapsTheBorderAboveIt)
    {
        using namespace FlatWidgetTestsInternal;
        ASSERT_TRUE(StyleManager::stylesheetsDisabled());

        SegmentBar bar(Qt::Vertical);
        fillBar(bar);

        // The first tab is checked (#555555); the second is not. Its border box starts one row above the widget, so its top
        // row is fill and the line above belongs to the neighbour; the bottom row and the left column are the border.
        const QImage first = paintButton(segmentButton(bar, 0), QStyle::CE_PushButtonBevel, Qt::magenta);
        EXPECT_TRUE(first.pixelColor(10, 0) == SegmentBorder);
        EXPECT_TRUE(first.pixelColor(10, first.height() - 1) == SegmentBorder);

        const QImage second = paintButton(segmentButton(bar, 1), QStyle::CE_PushButtonBevel, Qt::magenta);
        EXPECT_TRUE(second.pixelColor(10, 0) == SegmentFill);
        EXPECT_TRUE(second.pixelColor(10, second.height() - 1) == SegmentBorder);
        EXPECT_TRUE(second.pixelColor(0, 10) == SegmentBorder);
        EXPECT_TRUE(second.pixelColor(second.width() - 1, 10) == SegmentBorder);
    }

    TEST_F(FlatWidgetTests, SegmentBar_HorizontalFollowingButtonKeepsItsTopBorderAndOverlapsLeft)
    {
        using namespace FlatWidgetTestsInternal;
        ASSERT_TRUE(StyleManager::stylesheetsDisabled());

        SegmentBar bar(Qt::Horizontal);
        fillBar(bar);

        // As before this change: the border box starts one column to the left of the widget
        const QImage second = paintButton(segmentButton(bar, 1), QStyle::CE_PushButtonBevel, Qt::magenta);
        EXPECT_TRUE(second.pixelColor(10, 0) == SegmentBorder);
        EXPECT_TRUE(second.pixelColor(0, 10) == SegmentFill);
        EXPECT_TRUE(second.pixelColor(second.width() - 1, 10) == SegmentBorder);
    }

    TEST_F(FlatWidgetTests, SegmentBar_VerticalLabelIsLeftAlignedInTheContentBox)
    {
        using namespace FlatWidgetTestsInternal;
        ASSERT_TRUE(StyleManager::stylesheetsDisabled());

        SegmentBar vertical(Qt::Vertical);
        fillBar(vertical);
        SegmentBar horizontal(Qt::Horizontal);
        fillBar(horizontal);

        // SegmentControl.qss:57-60: the text starts at the border and padding (a glyph's own bearing allows a pixel or two)
        const QImage left = paintButton(segmentButton(vertical, 1), QStyle::CE_PushButtonLabel, Qt::black);
        const int leftInk = firstInkColumn(left, Qt::black);
        EXPECT_GE(leftInk, SegmentLabelLeft);
        EXPECT_LE(leftInk, SegmentLabelLeft + 3);

        // A horizontal bar keeps the centred label
        const QImage centred = paintButton(segmentButton(horizontal, 1), QStyle::CE_PushButtonLabel, Qt::black);
        EXPECT_GT(firstInkColumn(centred, Qt::black), SegmentLabelLeft + 10);
    }

    // ----------------------------------------------------------------------------------
    // T-SC: scroll bar hint (ScrollBar.qss:10-25; Native dumps: 8x56 and 56x8 for 138 scroll bars, minimum size hint 64x64 of a view)
    // ----------------------------------------------------------------------------------

    TEST_F(FlatWidgetTests, ScrollBar_SizeHintIsTheLegacyBoxAlongTheBar)
    {
        ASSERT_TRUE(StyleManager::stylesheetsDisabled());

        QScrollBar vertical(Qt::Vertical);
        QScrollBar horizontal(Qt::Horizontal);
        vertical.ensurePolished();
        horizontal.ensurePolished();

        EXPECT_TRUE(vertical.sizeHint() == QSize(8, 56));
        EXPECT_TRUE(horizontal.sizeHint() == QSize(56, 8));
    }

    TEST_F(FlatWidgetTests, ScrollBar_ItemViewMinimumSizeHintFollowsTheScrollBars)
    {
        ASSERT_TRUE(StyleManager::stylesheetsDisabled());

        // QAbstractScrollArea::minimumSizeHint: one bar's length plus the other's thickness, no frame (Style::polish clears it)
        QTableView view;
        view.ensurePolished();

        EXPECT_TRUE(view.minimumSizeHint() == QSize(64, 64));
    }

    // ----------------------------------------------------------------------------------
    // T-DK: the plain QDockWidget (QDockWidget.qss:14-31)
    // ----------------------------------------------------------------------------------

    TEST_F(FlatWidgetTests, PlainDockWidget_FloatButtonShowsThePopOutGlyph)
    {
        ASSERT_TRUE(StyleManager::stylesheetsDisabled());
        ASSERT_TRUE(QFile::exists(QStringLiteral(":/Application/titlebar-popout-small.svg")));
        ASSERT_TRUE(QFile::exists(QStringLiteral(":/Application/titlebar-restore.svg")));

        QDockWidget dock(QStringLiteral("Log"));
        QStyleOptionDockWidget option;
        option.initFrom(&dock);

        // QDockWidget.qss:17 titlebar-normal-icon (the exact class); the close glyph is the same everywhere (:16)
        const QSize size(16, 16);
        const QImage floatGlyph = QApplication::style()->standardIcon(QStyle::SP_TitleBarNormalButton, &option, &dock).pixmap(size).toImage();
        EXPECT_TRUE(floatGlyph == QIcon(QStringLiteral(":/Application/titlebar-popout-small.svg")).pixmap(size).toImage());
        EXPECT_FALSE(floatGlyph == QIcon(QStringLiteral(":/Application/titlebar-restore.svg")).pixmap(size).toImage());
    }

    TEST_F(FlatWidgetTests, PlainDockWidget_TitleButtonsDrawTheirGlyph)
    {
        using namespace FlatWidgetTestsInternal;
        ASSERT_TRUE(StyleManager::stylesheetsDisabled());

        QDockWidget dock(QStringLiteral("Log"));
        auto* closeButton = dock.findChild<QAbstractButton*>(QStringLiteral("qt_dockwidget_closebutton"));
        ASSERT_TRUE(closeButton != nullptr);
        ASSERT_FALSE(closeButton->icon().isNull());
        closeButton->resize(18, 14);    // the Native and flattened dumps agree on the button box

        // What QDockWidgetTitleButton::paintEvent hands the style (qdockwidget.cpp): no sub controls, a 10px icon
        QStyleOptionToolButton option;
        option.initFrom(closeButton);
        option.state |= QStyle::State_AutoRaise;
        option.icon = closeButton->icon();
        option.subControls = QStyle::SubControls();
        option.activeSubControls = QStyle::SubControls();
        option.features = QStyleOptionToolButton::None;
        option.arrowType = Qt::NoArrow;
        option.iconSize = QSize(10, 10);

        // The whole button rect reaches the label (a tool button's inset by PM_ButtonMargin left 6x2)
        EXPECT_TRUE(QApplication::style()->subControlRect(QStyle::CC_ToolButton, &option, QStyle::SC_ToolButton, closeButton) == option.rect);

        QImage image(18, 14, QImage::Format_ARGB32_Premultiplied);
        image.fill(DockTitleFill);
        QPainter painter(&image);
        QApplication::style()->drawComplexControl(QStyle::CC_ToolButton, &option, &painter, closeButton);
        painter.end();

        // A 10x10 glyph centred in the box: ink in the middle, none within two columns of the edge
        EXPECT_GT(inkPixels(image.copy(4, 2, 10, 10), DockTitleFill), 10);
        EXPECT_EQ(inkPixels(image.copy(0, 0, 3, 14), DockTitleFill), 0);
        EXPECT_EQ(inkPixels(image.copy(15, 0, 3, 14), DockTitleFill), 0);
    }

    TEST_F(FlatWidgetTests, PlainDockWidget_TitleButtonsHaveNoPanelUnderTheMouse)
    {
        using namespace FlatWidgetTestsInternal;
        ASSERT_TRUE(StyleManager::stylesheetsDisabled());

        QDockWidget dock(QStringLiteral("Log"));
        auto* floatButton = dock.findChild<QAbstractButton*>(QStringLiteral("qt_dockwidget_floatbutton"));
        ASSERT_TRUE(floatButton != nullptr);
        floatButton->resize(18, 14);

        // QDockWidget.qss:26-31 border: none; background-color: transparent. Fusion raises a panel on a hovered button.
        QStyleOption option;
        option.initFrom(floatButton);
        option.state |= QStyle::State_Enabled | QStyle::State_AutoRaise | QStyle::State_Raised | QStyle::State_MouseOver;

        QImage image(18, 14, QImage::Format_ARGB32_Premultiplied);
        image.fill(DockTitleFill);
        QPainter painter(&image);
        QApplication::style()->drawPrimitive(QStyle::PE_PanelButtonTool, &option, &painter, floatButton);
        painter.end();

        EXPECT_EQ(inkPixels(image, DockTitleFill), 0);
    }

    TEST_F(FlatWidgetTests, PlainDockWidget_TitleTextIsCccccInAnyState)
    {
        using namespace FlatWidgetTestsInternal;
        ASSERT_TRUE(StyleManager::stylesheetsDisabled());

        QDockWidget dock(QStringLiteral("Log"));
        QStyleOptionDockWidget option;
        option.initFrom(&dock);
        option.rect = QRect(0, 0, 200, 19);
        option.title = QStringLiteral("Log");
        option.closable = true;
        option.floatable = true;
        option.movable = true;

        for (const bool enabled : { true, false })
        {
            option.state.setFlag(QStyle::State_Enabled, enabled);
            option.palette.setCurrentColorGroup(enabled ? QPalette::Active : QPalette::Disabled);

            // A bold face, so that some pixel is covered whole whatever font the machine falls back to
            QFont font = dock.font();
            font.setPixelSize(16);
            font.setBold(true);

            QImage image(option.rect.size(), QImage::Format_ARGB32_Premultiplied);
            image.fill(Qt::magenta);
            QPainter painter(&image);
            painter.setFont(font);
            QApplication::style()->drawControl(QStyle::CE_DockWidgetTitle, &option, &painter, &dock);
            painter.end();

            // The strip is #333333 to its corners; the text is #cccccc where a glyph covers a pixel, never white
            EXPECT_TRUE(image.pixelColor(0, 0) == DockTitleFill);
            EXPECT_TRUE(image.pixelColor(199, 18) == DockTitleFill);
            EXPECT_EQ(brightestChannel(image), DockTitleText.red());

            // The text starts 4px in from the left (qfusionstyle.cpp:3473-3487), not at the former 8px
            const int ink = firstInkColumn(image, DockTitleFill);
            EXPECT_GE(ink, 4);
            EXPECT_LT(ink, 8);
        }
    }
} // namespace AzQtComponents
