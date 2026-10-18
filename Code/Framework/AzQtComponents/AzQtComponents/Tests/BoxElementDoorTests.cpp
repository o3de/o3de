/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzTest/AzTest.h>
#include <AzQtComponents/Components/Elements/BoxElement.h>
#include <AzQtComponents/Components/Elements/IndicatorElement.h>
#include <AzQtComponents/Components/Style.h>
#include <AzQtComponents/Components/StyleManager.h>

#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>

#include <QAbstractScrollArea>
#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QFrame>
#include <QImage>
#include <QLineEdit>
#include <QMenu>
#include <QPainter>
#include <QPixmap>
#include <QRadioButton>
#include <QScrollBar>
#include <QScrollArea>
#include <QSpinBox>
#include <QStyleFactory>
#include <QStyleOption>
#include <QTemporaryDir>
#include <QToolButton>
#include <QVBoxLayout>
#include <QtMath>

#include <string>

namespace AzQtComponents
{
    // ----------------------------------------------------------------------------------
    // The doors of the box element for the classes whose style has no door of its own besides the frame and the push button: a line
    // edit, a check box, a radio button, a combo box, a tool button and a scroll area. The legacy twin of a widget is Qt's own style
    // sheet style over Fusion with the rules of the legacy sheets; the flat twin is the flattened Style with the element that
    // carries the same box. The same box, the same hints, the same pixels, left to right and right to left, at the device ratios 1
    // and 1.5 (painted through the style on an image of the ratio; QWidget::render, which hands the style the screen's ratio, only
    // compares the two twins as they paint themselves, text included). A widget no element matches is answered as before the doors
    // (the last tests).
    // ----------------------------------------------------------------------------------

