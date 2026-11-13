/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzTest/AzTest.h>
#include <AzCore/std/functional.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>

#include <AzQtComponents/Components/Elements/BoxElement.h>
#include <AzQtComponents/Components/Elements/MenuElement.h>
#include <AzQtComponents/Components/Elements/TextElement.h>
#include <AzQtComponents/Components/GuiPerfFloor.h>
#include <AzQtComponents/Components/Style.h>

#include <Editor/DocumentationLinkWidget.h>

#include <QApplication>
#include <QFont>
#include <QFontInfo>
#include <QFrame>
#include <QImage>
#include <QLabel>
#include <QMenu>
#include <QPainter>
#include <QStyleFactory>
#include <QStyleOption>
#include <QWidget>

#include <string>

namespace UnitTest
{
    // ----------------------------------------------------------------------------------
    // The elements of the PhysX documentation link: which widgets they reach, and that they paint and lay out what the link's own
    // sheet did (Qt's style sheet style over Fusion, with the same sheet and the rules of the application sheet that reach a label
    // and the menu it pops up), at the device ratios the GUI runs at
    // ----------------------------------------------------------------------------------

    namespace PhysXFlatSheetTestsInternal
    {
        constexpr const char* LinkElementName = "PhysX.Box.DocumentationLink";
        constexpr const char* LinkMenuElementName = "PhysX.Menu.DocumentationLink";

        // The sheet the link gave itself (DocumentationLinkWidget.cpp:24 before the flattening)
        constexpr const char* LinkSheet = "background-color: rgb(51, 51, 51);";

        // The rules of the application sheet that reach a label in the Editor, and only those: the universal rule
        // (BaseStyleSheet.qss:15-21) and the label rule (Text.qss:89-95). Editor.qss has no rule for a label like this one.
        constexpr const char* LabelRules =
            "* { background-color: transparent; color: white; font-family: \"Open Sans\"; font-weight: 400; }"
            "QLabel { font-family: \"Open Sans\"; font-size: 12px; margin-top: 1px; margin-bottom: 1px; }";

        // The rule of the application sheet that reaches the menu the link pops up, and sets what the link's sheet also sets:
        // `QMenu { background-color; padding }` (Menu.qss:10-14). The universal rule above reaches the menu too.
        constexpr const char* MenuRule = "QMenu { background-color: #222222; padding: 4px 2px; }";

        // The link's text and size
        constexpr const char* LinkFormat = "Learn more about <a href=%1>configuring PhysX.</a>";
        constexpr const char* LinkAddress = "https://example.com/physx";
        constexpr QSize LinkSize(200, 33);
        constexpr QSize MenuSize(120, 60);

        constexpr QRgb ParentColor = 0x808080;          // what neither the sheet nor the element draws stays this grey

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

        //! The label as the link built itself before the flattening (DocumentationLinkWidget.cpp:17-23): the same text, alignment,
        //! margins and word wrap, and no sheet yet
        void configureLikeTheLink(QLabel& label)
        {
            label.setText(QString::fromLatin1(LinkFormat).arg(QString::fromLatin1(LinkAddress)));
            label.setTextInteractionFlags(Qt::TextBrowserInteraction);
            label.setOpenExternalLinks(true);
            label.setAlignment(Qt::AlignCenter);
            label.setContentsMargins(60, 7, 60, 7);
            label.setWordWrap(true);
            label.resize(LinkSize);
        }

