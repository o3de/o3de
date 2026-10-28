/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzTest/AzTest.h>
#include <AzQtComponents/Components/DockBarButton.h>
#include <AzQtComponents/Components/DockMainWindow.h>
#include <AzQtComponents/Components/Elements/BoxElement.h>
#include <AzQtComponents/Components/Elements/ControlElements.h>
#include <AzQtComponents/Components/Elements/SpinBoxElement.h>
#include <AzQtComponents/Components/FilteredSearchWidget.h>
#include <AzQtComponents/Components/Style.h>
#include <AzQtComponents/Components/StyleManager.h>
#include <AzQtComponents/Components/Titlebar.h>

#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>

#include <QApplication>
#include <QDoubleSpinBox>
#include <QFrame>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QImage>
#include <QLineEdit>
#include <QMainWindow>
#include <QPainter>
#include <QPushButton>
#include <QSpinBox>
#include <QSplitter>
#include <QStackedLayout>
#include <QStyleFactory>
#include <QStyleOption>
#include <QStyleOptionSpinBox>
#include <QToolBar>
#include <QVBoxLayout>

#include <algorithm>
#include <initializer_list>

namespace AzQtComponents
{
    // ----------------------------------------------------------------------------------
    // The Global elements and flat code of the small controls, each beside the legacy rules it carries (Splitter.qss, ToolBar.qss,
    // TitleBar.qss, FilteredSearchWidget.qss, SpinBox.qss, LineEdit.qss). The legacy twin of a widget is Qt's own style sheet style
    // over Fusion with those rules; the flat twin is the flattened Style. Same boxes, same hints, same pixels.
    // ----------------------------------------------------------------------------------

    //! Named, not anonymous: a unity build joins this file with the other tests, where anonymous namespaces merge
    namespace ControlElementTestsInternal
    {
        // Splitter.qss:9-24
        const char* const SplitterRules =
            "QSplitter::handle { background-color: #222222; }"
            "QSplitter::handle:horizontal { width: 1px; }"
            "QSplitter::handle:vertical { height: 1px; }"
            "QSplitter::handle:hover { background-color: #1E70EB; }";

        // ToolBar.qss:15-29 and :71-137 (the rules of the toolbar and its handle); `MainWindow QToolBar` (:31-34) is #mainWindow
        // here: the flat twin's window carries the class name Native's selector named, the legacy twin's sheet its object name.
        // The spacing of the items is the style's, ToolBar::itemSpacing (ToolBarConfig.ini, 12), under Native's sheet; the legacy
        // twin has Fusion under it, so the sheet states it
        const char* const ToolBarRules =
            "QToolBar { background-color: #444444; border: 0 none; padding: 5px; spacing: 12px; }"
            "QToolBar::handle:horizontal { image: url(:/stylesheet/img/UI20/tear.svg); }"
            "QToolBar::handle:vertical { image: url(:/stylesheet/img/UI20/tear-vertical.svg); }"
            "QToolBar.MainToolBar.IconLarge::handle:horizontal { padding: 1px 4px; }"
            "QToolBar.MainToolBar.IconLarge::handle:vertical { padding: 2px 1px; }"
            "QToolBar.MainToolBar::handle:horizontal, QToolBar.MainToolBar.IconNormal::handle:horizontal { padding: 1px 4px; }"
            "QToolBar.MainToolBar::handle:vertical, QToolBar.MainToolBar.IconNormal::handle:vertical { padding: 2px -2px; }"
            "QToolBar.IconLarge::handle:horizontal { padding: 4px 3px; }"
            "QToolBar.IconLarge::handle:vertical { padding: 2px -2px; }"
            "QToolBar::handle:horizontal, QToolBar.IconNormal::handle:horizontal { padding: 2px 4px; }"
            "QToolBar::handle:vertical, QToolBar.IconNormal::handle:vertical { padding: 2px -2px; }";
        const char* const ToolBarMainWindowRule = "#mainWindow QToolBar { border-bottom: 2px solid #111111; }";

        //! A widget that paints one colour over its whole rect: independent of the palette and of the style
        class Swatch
            : public QWidget
        {
        public:
            Swatch(const QColor& color, QWidget* parent)
                : QWidget(parent)
                , m_color(color)
            {
            }

        protected:
            void paintEvent(QPaintEvent*) override
            {
                QPainter(this).fillRect(rect(), m_color);
            }

        private:
            QColor m_color;
        };

        //! A QMainWindow that answers to the class name of the Editor's main window, as Qt's `inherits` asks the class
        //! (qt_metacast); a class without Q_OBJECT in a test has no meta object of its own
        class NamedMainWindow
            : public QMainWindow
        {
        public:
            using QMainWindow::QMainWindow;

            void* qt_metacast(const char* name) override
            {
                return qstrcmp(name, "MainWindow") == 0 ? static_cast<void*>(this) : QMainWindow::qt_metacast(name);
            }
        };

        // ----------------------------------------------------------------------------------
        // Pixels. A control is painted through its style onto an image of an explicit device ratio, with the options its paint event
        // gives the style: the image is the painter's device, so the style scales by the image's ratio. QWidget::render would hand the
        // style the widget instead, and the ratio of the screen the test runs on (FlatFramePaintTests.cpp, paintThroughItsStyle).
        // ----------------------------------------------------------------------------------

        //! An image of a logical size at a device ratio, filled with the colour behind the control
        QImage newImage(const QSize& logical, qreal ratio, const QColor& behind)
        {
            QImage image((QSizeF(logical) * ratio).toSize(), QImage::Format_ARGB32_Premultiplied);
            image.setDevicePixelRatio(ratio);
            image.fill(behind);
            return image;
        }

        //! A handle of a splitter as QSplitterHandle::paintEvent asks the splitter's style for it (qsplitter.cpp:306-324): CE_Splitter
        //! over the contents rect of the handle, in the state of the splitter's orientation and of the pointer. The handle sits
        //! `offset` logical pixels from the image's origin (an odd offset puts it on a half device pixel at the ratio 1.5)
        QImage drawSplitterHandle(const QSplitter* splitter, int index, bool hover, qreal ratio, const QPoint& offset)
        {
            const QWidget* handle = splitter->handle(index);
            QImage image = newImage(splitter->size() + QSize(4, 4), ratio, QColor(0x44, 0x44, 0x44));
            QPainter painter(&image);
            painter.translate(offset + handle->pos());
            QStyleOption option;
            option.rect = handle->contentsRect();
            option.state = QStyle::State_Enabled | (splitter->orientation() == Qt::Horizontal ? QStyle::State_Horizontal : QStyle::State_None)
                | (hover ? QStyle::State_MouseOver : QStyle::State_None);
            splitter->style()->drawControl(QStyle::CE_Splitter, &option, &painter, splitter);
            return image;
        }

        //! A toolbar as QToolBar::paintEvent asks its style for it (qtoolbar.cpp:925-948): the panel (CE_ToolBar) and, of a movable
        //! toolbar, the handle in the rect the style gives it (SE_ToolBarHandle), clipped to the toolbar's rect as a widget paints.
        //! `behind` is what the toolbar's last device row blends with at a fractional ratio
        QImage drawToolBar(QToolBar* toolBar, const QSize& size, bool horizontal, bool handle, qreal ratio, const QPoint& offset, const QColor& behind)
        {
            QImage image = newImage(size + QSize(2 * offset.x() + 2, 2 * offset.y() + 2), ratio, behind);
            QPainter painter(&image);
            painter.translate(offset);
            painter.setClipRect(QRect(QPoint(0, 0), size));
            QStyleOptionToolBar option;
            option.initFrom(toolBar);
            option.state = QStyle::State_Enabled | (horizontal ? QStyle::State_Horizontal : QStyle::State_None);
            option.rect = QRect(QPoint(0, 0), size);
            option.features = handle ? QStyleOptionToolBar::Movable : QStyleOptionToolBar::None;
            toolBar->style()->drawControl(QStyle::CE_ToolBar, &option, &painter, toolBar);
            if (handle)
            {
                option.rect = toolBar->style()->subElementRect(QStyle::SE_ToolBarHandle, &option, toolBar);
                if (option.rect.isValid())
                {
                    toolBar->style()->drawPrimitive(QStyle::PE_IndicatorToolBarHandle, &option, &painter, toolBar);
                }
            }
            return image;
        }

        //! A push button as QPushButton::paintEvent asks its style for it: CE_PushButton with the options the button gives
        QImage drawPushButton(const QPushButton* button, qreal ratio, const QColor& behind)
        {
            QImage image = newImage(button->size(), ratio, behind);
            QPainter painter(&image);
            QStyleOptionButton option;
            option.initFrom(button);
            option.rect = button->rect();
            option.features = button->isFlat() ? QStyleOptionButton::Flat : QStyleOptionButton::None;
            option.icon = button->icon();
            option.iconSize = button->iconSize();
            option.state |= QStyle::State_Raised;
            button->style()->drawControl(QStyle::CE_PushButton, &option, &painter, button);
            return image;
        }

        //! A block of the pixels of a Native capture (rrggbb words, a row to a string), from a column and a row of the image of a control
        struct NativeBlock
        {
            const char* m_what;
            int m_x;
            int m_y;
            std::initializer_list<const char*> m_rows;
        };

        void expectBlock(const QImage& image, const NativeBlock& block)
        {
            int row = block.m_y;
            for (const char* line : block.m_rows)
            {
                int column = block.m_x;
                for (const QByteArray& word : QByteArray(line).split(' '))
                {
                    const QColor expected(QStringLiteral("#") + QString::fromLatin1(word));
                    EXPECT_EQ(image.pixelColor(column, row).rgb(), expected.rgb()) << block.m_what << " at " << column << "," << row;
                    ++column;
                }
                ++row;
            }
        }

        //! How many pixels of two images differ (every pixel when the images are not the same size)
        int differing(const QImage& a, const QImage& b)
        {
            if (a.size() != b.size())
            {
                return a.width() * a.height();
            }
            int count = 0;
            for (int y = 0; y < a.height(); ++y)
            {
                for (int x = 0; x < a.width(); ++x)
                {
                    count += (a.pixel(x, y) != b.pixel(x, y)) ? 1 : 0;
                }
            }
            return count;
        }

