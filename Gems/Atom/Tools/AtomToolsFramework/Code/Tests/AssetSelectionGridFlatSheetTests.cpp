/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AtomToolsFramework/AssetSelection/AssetSelectionGrid.h>
#include <AzTest/AzTest.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/std/functional.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>

#include <AzQtComponents/Components/Elements/BoxElement.h>
#include <AzQtComponents/Components/Elements/TextElement.h>
#include <AzQtComponents/Components/Elements/ToolTipElement.h>
#include <AzQtComponents/Components/GuiPerfFloor.h>
#include <AzQtComponents/Components/Style.h>
#include <AzQtComponents/Components/Widgets/ElidingLabel.h>
#include <AzQtComponents/Tests/ToolTipTwins.h>

#include <QApplication>
#include <QFont>
#include <QFontInfo>
#include <QFrame>
#include <QImage>
#include <QLabel>
#include <QListWidget>
#include <QPainter>
#include <QPalette>
#include <QStyleFactory>
#include <QStyleOption>
#include <QWidget>

#include <string>

namespace UnitTest
{
    // ----------------------------------------------------------------------------------
    // The element of the asset tiles' headers: which widgets it reaches, and that it paints what the header's sheet painted (Qt's own
    // style sheet style over Fusion, with the same sheet and the rules of the application's sheet that reach a header) at the device
    // ratios of the GUI. The header is the ElidingLabel AddPath builds for every tile: 128 x 15, no contents margins, tagged
    // primaryText and Label. AddPath itself needs the asset system and the thumbnailer, so the tests build the header and its
    // place: the grid > its list (m_assetList) > the list's viewport > the tile > the header.
    // ----------------------------------------------------------------------------------

    namespace AssetSelectionGridFlatSheetTestsInternal
    {
        //! A widget that Qt takes for an instance of a class it is not. The matcher asks for the grid by its class name, as a
        //! selector does, and a real grid is the whole asset dialog.
        class ClassNamedWidget
            : public QWidget
        {
        public:
            ClassNamedWidget(const char* className, QWidget* parent = nullptr)
                : QWidget(parent)
                , m_className(className)
            {
            }

            void* qt_metacast(const char* name) override
            {
                return qstrcmp(name, m_className) == 0 ? this : QWidget::qt_metacast(name);
            }

        private:
            const char* m_className;
        };

        constexpr const char* GridClass = "AtomToolsFramework::AssetSelectionGrid";
        constexpr const char* HeaderElementName = "AtomTools.Box.AssetSelectionGridHeader";

        // AddPath: the tile is the size the grid is given (128 by default), the header is headerHeight high (15 by default)
        constexpr int HeaderWidth = 128;
        constexpr int HeaderHeight = 15;

        //! The sheet AddPath gave the header (AssetSelectionGrid.cpp:159 before the flattening). Copied, not read from the product:
        //! AddPath needs the asset system and the thumbnailer, so no test runs it. If the text of the setLegacyStyleSheet call in
        //! AddPath changes, change this one with it.
        constexpr const char* HeaderSheet = "background-color: rgb(35, 35, 35)";

        //! The rules of the application's sheet that reach a header, which the legacy cascade laid under its own sheet. The header is
        //! an ElidingLabel (a QLabel) with the class tags primaryText and Label.
        constexpr const char* ApplicationRules =
            // BaseStyleSheet.qss:15-21, the universal rule: the header's own background-color wins over it
            "* { background-color: transparent; color: white; font-family: \"Open Sans\"; font-weight: 400; }"
            // Text.qss:11-14
            ".primaryText { color: #FFFFFF; }"
            // Text.qss:89-95: the font and the margin of every label
            ".Label, QLabel { font-family: \"Open Sans\"; font-size: 12px; margin-top: 1px; margin-bottom: 1px; }";

        constexpr QRgb ParentColor = 0x808080;          // what neither the sheet nor the element draws stays this grey
        constexpr QRgb FillColor = 0x232323;            // rgb(35, 35, 35)