        class PhysXFlatSheetTests
            : public ::testing::Test
        {
        protected:
            void SetUp() override
            {
                // The test application carries no legacy sheets, and the twins name their own
                m_applicationSheet = qApp->styleSheet();
                qApp->setStyleSheet(QString());
                m_applicationFont = QApplication::font();

                // The flattened GUI's branches of Style (CE_ShapedFrame) and the link's constructor read the rung
                AzQtComponents::GuiPerfFloor::setRungForTesting(AzQtComponents::GuiPerfFloor::Rung::Flat);

                m_style = AZStd::make_unique<AzQtComponents::Style>(QStyleFactory::create(QStringLiteral("Fusion")));
                m_fusion = AZStd::unique_ptr<QStyle>(QStyleFactory::create(QStringLiteral("Fusion")));

                // The class fonts the flattened GUI sets up for the application (Style::polish(QApplication*)): the font of a label.
                // They replace the platform's class fonts (QMenu, QMessageBox, ...), and TearDown cannot give those back: the list of
                // the classes is private to TextElement.cpp, and QApplication::setFont(font) without a class name clears all of them.
                // No other test of PhysX5.Editor.Tests builds a widget, so nothing reads them after this fixture.
                AzQtComponents::TextElement::installClassFonts();
            }

            void TearDown() override
            {
                m_fusion.reset();
                m_style.reset();
                AzQtComponents::GuiPerfFloor::resetForTesting();
                QApplication::setFont(m_applicationFont);       // without a class name this clears the class fonts (see SetUp)
                qApp->setStyleSheet(m_applicationSheet);
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

            //! The link as the legacy GUI styled it: Qt's style sheet style over Fusion, with the link's own sheet on the label and the
            //! rules of the application sheet above it. The callback gets the polished label.
            void withLegacyLink(const AZStd::function<void(QLabel&)>& use) const
            {
                QWidget root;
                auto* label = new QLabel(&root);
                configureLikeTheLink(*label);
                root.setStyle(m_fusion.get());
                label->setStyle(m_fusion.get());
                label->setStyleSheet(QString::fromLatin1(LinkSheet));
                root.setStyleSheet(QString::fromLatin1(LabelRules));
                root.ensurePolished();
                label->ensurePolished();
                use(*label);
            }

            //! The same link in the flattened GUI: the real widget, built in the flat rung (no sheet, its element registered). The
            //! callback gets the polished link.
            void withFlatLink(const AZStd::function<void(QLabel&)>& use)
            {
                QWidget root;
                auto* link = new PhysX::Editor::DocumentationLinkWidget(QString::fromLatin1(LinkFormat), QString::fromLatin1(LinkAddress));
                link->setParent(&root);
                link->resize(LinkSize);
                flatten(*link);
                link->ensurePolished();
                use(*link);
            }

            //! The link's right-click menu as the legacy GUI styled it: a QMenu the label parents (QLabel's standard context menu), under
            //! the link's own sheet and the rules of the application sheet above it. The callback gets the polished menu.
            void withLegacyLinkMenu(const AZStd::function<void(QMenu&)>& use) const
            {
                QWidget root;
                auto* label = new QLabel(&root);
                configureLikeTheLink(*label);
                root.setStyle(m_fusion.get());
                label->setStyle(m_fusion.get());
                label->setStyleSheet(QString::fromLatin1(LinkSheet));
                root.setStyleSheet(QString::fromLatin1(LabelRules) + QString::fromLatin1(MenuRule));
                root.ensurePolished();
                label->ensurePolished();

                QMenu menu(label);
                menu.resize(MenuSize);
                menu.ensurePolished();
                use(menu);
            }

            //! The same menu in the flattened GUI: the real link, a QMenu it parents, and the flat style on the menu
            void withFlatLinkMenu(const AZStd::function<void(QMenu&)>& use)
            {
                QWidget root;
                auto* link = new PhysX::Editor::DocumentationLinkWidget(QString::fromLatin1(LinkFormat), QString::fromLatin1(LinkAddress));
                link->setParent(&root);
                link->resize(LinkSize);
                flatten(*link);

                QMenu menu(link);
                menu.resize(MenuSize);
                menu.setStyle(m_style.get());
                menu.ensurePolished();
                use(menu);
            }

            //! The colour in the middle of the menu, where only its surface is: the menu as the paint event draws it
            static QColor menuSurface(QMenu& menu)
            {
                const QImage image = menu.grab().toImage();
                return image.pixelColor(image.width() / 2, image.height() / 2);
            }

            QImage legacyLink(qreal ratio) const
            {
                QImage image;
                withLegacyLink([&image, ratio](QLabel& label) { image = paintBox(label, ratio); });
                return image;
            }

            QImage flatLink(qreal ratio)
            {
                QImage image;
                withFlatLink([&image, ratio](QLabel& label) { image = paintBox(label, ratio); });
                return image;
            }

            //! The contents box of a frame (SE_ShapedFrameContents): where a label lays out its text, and whether it indents it
            static QRect frameContents(QLabel& label)
            {
                const QStyleOptionFrame option = frameOption(&label);
                return label.style()->subElementRect(QStyle::SE_ShapedFrameContents, &option, &label);
            }

            QString m_applicationSheet;
            QFont m_applicationFont;
            AZStd::unique_ptr<AzQtComponents::Style> m_style;
            AZStd::unique_ptr<QStyle> m_fusion;
        };
    } // namespace PhysXFlatSheetTestsInternal