        // ----------------------------------------------------------------------------------
        // Fixture: the flattened Style over Fusion, Fusion as the base of the legacy sheet, the Global elements registered
        // ----------------------------------------------------------------------------------

        class ControlElementTests
            : public ::UnitTest::LeakDetectionFixture
        {
        public:
            ControlElementTests()
            {
                m_application = AZStd::make_unique<QApplication>(m_argc, m_argv);
                QApplication::setStyle(QStyleFactory::create(QStringLiteral("Fusion")));

                // The application palette of Native: Base #444444 (the n2 dumps; Style::polish(QApplication*) sets it in the flattened GUI).
                // A rule with no background draws the base of the application palette, whatever machine the test runs on
                QPalette palette = QApplication::palette();
                palette.setColor(QPalette::Base, QColor(0x44, 0x44, 0x44));
                QApplication::setPalette(palette);
                m_style = AZStd::make_unique<Style>(QStyleFactory::create(QStringLiteral("Fusion")));
                ControlElements::registerGlobal();      // once for the process, as Style::polish(QApplication*) calls it
            }

            ~ControlElementTests() override
            {
                m_style.reset();
                m_application.reset();
            }

            //! The legacy twin: the sheet on the root, over the application's Fusion. The flat twin: the style on the root and on
            //! everything under it (a widget's style is not its children's).
            static void legacy(QWidget& root, const QString& rules)
            {
                root.setStyleSheet(rules);
            }

            void flat(QWidget& root) const
            {
                root.setStyle(m_style.get());
                for (QWidget* child : root.findChildren<QWidget*>())
                {
                    child->setStyle(m_style.get());
                }
            }

            static void show(QWidget& root, const QSize& size)
            {
                root.setAttribute(Qt::WA_DontShowOnScreen);
                root.resize(size);
                root.show();
                QApplication::processEvents();
            }

            //! One tree twice, shown: the build function fills a root with it, the rules are the legacy twin's sheet
            template<typename Root>
            struct Twins
            {
                template<typename Build>
                Twins(const ControlElementTests& tests, Build build, const QString& rules, const QSize& size)
                {
                    build(m_legacy);
                    legacy(m_legacy, rules);
                    show(m_legacy, size);
                    build(m_flat);
                    tests.flat(m_flat);
                    show(m_flat, size);
                }

                template<typename W>
                W* legacyChild(const char* name)
                {
                    return m_legacy.template findChild<W*>(QString::fromLatin1(name));
                }

                template<typename W>
                W* flatChild(const char* name)
                {
                    return m_flat.template findChild<W*>(QString::fromLatin1(name));
                }

                Root m_legacy;
                Root m_flat;
            };

        protected:
            int m_argc = 1;                                     // QApplication keeps a reference to it
            char m_programName[5] = "test";
            char* m_argv[2] = { m_programName, nullptr };
            AZStd::unique_ptr<QApplication> m_application;
            AZStd::unique_ptr<Style> m_style;
        };

        // ----------------------------------------------------------------------------------
        // T-SPL: the splitter handle (Splitter.qss; Native AssetBrowser splitter: panes 1px apart, handle widget 5px)
        // ----------------------------------------------------------------------------------

        //! A splitter of two swatches, laid out the way the capture's asset browser is
        void buildSplitter(QWidget& root, Qt::Orientation orientation)
        {
            QSplitter* splitter = new QSplitter(orientation, &root);
            splitter->setObjectName(QStringLiteral("splitter"));
            splitter->setGeometry(0, 0, 300, 120);
            splitter->addWidget(new Swatch(QColor(0x80, 0x20, 0x20), splitter));
            splitter->addWidget(new Swatch(QColor(0x20, 0x80, 0x20), splitter));
            splitter->setSizes({ 100, 100 });
        }

        TEST_F(ControlElementTests, Splitter_PanesAreOnePixelApartAndTheHandleGrabsTwoMoreOnEachSideAsTheSheetHasIt)
        {
            ASSERT_TRUE(StyleManager::stylesheetsDisabled());   // the test environment sets it (AzQtComponentTests.cpp)

            for (const Qt::Orientation orientation : { Qt::Horizontal, Qt::Vertical })
            {
                Twins<QWidget> twins(*this, [&](QWidget& root) { buildSplitter(root, orientation); }, QString::fromUtf8(SplitterRules), QSize(300, 120));
                const QSplitter* legacySplitter = twins.legacyChild<QSplitter>("splitter");
                const QSplitter* flatSplitter = twins.flatChild<QSplitter>("splitter");

                // The Native numbers (n2 Editor, Gallery, Material Editor: every handle): a handle width of 1 (Splitter.qss:17-19, read
                // with no option as a vertical handle), a hint of 1x1, contents margins of 2 on the sides along the splitter
                EXPECT_EQ(legacySplitter->handleWidth(), 1);
                EXPECT_EQ(flatSplitter->handleWidth(), legacySplitter->handleWidth());

                const QList<QWidget*> legacyChildren = legacySplitter->findChildren<QWidget*>(QString(), Qt::FindDirectChildrenOnly);
                const QList<QWidget*> flatChildren = flatSplitter->findChildren<QWidget*>(QString(), Qt::FindDirectChildrenOnly);
                ASSERT_EQ(flatChildren.size(), legacyChildren.size());
                ASSERT_GE(flatChildren.size(), 3);          // two panes and the handles (the first, hidden, one is the first pane's)
                for (int index = 0; index < flatChildren.size(); ++index)
                {
                    EXPECT_EQ(flatChildren[index]->geometry(), legacyChildren[index]->geometry()) << "child " << index;
                    EXPECT_TRUE(flatChildren[index]->contentsMargins() == legacyChildren[index]->contentsMargins()) << "child " << index;
                    EXPECT_EQ(flatChildren[index]->sizeHint(), legacyChildren[index]->sizeHint()) << "child " << index;
                }
                const QWidget* handle = flatSplitter->handle(1);
                const QMargins margins = handle->contentsMargins();
                EXPECT_EQ(handle->sizeHint(), QSize(1, 1));
                EXPECT_TRUE(margins == (orientation == Qt::Horizontal ? QMargins(2, 0, 2, 0) : QMargins(0, 2, 0, 2)));

                // The handle's pixels at the ratio of one and of the 1.5 the captures were taken at, on a whole device pixel (offset 0) and
                // on a half one (an offset of 1 is 1.5 device pixels)
                for (const qreal ratio : { 1.0, 1.5 })
                {
                    for (const QPoint offset : { QPoint(0, 0), QPoint(1, 1) })
                    {
                        EXPECT_EQ(differing(drawSplitterHandle(legacySplitter, 1, false, ratio, offset),
                                      drawSplitterHandle(flatSplitter, 1, false, ratio, offset)), 0)
                            << (orientation == Qt::Horizontal ? "horizontal" : "vertical") << " splitter at ratio " << ratio << ", offset " << offset.x();
                    }
                }
            }
        }

        TEST_F(ControlElementTests, Splitter_HandleLineIsOneDeviceColumnOnAHalfPixelAndTwoOnAWholeOneAsInTheCaptures)
        {
            // n2: a 1px handle is two device columns of #222222 where it starts on a whole device pixel (Editor m_splitter at x 256:
            // columns 384 and 385; Material Canvas at x 2046: 3069 and 3070) and one where it starts on a half one (Material Canvas at
            // x 1743, device x 2614.5: column 2615). Qt's fill takes the pixels whose centres lie inside the rect.
            QWidget root;
            buildSplitter(root, Qt::Horizontal);
            flat(root);
            show(root, QSize(300, 120));
            const QSplitter* splitter = root.findChild<QSplitter*>(QStringLiteral("splitter"));
            ASSERT_EQ(splitter->handle(1)->contentsRect().width(), 1);
            const int contentsX = splitter->handle(1)->x() + splitter->handle(1)->contentsRect().x();     // logical, 100

            const QColor line(0x22, 0x22, 0x22);
            const QColor behind(0x44, 0x44, 0x44);
            const QImage whole = drawSplitterHandle(splitter, 1, false, 1.5, QPoint(0, 0));
            const int wholeX = contentsX * 3 / 2;
            for (const int column : { wholeX - 1, wholeX + 2 })
            {
                EXPECT_EQ(whole.pixelColor(column, 60).rgb(), behind.rgb()) << column;
            }
            for (const int column : { wholeX, wholeX + 1 })
            {
                EXPECT_EQ(whole.pixelColor(column, 60).rgb(), line.rgb()) << column;
            }

            const QImage half = drawSplitterHandle(splitter, 1, false, 1.5, QPoint(1, 0));
            const int halfX = (contentsX + 1) * 3 / 2 + 1;                      // x 101 is device 151.5: the column 152
            for (const int column : { halfX - 1, halfX + 1 })
            {
                EXPECT_EQ(half.pixelColor(column, 60).rgb(), behind.rgb()) << column;
            }
            EXPECT_EQ(half.pixelColor(halfX, 60).rgb(), line.rgb());
        }

        TEST_F(ControlElementTests, Splitter_TheHandleUnderThePointerIsBlueAndFillsTheWholeRectItIsGiven)
        {
            // CE_Splitter as QSplitterHandle::paintEvent calls it (qsplitter.cpp:306-324), in the state the pointer gives it, over
            // the rects of a default handle (1px) and of one a module gave a width (AssetBrowserEntityInspectorWidget.cpp:242: 2px)
            Twins<QWidget> twins(*this, [](QWidget& root) { buildSplitter(root, Qt::Horizontal); }, QString::fromUtf8(SplitterRules), QSize(300, 120));
            const QSplitter* legacySplitter = twins.legacyChild<QSplitter>("splitter");
            const QSplitter* flatSplitter = twins.flatChild<QSplitter>("splitter");

            for (const int width : { 1, 2, 5 })
            {
                for (const bool hover : { false, true })
                {
                    QImage images[2];
                    int index = 0;
                    for (const QSplitter* splitter : { legacySplitter, flatSplitter })
                    {
                        QStyleOption option;
                        option.rect = QRect(0, 0, width, 40);
                        option.state = QStyle::State_Horizontal | QStyle::State_Enabled | (hover ? QStyle::State_MouseOver : QStyle::State_None);
                        QImage& image = images[index++];
                        image = QImage(QSize(width, 40) * 3 / 2, QImage::Format_ARGB32_Premultiplied);
                        image.setDevicePixelRatio(1.5);
                        image.fill(Qt::transparent);
                        QPainter painter(&image);
                        splitter->style()->drawControl(QStyle::CE_Splitter, &option, &painter, splitter);
                    }
                    EXPECT_EQ(differing(images[0], images[1]), 0) << "handle of " << width << "px, hover " << hover;
                    EXPECT_TRUE(images[1].pixelColor(0, 0) == (hover ? QColor(0x1E, 0x70, 0xEB) : QColor(0x22, 0x22, 0x22))) << width << " " << hover;
                }
            }
        }

