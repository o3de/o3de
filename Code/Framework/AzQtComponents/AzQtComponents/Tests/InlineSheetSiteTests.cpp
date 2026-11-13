/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzQtComponents/Tests/SheetTwins.h>
#include <AzQtComponents/Tests/SiteTwins.h>

#include <AzCore/IO/Path/Path.h>
#include <AzQtComponents/Components/FilteredSearchWidget.h>
#include <AzQtComponents/Components/StyleManager.h>
#include <AzQtComponents/Components/StyledSpinBox.h>
#include <AzQtComponents/Components/Widgets/ScrollBar.h>
#include <AzQtComponents/Components/Widgets/SegmentBar.h>

#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPixmap>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QWidget>

namespace AzQtComponents
{
    // ----------------------------------------------------------------------------------
    // Sites of AzQtComponents' own code that set a sheet inline:
    //   - C011, C012: the popup slider of a styled spin box (StyledSpinBox.cpp:385, :390), a bare rule with a border image
    //     whose pictures are in no resource: the sheet draws nothing the slider does not draw without it;
    //   - C023-C025, C030: the purposeful samples of the Gallery (D.10), whose sheets stay live in the flattened GUI as in the
    //     legacy one: the sample renders in both as its sheet and the widgets under it say;
    //   - C026-C029: the sheet of the Gallery's StyleSheetPage, set through StyleManager::setSampleStyleSheet in both GUIs.
    // The legacy twin is Qt's own style sheet style over AzQtComponents::Style with the real application sheet; the flat twin is the
    // flattened Style (SheetTwins.h).
    // ----------------------------------------------------------------------------------

    namespace InlineSheetSiteTestsInternal
    {
        using namespace SheetTwins;

        class InlineSheetSiteTests
            : public ::UnitTest::LeakDetectionFixture
        {
        protected:
            Context m_context;
        };

        //! A StyleManager of its own, as the application makes one, for the calls that need the instance. No sheet is set on the
        //! application: the instance is made at the flat rung, and the calls run at the rung of the test.
        class SampleSheetFixture
            : public InlineSheetSiteTests
        {
        protected:
            SampleSheetFixture()
                : m_styleManager(nullptr)
            {
                RungScope flat(Rung::Flat);
                m_styleManager.initialize(qobject_cast<QApplication*>(QCoreApplication::instance()), AZ::IO::PathView());
                StyleManager::addSearchPaths(
                    QStringLiteral("sampletests"), QStringLiteral("Tests/NoSuchFolder"), QStringLiteral(":/StyleSheetTests1"),
                    AZ::IO::PathView());
            }

            StyleManager m_styleManager;
        };

        // ----------------------------------------------------------------------------------
        // Helpers
        // ----------------------------------------------------------------------------------

        //! The legacy twin of a row, read idle and in each state
        std::vector<Reading> readLegacyTwin(const Row& row, Context& context)
        {
            RungScope rung(row.m_legacyRung);
            const Twin twin(row, context, true);
            return readStates(row, *twin.widget(), context, true, nullptr);
        }

        //! The differences between two readings of the same widget, state by state: what a sheet decides in the legacy GUI
        Result compareLegacyReadings(const Row& row, const std::vector<Reading>& with, const std::vector<Reading>& without)
        {
            Result result;
            EXPECT_EQ(with.size(), without.size());
            for (size_t i = 0; i < with.size() && i < without.size(); ++i)
            {
                compareReadings(row, with[i], without[i], result);
            }
            return result;
        }

        //! A purposeful sample (D.10) sets its sheet in both GUIs. The twins differ in what is around the sheet: the legacy one has the
        //! application's rules under it, the flat one the flattened Style. The factory sets the sheet of the sample; the flat twin keeps
        //! it.
        Result runSample(const Row& row, Context& context)
        {
            Result result;
            Context::registerOnce(row.m_register);

            std::vector<Reading> legacy = readLegacyTwin(row, context);

            std::vector<Reading> flat;
            {
                RungScope rung(Rung::Flat);
                const Twin twin(row, context, false);
                flat = readStates(row, *twin.widget(), context, false, &legacy);
            }

            for (size_t i = 0; i < legacy.size(); ++i)
            {
                compareReadings(row, legacy[i], flat[i], result);
            }
            if (result.m_compared == 0)
            {
                result.m_differences.push_back(std::string(row.m_site) + ": the row compares nothing");
            }
            return result;
        }