    // A unity build joins this file with others of namespace UnitTest: the fixture is the only name brought in, the rest is named
    // through the alias
    using PhysXFlatSheetTestsInternal::PhysXFlatSheetTests;
    namespace PhysXFlat = PhysXFlatSheetTestsInternal;

    // ----------------------------------------------------------------------------------
    // Resolution
    // ----------------------------------------------------------------------------------

    TEST_F(PhysXFlatSheetTests, Registry_ADocumentationLinkHasTheElementAndAStyledBackground)
    {
        QWidget root;
        auto* link = new PhysX::Editor::DocumentationLinkWidget(
            QString::fromLatin1(PhysXFlat::LinkFormat), QString::fromLatin1(PhysXFlat::LinkAddress));
        link->setParent(&root);

        flatten(*link);

        EXPECT_STREQ(elementOf(*link), PhysXFlat::LinkElementName);
        EXPECT_TRUE(link->testAttribute(Qt::WA_StyledBackground));      // the fill is drawn at PE_Widget
    }

    TEST_F(PhysXFlatSheetTests, Registry_OnlyTheLinkClassHasTheBoxElement)
    {
        // A label like the link in a window: the element is the class's (DocumentationLinkWidget has a metaobject of its own, which
        // a plain QLabel does not share). The links are the only labels the sheet was set on, and a label has no child widget but
        // its context menu, which is the menu element's (the next test).
        QWidget window;
        auto* plain = new QLabel(QString::fromLatin1(PhysXFlat::LinkFormat), &window);
        auto* link = new PhysX::Editor::DocumentationLinkWidget(
            QString::fromLatin1(PhysXFlat::LinkFormat), QString::fromLatin1(PhysXFlat::LinkAddress));
        link->setParent(&window);

        flatten(*plain);
        flatten(*link);

        EXPECT_STRNE(elementOf(*plain), PhysXFlat::LinkElementName);
        EXPECT_STREQ(elementOf(*link), PhysXFlat::LinkElementName);
    }

