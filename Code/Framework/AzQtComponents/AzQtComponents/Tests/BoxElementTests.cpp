/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzTest/AzTest.h>
#include <AzQtComponents/Components/Elements/BoxElement.h>
#include <AzQtComponents/Components/Style.h>
#include <AzQtComponents/Components/StyleManager.h>

#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>

#include <QApplication>
#include <QFrame>
#include <QHeaderView>
#include <QImage>
#include <QLabel>
#include <QMainWindow>
#include <QMdiSubWindow>
#include <QMenu>
#include <QMenuBar>
#include <QPainter>
#include <QPushButton>
#include <QScrollArea>
#include <QStyleFactory>
#include <QStyleOption>
#include <QTabBar>
#include <QTemporaryDir>
#include <QVBoxLayout>

namespace AzQtComponents
{
    // ----------------------------------------------------------------------------------
    // The elements under test, written as a module writes them, each beside the legacy rules it carries (ProjectManager.qss,
    // AssetProcessor.qss, Text.qss, PushButton.qss). The legacy twin of a widget is Qt's own style sheet style over Fusion with
    // those rules; the flat twin is the flattened Style with the element. Same box, same hints, same pixels.
    // ----------------------------------------------------------------------------------

    namespace BoxElementTestData
    {
        QColor rgb(int value)
        {
            return QColor((value >> 16) & 0xFF, (value >> 8) & 0xFF, value & 0xFF);
        }

        QBrush solid(int value)
        {
            return QBrush(rgb(value));
        }

        //! The flat twin is the window that carries the property; the legacy twin carries the sheet instead
        bool named(const QWidget* widget, const char* objectName)
        {
            return widget->window()->property("boxElementTestsFlat").toBool() && widget->objectName() == QLatin1String(objectName);
        }

        bool tagged(const QWidget* widget, const char* property)
        {
            return widget->window()->property("boxElementTestsFlat").toBool() && widget->property(property).toString() == QLatin1String("true");
        }

        //! Two images for the elements and the sheets: files of a temporary directory, each with a mark in its corner
        struct Images
        {
            QTemporaryDir m_directory;
            QByteArray m_plain;
            QByteArray m_hover;
        };

        Images& images()
        {
            static Images shared;
            if (shared.m_plain.isEmpty())
            {
                for (const bool hover : { false, true })
                {
                    QImage image(14, 10, QImage::Format_ARGB32);
                    image.fill(hover ? rgb(0x1E70EB) : rgb(0xFF8000));
                    QPainter painter(&image);
                    painter.fillRect(0, 0, 4, 4, rgb(0xFFFFFF));
                    painter.end();
                    const QString path = shared.m_directory.filePath(hover ? QStringLiteral("hover.png") : QStringLiteral("plain.png"));
                    image.save(path, "PNG");
                    (hover ? shared.m_hover : shared.m_plain) = path.toUtf8();
                }
            }
            return shared;
        }

        QString url(const QByteArray& path)
        {
            return QStringLiteral("url(%1)").arg(QString::fromUtf8(path));
        }

        // Text.qss:89-95 and PushButton.qss:48-52, the global rules the elements complete
        const char* const GlobalRules =
            "QLabel { margin-top: 1px; margin-bottom: 1px; }"
            "QPushButton:flat { border: none; background-color: transparent; }";

        // ---- ProjectManager.qss:2-8. The header band: a window's fill and an image, placed in the content box by the padding
        BoxElement HeaderBand{
            .m_name = "Test.Box.HeaderBand",
            .m_padding = QMargins(16, 25, 16, 25),                                  // padding: 25px 16px (:6)
            .m_imageOrigin = BoxElement::Origin::Content,                           // background-origin: content (:5)
        };                                                                          // the fill and the image (:3) are set with the image files
        QString headerBandRules()
        {
            return QStringLiteral("QMainWindow { background: #131313 %1 no-repeat top left; background-origin: content; padding: 25px 16px; margin: 0; }")
                .arg(url(images().m_plain));
        }

        // ---- ProjectManager.qss:208-216, 261-263. A form frame: a bordered, rounded card inset in its row; another border disabled
        const char* const FormFrameRules =
            "#formFrame { max-width: 840px; background-color: #444444; border: 1px solid #dddddd; border-radius: 4px;"
            " padding: 0px 10px 2px 6px; margin-top: 10px; margin-left: 50px; }"
            "#formFrame:disabled { border: 1px solid #959595; }";
        const BoxElement FormFrame{
            .m_name = "Test.Box.FormFrame",
            .m_margin = QMargins(50, 10, 0, 0),                                     // margin-left (:215), margin-top (:214)
            .m_padding = QMargins(6, 0, 10, 2),                                     // padding (:213)
            .m_nativeBorder = false,                                                // border (:211)
            .m_radius = 4,                                                          // border-radius (:212)
            .m_maxSize = QSize(840, -1),                                            // max-width (:209)
            .m_idle = { .m_fill = solid(0x444444), .m_edges = BoxElement::border(1, rgb(0xDDDDDD)) },      // :210-211
            .m_stateFaces = { { BoxElement::Disabled, { .m_fill = solid(0x444444), .m_edges = BoxElement::border(1, rgb(0x959595)) } } }, // :261-263
        };

        // ---- ProjectManager.qss:681-684 over BaseStyleSheet.qss:15, :536-541 over Text.qss:89-95, :686-694 and :709-711. A scroll area
        // with a margin, transparent over the style's own frame; a label with a line under it, and the same with no fill, which draws
        // nothing at all; a label with an outline that turns blue under the pointer. Not from a sheet: frames with the style's own
        // border, a corner that does not fit, an image in the padding box.
        const char* const FrameRules =
            "#projectGrid { background-color: transparent; margin-top: 40px; margin-bottom: 5px; }"
            "#summary { background-color: transparent; padding-bottom: 0px; border-bottom: 2px solid #555555; min-height: 80px; }"
            "#summaryNoFill { padding-bottom: 0px; border-bottom: 2px solid #555555; min-height: 80px; }"
            "#outline { border: 1px solid white; margin: 0; qproperty-indent: 0; }"
            "#outline:hover { border: 1px solid #1e70eb; }"
            "#nativePanel { margin: 6px; background-color: #444444; }"
            "#boxFrame { background-color: #333333; }"
            "#tooRound { background-color: #336699; border: 2px solid #cc9933; border-radius: 40px; }";
        const BoxElement ProjectGrid{
            .m_name = "Test.Box.ProjectGrid",
            .m_margin = QMargins(0, 40, 0, 5),                                      // :682-683
            .m_idle = { .m_fill = QBrush(Qt::transparent) },                        // BaseStyleSheet.qss:15
        };
        const BoxElement Summary{
            .m_name = "Test.Box.Summary",
            .m_margin = QMargins(0, 1, 0, 1),                                       // Text.qss:89-95
            .m_minSize = QSize(-1, 80),                                             // min-height (:539)
            .m_idle = { .m_fill = QBrush(Qt::transparent), .m_edges = { {}, {}, { 2, rgb(0x555555) }, {} } },  // border-bottom (:538)
        };
        const BoxElement SummaryNoFill{
            .m_name = "Test.Box.SummaryNoFill",
            .m_margin = QMargins(0, 1, 0, 1),
            .m_minSize = QSize(-1, 80),
            .m_idle = { .m_edges = { {}, {}, { 2, rgb(0x555555) }, {} } },
        };
        const BoxElement Outline{
            .m_name = "Test.Box.Outline",
            .m_nativeBorder = false,                                                // border (:687)
            .m_idle = { .m_edges = BoxElement::border(1, rgb(0xFFFFFF)) },
            .m_stateFaces = { { BoxElement::Hover, { .m_edges = BoxElement::border(1, rgb(0x1E70EB)) } } },    // :690-694
            .m_properties = { { "indent", 0 } },                                    // qproperty-indent (:710); margin: 0 (:711)
        };
        const BoxElement NativePanel{ .m_name = "Test.Box.NativePanel", .m_margin = QMargins(6, 6, 6, 6), .m_idle = { .m_fill = solid(0x444444) } };
        const BoxElement BoxFrame{ .m_name = "Test.Box.BoxFrame", .m_idle = { .m_fill = solid(0x333333) } };
        const BoxElement TooRound{
            .m_name = "Test.Box.TooRound",
            .m_nativeBorder = false,
            .m_radius = 40,
            .m_idle = { .m_fill = solid(0x336699), .m_edges = BoxElement::border(2, rgb(0xCC9933)) },
        };
        BoxElement ImagePadding{
            .m_name = "Test.Box.ImagePadding",
            .m_padding = QMargins(20, 10, 4, 6),
            .m_nativeBorder = false,
            .m_idle = { .m_fill = solid(0x202020), .m_edges = BoxElement::border(3, rgb(0x3366CC)) },
            .m_imageAlign = Qt::AlignRight | Qt::AlignBottom,
        };
        QString imagePaddingRules()
        {
            return QStringLiteral("#imagePadding { padding: 10px 4px 6px 20px; background: #202020 %1 no-repeat right bottom; border: 3px solid #3366cc; }")
                .arg(url(images().m_plain));
        }