        //! The header as AddPath builds it (the order of its lines, less the sheet)
        AzQtComponents::ElidingLabel* makeHeader(QWidget* parent)
        {
            auto* header = new AzQtComponents::ElidingLabel(parent);
            header->setText(QStringLiteral("Asset title"));
            header->setFixedSize(QSize(HeaderWidth, HeaderHeight));
            header->setContentsMargins(0, 0, 0, 0);
            header->setProperty("class", QStringLiteral("primaryText Label"));         // Text::addPrimaryStyle, Text::addLabelStyle
            return header;
        }

        //! A header in its place below a stand-in of the grid
        struct FlatTile
        {
            FlatTile()
                : m_grid(GridClass)
            {
                m_list = new QListWidget(&m_grid);
                m_list->setObjectName(QStringLiteral("m_assetList"));
                m_tile = new QWidget(m_list->viewport());
                m_header = makeHeader(m_tile);
            }

            ClassNamedWidget m_grid;
            QListWidget* m_list = nullptr;
            QWidget* m_tile = nullptr;
            AzQtComponents::ElidingLabel* m_header = nullptr;
        };

        //! The option QFrame::paintEvent builds for a frame (QFrame::initStyleOption, protected)
        QStyleOptionFrame frameOption(const QFrame* frame)
        {
            QStyleOptionFrame option;
            option.initFrom(frame);
            option.frameShape = frame->frameShape();
            option.lineWidth = frame->lineWidth();
            option.midLineWidth = frame->midLineWidth();
            if (frame->frameShadow() == QFrame::Sunken)
            {
                option.state |= QStyle::State_Sunken;
            }
            else if (frame->frameShadow() == QFrame::Raised)
            {
                option.state |= QStyle::State_Raised;
            }
            return option;
        }

        //! What a widget paints for its box through its own style, in the order Qt paints it: the styled background (PE_Widget,
        //! QWidgetPrivate::paintBackground), then a frame's frame (CE_ShapedFrame, QFrame::drawFrame). The image is the painter's
        //! device, so the style scales the box to the ratio as a screen of that ratio does.
        QImage paintBox(const QWidget& widget, qreal ratio)
        {
            QImage image((QSizeF(widget.size()) * ratio).toSize(), QImage::Format_ARGB32_Premultiplied);
            image.setDevicePixelRatio(ratio);
            image.fill(QColor::fromRgb(ParentColor));
            QPainter painter(&image);
            if (widget.testAttribute(Qt::WA_StyledBackground))
            {
                QStyleOption background;
                background.initFrom(&widget);
                widget.style()->drawPrimitive(QStyle::PE_Widget, &background, &painter, &widget);
            }
            if (const auto* frame = qobject_cast<const QFrame*>(&widget))
            {
                const QStyleOptionFrame option = frameOption(frame);
                widget.style()->drawControl(QStyle::CE_ShapedFrame, &option, &painter, &widget);
            }
            painter.end();
            return image;
        }

        //! The first pixel two images of one size differ at, as text for a failure message ("" when they are equal)
        std::string firstDifference(const QImage& a, const QImage& b)
        {
            if (a.size() != b.size())
            {
                return "the sizes differ";
            }
            for (int y = 0; y < a.height(); ++y)
            {
                for (int x = 0; x < a.width(); ++x)
                {
                    if (a.pixel(x, y) != b.pixel(x, y))
                    {
                        const auto hex = [](QRgb rgb)
                        {
                            return QString::number(rgb & 0xFFFFFF, 16).toStdString();
                        };
                        return "at (" + std::to_string(x) + ", " + std::to_string(y) + "): legacy #" + hex(a.pixel(x, y)) + ", flat #"
                            + hex(b.pixel(x, y));
                    }
                }
            }
            return "";
        }