        // ----------------------------------------------------------------------------------
        // T-TBR: the panel and the handle of a toolbar (ToolBar.qss; Native LuaIDE and Gallery toolbars: buttons 1px further in, tear dots)
        // ----------------------------------------------------------------------------------

        //! A toolbar of three 18px swatches in a main window, as the Native LuaIDE and Gallery toolbars hold their buttons
        QToolBar* buildToolBar(QMainWindow& window, const char* tags, bool movable, Qt::ToolBarArea area = Qt::TopToolBarArea, int items = 3, int itemHeight = 18)
        {
            window.setObjectName(QStringLiteral("mainWindow"));        // the legacy sheet's stand-in for the class `MainWindow`
            QToolBar* toolBar = new QToolBar(&window);
            toolBar->setObjectName(QStringLiteral("toolBar"));
            toolBar->setMovable(movable);
            if (tags)
            {
                toolBar->setProperty("class", QString::fromLatin1(tags));
            }
            for (int index = 0; index < items; ++index)
            {
                Swatch* swatch = new Swatch(QColor(0x40 + 0x30 * (index % 4), 0xC0 - 0x20 * (index % 3), 0x60 + 0x20 * (index % 5)), toolBar);
                swatch->setFixedSize(18, itemHeight);
                toolBar->addWidget(swatch);
            }
            window.addToolBar(area, toolBar);
            window.setCentralWidget(new Swatch(QColor(0x30, 0x30, 0x30), &window));
            return toolBar;
        }

        //! The twins' toolbars paint the same pixels, panel and handle, at the ratio of one and of the 1.5 the captures were taken at, on a
        //! whole device pixel (offset 0) and on a half one (an offset of 1 is 1.5 device pixels). The colour behind is none of the panel's
        //! (the Editor's top line blends with #444444 below it, but a panel that is not painted must show)
        void expectToolBarsPaintAlike(QToolBar* legacyBar, QToolBar* flatBar, bool horizontal, bool handle, const QString& what)
        {
            ASSERT_EQ(flatBar->size(), legacyBar->size()) << qPrintable(what);
            for (const qreal ratio : { 1.0, 1.5 })
            {
                for (const QPoint offset : { QPoint(0, 0), QPoint(1, 1) })
                {
                    EXPECT_EQ(differing(drawToolBar(legacyBar, legacyBar->size(), horizontal, handle, ratio, offset, QColor(0x20, 0x40, 0x60)),
                                  drawToolBar(flatBar, flatBar->size(), horizontal, handle, ratio, offset, QColor(0x20, 0x40, 0x60))), 0)
                        << qPrintable(what) << " at ratio " << ratio << ", offset " << offset.x();
                }
            }
        }

        TEST_F(ControlElementTests, ToolBarHandle_ExtentPaddingAndImageAreThoseOfTheWinningRuleForEveryLook)
        {
            ASSERT_TRUE(StyleManager::stylesheetsDisabled());

            const char* const looks[] = { nullptr, "IconNormal", "IconLarge", "MainToolBar", "MainToolBar IconNormal", "MainToolBar IconLarge" };
            for (const char* look : looks)
            {
                for (const Qt::ToolBarArea area : { Qt::TopToolBarArea, Qt::LeftToolBarArea })
                {
                    Twins<QMainWindow> twins(*this, [&](QMainWindow& window) { buildToolBar(window, look, true, area); },
                        QString::fromUtf8(ToolBarRules), QSize(400, 300));
                    QToolBar* legacyBar = twins.legacyChild<QToolBar>("toolBar");
                    QToolBar* flatBar = twins.flatChild<QToolBar>("toolBar");
                    const QString what = QStringLiteral("%1, %2").arg(QString::fromLatin1(look ? look : "plain"), area == Qt::TopToolBarArea ? QStringLiteral("horizontal") : QStringLiteral("vertical"));

                    // The size the toolbar asks for counts the handle's extent: 10 along a horizontal one (2 + 4 + 4), 8 for IconLarge
                    EXPECT_EQ(flatBar->sizeHint(), legacyBar->sizeHint()) << qPrintable(what);
                    EXPECT_EQ(flatBar->minimumSizeHint(), legacyBar->minimumSizeHint()) << qPrintable(what);
                    const QList<QWidget*> legacyItems = legacyBar->findChildren<QWidget*>(QString(), Qt::FindDirectChildrenOnly);
                    const QList<QWidget*> flatItems = flatBar->findChildren<QWidget*>(QString(), Qt::FindDirectChildrenOnly);
                    ASSERT_EQ(flatItems.size(), legacyItems.size()) << qPrintable(what);
                    for (int index = 0; index < flatItems.size(); ++index)
                    {
                        EXPECT_EQ(flatItems[index]->geometry(), legacyItems[index]->geometry()) << qPrintable(what) << " item " << index;
                    }
                    expectToolBarsPaintAlike(legacyBar, flatBar, area == Qt::TopToolBarArea, true, what);
                }
            }
        }

        TEST_F(ControlElementTests, ToolBarHandle_TearDotsAreThePixelsOfTheLuaIdeCapture)
        {
            // n2 LuaIDE fileToolBar (x 1, y 62, 128 x 34: a whole device row, a half device column): the three dots of tear.svg, each
            // 3 columns by 4 rows of the red channel, from the device column 15 (the toolbar's own column 13.5 + the dot at 1.5)
            QMainWindow window;
            QToolBar* bar = buildToolBar(window, nullptr, true, Qt::TopToolBarArea, 4, 24);
            flat(window);
            show(window, QSize(600, 300));
            const QImage image = drawToolBar(bar, QSize(128, 34), true, true, 1.5, QPoint(1, 0), QColor(0x44, 0x44, 0x44));

            const int dot[4][3] = { { 0x7F, 0xB0, 0x7F }, { 0xB8, 0xBB, 0xB8 }, { 0xB8, 0xBB, 0xB8 }, { 0x7F, 0xB0, 0x7F } };
            for (const int firstRow : { 24, 37 })             // device rows 117 and 130 of the capture, less the toolbar's 93
            {
                for (int row = 0; row < 4; ++row)
                {
                    for (int column = 0; column < 3; ++column)
                    {
                        EXPECT_EQ(image.pixelColor(15 + column, firstRow + row).red(), dot[row][column])
                            << "dot at row " << firstRow << ": " << row << "," << column;
                    }
                }
            }
        }

        TEST_F(ControlElementTests, ToolBarHandle_NativeNumbersOfTheLuaIdeAndGalleryToolbars)
        {
            // n2 LuaIDE fileToolBar: width 128 with four 18px buttons 12px apart, the first at x 15 of the toolbar (5 + a handle of
            // 10); the base style's handle of 9 made it 127 and 14. Not movable: no handle, the first button at 5
            for (const bool movable : { true, false })
            {
                QMainWindow window;
                QToolBar* bar = buildToolBar(window, nullptr, movable, Qt::TopToolBarArea, 4);
                window.setStyle(m_style.get());
                for (QWidget* child : window.findChildren<QWidget*>())
                {
                    child->setStyle(m_style.get());
                }
                show(window, QSize(600, 300));
                QList<QWidget*> items = bar->findChildren<QWidget*>(QString(), Qt::FindDirectChildrenOnly);
                items.erase(std::remove_if(items.begin(), items.end(), [](QWidget* item) { return !dynamic_cast<Swatch*>(item); }), items.end());
                ASSERT_EQ(items.size(), 4);
                EXPECT_EQ(items.first()->x(), movable ? 15 : 5);
                EXPECT_EQ(bar->sizeHint().width(), movable ? 128 : 118);
            }
        }

        //! A toolbar of an odd height, as n2's Editor main toolbar is (29): at the ratio of the captures the end rows of a line blend.
        //! The last four device rows are those of Native for the toolbar's panel (below the main window: #2a2a2a #111111 #111111
        //! #2a2a2a, the two logical pixels of the line on a half pixel; the base style's bevel there was #343434 #868686 #868686)
        template<typename Twin>
        void expectToolBarPanelsAlike(Twin& twins, const char* what, std::initializer_list<int> lastFourRows)
        {
            QToolBar* legacyBar = twins.template legacyChild<QToolBar>("toolBar");
            QToolBar* flatBar = twins.template flatChild<QToolBar>("toolBar");
            expectToolBarsPaintAlike(legacyBar, flatBar, true, false, QString::fromLatin1(what));

            // The flat panel at the ratio of the captures, over the #444444 of the window below it: the four last device rows of a
            // toolbar 29 high (43.5 device rows) are the rows 40 to 43
            const QImage image = drawToolBar(flatBar, flatBar->size(), true, false, 1.5, QPoint(0, 0), QColor(0x44, 0x44, 0x44));
            const int x = image.width() - 20;                       // right of the items, where only the panel is
            EXPECT_EQ(image.pixelColor(x, 10).rgb(), QColor(0x44, 0x44, 0x44).rgb()) << what;
            int row = 40;
            for (const int grey : lastFourRows)
            {
                EXPECT_EQ(image.pixelColor(x, row).rgb(), QColor(grey, grey, grey).rgb()) << what << " row " << row;
                ++row;
            }
        }