        // ---- ProjectManager.qss:15-59. Push buttons: a flat button whose face is a vertical gradient in each state
        BoxElement gradientButton(const char* name, int top, int bottom, int hoverTop, int hoverBottom, int pressedTop, int pressedBottom)
        {
            BoxElement element;
            element.m_name = name;
            element.m_idle.m_fill = BoxElement::vertical(rgb(top), rgb(bottom));
            element.m_stateFaces[0] = { BoxElement::Hover, { .m_fill = BoxElement::vertical(rgb(hoverTop), rgb(hoverBottom)) } };
            element.m_stateFaces[1] = { BoxElement::Pressed, { .m_fill = BoxElement::vertical(rgb(pressedTop), rgb(pressedBottom)) } };
            element.m_properties[0] = { "flat", true };                             // qproperty-flat (:16, :34)
            return element;
        }
        QString gradientRules(const char* kind, int top, int bottom, int hoverTop, int hoverBottom, int pressedTop, int pressedBottom)
        {
            const auto gradient = [](int from, int to)
            {
                return QStringLiteral("background: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1, stop: 0 #%1, stop: 1.0 #%2);")
                    .arg(from, 6, 16, QLatin1Char('0')).arg(to, 6, 16, QLatin1Char('0'));
            };
            return QStringLiteral("QPushButton[%1='true'] { qproperty-flat: true; %2 }"
                                  "QPushButton[%1='true']:hover { %3 }"
                                  "QPushButton[%1='true']:pressed { %4 }")
                .arg(QString::fromLatin1(kind), gradient(top, bottom), gradient(hoverTop, hoverBottom), gradient(pressedTop, pressedBottom));
        }
        const QString PrimaryRules = gradientRules("primary", 0x0095F2, 0x1E70EB, 0x10A5F2, 0x2E80EB, 0x0085E2, 0x0E60DB);    // :17-26
        const QString SecondaryRules = gradientRules("secondary", 0x888888, 0x555555, 0x999999, 0x666666, 0x555555, 0x777777); // :35-44
        const BoxElement Primary = gradientButton("Test.Box.Button.Primary", 0x0095F2, 0x1E70EB, 0x10A5F2, 0x2E80EB, 0x0085E2, 0x0E60DB);
        const BoxElement Secondary = gradientButton("Test.Box.Button.Secondary", 0x888888, 0x555555, 0x999999, 0x666666, 0x555555, 0x777777);

        // ProjectManager.qss:466-473. A footer button: a radius, a margin on the right, a fixed height; and with the secondary face
        // in one definition, for a secondary button in a footer
        const char* const FooterRules = "#footerButton { border-radius: 3px; min-height: 28px; max-height: 28px; min-width: 150px; margin-right: 30px; }";
        BoxElement footerButton(BoxElement element, const char* name)
        {
            element.m_name = name;
            element.m_margin = QMargins(0, 0, 30, 0);                               // margin-right (:471)
            element.m_nativeBorder = false;                                         // border-radius (:467) cancels the native border
            element.m_radius = 3;
            element.m_minSize = QSize(150, 28);                                     // min-width (:470), min-height (:468)
            element.m_maxSize = QSize(-1, 28);                                      // max-height (:469)
            return element;
        }
        const BoxElement Footer = footerButton(BoxElement(), "Test.Box.Button.Footer");
        const BoxElement FooterSecondary = footerButton(Secondary, "Test.Box.Button.Footer.Secondary");

        // ProjectManager.qss:408-427. The wizard's back button: a flat button with an image, another under the pointer, a margin, a box of 24x24
        BoxElement Back{
            .m_name = "Test.Box.Button.Back",
            .m_margin = QMargins(10, 20, 10, 0),                                    // margin (:414)
            .m_minSize = QSize(24, 24),                                             // :410-413
            .m_maxSize = QSize(24, 24),
            .m_imageAlign = Qt::AlignCenter,                                        // no-repeat center (:415)
            .m_imageOrigin = BoxElement::Origin::Content,                           // background-origin (:416)
            .m_properties = { { "flat", true }, { "iconSize", QSize(50, 50) } },    // qproperty-flat (:417), -iconSize (:418)
        };
        QString backRules()
        {
            return QStringLiteral("#backButton { background: transparent %1 no-repeat center; background-origin: content;"
                                  " margin: 20px 10px 0px 10px; max-height: 24px; max-width: 24px; min-height: 24px; min-width: 24px;"
                                  " qproperty-flat: true; qproperty-iconSize: 50px; }"
                                  "#backButton:hover, #backButton:pressed { background: transparent %2 no-repeat center; }")
                .arg(url(images().m_plain), url(images().m_hover));
        }

        // ProjectManager.qss:623-646. A start card: a flat button with a border and a label colour that change with the state. Hovered
        // and focused, both rules apply: the colour of the first, the border of both, an entry of its own
        const char* const StartCardRules =
            "#startCard { min-width: 210px; max-width: 210px; min-height: 276px; max-height: 276px; qproperty-flat: true;"
            " border: 1px solid #ffffff; background: rgba(0,0,0,180); }"
            "#startCard:hover { border: 1px solid #1e70eb; color: #1e70eb; }"
            "#startCard:focus { border: 1px solid #1e70eb; }"
            "#startCard:pressed { border: 1px solid #0e60eb; color: #0e60eb; }";
        BoxElement::Face startCardFace(int border, int text = -1)
        {
            return { .m_fill = QBrush(QColor(0, 0, 0, 180)), .m_edges = BoxElement::border(1, rgb(border)), .m_text = text < 0 ? QColor() : rgb(text) };
        }
        const BoxElement StartCard{
            .m_name = "Test.Box.Button.StartCard",
            .m_nativeBorder = false,                                                // border (:631)
            .m_minSize = QSize(210, 276),                                           // :624-627
            .m_maxSize = QSize(210, 276),
            .m_idle = startCardFace(0xFFFFFF),
            .m_stateFaces = {
                { BoxElement::Hover, startCardFace(0x1E70EB, 0x1E70EB) },                           // :634-637
                { BoxElement::Focus, startCardFace(0x1E70EB) },                                     // :639-641
                { BoxElement::Hover | BoxElement::Focus, startCardFace(0x1E70EB, 0x1E70EB) },
                { BoxElement::Pressed, startCardFace(0x0E60EB, 0x0E60EB) },                         // :643-646
            },
            .m_properties = { { "flat", true } },                                   // qproperty-flat (:628)
        };