        class AssetSelectionGridFlatSheetTests
            : public LeakDetectionFixture
        {
        protected:
            void SetUp() override
            {
                LeakDetectionFixture::SetUp();
                if (!QApplication::instance())
                {
                    m_application = AZStd::make_unique<QApplication>(m_argc, m_argv);
                }

                // The test application may carry StyleManager's legacy sheets: the twins name their own
                m_applicationSheet = qApp->styleSheet();
                qApp->setStyleSheet(QString());

                // The flattened GUI's branches of Style (CE_ShapedFrame) read the rung
                AzQtComponents::GuiPerfFloor::setRungForTesting(AzQtComponents::GuiPerfFloor::Rung::Flat);

                m_style = AZStd::make_unique<AzQtComponents::Style>(QStyleFactory::create(QStringLiteral("Fusion")));
                m_fusion = AZStd::unique_ptr<QStyle>(QStyleFactory::create(QStringLiteral("Fusion")));
                AtomToolsFramework::AssetSelectionGrid::RegisterFlatElements();
            }

            void TearDown() override
            {
                m_fusion.reset();
                m_style.reset();
                AzQtComponents::GuiPerfFloor::resetForTesting();
                qApp->setStyleSheet(m_applicationSheet);
                m_application.reset();
                LeakDetectionFixture::TearDown();
            }

            //! The flat way: the style on the widget, and the element resolved as Style::polish does in the flattened GUI
            void flatten(QWidget& widget)
            {
                widget.setStyle(m_style.get());
                AzQtComponents::BoxElement::resolve(&widget);
            }

            static const char* elementOf(const QWidget& widget)
            {
                const AzQtComponents::BoxElement* element = AzQtComponents::BoxElement::forWidget(&widget);
                return element ? element->m_name : "";
            }

            //! The header as the legacy GUI styled it: Qt's style sheet style over Fusion, with the application's rules above it as
            //! the sheet of its ancestor and, if asked, the sheet AddPath gave it. The callback gets the polished header.
            void withLegacyHeader(bool ownSheet, const AZStd::function<void(QWidget&)>& use) const
            {
                QWidget root;
                QWidget* header = makeHeader(&root);
                root.setStyle(m_fusion.get());
                header->setStyle(m_fusion.get());
                if (ownSheet)
                {
                    header->setStyleSheet(QString::fromLatin1(HeaderSheet));
                }
                root.setStyleSheet(QString::fromLatin1(ApplicationRules));
                root.ensurePolished();
                header->ensurePolished();
                use(*header);
            }

            //! The same header in the flattened GUI: in the asset list of a grid, with its element
            void withFlatHeader(const AZStd::function<void(QWidget&)>& use)
            {
                FlatTile tile;
                flatten(*tile.m_header);
                use(*tile.m_header);
            }

            QImage legacyHeader(qreal ratio) const
            {
                QImage image;
                withLegacyHeader(true, [&image, ratio](QWidget& widget) { image = paintBox(widget, ratio); });
                return image;
            }

            QImage flatHeader(qreal ratio)
            {
                QImage image;
                withFlatHeader([&image, ratio](QWidget& widget) { image = paintBox(widget, ratio); });
                return image;
            }

            //! The contents box of a frame (SE_ShapedFrameContents): where a label lays out its text
            static QRect frameContents(QWidget& widget)
            {
                const QStyleOptionFrame option = frameOption(qobject_cast<QFrame*>(&widget));
                return widget.style()->subElementRect(QStyle::SE_ShapedFrameContents, &option, &widget);
            }

            int m_argc = 1;                                     // QApplication keeps a reference to it
            char m_programName[5] = "test";
            char* m_argv[2] = { m_programName, nullptr };
            AZStd::unique_ptr<QApplication> m_application;
            QString m_applicationSheet;
            AZStd::unique_ptr<AzQtComponents::Style> m_style;
            AZStd::unique_ptr<QStyle> m_fusion;
        };
    } // namespace AssetSelectionGridFlatSheetTestsInternal