        TEST_F(ControlElementTests, ToolBarPanel_UnderTheMainWindowIsTheFillAndTheTwoPixelLineAtItsBottom)
        {
            ASSERT_TRUE(StyleManager::stylesheetsDisabled());

            Twins<NamedMainWindow> twins(*this, [](NamedMainWindow& window) { buildToolBar(window, nullptr, false, Qt::TopToolBarArea, 3, 19); },
                QString::fromUtf8(ToolBarRules) + QString::fromUtf8(ToolBarMainWindowRule), QSize(400, 200));
            ASSERT_EQ(twins.flatChild<QToolBar>("toolBar")->height(), 29);
            expectToolBarPanelsAlike(twins, "toolbar under the main window", { 0x2A, 0x11, 0x11, 0x2A });
        }

        TEST_F(ControlElementTests, ToolBarPanel_UnderADockMainWindowHasNoBorderAtAll)
        {
            // ToolBar.qss:36-39 `AzQtComponents--DockMainWindow QToolBar { border: 0 none }` follows the MainWindow rule, as specific: the
            // toolbars of the Editor's viewport panes have no line (n2: rows #444444 where the base style's bevel was dark and light
            // lines). The stand-in of the first rule has an id, so the second is given the same, to be as specific and later
            const QString rules = QString::fromUtf8(ToolBarRules) + QString::fromUtf8(ToolBarMainWindowRule)
                + QStringLiteral("#mainWindow AzQtComponents--DockMainWindow QToolBar { border: 0 none; }");
            Twins<NamedMainWindow> twins(*this, [](NamedMainWindow& window)
            {
                DockMainWindow* dock = new DockMainWindow(&window);
                buildToolBar(*dock, nullptr, false, Qt::TopToolBarArea, 3, 19);
                dock->setObjectName(QStringLiteral("dockMainWindow"));
                window.setObjectName(QStringLiteral("mainWindow"));
                window.setCentralWidget(dock);
            }, rules, QSize(400, 200));
            expectToolBarPanelsAlike(twins, "toolbar under a dock main window", { 0x44, 0x44, 0x44, 0x44 });
        }

        TEST_F(ControlElementTests, ToolBarPanel_OutsideAMainWindowIsTheFillAlone)
        {
            // A window that is not a `MainWindow`: the rule of ToolBar.qss:31-34 does not reach it
            Twins<QMainWindow> twins(*this, [](QMainWindow& window)
            {
                buildToolBar(window, nullptr, false, Qt::TopToolBarArea, 3, 19);
                window.setObjectName(QStringLiteral("otherWindow"));
            }, QString::fromUtf8(ToolBarRules) + QString::fromUtf8(ToolBarMainWindowRule), QSize(400, 200));
            expectToolBarPanelsAlike(twins, "toolbar of another window", { 0x44, 0x44, 0x44, 0x44 });
        }

        // ----------------------------------------------------------------------------------
        // T-TAB: the frame of the buttons of a dock's tab bar (TitleBar.qss; Native Script Canvas docks: buttons 2px higher)
        // ----------------------------------------------------------------------------------

        // TitleBar.qss:9-29 and :54-57, the rules that reach the frame; the buttons' sizes are code in the flattened GUI, as are the
        // height of the title bar (PM_TitleBarHeight) and the stacked page of a title bar drawn as a tab bar
        const char* const TitleBarRules =
            "AzQtComponents--TitleBar { background-color: #111111; border: 1px solid #000000; }"
            "AzQtComponents--TitleBar[drawAsTabBar=\"true\"] { border: none; }"
            "AzQtComponents--TitleBar * { background-color: transparent; }"
            "AzQtComponents--TitleBar[drawAsTabBar=\"true\"] #tabButtons { margin-bottom: 3px; }";

        // The tree is built under Fusion with the property preset, and given the Style after that (Twins): the boxes of the two twins.
        // The order the application builds it in is the next test
        TEST_F(ControlElementTests, TabButtons_FrameHasThreePixelsOfMarginBelowAndTheButtonsSitAtItsTop)
        {
            ASSERT_TRUE(StyleManager::stylesheetsDisabled());

            for (const bool drawAsTabBar : { true, false })
            {
                Twins<QWidget> twins(*this, [&](QWidget& root)
                {
                    TitleBar* bar = new TitleBar(&root);
                    bar->setGeometry(0, 0, 600, 30);
                    bar->setFixedHeight(30);
                    bar->setButtons({ DockBarButton::MinimizeButton, DockBarButton::MaximizeButton, DockBarButton::CloseButton });
                    bar->setDrawAsTabBar(drawAsTabBar);
                    // A title bar of a dock widget shows the page of the tab bar (Titlebar.cpp:432)
                    bar->findChild<QStackedLayout*>()->setCurrentIndex(0);
                }, QString::fromUtf8(TitleBarRules), QSize(600, 30));
                if (!drawAsTabBar)
                {
                    // The flattened Style's TitleBar::polish sets the property from the configuration (true) when the show polishes the
                    // title bar, and the sheet style leaves what the tree set: set false again, the element follows the property
                    twins.m_flat.findChild<TitleBar*>()->setDrawAsTabBar(false);
                    QApplication::processEvents();
                }
                const QFrame* legacyFrame = twins.legacyChild<QFrame>("tabButtons");
                const QFrame* flatFrame = twins.flatChild<QFrame>("tabButtons");

                // The Native numbers (n2 Editor Script Canvas docks): contents [0, 0, 0, 3] of a frame 30 high, and the 26px buttons at the
                // top of it where centring them in the whole frame put them at 2
                EXPECT_TRUE(legacyFrame->contentsMargins() == (drawAsTabBar ? QMargins(0, 0, 0, 3) : QMargins()));
                EXPECT_TRUE(flatFrame->contentsMargins() == legacyFrame->contentsMargins()) << drawAsTabBar;
                EXPECT_EQ(flatFrame->geometry(), legacyFrame->geometry()) << drawAsTabBar;
                const QList<DockBarButton*> legacyButtons = legacyFrame->findChildren<DockBarButton*>();
                const QList<DockBarButton*> flatButtons = flatFrame->findChildren<DockBarButton*>();
                ASSERT_EQ(flatButtons.size(), 3);
                ASSERT_EQ(legacyButtons.size(), 3);
                for (int index = 0; index < 3; ++index)
                {
                    EXPECT_EQ(flatButtons[index]->geometry(), legacyButtons[index]->geometry()) << drawAsTabBar << " button " << index;
                    if (drawAsTabBar)
                    {
                        EXPECT_EQ(flatButtons[index]->y(), 0) << "button " << index;
                    }
                }
            }
        }

        // The order of the application. The flattened Style is the application's from the start, so the frame is polished by it in the
        // constructor of its title bar (the stacked layout shows its first page there, which polishes everything on the page) while
        // drawAsTabBar is still false. TitleBar::polish sets the property at the first show, and the element follows the property.
        // The test above builds the tree under Fusion and gives it the Style after the property is set: it cannot see this order.
        TEST_F(ControlElementTests, TabButtons_AFrameBuiltUnderTheApplicationStyleFollowsTheProperty)
        {
            ASSERT_TRUE(StyleManager::stylesheetsDisabled());
            QApplication::setStyle(new Style(QStyleFactory::create(QStringLiteral("Fusion"))));

            QWidget root;
            TitleBar* bar = new TitleBar(&root);
            bar->setGeometry(0, 0, 600, 30);
            bar->setFixedHeight(30);
            bar->setButtons({ DockBarButton::MinimizeButton, DockBarButton::MaximizeButton, DockBarButton::CloseButton });
            QFrame* frame = bar->findChild<QFrame*>(QStringLiteral("tabButtons"));
            ASSERT_TRUE(frame != nullptr);

            // The order under test: the frame is polished, and has no element, before its title bar is polished
            EXPECT_TRUE(frame->testAttribute(Qt::WA_WState_Polished));
            EXPECT_FALSE(bar->drawAsTabBar());
            EXPECT_TRUE(BoxElement::forWidget(frame) == nullptr);

            show(root, QSize(600, 30));

            // The Native numbers (n2 Editor Script Canvas docks): contents [0, 0, 0, 3] of a frame 30 high, the 26px buttons at its top
            ASSERT_TRUE(bar->drawAsTabBar());
            EXPECT_TRUE(BoxElement::forWidget(frame) != nullptr);
            EXPECT_EQ(frame->height(), 30);
            EXPECT_TRUE(frame->contentsMargins() == QMargins(0, 0, 0, 3));
            const QLayout* layout = frame->layout();
            ASSERT_EQ(layout->count(), 3);
            for (int index = 0; index < 3; ++index)
            {
                EXPECT_EQ(layout->itemAt(index)->widget()->height(), 26) << "button " << index;
                EXPECT_EQ(layout->itemAt(index)->widget()->y(), 0) << "button " << index;
            }

            // The element follows the property both ways (TitleBar::unpolish sets it false)
            bar->setDrawAsTabBar(false);
            EXPECT_TRUE(BoxElement::forWidget(frame) == nullptr);
            EXPECT_TRUE(frame->contentsMargins() == QMargins());
            bar->setDrawAsTabBar(true);
            EXPECT_TRUE(BoxElement::forWidget(frame) != nullptr);
            EXPECT_TRUE(frame->contentsMargins() == QMargins(0, 0, 0, 3));
        }

        // ----------------------------------------------------------------------------------
        // T-TAG: the close button of a filter tag (FilteredSearchWidget.qss; Native Gallery, Material Editor, Pass Canvas: 16x16)
        // ----------------------------------------------------------------------------------

        // FilteredSearchWidget.qss:15-19 (`AzQtComponents--FilteredSearchWidget QPushButton`: #searchWidget here), :26-48 and :90-101
        const char* const FilterTagRules =
            "#searchWidget QPushButton { background-color: transparent; border: none; }"
            "AzQtComponents--FilterCriteriaButton { padding: 0px; border-radius: 2px; border: 1px solid #808080;"
            " background-color: rgb(46, 46, 46); min-height: 22px; max-height: 22px; }"
            "AzQtComponents--FilterCriteriaButton QPushButton#closeTag { qproperty-icon: url(:/stylesheet/img/close_x.svg);"
            " qproperty-iconSize: 10px 10px; qproperty-flat: true; background-color: transparent; padding: 0px; margin: 0px;"
            " max-height: 16px; max-width: 16px; min-width: 16px; }"
            "AzQtComponents--FilterCriteriaButton QLabel { background-color: transparent; padding: 0px; margin: 0px; }"
            "AzQtComponents--FilterTextButton { padding: 0px; border-radius: 2px; border: 1px solid #808080;"
            " background-color: rgb(46, 46, 46); min-height: 22px; max-height: 22px; }"
            "AzQtComponents--FilterTextButton QPushButton#closeTag { qproperty-icon: url(:/stylesheet/img/close_x.svg);"
            " qproperty-iconSize: 10px 10px; qproperty-flat: true; background-color: transparent; padding: 0px; margin: 0px;"
            " max-height: 16px; max-width: 16px; min-width: 16px; }";