        // ProjectManager.qss:765-770, AssetBundler.qss:78-79. Buttons with a box and nothing else, not flat: the style's own border is
        // the box's, and the limits count it
        const char* const PlainButtonRules =
            "#newProjectButton { min-width: 150px; max-width: 150px; min-height: 26px; max-height: 26px; }"
            "#insetButton { margin-left: 10px; margin-right: 10px; }";
        const BoxElement NewProject{ .m_name = "Test.Box.Button.NewProject", .m_minSize = QSize(150, 26), .m_maxSize = QSize(150, 26) };   // :766-769
        const BoxElement Inset{ .m_name = "Test.Box.Button.Inset", .m_margin = QMargins(10, 0, 10, 0) };

        // The registry tests: elements told apart by object name and, for a switching widget, a property
        const BoxElement WideA{ .m_name = "Test.Box.WideA", .m_margin = QMargins(7, 0, 0, 0), .m_minSize = QSize(10, -1) };
        const BoxElement WideB{ .m_name = "Test.Box.WideB", .m_margin = QMargins(3, 0, 0, 0), .m_minSize = QSize(20, -1) };
        const BoxElement TallC{ .m_name = "Test.Box.TallC", .m_margin = QMargins(0, 5, 0, 0), .m_maxSize = QSize(-1, 30) };
        bool isSwitching(const QWidget* widget, int variant)
        {
            return named(widget, "switching") && widget->property("variant").toInt() == variant;
        }

        // A margin written as 0 still hands the box to the rule (QRenderRule::hasBox): a Box frame of line width 3 has the native 1 for
        // its contents, a button the box for its size
        const char* const ZeroBoxRules =
            "#zeroBoxFrame { margin: 0; background-color: #333333; }"
            "#zeroBoxButton { margin: 0; background-color: #333333; }";
        const BoxElement ZeroBoxFrame{ .m_name = "Test.Box.ZeroBoxFrame", .m_hasBox = true, .m_idle = { .m_fill = solid(0x333333) } };
        const BoxElement ZeroBoxButton{ .m_name = "Test.Box.Button.ZeroBox", .m_hasBox = true, .m_idle = { .m_fill = solid(0x333333) } };

        // The classes besides frames and plain widgets whose own background the sheet paints (qstylesheetstyle.cpp:2926-2948)
        const char* const BarRules =
            "#tabBar { background-color: #444444; } #menuBar { background-color: #444444; }"
            "#headerView { background-color: #444444; } #subWindow { background-color: #444444; }";
        const BoxElement BarFill{ .m_name = "Test.Box.BarFill", .m_idle = { .m_fill = solid(0x444444) } };

        // The scope tests: a Global element registered before the module's
        const BoxElement GlobalFrame{ .m_name = "Test.Box.GlobalFrame", .m_margin = QMargins(1, 0, 0, 0) };
        const BoxElement GlobalFrameLater{ .m_name = "Test.Box.GlobalFrameLater", .m_margin = QMargins(3, 0, 0, 0) };
        const BoxElement ModuleFrame{ .m_name = "Test.Box.ModuleFrame", .m_margin = QMargins(2, 0, 0, 0) };
        const BoxElement ModuleWidget{ .m_name = "Test.Box.ModuleWidget", .m_margin = QMargins(4, 0, 0, 0) };

        // ProjectManager.qss:623-646 as the sheet cascades them for the start page buttons: hover and pressed change the image, the
        // border colour and the label colour, focus only the border colour; the hover rules come first, then focus, then pressed
        QString realCardRules(const char* name)
        {
            return QStringLiteral("#%1 { qproperty-flat: true; min-width: 210px; max-width: 210px; min-height: 276px; max-height: 276px;"
                                  " border: 1px solid #ffffff; background: rgba(0,0,0,180) %2 no-repeat center; background-origin: content; }"
                                  "#%1:hover { background: rgba(0,0,0,180) %3 no-repeat center; }"
                                  "#%1:pressed { background: rgba(0,0,0,180) %3 no-repeat center; }"
                                  "#%1:hover { border: 1px solid #1e70eb; color: #1e70eb; }"
                                  "#%1:focus { border: 1px solid #1e70eb; }"
                                  "#%1:pressed { border: 1px solid #0e60eb; color: #0e60eb; }")
                .arg(QString::fromLatin1(name), url(images().m_plain), url(images().m_hover));
        }
        BoxElement realCard(const char* name)
        {
            BoxElement element;
            element.m_name = name;
            element.m_nativeBorder = false;
            element.m_minSize = QSize(210, 276);
            element.m_maxSize = QSize(210, 276);
            element.m_imageAlign = Qt::AlignCenter;
            element.m_imageOrigin = BoxElement::Origin::Content;
            element.m_properties[0] = { "flat", true };
            return element;
        }
        BoxElement::Face cardFace(const QByteArray& image, int border, int text = -1)
        {
            return { .m_fill = QBrush(QColor(0, 0, 0, 180)), .m_edges = BoxElement::border(1, rgb(border)), .m_text = text < 0 ? QColor() : rgb(text),
                     .m_image = image.constData() };
        }
        BoxElement RealCard = realCard("Test.Box.Button.RealCard");
        BoxElement DraftedCard = realCard("Test.Box.Button.DraftedCard");
    } // namespace BoxElementTestData

    // The tests and their fixture sit in a namespace of their own: the using-directive below must not leak into the
    // other files of a unity build
    namespace BoxElementTestsInternal
    {
        using namespace BoxElementTestData;

        // ----------------------------------------------------------------------------------
        // Fixture: the flattened Style over Fusion, Fusion as the base of the legacy sheet, the elements registered
        // ----------------------------------------------------------------------------------