    // A unity build joins this file with others of namespace UnitTest: the fixture is the only name brought in, the rest is named
    // through the alias
    using AssetSelectionGridFlatSheetTestsInternal::AssetSelectionGridFlatSheetTests;
    namespace GridFlat = AssetSelectionGridFlatSheetTestsInternal;

    // ----------------------------------------------------------------------------------
    // Resolution
    // ----------------------------------------------------------------------------------

    TEST_F(AssetSelectionGridFlatSheetTests, Registry_EveryHeaderInAGridsAssetListHasTheElement)
    {
        GridFlat::FlatTile tile;
        auto* anotherTile = new QWidget(tile.m_list->viewport());
        AzQtComponents::ElidingLabel* anotherHeader = GridFlat::makeHeader(anotherTile);

        flatten(*tile.m_header);
        flatten(*anotherHeader);

        EXPECT_STREQ(elementOf(*tile.m_header), GridFlat::HeaderElementName);
        EXPECT_STREQ(elementOf(*anotherHeader), GridFlat::HeaderElementName);
        EXPECT_TRUE(tile.m_header->testAttribute(Qt::WA_StyledBackground));         // the fill is drawn at PE_Widget
    }

    TEST_F(AssetSelectionGridFlatSheetTests, Registry_TheHeaderBeforeItsTileIsInTheViewportIsTheSame)
    {
        // AddPath builds the tile on the list itself, and setItemWidget moves it into the viewport: the header is the grid's
        // either way
        GridFlat::FlatTile tile;
        auto* earlyTile = new QWidget(tile.m_list);
        AzQtComponents::ElidingLabel* earlyHeader = GridFlat::makeHeader(earlyTile);

        flatten(*earlyHeader);

        EXPECT_STREQ(elementOf(*earlyHeader), GridFlat::HeaderElementName);
    }

    TEST_F(AssetSelectionGridFlatSheetTests, Registry_NothingElseInTheGridOrOutsideItHasTheElement)
    {
        GridFlat::FlatTile tile;

        // The label the header keeps hidden to measure its text is a plain QLabel; the tile and the list are not labels
        auto* metrics = tile.m_header->findChild<QLabel*>();
        ASSERT_TRUE(metrics != nullptr);
        auto* plainLabel = new QLabel(tile.m_tile);
        flatten(*metrics);
        flatten(*plainLabel);
        EXPECT_STRNE(elementOf(*metrics), GridFlat::HeaderElementName);
        EXPECT_STRNE(elementOf(*plainLabel), GridFlat::HeaderElementName);

        // A header in the grid, but outside its list, and below another list of the grid
        auto* looseHeader = GridFlat::makeHeader(&tile.m_grid);
        auto* otherList = new QListWidget(&tile.m_grid);
        otherList->setObjectName(QStringLiteral("m_otherList"));
        auto* otherHeader = GridFlat::makeHeader(otherList->viewport());
        flatten(*looseHeader);
        flatten(*otherHeader);
        EXPECT_STRNE(elementOf(*looseHeader), GridFlat::HeaderElementName);
        EXPECT_STRNE(elementOf(*otherHeader), GridFlat::HeaderElementName);

        // The same list name and the same kind of label, but no grid above them
        QWidget window;
        auto* list = new QListWidget(&window);
        list->setObjectName(QStringLiteral("m_assetList"));
        auto* strayHeader = GridFlat::makeHeader(list->viewport());
        flatten(*strayHeader);
        EXPECT_STRNE(elementOf(*strayHeader), GridFlat::HeaderElementName);
    }

    // ----------------------------------------------------------------------------------
    // Pixels: the element against the header's sheet, at the device ratios of the GUI
    // ----------------------------------------------------------------------------------