    namespace BoxElementDoorTestData
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
            return widget->window()->property("boxElementDoorTestsFlat").toBool() && widget->objectName() == QLatin1String(objectName);
        }

        // ---- LineEdit.qss:11-20 over BaseStyleSheet.qss:68-84 and :28-35: the field of the legacy line edit (the drafts' Global.Box.LineEdit):
        // a 2px ring in #e9e9e9 with the corners of a 2px radius, 16px high inside it, a padding of -1px above and 1px below, #cccccc and
        // #666666 when disabled. The font and the colours of the text are not a box.
        const char* const FieldRules =
            "QLineEdit { background-color: #CCCCCC; border-radius: 2px; border-width: 0px; border-color: #e9e9e9; }"
            "QLineEdit { height: 16px; padding-left: 0px; border-width: 2px; border-style: solid; padding-bottom: 1px; padding-top: -1px; }"
            "QLineEdit:disabled { background-color: #666666; }";
        const BoxElement Field{
            .m_name = "Test.Door.Field",
            .m_padding = QMargins(0, -1, 0, 1),                                     // LineEdit.qss:15, :18-19
            .m_nativeBorder = false,                                                // LineEdit.qss:16-17
            .m_radius = 2,                                                          // BaseStyleSheet.qss:79
            .m_size = QSize(-1, 16),                                                // LineEdit.qss:13
            .m_idle = { .m_fill = solid(0xCCCCCC), .m_edges = BoxElement::border(2, rgb(0xE9E9E9)) },           // BaseStyleSheet.qss:76, :81
            .m_stateFaces = { { BoxElement::Disabled, { .m_fill = solid(0x666666), .m_edges = BoxElement::border(2, rgb(0xE9E9E9)) } } },  // LineEdit.qss:28-35
        };

        // ---- the same field with the faces of QTextEdit:hover and :focus (BaseStyleSheet.qss:97-115) on a line edit: the ring turns dark
        // under the pointer, light blue with the focus (and the field white). A focused disabled field keeps the blue ring of the
        // focus rule and the grey of the disabled one, an entry of its own.
        const char* const StateFieldRules =
            "#doorStateField { background-color: #CCCCCC; border-radius: 2px; border-width: 2px; border-style: solid; border-color: #e9e9e9;"
            " height: 16px; padding: -1px 0px 1px 0px; }"
            "#doorStateField:hover { border-color: #222222; }"
            "#doorStateField:focus { border-color: #94D2FF; background-color: #FFFFFF; }"
            "#doorStateField:disabled { background-color: #666666; }";
        const BoxElement StateField{
            .m_name = "Test.Door.StateField",
            .m_padding = QMargins(0, -1, 0, 1),
            .m_nativeBorder = false,
            .m_radius = 2,
            .m_size = QSize(-1, 16),
            .m_idle = { .m_fill = solid(0xCCCCCC), .m_edges = BoxElement::border(2, rgb(0xE9E9E9)) },
            .m_stateFaces = {
                { BoxElement::Hover, { .m_fill = solid(0xCCCCCC), .m_edges = BoxElement::border(2, rgb(0x222222)) } },
                { BoxElement::Focus, { .m_fill = solid(0xFFFFFF), .m_edges = BoxElement::border(2, rgb(0x94D2FF)) } },
                { BoxElement::Disabled, { .m_fill = solid(0x666666), .m_edges = BoxElement::border(2, rgb(0xE9E9E9)) } },
                { BoxElement::Disabled | BoxElement::Focus, { .m_fill = solid(0x666666), .m_edges = BoxElement::border(2, rgb(0x94D2FF)) } },
            },
        };

        // ---- ProjectManager.qss:299-306 (#formFrame QLineEdit) over LineEdit.qss and BaseStyleSheet.qss:79: the field of the project
        // forms: no border, a clear fill, a padding of -4px above and 1px below, a radius of 2px. The height 1em follows the font
        // (qcssparser.cpp lengthValueFromData), which no element carries: 18px here.
        const char* const FormFieldRules =
            "#doorFormField { background-color: rgba(0,0,0,0); border: 0; border-radius: 2px; height: 18px; padding: -4px 0px 1px 0px; }";
        const BoxElement FormField{
            .m_name = "Test.Door.FormField",
            .m_padding = QMargins(0, -4, 0, 1),                                     // ProjectManager.qss:306, LineEdit.qss:18
            .m_nativeBorder = false,                                                // ProjectManager.qss:303
            .m_radius = 2,                                                          // BaseStyleSheet.qss:79
            .m_size = QSize(-1, 18),                                                // ProjectManager.qss:305 (1em, in px)
            .m_idle = { .m_fill = QBrush(QColor(0, 0, 0, 0)) },                     // ProjectManager.qss:300
        };

        // ---- Fields with the own border of the style: a margin around it, a padding inside it, a gradient the style cannot draw, limits
        const char* const NativeFieldRules =
            "#doorNativeField { margin: 3px; background-color: #CCCCCC; min-height: 20px; max-height: 24px; min-width: 70px; }"
            "#doorMarginField { margin: 4px 2px 5px 6px; }"
            "#doorPaddingField { padding: 2px 3px 4px 5px; background-color: #BBBBBB; }"
            "#doorGradientField { margin: 2px; background: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1, stop: 0 #DDDDDD, stop: 1 #888888); }";
        const BoxElement NativeField{
            .m_name = "Test.Door.NativeField",
            .m_margin = QMargins(3, 3, 3, 3),
            .m_minSize = QSize(70, 20),
            .m_maxSize = QSize(-1, 24),
            .m_idle = { .m_fill = solid(0xCCCCCC) },
        };
        const BoxElement MarginField{ .m_name = "Test.Door.MarginField", .m_margin = QMargins(6, 4, 2, 5) };
        const BoxElement PaddingField{ .m_name = "Test.Door.PaddingField", .m_padding = QMargins(5, 2, 3, 4), .m_idle = { .m_fill = solid(0xBBBBBB) } };
        const BoxElement GradientField{
            .m_name = "Test.Door.GradientField",
            .m_margin = QMargins(2, 2, 2, 2),
            .m_idle = { .m_fill = BoxElement::vertical(rgb(0xDDDDDD), rgb(0x888888)) },
        };

        // ---- AssetProcessor.qss:195-201 (QToolButton#serverAddressToolButton) without its border-image, over BaseStyleSheet.qss:17: a tool
        // button with no border, no padding and a clear fill. A box with a 0 in it is still the rule's box (QRenderRule::hasBox).
        const char* const ServerToolRules =
            "#doorServerTool { background-color: transparent; min-width: -1px; border-width: 0px; border-radius: 0px; padding: 0 0 0 0; }";
        const BoxElement ServerTool{
            .m_name = "Test.Door.ServerTool",
            .m_hasBox = true,                                                       // AssetProcessor.qss:200
            .m_nativeBorder = false,                                                // :197-198
            .m_idle = { .m_fill = QBrush(Qt::transparent) },                        // BaseStyleSheet.qss:17
        };

        // ---- A tool button with a box of its own, drawn by the element in each state. The fills and the label colours follow the order of
        // the rules (a later one wins, each sets both), so no state needs an entry for a combination: pressed over hover, checked over
        // both, disabled over all.
        const char* const FacedToolRules =
            "#doorFacedTool { margin: 2px; padding: 3px 5px; border: 1px solid #222222; border-radius: 3px; background-color: #444444; color: #CCCCCC; }"
            "#doorFacedTool:hover { background-color: #555555; color: #FFFFFF; }"
            "#doorFacedTool:pressed { background-color: #333333; color: #EEEEEE; }"
            "#doorFacedTool:checked { background-color: #1E70EB; color: #DDDDDD; }"
            "#doorFacedTool:disabled { background-color: #666666; color: #888888; }";
        const BoxElement FacedTool{
            .m_name = "Test.Door.FacedTool",
            .m_margin = QMargins(2, 2, 2, 2),
            .m_padding = QMargins(5, 3, 5, 3),
            .m_nativeBorder = false,
            .m_radius = 3,
            .m_idle = { .m_fill = solid(0x444444), .m_edges = BoxElement::border(1, rgb(0x222222)), .m_text = rgb(0xCCCCCC) },
            .m_stateFaces = {
                { BoxElement::Hover, { .m_fill = solid(0x555555), .m_edges = BoxElement::border(1, rgb(0x222222)), .m_text = rgb(0xFFFFFF) } },
                { BoxElement::Pressed, { .m_fill = solid(0x333333), .m_edges = BoxElement::border(1, rgb(0x222222)), .m_text = rgb(0xEEEEEE) } },
                { BoxElement::Checked, { .m_fill = solid(0x1E70EB), .m_edges = BoxElement::border(1, rgb(0x222222)), .m_text = rgb(0xDDDDDD) } },
                { BoxElement::Disabled, { .m_fill = solid(0x666666), .m_edges = BoxElement::border(1, rgb(0x222222)), .m_text = rgb(0x888888) } },
            },
        };

        // ---- TitleBar.qss:79-88 (AzQtComponents--TabBar AzQtComponents--DockBarButton): a margin to the right (:87) and a size of 14 x 14
        // (:83-86), here with the style's own border in place of the `border: 0 none` of :82 (a tool button with a box and a native border)
        const char* const NativeToolRules =
            "#doorNativeTool { background: transparent; margin-right: 4px; min-width: 14px; max-width: 14px; min-height: 14px; max-height: 14px; }";
        const BoxElement NativeTool{
            .m_name = "Test.Door.NativeTool",
            .m_margin = QMargins(0, 0, 4, 0),
            .m_minSize = QSize(14, 14),
            .m_maxSize = QSize(14, 14),
            .m_idle = { .m_fill = QBrush(Qt::transparent) },
        };

        // ---- A rule for :pressed on a line edit never matches: the field's State_Sunken only says its frame is sunken
        // (qstylesheetstyle.cpp:2121-2123), so the red of its pressed entry must never show
        const char* const PressedFieldRules =
            "#doorPressedField { background-color: #CCCCCC; border: 2px solid #E9E9E9; border-radius: 2px; }"
            "#doorPressedField:pressed { background-color: #FF0000; }";
        const BoxElement PressedField{
            .m_name = "Test.Door.PressedField",
            .m_nativeBorder = false,
            .m_radius = 2,
            .m_idle = { .m_fill = solid(0xCCCCCC), .m_edges = BoxElement::border(2, rgb(0xE9E9E9)) },
            .m_stateFaces = { { BoxElement::Pressed, { .m_fill = solid(0xFF0000), .m_edges = BoxElement::border(2, rgb(0xE9E9E9)) } } },
        };

        // ---- CheckBox.qss:11-28 and RadioButton.qss:9-33: what every check box and radio button had under the sheet: a clear fill, no
        // focus rectangle, and the rule for the indicator that makes the sheet lay a button out around one (the image is the indicator
        // element's). The radio button's indicator is larger with the focus and its margin smaller: the box is 20px either way.
        const char* const IndicatorRules =
            "QCheckBox { background-color: transparent; outline: 0; }"
            "QCheckBox:disabled { color: #999999; }"
            "QCheckBox::indicator { width: 16px; height: 16px; margin: 2px; }"
            "QRadioButton { background-color: transparent; outline: 0; }"
            "QRadioButton:disabled { color: #999999; }"
            "QRadioButton::indicator:!focus { width: 16px; height: 16px; margin: 2px; }"
            "QRadioButton::indicator:focus { width: 18px; height: 18px; margin: 1px; }";

        // ---- AssetBundler.qss:82-84: a check box with a margin all round
        const char* const SeedCheckRules = "QCheckBox#doorSeedCheck { margin: 5px; }";
        const BoxElement SeedCheck{
            .m_name = "Test.Door.SeedCheck",
            .m_margin = QMargins(5, 5, 5, 5),                                       // AssetBundler.qss:83
            .m_idle = { .m_fill = QBrush(Qt::transparent) },                        // CheckBox.qss:12
            .m_stateFaces = { { BoxElement::Disabled, { .m_fill = QBrush(Qt::transparent), .m_text = rgb(0x999999) } } },     // CheckBox.qss:18-21
        };

        // ---- ProjectManager.qss:79-95 (#createAGemLHS QRadioButton): a radio button 40px high and 180px wide at least, with a colour
        // that follows its state and a fill when it is checked (without the font and the spacing, which no element carries here)
        const char* const LhsRadioRules =
            "#doorLhsRadio { height: 40px; min-width: 180px; color: #6b6b6b; }"
            "#doorLhsRadio:enabled { color: #aaaaaa; }"
            "#doorLhsRadio:checked { background-color: #333333; color: #FFFFFF; }";
        const BoxElement LhsRadio{
            .m_name = "Test.Door.LhsRadio",
            .m_size = QSize(-1, 40),                                                // ProjectManager.qss:81
            .m_minSize = QSize(180, -1),                                            // :82
            .m_idle = { .m_fill = QBrush(Qt::transparent), .m_text = rgb(0xAAAAAA) },       // :87-89
            .m_stateFaces = {
                { BoxElement::Disabled, { .m_fill = QBrush(Qt::transparent), .m_text = rgb(0x6B6B6B) } },   // :84
                { BoxElement::Checked, { .m_fill = solid(0x333333), .m_text = rgb(0xFFFFFF) } },          // :91-95
            },
        };

        // ---- A check box with the whole box, drawn by the element in each state. The fills follow the order of the rules, so no
        // state needs an entry for a combination.
        const char* const FacedCheckRules =
            "#doorFacedCheck { margin: 2px; padding: 3px 6px; border: 1px solid #222222; border-radius: 3px; background-color: #444444; color: #CCCCCC; }"
            "#doorFacedCheck:hover { background-color: #555555; }"
            "#doorFacedCheck:focus { background-color: #666666; }"
            "#doorFacedCheck:checked { background-color: #1E70EB; color: #FFFFFF; }"
            "#doorFacedCheck:disabled { background-color: #333333; color: #888888; }";
        const BoxElement FacedCheck{
            .m_name = "Test.Door.FacedCheck",
            .m_margin = QMargins(2, 2, 2, 2),
            .m_padding = QMargins(6, 3, 6, 3),
            .m_nativeBorder = false,
            .m_radius = 3,
            .m_idle = { .m_fill = solid(0x444444), .m_edges = BoxElement::border(1, rgb(0x222222)), .m_text = rgb(0xCCCCCC) },
            .m_stateFaces = {
                { BoxElement::Hover, { .m_fill = solid(0x555555), .m_edges = BoxElement::border(1, rgb(0x222222)), .m_text = rgb(0xCCCCCC) } },
                { BoxElement::Focus, { .m_fill = solid(0x666666), .m_edges = BoxElement::border(1, rgb(0x222222)), .m_text = rgb(0xCCCCCC) } },
                { BoxElement::Checked, { .m_fill = solid(0x1E70EB), .m_edges = BoxElement::border(1, rgb(0x222222)), .m_text = rgb(0xFFFFFF) } },
                { BoxElement::Disabled, { .m_fill = solid(0x333333), .m_edges = BoxElement::border(1, rgb(0x222222)), .m_text = rgb(0x888888) } },
            },
        };

        // ---- A scroll area with the whole box and a fill, drawn by the element (the viewport is transparent over it); the same with a
        // border and no fill (the viewport then fills itself, as the sheet had it do); the same with a margin and a padding only
        const char* const AreaRules =
            "#doorFilledArea { margin: 2px; padding: 3px; border: 2px solid #555555; border-radius: 4px; background-color: #333333; }"
            "#doorFilledArea:hover { border-color: #1E70EB; }"
            "#doorBareArea { border: 1px solid #AAAAAA; }"
            "#doorMarginArea { margin: 4px; padding: 2px 3px 4px 5px; background-color: #222222; }";
        const BoxElement FilledArea{
            .m_name = "Test.Door.FilledArea",
            .m_margin = QMargins(2, 2, 2, 2),
            .m_padding = QMargins(3, 3, 3, 3),
            .m_nativeBorder = false,
            .m_radius = 4,
            .m_idle = { .m_fill = solid(0x333333), .m_edges = BoxElement::border(2, rgb(0x555555)) },
            .m_stateFaces = { { BoxElement::Hover, { .m_fill = solid(0x333333), .m_edges = BoxElement::border(2, rgb(0x1E70EB)) } } },
        };
        const BoxElement BareArea{
            .m_name = "Test.Door.BareArea",
            .m_nativeBorder = false,
            .m_idle = { .m_edges = BoxElement::border(1, rgb(0xAAAAAA)) },
        };
        const BoxElement MarginArea{
            .m_name = "Test.Door.MarginArea",
            .m_margin = QMargins(4, 4, 4, 4),
            .m_padding = QMargins(5, 2, 3, 4),
            .m_idle = { .m_fill = solid(0x222222) },
        };
        //! An image for the elements and the sheets: a file of a temporary directory, with a mark in its corner
        struct Image
        {
            QTemporaryDir m_directory;
            QByteArray m_path;
            QByteArray m_arrow;
        };

        Image& image()
        {
            static Image shared;
            if (shared.m_path.isEmpty())
            {
                QImage picture(14, 10, QImage::Format_ARGB32);
                picture.fill(rgb(0xFF8000));
                QPainter painter(&picture);
                painter.fillRect(0, 0, 4, 4, rgb(0xFFFFFF));
                painter.end();
                const QString path = shared.m_directory.filePath(QStringLiteral("plain.png"));
                picture.save(path, "PNG");
                shared.m_path = path.toUtf8();

                // the arrow of a drop-down: 16 x 16, the colour of nothing else
                QImage arrow(16, 16, QImage::Format_ARGB32);
                arrow.fill(rgb(0xFF00FF));
                const QString arrowPath = shared.m_directory.filePath(QStringLiteral("arrow.png"));
                arrow.save(arrowPath, "PNG");
                shared.m_arrow = arrowPath.toUtf8();
            }
            return shared;
        }


        // ---- A scroll area with an image behind its contents, which scrolls with them (qstylesheetstyle.cpp:4700, 1221-1250), and one with a
        // margin and no fill, which the sheet filled with its own background role
        BoxElement ImageArea{
            .m_name = "Test.Door.ImageArea",
            .m_nativeBorder = false,
            .m_idle = { .m_fill = solid(0x202020), .m_edges = BoxElement::border(1, rgb(0x3366CC)) },
        };
        QString imageAreaRules()
        {
            return QStringLiteral("#doorImageArea { background: #202020 url(%1) no-repeat top left; border: 1px solid #3366CC; }")
                .arg(QString::fromUtf8(image().m_path));
        }
        // ... and the same image in the middle of the box: the offset of the scrolled contents moves the whole image there, where at the top
        // left it only cuts the corner of it (the image is translated by the offset before it is cut to its box, qstylesheetstyle.cpp:1228-1229, 1247-1251)
        BoxElement CenteredImageArea{
            .m_name = "Test.Door.CenteredImageArea",
            .m_nativeBorder = false,
            .m_idle = { .m_fill = solid(0x202020), .m_edges = BoxElement::border(1, rgb(0x3366CC)) },
            .m_imageAlign = Qt::AlignCenter,
        };
        QString centeredImageAreaRules()
        {
            return QStringLiteral("#doorCenteredImageArea { background: #202020 url(%1) no-repeat center; border: 1px solid #3366CC; }")
                .arg(QString::fromUtf8(image().m_path));
        }
        const char* const PlainAreaRules = "#doorPlainArea { margin: 6px 3px 2px 5px; }";
        const BoxElement PlainArea{ .m_name = "Test.Door.PlainArea", .m_margin = QMargins(5, 6, 3, 2) };

        // ---- ComboBox.qss:13-20, :53-75 over BaseStyleSheet.qss:68-84: the field of the legacy combo box with the arrow of its drop-down.
        // The :hover rule of ComboBox.qss (:22-29) also takes the margin away and gives a border: that is a face with a margin and a
        // radius of its own (BoxElementFaceTests, with the real sheet), so the hover here is the fill alone.
        QString comboRules(const char* name)
        {
            return QStringLiteral("#%1 { background-color: #CCCCCC; margin: 1px; padding-left: 4px; border-radius: 2px; border-width: 0px; height: 16px; }"
                                  "#%1:hover { background-color: #DDDDDD; }"
                                  "#%1:disabled { background-color: #666666; color: #999999; }"
                                  "#%1::drop-down { border-width: 0; }"
                                  "#%1::down-arrow { width: 16px; height: 16px; margin-right: 4px; image: url(%2); }")
                .arg(QString::fromLatin1(name), QString::fromUtf8(image().m_arrow));
        }
        const BoxElement ComboField{
            .m_name = "Test.Door.ComboField",
            .m_margin = QMargins(1, 1, 1, 1),                                       // ComboBox.qss:17
            .m_padding = QMargins(4, 0, 0, 0),                                      // :16
            .m_nativeBorder = false,                                                // BaseStyleSheet.qss:79-80
            .m_radius = 2,
            .m_size = QSize(-1, 16),                                                // :15
            .m_idle = { .m_fill = solid(0xCCCCCC) },                                // :19
            .m_stateFaces = {
                { BoxElement::Hover, { .m_fill = solid(0xDDDDDD) } },
                { BoxElement::Disabled, { .m_fill = solid(0x666666), .m_text = rgb(0x999999) } },                      // :53-57
            },
        };

        // ---- ProjectManager.qss:149-159 and :313-323 (#formFrame QComboBox): a combo box 26px high (:156) with no padding at its sides
        // (:157-158), and a fixed width (:152-153, 810px there, 150px here)
        const char* const WideComboRules =
            "#doorWideCombo { background-color: #CCCCCC; margin: 1px; padding: 0px; border-radius: 2px; border-width: 0px; height: 26px;"
            " min-width: 150px; max-width: 150px; }"
            "#doorWideCombo:disabled { background-color: #666666; color: #999999; }";
        const BoxElement WideCombo{
            .m_name = "Test.Door.WideCombo",
            .m_margin = QMargins(1, 1, 1, 1),
            .m_padding = QMargins(0, 0, 0, 0),
            .m_hasBox = true,
            .m_nativeBorder = false,
            .m_radius = 2,
            .m_size = QSize(-1, 26),
            .m_minSize = QSize(150, -1),
            .m_maxSize = QSize(150, -1),
            .m_idle = { .m_fill = solid(0xCCCCCC) },
            .m_stateFaces = { { BoxElement::Disabled, { .m_fill = solid(0x666666), .m_text = rgb(0x999999) } } },
        };

        // ---- A combo box with a box around the style's own border (a margin, a padding, a minimum width): the sheet sizes and lays it out by
        // the box, the base style draws its frame (qstylesheetstyle.cpp:5336-5347, 5799-5830), so the hints and the rects are compared
        const char* const NativeComboRules = "#doorNativeCombo { margin: 2px 3px 1px 4px; padding: 1px 2px 3px 5px; min-width: 120px; }";
        const BoxElement NativeCombo{
            .m_name = "Test.Door.NativeCombo",
            .m_margin = QMargins(4, 2, 3, 1),
            .m_padding = QMargins(5, 1, 2, 3),
            .m_minSize = QSize(120, -1),
        };

        // ---- A combo box with a ring that changes colour and a field that turns white with the focus (ComboBox.qss:31-42 without the
        // margin and radius it changes)
        const char* const RingComboRules =
            "#doorRingCombo { background-color: #CCCCCC; padding-left: 4px; border: 1px solid #222222; border-radius: 3px; height: 16px; }"
            "#doorRingCombo:hover { border-color: #FFFFFF; }"
            "#doorRingCombo:focus { border-color: #94D2FF; background-color: #FFFFFF; }"
            "#doorRingCombo:disabled { background-color: #666666; color: #999999; }"
            "#doorRingCombo::drop-down { border-width: 0; }"
            "#doorRingCombo::down-arrow { width: 16px; height: 16px; margin-right: 4px; image: url(%1); }";
        const BoxElement RingCombo{
            .m_name = "Test.Door.RingCombo",
            .m_padding = QMargins(4, 0, 0, 0),
            .m_nativeBorder = false,
            .m_radius = 3,
            .m_size = QSize(-1, 16),
            .m_idle = { .m_fill = solid(0xCCCCCC), .m_edges = BoxElement::border(1, rgb(0x222222)) },
            .m_stateFaces = {
                { BoxElement::Hover, { .m_fill = solid(0xCCCCCC), .m_edges = BoxElement::border(1, rgb(0xFFFFFF)) } },
                { BoxElement::Focus, { .m_fill = solid(0xFFFFFF), .m_edges = BoxElement::border(1, rgb(0x94D2FF)) } },
                { BoxElement::Disabled, { .m_fill = solid(0x666666), .m_edges = BoxElement::border(1, rgb(0x222222)), .m_text = rgb(0x999999) } },
                { BoxElement::Disabled | BoxElement::Focus, { .m_fill = solid(0x666666), .m_edges = BoxElement::border(1, rgb(0x94D2FF)), .m_text = rgb(0x999999) } },
            },
        };

        // ---- A rule that only fills a line edit, a tool button and a combo box: no box and the border of the style, so the base style
        // and the components lay the widget out, as they did for the sheet (qstylesheetstyle.cpp:5336-5350, 5389-5397, 6176-6182)
        const BoxElement FillEdit{ .m_name = "Test.Door.FillEdit", .m_idle = { .m_fill = solid(0x445566) } };
        const BoxElement FillTool{ .m_name = "Test.Door.FillTool", .m_idle = { .m_fill = solid(0x445566) } };
        const BoxElement FillCombo{ .m_name = "Test.Door.FillCombo", .m_idle = { .m_fill = solid(0x445566) } };
        // ... and a gradient under the style's own border: the default sheet of a combo box lists the gradient among the features of the base
        // style (qstylesheetstyle_default.cpp:274-301), which draws the line edit of an editable one over the palette of the gradient
        const BoxElement GradientCombo{ .m_name = "Test.Door.GradientCombo", .m_idle = { .m_fill = BoxElement::vertical(rgb(0xDDDDDD), rgb(0x888888)) } };
        // ... and an image under it: the rule draws that, and the line edit inside draws no panel at all (qstylesheetstyle.cpp:4655-4661)
        BoxElement ImageCombo{ .m_name = "Test.Door.ImageCombo", .m_idle = { .m_fill = solid(0x445566) } };

        // ---- An element for every line edit of a combo box or a spin box: the sheet styled none of them by a rule of its own, whatever its
        // selectors said (qstylesheetstyle.cpp:2499-2506), so such a line edit has no element
        const BoxElement EmbeddedField{
            .m_name = "Test.Door.EmbeddedField",
            .m_margin = QMargins(7, 7, 7, 7),
            .m_nativeBorder = false,
            .m_idle = { .m_fill = solid(0xFF0000), .m_edges = BoxElement::border(3, rgb(0x00FF00)) },
        };

        // ---- An element for every viewport of a scroll area: the same, the sheet styled none by a rule of its own
        const BoxElement EmbeddedViewport{
            .m_name = "Test.Door.EmbeddedViewport",
            .m_nativeBorder = false,
            .m_idle = { .m_fill = solid(0xFF0000) },
        };
    } // namespace BoxElementDoorTestData

    // The tests and their fixture sit in a namespace of their own: the using-directive below must not leak into the
    // other files of a unity build
    namespace BoxElementDoorTestsInternal
    {
        using namespace BoxElementDoorTestData;

        // ----------------------------------------------------------------------------------
        // Fixture: the flattened Style over Fusion, Fusion as the base of the legacy sheet, the elements registered
        // ----------------------------------------------------------------------------------

        class BoxElementDoorTests
            : public ::UnitTest::LeakDetectionFixture
        {
        public:
            BoxElementDoorTests()
            {
                m_application = AZStd::make_unique<QApplication>(m_argc, m_argv);
                QApplication::setStyle(QStyleFactory::create(QStringLiteral("Fusion")));
                m_style = AZStd::make_unique<Style>(QStyleFactory::create(QStringLiteral("Fusion")));
                registerElements();
            }

            ~BoxElementDoorTests() override
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

                BoxElement::registerElement(&Field, &QLineEdit::staticMetaObject, [](const QWidget* w) { return named(w, "doorField"); });
                BoxElement::registerElement(&StateField, &QLineEdit::staticMetaObject, [](const QWidget* w) { return named(w, "doorStateField"); });
                BoxElement::registerElement(&FormField, &QLineEdit::staticMetaObject, [](const QWidget* w) { return named(w, "doorFormField"); });
                BoxElement::registerElement(&NativeField, &QLineEdit::staticMetaObject, [](const QWidget* w) { return named(w, "doorNativeField"); });
                BoxElement::registerElement(&MarginField, &QLineEdit::staticMetaObject, [](const QWidget* w) { return named(w, "doorMarginField"); });
                BoxElement::registerElement(&PaddingField, &QLineEdit::staticMetaObject, [](const QWidget* w) { return named(w, "doorPaddingField"); });
                BoxElement::registerElement(&GradientField, &QLineEdit::staticMetaObject, [](const QWidget* w) { return named(w, "doorGradientField"); });
                BoxElement::registerElement(&ServerTool, &QToolButton::staticMetaObject, [](const QWidget* w) { return named(w, "doorServerTool"); });
                BoxElement::registerElement(&FacedTool, &QToolButton::staticMetaObject, [](const QWidget* w) { return named(w, "doorFacedTool"); });
                BoxElement::registerElement(&NativeTool, &QToolButton::staticMetaObject, [](const QWidget* w) { return named(w, "doorNativeTool"); });
                BoxElement::registerElement(&PressedField, &QLineEdit::staticMetaObject, [](const QWidget* w) { return named(w, "doorPressedField"); });
                BoxElement::registerElement(&SeedCheck, &QCheckBox::staticMetaObject, [](const QWidget* w) { return named(w, "doorSeedCheck"); });
                BoxElement::registerElement(&LhsRadio, &QRadioButton::staticMetaObject, [](const QWidget* w) { return named(w, "doorLhsRadio"); });
                BoxElement::registerElement(&FacedCheck, &QCheckBox::staticMetaObject, [](const QWidget* w) { return named(w, "doorFacedCheck"); });
                BoxElement::registerElement(&FacedCheck, &QRadioButton::staticMetaObject, [](const QWidget* w) { return named(w, "doorFacedRadio"); });
                BoxElement::registerElement(&FilledArea, &QAbstractScrollArea::staticMetaObject, [](const QWidget* w) { return named(w, "doorFilledArea"); });
                BoxElement::registerElement(&BareArea, &QAbstractScrollArea::staticMetaObject, [](const QWidget* w) { return named(w, "doorBareArea"); });
                BoxElement::registerElement(&MarginArea, &QAbstractScrollArea::staticMetaObject, [](const QWidget* w) { return named(w, "doorMarginArea"); });
                ImageArea.m_idle.m_image = image().m_path.constData();
                BoxElement::registerElement(&ImageArea, &QAbstractScrollArea::staticMetaObject, [](const QWidget* w) { return named(w, "doorImageArea"); });
                CenteredImageArea.m_idle.m_image = image().m_path.constData();
                BoxElement::registerElement(&CenteredImageArea, &QAbstractScrollArea::staticMetaObject, [](const QWidget* w) { return named(w, "doorCenteredImageArea"); });
                BoxElement::registerElement(&PlainArea, &QAbstractScrollArea::staticMetaObject, [](const QWidget* w) { return named(w, "doorPlainArea"); });
                BoxElement::registerElement(&ComboField, &QComboBox::staticMetaObject, [](const QWidget* w) { return named(w, "doorCombo") || named(w, "doorEditCombo"); });
                BoxElement::registerElement(&WideCombo, &QComboBox::staticMetaObject, [](const QWidget* w) { return named(w, "doorWideCombo"); });
                BoxElement::registerElement(&RingCombo, &QComboBox::staticMetaObject, [](const QWidget* w) { return named(w, "doorRingCombo"); });
                BoxElement::registerElement(&NativeCombo, &QComboBox::staticMetaObject, [](const QWidget* w) { return named(w, "doorNativeCombo"); });
                BoxElement::registerElement(&FillEdit, &QLineEdit::staticMetaObject, [](const QWidget* w) { return named(w, "doorFillEdit"); });
                BoxElement::registerElement(&FillTool, &QToolButton::staticMetaObject, [](const QWidget* w) { return named(w, "doorFillTool"); });
                BoxElement::registerElement(&FillCombo, &QComboBox::staticMetaObject, [](const QWidget* w) { return named(w, "doorFillCombo"); });
                BoxElement::registerElement(&GradientCombo, &QComboBox::staticMetaObject, [](const QWidget* w) { return named(w, "doorGradientCombo"); });
                ImageCombo.m_idle.m_image = image().m_path.constData();
                BoxElement::registerElement(&ImageCombo, &QComboBox::staticMetaObject, [](const QWidget* w) { return named(w, "doorImageCombo"); });
                BoxElement::registerElement(&EmbeddedField, &QLineEdit::staticMetaObject, [](const QWidget* w)
                    {
                        return w->window()->property("boxElementDoorTestsFlat").toBool()
                            && (qobject_cast<const QComboBox*>(w->parentWidget()) || qobject_cast<const QAbstractSpinBox*>(w->parentWidget()));
                    });
                BoxElement::registerElement(&EmbeddedViewport, &QWidget::staticMetaObject, [](const QWidget* w)
                    {
                        return w->window()->property("boxElementDoorTestsFlat").toBool() && w->metaObject() == &QWidget::staticMetaObject
                            && qobject_cast<const QAbstractScrollArea*>(w->parentWidget());
                    });
            }

            //! The legacy twin: the sheet on the root, over the application's Fusion. The flat twin: the style on the root and on
            //! everything under it (a widget's style is not its children's).
            static void legacy(QWidget& root, const QString& rules)
            {
                root.setStyleSheet(rules);
            }

            void flat(QWidget& root) const
            {
                root.setProperty("boxElementDoorTestsFlat", true);
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
                return widget;
            }

            //! One tree twice, shown: the build function fills a root with it, the rules are the legacy twin's sheet
            struct Twins
            {
                template<typename Build>
                Twins(const BoxElementDoorTests& tests, Build build, const QString& rules, const QSize& size)
                {
                    m_legacy.setLayoutDirection(tests.m_direction);
                    build(m_legacy);
                    legacy(m_legacy, rules);
                    show(m_legacy, size);
                    m_flat.setLayoutDirection(tests.m_direction);
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

                QWidget m_legacy;
                QWidget m_flat;
            };

            //! What a style draws for one call, over a grey image of an explicit device ratio: the painter is the image's, so the
            //! style scales its lines by that ratio (QWidget::render would hand the style the widget, and the ratio of the screen)
            template<typename Draw>
            static QImage drawn(const QSize& size, qreal ratio, Draw draw)
            {
                QImage image((QSizeF(size) * ratio).toSize(), QImage::Format_ARGB32_Premultiplied);
                image.setDevicePixelRatio(ratio);
                image.fill(rgb(0x808080));
                QPainter painter(&image);
                draw(&painter);
                painter.end();
                return image;
            }

            //! The pixels that differ, those of a rect of the logical image left out
            static int differing(const QImage& a, const QImage& b, const QRect& ignored = QRect())
            {
                if (a.size() != b.size())
                {
                    return a.width() * a.height();
                }
                const qreal ratio = a.devicePixelRatio();
                const QRect device(QPoint(qFloor(ignored.left() * ratio), qFloor(ignored.top() * ratio)),
                    QPoint(qCeil((ignored.right() + 1) * ratio) - 1, qCeil((ignored.bottom() + 1) * ratio) - 1));
                int count = 0;
                for (int y = 0; y < a.height(); ++y)
                {
                    for (int x = 0; x < a.width(); ++x)
                    {
                        count += (a.pixel(x, y) != b.pixel(x, y) && !(ignored.isValid() && device.contains(x, y))) ? 1 : 0;
                    }
                }
                return count;
            }

            //! The pixels of an image that are not the grey it started from
            static int touched(const QImage& image)
            {
                int count = 0;
                for (int y = 0; y < image.height(); ++y)
                {
                    for (int x = 0; x < image.width(); ++x)
                    {
                        count += (image.pixel(x, y) != rgb(0x808080).rgb()) ? 1 : 0;
                    }
                }
                return count;
            }

        protected:
            int m_argc = 1;                                     // QApplication keeps a reference to it
            char m_programName[5] = "test";
            char* m_argv[2] = { m_programName, nullptr };
            AZStd::unique_ptr<QApplication> m_application;
            AZStd::unique_ptr<Style> m_style;
            Qt::LayoutDirection m_direction = Qt::LeftToRight;      //!< the direction the twins of a test are laid out in
        };

        //! The colour group of the palette follows the enabled state, as QStyleOption::initFrom gives it for a real widget
        void followsEnabled(QStyleOption& option)
        {
            option.palette.setCurrentColorGroup(option.state.testFlag(QStyle::State_Enabled) ? QPalette::Active : QPalette::Disabled);
        }

        const std::initializer_list<qreal> Ratios = { 1.0, 1.5 };
        const std::initializer_list<Qt::LayoutDirection> Directions = { Qt::LeftToRight, Qt::RightToLeft };       // the twins of a test are laid out in each
        using States = std::initializer_list<QStyle::State>;

        //! A rect as text, for the failure message of a comparison
        std::string rectText(const QRect& rect)
        {
            return QStringLiteral("(%1, %2, %3 x %4)").arg(rect.x()).arg(rect.y()).arg(rect.width()).arg(rect.height()).toStdString();
        }

        //! Nothing resolved for the widget, nothing kept on it
        void expectNoElement(const QWidget* widget)
        {
            EXPECT_TRUE(BoxElement::forWidget(widget) == nullptr) << widget->objectName().toStdString();
            EXPECT_FALSE(widget->dynamicPropertyNames().contains(QByteArrayLiteral("_o3de_boxElement"))) << widget->objectName().toStdString();
        }

        // ----------------------------------------------------------------------------------
        // T1: the line edit: the box reaches Qt as the contents rect of the field, the hint and the panel
        // ----------------------------------------------------------------------------------

        //! The option QLineEdit::initStyleOption builds (qlineedit.cpp:75-94), in a state and a rect
        QStyleOptionFrame fieldOption(const QLineEdit* lineEdit, QStyle::State state, const QRect& rect)
        {
            QStyleOptionFrame option;
            option.initFrom(lineEdit);
            option.rect = rect;
            option.state = state | QStyle::State_Sunken;
            option.lineWidth = lineEdit->hasFrame() ? lineEdit->style()->pixelMetric(QStyle::PM_DefaultFrameWidth, &option, lineEdit) : 0;
            option.midLineWidth = 0;
            followsEnabled(option);
            return option;
        }

        //! The widget as it paints itself and its children
        QImage rendered(QWidget& root)
        {
            QImage image(root.size(), QImage::Format_ARGB32_Premultiplied);
            image.fill(Qt::transparent);
            root.render(&image, QPoint(), QRegion(), QWidget::DrawChildren);
            return image;
        }

        TEST_F(BoxElementDoorTests, LineEdit_TheFieldHasTheSheetsHintsLimitsContentsRectAndPanel)
        {
            for (const Qt::LayoutDirection direction : Directions)
            {
                SCOPED_TRACE(direction == Qt::RightToLeft ? "right to left" : "left to right");
                m_direction = direction;

                ASSERT_TRUE(StyleManager::stylesheetsDisabled());   // the test environment sets it (AzQtComponentTests.cpp)

                Twins twins(*this, [](QWidget& root) { add<QLineEdit>(root, "doorField", QRect(5, 5, 120, 20))->setText(QStringLiteral("Text")); },
                    QString::fromUtf8(FieldRules), QSize(140, 34));
                QLineEdit* legacyEdit = twins.legacyChild<QLineEdit>("doorField");
                QLineEdit* flatEdit = twins.flatChild<QLineEdit>("doorField");
                ASSERT_TRUE(BoxElement::forWidget(flatEdit) == &Field);
                ASSERT_TRUE(BoxElement::forWidget(legacyEdit) == nullptr);
                // The polish gave the field the palette of the text entries (black text), the sheet's twin has the application's, which
                // is white text on a machine in dark mode: the text colour is not the box
                flatEdit->setPalette(QPalette());

                // The Native numbers: 16px high inside the 2px ring and a padding of -1 above and 1 below make 20
                EXPECT_EQ(legacyEdit->sizeHint().height(), 20);
                EXPECT_EQ(flatEdit->sizeHint(), legacyEdit->sizeHint());
                EXPECT_EQ(flatEdit->minimumSizeHint(), legacyEdit->minimumSizeHint());
                EXPECT_EQ(flatEdit->minimumSize(), legacyEdit->minimumSize());
                EXPECT_EQ(flatEdit->maximumSize(), legacyEdit->maximumSize());

                // The rect QLineEdit lays its text out in
                for (const QRect& rect : { QRect(0, 0, 120, 20), QRect(3, 4, 77, 31) })
                {
                    const QStyleOptionFrame legacyOption = fieldOption(legacyEdit, QStyle::State_Enabled, rect);
                    const QStyleOptionFrame flatOption = fieldOption(flatEdit, QStyle::State_Enabled, rect);
                    const QRect expected = legacyEdit->style()->subElementRect(QStyle::SE_LineEditContents, &legacyOption, legacyEdit);
                    EXPECT_EQ(flatEdit->style()->subElementRect(QStyle::SE_LineEditContents, &flatOption, flatEdit), expected) << rectText(rect);
                    EXPECT_EQ(expected, rect.marginsRemoved(QMargins(2, 1, 2, 3))) << rectText(rect);       // border 2 + padding -1 above and 1 below
                }

                // The panel, in each state and at each ratio; the field is disabled when the widget is, whatever the state of the option
                const QStyle::State e = QStyle::State_Enabled;
                for (const qreal ratio : Ratios)
                {
                    for (const QStyle::State state : States{ e, e | QStyle::State_MouseOver, e | QStyle::State_HasFocus, QStyle::State_None, QStyle::State_HasFocus })
                    {
                        const auto panel = [&](const QLineEdit* lineEdit)
                        {
                            return drawn(QSize(60, 20), ratio, [&](QPainter* painter)
                            {
                                const QStyleOptionFrame option = fieldOption(lineEdit, state, QRect(0, 0, 60, 20));
                                lineEdit->style()->drawPrimitive(QStyle::PE_PanelLineEdit, &option, painter, lineEdit);
                            });
                        };
                        EXPECT_EQ(differing(panel(flatEdit), panel(legacyEdit)), 0) << "panel in state " << int(state) << " at ratio " << ratio;
                    }
                }

                // The whole widget, as it paints itself: the panel, then the text in the rect above
                EXPECT_EQ(differing(rendered(twins.m_flat), rendered(twins.m_legacy)), 0);
            }
        }

        struct LineEditCase
        {
            const char* m_name;
            QString m_rules;
            const BoxElement* m_element;
            bool m_frame = true;            //!< QLineEdit::hasFrame: the sheet's default rule gives a field without one `border: none`
        };

        //! The flat field and its legacy twin are the same in hints, limits, contents rect and panel (every state at both ratios),
        //! and as the widget paints itself
        TEST_F(BoxElementDoorTests, LineEdit_TheVariantsOfAFieldAreTheSheetsInEveryStateAtBothRatios)
        {
            for (const Qt::LayoutDirection direction : Directions)
            {
                SCOPED_TRACE(direction == Qt::RightToLeft ? "right to left" : "left to right");
                m_direction = direction;

                const QStyle::State e = QStyle::State_Enabled;
                const QStyle::State h = QStyle::State_MouseOver;
                const QStyle::State f = QStyle::State_HasFocus;
                const States states = { e, e | h, e | f, e | h | f, QStyle::State(QStyle::State_None), f };
                const LineEditCase cases[] = {
                    { "doorStateField", QString::fromUtf8(StateFieldRules), &StateField },
                    { "doorFormField", QString::fromUtf8(FormFieldRules), &FormField },
                    { "doorNativeField", QString::fromUtf8(NativeFieldRules), &NativeField },
                    { "doorMarginField", QString::fromUtf8(NativeFieldRules), &MarginField },
                    { "doorPaddingField", QString::fromUtf8(NativeFieldRules), &PaddingField },
                    { "doorGradientField", QString::fromUtf8(NativeFieldRules), &GradientField },
                    { "doorPressedField", QString::fromUtf8(PressedFieldRules), &PressedField },
                    { "doorFillEdit", QStringLiteral("#doorFillEdit { background-color: #445566; }"), &FillEdit },
                    // no frame: a border of the style's own is gone (qstylesheetstyle_default.cpp:155-169), a declared one stays
                    { "doorNativeField", QString::fromUtf8(NativeFieldRules), &NativeField, false },
                    { "doorFillEdit", QStringLiteral("#doorFillEdit { background-color: #445566; }"), &FillEdit, false },
                    { "doorStateField", QString::fromUtf8(StateFieldRules), &StateField, false },
                };
                for (const LineEditCase& test : cases)
                {
                    Twins twins(*this, [&](QWidget& root) { auto* edit = add<QLineEdit>(root, test.m_name, QRect(5, 5, 100, 30)); edit->setText(QStringLiteral("Text")); edit->setFrame(test.m_frame); },
                        test.m_rules, QSize(120, 44));
                    QLineEdit* legacyEdit = twins.legacyChild<QLineEdit>(test.m_name);
                    QLineEdit* flatEdit = twins.flatChild<QLineEdit>(test.m_name);
                    ASSERT_TRUE(BoxElement::forWidget(flatEdit) == test.m_element) << test.m_name;
                    flatEdit->setPalette(QPalette());       // the polish gave the field the palette of the text entries (a Base of #CCCCCC), the sheet's twin has the application's

                    EXPECT_EQ(flatEdit->sizeHint(), legacyEdit->sizeHint()) << test.m_name;
                    EXPECT_EQ(flatEdit->minimumSizeHint(), legacyEdit->minimumSizeHint()) << test.m_name;
                    EXPECT_EQ(flatEdit->minimumSize(), legacyEdit->minimumSize()) << test.m_name;
                    EXPECT_EQ(flatEdit->maximumSize(), legacyEdit->maximumSize()) << test.m_name;

                    for (const QRect& rect : { QRect(0, 0, 100, 30), QRect(3, 4, 77, 31) })
                    {
                        const QStyleOptionFrame legacyOption = fieldOption(legacyEdit, e, rect);
                        const QStyleOptionFrame flatOption = fieldOption(flatEdit, e, rect);
                        EXPECT_EQ(flatEdit->style()->subElementRect(QStyle::SE_LineEditContents, &flatOption, flatEdit),
                            legacyEdit->style()->subElementRect(QStyle::SE_LineEditContents, &legacyOption, legacyEdit)) << test.m_name << " " << rectText(rect);
                    }

                    for (const qreal ratio : Ratios)
                    {
                        for (const QStyle::State state : states)
                        {
                            const auto panel = [&](const QLineEdit* lineEdit)
                            {
                                return drawn(QSize(90, 30), ratio, [&](QPainter* painter)
                                {
                                    const QStyleOptionFrame option = fieldOption(lineEdit, state, QRect(0, 0, 90, 30));
                                    lineEdit->style()->drawPrimitive(QStyle::PE_PanelLineEdit, &option, painter, lineEdit);
                                });
                            };
                            EXPECT_EQ(differing(panel(flatEdit), panel(legacyEdit)), 0) << test.m_name << " in state " << int(state) << " at ratio " << ratio;
                        }
                    }
                    EXPECT_EQ(differing(rendered(twins.m_flat), rendered(twins.m_legacy)), 0) << test.m_name;
                }
            }
        }

        // ----------------------------------------------------------------------------------
        // T2: the tool button: the rule's box around its label, the drop-down and the menu indicator the base style adds, its hint
        // ----------------------------------------------------------------------------------

        //! A tool button that tells the option it paints with (QToolButton::initStyleOption is protected), in a state and a rect
        class OptionToolButton
            : public QToolButton
        {
        public:
            using QToolButton::QToolButton;

            QStyleOptionToolButton option(QStyle::State state, const QRect& rect) const
            {
                QStyleOptionToolButton option;
                initStyleOption(&option);
                option.rect = rect;
                option.state = state;
                followsEnabled(option);
                return option;
            }
        };

        QIcon swatchIcon(int color)
        {
            QPixmap pixmap(16, 16);
            pixmap.fill(rgb(color));
            return QIcon(pixmap);
        }

        //! What a style draws for a primitive of a tool button with a menu, in a 16 x 16 rect
        QImage menuPrimitive(const QStyle* style, QStyle::PrimitiveElement element, const QToolButton* button)
        {
            return BoxElementDoorTests::drawn(QSize(16, 16), 1.0, [&](QPainter* painter)
            {
                QStyleOptionToolButton option;      // the same palette and state for both: the painters are compared, not the widgets
                option.rect = QRect(0, 0, 16, 16);
                option.state = QStyle::State_Enabled | QStyle::State_Raised;
                style->drawPrimitive(element, &option, painter, button);
            });
        }

        //! Whether the Style draws the arrow of a menu and the drop-down of a split button as the style under the legacy twin does. The
        //! door chooses their rects and states; the base style paints them, and the component library of the Style may paint its own
        //! icon where Fusion paints a triangle. The pixels of their rects are compared where the two paint alike.
        bool drawsMenuAlike(const QToolButton* flatButton, const QToolButton* legacyButton)
        {
            for (const QStyle::PrimitiveElement element : { QStyle::PE_IndicatorArrowDown, QStyle::PE_IndicatorButtonDropDown })
            {
                if (BoxElementDoorTests::differing(menuPrimitive(flatButton->style(), element, flatButton),
                        menuPrimitive(QApplication::style(), element, legacyButton)) != 0)
                {
                    return false;
                }
            }
            return true;
        }

        //! What a menu adds to a tool button: the pixels that differ between the button drawn with it and without it
        QRect addedByMenu(const OptionToolButton* button)
        {
            const auto paint = [&](bool withMenu)
            {
                return BoxElementDoorTests::drawn(QSize(80, 40), 1.0, [&](QPainter* painter)
                {
                    QStyleOptionToolButton option = button->option(QStyle::State_Enabled | QStyle::State_Raised, QRect(0, 0, 80, 40));
                    if (!withMenu)
                    {
                        option.features &= ~(QStyleOptionToolButton::Menu | QStyleOptionToolButton::HasMenu);
                    }
                    button->style()->drawComplexControl(QStyle::CC_ToolButton, &option, painter, button);
                });
            };
            const QImage with = paint(true);
            const QImage without = paint(false);
            QRect added;
            for (int y = 0; y < with.height(); ++y)
            {
                for (int x = 0; x < with.width(); ++x)
                {
                    if (with.pixel(x, y) != without.pixel(x, y))
                    {
                        added = added.united(QRect(x, y, 1, 1));
                    }
                }
            }
            return added;
        }

        TEST_F(BoxElementDoorTests, ToolButton_TheBoxOfTheRuleHasTheSheetsHintsRectsAndPixelsInEveryState)
        {
            for (const Qt::LayoutDirection direction : Directions)
            {
                SCOPED_TRACE(direction == Qt::RightToLeft ? "right to left" : "left to right");
                m_direction = direction;

                const QStyle::State e = QStyle::State_Enabled | QStyle::State_Raised;
                const QStyle::State h = QStyle::State_MouseOver;
                const QStyle::State s = QStyle::State_Sunken;
                const QStyle::State on = QStyle::State_On;
                const QStyle::State f = QStyle::State_HasFocus;
                const States states = { e, e | h, e | h | s, e | s, e | on, e | on | h, e | f, QStyle::State(QStyle::State_Raised) };

                struct ToolCase
                {
                    const char* m_name;
                    const QString m_rules;
                    const BoxElement* m_element;
                    Qt::ToolButtonStyle m_style;
                    QToolButton::ToolButtonPopupMode m_popup;
                    bool m_menu;
                    QRect m_indicator;          //!< where a button with a menu and no drop-down shows its arrow, if it has a menu
                    Qt::ArrowType m_arrow = Qt::NoArrow;
                };
                const ToolCase cases[] = {
                    { "doorServerTool", QString::fromUtf8(ServerToolRules), &ServerTool, Qt::ToolButtonTextOnly, QToolButton::DelayedPopup, false },
                    { "doorServerTool", QString::fromUtf8(ServerToolRules), &ServerTool, Qt::ToolButtonIconOnly, QToolButton::DelayedPopup, false },
                    { "doorFacedTool", QString::fromUtf8(FacedToolRules), &FacedTool, Qt::ToolButtonTextBesideIcon, QToolButton::DelayedPopup, false },
                    { "doorFacedTool", QString::fromUtf8(FacedToolRules), &FacedTool, Qt::ToolButtonTextUnderIcon, QToolButton::DelayedPopup, false },
                    // 13 x 13 (qstylesheetstyle.cpp:2307-2319) at the bottom right of the padding box: 80 x 40 less the margin 2 and the border 1
                    { "doorFacedTool", QString::fromUtf8(FacedToolRules), &FacedTool, Qt::ToolButtonIconOnly, QToolButton::InstantPopup, true, QRect(64, 24, 13, 13) },
                    { "doorFacedTool", QString::fromUtf8(FacedToolRules), &FacedTool, Qt::ToolButtonTextBesideIcon, QToolButton::MenuButtonPopup, true },
                    { "doorNativeTool", QString::fromUtf8(NativeToolRules), &NativeTool, Qt::ToolButtonIconOnly, QToolButton::DelayedPopup, false },
                    { "doorFacedTool", QString::fromUtf8(FacedToolRules), &FacedTool, Qt::ToolButtonIconOnly, QToolButton::DelayedPopup, false, QRect(), Qt::DownArrow },
                    { "doorFacedTool", QString::fromUtf8(FacedToolRules), &FacedTool, Qt::ToolButtonTextBesideIcon, QToolButton::DelayedPopup, false, QRect(), Qt::RightArrow },
                };
                QMenu menu;
                menu.addAction(QStringLiteral("One"));
                for (const ToolCase& test : cases)
                {
                    Twins twins(*this, [&](QWidget& root)
                    {
                        auto* button = add<OptionToolButton>(root, test.m_name, QRect(5, 5, 80, 40));
                        button->setIcon(swatchIcon(0xFF8000));
                        button->setIconSize(QSize(16, 16));
                        button->setText(QStringLiteral("Tool"));
                        button->setToolButtonStyle(test.m_style);
                        button->setArrowType(test.m_arrow);
                        button->setCheckable(true);
                        if (test.m_menu)
                        {
                            button->setMenu(&menu);
                            button->setPopupMode(test.m_popup);
                        }
                    }, test.m_rules, QSize(100, 54));
                    const auto* legacyButton = static_cast<const OptionToolButton*>(twins.legacyChild<QToolButton>(test.m_name));
                    const auto* flatButton = static_cast<const OptionToolButton*>(twins.flatChild<QToolButton>(test.m_name));
                    const QString what = QStringLiteral("%1 style %2 popup %3").arg(QLatin1String(test.m_name)).arg(int(test.m_style)).arg(int(test.m_popup));
                    ASSERT_TRUE(BoxElement::forWidget(flatButton) == test.m_element) << what.toStdString();

                    EXPECT_EQ(flatButton->sizeHint(), legacyButton->sizeHint()) << what.toStdString();
                    EXPECT_EQ(flatButton->minimumSizeHint(), legacyButton->minimumSizeHint()) << what.toStdString();
                    EXPECT_EQ(flatButton->minimumSize(), legacyButton->minimumSize()) << what.toStdString();
                    EXPECT_EQ(flatButton->maximumSize(), legacyButton->maximumSize()) << what.toStdString();

                    // the sub-controls, in a rect other than the button's own. The menu button is as wide as the base style's
                    // PM_MenuButtonIndicator for a tool button (qstylesheetstyle.cpp:2329-2332): Fusion's under the legacy twin, the one the
                    // components give a tool button under the Style
                    QToolButton plainButton;
                    plainButton.setStyle(m_style.get());
                    const int menuWidth = m_style->pixelMetric(QStyle::PM_MenuButtonIndicator, nullptr, &plainButton);
                    for (const QRect& rect : { QRect(0, 0, 80, 40), QRect(3, 4, 61, 30) })
                    {
                        const QStyleOptionToolButton legacyOption = legacyButton->option(e, rect);
                        const QStyleOptionToolButton flatOption = flatButton->option(e, rect);
                        for (const QStyle::SubControl control : { QStyle::SC_ToolButton, QStyle::SC_ToolButtonMenu })
                        {
                            QRect expected = legacyButton->style()->subControlRect(QStyle::CC_ToolButton, &legacyOption, control, legacyButton);
                            if (control == QStyle::SC_ToolButtonMenu)
                            {
                                if (direction == Qt::LeftToRight)
                                {
                                    expected.setLeft(expected.right() + 1 - menuWidth);
                                }
                                else
                                {
                                    expected.setRight(expected.left() + menuWidth - 1);
                                }
                            }
                            EXPECT_EQ(flatButton->style()->subControlRect(QStyle::CC_ToolButton, &flatOption, control, flatButton), expected)
                                << what.toStdString() << " " << int(control) << " in " << rectText(rect);
                        }
                    }

                    // the pixels exist for a border of the rule's own; a border of the style's is the style's to draw
                    if (test.m_element == &NativeTool)
                    {
                        continue;
                    }
                    // The arrow of a menu and the drop-down of a split button are the base style's to paint, in the width it gives them. Where
                    // the base style under the Style paints them otherwise than the one under the legacy twin (its component's icon for
                    // Fusion's triangle), or gives the drop-down another width, the pixels of their rects are not the door's to compare.
                    QRect menuPixels;
                    if (test.m_menu)
                    {
                        const QStyleOptionToolButton legacyOption = legacyButton->option(e, QRect(0, 0, 80, 40));
                        const QStyleOptionToolButton flatOption = flatButton->option(e, QRect(0, 0, 80, 40));
                        const QRect legacyMenu = legacyButton->style()->subControlRect(QStyle::CC_ToolButton, &legacyOption, QStyle::SC_ToolButtonMenu, legacyButton);
                        const QRect flatMenu = flatButton->style()->subControlRect(QStyle::CC_ToolButton, &flatOption, QStyle::SC_ToolButtonMenu, flatButton);
                        const bool split = (test.m_popup == QToolButton::MenuButtonPopup);
                        if (!drawsMenuAlike(flatButton, legacyButton) || (split && legacyMenu.width() != flatMenu.width()))
                        {
                            menuPixels = split ? legacyMenu.united(flatMenu) : QStyle::visualRect(direction, QRect(0, 0, 80, 40), test.m_indicator);
                        }
                    }
                    for (const qreal ratio : Ratios)
                    {
                        for (const QStyle::State state : states)
                        {
                            const auto button = [&](const OptionToolButton* tool)
                            {
                                return drawn(QSize(80, 40), ratio, [&](QPainter* painter)
                                {
                                    const QStyleOptionToolButton option = tool->option(state, QRect(0, 0, 80, 40));
                                    tool->style()->drawComplexControl(QStyle::CC_ToolButton, &option, painter, tool);
                                });
                            };
                            EXPECT_EQ(differing(button(flatButton), button(legacyButton), menuPixels), 0)
                                << what.toStdString() << " in state " << int(state) << " at ratio " << ratio;
                        }
                    }

                    // The arrow of a menu lies in the rect the sheet gave it whatever glyph the style draws there (Fusion a triangle, the
                    // components of the Style an icon), which the pixels above leave out: what the menu adds to the button has the same
                    // centre in the twins
                    if (test.m_menu && test.m_popup != QToolButton::MenuButtonPopup)
                    {
                        const QRect legacyArrow = addedByMenu(legacyButton);
                        const QRect flatArrow = addedByMenu(flatButton);
                        ASSERT_FALSE(legacyArrow.isNull() || flatArrow.isNull()) << what.toStdString();
                        EXPECT_LE(qAbs(flatArrow.center().x() - legacyArrow.center().x()), 1) << what.toStdString();
                        EXPECT_LE(qAbs(flatArrow.center().y() - legacyArrow.center().y()), 1) << what.toStdString();
                    }

                    // The drop-down of a split button is a panel over the whole rect of the menu button, in the twins: that rect is what the
                    // menu adds to the button
                    if (test.m_menu && test.m_popup == QToolButton::MenuButtonPopup)
                    {
                        const QStyleOptionToolButton legacyOption = legacyButton->option(e, QRect(0, 0, 80, 40));
                        const QStyleOptionToolButton flatOption = flatButton->option(e, QRect(0, 0, 80, 40));
                        EXPECT_EQ(addedByMenu(legacyButton),
                            legacyButton->style()->subControlRect(QStyle::CC_ToolButton, &legacyOption, QStyle::SC_ToolButtonMenu, legacyButton)) << what.toStdString();
                        EXPECT_EQ(addedByMenu(flatButton),
                            flatButton->style()->subControlRect(QStyle::CC_ToolButton, &flatOption, QStyle::SC_ToolButtonMenu, flatButton)) << what.toStdString();
                    }
                }
            }
        }

        // ----------------------------------------------------------------------------------
        // T3: the check box and the radio button: the box of the rule around the indicator element and the label
        // ----------------------------------------------------------------------------------

        //! A check box and a radio button that tell the option they paint with (the initStyleOption of both is protected)
        class OptionCheckBox
            : public QCheckBox
        {
        public:
            using QCheckBox::QCheckBox;

            QStyleOptionButton option(QStyle::State state, const QRect& rect) const
            {
                QStyleOptionButton option;
                initStyleOption(&option);
                option.rect = rect;
                option.state = state;
                followsEnabled(option);
                return option;
            }
        };

        class OptionRadioButton
            : public QRadioButton
        {
        public:
            using QRadioButton::QRadioButton;

            QStyleOptionButton option(QStyle::State state, const QRect& rect) const
            {
                QStyleOptionButton option;
                initStyleOption(&option);
                option.rect = rect;
                option.state = state;
                followsEnabled(option);
                return option;
            }
        };

        struct ButtonCase
        {
            const char* m_name;
            QString m_rules;
            const BoxElement* m_element;
            bool m_radio;
            QString m_text = QStringLiteral("Label text");
        };

        TEST_F(BoxElementDoorTests, Buttons_TheBoxOfTheRuleAroundTheIndicatorHasTheSheetsHintsRectsAndPixels)
        {
            for (const Qt::LayoutDirection direction : Directions)
            {
                SCOPED_TRACE(direction == Qt::RightToLeft ? "right to left" : "left to right");
                m_direction = direction;

                const QStyle::State e = QStyle::State_Enabled;
                const QStyle::State h = QStyle::State_MouseOver;
                const QStyle::State f = QStyle::State_HasFocus;
                const QStyle::State on = QStyle::State_On;
                const QStyle::State s = QStyle::State_Sunken;
                const States states = { e | QStyle::State_Off, e | QStyle::State_Off | h, e | QStyle::State_Off | f, e | on, e | on | h, e | on | f,
                    e | QStyle::State_Off | s | h, QStyle::State(QStyle::State_Off), QStyle::State(on) };
                const QString indicator = QString::fromUtf8(IndicatorRules);
                const QString faced = QString::fromUtf8(FacedCheckRules);
                const ButtonCase cases[] = {
                    { "doorSeedCheck", indicator + QString::fromUtf8(SeedCheckRules), &SeedCheck, false },
                    { "doorLhsRadio", indicator + QString::fromUtf8(LhsRadioRules), &LhsRadio, true },
                    { "doorFacedCheck", indicator + faced, &FacedCheck, false },
                    { "doorFacedRadio", indicator + QString(faced).replace(QLatin1String("doorFacedCheck"), QLatin1String("doorFacedRadio")), &FacedCheck, true },
                    { "doorFacedCheck", indicator + faced, &FacedCheck, false, QString() },
                    { "doorSeedCheck", indicator + QString::fromUtf8(SeedCheckRules), &SeedCheck, false, QString() },
                    { "doorLhsRadio", indicator + QString::fromUtf8(LhsRadioRules), &LhsRadio, true, QString() },
                };
                for (const ButtonCase& test : cases)
                {
                    Twins twins(*this, [&](QWidget& root)
                    {
                        QAbstractButton* button = test.m_radio ? static_cast<QAbstractButton*>(add<OptionRadioButton>(root, test.m_name, QRect(5, 5, 200, 44)))
                                                                : static_cast<QAbstractButton*>(add<OptionCheckBox>(root, test.m_name, QRect(5, 5, 200, 44)));
                        button->setText(test.m_text);
                    }, test.m_rules, QSize(210, 54));
                    const auto option = [&](const QAbstractButton* button, QStyle::State state, const QRect& rect)
                    {
                        return test.m_radio ? static_cast<const OptionRadioButton*>(button)->option(state, rect)
                                            : static_cast<const OptionCheckBox*>(button)->option(state, rect);
                    };
                    const QAbstractButton* legacyButton = test.m_radio ? static_cast<QAbstractButton*>(twins.legacyChild<QRadioButton>(test.m_name))
                                                                        : static_cast<QAbstractButton*>(twins.legacyChild<QCheckBox>(test.m_name));
                    const QAbstractButton* flatButton = test.m_radio ? static_cast<QAbstractButton*>(twins.flatChild<QRadioButton>(test.m_name))
                                                                      : static_cast<QAbstractButton*>(twins.flatChild<QCheckBox>(test.m_name));
                    ASSERT_TRUE(BoxElement::forWidget(flatButton) == test.m_element) << test.m_name;

                    EXPECT_EQ(flatButton->sizeHint(), legacyButton->sizeHint()) << test.m_name;
                    EXPECT_EQ(flatButton->minimumSizeHint(), legacyButton->minimumSizeHint()) << test.m_name;
                    EXPECT_EQ(flatButton->minimumSize(), legacyButton->minimumSize()) << test.m_name;
                    EXPECT_EQ(flatButton->maximumSize(), legacyButton->maximumSize()) << test.m_name;

                    // where the indicator and the label are, and where a click toggles the button
                    const QStyle::SubElement indicatorElement = test.m_radio ? QStyle::SE_RadioButtonIndicator : QStyle::SE_CheckBoxIndicator;
                    const QStyle::SubElement contentsElement = test.m_radio ? QStyle::SE_RadioButtonContents : QStyle::SE_CheckBoxContents;
                    const QStyle::SubElement clickElement = test.m_radio ? QStyle::SE_RadioButtonClickRect : QStyle::SE_CheckBoxClickRect;
                    for (const QRect& rect : { QRect(0, 0, 200, 44), QRect(3, 4, 150, 31) })
                    {
                        const QStyleOptionButton legacyOption = option(legacyButton, e, rect);
                        const QStyleOptionButton flatOption = option(flatButton, e, rect);
                        for (const QStyle::SubElement element : { indicatorElement, contentsElement, clickElement })
                        {
                            EXPECT_EQ(flatButton->style()->subElementRect(element, &flatOption, flatButton),
                                legacyButton->style()->subElementRect(element, &legacyOption, legacyButton)) << test.m_name << " " << int(element) << " in " << rectText(rect);
                        }
                    }

                    // the pixels around the indicator, whose image the indicator element draws
                    const QStyleOptionButton whole = option(legacyButton, e, QRect(0, 0, 200, 44));
                    const QRect indicatorBox = legacyButton->style()->subElementRect(indicatorElement, &whole, legacyButton);
                    for (const qreal ratio : Ratios)
                    {
                        for (const QStyle::State state : states)
                        {
                            const auto button = [&](const QAbstractButton* check)
                            {
                                return drawn(QSize(200, 44), ratio, [&](QPainter* painter)
                                {
                                    const QStyleOptionButton paint = option(check, state, QRect(0, 0, 200, 44));
                                    check->style()->drawControl(test.m_radio ? QStyle::CE_RadioButton : QStyle::CE_CheckBox, &paint, painter, check);
                                });
                            };
                            const QImage flatImage = button(flatButton);
                            EXPECT_EQ(differing(flatImage, button(legacyButton), indicatorBox), 0) << test.m_name << " in state " << int(state) << " at ratio " << ratio;
                            EXPECT_GT(touched(flatImage), 0) << test.m_name << " in state " << int(state);
                        }
                    }
                }
            }
        }

        // ----------------------------------------------------------------------------------
        // T4: the scroll area: the box of the rule around its viewport, the fill under it
        // ----------------------------------------------------------------------------------

        //! A scroll bar whose hint is the same under any style: its extent is the style's (PM_ScrollBarExtent), which differs between the
        //! legacy twin's base style and the Style (ScrollBar.qss gave it 8px), and the viewport sits where the visible bars leave room
        class FixedScrollBar
            : public QScrollBar
        {
        public:
            using QScrollBar::QScrollBar;

            QSize sizeHint() const override
            {
                return orientation() == Qt::Vertical ? QSize(8, 56) : QSize(56, 8);
            }
        };

        //! A scroll area as a window paints it (QWidgetPrivate::drawWidget), through the style it has, at an explicit device ratio: its
        //! styled background (PE_Widget), its frame (CE_ShapedFrame), then the same of its viewport at the viewport's place
        QImage paintedArea(const QAbstractScrollArea* area, QStyle::State state, qreal ratio)
        {
            return BoxElementDoorTests::drawn(area->size(), ratio, [&](QPainter* painter)
            {
                const auto paintWidget = [&](const QWidget* widget, const QPoint& at)
                {
                    painter->save();
                    painter->translate(at);
                    painter->setClipRect(widget->rect());
                    if (widget->testAttribute(Qt::WA_StyledBackground))
                    {
                        QStyleOption background;
                        background.initFrom(widget);
                        background.state = state;
                        widget->style()->drawPrimitive(QStyle::PE_Widget, &background, painter, widget);
                    }
                    if (auto frame = qobject_cast<const QFrame*>(widget))
                    {
                        QStyleOptionFrame option;
                        option.initFrom(frame);
                        option.state = state;
                        option.frameShape = frame->frameShape();
                        option.lineWidth = frame->lineWidth();
                        option.midLineWidth = frame->midLineWidth();
                        widget->style()->drawControl(QStyle::CE_ShapedFrame, &option, painter, frame);
                    }
                    painter->restore();
                };
                paintWidget(area, QPoint());
                paintWidget(area->viewport(), area->viewport()->pos());
            });
        }

        TEST_F(BoxElementDoorTests, ScrollArea_TheBoxOfTheRuleHasTheSheetsViewportContentsAndPixels)
        {
            for (const Qt::LayoutDirection direction : Directions)
            {
                SCOPED_TRACE(direction == Qt::RightToLeft ? "right to left" : "left to right");
                m_direction = direction;

                const QStyle::State e = QStyle::State_Enabled;
                const QStyle::State h = QStyle::State_MouseOver;
                const States states = { e, e | h, QStyle::State(QStyle::State_None) };
                struct AreaCase
                {
                    const char* m_name;
                    const BoxElement* m_element;
                    QFrame::Shape m_shape = QFrame::StyledPanel;
                };
                const AreaCase cases[] = { { "doorFilledArea", &FilledArea }, { "doorBareArea", &BareArea }, { "doorMarginArea", &MarginArea },
                    { "doorPlainArea", &PlainArea }, { "doorImageArea", &ImageArea }, { "doorCenteredImageArea", &CenteredImageArea },
                    // no frame: the border of the style's own is none wide (qstylesheetstyle.cpp:1745-1747), a declared one stays
                    { "doorMarginArea", &MarginArea, QFrame::NoFrame }, { "doorPlainArea", &PlainArea, QFrame::NoFrame },
                    { "doorBareArea", &BareArea, QFrame::NoFrame } };
                for (const AreaCase& test : cases)
                {
                    Twins twins(*this, [&](QWidget& root)
                    {
                        // A plain scroll area, which the style's polish leaves as it finds it (a QScrollArea has its viewport made clear)
                        QAbstractScrollArea* area = add<QAbstractScrollArea>(root, test.m_name, QRect(5, 5, 160, 90));
                        area->setFrameShape(test.m_shape);
                        area->setVerticalScrollBar(new FixedScrollBar(Qt::Vertical));
                        area->setHorizontalScrollBar(new FixedScrollBar(Qt::Horizontal));
                        area->verticalScrollBar()->setRange(0, 200);
                        area->verticalScrollBar()->setPageStep(80);
                        area->horizontalScrollBar()->setRange(0, 200);
                        area->horizontalScrollBar()->setPageStep(100);
                        area->verticalScrollBar()->setValue(7);
                        area->horizontalScrollBar()->setValue(11);
                    }, QString::fromUtf8(AreaRules) + QString::fromUtf8(PlainAreaRules) + imageAreaRules() + centeredImageAreaRules(), QSize(170, 100));
                    const QAbstractScrollArea* legacyArea = twins.legacyChild<QAbstractScrollArea>(test.m_name);
                    const QAbstractScrollArea* flatArea = twins.flatChild<QAbstractScrollArea>(test.m_name);
                    ASSERT_TRUE(BoxElement::forWidget(flatArea) == test.m_element) << test.m_name;
                    expectNoElement(flatArea->viewport());

                    // the viewport sits where the box leaves room for it, and the area is as big as the box and the contents make it
                    EXPECT_EQ(flatArea->viewport()->geometry(), legacyArea->viewport()->geometry()) << test.m_name;
                    EXPECT_EQ(flatArea->frameWidth(), legacyArea->frameWidth()) << test.m_name;
                    EXPECT_EQ(flatArea->sizeHint(), legacyArea->sizeHint()) << test.m_name;
                    EXPECT_EQ(flatArea->minimumSizeHint(), legacyArea->minimumSizeHint()) << test.m_name;
                    EXPECT_EQ(flatArea->viewport()->autoFillBackground(), legacyArea->viewport()->autoFillBackground()) << test.m_name;
                    EXPECT_EQ(flatArea->viewport()->testAttribute(Qt::WA_StyledBackground), legacyArea->viewport()->testAttribute(Qt::WA_StyledBackground)) << test.m_name;
                    EXPECT_EQ(flatArea->testAttribute(Qt::WA_StyledBackground), legacyArea->testAttribute(Qt::WA_StyledBackground)) << test.m_name;

                    for (const qreal ratio : Ratios)
                    {
                        for (const QStyle::State state : states)
                        {
                            EXPECT_EQ(differing(paintedArea(flatArea, state, ratio), paintedArea(legacyArea, state, ratio)), 0)
                                << test.m_name << " in state " << int(state) << " at ratio " << ratio;
                        }
                    }

                    // The whole area as it paints itself, without scroll bars: the sheet's fill is also the palette of the scroll bars (qstylesheetstyle.cpp:2680-2727)
                    for (QAbstractScrollArea* area : { twins.legacyChild<QAbstractScrollArea>(test.m_name), twins.flatChild<QAbstractScrollArea>(test.m_name) })
                    {
                        area->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
                        area->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
                    }
                    QApplication::processEvents();
                    EXPECT_EQ(differing(rendered(twins.m_flat), rendered(twins.m_legacy)), 0) << test.m_name;
                }
            }
        }

        // ----------------------------------------------------------------------------------
        // T5: the combo box: the box of the rule around its edit field and drop-down
        // ----------------------------------------------------------------------------------

        //! A combo box that tells the option it paints with (QComboBox::initStyleOption is protected), in a state and a rect
        class OptionComboBox
            : public QComboBox
        {
        public:
            using QComboBox::QComboBox;

            QStyleOptionComboBox option(QStyle::State state, const QRect& rect) const
            {
                QStyleOptionComboBox option;
                initStyleOption(&option);
                option.rect = rect;
                option.state = state;
                followsEnabled(option);
                return option;
            }
        };

        struct ComboCase
        {
            const char* m_name;
            QString m_rules;
            const BoxElement* m_element;
            bool m_editable;
            int m_index;
            bool m_pixels = true;           //!< a frame of the style's own is the base style's to draw: no pixels to compare
        };

        TEST_F(BoxElementDoorTests, ComboBox_TheBoxOfTheRuleHasTheSheetsHintsRectsAndPixels)
        {
            for (const Qt::LayoutDirection direction : Directions)
            {
                SCOPED_TRACE(direction == Qt::RightToLeft ? "right to left" : "left to right");
                m_direction = direction;

                const QStyle::State e = QStyle::State_Enabled;
                const QStyle::State h = QStyle::State_MouseOver;
                const QStyle::State f = QStyle::State_HasFocus;
                const States states = { e, e | h, e | f, e | h | f, e | QStyle::State_On, QStyle::State(QStyle::State_None), f };
                const QString arrow = QString::fromUtf8(image().m_arrow);
                const ComboCase cases[] = {
                    { "doorCombo", comboRules("doorCombo"), &ComboField, false, 0 },
                    { "doorCombo", comboRules("doorCombo"), &ComboField, false, 1 },
                    { "doorCombo", comboRules("doorCombo"), &ComboField, false, 2 },
                    { "doorEditCombo", comboRules("doorEditCombo"), &ComboField, true, 0 },
                    { "doorWideCombo", QString::fromUtf8(WideComboRules) + QString(comboRules("doorWideCombo")).mid(QString(comboRules("doorWideCombo")).indexOf(QLatin1String("#doorWideCombo::drop-down"))), &WideCombo, false, 0 },
                    { "doorRingCombo", QString::fromUtf8(RingComboRules).arg(arrow), &RingCombo, false, 0 },
                    { "doorNativeCombo", QString::fromUtf8(NativeComboRules), &NativeCombo, false, 1, false },
                    { "doorNativeCombo", QString::fromUtf8(NativeComboRules), &NativeCombo, true, 0, false },
                };
                for (const ComboCase& test : cases)
                {
                    Twins twins(*this, [&](QWidget& root)
                    {
                        auto* combo = add<OptionComboBox>(root, test.m_name, QRect(5, 5, 160, 30));
                        combo->addItem(QStringLiteral("First"));
                        combo->addItem(swatchIcon(0x3366CC), QStringLiteral("Second"));
                    combo->addItem(QStringLiteral("A rather long item text that does not fit in the edit field at all, whatever its width"));
                        combo->setEditable(test.m_editable);
                        combo->setCurrentIndex(test.m_index);
                    }, test.m_rules, QSize(170, 40));
                    const auto* legacyCombo = static_cast<const OptionComboBox*>(twins.legacyChild<QComboBox>(test.m_name));
                    const auto* flatCombo = static_cast<const OptionComboBox*>(twins.flatChild<QComboBox>(test.m_name));
                    const QString what = QStringLiteral("%1 index %2").arg(QLatin1String(test.m_name)).arg(test.m_index);
                    ASSERT_TRUE(BoxElement::forWidget(flatCombo) == test.m_element) << what.toStdString();
                    // The polish gave the box the palette of the text entries (black text), the sheet's twin has the application's: the
                    // text colour is not the box
                    twins.flatChild<QComboBox>(test.m_name)->setPalette(QPalette());

                    EXPECT_EQ(flatCombo->sizeHint(), legacyCombo->sizeHint()) << what.toStdString();
                    EXPECT_EQ(flatCombo->minimumSizeHint(), legacyCombo->minimumSizeHint()) << what.toStdString();
                    EXPECT_EQ(flatCombo->minimumSize(), legacyCombo->minimumSize()) << what.toStdString();
                    EXPECT_EQ(flatCombo->maximumSize(), legacyCombo->maximumSize()) << what.toStdString();
                    if (test.m_editable)
                    {
                        // the line edit sits in the edit field of the box
                        EXPECT_EQ(flatCombo->lineEdit()->geometry(), legacyCombo->lineEdit()->geometry()) << what.toStdString();
                    }

                    for (const QRect& rect : { QRect(0, 0, 160, 30), QRect(3, 4, 121, 27) })
                    {
                        const QStyleOptionComboBox legacyOption = legacyCombo->option(e, rect);
                        const QStyleOptionComboBox flatOption = flatCombo->option(e, rect);
                        for (const QStyle::SubControl control : { QStyle::SC_ComboBoxFrame, QStyle::SC_ComboBoxEditField, QStyle::SC_ComboBoxArrow })
                        {
                            EXPECT_EQ(flatCombo->style()->subControlRect(QStyle::CC_ComboBox, &flatOption, control, flatCombo),
                                legacyCombo->style()->subControlRect(QStyle::CC_ComboBox, &legacyOption, control, legacyCombo))
                                << what.toStdString() << " " << int(control) << " in " << rectText(rect);
                        }
                    }

                    if (!test.m_pixels)
                    {
                        continue;
                    }

                    // the arrow lies where the sheet drew its image: the one place of the legacy image with the colour of the arrow
                    const QStyleOptionComboBox whole = legacyCombo->option(e, QRect(0, 0, 160, 30));
                    const QImage legacyIdle = drawn(QSize(160, 30), 1.0, [&](QPainter* painter)
                    {
                        legacyCombo->style()->drawComplexControl(QStyle::CC_ComboBox, &whole, painter, legacyCombo);
                    });
                    QRect legacyArrow;
                    for (int y = 0; y < legacyIdle.height(); ++y)
                    {
                        for (int x = 0; x < legacyIdle.width(); ++x)
                        {
                            if (legacyIdle.pixelColor(x, y) == rgb(0xFF00FF))
                            {
                                legacyArrow = legacyArrow.united(QRect(x, y, 1, 1));
                            }
                        }
                    }
                    const QStyleOptionComboBox flatWhole = flatCombo->option(e, QRect(0, 0, 160, 30));
                    // (the legacy image shows what is inside the widget: the arrow of a mirrored box sticks out at its left edge by a pixel)
                    EXPECT_EQ(BoxElement::forWidget(flatCombo)->arrowRect(&flatWhole, flatCombo).intersected(QRect(0, 0, 160, 30)), legacyArrow)
                        << what.toStdString();

                    // The component draws its own glyph where the sheet drew the image, which the pixels below leave out: some of it lies in that
                    // rect (what is not the colour of the field beside it), around its centre
                    {
                        const QImage flatIdle = drawn(QSize(160, 30), 1.0, [&](QPainter* painter)
                        {
                            flatCombo->style()->drawComplexControl(QStyle::CC_ComboBox, &flatWhole, painter, flatCombo);
                        });
                        const QColor field = flatIdle.pixelColor(direction == Qt::LeftToRight ? legacyArrow.left() - 3 : legacyArrow.right() + 3, legacyArrow.center().y());
                        QRect ink;
                        for (int y = legacyArrow.top(); y <= legacyArrow.bottom(); ++y)
                        {
                            for (int x = legacyArrow.left(); x <= legacyArrow.right(); ++x)
                            {
                                if (flatIdle.pixelColor(x, y) != field)
                                {
                                    ink = ink.united(QRect(x, y, 1, 1));
                                }
                            }
                        }
                        ASSERT_FALSE(ink.isNull()) << what.toStdString();
                        EXPECT_LE(qAbs(ink.center().x() - legacyArrow.center().x()), 2) << what.toStdString();
                        EXPECT_LE(qAbs(ink.center().y() - legacyArrow.center().y()), 2) << what.toStdString();
                    }

                    // the pixels around the arrow, whose image the component draws: the frame, the fill and the label
                    for (const qreal ratio : Ratios)
                    {
                        for (const QStyle::State state : states)
                        {
                            const auto paint = [&](const OptionComboBox* combo)
                            {
                                return drawn(QSize(160, 30), ratio, [&](QPainter* painter)
                                {
                                    const QStyleOptionComboBox option = combo->option(state, QRect(0, 0, 160, 30));
                                    combo->style()->drawComplexControl(QStyle::CC_ComboBox, &option, painter, combo);
                                    combo->style()->drawControl(QStyle::CE_ComboBoxLabel, &option, painter, combo);
                                });
                            };
                            const QImage flatImage = paint(flatCombo);
                            EXPECT_EQ(differing(flatImage, paint(legacyCombo), legacyArrow), 0) << what.toStdString() << " in state " << int(state) << " at ratio " << ratio;
                            EXPECT_GT(touched(flatImage), 0) << what.toStdString() << " in state " << int(state);
                        }
                    }
                }
            }
        }

        //! The line edit of an editable combo box paints no panel of its own where the box draws its border, and the panel of the box
        //! where the box's border is the style's (qstylesheetstyle.cpp:4651-4660), and sits in the edit field the box leaves it
        TEST_F(BoxElementDoorTests, ComboBox_TheLineEditOfAnEditableBoxHasThePanelAndThePlaceOfTheSheets)
        {
            for (const Qt::LayoutDirection direction : Directions)
            {
                SCOPED_TRACE(direction == Qt::RightToLeft ? "right to left" : "left to right");
                m_direction = direction;

                const QStyle::State e = QStyle::State_Enabled;
                const States states = { e, e | QStyle::State_MouseOver, e | QStyle::State_HasFocus, QStyle::State(QStyle::State_None) };
                struct EditableCase
                {
                    const char* m_name;
                    QString m_rules;
                    const BoxElement* m_element;
                    bool m_geometry = true;     //!< a rule the base style cannot draw is laid out by QWindowsStyle (qstylesheetstyle.cpp:5828-5829): an open item
                };
                const EditableCase cases[] = {
                    { "doorEditCombo", comboRules("doorEditCombo"), &ComboField },
                    { "doorFillCombo", QStringLiteral("#doorFillCombo { background-color: #445566; }"), &FillCombo },
                    { "doorGradientCombo", QStringLiteral("#doorGradientCombo { background: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1, stop: 0 #DDDDDD, stop: 1 #888888); }"), &GradientCombo },
                    { "doorImageCombo", QStringLiteral("#doorImageCombo { background: #445566 url(%1) no-repeat top left; }").arg(QString::fromUtf8(image().m_path)), &ImageCombo, false },
                };
                for (const EditableCase& test : cases)
                {
                    Twins twins(*this, [&](QWidget& root)
                    {
                        auto* combo = add<OptionComboBox>(root, test.m_name, QRect(5, 5, 160, 30));
                        combo->setEditable(true);
                        combo->addItem(QStringLiteral("First"));
                        add<QSpinBox>(root, "doorSpin", QRect(5, 40, 80, 24));
                    }, test.m_rules + QStringLiteral(" QLineEdit { background-color: #FF0000; margin: 7px; border: 3px solid #00FF00; }"), QSize(170, 70));
                    const QComboBox* legacyCombo = twins.legacyChild<QComboBox>(test.m_name);
                    const QComboBox* flatCombo = twins.flatChild<QComboBox>(test.m_name);
                    ASSERT_TRUE(BoxElement::forWidget(flatCombo) == test.m_element) << test.m_name;
                    QLineEdit* legacyEdit = legacyCombo->lineEdit();
                    QLineEdit* flatEdit = flatCombo->lineEdit();
                    ASSERT_TRUE(legacyEdit && flatEdit);
                    expectNoElement(flatEdit);
                    expectNoElement(twins.flatChild<QSpinBox>("doorSpin")->findChild<QLineEdit*>());
                    flatEdit->setPalette(QPalette());       // the polish gave the field the palette of the text entries, the sheet's twin has the application's

                    if (test.m_geometry)
                    {
                        EXPECT_EQ(flatEdit->geometry(), legacyEdit->geometry()) << test.m_name;
                    }
                    for (const qreal ratio : Ratios)
                    {
                        for (const QStyle::State state : states)
                        {
                            const auto panel = [&](const QLineEdit* lineEdit)
                            {
                                return drawn(QSize(100, 20), ratio, [&](QPainter* painter)
                                {
                                    const QStyleOptionFrame option = fieldOption(lineEdit, state, QRect(0, 0, 100, 20));
                                    lineEdit->style()->drawPrimitive(QStyle::PE_PanelLineEdit, &option, painter, lineEdit);
                                });
                            };
                            EXPECT_EQ(differing(panel(flatEdit), panel(legacyEdit)), 0) << test.m_name << " in state " << int(state) << " at ratio " << ratio;
                        }
                    }
                }
            }
        }

        // ----------------------------------------------------------------------------------
        // T6: a rule with no box of its own leaves the geometry to the base style and the components, as the sheet did
        // ----------------------------------------------------------------------------------

        TEST_F(BoxElementDoorTests, NativeBorders_AnElementWithNoBoxLeavesTheGeometryToTheBaseStyleAndTheComponents)
        {
            const QStyle::State e = QStyle::State_Enabled | QStyle::State_Raised;

            // A line edit and a combo box: the answers of the base style, which the sheet delegated them to (qstylesheetstyle.cpp:5336-5350,
            // 5389-5397, 5799-5830, 6176-6182). The base style counts the metrics of the Style under it (CT_ComboBox counts the extent of
            // a scroll bar, qcommonstyle.cpp:4943-4945), which flattening changed, so they are compared with the base style under the Style.
            {
                QWidget root;
                add<QLineEdit>(root, "doorFillEdit", QRect(5, 5, 100, 30));
                auto* combo = add<OptionComboBox>(root, "doorFillCombo", QRect(5, 40, 160, 30));
                combo->addItem(QStringLiteral("First"));
                flat(root);
                show(root, QSize(180, 80));
                const auto* edit = root.findChild<QLineEdit*>(QStringLiteral("doorFillEdit"));
                ASSERT_TRUE(BoxElement::forWidget(edit) == &FillEdit);
                ASSERT_TRUE(BoxElement::forWidget(combo) == &FillCombo);
                EXPECT_FALSE(FillEdit.ownsBox(nullptr));
                EXPECT_FALSE(FillCombo.ownsBox(nullptr));

                // the base style takes the rect inside the line width of the option (qcommonstyle.cpp:2952-2956), a box inside the native
                // frame width of the widget: a line width of 3 tells them apart
                QStyleOptionFrame editOption = fieldOption(edit, e, QRect(0, 0, 100, 30));
                editOption.lineWidth = 3;
                EXPECT_EQ(m_style->sizeFromContents(QStyle::CT_LineEdit, &editOption, QSize(100, 14), edit),
                    m_style->baseStyle()->sizeFromContents(QStyle::CT_LineEdit, &editOption, QSize(100, 14), edit));
                EXPECT_EQ(m_style->subElementRect(QStyle::SE_LineEditContents, &editOption, edit),
                    m_style->baseStyle()->subElementRect(QStyle::SE_LineEditContents, &editOption, edit));

                const QStyleOptionComboBox comboOption = combo->option(e, QRect(0, 0, 160, 30));
                EXPECT_EQ(m_style->sizeFromContents(QStyle::CT_ComboBox, &comboOption, QSize(50, 14), combo),
                    m_style->baseStyle()->sizeFromContents(QStyle::CT_ComboBox, &comboOption, QSize(50, 14), combo));
                for (const QStyle::SubControl control : { QStyle::SC_ComboBoxFrame, QStyle::SC_ComboBoxEditField, QStyle::SC_ComboBoxArrow })
                {
                    EXPECT_EQ(m_style->subControlRect(QStyle::CC_ComboBox, &comboOption, control, combo),
                        m_style->baseStyle()->subControlRect(QStyle::CC_ComboBox, &comboOption, control, combo)) << int(control);
                }
            }

            // A tool button: what the components give a tool button with no element (ToolButton::sizeFromContents, subControlRect,
            // PM_MenuButtonIndicator), not the box of the element, which has none
            {
                QWidget root;
                for (const char* name : { "doorPlainTool", "doorFillTool" })
                {
                    auto* button = add<OptionToolButton>(root, name, QRect(5, 5, 80, 40));
                    button->setIcon(swatchIcon(0xFF8000));
                    button->setIconSize(QSize(16, 16));
                    button->setText(QStringLiteral("Tool"));
                }
                flat(root);
                show(root, QSize(100, 60));
                const auto* plain = static_cast<const OptionToolButton*>(root.findChild<QToolButton*>(QStringLiteral("doorPlainTool")));
                const auto* fill = static_cast<const OptionToolButton*>(root.findChild<QToolButton*>(QStringLiteral("doorFillTool")));
                ASSERT_TRUE(BoxElement::forWidget(plain) == nullptr);
                ASSERT_TRUE(BoxElement::forWidget(fill) == &FillTool);
                EXPECT_FALSE(FillTool.ownsBox(nullptr));

                EXPECT_EQ(fill->sizeHint(), plain->sizeHint());
                EXPECT_EQ(fill->minimumSizeHint(), plain->minimumSizeHint());
                const QStyleOptionToolButton plainOption = plain->option(e, QRect(0, 0, 80, 40));
                const QStyleOptionToolButton fillOption = fill->option(e, QRect(0, 0, 80, 40));
                EXPECT_GT(m_style->pixelMetric(QStyle::PM_MenuButtonIndicator, &fillOption, fill), 0);
                EXPECT_EQ(m_style->pixelMetric(QStyle::PM_MenuButtonIndicator, &fillOption, fill),
                    m_style->pixelMetric(QStyle::PM_MenuButtonIndicator, &plainOption, plain));
                for (const QStyle::SubControl control : { QStyle::SC_ToolButton, QStyle::SC_ToolButtonMenu })
                {
                    EXPECT_EQ(m_style->subControlRect(QStyle::CC_ToolButton, &fillOption, control, fill),
                        m_style->subControlRect(QStyle::CC_ToolButton, &plainOption, control, plain)) << int(control);
                }
            }
        }

        // ----------------------------------------------------------------------------------
        // T7: a widget with no element: every door is shut. The registry holds elements for the same classes (the named widgets above),
        // and a widget that none of them matches keeps what the style gave it before the doors: the answers of the base style, of the
        // components and of the indicator element.
        // ----------------------------------------------------------------------------------

        TEST_F(BoxElementDoorTests, LineEdit_WithNoElementTheFlatFieldIsWhatItWas)
        {
            QWidget root;
            add<QLineEdit>(root, "doorField", QRect(5, 5, 120, 20));                    // the registry covers this one
            QLineEdit* plain = add<QLineEdit>(root, "doorPlainEdit", QRect(5, 30, 120, 20));
            flat(root);
            show(root, QSize(140, 60));
            expectNoElement(plain);
            ASSERT_TRUE(BoxElement::forWidget(root.findChild<QLineEdit*>(QStringLiteral("doorField"))) == &Field);

            // LineEditFieldTests T-LE.1: two wider than the base box, twenty high
            QStyleOptionFrame option;
            option.initFrom(plain);
            const QSize base = m_style->baseStyle()->sizeFromContents(QStyle::CT_LineEdit, &option, QSize(100, 14), plain);
            EXPECT_EQ(m_style->sizeFromContents(QStyle::CT_LineEdit, &option, QSize(100, 14), plain), QSize(base.width() + 2, 20));

            // T-LE.2: inside the 2px border and the padding of -1 above and 1 below
            option.rect = QRect(0, 0, 100, 20);
            EXPECT_EQ(m_style->subElementRect(QStyle::SE_LineEditContents, &option, plain), QRect(2, 1, 96, 16));

            // T-LE.3: the ring of 2px #E9E9E9 around the field #CCCCCC, #666666 when disabled
            for (const QStyle::State state : States{ QStyle::State_Enabled, QStyle::State(QStyle::State_None) })
            {
                const QImage image = drawn(QSize(60, 20), 1.0, [&](QPainter* painter)
                {
                    const QStyleOptionFrame frame = fieldOption(plain, state, QRect(0, 0, 60, 20));
                    m_style->drawPrimitive(QStyle::PE_PanelLineEdit, &frame, painter, plain);
                });
                const QColor field = rgb(state.testFlag(QStyle::State_Enabled) ? 0xCCCCCC : 0x666666);
                for (const QPoint ring : { QPoint(0, 10), QPoint(1, 10), QPoint(59, 10), QPoint(58, 10), QPoint(30, 0), QPoint(30, 1), QPoint(30, 19), QPoint(30, 18) })
                {
                    EXPECT_EQ(image.pixelColor(ring), rgb(0xE9E9E9)) << ring.x() << ", " << ring.y();
                }
                for (const QPoint inside : { QPoint(2, 10), QPoint(30, 2), QPoint(2, 2), QPoint(57, 17), QPoint(30, 10) })
                {
                    EXPECT_EQ(image.pixelColor(inside), field) << inside.x() << ", " << inside.y();
                }
            }
        }

        TEST_F(BoxElementDoorTests, ToolButton_WithNoElementTheComponentsAnswerStands)
        {
            QMenu menu;
            menu.addAction(QStringLiteral("One"));
            QWidget root;
            add<OptionToolButton>(root, "doorFacedTool", QRect(5, 5, 80, 40));         // the registry covers this one
            auto* plain = add<OptionToolButton>(root, "doorPlainTool", QRect(5, 50, 80, 40));
            auto* withMenu = add<OptionToolButton>(root, "doorPlainMenuTool", QRect(5, 95, 80, 40));
            withMenu->setMenu(&menu);
            withMenu->setPopupMode(QToolButton::InstantPopup);
            flat(root);
            show(root, QSize(100, 140));
            expectNoElement(plain);
            expectNoElement(withMenu);
            ASSERT_TRUE(BoxElement::forWidget(root.findChild<QToolButton*>(QStringLiteral("doorFacedTool"))) == &FacedTool);

            // ToolButton::sizeFromContents and subControlRect, by the metrics the components give a tool button: the margin on every
            // side, the menu indicator in place of the right margin of a button with a menu
            const QStyle::State e = QStyle::State_Enabled | QStyle::State_Raised;
            const QSize contents(40, 20);
            const QRect rect(0, 0, 80, 40);
            for (const OptionToolButton* button : { plain, withMenu })
            {
                const QStyleOptionToolButton option = button->option(e, rect);
                const int margin = m_style->pixelMetric(QStyle::PM_ButtonMargin, &option, button);
                const int indicator = m_style->pixelMetric(QStyle::PM_MenuButtonIndicator, &option, button);
                const bool hasMenu = (button == withMenu);
                const std::string name = button->objectName().toStdString();
                EXPECT_GT(indicator, 0) << name;            // the box of an element has none (qstylesheetstyle.cpp:5006-5011)
                EXPECT_EQ(m_style->sizeFromContents(QStyle::CT_ToolButton, &option, contents, button),
                    QSize(contents.width() + margin + (hasMenu ? indicator : margin), contents.height() + 2 * margin)) << name;
                const QRect inside = rect.adjusted(0, margin, 0, -margin);
                EXPECT_EQ(m_style->subControlRect(QStyle::CC_ToolButton, &option, QStyle::SC_ToolButton, button),
                    hasMenu ? inside.adjusted(margin, 0, -(indicator + margin), 0) : inside.adjusted(margin, 0, -margin, 0)) << name;
                EXPECT_EQ(m_style->subControlRect(QStyle::CC_ToolButton, &option, QStyle::SC_ToolButtonMenu, button),
                    QRect(rect.width() - indicator, inside.top(), indicator, inside.height())) << name;
            }
        }

        TEST_F(BoxElementDoorTests, Buttons_WithNoElementTheIndicatorElementHasTheWholeButton)
        {
            QWidget root;
            add<OptionCheckBox>(root, "doorFacedCheck", QRect(5, 5, 200, 44));         // the registry covers these two
            add<OptionRadioButton>(root, "doorLhsRadio", QRect(5, 55, 200, 44));
            auto* check = add<OptionCheckBox>(root, "doorPlainCheck", QRect(5, 105, 200, 44));
            auto* radio = add<OptionRadioButton>(root, "doorPlainRadio", QRect(5, 155, 200, 44));
            check->setText(QStringLiteral("Label text"));
            radio->setText(QStringLiteral("Label text"));
            flat(root);
            show(root, QSize(210, 205));
            expectNoElement(check);
            expectNoElement(radio);
            ASSERT_TRUE(IndicatorElement::forWidget(check) != nullptr);
            ASSERT_TRUE(IndicatorElement::forWidget(radio) != nullptr);

            for (const QAbstractButton* button : { static_cast<const QAbstractButton*>(check), static_cast<const QAbstractButton*>(radio) })
            {
                const bool isRadio = (button == radio);
                const IndicatorElement* indicator = IndicatorElement::forWidget(button);
                const QRect rect(0, 0, 200, 44);
                const QStyleOptionButton option = isRadio ? radio->option(QStyle::State_Enabled, rect) : check->option(QStyle::State_Enabled, rect);
                const int spacing = m_style->pixelMetric(isRadio ? QStyle::PM_RadioButtonLabelSpacing : QStyle::PM_CheckBoxLabelSpacing, &option, button);
                const std::string name = button->objectName().toStdString();

                // CheckBox::sizeFromContents and RadioButton::sizeFromContents: the indicator element's box and the label spacing
                EXPECT_EQ(m_style->sizeFromContents(isRadio ? QStyle::CT_RadioButton : QStyle::CT_CheckBox, &option, QSize(50, 14), button),
                    indicator->sizeFromContents(QSize(50, 14), spacing)) << name;

                // the rects of the indicator and of the label are the base style's, the click rects the indicator element's
                for (const QStyle::SubElement element : { isRadio ? QStyle::SE_RadioButtonIndicator : QStyle::SE_CheckBoxIndicator,
                         isRadio ? QStyle::SE_RadioButtonContents : QStyle::SE_CheckBoxContents })
                {
                    EXPECT_EQ(m_style->subElementRect(element, &option, button), m_style->baseStyle()->subElementRect(element, &option, button)) << name << " " << int(element);
                }
                EXPECT_EQ(m_style->subElementRect(isRadio ? QStyle::SE_RadioButtonClickRect : QStyle::SE_CheckBoxClickRect, &option, button),
                    isRadio ? rect : indicator->clickRect(m_style.get(), option, spacing)) << name;

                // the indicator element draws the image and the label, and no box: the corners of the rect stay as they were
                const QImage image = drawn(rect.size(), 1.0, [&](QPainter* painter)
                {
                    m_style->drawControl(isRadio ? QStyle::CE_RadioButton : QStyle::CE_CheckBox, &option, painter, button);
                });
                EXPECT_GT(touched(image), 0) << name;
                for (const QPoint corner : { QPoint(0, 0), QPoint(199, 0), QPoint(0, 43), QPoint(199, 43) })
                {
                    EXPECT_EQ(image.pixelColor(corner), rgb(0x808080)) << name << " " << corner.x() << ", " << corner.y();
                }
            }
        }

        //! The watcher of a combo box polishes it again through the style sheet style of the StyleManager when a property changes (ComboBox.qss
        //! selects on [HasError=true]); these tests have no StyleManager, and the element a door keeps on a combo box is a property as well
        TEST_F(BoxElementDoorTests, ComboBox_AChangedPropertyWithNoStyleManagerIsNotAFault)
        {
            QWidget root;
            auto* combo = add<OptionComboBox>(root, "doorCombo", QRect(5, 5, 160, 30));
            flat(root);
            show(root, QSize(170, 40));
            ASSERT_TRUE(BoxElement::forWidget(combo) == &ComboField);

            combo->setProperty("HasError", true);
            combo->setProperty("HasError", false);
            EXPECT_TRUE(BoxElement::forWidget(combo) == &ComboField);
        }

        TEST_F(BoxElementDoorTests, ComboBox_WithNoElementTheFlatComboBoxIsTheSheetsOwnLook)
        {
            QWidget root;
            add<OptionComboBox>(root, "doorCombo", QRect(5, 5, 160, 30));                // the registry covers this one
            auto* plain = add<OptionComboBox>(root, "doorPlainCombo", QRect(5, 40, 160, 30));
            plain->addItem(QStringLiteral("First"));
            flat(root);
            show(root, QSize(170, 80));

            // No module element covers it: the fallback of ComboBox.cpp, or the Global element where an application style registered it
            // (ComboBox::registerFlatElements, from Style::polish(QApplication*): whichever test set a style first), which looks the same
            const BoxElement* resolved = BoxElement::forWidget(plain);
            EXPECT_TRUE(resolved == nullptr || qstrcmp(resolved->m_name, "Global.Box.ComboBox") == 0) << (resolved ? resolved->m_name : "");
            ASSERT_TRUE(BoxElement::forWidget(root.findChild<QComboBox*>(QStringLiteral("doorCombo"))) == &ComboField);

            // ComboBox.qss gives the look (ComboBox::registerFlatElements; FieldComboBoxTests holds it against the sheet in every state):
            // the contents 16 high and as wide as asked, the drop-down of 16 and the 2px the base style keeps in the label, the margin of 1
            // and the padding-left of 4: 18 high (16 and the margin) and 24 wider than the contents (n2 Gallery 00_assetbrowserfolder: 68 x 18)
            const QStyleOptionComboBox option = plain->option(QStyle::State_Enabled, QRect(0, 0, 80, 18));
            EXPECT_EQ(m_style->sizeFromContents(QStyle::CT_ComboBox, &option, QSize(50, 14), plain), QSize(50 + 24, 18));

            // the field is inset by the 1px margin and rounded by 2px (BaseStyleSheet.qss:79), with no border of its own: the rect's edge
            // stays as it was, the field is #CCCCCC, and the corner pixel of the field is a blend of it and the edge
            const QImage image = drawn(QSize(80, 18), 1.0, [&](QPainter* painter)
            {
                m_style->drawComplexControl(QStyle::CC_ComboBox, &option, painter, plain);
            });
            for (const QPoint edge : { QPoint(0, 0), QPoint(0, 9), QPoint(40, 0), QPoint(40, 17), QPoint(79, 0) })
            {
                EXPECT_EQ(image.pixelColor(edge), rgb(0x808080)) << edge.x() << ", " << edge.y();
            }
            for (const QPoint field : { QPoint(3, 3), QPoint(3, 14), QPoint(30, 9), QPoint(76, 3) })
            {
                EXPECT_EQ(image.pixelColor(field), rgb(0xCCCCCC)) << field.x() << ", " << field.y();
            }
            for (const QPoint corner : { QPoint(1, 1), QPoint(78, 1), QPoint(1, 16), QPoint(78, 16) })
            {
                EXPECT_NE(image.pixelColor(corner), rgb(0xCCCCCC)) << corner.x() << ", " << corner.y();
                EXPECT_NE(image.pixelColor(corner), rgb(0x808080)) << corner.x() << ", " << corner.y();
            }
        }

        TEST_F(BoxElementDoorTests, ScrollArea_WithNoElementTheViewportAndTheBackgroundAreLeftAlone)
        {
            QWidget root;
            add<QAbstractScrollArea>(root, "doorFilledArea", QRect(5, 5, 160, 90));      // the registry covers this one
            auto* plain = add<QAbstractScrollArea>(root, "doorPlainScrollArea", QRect(5, 100, 160, 90));
            flat(root);
            show(root, QSize(170, 200));
            expectNoElement(plain);
            expectNoElement(plain->viewport());
            ASSERT_TRUE(BoxElement::forWidget(root.findChild<QAbstractScrollArea*>(QStringLiteral("doorFilledArea"))) == &FilledArea);

            // the polish took nothing from the viewport: it fills itself and is not styled
            EXPECT_TRUE(plain->viewport()->autoFillBackground());
            EXPECT_FALSE(plain->viewport()->testAttribute(Qt::WA_StyledBackground));
            EXPECT_FALSE(plain->testAttribute(Qt::WA_StyledBackground));
            EXPECT_FALSE(plain->property("_o3de_boxElementAutoFill").isValid());

            // the background of the viewport is not the style's to paint (PE_Widget), whatever the state
            for (const QStyle::State state : States{ QStyle::State_Enabled, QStyle::State_Enabled | QStyle::State_MouseOver, QStyle::State(QStyle::State_None) })
            {
                const QImage image = drawn(plain->viewport()->size(), 1.0, [&](QPainter* painter)
                {
                    QStyleOption option;
                    option.initFrom(plain->viewport());
                    option.state = state;
                    m_style->drawPrimitive(QStyle::PE_Widget, &option, painter, plain->viewport());
                });
                EXPECT_EQ(touched(image), 0) << int(state);
            }
        }
    } // namespace BoxElementDoorTestsInternal
} // namespace AzQtComponents