        class BoxElementTests
            : public ::UnitTest::LeakDetectionFixture
        {
        public:
            BoxElementTests()
            {
                m_application = AZStd::make_unique<QApplication>(m_argc, m_argv);
                QApplication::setStyle(QStyleFactory::create(QStringLiteral("Fusion")));
                m_style = AZStd::make_unique<Style>(QStyleFactory::create(QStringLiteral("Fusion")));
                registerElements();
            }

            ~BoxElementTests() override
            {
                m_style.reset();
                m_application.reset();
            }

            //! Registered once for the process, as a module registers its own: the first registered match wins
            static void registerElements()
            {
                static bool registered = false;
                if (registered)
                {
                    return;
                }
                registered = true;

                // The image files exist now: the elements that show them name them
                HeaderBand.m_idle = { .m_fill = solid(0x131313), .m_image = images().m_plain.constData() };                 // :3
                Back.m_idle = { .m_image = images().m_plain.constData() };                                                    // :415
                Back.m_stateFaces[0] = { BoxElement::Hover, { .m_image = images().m_hover.constData() } };                  // :424-427
                Back.m_stateFaces[1] = { BoxElement::Pressed, { .m_image = images().m_hover.constData() } };
                ImagePadding.m_idle.m_image = images().m_plain.constData();

                const QMetaObject* frame = &QFrame::staticMetaObject;
                const QMetaObject* label = &QLabel::staticMetaObject;
                const QMetaObject* button = &QPushButton::staticMetaObject;
                BoxElement::registerElement(&HeaderBand, &QMainWindow::staticMetaObject, [](const QWidget* w) { return named(w, "headerBand"); });
                BoxElement::registerElement(&FormFrame, frame, [](const QWidget* w) { return named(w, "formFrame"); });
                BoxElement::registerElement(&ProjectGrid, &QScrollArea::staticMetaObject, [](const QWidget* w) { return named(w, "projectGrid"); });
                BoxElement::registerElement(&Summary, label, [](const QWidget* w) { return named(w, "summary"); });
                BoxElement::registerElement(&SummaryNoFill, label, [](const QWidget* w) { return named(w, "summaryNoFill"); });
                BoxElement::registerElement(&Outline, label, [](const QWidget* w) { return named(w, "outline"); });
                BoxElement::registerElement(&NativePanel, frame, [](const QWidget* w) { return named(w, "nativePanel"); });
                BoxElement::registerElement(&BoxFrame, frame, [](const QWidget* w) { return named(w, "boxFrame"); });
                BoxElement::registerElement(&TooRound, frame, [](const QWidget* w) { return named(w, "tooRound"); });
                BoxElement::registerElement(&ImagePadding, frame, [](const QWidget* w) { return named(w, "imagePadding"); });
                BoxElement::registerElement(&FooterSecondary, button, [](const QWidget* w) { return named(w, "footerButton") && tagged(w, "secondary"); });
                BoxElement::registerElement(&Primary, button, [](const QWidget* w) { return tagged(w, "primary"); });
                BoxElement::registerElement(&Secondary, button, [](const QWidget* w) { return tagged(w, "secondary"); });
                BoxElement::registerElement(&Footer, button, [](const QWidget* w) { return named(w, "footerButton"); });
                BoxElement::registerElement(&Back, button, [](const QWidget* w) { return named(w, "backButton"); });
                BoxElement::registerElement(&StartCard, button, [](const QWidget* w) { return named(w, "startCard"); });
                BoxElement::registerElement(&NewProject, button, [](const QWidget* w) { return named(w, "newProjectButton"); });
                BoxElement::registerElement(&Inset, button, [](const QWidget* w) { return named(w, "insetButton"); });
                BoxElement::registerElement(&ZeroBoxFrame, frame, [](const QWidget* w) { return named(w, "zeroBoxFrame"); });
                BoxElement::registerElement(&ZeroBoxButton, button, [](const QWidget* w) { return named(w, "zeroBoxButton"); });
                BoxElement::registerElement(&BarFill, &QWidget::staticMetaObject,
                    [](const QWidget* w) { return named(w, "tabBar") || named(w, "menuBar") || named(w, "headerView") || named(w, "subWindow"); });

                // The faces of the start page buttons. The entries of RealCard are in the order the rules cascade (the focus border is the
                // hover border, so a hovered button that has the focus shows the hover face, image and colour included); those of
                // DraftedCard are the same faces in a fixed hover, pressed, focus order
                const BoxElement::Face idle = cardFace(images().m_plain, 0xFFFFFF);
                const BoxElement::Face hover = cardFace(images().m_hover, 0x1E70EB, 0x1E70EB);
                const BoxElement::Face pressed = cardFace(images().m_hover, 0x0E60EB, 0x0E60EB);
                const BoxElement::Face focus = cardFace(images().m_plain, 0x1E70EB);
                RealCard.m_idle = idle;
                RealCard.m_stateFaces[0] = { BoxElement::Focus, focus };
                RealCard.m_stateFaces[1] = { BoxElement::Hover, hover };
                RealCard.m_stateFaces[2] = { BoxElement::Pressed, pressed };
                DraftedCard.m_idle = idle;
                DraftedCard.m_stateFaces[0] = { BoxElement::Hover, hover };
                DraftedCard.m_stateFaces[1] = { BoxElement::Pressed, pressed };
                DraftedCard.m_stateFaces[2] = { BoxElement::Focus, focus };
                BoxElement::registerElement(&RealCard, button, [](const QWidget* w) { return named(w, "realCard"); });
                BoxElement::registerElement(&DraftedCard, button, [](const QWidget* w) { return named(w, "draftedCard"); });
            }

            //! The legacy twin: the sheet on the root, over the application's Fusion. The flat twin: the style on the root and on
            //! everything under it (a widget's style is not its children's).
            static void legacy(QWidget& root, const QString& rules)
            {
                root.setStyleSheet(QString::fromUtf8(GlobalRules) + rules);
            }

            void flat(QWidget& root) const
            {
                root.setProperty("boxElementTestsFlat", true);
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

            //! A widget of a twin tree, named as the sheet and the matchers name it
            template<typename W>
            static W* add(QWidget& root, const char* name, const QRect& geometry = QRect())
            {
                W* widget = new W(&root);
                widget->setObjectName(QString::fromLatin1(name));
                widget->setGeometry(geometry);
                if (auto* label = qobject_cast<QLabel*>(widget))
                {
                    label->setText(QStringLiteral("Text"));
                }
                return widget;
            }

            //! One tree twice, shown: the build function fills a root with it, the rules are the legacy twin's sheet
            template<typename Root>
            struct Twins
            {
                template<typename Build>
                Twins(const BoxElementTests& tests, Build build, const QString& rules, const QSize& size)
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

            //! The widget as it paints itself and its children, without the window background Qt would add
            static QImage paint(QWidget& root, qreal ratio)
            {
                QImage image((QSizeF(root.size()) * ratio).toSize(), QImage::Format_ARGB32_Premultiplied);
                image.setDevicePixelRatio(ratio);
                image.fill(Qt::transparent);
                root.render(&image, QPoint(), QRegion(), QWidget::DrawChildren);
                return image;
            }

            static int differing(const QImage& a, const QImage& b)
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

            //! The box Qt derived from the element is the one it derived from the sheet, and the twins paint the same pixels, at the
            //! device ratio of one and of the 1.5 the captures were taken at
            template<typename Twin>
            static void expectSame(Twin& twins, std::initializer_list<const char*> names)
            {
                for (const char* name : names)
                {
                    const QWidget* legacyWidget = twins.template legacyChild<QWidget>(name);
                    const QWidget* flatWidget = twins.template flatChild<QWidget>(name);
                    EXPECT_EQ(flatWidget->geometry(), legacyWidget->geometry()) << name;
                    EXPECT_TRUE(flatWidget->contentsMargins() == legacyWidget->contentsMargins()) << name;
                    EXPECT_EQ(flatWidget->minimumSize(), legacyWidget->minimumSize()) << name;
                    EXPECT_EQ(flatWidget->maximumSize(), legacyWidget->maximumSize()) << name;
                    EXPECT_EQ(flatWidget->sizeHint(), legacyWidget->sizeHint()) << name;
                    EXPECT_TRUE(BoxElement::forWidget(flatWidget) != nullptr && BoxElement::forWidget(legacyWidget) == nullptr) << name;
                }
                for (const qreal ratio : { 1.0, 1.5 })
                {
                    EXPECT_EQ(differing(paint(twins.m_legacy, ratio), paint(twins.m_flat, ratio)), 0) << *names.begin() << " at ratio " << ratio;
                }
            }

            //! One push button twice, polished: the sheet's selectors are the button's object name here; a primary or a secondary
            //! button carries the property the sheet selects on
            struct ButtonTwins
            {
                ButtonTwins(const BoxElementTests& tests, const char* name, const char* text, const QString& rules, const char* kind = nullptr)
                {
                    for (QWidget* root : { &m_legacyRoot, &m_flatRoot })
                    {
                        QPushButton* button = add<QPushButton>(*root, name);
                        button->setText(QString::fromLatin1(text));
                        (new QVBoxLayout(root))->addWidget(button);
                        if (kind)
                        {
                            button->setProperty(kind, QStringLiteral("true"));
                        }
                        (root == &m_legacyRoot ? m_legacy : m_flat) = button;
                    }
                    // Native polished a button with the components first (PushButton::polish gives it a maximum height) and applied the
                    // sheet's geometry over that: the legacy twin has what a plain button has
                    QPushButton plain;
                    plain.setStyle(tests.m_style.get());
                    plain.ensurePolished();
                    m_legacy->setMaximumHeight(plain.maximumHeight());
                    legacy(m_legacyRoot, rules);
                    tests.flat(m_flatRoot);
                    m_legacyRoot.ensurePolished();
                    m_flatRoot.ensurePolished();
                }

                //! The button as its style draws it for a state (CE_PushButton: bevel and label), at a device ratio
                static QImage draw(const QPushButton* button, QStyle::State state, const QRect& rect, qreal ratio)
                {
                    QStyleOptionButton option;
                    option.initFrom(button);
                    option.state = state;
                    option.features = button->isFlat() ? QStyleOptionButton::Flat : QStyleOptionButton::None;
                    if (button->menu())
                    {
                        option.features |= QStyleOptionButton::HasMenu;
                    }
                    option.rect = rect;
                    option.text = button->text();
                    QImage image((QSizeF(rect.size()) * ratio).toSize(), QImage::Format_ARGB32_Premultiplied);
                    image.setDevicePixelRatio(ratio);
                    image.fill(rgb(0x808080));
                    QPainter painter(&image);
                    button->style()->drawControl(QStyle::CE_PushButton, &option, &painter, button);
                    return image;
                }

                //! The twins draw the same pixels in each of the states, at both device ratios
                void expectSameDraw(const QRect& rect, std::initializer_list<QStyle::State> states, const char* what) const
                {
                    for (const qreal ratio : { 1.0, 1.5 })
                    {
                        for (const QStyle::State state : states)
                        {
                            EXPECT_EQ(differing(draw(m_legacy, state, rect, ratio), draw(m_flat, state, rect, ratio)), 0)
                                << what << " in state " << int(state) << " at ratio " << ratio;
                        }
                    }
                }

                QWidget m_legacyRoot;
                QWidget m_flatRoot;
                QPushButton* m_legacy = nullptr;
                QPushButton* m_flat = nullptr;
            };

        protected:
            int m_argc = 1;                                     // QApplication keeps a reference to it
            char m_programName[5] = "test";
            char* m_argv[2] = { m_programName, nullptr };
            AZStd::unique_ptr<QApplication> m_application;
            AZStd::unique_ptr<Style> m_style;
        };

        // ----------------------------------------------------------------------------------
        // T1: frames and labels: the box reaches Qt as the contents rect, and Qt derives the rest
        // ----------------------------------------------------------------------------------

        TEST_F(BoxElementTests, FormFrame_BoxIsMarginBorderPaddingAndTheFillBorderAndRadiusPaintLikeTheSheet)
        {
            ASSERT_TRUE(StyleManager::stylesheetsDisabled());   // the test environment sets it (AzQtComponentTests.cpp)

            for (const bool enabled : { true, false })
            {
                Twins<QWidget> twins(*this, [&](QWidget& root) { add<QFrame>(root, "formFrame", QRect(0, 0, 400, 80))->setEnabled(enabled); },
                    QString::fromUtf8(FormFrameRules), QSize(400, 80));
                const QFrame* legacyFrame = twins.legacyChild<QFrame>("formFrame");
                const QFrame* flatFrame = twins.flatChild<QFrame>("formFrame");

                // The Native numbers: contents [57, 11, 11, 3] (margin + border + padding), maximum 908 = 840 + 50 + 2 + 16
                EXPECT_TRUE(legacyFrame->contentsMargins() == QMargins(57, 11, 11, 3));
                EXPECT_EQ(legacyFrame->maximumWidth(), 908);
                expectSame(twins, { "formFrame" });
                EXPECT_EQ(flatFrame->frameWidth(), legacyFrame->frameWidth());
                EXPECT_TRUE(flatFrame->testAttribute(Qt::WA_StyledBackground));

                // The margin is left unpainted, the border is the first column of the border box, the fill is inside it
                const QImage image = paint(twins.m_flat, 1.0);
                EXPECT_EQ(image.pixelColor(49, 40).alpha(), 0);
                EXPECT_TRUE(image.pixelColor(50, 40).rgb() == (enabled ? rgb(0xDDDDDD) : rgb(0x959595)).rgb());
                EXPECT_TRUE(image.pixelColor(60, 40).rgb() == rgb(0x444444).rgb());
            }
        }

        TEST_F(BoxElementTests, HeaderBand_FillsTheWindowAndPlacesTheImageInTheContentBoxWithoutInsettingTheLayout)
        {
            Twins<QMainWindow> twins(*this, [](QMainWindow& window)
            {
                window.setObjectName(QStringLiteral("headerBand"));        // the sheet's selector is the class, the matcher's the name
                window.setCentralWidget(new QWidget);
            }, headerBandRules(), QSize(300, 200));

            // The padding positions the image and nothing else: the central widget keeps the window's rect
            EXPECT_EQ(twins.m_flat.centralWidget()->geometry(), twins.m_legacy.centralWidget()->geometry());
            EXPECT_TRUE(twins.m_flat.contentsMargins() == twins.m_legacy.contentsMargins());
            EXPECT_TRUE(twins.m_flat.testAttribute(Qt::WA_StyledBackground));
            for (const qreal ratio : { 1.0, 1.5 })
            {
                EXPECT_EQ(differing(paint(twins.m_legacy, ratio), paint(twins.m_flat, ratio)), 0) << "header band at ratio " << ratio;
            }

            // The fill, and the image's corner mark at the content box's top left: the window origin + (16, 25)
            const QImage image = paint(twins.m_flat, 1.0);
            EXPECT_TRUE(image.pixelColor(5, 5).rgb() == rgb(0x131313).rgb());
            EXPECT_TRUE(image.pixelColor(16, 25).rgb() == rgb(0xFFFFFF).rgb());
            EXPECT_TRUE(image.pixelColor(15, 25).rgb() == rgb(0x131313).rgb());
        }

        TEST_F(BoxElementTests, Frames_AScrollAreaLabelsFramePanelsCornersAndAnImageHaveTheSheetsBoxAndPixels)
        {
            Twins<QWidget> twins(*this, [](QWidget& root)
            {
                QScrollArea* area = add<QScrollArea>(root, "projectGrid", QRect(0, 0, 200, 150));
                area->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);       // the scroll bars are the style's own, in both twins
                area->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
                QWidget* contents = new QWidget;
                contents->setMinimumSize(150, 300);
                area->setWidget(contents);
                contents->setAutoFillBackground(false);                         // Native's '*' rule made it transparent
                add<QLabel>(root, "summary", QRect(210, 0, 100, 90));
                add<QLabel>(root, "summaryNoFill", QRect(320, 0, 100, 90));
                add<QLabel>(root, "outline", QRect(430, 70, 70, 50));
                add<QFrame>(root, "tooRound", QRect(320, 100, 60, 30))->setFrameShape(QFrame::StyledPanel);     // a corner that does not fit is square
                add<QFrame>(root, "nativePanel", QRect(320, 140, 90, 70))->setFrameShape(QFrame::StyledPanel);  // the style's own frame, inside the margin
                QFrame* boxFrame = add<QFrame>(root, "boxFrame", QRect(430, 0, 70, 60));    // a shape Qt sizes from its line width: the base style's rect
                boxFrame->setFrameStyle(QFrame::Box | QFrame::Plain);
                boxFrame->setLineWidth(3);
                add<QFrame>(root, "imagePadding", QRect(210, 140, 90, 70));     // the image at the bottom right of the padding box
            }, QString::fromUtf8(FrameRules) + imagePaddingRules(), QSize(510, 220));

            // The Native numbers: the grid's contents [1, 41, 1, 6] (the 1px native frame inside the margin; the style's own frame is
            // invisible over the transparent fill), the summary's [0, 1, 0, 3] and minimum height 84 (the margin and the line)
            EXPECT_TRUE(twins.legacyChild<QScrollArea>("projectGrid")->contentsMargins() == QMargins(1, 41, 1, 6));
            EXPECT_TRUE(twins.legacyChild<QLabel>("summary")->contentsMargins() == QMargins(0, 1, 0, 3));
            EXPECT_EQ(twins.legacyChild<QLabel>("summary")->minimumHeight(), 84);
            expectSame(twins, { "projectGrid", "summary", "summaryNoFill", "outline", "nativePanel", "boxFrame", "tooRound", "imagePadding" });
            EXPECT_TRUE(twins.flatChild<QFrame>("nativePanel")->contentsMargins() == QMargins(7, 7, 7, 7));
            EXPECT_TRUE(twins.flatChild<QFrame>("boxFrame")->contentsMargins() == QMargins(3, 3, 3, 3));
            EXPECT_EQ(twins.flatChild<QScrollArea>("projectGrid")->viewport()->geometry(), twins.legacyChild<QScrollArea>("projectGrid")->viewport()->geometry());

            // Only an element with a hover face asks for the events; qproperty-indent reached the label
            const QLabel* outline = twins.flatChild<QLabel>("outline");
            EXPECT_TRUE(outline->testAttribute(Qt::WA_Hover));
            EXPECT_FALSE(twins.flatChild<QFrame>("tooRound")->testAttribute(Qt::WA_Hover));
            EXPECT_EQ(outline->indent(), twins.legacyChild<QLabel>("outline")->indent());
            EXPECT_EQ(outline->indent(), 0);

            // The line is the bottom two rows of the border box; a rule that draws nothing draws no line (qstylesheetstyle.cpp:4681-4725)
            const QImage image = paint(twins.m_flat, 1.0);
            EXPECT_TRUE(image.pixelColor(250, 88).rgb() == rgb(0x555555).rgb());
            EXPECT_EQ(image.pixelColor(360, 88).alpha(), 0);
        }

        // ----------------------------------------------------------------------------------
        // T2: push buttons: limits, hints, the contents rect, and the whole button as it draws
        // ----------------------------------------------------------------------------------

        TEST_F(BoxElementTests, Buttons_LimitsHintsRectsAndPixelsAreTheSheetsWithTheNativeBorderCountedAsTheSheetCountedIt)
        {
            struct Case
            {
                const char* m_name;
                QString m_rules;
                const char* m_text;
                QSize m_minimum;                // invalid: only equal to the sheet's
                QSize m_maximum;
                QRect m_drawn;                  // invalid: not drawn (its bevel is the style's own, which the sheet's base style does not share)
            };
            // The Native numbers. Limits, in the state of the polish (Enabled alone, a button not flat yet): 24x24 + margin [10, 20, 10, 0]
            // + the native border = 46x46, while the flat button's hint is 44x44; the start card 212x278 (its own border); the footer
            // button 150 + 30 and 28 (its radius cancels the native border). PushButton::polish gave every button a maximum height,
            // which the sheet's max-height replaced: the sheet's geometry came after.
            const Case cases[] = {
                { "backButton", backRules(), "Back", QSize(46, 46), QSize(46, 46), QRect(0, 0, 44, 44) },
                { "startCard", QString::fromUtf8(StartCardRules), "Create a project", QSize(212, 278), QSize(212, 278), QRect(0, 0, 212, 278) },
                { "footerButton", QString::fromUtf8(FooterRules), "Next", QSize(180, 28), QSize(QWIDGETSIZE_MAX, 28), QRect(0, 0, 180, 28) },
                { "insetButton", QString::fromUtf8(PlainButtonRules), "Add Seed", QSize(), QSize(), QRect() },      // margins only, not flat
            };
            const QStyle::State enabled = QStyle::State_Enabled;
            for (const Case& test : cases)
            {
                ButtonTwins twins(*this, test.m_name, test.m_text, test.m_rules);
                if (test.m_minimum.isValid())
                {
                    EXPECT_EQ(twins.m_legacy->minimumSize(), test.m_minimum) << test.m_name;
                    EXPECT_EQ(twins.m_legacy->maximumSize(), test.m_maximum) << test.m_name;
                }
                EXPECT_EQ(twins.m_flat->minimumSize(), twins.m_legacy->minimumSize()) << test.m_name;
                EXPECT_EQ(twins.m_flat->maximumSize(), twins.m_legacy->maximumSize()) << test.m_name;
                EXPECT_EQ(twins.m_flat->isFlat(), twins.m_legacy->isFlat()) << test.m_name;
                EXPECT_EQ(twins.m_flat->sizeHint(), twins.m_legacy->sizeHint()) << test.m_name;

                // the rect the label is laid out in, and the focus rect
                QStyleOptionButton option;
                option.initFrom(twins.m_legacy);
                option.rect = QRect(0, 0, 200, 60);
                option.features = twins.m_legacy->isFlat() ? QStyleOptionButton::Flat : QStyleOptionButton::None;
                for (const QStyle::SubElement element : { QStyle::SE_PushButtonContents, QStyle::SE_PushButtonFocusRect })
                {
                    EXPECT_EQ(twins.m_flat->style()->subElementRect(element, &option, twins.m_flat),
                        twins.m_legacy->style()->subElementRect(element, &option, twins.m_legacy)) << test.m_name << " " << element;
                }

                // the whole button, CE_PushButton: Fusion lays a label out in the contents rect it asks of itself, which the box of a
                // button with a margin is not
                if (test.m_drawn.isValid())
                {
                    twins.expectSameDraw(test.m_drawn, { enabled, enabled | QStyle::State_MouseOver, enabled | QStyle::State_HasFocus,
                        enabled | QStyle::State_MouseOver | QStyle::State_HasFocus,
                        enabled | QStyle::State_Sunken | QStyle::State_MouseOver | QStyle::State_HasFocus, QStyle::State_None }, test.m_name);
                }
            }

            // An element that sets sizes only: the limits are the contents' and the native border (the New Project button: 152x28, 2 over
            // its rule), and the sizing is left to the base style, from the contents the limits made (Native: its hint is [166, 24]):
            // what the flat style gives a plain button of those contents
            ButtonTwins twins(*this, "newProjectButton", "Text", QString::fromUtf8(PlainButtonRules));
            EXPECT_EQ(twins.m_legacy->minimumSize(), QSize(152, 28));
            EXPECT_EQ(twins.m_flat->minimumSize(), QSize(152, 28));
            EXPECT_EQ(twins.m_flat->maximumSize(), twins.m_legacy->maximumSize());
            QPushButton plain;
            plain.setStyle(m_style.get());
            plain.ensurePolished();
            QStyleOptionButton option;
            option.initFrom(twins.m_flat);
            QStyleOptionButton plainOption;
            plainOption.initFrom(&plain);
            EXPECT_EQ(m_style->sizeFromContents(QStyle::CT_PushButton, &option, QSize(60, 10), twins.m_flat),
                m_style->sizeFromContents(QStyle::CT_PushButton, &plainOption, QSize(150, 26), &plain));
        }

        TEST_F(BoxElementTests, Buttons_GradientFacesPaintLikeTheSheetInEveryStateAndAMenuKeepsItsArrow)
        {
            // A state with no face of its own (disabled) draws the idle one, as the sheet's rule did. The primary button on its own,
            // the secondary one on its own and in a footer, with the radius and the margin of the footer button around it.
            const QStyle::State enabled = QStyle::State_Enabled;
            const std::initializer_list<QStyle::State> states = { enabled, enabled | QStyle::State_MouseOver,
                enabled | QStyle::State_Sunken | QStyle::State_MouseOver, QStyle::State_None };
            for (const bool primary : { true, false })
            {
                for (const char* name : { "gradientButton", "footerButton" })
                {
                    const bool footer = QLatin1String(name) == QLatin1String("footerButton");
                    if (primary && footer)
                    {
                        continue;
                    }
                    ButtonTwins twins(*this, name, "Back", (primary ? PrimaryRules : SecondaryRules) + (footer ? QString::fromUtf8(FooterRules) : QString()),
                        primary ? "primary" : "secondary");
                    EXPECT_TRUE(twins.m_legacy->isFlat() && twins.m_flat->isFlat());        // qproperty-flat, applied at polish
                    twins.expectSameDraw(QRect(0, 0, 180, 28), states, name);
                }
            }

            // The menu indicator is the style's arrow, in a square of its size at the bottom right of the padding box: the face is the
            // element's and the arrow stays. The label is narrowed by, and the arrow sized by, the base style's menu indicator (the
            // legacy twin's is Fusion's, the flat one the style's own): only the fill left of both is compared.
            QMenu menu;
            ButtonTwins twins(*this, "gradientButton", "Menu", PrimaryRules, "primary");
            twins.m_legacy->setMenu(&menu);
            twins.m_flat->setMenu(&menu);
            const QRect rect(0, 0, 120, 28);
            const QRect arrow(rect.right() - 20, 0, 21, rect.height());
            const QImage legacyFace = ButtonTwins::draw(twins.m_legacy, enabled, rect, 1.0);
            const QImage flatFace = ButtonTwins::draw(twins.m_flat, enabled, rect, 1.0);
            EXPECT_EQ(differing(legacyFace.copy(0, 0, 20, rect.height()), flatFace.copy(0, 0, 20, rect.height())), 0);
            QPushButton noMenu(QStringLiteral("Menu"));
            noMenu.setProperty("primary", QStringLiteral("true"));
            noMenu.setProperty("boxElementTestsFlat", true);
            noMenu.setStyle(m_style.get());
            noMenu.ensurePolished();
            EXPECT_GT(differing(flatFace.copy(arrow), ButtonTwins::draw(&noMenu, enabled, rect, 1.0).copy(arrow)), 0);
        }

        // ----------------------------------------------------------------------------------
        // T3: resolution, faces and properties
        // ----------------------------------------------------------------------------------

        TEST_F(BoxElementTests, Registry_TheFirstRegisteredMatchWinsInAnyClassOfTheChainAndAWidgetWithNoElementKeepsNothing)
        {
            const BoxElement::Matcher matches = [](const QWidget* w) { return named(w, "resolutionFirst"); };
            BoxElement::registerElement(&WideA, &QFrame::staticMetaObject, matches);
            BoxElement::registerElement(&WideB, &QFrame::staticMetaObject, matches);
            BoxElement::registerElement(&TallC, &QLabel::staticMetaObject, matches);        // a more derived class, registered later
            // The own class is looked at first, but the order of registration decides: a registration on the class itself, made
            // before the one on its base class, wins
            const BoxElement::Matcher matchesOwn = [](const QWidget* w) { return named(w, "resolutionOwn"); };
            BoxElement::registerElement(&TallC, &QLabel::staticMetaObject, matchesOwn);
            BoxElement::registerElement(&WideB, &QFrame::staticMetaObject, matchesOwn);

            QWidget root;
            QFrame* frame = add<QFrame>(root, "resolutionFirst");
            QLabel* label = add<QLabel>(root, "resolutionFirst");
            QPushButton* button = add<QPushButton>(root, "resolutionFirst");                // not a frame: no row of its chain matches
            QLabel* own = add<QLabel>(root, "resolutionOwn");
            flat(root);
            EXPECT_TRUE(BoxElement::forWidget(label) == nullptr);       // forWidget never resolves: the polish does
            frame->ensurePolished();
            label->ensurePolished();
            button->ensurePolished();
            own->ensurePolished();

            EXPECT_TRUE(BoxElement::forWidget(own) == &TallC);
            EXPECT_TRUE(BoxElement::forWidget(frame) == &WideA);
            EXPECT_TRUE(BoxElement::forWidget(label) == &WideA);        // TallC lost: it was registered after WideA
            EXPECT_EQ(label->minimumWidth(), 7 + 10);
            EXPECT_EQ(label->maximumHeight(), QWIDGETSIZE_MAX);
            EXPECT_TRUE(BoxElement::forWidget(button) == nullptr);
            EXPECT_FALSE(button->dynamicPropertyNames().contains(QByteArrayLiteral("_o3de_boxElement")));
        }

        TEST_F(BoxElementTests, Registry_AWidgetThatSwitchesElementsTakesTheNewBoxAndLosesTheOldLimits_ALateRegistrationReachesPolishedWidgets)
        {
            BoxElement::registerElement(&WideA, &QFrame::staticMetaObject, [](const QWidget* w) { return isSwitching(w, 1); });
            BoxElement::registerElement(&TallC, &QFrame::staticMetaObject, [](const QWidget* w) { return isSwitching(w, 2); });

            QWidget root;
            QFrame* frame = add<QFrame>(root, "switching");
            QFrame* late = add<QFrame>(root, "late");
            QFrame* ignored = add<QFrame>(root, "late");
            Style::flagToIgnore(ignored);
            flat(root);
            frame->ensurePolished();
            late->ensurePolished();
            ignored->ensurePolished();
            frame->setProperty("variant", 1);
            frame->style()->polish(frame);
            EXPECT_EQ(frame->minimumWidth(), 7 + 10);
            EXPECT_EQ(frame->maximumHeight(), QWIDGETSIZE_MAX);
            EXPECT_EQ(frame->contentsMargins().left(), 7);      // the frame read its box again (a style change)

            // The sheet's way: set the property, repolish (AssetProcessor MainWindow.cpp:960-964 does it for the status label)
            frame->setProperty("variant", 2);
            frame->style()->unpolish(frame);
            frame->style()->polish(frame);
            EXPECT_TRUE(BoxElement::forWidget(frame) == &TallC);
            EXPECT_EQ(frame->minimumWidth(), 0);                // WideA's limit is gone
            EXPECT_EQ(frame->maximumHeight(), 5 + 30);
            EXPECT_EQ(frame->contentsMargins().top(), 5);
            frame->setProperty("variant", 3);                   // no element at all
            frame->style()->polish(frame);
            EXPECT_TRUE(BoxElement::forWidget(frame) == nullptr);
            EXPECT_EQ(frame->maximumHeight(), QWIDGETSIZE_MAX);
            EXPECT_EQ(frame->contentsMargins().top(), 0);

            // A module sets up its styles after its window was built: the polished widgets are resolved again; one flagged to ignore the
            // style has no element
            EXPECT_TRUE(BoxElement::forWidget(late) == nullptr);
            BoxElement::registerElement(&WideB, &QFrame::staticMetaObject, [](const QWidget* w) { return named(w, "late"); });
            EXPECT_TRUE(BoxElement::forWidget(late) == &WideB);
            EXPECT_EQ(late->minimumWidth(), 3 + 20);
            EXPECT_EQ(late->contentsMargins().left(), 3);
            EXPECT_TRUE(BoxElement::forWidget(ignored) == nullptr);
            EXPECT_EQ(ignored->minimumWidth(), 0);
        }

        TEST_F(BoxElementTests, Face_OfTheEntriesWhoseStatesAllHoldTheLastOneIsTheCascade)
        {
            const BoxElement element{
                .m_name = "Test.Box.Faces",
                .m_idle = { .m_text = rgb(1) },
                .m_stateFaces = {
                    { BoxElement::Disabled, { .m_text = rgb(5) } },
                    { BoxElement::Focus, { .m_text = rgb(2) } },
                    { BoxElement::Hover, { .m_text = rgb(3) } },
                    { BoxElement::Hover | BoxElement::Focus, { .m_text = rgb(4) } },
                },
            };
            const auto text = [&](QStyle::State state)
            {
                QStyleOption option;
                option.state = state;
                return element.face(&option).m_text;
            };
            const QStyle::State enabled = QStyle::State_Enabled;
            EXPECT_TRUE(text(enabled) == rgb(1));
            EXPECT_TRUE(text(enabled | QStyle::State_HasFocus) == rgb(2));
            EXPECT_TRUE(text(enabled | QStyle::State_MouseOver) == rgb(3));
            EXPECT_TRUE(text(enabled | QStyle::State_MouseOver | QStyle::State_HasFocus) == rgb(4));     // the entry of both, after both
            EXPECT_TRUE(text(enabled | QStyle::State_Sunken) == rgb(1));                                    // no pressed face: idle
            EXPECT_TRUE(text(QStyle::State_None) == rgb(5));                                                // disabled
            EXPECT_TRUE(text(QStyle::State_MouseOver) == rgb(5));                                           // a disabled widget is never hovered
            EXPECT_TRUE(text(QStyle::State_HasFocus) == rgb(2));                                            // the later entry of the two that hold
            EXPECT_TRUE(text(QStyle::State_MouseOver | QStyle::State_HasFocus) == rgb(2));
        }

        TEST_F(BoxElementTests, Properties_TakeTheTypeOfTheProperty_AnIconIsAStringInTheElement)
        {
            static const BoxElement help{
                .m_name = "Test.Box.Button.Help",
                .m_properties = { { "icon", QString::fromUtf8(images().m_plain) }, { "iconSize", QSize(24, 24) } },
            };
            BoxElement::registerElement(&help, &QPushButton::staticMetaObject, [](const QWidget* w) { return named(w, "helpButton"); });

            QWidget root;
            QPushButton* button = add<QPushButton>(root, "helpButton");
            flat(root);
            button->ensurePolished();
            EXPECT_FALSE(button->icon().isNull());
            EXPECT_EQ(button->iconSize(), QSize(24, 24));
        }

        // ----------------------------------------------------------------------------------
        // T4: what the sheet decided by class and by where the rules sit
        // ----------------------------------------------------------------------------------

        TEST_F(BoxElementTests, ADeclaredZeroBox_IsTheRulesBoxForAFrameAndForAButton)
        {
            Twins<QWidget> frames(*this, [](QWidget& root)
            {
                QFrame* frame = add<QFrame>(root, "zeroBoxFrame", QRect(0, 0, 100, 40));
                frame->setFrameStyle(QFrame::Box | QFrame::Plain);
                frame->setLineWidth(3);
            }, QString::fromUtf8(ZeroBoxRules), QSize(120, 60));
            EXPECT_TRUE(frames.legacyChild<QFrame>("zeroBoxFrame")->contentsMargins() == QMargins(1, 1, 1, 1));    // the native 1, not the 3 of the line width
            expectSame(frames, { "zeroBoxFrame" });

            ButtonTwins buttons(*this, "zeroBoxButton", "Text", QString::fromUtf8(ZeroBoxRules));
            EXPECT_EQ(buttons.m_flat->sizeHint(), buttons.m_legacy->sizeHint());                // the text and the native border, not the components' height
            EXPECT_EQ(buttons.m_flat->minimumSizeHint(), buttons.m_legacy->minimumSizeHint());
            EXPECT_EQ(buttons.m_legacy->sizeHint().height(), buttons.m_legacy->fontMetrics().height() + 2);
        }

        TEST_F(BoxElementTests, Attributes_TheStyledBackgroundIsSetOnTheClassesTheSheetSetsItOn)
        {
            Twins<QWidget> twins(*this, [](QWidget& root)
            {
                add<QTabBar>(root, "tabBar", QRect(0, 0, 80, 30))->addTab(QStringLiteral("One"));
                add<QMenuBar>(root, "menuBar", QRect(0, 40, 80, 30));
                auto* header = new QHeaderView(Qt::Horizontal, &root);
                header->setObjectName(QStringLiteral("headerView"));
                header->setGeometry(0, 80, 80, 30);
                auto* subWindow = new QMdiSubWindow(&root);
                subWindow->setObjectName(QStringLiteral("subWindow"));
                subWindow->setGeometry(0, 120, 80, 40);
            }, QString::fromUtf8(BarRules), QSize(120, 180));
            for (const char* name : { "tabBar", "menuBar", "headerView", "subWindow" })
            {
                const QWidget* legacy = twins.legacyChild<QWidget>(name);
                EXPECT_TRUE(legacy->testAttribute(Qt::WA_StyledBackground)) << name;
                EXPECT_EQ(twins.flatChild<QWidget>(name)->testAttribute(Qt::WA_StyledBackground), legacy->testAttribute(Qt::WA_StyledBackground)) << name;
            }
        }

        TEST_F(BoxElementTests, Registry_AModuleElementBeatsAGlobalOneWhicheverWasRegisteredFirst)
        {
            // AzQtComponents registers the Global elements when it starts, before a module has set up its styles
            BoxElement::registerElement(&GlobalFrame, &QFrame::staticMetaObject,
                [](const QWidget* w) { return named(w, "scopeBoth") || named(w, "scopeGlobalOnly"); }, BoxElement::Scope::Global);
            BoxElement::registerElement(&ModuleFrame, &QFrame::staticMetaObject, [](const QWidget* w) { return named(w, "scopeBoth"); });
            BoxElement::registerElement(&GlobalFrameLater, &QFrame::staticMetaObject,
                [](const QWidget* w) { return named(w, "scopeGlobalOnly") || named(w, "scopeChain"); }, BoxElement::Scope::Global);
            // a module element on a base class of the widget still comes before a Global one on the class itself
            BoxElement::registerElement(&ModuleWidget, &QWidget::staticMetaObject, [](const QWidget* w) { return named(w, "scopeChain"); });

            QWidget root;
            QFrame* both = add<QFrame>(root, "scopeBoth");
            QFrame* globalOnly = add<QFrame>(root, "scopeGlobalOnly");
            QFrame* chain = add<QFrame>(root, "scopeChain");
            flat(root);
            for (QFrame* frame : { both, globalOnly, chain })
            {
                frame->ensurePolished();
            }
            EXPECT_TRUE(BoxElement::forWidget(both) == &ModuleFrame);
            EXPECT_TRUE(BoxElement::forWidget(globalOnly) == &GlobalFrame);        // among the Global ones the first registered
            EXPECT_TRUE(BoxElement::forWidget(chain) == &ModuleWidget);
        }

        TEST_F(BoxElementTests, StartCardFaces_TheEntriesAreInTheOrderTheRulesCascade)
        {
            const QRect rect(0, 0, 212, 278);
            const QStyle::State e = QStyle::State_Enabled;
            const QStyle::State h = QStyle::State_MouseOver;
            const QStyle::State s = QStyle::State_Sunken;
            const QStyle::State f = QStyle::State_HasFocus;

            ButtonTwins ordered(*this, "realCard", "Create", realCardRules("realCard"));
            ordered.expectSameDraw(rect, { e, e | h, e | h | s, e | f, e | h | f, e | h | s | f, e | s, e | s | f, QStyle::State_None }, "start card, focus before hover");

            // The same faces in a fixed hover, pressed, focus order: the focus face wins wherever the button has the focus
            ButtonTwins fixed(*this, "draftedCard", "Create", realCardRules("draftedCard"));
            fixed.expectSameDraw(rect, { e, e | h, e | h | s, e | f, QStyle::State_None }, "start card, fixed order, states with no focus under the pointer");
            int differs = 0;
            for (const QStyle::State state : { e | h | f, e | h | s | f, e | s | f })
            {
                differs += differing(ButtonTwins::draw(fixed.m_legacy, state, rect, 1.0), ButtonTwins::draw(fixed.m_flat, state, rect, 1.0));
            }
            EXPECT_GT(differs, 0);
        }
    } // namespace BoxElementTestsInternal
} // namespace AzQtComponents