        //! The pixels that differ between the legacy and the flat twin of a row inside its picture less `outside` (logical pixels on each
        //! side), at the three ratios, in the idle state. A sample whose widgets are drawn by code the sample's sheet does not decide (the
        //! scroll bars, the margin of a label) is compared where the sheet decides. The legacy twin has drawn on the picture.
        int differingInside(const Row& row, Context& context, const QMargins& outside)
        {
            Row pixels = row;
            pixels.m_compare = Compare::Pixels;

            const std::vector<Reading> legacy = readLegacyTwin(pixels, context);
            std::vector<Reading> flat;
            {
                RungScope rung(Rung::Flat);
                const Twin twin(pixels, context, false);
                flat = readStates(pixels, *twin.widget(), context, false, &legacy);
            }

            int count = 0;
            for (size_t i = 0; i < legacy[0].m_pictures.size(); ++i)
            {
                const QImage& a = legacy[0].m_pictures[i].m_image;
                const QImage& b = flat[0].m_pictures[i].m_image;
                const qreal ratio = a.devicePixelRatio();
                const QRect inside(
                    QPoint(qRound(outside.left() * ratio), qRound(outside.top() * ratio)),
                    QSize(a.width() - qRound((outside.left() + outside.right()) * ratio),
                        a.height() - qRound((outside.top() + outside.bottom()) * ratio)));
                EXPECT_TRUE(FieldTwins::isDrawnOn(a.copy(inside))) << "the legacy twin drew nothing in the sample";
                count += FieldTwins::differing(a.copy(inside), b.copy(inside));
            }
            return count;
        }

        // ----------------------------------------------------------------------------------
        // C011, C012: the popup slider of a styled spin box
        // ----------------------------------------------------------------------------------

        // StyledSpinBox.cpp:385 and :390, the sheets the slider is given when it would overflow the window on the right, and when not
        const char* const RightSliderSheet = "background: transparent; border-image: url(:/stylesheet/img/styledspinbox-bg-right.png);";
        const char* const LeftSliderSheet = "background: transparent; border-image: url(:/stylesheet/img/styledspinbox-bg-left.png);";

        //! The slider, as StyledDoubleSpinBox::initSlider makes it (a frameless window), at the size prepareSlider gives it
        QWidget* makeSlider(QWidget* parent)
        {
            auto* slider = new StyledSliderPrivate(parent);
            slider->setWindowFlags(Qt::WindowFlags(Qt::Window) | Qt::WindowFlags(Qt::FramelessWindowHint));
            slider->setRange(0, 100);
            slider->setValue(40);
            slider->setAttribute(Qt::WA_DontShowOnScreen);
            slider->show();
            return slider;
        }

        Row sliderRow(const char* site, const char* cite, const char* sheet)
        {
            return Row{
                .m_site = site,
                .m_make = &makeSlider,
                .m_sheets = { { cite, SetOn::Widget, sheet } },
                .m_realApplicationSheet = true,
                .m_states = { States::hover(), States::disabled() },
                .m_compare = Compare::Geometry | Compare::Palette | Compare::Pixels,
                .m_roles = { QPalette::WindowText, QPalette::Text, QPalette::ButtonText },
                .m_size = QSize(192, 35),
                .m_base = Base::AzQtComponentsStyle,
                .m_mayDecideNothing = true,         // the sheet's border images load nothing, so it decides nothing (the tests below)
            };
        }

        TEST_F(InlineSheetSiteTests, Slider_TheBorderImagesOfTheSheetAreInNoResource)
        {
            // The sheet names two pictures that no .qrc holds: a border image that loads nothing is dropped (QRenderRule::fixupBorder)
            EXPECT_TRUE(QPixmap(QStringLiteral(":/stylesheet/img/styledspinbox-bg-right.png")).isNull());
            EXPECT_TRUE(QPixmap(QStringLiteral(":/stylesheet/img/styledspinbox-bg-left.png")).isNull());
        }