    TEST_F(PhysXFlatSheetTests, Registry_TheMenusInsideALinkHaveTheLinksFillAndNoOtherMenuDoes)
    {
        // The standard context menu of a text label is a QMenu the label parents, so the link's selector-less sheet reached it (and
        // any menu inside it) as a descendant
        QWidget window;
        auto* link = new PhysX::Editor::DocumentationLinkWidget(
            QString::fromLatin1(PhysXFlat::LinkFormat), QString::fromLatin1(PhysXFlat::LinkAddress));
        link->setParent(&window);
        auto* plain = new QLabel(QString::fromLatin1(PhysXFlat::LinkFormat), &window);
        QMenu inLink(link);
        QMenu inMenuInLink(&inLink);
        QMenu inPlain(plain);
        QMenu alone;

        const AzQtComponents::MenuElement& global = AzQtComponents::MenuElement::forWidget(nullptr);
        const AzQtComponents::MenuElement& element = AzQtComponents::MenuElement::forWidget(&inLink);

        EXPECT_STREQ(element.m_name, PhysXFlat::LinkMenuElementName);
        EXPECT_EQ(&AzQtComponents::MenuElement::forWidget(&inMenuInLink), &element);
        EXPECT_EQ(&AzQtComponents::MenuElement::forWidget(&inPlain), &global);
        EXPECT_EQ(&AzQtComponents::MenuElement::forWidget(&alone), &global);

        // The surface is the link's fill; everything else is the Global popup's, which the application sheet gave the menu
        EXPECT_TRUE(element.m_surface == QColor(0x33, 0x33, 0x33));
        EXPECT_FALSE(global.m_surface == element.m_surface);
        EXPECT_EQ(element.m_paddingH, global.m_paddingH);
        EXPECT_EQ(element.m_paddingV, global.m_paddingV);
        EXPECT_TRUE(element.m_text == global.m_text);
        EXPECT_TRUE(element.m_selectedFill == global.m_selectedFill);
        EXPECT_TRUE(element.m_disabledText == global.m_disabledText);
        EXPECT_EQ(element.m_separatorMarginTop, global.m_separatorMarginTop);
        EXPECT_EQ(element.m_separatorBorder, global.m_separatorBorder);
        EXPECT_EQ(element.m_separatorMarginBottom, global.m_separatorMarginBottom);
        EXPECT_TRUE(element.m_separatorLine == global.m_separatorLine);
    }

    TEST_F(PhysXFlatSheetTests, Rungs_TheLegacyRungSetsTheLinksSheetAndTheFlatRungSetsNone)
    {
        using AzQtComponents::GuiPerfFloor;
        const QString linkArguments[] = { QString::fromLatin1(PhysXFlat::LinkFormat), QString::fromLatin1(PhysXFlat::LinkAddress) };

        GuiPerfFloor::setRungForTesting(GuiPerfFloor::Rung::Qss);
        {
            PhysX::Editor::DocumentationLinkWidget legacy(linkArguments[0], linkArguments[1]);
            EXPECT_EQ(legacy.styleSheet(), QString::fromLatin1(PhysXFlat::LinkSheet));
        }

        GuiPerfFloor::setRungForTesting(GuiPerfFloor::Rung::Flat);
        {
            PhysX::Editor::DocumentationLinkWidget flat(linkArguments[0], linkArguments[1]);
            EXPECT_TRUE(flat.styleSheet().isEmpty());
        }
    }

    // ----------------------------------------------------------------------------------
    // Pixels: the element against the link's sheet, at the device ratios of the GUI
    // ----------------------------------------------------------------------------------

    TEST_F(PhysXFlatSheetTests, Pixels_TheLinkPaintsWhatItsSheetPaintedAtEveryRatio)
    {
        for (const qreal ratio : { 1.0, 1.5, 2.0 })
        {
            const QImage legacy = legacyLink(ratio);
            const QImage flat = flatLink(ratio);
            EXPECT_EQ(PhysXFlat::firstDifference(legacy, flat), std::string()) << "at ratio " << ratio;
        }
    }

    TEST_F(PhysXFlatSheetTests, Pixels_TheFillStopsAtTheLabelRulesMarginAboveAndBelow)
    {
        // The legacy cascade laid the label rule's margin of 1px under the link's sheet, so the fill leaves a line above and below:
        // the parent shows there (the capture of the legacy Editor has the parent's colour in the first and the last row)
        const QImage flat = flatLink(1.0);

        EXPECT_EQ(flat.pixel(0, 0) & 0xFFFFFF, PhysXFlat::ParentColor);
        EXPECT_EQ(flat.pixel(0, 1) & 0xFFFFFF, 0x333333u);
        EXPECT_EQ(flat.pixel(flat.width() - 1, flat.height() - 2) & 0xFFFFFF, 0x333333u);
        EXPECT_EQ(flat.pixel(flat.width() - 1, flat.height() - 1) & 0xFFFFFF, PhysXFlat::ParentColor);
    }