    TEST_F(AssetSelectionGridFlatSheetTests, Pixels_TheHeaderPaintsWhatItsSheetPaintedAtEveryRatio)
    {
        for (const qreal ratio : { 1.0, 1.5, 2.0 })
        {
            const QImage legacy = legacyHeader(ratio);
            const QImage flat = flatHeader(ratio);

            // The sheet drew its fill over the universal rule's transparent one, inside the margin of the label rule
            EXPECT_EQ(legacy.pixel(legacy.width() / 2, legacy.height() / 2) & 0xFFFFFF, GridFlat::FillColor) << "ratio " << ratio;
            EXPECT_EQ(GridFlat::firstDifference(legacy, flat), std::string()) << "at ratio " << ratio;
        }
    }

    TEST_F(AssetSelectionGridFlatSheetTests, Pixels_TheFillIsInsideTheOnePixelMarginAboveAndBelow)
    {
        // The header is 15 logical pixels high: the margin of one above and one below, 13 of fill between, the whole width
        const QImage one = flatHeader(1.0);
        EXPECT_EQ(one.pixel(64, 0) & 0xFFFFFF, GridFlat::ParentColor);
        EXPECT_EQ(one.pixel(64, 1) & 0xFFFFFF, GridFlat::FillColor);
        EXPECT_EQ(one.pixel(64, 13) & 0xFFFFFF, GridFlat::FillColor);
        EXPECT_EQ(one.pixel(64, 14) & 0xFFFFFF, GridFlat::ParentColor);
        EXPECT_EQ(one.pixel(0, 7) & 0xFFFFFF, GridFlat::FillColor);                       // no margin at the sides
        EXPECT_EQ(one.pixel(GridFlat::HeaderWidth - 1, 7) & 0xFFFFFF, GridFlat::FillColor);

        // and at ratio 2 the margin is two rows
        const QImage two = flatHeader(2.0);
        EXPECT_EQ(two.pixel(128, 1) & 0xFFFFFF, GridFlat::ParentColor);
        EXPECT_EQ(two.pixel(128, 2) & 0xFFFFFF, GridFlat::FillColor);
        EXPECT_EQ(two.pixel(128, 27) & 0xFFFFFF, GridFlat::FillColor);
        EXPECT_EQ(two.pixel(128, 28) & 0xFFFFFF, GridFlat::ParentColor);
    }

    // ----------------------------------------------------------------------------------
    // Contents, font and palette: what the sheet laid out, and what it left alone
    // ----------------------------------------------------------------------------------

    TEST_F(AssetSelectionGridFlatSheetTests, Box_TheHeadersContentsAreThoseOfTheLabelRuleUnderItsSheet)
    {
        // Where the label lays out its text (ElidingLabel::TextRect starts from the contents): the label rule's margin above and below
        QRect legacy;
        QRect flat;
        withLegacyHeader(true, [&legacy](QWidget& widget) { legacy = frameContents(widget); });
        withFlatHeader([&flat](QWidget& widget) { flat = frameContents(widget); });

        EXPECT_EQ(flat, legacy);
        EXPECT_EQ(flat, QRect(0, 1, GridFlat::HeaderWidth, GridFlat::HeaderHeight - 2));
    }

    TEST_F(AssetSelectionGridFlatSheetTests, Font_TheSheetAndTheElementSetNone)
    {
        // The legacy header has the font the application's rules gave every label, with its own sheet or without it
        QFont withSheet;
        QFont withoutSheet;
        withLegacyHeader(true, [&withSheet](QWidget& widget) { withSheet = widget.font(); });
        withLegacyHeader(false, [&withoutSheet](QWidget& widget) { withoutSheet = widget.font(); });

        EXPECT_EQ(QFontInfo(withSheet).pixelSize(), QFontInfo(withoutSheet).pixelSize());
        EXPECT_EQ(withSheet.pixelSize(), 12);                       // Text.qss:92
        EXPECT_EQ(withSheet.weight(), withoutSheet.weight());
        EXPECT_EQ(withSheet.weight(), QFont::Normal);               // BaseStyleSheet.qss:20

        // The flat header: Style::polish resolves the text element of the widget first, the box element last. The tags `primaryText
        // Label` give the Label tier (TextElement.cpp, TierRules), whose element sets the family, the size and the weight
        GridFlat::FlatTile tile;
        tile.m_header->setStyle(m_style.get());
        AzQtComponents::TextElement::resolve(tile.m_header);
        const QFont flat = tile.m_header->font();

        EXPECT_EQ(QFontInfo(flat).pixelSize(), QFontInfo(withSheet).pixelSize());
        EXPECT_EQ(flat.weight(), withSheet.weight());
        EXPECT_TRUE(flat.family() == withSheet.family());
        EXPECT_TRUE(QFontInfo(flat).family() == QFontInfo(withSheet).family());

        // The element leaves the font as the style and the text element made it
        AzQtComponents::BoxElement::resolve(tile.m_header);
        EXPECT_STREQ(elementOf(*tile.m_header), GridFlat::HeaderElementName);
        EXPECT_TRUE(tile.m_header->font() == flat);
    }