        TEST_F(InlineSheetSiteTests, Slider_TheSheetChangesNothingTheLegacySliderDraws)
        {
            for (const Row& row : { sliderRow("C011 the slider that overflows to the right", "StyledSpinBox.cpp:385", RightSliderSheet),
                                    sliderRow("C012 the slider that fits on the right", "StyledSpinBox.cpp:390", LeftSliderSheet) })
            {
                SCOPED_TRACE(row.m_site);
                Row without = row;
                without.m_sheets.clear();

                const std::vector<Reading> sheeted = readLegacyTwin(row, m_context);
                const std::vector<Reading> bare = readLegacyTwin(without, m_context);

                const Result result = compareLegacyReadings(row, sheeted, bare);
                EXPECT_TRUE(result.ok()) << result.text();
                EXPECT_GT(result.m_compared, 0);
            }
        }

        TEST_F(InlineSheetSiteTests, Slider_TheFlatSliderHasTheBoxOfTheSliderOfTheSheet)
        {
            // What a sheet with a background and a border image could decide about the slider is its box: the flat twin has no sheet, and
            // the same size, hints, margins and contents rect. (The pixels are not compared: the legacy slider is drawn over the
            // transparent palette of the application's universal rule and the flat one over the base palette, with the sheet or without.)
            Row right = sliderRow("C011 the slider that overflows to the right", "StyledSpinBox.cpp:385", RightSliderSheet);
            Row left = sliderRow("C012 the slider that fits on the right", "StyledSpinBox.cpp:390", LeftSliderSheet);
            right.m_compare = Compare::Geometry;
            left.m_compare = Compare::Geometry;
            EXPECT_TRUE(check(right, m_context));
            EXPECT_TRUE(check(left, m_context));
        }

        //! The sheet the real slider of a spin box has after displaySlider, with the spin box `x` pixels from the left of a window 400
        //! wide: it overflows the window on the right when fewer than 192 pixels are left
        QString sliderSheetOfSpinBoxAt(int x)
        {
            QWidget window;
            window.resize(400, 200);
            auto* spinBox = new StyledDoubleSpinBox(&window);
            spinBox->setGeometry(x, 0, 80, 24);
            window.setAttribute(Qt::WA_DontShowOnScreen);
            window.show();
            States::setActiveWindowNow(&window);

            spinBox->displaySlider();

            auto* slider = spinBox->findChild<StyledSliderPrivate*>();
            QString sheet = slider ? slider->styleSheet() : QStringLiteral("(no slider)");
            if (slider && slider->style()->inherits("QStyleSheetStyle") != !sheet.isEmpty())
            {
                sheet += QStringLiteral(" (and the style sheet style does not follow it)");
            }
            States::setActiveWindowNow(nullptr);
            return sheet;
        }

        TEST_F(InlineSheetSiteTests, Slider_TheLegacyGuiSetsTheSheetsAsBeforeAndTheFlatOneSetsNone)
        {
            {
                RungScope rung(Rung::Qss);
                EXPECT_EQ(sliderSheetOfSpinBoxAt(0), QString::fromLatin1(LeftSliderSheet));
                EXPECT_EQ(sliderSheetOfSpinBoxAt(300), QString::fromLatin1(RightSliderSheet));
            }
            {
                RungScope rung(Rung::Flat);
                EXPECT_EQ(sliderSheetOfSpinBoxAt(0), QString());
                EXPECT_EQ(sliderSheetOfSpinBoxAt(300), QString());
            }
        }

        TEST_F(InlineSheetSiteTests, Slider_TheCensusListsTheSheetInTheLegacyGuiAndNoneInTheFlatOne)
        {
            const auto build = []()
            {
                QWidget window;
                window.resize(400, 200);
                auto* spinBox = new StyledDoubleSpinBox(&window);
                spinBox->setGeometry(0, 0, 80, 24);
                window.setAttribute(Qt::WA_DontShowOnScreen);
                window.show();
                States::setActiveWindowNow(&window);
                spinBox->displaySlider();
                window.ensurePolished();
                States::setActiveWindowNow(nullptr);
            };

            const QStringList legacy = SiteTwins::sheetLinesAt(Rung::Qss, build);
            ASSERT_EQ(legacy.size(), 1) << legacy.join("\n").toStdString();
            EXPECT_TRUE(SiteTwins::namesCarrier(legacy[0], QStringLiteral("AzQtComponents::StyledSliderPrivate")))
                << legacy[0].toStdString();

            const QStringList flat = SiteTwins::sheetLinesAt(Rung::Flat, build);
            EXPECT_TRUE(flat.isEmpty()) << flat.join("\n").toStdString();
        }