        TEST_F(ControlElementTests, CloseTag_IsAFlatSixteenPixelButtonAtMostSixteenHighWithTheGlyphAndNoMinimumHeight)
        {
            ASSERT_TRUE(StyleManager::stylesheetsDisabled());

            for (const bool criteria : { true, false })
            {
                Twins<QWidget> twins(*this, [&](QWidget& root)
                {
                    root.setObjectName(QStringLiteral("searchWidget"));
                    QHBoxLayout* layout = new QHBoxLayout(&root);
                    layout->setContentsMargins(0, 0, 0, 0);
                    if (criteria)
                    {
                        layout->addWidget(new FilterCriteriaButton(QStringLiteral("Tag"), &root));
                    }
                    else
                    {
                        layout->addWidget(new FilterTextButton(QStringLiteral("Tag"), &root));
                    }
                }, QString::fromUtf8(FilterTagRules), QSize(200, 40));
                const QPushButton* legacyButton = twins.legacyChild<QPushButton>("closeTag");
                const QPushButton* flatButton = twins.flatChild<QPushButton>("closeTag");

                // The Native numbers (n2 Gallery 10_filtered-search-widget): 16x16, min [16, 0], max [16, 16], hint [16, 10]; the flat button
                // was 16 wide and 20 high, its maximum 24 (PushButton::polish) over the 16 the code gave it
                EXPECT_EQ(legacyButton->minimumSize(), QSize(16, 0));
                EXPECT_EQ(legacyButton->maximumSize(), QSize(16, 16));
                EXPECT_EQ(legacyButton->sizeHint(), QSize(16, 10));
                EXPECT_EQ(flatButton->minimumSize(), legacyButton->minimumSize()) << criteria;
                EXPECT_EQ(flatButton->maximumSize(), legacyButton->maximumSize()) << criteria;
                EXPECT_EQ(flatButton->sizeHint(), legacyButton->sizeHint()) << criteria;
                EXPECT_EQ(flatButton->size(), legacyButton->size()) << criteria;
                EXPECT_TRUE(flatButton->isFlat());
                EXPECT_EQ(flatButton->iconSize(), QSize(10, 10));
                EXPECT_FALSE(flatButton->icon().isNull());
                EXPECT_TRUE(BoxElement::forWidget(flatButton) != nullptr && BoxElement::forWidget(legacyButton) == nullptr);

                // The glyph over the chip's fill (FilteredSearchWidget.qss:31, rgb(46, 46, 46)), at the ratio of one and of the 1.5 the captures
                // were taken at
                for (const qreal ratio : { 1.0, 1.5 })
                {
                    EXPECT_EQ(differing(drawPushButton(legacyButton, ratio, QColor(46, 46, 46)), drawPushButton(flatButton, ratio, QColor(46, 46, 46))), 0)
                        << "close tag at ratio " << ratio << " criteria " << criteria;
                }
            }
        }

        // The order of the application again: the Style is the application's, no widget is given it after it is built. The button is
        // sized when it is polished (it was sized in the constructor of its tag before), so the size is the element's at the first show.
        TEST_F(ControlElementTests, CloseTag_AButtonBuiltUnderTheApplicationStyleHasTheElementsBoxAtTheFirstShow)
        {
            ASSERT_TRUE(StyleManager::stylesheetsDisabled());
            QApplication::setStyle(new Style(QStyleFactory::create(QStringLiteral("Fusion"))));

            for (const bool criteria : { true, false })
            {
                QWidget root;
                QHBoxLayout* layout = new QHBoxLayout(&root);
                layout->setContentsMargins(0, 0, 0, 0);
                if (criteria)
                {
                    layout->addWidget(new FilterCriteriaButton(QStringLiteral("Tag"), &root));
                }
                else
                {
                    layout->addWidget(new FilterTextButton(QStringLiteral("Tag"), &root));
                }
                show(root, QSize(200, 40));
                const QPushButton* button = root.findChild<QPushButton*>(QStringLiteral("closeTag"));
                ASSERT_TRUE(button != nullptr);

                // The Native numbers of the test above (n2 Gallery 10_filtered-search-widget, a FilterCriteriaButton): 16x16, min [16, 0],
                // max [16, 16], hint [16, 10]. A FilterTextButton has the rule repeated (FilteredSearchWidget.qss:90-101); no capture
                // holds one: UNVERIFIED
                EXPECT_TRUE(BoxElement::forWidget(button) != nullptr) << criteria;
                EXPECT_EQ(button->minimumSize(), QSize(16, 0)) << criteria;
                EXPECT_EQ(button->maximumSize(), QSize(16, 16)) << criteria;
                EXPECT_EQ(button->sizeHint(), QSize(16, 10)) << criteria;
                EXPECT_EQ(button->size(), QSize(16, 16)) << criteria;
                EXPECT_TRUE(button->isFlat());
                EXPECT_EQ(button->iconSize(), QSize(10, 10));
                EXPECT_FALSE(button->icon().isNull());
            }
        }

        // ----------------------------------------------------------------------------------
        // T-SPN: the spin box (SpinBox.qss over BaseStyleSheet.qss:68-84, VectorInput.qss:18-35, ColorPicker.qss:66-81)
        // ----------------------------------------------------------------------------------

        // A spin box of a vector element and of a color picker are named by class in the sheets. Their stand-ins in the twins are
        // Qt classes of the same specificity (a type selector: QFrame, QGroupBox) that the flat twin's matchers know by the class
        // name Native's selectors named (qt_metacast: a class without Q_OBJECT in a test has no meta object of its own)
        template<typename Base>
        class NamedAs
            : public Base
        {
        public:
            NamedAs(const char* className, QWidget* parent)
                : Base(parent)
                , m_className(className)
            {
            }

            void* qt_metacast(const char* name) override
            {
                return qstrcmp(name, m_className) == 0 ? static_cast<void*>(this) : Base::qt_metacast(name);
            }

        private:
            const char* m_className;
        };

        const char* const SpinBoxRules =
            // BaseStyleSheet.qss:68-84
            "QSpinBox, QDoubleSpinBox { background-color: #CCCCCC; color: black; border-radius: 2px; border-width: 0px; border-color: #e9e9e9; }"
            // SpinBox.qss:9-74
            "QSpinBox, QDoubleSpinBox { height: 16px; margin: 2px; border: none; padding-right: 12px; padding-left: 0px;"
            " padding-bottom: 1px; padding-top: -1px; }"
            "QSpinBox:focus, QDoubleSpinBox:focus, QSpinBox:hover, QDoubleSpinBox:hover { margin: 1px; border-width: 1px;"
            " border-style: solid; border-radius: 3px; }"
            "QSpinBox:hover, QDoubleSpinBox:hover { border-color: #222222; background-image: url(:/SpinBox/hoverBackground.svg);"
            " background-repeat: no-repeat; background-position: right; }"
            "QSpinBox:focus, QDoubleSpinBox:focus { border-color: #94D2FF; background-color: #FFFFFF; }"
            "QSpinBox:hover:focus, QDoubleSpinBox:hover:focus { background-image: url(:/SpinBox/focusedBackground.svg);"
            " background-repeat: no-repeat; background-position: right; }"
            "QSpinBox:enabled[hasAcceptableInput=false], QDoubleSpinBox:enabled[hasAcceptableInput=false],"
            " QSpinBox:enabled[HasError=true], QDoubleSpinBox:enabled[HasError=true] { margin: 0px; border-width: 2px;"
            " border-style: solid; border-radius: 4px; border-color: #E25243; }"
            "QSpinBox:disabled, QDoubleSpinBox:disabled { background-color: #666666; color: #999999; }"
            // SpinBox.qss:76-176, the buttons
            "QSpinBox::up-button, QDoubleSpinBox::up-button, QSpinBox::down-button, QDoubleSpinBox::down-button { width: 6px; height: 10px; }"
            "QSpinBox::down-button, QDoubleSpinBox::down-button { right: 8px; subcontrol-position: right center;"
            " image: url(:/SpinBox/arrowLeftHover.svg); }"
            "QSpinBox::up-button, QDoubleSpinBox::up-button { right: 2px; subcontrol-position: right center;"
            " image: url(:/SpinBox/arrowRightHover.svg); }"
            "QSpinBox[SpinBoxValueIncreasing=true]::up-button, QDoubleSpinBox[SpinBoxValueIncreasing=true]::up-button"
            " { image: url(:/SpinBox/arrowRightIncreasing.svg) }"
            "QSpinBox[SpinBoxValueIncreasing=true]::down-button, QDoubleSpinBox[SpinBoxValueIncreasing=true]::down-button"
            " { image: url(:/SpinBox/arrowLeftIncreasing.svg) }"
            "QSpinBox[SpinBoxValueDecreasing=true]::up-button, QDoubleSpinBox[SpinBoxValueDecreasing=true]::up-button"
            " { image: url(:/SpinBox/arrowRightDecreasing.svg) }"
            "QSpinBox[SpinBoxValueDecreasing=true]::down-button, QDoubleSpinBox[SpinBoxValueDecreasing=true]::down-button"
            " { image: url(:/SpinBox/arrowLeftDecreasing.svg) }"
            "QSpinBox[SpinBoxMinReached=true]::up-button, QDoubleSpinBox[SpinBoxMinReached=true]::up-button"
            " { image: url(:/SpinBox/arrowRightMinReached.svg) }"
            "QSpinBox[SpinBoxMinReached=true]::down-button, QDoubleSpinBox[SpinBoxMinReached=true]::down-button"
            " { image: url(:/SpinBox/arrowLeftMinReached.svg) }"
            "QSpinBox[SpinBoxMaxReached=true]::up-button, QDoubleSpinBox[SpinBoxMaxReached=true]::up-button"
            " { image: url(:/SpinBox/arrowRightMaxReached.svg) }"
            "QSpinBox[SpinBoxMaxReached=true]::down-button, QDoubleSpinBox[SpinBoxMaxReached=true]::down-button"
            " { image: url(:/SpinBox/arrowLeftMaxReached.svg) }"
            "QSpinBox[SpinBoxDragging=true][SpinBoxMaxReached=false][SpinBoxValueIncreasing=false]::up-button,"
            " QDoubleSpinBox[SpinBoxDragging=true][SpinBoxMaxReached=false][SpinBoxValueIncreasing=false]::up-button"
            " { image: url(:/SpinBox/arrowRightDragging.svg); }"
            "QSpinBox[SpinBoxDragging=true][SpinBoxMinReached=false][SpinBoxValueDecreasing=false]::down-button,"
            " QDoubleSpinBox[SpinBoxDragging=true][SpinBoxMinReached=false][SpinBoxValueDecreasing=false]::down-button"
            " { image: url(:/SpinBox/arrowLeftDragging.svg); }"
            "QSpinBox[SpinBoxFocused=true][SpinBoxMaxReached=false][SpinBoxValueIncreasing=false]::up-button,"
            " QDoubleSpinBox[SpinBoxFocused=true][SpinBoxMaxReached=false][SpinBoxValueIncreasing=false]::up-button"
            " { image: url(:/SpinBox/arrowRightFocused.svg); }"
            "QSpinBox[SpinBoxFocused=true][SpinBoxMinReached=false][SpinBoxValueDecreasing=false]::down-button,"
            " QDoubleSpinBox[SpinBoxFocused=true][SpinBoxMinReached=false][SpinBoxValueDecreasing=false]::down-button"
            " { image: url(:/SpinBox/arrowLeftFocused.svg); }"
            // VectorInput.qss:18-29, with QFrame for `AzQtComponents--VectorElement`
            "QFrame QSpinBox, QFrame QDoubleSpinBox, QFrame QSpinBox:hover, QFrame QDoubleSpinBox:hover, QFrame QSpinBox:focus,"
            " QFrame QDoubleSpinBox:focus { margin-left: 0px; border-left: 0px; border-top-left-radius: 0px; border-bottom-left-radius: 0px; }"
            // ColorPicker.qss:66-81, with QGroupBox for `AzQtComponents--ColorPicker`
            "QGroupBox QSpinBox:hover, QGroupBox QDoubleSpinBox:hover, QGroupBox QSpinBox:hover:focus, QGroupBox QDoubleSpinBox:hover:focus"
            " { background: none; }"
            "QGroupBox QSpinBox::down-button, QGroupBox QSpinBox::up-button, QGroupBox QDoubleSpinBox::down-button,"
            " QGroupBox QDoubleSpinBox::up-button { width: 0px; border-style: none; }";