    TEST_F(PhysXFlatSheetTests, Pixels_TheLinksMenuIsFilledAsItsSheetFilledIt)
    {
        // The link's `background-color` outranked the application sheet's `QMenu` rule (#222222) on the menu it parents
        QColor legacy;
        QColor flat;
        withLegacyLinkMenu([&legacy](QMenu& menu) { legacy = menuSurface(menu); });
        withFlatLinkMenu([&flat](QMenu& menu) { flat = menuSurface(menu); });

        EXPECT_TRUE(flat == legacy) << "legacy " << legacy.name().toStdString() << ", flat " << flat.name().toStdString();
        EXPECT_TRUE(flat == QColor(0x33, 0x33, 0x33)) << "flat " << flat.name().toStdString();
    }

    // ----------------------------------------------------------------------------------
    // Box: where the contents are laid out
    // ----------------------------------------------------------------------------------

    TEST_F(PhysXFlatSheetTests, Box_TheLinksFrameContentsAreThoseOfTheLabelRuleUnderItsSheet)
    {
        // The margin above and below, which also switches on QLabel's automatic indent of half an 'x'
        QRect legacy;
        QRect flat;
        withLegacyLink([&legacy](QLabel& label) { legacy = frameContents(label); });
        withFlatLink([&flat](QLabel& label) { flat = frameContents(label); });

        EXPECT_EQ(flat, legacy);
        EXPECT_EQ(flat, QRect(0, 1, 200, 31));          // the link is 200 x 33
    }

    TEST_F(PhysXFlatSheetTests, Box_TheLinksContentsRectAndSizeHintAreTheLegacys)
    {
        // The link's contents margins (60, 7) plus the frame box the label rule gave it (0, 1): the text is laid out in (60, 8, 80, 17)
        QRect legacyContents;
        QRect flatContents;
        QSize legacyHint;
        QSize flatHint;
        QSize legacyMinimumHint;
        QSize flatMinimumHint;
        withLegacyLink(
            [&](QLabel& label)
            {
                legacyContents = label.contentsRect();
                legacyHint = label.sizeHint();
                legacyMinimumHint = label.minimumSizeHint();
            });
        withFlatLink(
            [&](QLabel& label)
            {
                flatContents = label.contentsRect();
                flatHint = label.sizeHint();
                flatMinimumHint = label.minimumSizeHint();
            });

        EXPECT_EQ(flatContents, legacyContents);
        EXPECT_EQ(flatContents, QRect(60, 8, 80, 17));
        EXPECT_EQ(flatHint, legacyHint);
        EXPECT_EQ(flatMinimumHint, legacyMinimumHint);
    }

    // ----------------------------------------------------------------------------------
    // Text: the font the link's text is drawn with. The link is a leaf with no children to hand a palette to, and a label reads none of
    // the roles the sheet's fill set (Window, Base and Button): there is no palette role to twin.
    // ----------------------------------------------------------------------------------

    TEST_F(PhysXFlatSheetTests, Text_TheLinkHasTheFontTheApplicationSheetGaveALabel)
    {
        // `* { font-family; font-weight }` and `QLabel { font-family; font-size }` (BaseStyleSheet.qss:15-21, Text.qss:89-95); the
        // element sets none, the class font of the label does
        QFont legacy;
        QFont flat;
        withLegacyLink([&legacy](QLabel& label) { legacy = label.font(); });
        withFlatLink([&flat](QLabel& label) { flat = label.font(); });

        EXPECT_EQ(flat.family(), legacy.family());
        EXPECT_EQ(flat.pixelSize(), legacy.pixelSize());
        EXPECT_EQ(flat.weight(), legacy.weight());
        EXPECT_EQ(QFontInfo(flat).pixelSize(), QFontInfo(legacy).pixelSize());
        EXPECT_EQ(QFontInfo(flat).weight(), QFontInfo(legacy).weight());
        EXPECT_EQ(flat.pixelSize(), 12);
        EXPECT_EQ(flat.weight(), static_cast<int>(QFont::Normal));
    }
} // namespace UnitTest