        // ----------------------------------------------------------------------------------
        // C023-C025: the dark scroll areas of the Gallery's ScrollBarPage (D.10)
        // ----------------------------------------------------------------------------------

        // ScrollBarPage.cpp:25-27, the sheet of each of the three dark scroll areas
        const char* const ScrollAreaSampleSheet = "QScrollArea { background: #FFFFFF; } QLabel { color: #000000; } ";

        //! One of the three scroll areas as the page makes them (ScrollBarPage.ui, ScrollBarPage.cpp:25-37): a label in contents 500 by
        //! 500 in an area at most 100 wide, with the sample's sheet and the dark scroll bars
        QWidget* makeScrollSample(QWidget* parent, ScrollBar::ScrollBarMode mode, bool setMode)
        {
            auto* area = new QScrollArea(parent);
            area->setMaximumWidth(100);
            area->setWidgetResizable(false);
            auto* contents = new QWidget();
            contents->resize(500, 500);
            auto* label = new QLabel(QStringLiteral("Default"), contents);
            label->setGeometry(0, 0, 44, 16);
            area->setWidget(contents);

            area->setStyleSheet(QString::fromLatin1(ScrollAreaSampleSheet));
            ScrollBar::applyDarkStyle(area);
            if (setMode)
            {
                ScrollBar::setDisplayMode(area, mode);
            }
            return area;
        }

        Row scrollSampleRow(const char* site, ScrollBar::ScrollBarMode mode, bool setMode)
        {
            return Row{
                .m_site = site,
                .m_make = [mode, setMode](QWidget* parent) -> QWidget* { return makeScrollSample(parent, mode, setMode); },
                .m_realApplicationSheet = true,
                .m_compare = Compare::Geometry | Compare::Palette,
                .m_roles = { QPalette::WindowText, QPalette::Text, QPalette::ButtonText },
                .m_size = QSize(100, 120),
                .m_base = Base::AzQtComponentsStyle,
            };
        }

        //! The lanes of the scroll bars of a 100 by 120 area, outside which the sample's sheet decides what is drawn: the sheet gives the
        //! area, its viewport and the label their white and black, and the scroll bars are ScrollBar.qss's and the flattened Style's own
        constexpr QMargins ScrollBarLanes(0, 0, 10, 10);

        TEST_F(SampleSheetFixture, Sample_TheScrollAreasOfTheScrollBarPageRenderAsTheirSheetsSay)
        {
            const Row rows[] = {
                scrollSampleRow("C023 the default scroll area", ScrollBar::ScrollBarMode::AlwaysShow, false),
                scrollSampleRow("C024 the always-show scroll area", ScrollBar::ScrollBarMode::AlwaysShow, true),
                scrollSampleRow("C025 the show-on-hover scroll area", ScrollBar::ScrollBarMode::ShowOnHover, true),
            };
            for (const Row& row : rows)
            {
                SCOPED_TRACE(row.m_site);
                const Result result = runSample(row, m_context);
                EXPECT_TRUE(result.ok()) << result.text();
                EXPECT_EQ(differingInside(row, m_context, ScrollBarLanes), 0);
            }
        }

        TEST_F(SampleSheetFixture, Sample_TheScrollAreasKeepTheirSheetsInTheFlatGui)
        {
            RungScope rung(Rung::Flat);
            QWidget root;
            QWidget* area = makeScrollSample(&root, ScrollBar::ScrollBarMode::AlwaysShow, false);

            EXPECT_EQ(area->styleSheet(), QString::fromLatin1(ScrollAreaSampleSheet));
        }

        // ----------------------------------------------------------------------------------
        // C030: the example widget of the Gallery's StyleSheetPage (D.10)
        // ----------------------------------------------------------------------------------

        // ExampleWidget.qss:15-25, the three rules of the sample. The pictures are the Gallery's own (Gallery.qrc), which the tests do not
        // link: the twin swaps them for three of AzQtComponents' resources.
        const char* const ExampleWidgetSheet = R"qss(
Example--Widget #icon {
    qproperty-pixmap: url(:/Gallery/img/complex.png);
}

Example--Widget[drawSimple="true"] #icon {
    qproperty-pixmap: url(:/Gallery/img/simple.png);
}