        enum class SpinContext
        {
            Plain,
            Vector,
            ColorPicker,
        };

        //! A spin box of the given kind in the given place, 120 by 23 as an inspector row gives it
        QAbstractSpinBox* buildSpinBox(QWidget& root, SpinContext context, bool doubleBox)
        {
            QWidget* holder = nullptr;
            switch (context)
            {
            case SpinContext::Plain:
                holder = new QWidget(&root);
                break;
            case SpinContext::Vector:
                holder = new NamedAs<QFrame>("AzQtComponents::VectorElement", &root);
                break;
            case SpinContext::ColorPicker:
                holder = new NamedAs<QGroupBox>("AzQtComponents::ColorPicker", &root);
                break;
            }
            holder->setGeometry(0, 0, 200, 40);
            QAbstractSpinBox* spinBox = doubleBox ? static_cast<QAbstractSpinBox*>(new QDoubleSpinBox(holder)) : static_cast<QAbstractSpinBox*>(new QSpinBox(holder));
            spinBox->setObjectName(QStringLiteral("spin"));
            spinBox->setGeometry(0, 8, 120, 23);
            return spinBox;
        }

        //! CC_SpinBox as the widget's style draws it in a state, over the dark the box's margin shows, at a device ratio, as the
        //! widget would be painted `top` logical pixels from the top of the image (clipped to its rect: an odd top puts the box on
        //! a half device pixel at the ratio 1.5, where the captures have the boxes of an inspector's rows)
        QImage drawSpinBox(const QAbstractSpinBox* spinBox, QStyle::State state, qreal ratio, int top, bool buttons = true, int left = 0)
        {
            QStyleOptionSpinBox option;
            option.initFrom(spinBox);
            option.state = state;
            option.rect = QRect(0, 0, spinBox->width(), spinBox->height());
            // As QAbstractSpinBox::initStyleOption fills it: the buttons are subcontrols unless the box has none
            option.buttonSymbols = buttons ? QAbstractSpinBox::UpDownArrows : QAbstractSpinBox::NoButtons;
            option.subControls = QStyle::SC_SpinBoxFrame | QStyle::SC_SpinBoxEditField;
            if (buttons)
            {
                option.subControls |= QStyle::SC_SpinBoxUp | QStyle::SC_SpinBoxDown;
            }
            option.frame = true;
            QImage image = newImage(QSize(option.rect.width() + 2 * left, option.rect.height() + 2 * top), ratio, QColor(0x44, 0x44, 0x44));
            QPainter painter(&image);
            painter.translate(left, top);
            painter.setClipRect(option.rect);
            spinBox->style()->drawComplexControl(QStyle::CC_SpinBox, &option, &painter, spinBox);
            return image;
        }

        //! The properties of a spin box are set before the style polishes it: the watcher of the flattened Style re-polishes a spin box
        //! whose dynamic property changes (SpinBox.cpp, QEvent::DynamicPropertyChange) through StyleManager::styleSheetStyle, which needs
        //! the StyleManager a unit test does not have. PropertyInitFlag keeps the polish from setting the flags it initializes.
        void presetProperties(QAbstractSpinBox* spinBox, bool error, std::initializer_list<const char*> flagsSet)
        {
            spinBox->setProperty("PropertyInitFlag", true);
            for (const char* name : { "SpinBoxUpButtonPressed", "SpinBoxDownButtonPressed", "SpinBoxValueIncreasing", "SpinBoxValueDecreasing",
                "SpinBoxScrollIncreasing", "SpinBoxScrollDecreasing", "SpinBoxInitializedValue", "SpinBoxMinReached", "SpinBoxMaxReached",
                "SpinBoxFocused", "SpinBoxDragging" })
            {
                spinBox->setProperty(name, false);
            }
            for (const char* name : flagsSet)
            {
                spinBox->setProperty(name, true);
            }
            if (error)
            {
                spinBox->setProperty("HasError", true);
            }
        }

        TEST_F(ControlElementTests, SpinBox_EveryStateOfEveryPlaceHasTheSheetsFaceAtBothRatiosAndOnAHalfDevicePixel)
        {
            ASSERT_TRUE(StyleManager::stylesheetsDisabled());

            const QStyle::State enabled = QStyle::State_Enabled;
            const QStyle::State hover = QStyle::State_MouseOver;
            const QStyle::State focus = QStyle::State_HasFocus;
            const struct { const char* m_name; QStyle::State m_state; } states[] = {
                { "rest", enabled },
                { "disabled", QStyle::State_None },
                { "hover", enabled | hover },
                { "focus", enabled | focus },
                { "hover and focus", enabled | hover | focus },
            };
            const struct { const char* m_name; SpinContext m_context; } places[] = {
                { "plain", SpinContext::Plain }, { "vector element", SpinContext::Vector }, { "color picker", SpinContext::ColorPicker },
            };
            for (const auto& place : places)
            {
                for (const bool doubleBox : { false, true })
                {
                    for (const bool error : { false, true })
                    {
                        Twins<QWidget> twins(*this, [&](QWidget& root) { presetProperties(buildSpinBox(root, place.m_context, doubleBox), error, {}); },
                            QString::fromUtf8(SpinBoxRules), QSize(200, 40));
                        const QAbstractSpinBox* legacyBox = twins.legacyChild<QAbstractSpinBox>("spin");
                        const QAbstractSpinBox* flatBox = twins.flatChild<QAbstractSpinBox>("spin");
                        for (const auto& state : states)
                        {
                            for (const qreal ratio : { 1.0, 1.5 })
                            {
                                for (const int top : { 0, 1 })
                                {
                                    EXPECT_EQ(differing(drawSpinBox(legacyBox, state.m_state, ratio, top), drawSpinBox(flatBox, state.m_state, ratio, top)), 0)
                                        << place.m_name << (doubleBox ? " double" : " int") << " spin box, " << (error ? "error, " : "") << state.m_name
                                        << ", ratio " << ratio << ", " << top << " from the top";
                                }
                            }
                        }
                    }
                }
            }
        }

        TEST_F(ControlElementTests, SpinBox_GlyphsAreTheImagesOfTheRuleThatWinsForTheFlagsTheWatcherSets)
        {
            // SpinBox.qss:93-176: of the flags (increasing, decreasing, a limit reached, dragging, focused) the images of the buttons
            const std::initializer_list<const char*> flagSets[] = {
                {}, { "SpinBoxValueIncreasing" }, { "SpinBoxValueDecreasing" }, { "SpinBoxMinReached" }, { "SpinBoxMaxReached" },
                { "SpinBoxDragging" }, { "SpinBoxFocused" }, { "SpinBoxDragging", "SpinBoxFocused" },
                { "SpinBoxFocused", "SpinBoxValueIncreasing" }, { "SpinBoxFocused", "SpinBoxValueDecreasing" },
                { "SpinBoxFocused", "SpinBoxMaxReached" }, { "SpinBoxFocused", "SpinBoxMinReached" },
                { "SpinBoxDragging", "SpinBoxValueIncreasing" }, { "SpinBoxDragging", "SpinBoxMinReached" },
                { "SpinBoxValueIncreasing", "SpinBoxMaxReached" }, { "SpinBoxValueDecreasing", "SpinBoxMinReached" },
                { "SpinBoxValueIncreasing", "SpinBoxValueDecreasing", "SpinBoxMinReached", "SpinBoxMaxReached" },
            };
            int index = 0;
            for (const auto& flags : flagSets)
            {
                Twins<QWidget> twins(*this, [&](QWidget& root) { presetProperties(buildSpinBox(root, SpinContext::Plain, true), false, flags); },
                    QString::fromUtf8(SpinBoxRules), QSize(200, 40));
                const QAbstractSpinBox* legacyBox = twins.legacyChild<QAbstractSpinBox>("spin");
                const QAbstractSpinBox* flatBox = twins.flatChild<QAbstractSpinBox>("spin");
                for (const qreal ratio : { 1.0, 1.5 })
                {
                    EXPECT_EQ(differing(drawSpinBox(legacyBox, QStyle::State_Enabled, ratio, 0), drawSpinBox(flatBox, QStyle::State_Enabled, ratio, 0)), 0)
                        << "flag set " << index << " at ratio " << ratio;
                }
                ++index;
            }
        }