    TEST_F(AssetSelectionGridFlatSheetTests, Palette_TheSheetFillsTheBackgroundRolesAndTheElementLeavesThePaletteAlone)
    {
        QPalette withSheet;
        QPalette withoutSheet;
        withLegacyHeader(true, [&withSheet](QWidget& widget) { withSheet = widget.palette(); });
        withLegacyHeader(false, [&withoutSheet](QWidget& widget) { withoutSheet = widget.palette(); });

        // The sheet's fill is the Window, Base and Button of the header and of what is below it (QRenderRule::configurePalette); a label
        // draws its text from the roles the sheet left alone, which are the same without it
        for (const QPalette::ColorRole role : { QPalette::Window, QPalette::Base, QPalette::Button })
        {
            EXPECT_EQ(withSheet.color(QPalette::Active, role).rgb() & 0xFFFFFF, GridFlat::FillColor) << "role " << static_cast<int>(role);
        }
        for (const QPalette::ColorGroup group : { QPalette::Active, QPalette::Inactive, QPalette::Disabled })
        {
            for (const QPalette::ColorRole role : { QPalette::WindowText, QPalette::Text, QPalette::ButtonText })
            {
                EXPECT_TRUE(withSheet.color(group, role) == withoutSheet.color(group, role))
                    << "group " << static_cast<int>(group) << ", role " << static_cast<int>(role);
            }
        }

        // The colour of the text: white in the legacy header (BaseStyleSheet.qss:18 `*`, Text.qss:11-14 `.primaryText`). The Label tier's
        // text element sets no colour, so the flat header keeps the application palette's. That palette is the application style's
        // (StyleManager::initialize), which this fixture does not install, so the flat half cannot be compared here: AzQtComponents.Tests
        // FlatPaletteTests proves the application palette's white for a label (DisabledText_ApplicationPaletteKeepsTheStarRulesWhite,
        // DisabledText_LabelIsGreyOnlyInsideACard), and the A/B of the flattened GUI covers the rest.
        for (const QPalette::ColorGroup group : { QPalette::Active, QPalette::Inactive })
        {
            EXPECT_EQ(withSheet.color(group, QPalette::WindowText).rgb() & 0xFFFFFF, 0xFFFFFFu) << "group " << static_cast<int>(group);
        }

        // The element draws the fill itself: it puts nothing in the widget's palette, and nothing is drawn from the roles it leaves
        GridFlat::FlatTile tile;
        tile.m_header->setStyle(m_style.get());
        const QPalette before = tile.m_header->palette();
        AzQtComponents::BoxElement::resolve(tile.m_header);
        EXPECT_TRUE(tile.m_header->palette() == before);
    }
    // ----------------------------------------------------------------------------------
    // The tooltip of a header (the full name where it is elided, or the description). The header's sheet had no selector, so it reached
    // the tip the header shows: `background-color: rgb(35, 35, 35)` outranked `QToolTip { background-color: black }` (ToolTip.qss:13).
    // ----------------------------------------------------------------------------------