Example--Widget[drawSimple="false"][tearEnabled="true"] #icon {
    qproperty-pixmap: url(:/Gallery/img/intermediate.png);
}
)qss";

        //! The sheet of the sample with the class selector of the stand-in (the Gallery's class Example::Widget cannot be linked) and
        //! the pictures of the tests
        QString exampleWidgetSheet()
        {
            return QString::fromLatin1(ExampleWidgetSheet)
                .replace(QStringLiteral("Example--Widget"), QStringLiteral("QWidget#widget"))
                .replace(QStringLiteral(":/Gallery/img/complex.png"), QStringLiteral(":/stylesheet/img/question.png"))
                .replace(QStringLiteral(":/Gallery/img/simple.png"), QStringLiteral(":/stylesheet/img/lineedit-invalid.png"))
                .replace(QStringLiteral(":/Gallery/img/intermediate.png"), QStringLiteral(":/stylesheet/img/lineedit-clear.png"));
        }

        //! The widget of the page: a label named icon in a plain widget, which has the two properties of Example::Widget
        QWidget* makeExampleWidget(QWidget* parent)
        {
            auto* widget = new QWidget(parent);
            widget->setObjectName(QStringLiteral("widget"));
            widget->setProperty("drawSimple", false);
            widget->setProperty("tearEnabled", false);
            auto* layout = new QHBoxLayout(widget);
            auto* icon = new QLabel(widget);
            icon->setObjectName(QStringLiteral("icon"));
            layout->addWidget(icon);
            widget->setStyleSheet(exampleWidgetSheet());
            return widget;
        }

        TEST_F(SampleSheetFixture, Sample_TheExampleWidgetSwapsItsPictureInBothGuis)
        {
            const Row row{
                .m_site = "C030 the example widget",
                .m_make = &makeExampleWidget,
                .m_realApplicationSheet = true,
                .m_states = { States::property("drawSimple", true), States::property("tearEnabled", true) },
                .m_compare = Compare::Geometry | Compare::Pixels,
                .m_size = QSize(80, 60),
                .m_base = Base::AzQtComponentsStyle,
            };

            const Result result = runSample(row, m_context);
            EXPECT_TRUE(result.ok()) << result.text();

            // The pictures are three: each state of the widget showed another one
            const std::vector<Reading> legacy = readLegacyTwin(row, m_context);
            ASSERT_EQ(legacy.size(), 3u);
            EXPECT_GT(FieldTwins::differing(legacy[0].m_pictures[0].m_image, legacy[1].m_pictures[0].m_image), 0);
            EXPECT_GT(FieldTwins::differing(legacy[0].m_pictures[0].m_image, legacy[2].m_pictures[0].m_image), 0);
            EXPECT_GT(FieldTwins::differing(legacy[1].m_pictures[0].m_image, legacy[2].m_pictures[0].m_image), 0);
        }

        // ----------------------------------------------------------------------------------
        // C026-C029: the sheet of the Gallery's StyleSheetPage (D.10)
        // ----------------------------------------------------------------------------------

        // The sheet of the page as StyleManager::setSampleStyleSheet loads it: Tests/qrc1/pagesheet.qss is StyleSheetPage.qss:12-20 with
        // its two @import lines (:9-10) replaced by the files they name, StyleSheetView.qss:9-12 and StyleSheetLabel.qss:9-11. Both
        // twins set it through the real call, as the page does.
        const char* const PageSheetFile = "sampletests:pagesheet.qss";

        //! The widget of the page that carries the sheet (StyleSheetPage.ui:88) with the widgets whose looks the sheet's own rules
        //! decide: a label and a line edit (StyleSheetPage.cpp:70-73). With `controls` also the segment bar and the filtered search
        //! widget that the page puts in it (StyleSheetPage.cpp:25-37), which the flattened GUI draws with elements of their own.
        QWidget* makeStyleSheetPageWidget(QWidget* parent, bool controls)
        {
            auto* widget = new QWidget(parent);
            widget->setObjectName(QStringLiteral("exampleWidget"));
            auto* layout = new QVBoxLayout(widget);
            layout->setContentsMargins(0, 0, 0, 0);

            auto* label = new QLabel(QStringLiteral("Styled using a prefix"), widget);
            label->setObjectName(QStringLiteral("exampleLabel"));
            layout->addWidget(label);

            auto* lineEdit = new QLineEdit(QStringLiteral("A line edit"), widget);
            lineEdit->setObjectName(QStringLiteral("lineEdit"));
            layout->addWidget(lineEdit);

            if (controls)
            {
                auto* segmentBar = new SegmentBar(widget);
                segmentBar->setObjectName(QStringLiteral("segmentBar"));
                for (int i = 0; i < 4; ++i)
                {
                    segmentBar->addTab(QStringLiteral("Segment Bar %1").arg(i));
                }
                layout->addWidget(segmentBar);

                auto* search = new FilteredSearchWidget(widget);
                search->setObjectName(QStringLiteral("filteredSearchWidget"));
                for (const char* fruit : { "Apple", "Orange", "Pear", "Banana" })
                {
                    search->AddTypeFilter(QStringLiteral("Fruit"), QString::fromLatin1(fruit));
                }
                search->SetFilterState(0, true);
                search->SetFilterState(1, true);
                layout->addWidget(search);
            }
            layout->addStretch();

            EXPECT_TRUE(StyleManager::setSampleStyleSheet(widget, QString::fromLatin1(PageSheetFile)));
            return widget;
        }

        //! The labels of the page that carry the same sheet on their own (StyleSheetPage.ui:53, :60, :67)
        QWidget* makeStyleSheetPageLabel(QWidget* parent)
        {
            auto* label = new QLabel(QStringLiteral("Styled using qrc"), parent);
            label->setObjectName(QStringLiteral("qrcLabel"));
            EXPECT_TRUE(StyleManager::setSampleStyleSheet(label, QString::fromLatin1(PageSheetFile)));
            return label;
        }

        TEST_F(SampleSheetFixture, Sample_TheStyleSheetPageRendersAsItsSheetSaysInBothGuis)
        {
            const Row page{
                .m_site = "C026 the widget of the StyleSheetPage",
                .m_make = [](QWidget* parent) { return makeStyleSheetPageWidget(parent, false); },
                .m_realApplicationSheet = true,
                .m_compare = Compare::Geometry | Compare::Palette | Compare::Pixels,
                .m_roles = { QPalette::WindowText, QPalette::Text, QPalette::ButtonText },
                .m_size = QSize(300, 80),
                .m_base = Base::AzQtComponentsStyle,
            };
            const Result result = runSample(page, m_context);
            EXPECT_TRUE(result.ok()) << result.text();
        }

        TEST_F(SampleSheetFixture, Sample_ALabelOfTheStyleSheetPageRendersAsItsSheetSaysInBothGuis)
        {
            // The whole label, its fill included: the sheet fills it inside the 1px above and below that Text.qss:93-94 gives every label
            // in the legacy GUI, and setSampleStyleSheet sets that margin in the flattened one
            const Row label{
                .m_site = "C027-C029 a label of the StyleSheetPage",
                .m_make = &makeStyleSheetPageLabel,
                .m_realApplicationSheet = true,
                .m_compare = Compare::Geometry | Compare::Fonts | Compare::Palette | Compare::Pixels,
                .m_roles = { QPalette::WindowText, QPalette::Text, QPalette::ButtonText },
                .m_size = QSize(160, 24),
                .m_base = Base::AzQtComponentsStyle,
            };

            const Result result = runSample(label, m_context);
            EXPECT_TRUE(result.ok()) << result.text();
        }

        TEST_F(SampleSheetFixture, Sample_TheSegmentBarAndTheSearchWidgetOfThePageBuildAndLayOutUnderTheSheetInBothGuis)
        {
            // The two classes of the page that the flattened GUI draws with elements and flat-gated code, under the live sheet of the
            // page: built and laid out in both GUIs, and compared by their boxes and hints
            const auto controlRow = [](const char* site, const char* objectName)
            {
                return Row{
                    .m_site = site,
                    .m_make = [objectName](QWidget* parent) -> QWidget*
                    { return makeStyleSheetPageWidget(parent, true)->findChild<QWidget*>(QString::fromLatin1(objectName)); },
                    .m_realApplicationSheet = true,
                    .m_compare = Compare::Geometry,
                    .m_base = Base::AzQtComponentsStyle,
                };
            };

            const Row rows[] = {
                controlRow("C026 the segment bar under the sheet of the page", "segmentBar"),
                controlRow("C026 the filtered search widget under the sheet of the page", "filteredSearchWidget"),
            };
            for (const Row& row : rows)
            {
                SCOPED_TRACE(row.m_site);
                const Result result = runSample(row, m_context);
                EXPECT_TRUE(result.ok()) << result.text();
            }
        }

        // ----------------------------------------------------------------------------------
        // C026-C029: StyleManager::setSampleStyleSheet
        // ----------------------------------------------------------------------------------

        TEST_F(SampleSheetFixture, SetSampleStyleSheet_SetsTheFileOnTheWidgetInBothGuis)
        {
            for (const Rung rung : { Rung::Qss, Rung::Flat })
            {
                RungScope scope(rung);
                QLabel sample;
                sample.setObjectName(QStringLiteral("sample"));

                const char* rungName = GuiPerfFloor::rungName(rung);

                EXPECT_TRUE(StyleManager::setSampleStyleSheet(&sample, QStringLiteral("sampletests:sheet1.qss"))) << rungName;
                EXPECT_TRUE(sample.styleSheet().contains(QStringLiteral("QLabel { background-color: red; }"))) << rungName;
                EXPECT_TRUE(sample.style()->inherits("QStyleSheetStyle")) << rungName;
            }
        }

        TEST_F(SampleSheetFixture, SetSampleStyleSheet_TheFlatGuiSetsTheMarginOfTheLabelsAheadOfTheFile)
        {
            // Text.qss:93-94: the legacy GUI has the margin from the application's sheet and sets the file as it is
            const QString margin = QStringLiteral("QLabel { margin-top: 1px; margin-bottom: 1px; }");
            {
                RungScope scope(Rung::Qss);
                QLabel sample;
                ASSERT_TRUE(StyleManager::setSampleStyleSheet(&sample, QStringLiteral("sampletests:sheet1.qss")));
                EXPECT_FALSE(sample.styleSheet().contains(QStringLiteral("margin")));
            }
            {
                RungScope scope(Rung::Flat);
                QLabel sample;
                ASSERT_TRUE(StyleManager::setSampleStyleSheet(&sample, QStringLiteral("sampletests:sheet1.qss")));
                EXPECT_TRUE(sample.styleSheet().startsWith(margin)) << sample.styleSheet().toStdString();
                EXPECT_TRUE(sample.styleSheet().contains(QStringLiteral("QLabel { background-color: red; }")));
            }
        }

        TEST_F(SampleSheetFixture, SetSampleStyleSheet_TheFlatGuiReloadsTheSheetOnRefresh)
        {
            RungScope scope(Rung::Flat);
            QLabel sample;
            ASSERT_TRUE(StyleManager::setSampleStyleSheet(&sample, QStringLiteral("sampletests:sheet1.qss")));
            const QString set = sample.styleSheet();

            // The widget has another sheet than the file's, as it has when the file changes on disk
            sample.setStyleSheet(QStringLiteral("QLabel { background-color: blue; }"));
            m_styleManager.refresh();

            EXPECT_EQ(sample.styleSheet(), set);
            EXPECT_TRUE(sample.style()->inherits("QStyleSheetStyle"));
        }

        TEST_F(SampleSheetFixture, SetStyleSheet_StillSetsNoSheetInTheFlatGuiAndTheFileInTheLegacyOne)
        {
            {
                RungScope scope(Rung::Qss);
                QLabel carried;
                EXPECT_TRUE(StyleManager::setStyleSheet(&carried, QStringLiteral("sampletests:sheet1.qss")));
                EXPECT_TRUE(carried.styleSheet().contains(QStringLiteral("QLabel { background-color: red; }")));
            }
            {
                RungScope scope(Rung::Flat);
                QLabel carried;
                EXPECT_FALSE(StyleManager::setStyleSheet(&carried, QStringLiteral("sampletests:sheet1.qss")));
                EXPECT_TRUE(carried.styleSheet().isEmpty());
                EXPECT_FALSE(carried.style()->inherits("QStyleSheetStyle"));
            }
        }

        TEST_F(SampleSheetFixture, SetSampleStyleSheet_AFileThatIsNotFoundSetsNothingInEitherGui)
        {
            for (const Rung rung : { Rung::Qss, Rung::Flat })
            {
                RungScope scope(rung);
                QLabel sample;
                EXPECT_FALSE(StyleManager::setSampleStyleSheet(&sample, QStringLiteral("sampletests:nosuchsheet.qss")));
                EXPECT_TRUE(sample.styleSheet().isEmpty());
            }
        }
    } // namespace InlineSheetSiteTestsInternal
} // namespace AzQtComponents