        TEST_F(ControlElementTests, SpinBox_OptionWithoutButtonSubcontrolsHasNoGlyphsAsTheSheetStyleLeavesThemOut)
        {
            ASSERT_TRUE(StyleManager::stylesheetsDisabled());

            // QAbstractSpinBox::NoButtons (StyledSpinBox.cpp:109) leaves the buttons out of the subcontrols of the option, and the
            // sheet draws each button only when its subcontrol is there (qstylesheetstyle.cpp:3123, :3137). The Style's polish gives a
            // spin box its buttons back (SpinBox::setButtonSymbolsForStyle), so the option is asked for here by hand
            for (const bool doubleBox : { false, true })
            {
                Twins<QWidget> twins(*this, [&](QWidget& root) { presetProperties(buildSpinBox(root, SpinContext::Plain, doubleBox), false, {}); },
                    QString::fromUtf8(SpinBoxRules), QSize(200, 40));
                const QAbstractSpinBox* legacyBox = twins.legacyChild<QAbstractSpinBox>("spin");
                const QAbstractSpinBox* flatBox = twins.flatChild<QAbstractSpinBox>("spin");
                const QStyle::State enabled = QStyle::State_Enabled;
                const QStyle::State states[] = { enabled, QStyle::State_None, enabled | QStyle::State_MouseOver };
                for (const QStyle::State state : states)
                {
                    for (const qreal ratio : { 1.0, 1.5 })
                    {
                        EXPECT_EQ(differing(drawSpinBox(legacyBox, state, ratio, 0, false), drawSpinBox(flatBox, state, ratio, 0, false)), 0)
                            << (doubleBox ? "double" : "int") << " spin box, state " << int(state) << ", ratio " << ratio;
                    }
                }

                // The field of a box at rest is the plain fill from the margin to the margin, nothing over it
                const QImage image = drawSpinBox(flatBox, QStyle::State_Enabled, 1.0, 0, false);
                for (int y = 3; y < image.height() - 3; ++y)
                {
                    for (int x = 3; x < image.width() - 3; ++x)
                    {
                        ASSERT_EQ(image.pixelColor(x, y).rgb(), QColor(0xCC, 0xCC, 0xCC).rgb()) << x << "," << y;
                    }
                }
            }
        }

        //! A block of pixels of the Native capture (rrggbb words, a row to a string) and the box it belongs to
        struct NativeSpinBlock
        {
            const char* m_what;
            SpinContext m_context;
            bool m_error;
            QStyle::State m_state;
            int m_boxWidth;
            int m_x;                                    //!< the block's first column in the image of the box
            int m_y;                                    //!< and its first row
            std::initializer_list<const char*> m_rows;
        };

        TEST_F(ControlElementTests, SpinBox_CornersAndEdgesOnAHalfDevicePixelAreThePixelsOfTheNativeCapture)
        {
            ASSERT_TRUE(StyleManager::stylesheetsDisabled());

            // n2 Gallery 23_spin-box.png (ratio 1.5), boxes 23 high at an odd logical y, so their top is at a half device pixel: the image
            // of the box here is drawn 1 logical pixel (1.5 device pixels) from its top and its left at a whole device pixel, as there
            // (defaultSpinBox at 712,439, hasErrorSpinBox 712,569, disabledSpinBox 712,595, a vector element's box at 726,621)
            const QStyle::State enabled = QStyle::State_Enabled;
            const NativeSpinBlock blocks[] = {
                { "rest, top left", SpinContext::Plain, false, enabled, 217, 2, 3, {
                    "444444 444444 444444 444444 444444 444444",
                    "444444 444444 535353 818181 888888 888888",
                    "444444 626262 c5c5c5 cccccc cccccc cccccc",
                    "444444 b2b2b2 cccccc cccccc cccccc cccccc",
                    "444444 cccccc cccccc cccccc cccccc cccccc",
                    "444444 cccccc cccccc cccccc cccccc cccccc",
                } },
                { "rest, bottom right", SpinContext::Plain, false, enabled, 217, 318, 29, {
                    "cccccc cccccc cccccc cccccc 888888 444444 444444 444444",
                    "cccccc cccccc cccccc cccccc 818181 444444 444444 444444",
                    "cccccc cccccc cccccc c4c4c4 535353 444444 444444 444444",
                    "cccccc cccccc b1b1b1 616161 444444 444444 444444 444444",
                    "444444 444444 444444 444444 444444 444444 444444 444444",
                } },
                { "error (margin 0, border 2, radius 4), top left", SpinContext::Plain, true, enabled, 217, 0, 1, {
                    "444444 444444 444444 444444 444444 444444 444444 444444",
                    "444444 444444 a24c44 d05044 e25243 e25243 e25243 e25243",
                    "444444 b24e44 e25243 e25243 e25243 e25243 e25243 e25243",
                    "634744 e15243 e25243 e25243 e15748 d88981 dc7065 da8076",
                    "a44d44 e25243 e25243 e1594c cec2c0 cccccc cccccc cccccc",
                    "d35143 e25243 e25243 d0b4b1 cccccc cccccc cccccc cccccc",
                    "e25243 e25243 e25243 cccbcb cccccc cccccc cccccc cccccc",
                } },
                { "disabled, top left", SpinContext::Plain, false, QStyle::State_None, 217, 2, 3, {
                    "444444 444444 444444 444444 444444 444444",
                    "444444 444444 484848 545454 555555 555555",
                    "444444 4b4b4b 646464 666666 666666 666666",
                    "444444 5f5f5f 666666 666666 666666 666666",
                    "444444 666666 666666 666666 666666 666666",
                } },
                { "vector element (no margin at the left, square corners there), top left", SpinContext::Vector, false, enabled, 204, 0, 3, {
                    "444444 444444 444444 444444 444444 444444",
                    "888888 888888 888888 888888 888888 888888",
                    "cccccc cccccc cccccc cccccc cccccc cccccc",
                    "cccccc cccccc cccccc cccccc cccccc cccccc",
                } },
            };
            for (const NativeSpinBlock& block : blocks)
            {
                Twins<QWidget> twins(*this, [&](QWidget& root)
                {
                    QAbstractSpinBox* spinBox = buildSpinBox(root, block.m_context, false);
                    spinBox->setGeometry(0, 8, block.m_boxWidth, 23);
                    presetProperties(spinBox, block.m_error, {});
                }, QString::fromUtf8(SpinBoxRules), QSize(300, 40));
                const QImage image = drawSpinBox(twins.flatChild<QAbstractSpinBox>("spin"), block.m_state, 1.5, 1);
                expectBlock(image, { block.m_what, block.m_x, block.m_y, block.m_rows });
            }
        }

        TEST_F(ControlElementTests, SpinBox_FocusFaceIsThePixelsOfTheNativeColorPickerCapture)
        {
            ASSERT_TRUE(StyleManager::stylesheetsDisabled());

            // n2 Gallery 07_color-picker__dialog0.png: the red spin box of the RGBA row has the focus (ColorRGBAEdit's first, 52 x 23 at
            // x 39, y 423: the device origin 58.5, 634.5), so the focus face of SpinBox.qss:42-47 over the margin 1, border 1, radius 3 of
            // :22-31, with no buttons (ColorPicker.qss:74-81). Here the box is 1 logical pixel (1.5 device pixels) from the image's top and
            // left, so the capture's device column 58 and row 634 are the image's column 1 and row 1. The text is not part of the blocks.
            Twins<QWidget> twins(*this, [](QWidget& root)
            {
                QAbstractSpinBox* spinBox = buildSpinBox(root, SpinContext::ColorPicker, true);
                spinBox->setGeometry(0, 8, 52, 23);
                presetProperties(spinBox, false, { "SpinBoxFocused" });
            }, QString::fromUtf8(SpinBoxRules), QSize(100, 40));
            const QStyle::State focused = QStyle::State_Enabled | QStyle::State_HasFocus;
            const QAbstractSpinBox* flatBox = twins.flatChild<QAbstractSpinBox>("spin");
            const QImage image = drawSpinBox(flatBox, focused, 1.5, 1, true, 1);
            expectBlock(image, { "top left", 1, 1, {
                "444444 444444 444444 444444 444444",
                "444444 444444 444444 444444 444444",
                "444444 444444 444444 494d50 69869a",
                "444444 444444 494d50 90ccf7 94d2ff",
                "444444 444444 698498 94d2ff c6e7ff",
                "444444 444444 85b6da b5e0ff ffffff",
                "444444 444444 94d1fe bbe3ff ffffff",
            } });
            expectBlock(image, { "bottom right", 71, 32, {
                "ffffff ffffff feffff eaf6ff 9cd5fe 94d2ff 55626b 444444 444444",
                "94d2ff 94d2ff 94d2ff 94d2ff 8fc8f2 7092ab 444444 444444 444444",
                "6c8ba1 7194ad 749ab5 5b6d7a 494c4e 444444 444444 444444 444444",
                "444444 444444 444444 444444 444444 444444 444444 444444 444444",
            } });

            // The legacy twin draws the same, in the state the sheet reads (:focus)
            const QAbstractSpinBox* legacyBox = twins.legacyChild<QAbstractSpinBox>("spin");
            EXPECT_EQ(differing(drawSpinBox(legacyBox, focused, 1.5, 1, true, 1), image), 0);
        }