    TEST_F(AssetSelectionGridFlatSheetTests, ToolTipRegistry_EveryHeaderInAGridsAssetListHasTheFillAndNothingElseDoes)
    {
        using AzQtComponents::ToolTipElement;
        const QColor fill(35, 35, 35);

        GridFlat::FlatTile tile;
        auto* anotherTile = new QWidget(tile.m_list->viewport());
        AzQtComponents::ElidingLabel* anotherHeader = GridFlat::makeHeader(anotherTile);
        for (const AzQtComponents::ElidingLabel* header : { tile.m_header, anotherHeader })
        {
            EXPECT_EQ(ToolTipElement::lookFor(header).m_fill, fill);
            EXPECT_EQ(ToolTipElement::lookFor(header).m_text, nullptr);
        }

        // The sheet was set on the header alone: the tips of the list, of its viewport and of the tile (the thumbnail's, the items')
        // are not the header's; nor is a header outside any grid
        for (const QWidget* widget : { static_cast<const QWidget*>(tile.m_list), static_cast<const QWidget*>(tile.m_list->viewport()),
                                       static_cast<const QWidget*>(tile.m_tile), static_cast<const QWidget*>(&tile.m_grid) })
        {
            EXPECT_FALSE(ToolTipElement::lookFor(widget).m_fill.isValid()) << widget->metaObject()->className();
        }
        QWidget window;
        AzQtComponents::ElidingLabel* loose = GridFlat::makeHeader(&window);
        EXPECT_FALSE(ToolTipElement::lookFor(loose).m_fill.isValid());
    }

    TEST_F(AssetSelectionGridFlatSheetTests, ToolTip_TheHeaderShowsTheFillOfItsSheetAtEveryRatio)
    {
        AzQtComponents::ToolTipTwins::Guard tips;
        AzQtComponents::ToolTipTwins::StyleFilter filter(m_style.get());
        namespace Twins = AzQtComponents::ToolTipTwins;

        // The header as the legacy GUI styled it, with the rules of the application's sheet that reach a tip above it
        Twins::TipFacts legacy;
        {
            QWidget root;
            AzQtComponents::ElidingLabel* header = GridFlat::makeHeader(&root);
            header->setToolTip(QStringLiteral("Tip"));         // not setDescription: it stores the string, ElidingLabel::elide sets the tip
            root.setStyle(m_fusion.get());
            header->setStyle(m_fusion.get());
            header->setStyleSheet(QString::fromLatin1(GridFlat::HeaderSheet));
            root.setStyleSheet(QString::fromLatin1(Twins::AppRules) + QString::fromLatin1(GridFlat::ApplicationRules));
            root.ensurePolished();
            header->ensurePolished();
            legacy = Twins::showAndReadTip(header, nullptr);
        }

        // The same in the flattened GUI, in the asset list of a grid. The font and the margin of the label are the legacy tip's: this
        // process has the platform's fonts and style, and what a fill proves is not a font
        Twins::TipFacts flat;
        {
            GridFlat::FlatTile tile;
            tile.m_header->setToolTip(QStringLiteral("Tip"));
            flat = Twins::showAndReadTip(tile.m_header, m_style.get(), &legacy.m_font, legacy.m_margin);
        }

        ASSERT_TRUE(legacy.m_found && flat.m_found);
        EXPECT_EQ(legacy.m_base.name(), QStringLiteral("#232323"));
        EXPECT_EQ(flat.m_base.name(), legacy.m_base.name());
        EXPECT_EQ(flat.m_text.name(), legacy.m_text.name());
        EXPECT_EQ(Twins::marginsText(flat.m_contents), Twins::marginsText(legacy.m_contents));
        EXPECT_EQ(flat.m_hint, legacy.m_hint);
        EXPECT_EQ(flat.m_contentsRect, legacy.m_contentsRect);
        for (int i = 0; i < 3; ++i)
        {
            EXPECT_EQ(Twins::differingPixels(legacy.m_images[i], flat.m_images[i]), 0) << "at ratio " << Twins::Ratios[i];
        }
    }
} // namespace UnitTest