        TEST_F(ControlElementTests, SpinBox_SizeHintsAreTheContentsPlusTheUpButtonAndTheBoxOfTheRule)
        {
            ASSERT_TRUE(StyleManager::stylesheetsDisabled());

            // The sheet style's size (qstylesheetstyle.cpp:5309-5330): the base style's, 6px of button (none in a color picker), the margins
            // and the padding of 12 on the right, so 22 more than the flat box asked for, 20 in a vector element, 16 in a color picker
            // (n2 Gallery 23_spin-box: the hints of the boxes were 80 and 66 where the flat ones were 58 and 44, 78 and 58 in a vector element)
            const struct { const char* m_name; SpinContext m_context; int m_extra; } places[] = {
                { "plain", SpinContext::Plain, 22 }, { "vector element", SpinContext::Vector, 20 }, { "color picker", SpinContext::ColorPicker, 16 },
            };
            for (const auto& place : places)
            {
                for (const bool doubleBox : { false, true })
                {
                    Twins<QWidget> twins(*this, [&](QWidget& root) { presetProperties(buildSpinBox(root, place.m_context, doubleBox), false, {}); },
                        QString::fromUtf8(SpinBoxRules), QSize(200, 40));
                    const QAbstractSpinBox* legacyBox = twins.legacyChild<QAbstractSpinBox>("spin");
                    const QAbstractSpinBox* flatBox = twins.flatChild<QAbstractSpinBox>("spin");
                    const QString what = QStringLiteral("%1, %2").arg(QString::fromLatin1(place.m_name), doubleBox ? QStringLiteral("double") : QStringLiteral("int"));

                    EXPECT_EQ(flatBox->sizeHint(), legacyBox->sizeHint()) << qPrintable(what);
                    EXPECT_EQ(flatBox->minimumSizeHint(), legacyBox->minimumSizeHint()) << qPrintable(what);
                    EXPECT_EQ(flatBox->sizeHint().height(), 23) << qPrintable(what);
                    EXPECT_EQ(SpinBoxElement::extraSize(flatBox), QSize(place.m_extra, 4)) << qPrintable(what);
                }
            }
        }

        // ----------------------------------------------------------------------------------
        // T-LED: the frame of a line edit (LineEdit.qss:11-35 over BaseStyleSheet.qss:68-84; Native Asset Browser search fields:
        // border rows #e9e9e9 #e9e9e9 #dadada at the ratio of the captures, where the flat frame had three solid rows)
        // ----------------------------------------------------------------------------------

        const char* const LineEditRules =
            "QLineEdit { background-color: #CCCCCC; color: black; border-radius: 2px; border-width: 0px; border-color: #e9e9e9; }"
            "QLineEdit { height: 16px; padding-left: 0px; border-width: 2px; border-style: solid; padding-bottom: 1px; padding-top: -1px; }"
            "QLineEdit:disabled { background-color: #666666; color: #999999; }";

        //! PE_PanelLineEdit as QLineEdit::paintEvent asks the widget's style for it, in a rect of its own clipped as a widget paints,
        //! `offset` logical pixels from the origin of the image, over the dark behind a field
        QImage drawLineEditPanel(const QLineEdit* lineEdit, QStyle::State state, const QSize& size, qreal ratio, const QPoint& offset = QPoint(0, 0))
        {
            QStyleOptionFrame option;
            option.initFrom(lineEdit);
            option.state = state;
            option.rect = QRect(QPoint(0, 0), size);
            option.lineWidth = 1;
            QImage image = newImage(size + QSize(2 * offset.x() + 2, 2 * offset.y() + 2), ratio, QColor(0x44, 0x44, 0x44));
            QPainter painter(&image);
            painter.translate(offset);
            painter.setClipRect(option.rect);
            lineEdit->style()->drawPrimitive(QStyle::PE_PanelLineEdit, &option, &painter, lineEdit);
            return image;
        }

        TEST_F(ControlElementTests, LineEditFrame_FillBorderAndCornersBlendAtTheEdgesAsTheSheetDrawsThem)
        {
            ASSERT_TRUE(StyleManager::stylesheetsDisabled());

            Twins<QWidget> twins(*this, [](QWidget& root)
            {
                QLineEdit* lineEdit = new QLineEdit(&root);
                lineEdit->setObjectName(QStringLiteral("edit"));
                lineEdit->setGeometry(0, 0, 200, 20);
            }, QString::fromUtf8(LineEditRules), QSize(200, 40));
            const QLineEdit* legacyEdit = twins.legacyChild<QLineEdit>("edit");
            const QLineEdit* flatEdit = twins.flatChild<QLineEdit>("edit");

            for (const QSize size : { QSize(120, 20), QSize(428, 20), QSize(99, 23), QSize(31, 19), QSize(200, 29) })
            {
                for (const QStyle::State state : { QStyle::State_Enabled, QStyle::State_None })
                {
                    for (const qreal ratio : { 1.0, 1.5 })
                    {
                        EXPECT_EQ(differing(drawLineEditPanel(legacyEdit, state, size, ratio), drawLineEditPanel(flatEdit, state, size, ratio)), 0)
                            << size.width() << "x" << size.height() << ", enabled " << int(state.testFlag(QStyle::State_Enabled)) << ", ratio " << ratio;
                    }
                }
            }

            // On whole device pixels (a field at the origin, 1.5 device pixels to the logical one) the 2px border is three solid rows
            const QImage image = drawLineEditPanel(flatEdit, QStyle::State_Enabled, QSize(120, 20), 1.5);
            for (const int row : { 0, 1, 2 })
            {
                EXPECT_EQ(image.pixelColor(60, row).rgb(), QColor(0xE9, 0xE9, 0xE9).rgb()) << row;
            }
            EXPECT_EQ(image.pixelColor(60, 15).rgb(), QColor(0xCC, 0xCC, 0xCC).rgb());
        }

        TEST_F(ControlElementTests, LineEditFrame_AtAHalfDevicePixelTheEdgesAreThePixelsOfTheLuaIdeCapture)
        {
            ASSERT_TRUE(StyleManager::stylesheetsDisabled());

            // The fields of the captures sit at an odd logical y, so at the ratio 1.5 their top is at a half device pixel. n2 LuaIDE
            // textSearch (Watched Variables dock): 428 x 20 at x 1, y 323 (the device origin 1.5, 484.5). Its device rows 483 to 516 are
            // the rows 0 to 33 of the image here, an offset of 1 logical pixel from the image's origin in both directions:
            // the top edge #444444 (the parent, outside the widget's clip) #e9e9e9 #e9e9e9 #dadada (half border, half field), the
            // bottom edge #dadada #e9e9e9 #e9e9e9 and a half of #e9e9e9 over the parent. The flat frame drew #e9e9e9 for all three rows
            // at the top and #cccccc for the first at the bottom.
            Twins<QWidget> twins(*this, [](QWidget& root)
            {
                QLineEdit* lineEdit = new QLineEdit(&root);
                lineEdit->setObjectName(QStringLiteral("edit"));
                lineEdit->setGeometry(0, 0, 428, 20);
            }, QString::fromUtf8(LineEditRules), QSize(430, 22));
            const QLineEdit* legacyEdit = twins.legacyChild<QLineEdit>("edit");
            const QLineEdit* flatEdit = twins.flatChild<QLineEdit>("edit");

            for (const QPoint offset : { QPoint(1, 1), QPoint(1, 0), QPoint(0, 1) })
            {
                for (const qreal ratio : { 1.0, 1.5 })
                {
                    EXPECT_EQ(differing(drawLineEditPanel(legacyEdit, QStyle::State_Enabled, QSize(428, 20), ratio, offset),
                                  drawLineEditPanel(flatEdit, QStyle::State_Enabled, QSize(428, 20), ratio, offset)), 0)
                        << "offset " << offset.x() << "," << offset.y() << " ratio " << ratio;
                }
            }

            const QImage image = drawLineEditPanel(flatEdit, QStyle::State_Enabled, QSize(428, 20), 1.5, QPoint(1, 1));
            expectBlock(image, { "top edge, column 300", 300, 0, { "444444", "444444", "e9e9e9", "e9e9e9", "dadada", "cccccc", "cccccc" } });
            expectBlock(image, { "bottom edge, column 300", 300, 26, { "cccccc", "cccccc", "dadada", "e9e9e9", "e9e9e9", "969696", "444444", "444444" } });
            expectBlock(image, { "top left corner", 2, 2, {
                "d8d8d8 e9e9e9 e9e9e9 e9e9e9 e9e9e9 e9e9e9",
                "e9e9e9 e9e9e9 e9e9e9 e9e9e9 e9e9e9 e9e9e9",
                "e9e9e9 e9e9e9 e7e7e7 e2e2e2 dadada dadada",
                "e9e9e9 e9e9e9 e2e2e2 cccccc cccccc cccccc",
                "e9e9e9 e9e9e9 dadada cccccc cccccc cccccc",
                "e9e9e9 e9e9e9 dadada cccccc cccccc cccccc",
            } });
            expectBlock(image, { "bottom right corner", 636, 25, {
                "cccccc cccccc cccccc cccccc dadada e9e9e9 e9e9e9 969696 444444",
                "cccccc cccccc cccccc cccccc dadada e9e9e9 e9e9e9 969696 444444",
                "cccccc cccccc cccccc cccccc e2e2e2 e9e9e9 e9e9e9 c0c0c0 444444",
                "dadada dadada dadada e2e2e2 e7e7e7 e9e9e9 e9e9e9 ababab 444444",
                "e9e9e9 e9e9e9 e9e9e9 e9e9e9 e9e9e9 e9e9e9 e9e9e9 a5a5a5 444444",
                "e9e9e9 e9e9e9 e9e9e9 e9e9e9 e9e9e9 e9e9e9 d8d8d8 4a4a4a 444444",
                "969696 969696 969696 c0c0c0 ababab a5a5a5 4a4a4a 444444 444444",
            } });
        }

    } // namespace ControlElementTestsInternal
} // namespace AzQtComponents
