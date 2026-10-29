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

#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QFile>
#include <QFrame>
#include <QIcon>
#include <QImage>
#include <QPainter>
#include <QLineEdit>
#include <QPixmap>
#include <QPushButton>
#include <QRadioButton>
#include <QRegularExpression>
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
    // What a face of the box element carries beyond the fill, the border and the image: the margin and the border radius of its
    // state (Face::m_margin, m_radius), the label spacing of a check box, a radio button and a combo box label (m_spacing), and the
    // border image of a tool button (m_borderImage). The legacy twin of a widget is Qt's own style sheet style over Fusion with the
    // rules of the legacy sheets, the real ones where the test can read them (ComboBox.qss, RadioButton.qss: the application sheet
    // as StyleSheetCache builds it) and the text of the Project Manager and Asset Processor rules beside them. The flat twin is the
    // flattened Style with the element that carries the same box. The same hints, rects and pixels, left to right and right to left,
    // at the device ratios 1 and 1.5, each painted through the style on an image of the ratio (QWidget::render would hand the style
    // the screen's ratio).
    // ----------------------------------------------------------------------------------

    namespace BoxElementFaceTestData
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
        bool flatTwin(const QWidget* widget)
        {
            return widget->window()->property("boxElementFaceTestsFlat").toBool();
        }

        bool named(const QWidget* widget, const char* objectName)
        {
            return flatTwin(widget) && widget->objectName() == QLatin1String(objectName);
        }

        bool childOf(const QWidget* widget, const char* parentName)
        {
            return flatTwin(widget) && widget->parentWidget() && widget->parentWidget()->objectName() == QLatin1String(parentName);
        }

        //! Images for the sheets and the elements: files of a temporary directory
        struct Images
        {
            QTemporaryDir m_directory;
            QByteArray m_arrow;                 //!< the arrow of a drop-down: 16 x 16, the colour of nothing else
            QByteArray m_border;                //!< a border image: 24 x 16, a gradient with a white mark in each corner, partly clear
            QByteArray m_borderHover;           //!< another one for :hover
        };

        Images& images()
        {
            static Images shared;
            if (shared.m_arrow.isEmpty())
            {
                QImage arrow(16, 16, QImage::Format_ARGB32);
                arrow.fill(rgb(0xFF00FF));
                const QString arrowPath = shared.m_directory.filePath(QStringLiteral("arrow.png"));
                arrow.save(arrowPath, "PNG");
                shared.m_arrow = arrowPath.toUtf8();

                for (const bool hover : { false, true })
                {
                    QImage border(24, 16, QImage::Format_ARGB32);
                    for (int y = 0; y < border.height(); ++y)
                    {
                        for (int x = 0; x < border.width(); ++x)
                        {
                            // red to blue from left to right, clearer toward the bottom (or the reverse for the hover image)
                            const int alpha = hover ? 255 - 8 * (border.height() - 1 - y) : 255 - 8 * y;
                            border.setPixelColor(x, y, QColor(hover ? 255 - x * 10 : x * 10, 40 + y * 8, hover ? x * 10 : 255 - x * 10, alpha));
                        }
                    }
                    QPainter painter(&border);
                    painter.fillRect(0, 0, 3, 3, Qt::white);
                    painter.fillRect(border.width() - 3, 0, 3, 3, Qt::white);
                    painter.fillRect(0, border.height() - 3, 3, 3, Qt::white);
                    painter.fillRect(border.width() - 3, border.height() - 3, 3, 3, Qt::white);
                    painter.end();
                    const QString path = shared.m_directory.filePath(hover ? QStringLiteral("borderHover.png") : QStringLiteral("border.png"));
                    border.save(path, "PNG");
                    (hover ? shared.m_borderHover : shared.m_border) = path.toUtf8();
                }
            }
            return shared;
        }

        //! The application sheet as StyleSheetCache builds it from BaseStyleSheet.qss: each @import line replaced by the file it names,
        //! from the real resources. The arrows of the combo box are the marker image, which the test finds in the pixels of the legacy
        //! twin (the flat combo box draws the component's glyph there; the SVG would draw where the marker does).
        const QString& appSheet()
        {
            static QString expanded;
            if (expanded.isEmpty())
            {
                const auto read = [](const QString& name)
                {
                    QFile file(QStringLiteral(":/AzQtComponents/Widgets/") + name);
                    EXPECT_TRUE(file.open(QIODevice::ReadOnly)) << name.toUtf8().constData();
                    return QString::fromUtf8(file.readAll());
                };
                expanded = read(QStringLiteral("BaseStyleSheet.qss"));
                const QRegularExpression import(QStringLiteral("@import\\s+\"([^\"]+)\"\\s*;"));
                for (QRegularExpressionMatch match = import.match(expanded); match.hasMatch(); match = import.match(expanded))
                {
                    expanded.replace(match.capturedStart(), match.capturedLength(), read(match.captured(1)));
                }
                const QString marker = QString::fromUtf8(images().m_arrow);
                expanded.replace(QStringLiteral(":/stylesheet/img/UI20/combobox-arrow-disabled.svg"), marker);
                expanded.replace(QStringLiteral(":/stylesheet/img/UI20/combobox-arrow.svg"), marker);
            }
            return expanded;
        }

        // ---- ProjectManager.qss:149-159: the combo box of a form field (the same rule at :313-323), over ComboBox.qss
        const char* const PmFormComboRules =
            "#formFrame QComboBox {"
            "    subcontrol-origin: padding;"
            "    subcontrol-position: top right;"
            "    min-width:810px;"
            "    max-width:810px;"
            "    line-height: 26px;"
            "    font-size: 18px;"
            "    height: 26px;"
            "    padding-left:0px;"
            "    padding-right:0px;"
            "}";

        // ProjectManager.qss:161-168: the drop-down of that combo box, 24px of contents and 20px of padding to their right: 44px, where the
        // arrow sits in the left 24 (the n2 capture of the project page: the combo box hints 858 x 28, which is 810 and 44 and the 2px the base
        // style leaves in the label and the margin of 2)
        const char* const PmFormDropDownRules =
            "#formFrame QComboBox::drop-down {"
            "    subcontrol-origin: padding;"
            "    subcontrol-position: top right;"
            "    padding-right:20px;"
            "    min-width: 24px;"
            "    max-width: 24px;"
            "    width:24px;"
            "}";

        // The element of that combo box: its idle box is ComboBox.qss:13-20 (margin 1px, :17) over BaseStyleSheet.qss:68-84 (radius 2px,
        // :79, no border, :80), with the width, the height and the padding of the rule above. :hover and :focus give it another box:
        // no margin, a border of 1px and a radius of 3px (ComboBox.qss:22-29, 31-42). The idle box is Native's (n2, the project page: the
        // combo box at 68, 485 hints 858 x 28, its limits are 812 wide, the ink of its arrow lies at 841 to 849, 497 to 501 in the
        // 16 x 16 image at 837, 491); the other states are proven against the legacy rules by the twin, UNVERIFIED by a capture (no capture
        // holds this combo box hovered, focused or disabled).
        const BoxElement PmFormCombo{
            .m_name = "Test.Face.PmFormCombo",
            .m_margin = QMargins(1, 1, 1, 1),                                       // ComboBox.qss:17
            .m_padding = QMargins(0, 0, 0, 0),                                      // ProjectManager.qss:157-158
            .m_hasBox = true,
            .m_nativeBorder = false,                                                // BaseStyleSheet.qss:79-80
            .m_radius = 2,                                                          // BaseStyleSheet.qss:79
            .m_size = QSize(-1, 26),                                                // ProjectManager.qss:156
            .m_minSize = QSize(810, -1),                                            // ProjectManager.qss:152
            .m_maxSize = QSize(810, -1),                                            // ProjectManager.qss:153
            .m_dropDownWidth = 24,                                                  // ProjectManager.qss:167 (:165 min, :166 max)
            .m_dropDownPadding = QMargins(0, 0, 20, 0),                             // ProjectManager.qss:164
            .m_idle = { .m_fill = solid(0xCCCCCC), .m_text = rgb(0x000000) },       // ComboBox.qss:19, BaseStyleSheet.qss:77
            .m_stateFaces = {
                { BoxElement::Hover, {
                    .m_fill = solid(0xCCCCCC), .m_edges = BoxElement::border(1, QColor(0, 0, 0, 50)), .m_text = rgb(0x000000),   // ComboBox.qss:25-28
                    .m_margin = QMargins(0, 0, 0, 0), .m_radius = 3 } },                                                          // :24, :27
                { BoxElement::Focus, {
                    .m_fill = solid(0xFFFFFF), .m_edges = BoxElement::border(1, rgb(0x94D2FF)), .m_text = rgb(0x000000),         // ComboBox.qss:41, :36-39
                    .m_margin = QMargins(0, 0, 0, 0), .m_radius = 3 } },                                                          // :35, :38
                // [HasPopupOpen=true] has the declarations of :focus (ComboBox.qss:31-42), after :hover; QComboBox::initStyleOption sets State_On
                // while the popup is shown (qcombobox.cpp:1499-1500), which is when the watcher of ComboBox.cpp sets the property.
                // UNVERIFIED by a capture: no capture holds an open popup (the legacy twin has the property set by hand)
                { BoxElement::Checked, {
                    .m_fill = solid(0xFFFFFF), .m_edges = BoxElement::border(1, rgb(0x94D2FF)), .m_text = rgb(0x000000),
                    .m_margin = QMargins(0, 0, 0, 0), .m_radius = 3 } },
                { BoxElement::Disabled, { .m_fill = solid(0x666666), .m_text = rgb(0x999999) } },                                 // ComboBox.qss:53-57
                // :disabled comes after :focus (ComboBox.qss:53 after :31): the fill and the colour of the first, the box of the second
                { BoxElement::Disabled | BoxElement::Focus, {
                    .m_fill = solid(0x666666), .m_edges = BoxElement::border(1, rgb(0x94D2FF)), .m_text = rgb(0x999999),
                    .m_margin = QMargins(0, 0, 0, 0), .m_radius = 3 } },
            },
        };

        //! The same element as the module had to write it while a face kept the margin and the radius of the element: one box for every state
        const BoxElement PmFormComboOneBox{
            .m_name = "Test.Face.PmFormComboOneBox",
            .m_margin = QMargins(1, 1, 1, 1),
            .m_padding = QMargins(0, 0, 0, 0),
            .m_hasBox = true,
            .m_nativeBorder = false,
            .m_radius = 2,
            .m_size = QSize(-1, 26),
            .m_minSize = QSize(810, -1),
            .m_maxSize = QSize(810, -1),
            .m_dropDownWidth = 24,
            .m_dropDownPadding = QMargins(0, 0, 20, 0),
            .m_idle = { .m_fill = solid(0xCCCCCC), .m_text = rgb(0x000000) },
            .m_stateFaces = {
                { BoxElement::Hover, { .m_fill = solid(0xCCCCCC), .m_edges = BoxElement::border(1, QColor(0, 0, 0, 50)), .m_text = rgb(0x000000) } },
                { BoxElement::Focus, { .m_fill = solid(0xFFFFFF), .m_edges = BoxElement::border(1, rgb(0x94D2FF)), .m_text = rgb(0x000000) } },
                { BoxElement::Disabled, { .m_fill = solid(0x666666), .m_text = rgb(0x999999) } },
                { BoxElement::Disabled | BoxElement::Focus, { .m_fill = solid(0x666666), .m_edges = BoxElement::border(1, rgb(0x94D2FF)), .m_text = rgb(0x999999) } },
            },
        };

        // ---- A combo box whose rule sets a spacing: the gap after the icon of its label (qstylesheetstyle.cpp:4029-4031), 9px, and 11px
        // while the pointer is over it
        const char* const SpaceComboRules =
            "#spaceCombo { background-color: #CCCCCC; color: #000000; margin: 1px; padding-left: 4px; border-radius: 2px; border-width: 0px;"
            " height: 16px; spacing: 9px; }"
            "#spaceCombo:hover { spacing: 11px; }"
            "#spaceCombo::drop-down { border-width: 0; }";
        const BoxElement SpaceCombo{
            .m_name = "Test.Face.SpaceCombo",
            .m_margin = QMargins(1, 1, 1, 1),
            .m_padding = QMargins(4, 0, 0, 0),
            .m_spacing = 9,
            .m_nativeBorder = false,
            .m_radius = 2,
            .m_size = QSize(-1, 16),
            .m_idle = { .m_fill = solid(0xCCCCCC), .m_text = rgb(0x000000) },
            .m_stateFaces = { { BoxElement::Hover, { .m_fill = solid(0xCCCCCC), .m_text = rgb(0x000000), .m_spacing = 11 } } },
        };

        // ---- A combo box whose drop-down has a padding on every side (the form combo's has one on the right only): the arrow is centered in
        // the contents of the drop-down, what the padding leaves (the origin of an arrow is its drop-down's contents, qstylesheetstyle.cpp:2198-2211)
        const char* const PaddedDropDownRules =
            "#paddedCombo { background-color: #CCCCCC; color: #000000; margin: 2px 1px 3px 4px; padding-left: 5px; border-radius: 2px;"
            " border-width: 0px; height: 20px; }"
            "#paddedCombo:hover { margin: 0px; border-width: 2px; border-style: solid; border-color: #94D2FF; border-radius: 3px; }"
            "#paddedCombo::drop-down { border-width: 0; width: 18px; padding: 3px 7px 2px 4px; }"
            "#paddedCombo::down-arrow { width: 16px; height: 16px; margin-right: 4px; image: url(%1); }";
        const BoxElement PaddedDropDown{
            .m_name = "Test.Face.PaddedDropDown",
            .m_margin = QMargins(4, 2, 1, 3),                                       // margin: 2px 1px 3px 4px (left, top, right, bottom)
            .m_padding = QMargins(5, 0, 0, 0),
            .m_nativeBorder = false,
            .m_radius = 2,
            .m_size = QSize(-1, 20),
            .m_dropDownWidth = 18,
            .m_dropDownPadding = QMargins(4, 3, 7, 2),                              // padding: 3px 7px 2px 4px
            .m_idle = { .m_fill = solid(0xCCCCCC), .m_text = rgb(0x000000) },
            .m_stateFaces = {
                { BoxElement::Hover, { .m_fill = solid(0xCCCCCC), .m_edges = BoxElement::border(2, rgb(0x94D2FF)), .m_text = rgb(0x000000),
                    .m_margin = QMargins(0, 0, 0, 0), .m_radius = 3 } },
            },
        };

        // ---- ProjectManager.qss:79-99: the steps of the create-a-gem wizard. A radio button 40px high and 180px wide at least, a colour that
        // follows the state, a lighter fill and a wider label spacing while it is checked. The image of the indicator is nothing.
        const char* const PmLhsRadioRules =
            "#createAGemLHS QRadioButton{"
            "    font-size: 16px;"
            "    height: 40px;"
            "    min-width: 180px;"
            "    spacing: 10px;"
            "    color: #6b6b6b;"
            "}"
            "#createAGemLHS QRadioButton:enabled{"
            "    color: #aaaaaa;"
            "}"
            "#createAGemLHS QRadioButton:checked{"
            "    background-color: #333333;"
            "    spacing: 30px;"
            "    color: #FFFFFF;"
            "}"
            "#createAGemLHS QRadioButton::indicator{"
            "    image: url(:/nothing.png);"
            "}";
        const BoxElement PmLhsRadio{
            .m_name = "Test.Face.PmLhsRadio",
            .m_spacing = 10,                                                        // ProjectManager.qss:83
            .m_size = QSize(-1, 40),                                                // :81
            .m_minSize = QSize(180, -1),                                            // :82
            .m_idle = { .m_fill = QBrush(Qt::transparent), .m_text = rgb(0xAAAAAA) },    // BaseStyleSheet.qss:17, ProjectManager.qss:87-89
            .m_stateFaces = {
                { BoxElement::Disabled, { .m_fill = QBrush(Qt::transparent), .m_text = rgb(0x6B6B6B) } },                   // :84
                { BoxElement::Checked, { .m_fill = solid(0x333333), .m_text = rgb(0xFFFFFF), .m_spacing = 30 } },           // :91-95
                { BoxElement::Disabled | BoxElement::Checked, { .m_fill = solid(0x333333), .m_text = rgb(0xFFFFFF), .m_spacing = 30 } },
            },
        };

        // ---- ProjectManager.qss:890-897: a check box of the gem dependencies dialog, with a spacing of 3px (the base style's is 6)
        const char* const PmDependencyCheckRules =
            "#GemDependenciesDialog QCheckBox {"
            "    background-color: #333333;"
            "    border-radius: 3px;"
            "    spacing:3px;"
            "    margin-right:5px;"
            "    padding:4px;"
            "    margin-top:5px;"
            "}";
        const BoxElement PmDependencyCheck{
            .m_name = "Test.Face.PmDependencyCheck",
            .m_margin = QMargins(0, 5, 5, 0),                                       // :896 (top), :894 (right)
            .m_padding = QMargins(4, 4, 4, 4),                                      // :895
            .m_spacing = 3,                                                         // :893
            .m_nativeBorder = false,                                                // :892
            .m_radius = 3,                                                          // :892
            .m_idle = { .m_fill = solid(0x333333), .m_text = rgb(0xFFFFFF) },       // :891, BaseStyleSheet.qss:18
            .m_stateFaces = { { BoxElement::Disabled, { .m_fill = solid(0x333333), .m_text = rgb(0x999999) } } },     // CheckBox.qss:18-21
        };

        // ---- The same check box with a rule for :checked that sets another spacing (11px). No sheet of the engine has one; it is the contract
        // of the label of a check box, which the wizard's radio buttons show for a radio button: the size reads the spacing of the state of
        // the option, the label and the click rect are laid out by the one Qt reads with no option.
        const char* const SpacedCheckRules =
            "#SpacedDialog QCheckBox {"
            "    background-color: #333333;"
            "    border-radius: 3px;"
            "    spacing:3px;"
            "    margin-right:5px;"
            "    padding:4px;"
            "    margin-top:5px;"
            "}"
            "#SpacedDialog QCheckBox:checked {"
            "    spacing:11px;"
            "}";
        const BoxElement SpacedCheck{
            .m_name = "Test.Face.SpacedCheck",
            .m_margin = QMargins(0, 5, 5, 0),
            .m_padding = QMargins(4, 4, 4, 4),
            .m_spacing = 3,
            .m_nativeBorder = false,
            .m_radius = 3,
            .m_idle = { .m_fill = solid(0x333333), .m_text = rgb(0xFFFFFF) },
            .m_stateFaces = {
                { BoxElement::Disabled, { .m_fill = solid(0x333333), .m_text = rgb(0x999999) } },
                { BoxElement::Checked, { .m_fill = solid(0x333333), .m_text = rgb(0xFFFFFF), .m_spacing = 11 } },
                { BoxElement::Disabled | BoxElement::Checked, { .m_fill = solid(0x333333), .m_text = rgb(0x999999), .m_spacing = 11 } },
            },
        };

        // ---- A check box whose :checked rule changes the margin and the widths of the border: the hint of a widget is the box of the state it is
        // in when it computes it (CT_CheckBox reads the rule of its option, qstylesheetstyle.cpp:5399-5413), 6px and 3px of the checked rule
        // where the other has 2px and 1px
        const char* const SizedCheckRules =
            "#SizedDialog QCheckBox {"
            "    background-color: #333333;"
            "    border: 1px solid #222222;"
            "    border-radius: 2px;"
            "    spacing: 4px;"
            "    margin: 2px;"
            "    padding: 1px 3px;"
            "}"
            "#SizedDialog QCheckBox:checked {"
            "    border-width: 3px 2px;"
            "    margin: 6px 1px;"
            "}";
        BoxElement::Edges sizedCheckedEdges()
        {
            return { { 3, rgb(0x222222) }, { 2, rgb(0x222222) }, { 3, rgb(0x222222) }, { 2, rgb(0x222222) } };      // top, right, bottom, left
        }
        const BoxElement SizedCheck{
            .m_name = "Test.Face.SizedCheck",
            .m_margin = QMargins(2, 2, 2, 2),
            .m_padding = QMargins(3, 1, 3, 1),
            .m_spacing = 4,
            .m_nativeBorder = false,
            .m_radius = 2,
            .m_idle = { .m_fill = solid(0x333333), .m_edges = BoxElement::border(1, rgb(0x222222)), .m_text = rgb(0xFFFFFF) },
            .m_stateFaces = {
                { BoxElement::Disabled, { .m_fill = solid(0x333333), .m_edges = BoxElement::border(1, rgb(0x222222)), .m_text = rgb(0x999999) } },
                { BoxElement::Checked, { .m_fill = solid(0x333333), .m_edges = sizedCheckedEdges(), .m_text = rgb(0xFFFFFF),
                    .m_margin = QMargins(1, 6, 1, 6) } },
                { BoxElement::Disabled | BoxElement::Checked, { .m_fill = solid(0x333333), .m_edges = sizedCheckedEdges(), .m_text = rgb(0x999999),
                    .m_margin = QMargins(1, 6, 1, 6) } },
            },
        };

        // ---- A push button, a tool button and a line edit whose rules change the margin and the radius with the state, the way the form combo
        // box's do: no sheet of the engine has this for these classes (the text entries of BaseStyleSheet.qss:86-115 do, with a margin and a
        // border), so these rules only prove that the doors of each class give the box of the state. The :pressed rule has no border colour
        // of its own, so the pointer over a pressed button keeps the blue of :hover: an entry of its own, after both. A line edit is never
        // pressed (its State_Sunken is its frame), so its :pressed rule must not show.
        const char* const FaceDoorRules =
            "#faceButton, #faceTool, #faceEdit { margin: 3px; padding: 2px 6px; border: 1px solid #222222; border-radius: 2px;"
            " background-color: #444444; }"
            "#faceButton:hover, #faceTool:hover, #faceEdit:hover { margin: 1px; border-radius: 6px; background-color: #555555;"
            " border-color: #94D2FF; }"
            "#faceButton:pressed, #faceTool:pressed, #faceEdit:pressed { margin: 2px; border-radius: 4px; background-color: #333333; }";
        const BoxElement FaceDoor{
            .m_name = "Test.Face.Door",
            .m_margin = QMargins(3, 3, 3, 3),
            .m_padding = QMargins(6, 2, 6, 2),
            .m_nativeBorder = false,
            .m_radius = 2,
            .m_idle = { .m_fill = solid(0x444444), .m_edges = BoxElement::border(1, rgb(0x222222)) },
            .m_stateFaces = {
                { BoxElement::Hover, { .m_fill = solid(0x555555), .m_edges = BoxElement::border(1, rgb(0x94D2FF)),
                    .m_margin = QMargins(1, 1, 1, 1), .m_radius = 6 } },
                { BoxElement::Pressed, { .m_fill = solid(0x333333), .m_edges = BoxElement::border(1, rgb(0x222222)),
                    .m_margin = QMargins(2, 2, 2, 2), .m_radius = 4 } },
                { BoxElement::Hover | BoxElement::Pressed, { .m_fill = solid(0x333333), .m_edges = BoxElement::border(1, rgb(0x94D2FF)),
                    .m_margin = QMargins(2, 2, 2, 2), .m_radius = 4 } },
            },
        };

        // ---- A frame, a line edit and a push button with a box and the style's own border: the margin of the rule changes with the state and the
        // base style draws inside it (qstylesheetstyle.cpp:3651-3686, 4502-4512, 4651-4679)
        const char* const FaceNativeRules =
            "#nativeFrame, #nativeEdit, #nativeButton { margin: 6px; }"
            "#nativeFrame:hover, #nativeEdit:hover, #nativeButton:hover { margin: 2px; }";
        const BoxElement FaceNative{
            .m_name = "Test.Face.Native",
            .m_margin = QMargins(6, 6, 6, 6),
            .m_stateFaces = { { BoxElement::Hover, { .m_margin = QMargins(2, 2, 2, 2) } } },
        };

        // ---- AssetProcessor.qss:195-201: the button that chooses the remote folder of the shared cache. No border, no radius, no padding,
        // over the clear fill of BaseStyleSheet.qss:15-21, and a border image: the image stretched over the whole button.
        QString apServerToolRules(const QByteArray& image, const QString& extra)
        {
            return QStringLiteral("* { background-color: transparent; color: white; font-family: \"Open Sans\"; font-weight: 400; }"
                                  "QToolButton#serverAddressToolButton {"
                                  "    min-width: -1px;"
                                  "    border-width: 0px;"
                                  "    border-radius: 0px;"
                                  "    border-image: url(\"%1\") 0 0 0 0 stretch stretch;"
                                  "    padding: 0 0 0 0;"
                                  "}%2").arg(QString::fromUtf8(image), extra);
        }
        BoxElement ApServerTool{
            .m_name = "Test.Face.ApServerTool",
            .m_hasBox = true,                                                       // AssetProcessor.qss:200
            .m_nativeBorder = false,                                                // :197-199
            .m_idle = { .m_fill = QBrush(Qt::transparent) },                        // BaseStyleSheet.qss:17
        };                                                                          // the border image (:199) is set with the image files

        // ... the same with a border of 3px (the widths are the border image's target margins, qstylesheetstyle.cpp:1178-1179), a radius of
        // 5px (the image is clipped to it, :1169) and another image while the pointer is over it
        const char* const ApServerToolExtra =
            "QToolButton#serverAddressToolButton3 { min-width: -1px; border-width: 3px; border-radius: 0px;"
            " border-image: url(\"%1\") 0 0 0 0 stretch stretch; padding: 0 0 0 0; }"
            "QToolButton#serverAddressToolButtonRound { min-width: -1px; border-width: 0px; border-radius: 5px;"
            " border-image: url(\"%1\") 0 0 0 0 stretch stretch; padding: 0 0 0 0; }"
            "QToolButton#serverAddressToolButtonImageOnly { border-image: url(\"%1\") 0 0 0 0 stretch stretch; }"
            "QToolButton#serverAddressToolButtonHover { min-width: -1px; border-width: 0px; border-radius: 0px;"
            " border-image: url(\"%1\") 0 0 0 0 stretch stretch; padding: 0 0 0 0; }"
            "QToolButton#serverAddressToolButtonHover:hover { border-image: url(\"%2\") 0 0 0 0 stretch stretch; }";
        BoxElement ApServerToolBordered{
            .m_name = "Test.Face.ApServerToolBordered",
            .m_hasBox = true,
            .m_nativeBorder = false,
            .m_idle = { .m_fill = QBrush(Qt::transparent), .m_edges = BoxElement::border(3, QColor(Qt::transparent)) },
        };
        BoxElement ApServerToolRound{
            .m_name = "Test.Face.ApServerToolRound",
            .m_hasBox = true,
            .m_nativeBorder = false,
            .m_radius = 5,
            .m_idle = { .m_fill = QBrush(Qt::transparent) },
        };
        // ... and a border image with nothing else of a border (no width, no radius, no padding): the style's own border is gone all the
        // same, as a border image is the rule's own border, though the element keeps the flag that says it is the style's
        BoxElement ApServerToolImageOnly{
            .m_name = "Test.Face.ApServerToolImageOnly",
            .m_idle = { .m_fill = QBrush(Qt::transparent) },
        };
        BoxElement ApServerToolHover{
            .m_name = "Test.Face.ApServerToolHover",
            .m_hasBox = true,
            .m_nativeBorder = false,
            .m_idle = { .m_fill = QBrush(Qt::transparent) },
            .m_stateFaces = { { BoxElement::Hover, { .m_fill = QBrush(Qt::transparent) } } },
        };
    } // namespace BoxElementFaceTestData

    // The tests and their fixture sit in a namespace of their own: the using-directive below must not leak into the
    // other files of a unity build
    namespace BoxElementFaceTestsInternal
    {
        using namespace BoxElementFaceTestData;

        // ----------------------------------------------------------------------------------
        // Fixture: the flattened Style over Fusion, Fusion as the base of the legacy sheet, the elements registered
        // ----------------------------------------------------------------------------------

        class BoxElementFaceTests
            : public ::UnitTest::LeakDetectionFixture
        {
        public:
            BoxElementFaceTests()
            {
                m_application = AZStd::make_unique<QApplication>(m_argc, m_argv);
                QApplication::setStyle(QStyleFactory::create(QStringLiteral("Fusion")));
                m_style = AZStd::make_unique<Style>(QStyleFactory::create(QStringLiteral("Fusion")));
                registerElements();
            }

            ~BoxElementFaceTests() override
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

                const QMetaObject* combo = &QComboBox::staticMetaObject;
                BoxElement::registerElement(&PmFormCombo, combo, [](const QWidget* w) { return named(w, "formCombo"); });
                BoxElement::registerElement(&PmFormComboOneBox, combo, [](const QWidget* w) { return named(w, "formComboOneBox"); });
                BoxElement::registerElement(&SpaceCombo, combo, [](const QWidget* w) { return named(w, "spaceCombo"); });
                BoxElement::registerElement(&PmLhsRadio, &QRadioButton::staticMetaObject, [](const QWidget* w) { return childOf(w, "createAGemLHS"); });
                BoxElement::registerElement(&PmDependencyCheck, &QCheckBox::staticMetaObject, [](const QWidget* w) { return childOf(w, "GemDependenciesDialog"); });
                BoxElement::registerElement(&SpacedCheck, &QCheckBox::staticMetaObject, [](const QWidget* w) { return childOf(w, "SpacedDialog"); });
                BoxElement::registerElement(&SizedCheck, &QCheckBox::staticMetaObject, [](const QWidget* w) { return childOf(w, "SizedDialog"); });
                BoxElement::registerElement(&PaddedDropDown, combo, [](const QWidget* w) { return named(w, "paddedCombo"); });

                BoxElement::registerElement(&FaceDoor, &QPushButton::staticMetaObject, [](const QWidget* w) { return named(w, "faceButton"); });
                BoxElement::registerElement(&FaceDoor, &QLineEdit::staticMetaObject, [](const QWidget* w) { return named(w, "faceEdit"); });
                BoxElement::registerElement(&FaceDoor, &QToolButton::staticMetaObject, [](const QWidget* w) { return named(w, "faceTool"); });
                BoxElement::registerElement(&FaceNative, &QFrame::staticMetaObject, [](const QWidget* w) { return named(w, "nativeFrame"); });
                BoxElement::registerElement(&FaceNative, &QLineEdit::staticMetaObject, [](const QWidget* w) { return named(w, "nativeEdit"); });
                BoxElement::registerElement(&FaceNative, &QPushButton::staticMetaObject, [](const QWidget* w) { return named(w, "nativeButton"); });

                const QMetaObject* tool = &QToolButton::staticMetaObject;
                ApServerTool.m_idle.m_borderImage = images().m_border.constData();
                ApServerToolBordered.m_idle.m_borderImage = images().m_border.constData();
                ApServerToolRound.m_idle.m_borderImage = images().m_border.constData();
                ApServerToolImageOnly.m_idle.m_borderImage = images().m_border.constData();
                ApServerToolHover.m_idle.m_borderImage = images().m_border.constData();
                ApServerToolHover.m_stateFaces[0].m_face.m_borderImage = images().m_borderHover.constData();
                BoxElement::registerElement(&ApServerTool, tool, [](const QWidget* w) { return named(w, "serverAddressToolButton"); });
                BoxElement::registerElement(&ApServerToolBordered, tool, [](const QWidget* w) { return named(w, "serverAddressToolButton3"); });
                BoxElement::registerElement(&ApServerToolRound, tool, [](const QWidget* w) { return named(w, "serverAddressToolButtonRound"); });
                BoxElement::registerElement(&ApServerToolImageOnly, tool, [](const QWidget* w) { return named(w, "serverAddressToolButtonImageOnly"); });
                BoxElement::registerElement(&ApServerToolHover, tool, [](const QWidget* w) { return named(w, "serverAddressToolButtonHover"); });
            }

            //! The legacy twin: the sheet on the root, over the application's Fusion. The flat twin: the style on the root and on
            //! everything under it (a widget's style is not its children's).
            static void legacy(QWidget& root, const QString& rules)
            {
                root.setStyleSheet(rules);
            }

            void flat(QWidget& root) const
            {
                root.setProperty("boxElementFaceTestsFlat", true);
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
            static W* add(QWidget& parent, const char* name, const QRect& geometry = QRect())
            {
                W* widget = new W(&parent);
                widget->setObjectName(QString::fromLatin1(name));
                widget->setGeometry(geometry);
                return widget;
            }

            //! One tree twice, shown: the build function fills a root with it, the rules are the legacy twin's sheet
            struct Twins
            {
                template<typename Build>
                Twins(const BoxElementFaceTests& tests, Build build, const QString& rules, const QSize& size)
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

                //! The font is not the box: the flat widget has the one the sheet gave its twin
                void shareFont(const char* name)
                {
                    QWidget* legacyWidget = m_legacy.template findChild<QWidget*>(QString::fromLatin1(name));
                    QWidget* flatWidget = m_flat.template findChild<QWidget*>(QString::fromLatin1(name));
                    flatWidget->setFont(legacyWidget->font());
                }

                QWidget m_legacy;
                QWidget m_flat;
            };

            //! What a style draws for one call, over a grey image of an explicit device ratio: the painter is the image's, so the
            //! style scales its lines by that ratio
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

            //! The smallest rect holding the pixels of a colour
            static QRect boundsOf(const QImage& image, const QColor& color)
            {
                QRect bounds;
                for (int y = 0; y < image.height(); ++y)
                {
                    for (int x = 0; x < image.width(); ++x)
                    {
                        if (image.pixelColor(x, y) == color)
                        {
                            bounds = bounds.united(QRect(x, y, 1, 1));
                        }
                    }
                }
                return bounds;
            }

        protected:
            int m_argc = 1;                                     // QApplication keeps a reference to it
            char m_programName[5] = "test";
            char* m_argv[2] = { m_programName, nullptr };
            AZStd::unique_ptr<QApplication> m_application;
            AZStd::unique_ptr<Style> m_style;
            Qt::LayoutDirection m_direction = Qt::LeftToRight;  //!< the direction the twins of a test are laid out in
        };

        //! The colour group of the palette follows the enabled state, as QStyleOption::initFrom gives it for a real widget
        void followsEnabled(QStyleOption& option)
        {
            option.palette.setCurrentColorGroup(option.state.testFlag(QStyle::State_Enabled) ? QPalette::Active : QPalette::Disabled);
        }

        const std::initializer_list<qreal> Ratios = { 1.0, 1.5 };
        const std::initializer_list<Qt::LayoutDirection> Directions = { Qt::LeftToRight, Qt::RightToLeft };
        using States = std::initializer_list<QStyle::State>;

        std::string rectText(const QRect& rect)
        {
            return QStringLiteral("(%1, %2, %3 x %4)").arg(rect.x()).arg(rect.y()).arg(rect.width()).arg(rect.height()).toStdString();
        }

        //! The widgets that tell the option they paint with (their initStyleOption is protected), in a state and a rect
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

        class OptionFrame
            : public QFrame
        {
        public:
            using QFrame::QFrame;

            QStyleOptionFrame option(QStyle::State state, const QRect& rect) const
            {
                QStyleOptionFrame option;
                initStyleOption(&option);
                option.rect = rect;
                option.state = state;
                followsEnabled(option);
                return option;
            }
        };

        class OptionPushButton
            : public QPushButton
        {
        public:
            using QPushButton::QPushButton;

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

        //! The option a line edit paints its panel with (qlineedit.cpp:75-94): its frame is sunken, in every state
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

        // ----------------------------------------------------------------------------------
        // T1: the members of a face: unset is the element's, a rect is the box of its state, a size is the box of the polish
        // ----------------------------------------------------------------------------------

        TEST_F(BoxElementFaceTests, Face_TheNewMembersAreUnsetUntilAnElementSetsThem)
        {
            const BoxElement::Face face{};
            EXPECT_FALSE(face.m_margin.has_value());
            EXPECT_EQ(face.m_radius, BoxElement::Unset);
            EXPECT_EQ(face.m_spacing, BoxElement::Unset);
            EXPECT_STREQ(face.m_borderImage, "");

            const BoxElement element{};
            EXPECT_EQ(element.m_spacing, BoxElement::Unset);

            // a face written before the members (by designated initializers) keeps what it said
            const BoxElement::Face old{ .m_fill = solid(0x123456), .m_edges = BoxElement::border(2, rgb(0xABCDEF)), .m_text = rgb(0x010203), .m_image = "x.png" };
            EXPECT_TRUE(old.m_fill == solid(0x123456));
            EXPECT_FALSE(old.m_margin.has_value());
            EXPECT_EQ(old.m_radius, BoxElement::Unset);
        }

        //! An element with a box of its own whose :hover rule takes the margin away and rounds the box more, :focus changes the margin
        //! only, :disabled the spacing only
        const BoxElement FacedBox{
            .m_name = "Test.Face.FacedBox",
            .m_margin = QMargins(1, 2, 3, 4),
            .m_padding = QMargins(5, 6, 7, 8),
            .m_spacing = 9,
            .m_nativeBorder = false,
            .m_radius = 2,
            .m_idle = { .m_fill = solid(0xCCCCCC), .m_edges = BoxElement::border(1, rgb(0x222222)) },
            .m_stateFaces = {
                { BoxElement::Hover, { .m_fill = solid(0xCCCCCC), .m_edges = BoxElement::border(1, rgb(0x222222)), .m_margin = QMargins(0, 0, 0, 0), .m_radius = 6 } },
                { BoxElement::Focus, { .m_fill = solid(0xFFFFFF), .m_edges = BoxElement::border(1, rgb(0x222222)), .m_margin = QMargins(-1, 0, 2, 3) } },
                { BoxElement::Disabled, { .m_fill = solid(0x666666), .m_edges = BoxElement::border(1, rgb(0x222222)), .m_radius = 0, .m_spacing = 0 } },
            },
        };

        QStyleOption optionOf(QStyle::State state, const QRect& rect = QRect(0, 0, 100, 50))
        {
            QStyleOption option;
            option.state = state;
            option.rect = rect;
            return option;
        }

        TEST_F(BoxElementFaceTests, Face_TheRectsAndTheSizesOfAStateAreTheBoxOfThatState)
        {
            const QStyle::State e = QStyle::State_Enabled;
            const QStyleOption idle = optionOf(e);
            const QStyleOption hover = optionOf(e | QStyle::State_MouseOver);
            const QStyleOption focus = optionOf(e | QStyle::State_HasFocus);
            const QStyleOption disabled = optionOf(QStyle::State_None);
            const QRect whole(0, 0, 100, 50);

            // the margin: the face's, else the element's
            EXPECT_EQ(FacedBox.borderRect(&idle), whole.marginsRemoved(QMargins(1, 2, 3, 4)));
            EXPECT_EQ(FacedBox.borderRect(&hover), whole);
            EXPECT_EQ(FacedBox.borderRect(&focus), whole.marginsRemoved(QMargins(-1, 0, 2, 3)));
            EXPECT_EQ(FacedBox.borderRect(&disabled), whole.marginsRemoved(QMargins(1, 2, 3, 4)));

            // the contents: margin, border (1px in every state here) and padding of the state
            const QMargins border(1, 1, 1, 1);
            const QMargins padding(5, 6, 7, 8);
            EXPECT_EQ(FacedBox.contentsRect(&idle, nullptr), whole.marginsRemoved(QMargins(1, 2, 3, 4) + border + padding));
            EXPECT_EQ(FacedBox.contentsRect(&hover, nullptr), whole.marginsRemoved(border + padding));
            EXPECT_EQ(FacedBox.contentsRect(&focus, nullptr), whole.marginsRemoved(QMargins(-1, 0, 2, 3) + border + padding));

            // the size: the box of the state of the option, as QStyleSheetStyle::sizeFromContents reads the rule of the option it is
            // given (qstylesheetstyle.cpp:5304), so a hint is the box of the state the widget is in when it computes it
            const QSize contents(40, 20);
            const auto grown = [&](const QMargins& margin)
            {
                const QMargins box = margin + border + padding;
                return QSize(contents.width() + box.left() + box.right(), contents.height() + box.top() + box.bottom());
            };
            EXPECT_EQ(FacedBox.boxSize(&idle, nullptr, contents), grown(QMargins(1, 2, 3, 4)));
            EXPECT_EQ(FacedBox.boxSize(&hover, nullptr, contents), grown(QMargins(0, 0, 0, 0)));
            EXPECT_EQ(FacedBox.boxSize(&focus, nullptr, contents), grown(QMargins(-1, 0, 2, 3)));
            EXPECT_EQ(FacedBox.boxSize(&disabled, nullptr, contents), grown(QMargins(1, 2, 3, 4)));
            EXPECT_NE(FacedBox.boxSize(&idle, nullptr, contents), FacedBox.boxSize(&hover, nullptr, contents));

            // the spacing: the face's, else the element's; no option is the state Qt reads then, Disabled alone
            EXPECT_EQ(FacedBox.spacing(&idle), 9);
            EXPECT_EQ(FacedBox.spacing(&hover), 9);
            EXPECT_EQ(FacedBox.spacing(&disabled), 0);
            EXPECT_EQ(FacedBox.spacing(nullptr), 0);
            EXPECT_EQ(FacedBox.face(nullptr).m_fill, solid(0x666666));

            // and the element without a spacing leaves it to the style
            EXPECT_EQ(PmFormCombo.spacing(&idle), BoxElement::Unset);
            EXPECT_EQ(PmFormCombo.spacing(nullptr), BoxElement::Unset);
        }

        //! The idle face is a face like the others: where it sets a margin, a radius or a spacing they are those of the state of no
        //! pseudo-class (and of the box a widget is polished in, the limits of its size); the other faces keep the element's
        TEST_F(BoxElementFaceTests, Face_TheIdleFaceMayGiveItsOwnBox)
        {
            static const BoxElement idleBox{
                .m_name = "Test.Face.IdleBox",
                .m_margin = QMargins(5, 5, 5, 5),
                .m_spacing = 8,
                .m_nativeBorder = false,
                .m_radius = 1,
                .m_minSize = QSize(20, 10),
                .m_idle = { .m_fill = solid(0x445566), .m_margin = QMargins(1, 2, 3, 4), .m_radius = 4, .m_spacing = 2 },
                .m_stateFaces = { { BoxElement::Hover, { .m_fill = solid(0x445566) } } },
            };
            static bool registered = false;
            if (!registered)
            {
                registered = true;
                BoxElement::registerElement(&idleBox, &QFrame::staticMetaObject, [](const QWidget* w) { return named(w, "idleBoxFrame"); });
            }
            const QStyle::State e = QStyle::State_Enabled;
            const QStyleOption idle = optionOf(e);
            const QStyleOption hover = optionOf(e | QStyle::State_MouseOver);
            const QRect whole(0, 0, 100, 50);
            EXPECT_EQ(idleBox.borderRect(&idle), whole.marginsRemoved(QMargins(1, 2, 3, 4)));
            EXPECT_EQ(idleBox.borderRect(&hover), whole.marginsRemoved(QMargins(5, 5, 5, 5)));
            EXPECT_EQ(idleBox.spacing(&idle), 2);
            EXPECT_EQ(idleBox.spacing(&hover), 8);
            EXPECT_TRUE(idleBox.ownsBox(&idle));

            // the radius: 4 in the idle face, the element's 1 where a face leaves it out
            const auto look = [](const QMargins& margin, int radius)
            {
                BoxElement element;
                element.m_margin = margin;
                element.m_nativeBorder = false;
                element.m_radius = radius;
                element.m_idle = { .m_fill = solid(0x445566) };
                return element;
            };
            for (const qreal ratio : Ratios)
            {
                const auto paint = [&](const BoxElement& element, QStyle::State state)
                {
                    return BoxElementFaceTests::drawn(QSize(100, 50), ratio, [&](QPainter* painter)
                    {
                        const QStyleOption option = optionOf(state);
                        element.draw(painter, &option, nullptr);
                    });
                };
                EXPECT_EQ(BoxElementFaceTests::differing(paint(idleBox, e), paint(look(QMargins(1, 2, 3, 4), 4), e)), 0) << "idle at ratio " << ratio;
                EXPECT_EQ(BoxElementFaceTests::differing(paint(idleBox, e | QStyle::State_MouseOver), paint(look(QMargins(5, 5, 5, 5), 1), e)), 0)
                    << "hover at ratio " << ratio;
            }

            // the limits are the idle box: the contents' 20 x 10 grown by the margin of the idle face
            QWidget root;
            root.setProperty("boxElementFaceTestsFlat", true);
            QFrame* frame = BoxElementFaceTests::add<QFrame>(root, "idleBoxFrame", QRect(0, 0, 200, 80));
            root.setStyle(m_style.get());
            frame->setStyle(m_style.get());
            frame->ensurePolished();
            ASSERT_TRUE(BoxElement::forWidget(frame) == &idleBox);
            EXPECT_EQ(frame->minimumSize(), QSize(20 + 1 + 3, 10 + 2 + 4));
        }

        //! Qt takes the box from a rule that declares a margin or a spacing, even where the rule of another state has none
        TEST_F(BoxElementFaceTests, Face_AMarginOrASpacingOfAFaceMakesTheBoxOfItsStateTheRulesOwn)
        {
            static const BoxElement bare{
                .m_name = "Test.Face.Bare",
                .m_idle = { .m_fill = solid(0x445566) },
                .m_stateFaces = {
                    { BoxElement::Hover, { .m_fill = solid(0x445566), .m_margin = QMargins(0, 0, 0, 0) } },
                    { BoxElement::Focus, { .m_fill = solid(0x445566), .m_spacing = 4 } },
                },
            };
            static const BoxElement spaced{ .m_name = "Test.Face.Spaced", .m_spacing = 5 };
            const QStyle::State e = QStyle::State_Enabled;
            const QStyleOption idle = optionOf(e);
            const QStyleOption hover = optionOf(e | QStyle::State_MouseOver);
            const QStyleOption focus = optionOf(e | QStyle::State_HasFocus);
            EXPECT_FALSE(bare.ownsBox(&idle));
            EXPECT_TRUE(bare.ownsBox(&hover));
            EXPECT_TRUE(bare.ownsBox(&focus));
            EXPECT_TRUE(spaced.ownsBox(&idle));
            EXPECT_FALSE(BoxElement{}.ownsBox(&idle));
        }

        //! A face of another margin and another radius draws the box of its element written the old way for that state alone
        TEST_F(BoxElementFaceTests, Face_TheMarginAndTheRadiusOfAStateAreWhatItPaints)
        {
            // the look of each state of FacedBox, as an element that has it in every state
            const auto fixed = [](const QMargins& margin, int radius, int fill)
            {
                BoxElement element;
                element.m_margin = margin;
                element.m_nativeBorder = false;
                element.m_radius = radius;
                element.m_idle = { .m_fill = solid(fill), .m_edges = BoxElement::border(1, rgb(0x222222)) };
                return element;
            };
            const BoxElement idleLook = fixed(QMargins(1, 2, 3, 4), 2, 0xCCCCCC);
            const BoxElement hoverLook = fixed(QMargins(0, 0, 0, 0), 6, 0xCCCCCC);
            const BoxElement focusLook = fixed(QMargins(-1, 0, 2, 3), 2, 0xFFFFFF);
            const BoxElement disabledLook = fixed(QMargins(1, 2, 3, 4), 0, 0x666666);        // a radius of 0 is the face's own, not the element's 2
            const QStyle::State e = QStyle::State_Enabled;
            for (const qreal ratio : Ratios)
            {
                const auto paint = [&](const BoxElement& element, QStyle::State state)
                {
                    return BoxElementFaceTests::drawn(QSize(100, 50), ratio, [&](QPainter* painter)
                    {
                        const QStyleOption option = optionOf(state);
                        element.draw(painter, &option, nullptr);
                    });
                };
                EXPECT_EQ(BoxElementFaceTests::differing(paint(FacedBox, e), paint(idleLook, e)), 0) << "idle at ratio " << ratio;
                EXPECT_EQ(BoxElementFaceTests::differing(paint(FacedBox, e | QStyle::State_MouseOver), paint(hoverLook, e)), 0) << "hover at ratio " << ratio;
                EXPECT_EQ(BoxElementFaceTests::differing(paint(FacedBox, e | QStyle::State_HasFocus), paint(focusLook, e)), 0) << "focus at ratio " << ratio;
                EXPECT_EQ(BoxElementFaceTests::differing(paint(FacedBox, QStyle::State_None), paint(disabledLook, e)), 0) << "disabled at ratio " << ratio;
                EXPECT_GT(BoxElementFaceTests::differing(paint(disabledLook, e), paint(fixed(QMargins(1, 2, 3, 4), 2, 0x666666), e)), 0) << "ratio " << ratio;
                // the radius is a look: a box rounded by 6 is not one rounded by 2
                EXPECT_GT(BoxElementFaceTests::differing(paint(hoverLook, e), paint(fixed(QMargins(0, 0, 0, 0), 2, 0xCCCCCC), e)), 0) << "ratio " << ratio;
            }
        }

        //! The limits of a widget are those of the box it was polished in: its idle face, whatever another state does with the margin
        TEST_F(BoxElementFaceTests, Face_TheLimitsOfAWidgetAreTheIdleBoxAndAFaceOfTheStateDoesNotMoveThem)
        {
            static const BoxElement limited{
                .m_name = "Test.Face.Limited",
                .m_margin = QMargins(1, 1, 1, 1),
                .m_padding = QMargins(0, 0, 0, 0),
                .m_hasBox = true,
                .m_nativeBorder = false,
                .m_minSize = QSize(100, 30),
                .m_maxSize = QSize(100, 30),
                .m_idle = { .m_fill = solid(0xCCCCCC) },
                .m_stateFaces = { { BoxElement::Hover, { .m_fill = solid(0xCCCCCC), .m_edges = BoxElement::border(3, rgb(0x222222)), .m_margin = QMargins(0, 0, 0, 0) } } },
            };
            static bool registered = false;
            if (!registered)
            {
                registered = true;
                BoxElement::registerElement(&limited, &QFrame::staticMetaObject, [](const QWidget* w) { return named(w, "limitedFrame"); });
            }
            QWidget root;
            root.setProperty("boxElementFaceTestsFlat", true);
            QFrame* frame = BoxElementFaceTests::add<QFrame>(root, "limitedFrame", QRect(0, 0, 200, 80));
            root.setStyle(m_style.get());
            frame->setStyle(m_style.get());
            frame->ensurePolished();
            ASSERT_TRUE(BoxElement::forWidget(frame) == &limited);
            EXPECT_EQ(frame->minimumSize(), QSize(102, 32));        // the contents' 100 x 30 and the idle margin of 1: not the 106 x 36 of hover
            EXPECT_EQ(frame->maximumSize(), QSize(102, 32));
        }

        //! The rule of a widget in every state (PseudoClass_Any) has a box when the rule of any state has one, and a rule with a box paints the
        //! background of a frame (QStyleSheetStyle::polish, qstylesheetstyle.cpp:2921-2950): a face that alone has a margin is enough
        TEST_F(BoxElementFaceTests, Face_ABoxInOneStateOnlyStillMakesTheStyleTheWidgetsBackgroundPainter)
        {
            static const BoxElement hoverMargin{
                .m_name = "Test.Face.HoverMargin",
                .m_stateFaces = { { BoxElement::Hover, { .m_margin = QMargins(0, 0, 0, 0) } } },
            };
            static const BoxElement nothing{ .m_name = "Test.Face.Nothing" };
            static bool registered = false;
            if (!registered)
            {
                registered = true;
                BoxElement::registerElement(&hoverMargin, &QFrame::staticMetaObject, [](const QWidget* w) { return named(w, "hoverMarginFrame"); });
                BoxElement::registerElement(&nothing, &QFrame::staticMetaObject, [](const QWidget* w) { return named(w, "nothingFrame"); });
            }
            QWidget root;
            root.setProperty("boxElementFaceTestsFlat", true);
            QFrame* withBox = add<QFrame>(root, "hoverMarginFrame", QRect(0, 0, 50, 50));
            QFrame* withoutBox = add<QFrame>(root, "nothingFrame", QRect(0, 50, 50, 50));
            root.setStyle(m_style.get());
            for (QFrame* frame : { withBox, withoutBox })
            {
                frame->setStyle(m_style.get());
                frame->ensurePolished();
            }
            ASSERT_TRUE(BoxElement::forWidget(withBox) == &hoverMargin);
            ASSERT_TRUE(BoxElement::forWidget(withoutBox) == &nothing);
            EXPECT_TRUE(withBox->testAttribute(Qt::WA_StyledBackground));
            EXPECT_FALSE(withoutBox->testAttribute(Qt::WA_StyledBackground));
        }

        //! A rule with a border image draws, whatever else it says (hasDrawable: its border is not the style's), and the style then paints the
        //! background of a frame it covers (QStyleSheetStyle::polish, qstylesheetstyle.cpp:2921-2950); the element keeps the flag that says the
        //! border is the style's, which the border image overrides
        TEST_F(BoxElementFaceTests, Face_ABorderImageMakesTheStyleTheBackgroundPainterOfAFrame)
        {
            static BoxElement imageFrame = []
            {
                BoxElement element;
                element.m_name = "Test.Face.ImageFrame";
                element.m_idle.m_borderImage = images().m_border.constData();
                return element;
            }();
            static bool registered = false;
            if (!registered)
            {
                registered = true;
                BoxElement::registerElement(&imageFrame, &QFrame::staticMetaObject, [](const QWidget* w) { return named(w, "imageFrame"); });
            }
            const QString rules = QStringLiteral("#imageFrame { border-image: url(\"%1\") 0 0 0 0 stretch stretch; }").arg(QString::fromUtf8(images().m_border));
            Twins twins(*this, [](QWidget& root)
            {
                add<QFrame>(root, "imageFrame", QRect(5, 5, 60, 30));
            }, rules, QSize(80, 40));
            const auto* legacyFrame = twins.legacyChild<QFrame>("imageFrame");
            const auto* flatFrame = twins.flatChild<QFrame>("imageFrame");
            ASSERT_TRUE(BoxElement::forWidget(flatFrame) == &imageFrame);
            EXPECT_TRUE(legacyFrame->testAttribute(Qt::WA_StyledBackground));
            EXPECT_EQ(flatFrame->testAttribute(Qt::WA_StyledBackground), legacyFrame->testAttribute(Qt::WA_StyledBackground));
        }

        // ----------------------------------------------------------------------------------
        // T2: the combo box of a form field: its box changes with its state (ComboBox.qss:22-42 over ProjectManager.qss:149-159)
        // ----------------------------------------------------------------------------------

        TEST_F(BoxElementFaceTests, ComboBox_TheBoxOfEachStateIsTheSheetsInHintsRectsAndPixelsAtBothRatios)
        {
            for (const Qt::LayoutDirection direction : Directions)
            {
                SCOPED_TRACE(direction == Qt::RightToLeft ? "right to left" : "left to right");
                m_direction = direction;

                ASSERT_TRUE(StyleManager::stylesheetsDisabled());   // the test environment sets it (AzQtComponentTests.cpp)
                ASSERT_TRUE(appSheet().contains(QStringLiteral("QComboBox:hover")));       // the embedded sheets are the current ones

                const QStyle::State e = QStyle::State_Enabled;
                const QStyle::State h = QStyle::State_MouseOver;
                const QStyle::State f = QStyle::State_HasFocus;
                const QStyle::State s = QStyle::State_Sunken;
                // idle, hovered, focused, both, pressed (with the pointer over and not), the popup shown, disabled (focused, hovered)
                const States states = { e, e | h, e | f, e | h | f, e | s, e | s | h, e | QStyle::State_On, e | QStyle::State_On | h,
                    QStyle::State(QStyle::State_None), f, h };
                const QString rules = appSheet() + QString::fromUtf8(PmFormComboRules) + QString::fromUtf8(PmFormDropDownRules);
                for (const char* variant : { "formCombo", "formComboOneBox" })
                {
                    // The control: the element as the module had to write it while a face kept the margin and the radius of the element
                    // differs from the sheet, in the pixels of the states that change them (385 px on hover and 4531 px on focus, measured
                    // at the size of a form field)
                    const bool oneBox = qstrcmp(variant, "formComboOneBox") == 0;
                    const QString what = QLatin1String(variant);
                    Twins twins(*this, [&](QWidget& root)
                    {
                        auto* frame = add<QFrame>(root, "formFrame", QRect(0, 0, 840, 40));
                        auto* box = add<OptionComboBox>(*frame, variant, QRect(5, 5, 812, 28));
                        box->addItem(QStringLiteral("First"));
                        box->addItem(swatchIcon(0x3366CC), QStringLiteral("Second"));
                    }, rules, QSize(840, 40));
                    twins.shareFont(variant);
                    const auto* legacyCombo = static_cast<const OptionComboBox*>(twins.legacyChild<QComboBox>(variant));
                    const auto* flatCombo = static_cast<const OptionComboBox*>(twins.flatChild<QComboBox>(variant));
                    ASSERT_TRUE(BoxElement::forWidget(flatCombo) == (oneBox ? &PmFormComboOneBox : &PmFormCombo)) << what.toStdString();

                    // QComboBox gives its option State_On while the popup is shown, and the sheet reads the property its watcher sets then
                    // ([HasPopupOpen=true], ComboBox.qss:32-33): the legacy twin has it for the states that have State_On
                    QComboBox* legacyWidget = twins.legacyChild<QComboBox>(variant);
                    const auto popup = [legacyWidget](QStyle::State state)
                    {
                        const bool open = state.testFlag(QStyle::State_On);
                        if (legacyWidget->property("HasPopupOpen").toBool() != open)
                        {
                            legacyWidget->setProperty("HasPopupOpen", open);
                            legacyWidget->style()->unpolish(legacyWidget);
                            legacyWidget->style()->polish(legacyWidget);
                        }
                    };

                    // sizes and limits: 810 plus the margin of the idle box; the Native numbers are the sheet's
                    EXPECT_EQ(flatCombo->sizeHint(), legacyCombo->sizeHint()) << what.toStdString();
                    EXPECT_EQ(flatCombo->minimumSizeHint(), legacyCombo->minimumSizeHint()) << what.toStdString();
                    EXPECT_EQ(flatCombo->minimumSize(), legacyCombo->minimumSize()) << what.toStdString();
                    EXPECT_EQ(flatCombo->maximumSize(), legacyCombo->maximumSize()) << what.toStdString();
                    EXPECT_EQ(legacyCombo->minimumSize().width(), 812) << what.toStdString();
                    EXPECT_EQ(legacyCombo->sizeHint(), QSize(858, 28)) << what.toStdString();         // the n2 capture of the project page

                    // the sub-controls of every state, in the widget's rect and in another
                    int differingRects = 0;
                    for (const QStyle::State state : states)
                    {
                        popup(state);
                        for (const QRect& rect : { QRect(0, 0, 812, 28), QRect(3, 4, 500, 27) })
                        {
                            const QStyleOptionComboBox legacyOption = legacyCombo->option(state, rect);
                            const QStyleOptionComboBox flatOption = flatCombo->option(state, rect);
                            for (const QStyle::SubControl control : { QStyle::SC_ComboBoxFrame, QStyle::SC_ComboBoxEditField, QStyle::SC_ComboBoxArrow })
                            {
                                const QRect expected = legacyCombo->style()->subControlRect(QStyle::CC_ComboBox, &legacyOption, control, legacyCombo);
                                const QRect actual = flatCombo->style()->subControlRect(QStyle::CC_ComboBox, &flatOption, control, flatCombo);
                                differingRects += (actual != expected) ? 1 : 0;
                                if (!oneBox)
                                {
                                    EXPECT_EQ(actual, expected) << what.toStdString() << " " << int(control) << " in " << rectText(rect) << " state " << int(state);
                                }
                            }
                        }
                    }

                    // the size asked in every state is the box of that state: :hover and :focus trade the margin of 1px for a border of 1px,
                    // so the box of the form field is the same size in all of them (the control, which keeps its margin, is not)
                    int differingSizes = 0;
                    for (const QStyle::State state : states)
                    {
                        popup(state);
                        const QStyleOptionComboBox legacyOption = legacyCombo->option(state, QRect(0, 0, 812, 28));
                        const QStyleOptionComboBox flatOption = flatCombo->option(state, QRect(0, 0, 812, 28));
                        const QSize expected = legacyCombo->style()->sizeFromContents(QStyle::CT_ComboBox, &legacyOption, QSize(100, 18), legacyCombo);
                        const QSize actual = flatCombo->style()->sizeFromContents(QStyle::CT_ComboBox, &flatOption, QSize(100, 18), flatCombo);
                        differingSizes += (actual != expected) ? 1 : 0;
                        if (!oneBox)
                        {
                            EXPECT_EQ(actual, expected) << what.toStdString() << " state " << int(state);
                        }
                    }

                    // the pixels of every state at both ratios, except where the arrow lies: the sheet drew its image there, the flat
                    // combo box draws the component's glyph (the marker is in the legacy twin's pixels, and the flat arrow rect is its rect)
                    int differingPixels = 0;
                    for (const QStyle::State state : states)
                    {
                        popup(state);
                        const auto paint = [&](const OptionComboBox* box, qreal ratio, int index)
                        {
                            return drawn(QSize(812, 28), ratio, [&](QPainter* painter)
                            {
                                QStyleOptionComboBox option = box->option(state, QRect(0, 0, 812, 28));
                                option.currentText = index == 0 ? QStringLiteral("First") : QStringLiteral("Second");
                                option.currentIcon = index == 0 ? QIcon() : swatchIcon(0x3366CC);
                                option.iconSize = QSize(16, 16);        // the size the widget asks its style for is not the box
                                box->style()->drawComplexControl(QStyle::CC_ComboBox, &option, painter, box);
                                box->style()->drawControl(QStyle::CE_ComboBoxLabel, &option, painter, box);
                            });
                        };
                        const QRect legacyArrow = boundsOf(paint(legacyCombo, 1.0, 0), rgb(0xFF00FF));
                        ASSERT_FALSE(legacyArrow.isNull()) << what.toStdString() << " state " << int(state);
                        const QStyleOptionComboBox flatOption = flatCombo->option(state, QRect(0, 0, 812, 28));
                        if (!oneBox)
                        {
                            EXPECT_EQ(BoxElement::forWidget(flatCombo)->arrowRect(&flatOption, flatCombo).intersected(QRect(0, 0, 812, 28)), legacyArrow)
                                << what.toStdString() << " state " << int(state);
                        }
                        for (const qreal ratio : Ratios)
                        {
                            for (const int index : { 0, 1 })
                            {
                                const QImage flatImage = paint(flatCombo, ratio, index);
                                const int count = differing(flatImage, paint(legacyCombo, ratio, index), legacyArrow);
                                differingPixels += count;
                                if (!oneBox)
                                {
                                    EXPECT_EQ(count, 0) << what.toStdString() << " in state " << int(state) << " at ratio " << ratio << " item " << index;
                                    EXPECT_GT(touched(flatImage), 0);
                                }
                            }
                        }
                    }
                    if (oneBox)
                    {
                        // the control proves the test sees the difference: with one margin and one radius the hover and focus boxes are wrong
                        EXPECT_GT(differingPixels, 0);
                        EXPECT_GT(differingRects, 0);
                        EXPECT_GT(differingSizes, 0);
                    }
                }
            }
        }

        //! The gap after the icon of the label is the spacing of the state (6px where the rule has none)
        TEST_F(BoxElementFaceTests, ComboBox_TheSpacingOfTheStateIsTheGapAfterTheIconOfTheLabel)
        {
            for (const Qt::LayoutDirection direction : Directions)
            {
                SCOPED_TRACE(direction == Qt::RightToLeft ? "right to left" : "left to right");
                m_direction = direction;

                const QStyle::State e = QStyle::State_Enabled;
                const States states = { e, e | QStyle::State_MouseOver, QStyle::State(QStyle::State_None) };
                Twins twins(*this, [](QWidget& root)
                {
                    auto* box = add<OptionComboBox>(root, "spaceCombo", QRect(5, 5, 160, 30));
                    box->addItem(swatchIcon(0x3366CC), QStringLiteral("Second"));
                }, QString::fromUtf8(SpaceComboRules), QSize(170, 40));
                twins.shareFont("spaceCombo");
                const auto* legacyCombo = static_cast<const OptionComboBox*>(twins.legacyChild<QComboBox>("spaceCombo"));
                const auto* flatCombo = static_cast<const OptionComboBox*>(twins.flatChild<QComboBox>("spaceCombo"));
                ASSERT_TRUE(BoxElement::forWidget(flatCombo) == &SpaceCombo);
                EXPECT_EQ(flatCombo->sizeHint(), legacyCombo->sizeHint());

                int differingFromSix = 0;
                for (const qreal ratio : Ratios)
                {
                    for (const QStyle::State state : states)
                    {
                        const auto label = [&](const OptionComboBox* box)
                        {
                            return drawn(QSize(160, 30), ratio, [&](QPainter* painter)
                            {
                                const QStyleOptionComboBox option = box->option(state, QRect(0, 0, 160, 30));
                                box->style()->drawControl(QStyle::CE_ComboBoxLabel, &option, painter, box);
                            });
                        };
                        const QImage flatImage = label(flatCombo);
                        EXPECT_EQ(differing(flatImage, label(legacyCombo)), 0) << "state " << int(state) << " at ratio " << ratio;
                        EXPECT_GT(touched(flatImage), 0);

                        // the same element with the spacing left out (the gap Qt uses then, 6px) draws the label somewhere else
                        static const BoxElement unspaced{
                            .m_name = "Test.Face.Unspaced",
                            .m_margin = QMargins(1, 1, 1, 1),
                            .m_padding = QMargins(4, 0, 0, 0),
                            .m_nativeBorder = false,
                            .m_radius = 2,
                            .m_size = QSize(-1, 16),
                            .m_idle = { .m_fill = solid(0xCCCCCC), .m_text = rgb(0x000000) },
                            .m_stateFaces = { { BoxElement::Hover, { .m_fill = solid(0xCCCCCC), .m_text = rgb(0x000000) } } },
                        };
                        const QStyleOptionComboBox option = flatCombo->option(state, QRect(0, 0, 160, 30));
                        const QImage unspacedImage = drawn(QSize(160, 30), ratio, [&](QPainter* painter)
                        {
                            unspaced.drawComboBoxLabel(flatCombo->style(), &option, painter, flatCombo);
                        });
                        differingFromSix += differing(flatImage, unspacedImage);
                    }
                }
                EXPECT_GT(differingFromSix, 0);
            }
        }

        // ----------------------------------------------------------------------------------
        // T2b: the doors of a push button, a tool button and a line edit give the box of the state to their rects and to what they paint
        // ----------------------------------------------------------------------------------

        TEST_F(BoxElementFaceTests, Doors_ThePushButtonTheToolButtonAndTheLineEditHaveTheBoxOfTheirState)
        {
            for (const Qt::LayoutDirection direction : Directions)
            {
                SCOPED_TRACE(direction == Qt::RightToLeft ? "right to left" : "left to right");
                m_direction = direction;

                const QStyle::State e = QStyle::State_Enabled | QStyle::State_Raised;
                const QStyle::State h = QStyle::State_MouseOver;
                const QStyle::State s = QStyle::State_Sunken;
                const QStyle::State f = QStyle::State_HasFocus;
                const States states = { e, e | h, e | h | s, e | s, e | f, e | f | h, QStyle::State(QStyle::State_Raised), h | QStyle::State_Raised };
                Twins twins(*this, [](QWidget& root)
                {
                    add<OptionPushButton>(root, "faceButton", QRect(5, 5, 100, 30))->setText(QStringLiteral("Go"));
                    auto* tool = add<OptionToolButton>(root, "faceTool", QRect(5, 40, 100, 30));
                    tool->setText(QStringLiteral("Tool"));
                    tool->setToolButtonStyle(Qt::ToolButtonTextOnly);
                    add<QLineEdit>(root, "faceEdit", QRect(5, 75, 100, 30))->setText(QStringLiteral("Text"));
                }, QString::fromUtf8(FaceDoorRules), QSize(120, 110));
                const auto* legacyButton = static_cast<const OptionPushButton*>(twins.legacyChild<QPushButton>("faceButton"));
                const auto* flatButton = static_cast<const OptionPushButton*>(twins.flatChild<QPushButton>("faceButton"));
                const auto* legacyTool = static_cast<const OptionToolButton*>(twins.legacyChild<QToolButton>("faceTool"));
                const auto* flatTool = static_cast<const OptionToolButton*>(twins.flatChild<QToolButton>("faceTool"));
                QLineEdit* legacyEdit = twins.legacyChild<QLineEdit>("faceEdit");
                QLineEdit* flatEdit = twins.flatChild<QLineEdit>("faceEdit");
                ASSERT_TRUE(BoxElement::forWidget(flatButton) == &FaceDoor);
                ASSERT_TRUE(BoxElement::forWidget(flatTool) == &FaceDoor);
                ASSERT_TRUE(BoxElement::forWidget(flatEdit) == &FaceDoor);
                flatEdit->setPalette(QPalette());       // the polish gave the field the palette of the text entries, the twin has the application's

                for (const QRect& rect : { QRect(0, 0, 100, 30), QRect(3, 4, 77, 25) })
                {
                    for (const QStyle::State state : states)
                    {
                        const QStyleOptionButton legacyOption = legacyButton->option(state, rect);
                        const QStyleOptionButton flatOption = flatButton->option(state, rect);
                        EXPECT_EQ(flatButton->style()->subElementRect(QStyle::SE_PushButtonContents, &flatOption, flatButton),
                            legacyButton->style()->subElementRect(QStyle::SE_PushButtonContents, &legacyOption, legacyButton))
                            << "button contents " << rectText(rect) << " state " << int(state);

                        const QStyleOptionToolButton legacyToolOption = legacyTool->option(state, rect);
                        const QStyleOptionToolButton flatToolOption = flatTool->option(state, rect);
                        EXPECT_EQ(flatTool->style()->subControlRect(QStyle::CC_ToolButton, &flatToolOption, QStyle::SC_ToolButton, flatTool),
                            legacyTool->style()->subControlRect(QStyle::CC_ToolButton, &legacyToolOption, QStyle::SC_ToolButton, legacyTool))
                            << "tool button " << rectText(rect) << " state " << int(state);

                        const QStyleOptionFrame legacyField = fieldOption(legacyEdit, state, rect);
                        const QStyleOptionFrame flatField = fieldOption(flatEdit, state, rect);
                        EXPECT_EQ(flatEdit->style()->subElementRect(QStyle::SE_LineEditContents, &flatField, flatEdit),
                            legacyEdit->style()->subElementRect(QStyle::SE_LineEditContents, &legacyField, legacyEdit))
                            << "line edit contents " << rectText(rect) << " state " << int(state);
                    }
                }

                for (const qreal ratio : Ratios)
                {
                    for (const QStyle::State state : states)
                    {
                        const QRect rect(0, 0, 100, 30);
                        const auto button = [&](const OptionPushButton* push)
                        {
                            return drawn(rect.size(), ratio, [&](QPainter* painter)
                            {
                                const QStyleOptionButton option = push->option(state, rect);
                                push->style()->drawControl(QStyle::CE_PushButtonBevel, &option, painter, push);     // the label is the component's own colour of a disabled button
                            });
                        };
                        const auto tool = [&](const OptionToolButton* toolButton)
                        {
                            return drawn(rect.size(), ratio, [&](QPainter* painter)
                            {
                                const QStyleOptionToolButton option = toolButton->option(state, rect);
                                toolButton->style()->drawComplexControl(QStyle::CC_ToolButton, &option, painter, toolButton);
                            });
                        };
                        const auto field = [&](const QLineEdit* lineEdit)
                        {
                            return drawn(rect.size(), ratio, [&](QPainter* painter)
                            {
                                const QStyleOptionFrame option = fieldOption(lineEdit, state, rect);
                                lineEdit->style()->drawPrimitive(QStyle::PE_PanelLineEdit, &option, painter, lineEdit);
                            });
                        };
                        const QImage flatButtonImage = button(flatButton);
                        EXPECT_EQ(differing(flatButtonImage, button(legacyButton)), 0) << "button state " << int(state) << " at ratio " << ratio;
                        EXPECT_GT(touched(flatButtonImage), 0);
                        const QImage flatToolImage = tool(flatTool);
                        EXPECT_EQ(differing(flatToolImage, tool(legacyTool)), 0) << "tool button state " << int(state) << " at ratio " << ratio;
                        EXPECT_GT(touched(flatToolImage), 0);
                        const QImage flatFieldImage = field(flatEdit);
                        EXPECT_EQ(differing(flatFieldImage, field(legacyEdit)), 0) << "line edit state " << int(state) << " at ratio " << ratio;
                        EXPECT_GT(touched(flatFieldImage), 0);
                    }
                }
            }
        }

        //! The smallest rect holding what is not the grey the image started from
        QRect touchedBounds(const QImage& image)
        {
            QRect bounds;
            const qreal ratio = image.devicePixelRatio();
            for (int y = 0; y < image.height(); ++y)
            {
                for (int x = 0; x < image.width(); ++x)
                {
                    if (image.pixel(x, y) != rgb(0x808080).rgb())
                    {
                        bounds = bounds.united(QRect(x, y, 1, 1));
                    }
                }
            }
            return QRect(QPoint(qFloor(bounds.left() / ratio), qFloor(bounds.top() / ratio)),
                QPoint(qCeil((bounds.right() + 1) / ratio) - 1, qCeil((bounds.bottom() + 1) / ratio) - 1));
        }

        TEST_F(BoxElementFaceTests, Doors_TheStylesOwnBorderIsDrawnInsideTheMarginOfTheState)
        {
            for (const Qt::LayoutDirection direction : Directions)
            {
                SCOPED_TRACE(direction == Qt::RightToLeft ? "right to left" : "left to right");
                m_direction = direction;

                const QStyle::State e = QStyle::State_Enabled | QStyle::State_Raised;
                const QStyle::State h = QStyle::State_MouseOver;
                const States states = { e, e | h, QStyle::State(QStyle::State_Raised) };
                Twins twins(*this, [](QWidget& root)
                {
                    auto* frame = add<OptionFrame>(root, "nativeFrame", QRect(5, 5, 100, 30));
                    frame->setFrameStyle(QFrame::Box | QFrame::Raised);
                    frame->setLineWidth(2);
                    add<QLineEdit>(root, "nativeEdit", QRect(5, 40, 100, 30))->setText(QStringLiteral("Text"));
                    add<OptionPushButton>(root, "nativeButton", QRect(5, 75, 100, 30))->setText(QStringLiteral("Go"));
                }, QString::fromUtf8(FaceNativeRules), QSize(120, 110));
                const auto* legacyFrame = static_cast<const OptionFrame*>(twins.legacyChild<QFrame>("nativeFrame"));
                const auto* flatFrame = static_cast<const OptionFrame*>(twins.flatChild<QFrame>("nativeFrame"));
                QLineEdit* legacyEdit = twins.legacyChild<QLineEdit>("nativeEdit");
                QLineEdit* flatEdit = twins.flatChild<QLineEdit>("nativeEdit");
                const auto* legacyButton = static_cast<const OptionPushButton*>(twins.legacyChild<QPushButton>("nativeButton"));
                const auto* flatButton = static_cast<const OptionPushButton*>(twins.flatChild<QPushButton>("nativeButton"));
                ASSERT_TRUE(BoxElement::forWidget(flatFrame) == &FaceNative);
                ASSERT_TRUE(BoxElement::forWidget(flatEdit) == &FaceNative);
                ASSERT_TRUE(BoxElement::forWidget(flatButton) == &FaceNative);
                flatEdit->setPalette(QPalette());       // the polish gave the field the palette of the text entries, the twin has the application's

                const QRect rect(0, 0, 100, 30);
                for (const qreal ratio : Ratios)
                {
                    for (const QStyle::State state : states)
                    {
                        // a frame and a line edit: the base style draws both the same under the twins, so their pixels are the sheet's
                        const auto frame = [&](const OptionFrame* shaped)
                        {
                            return drawn(rect.size(), ratio, [&](QPainter* painter)
                            {
                                const QStyleOptionFrame option = shaped->option(state, rect);
                                shaped->style()->drawControl(QStyle::CE_ShapedFrame, &option, painter, shaped);
                            });
                        };
                        const auto field = [&](const QLineEdit* lineEdit)
                        {
                            return drawn(rect.size(), ratio, [&](QPainter* painter)
                            {
                                const QStyleOptionFrame option = fieldOption(lineEdit, state, rect);
                                lineEdit->style()->drawPrimitive(QStyle::PE_PanelLineEdit, &option, painter, lineEdit);
                            });
                        };
                        const QImage flatFrameImage = frame(flatFrame);
                        EXPECT_EQ(differing(flatFrameImage, frame(legacyFrame)), 0) << "frame state " << int(state) << " at ratio " << ratio;
                        EXPECT_GT(touched(flatFrameImage), 0);
                        const QImage flatFieldImage = field(flatEdit);
                        EXPECT_EQ(differing(flatFieldImage, field(legacyEdit)), 0) << "line edit state " << int(state) << " at ratio " << ratio;
                        EXPECT_GT(touched(flatFieldImage), 0);

                        // a push button: the component draws its own bevel in the rect the sheet gives the base style (the border box of the
                        // state, SE_PushButtonBevel), so what it draws lies in the rect of the legacy twin and fills it but for its rounding
                        const QStyleOptionButton legacyOption = legacyButton->option(state, rect);
                        const QRect bevel = legacyButton->style()->subElementRect(QStyle::SE_PushButtonBevel, &legacyOption, legacyButton);
                        const QImage flatButtonImage = drawn(rect.size(), ratio, [&](QPainter* painter)
                        {
                            const QStyleOptionButton option = flatButton->option(state, rect);
                            flatButton->style()->drawControl(QStyle::CE_PushButtonBevel, &option, painter, flatButton);
                        });
                        const QRect drawnBounds = touchedBounds(flatButtonImage);
                        EXPECT_TRUE(bevel.contains(drawnBounds)) << "button state " << int(state) << " at ratio " << ratio << " bevel " << rectText(bevel) << " drawn " << rectText(drawnBounds);
                        EXPECT_GE(drawnBounds.width(), bevel.width() - 2) << "button state " << int(state) << " at ratio " << ratio;
                        EXPECT_GE(drawnBounds.height(), bevel.height() - 2) << "button state " << int(state) << " at ratio " << ratio;
                    }
                }
            }
        }

        //! The size of a widget is asked of the style with the state the widget is in, and is the box of that state: QStyleSheetStyle reads the
        //! rule of the option it is given (sizeFromContents, qstylesheetstyle.cpp:5304). The margin of the rules differs by state here.
        TEST_F(BoxElementFaceTests, Doors_TheSizeOfAStateIsTheBoxOfThatState)
        {
            for (const Qt::LayoutDirection direction : Directions)
            {
                SCOPED_TRACE(direction == Qt::RightToLeft ? "right to left" : "left to right");
                m_direction = direction;

                const QStyle::State e = QStyle::State_Enabled | QStyle::State_Raised;
                const QStyle::State h = QStyle::State_MouseOver;
                const QStyle::State s = QStyle::State_Sunken;
                const States states = { e, e | h, e | h | s, e | s, QStyle::State(QStyle::State_Raised), h | QStyle::State_Raised };
                Twins twins(*this, [](QWidget& root)
                {
                    add<OptionPushButton>(root, "faceButton", QRect(5, 5, 100, 30))->setText(QStringLiteral("Go"));
                    auto* tool = add<OptionToolButton>(root, "faceTool", QRect(5, 40, 100, 30));
                    tool->setText(QStringLiteral("Tool"));
                    tool->setToolButtonStyle(Qt::ToolButtonTextOnly);
                    add<QLineEdit>(root, "faceEdit", QRect(5, 75, 100, 30))->setText(QStringLiteral("Text"));
                }, QString::fromUtf8(FaceDoorRules), QSize(120, 110));
                const auto* legacyButton = static_cast<const OptionPushButton*>(twins.legacyChild<QPushButton>("faceButton"));
                const auto* flatButton = static_cast<const OptionPushButton*>(twins.flatChild<QPushButton>("faceButton"));
                const auto* legacyTool = static_cast<const OptionToolButton*>(twins.legacyChild<QToolButton>("faceTool"));
                const auto* flatTool = static_cast<const OptionToolButton*>(twins.flatChild<QToolButton>("faceTool"));
                QLineEdit* legacyEdit = twins.legacyChild<QLineEdit>("faceEdit");
                QLineEdit* flatEdit = twins.flatChild<QLineEdit>("faceEdit");
                ASSERT_TRUE(BoxElement::forWidget(flatButton) == &FaceDoor);
                ASSERT_TRUE(BoxElement::forWidget(flatTool) == &FaceDoor);
                ASSERT_TRUE(BoxElement::forWidget(flatEdit) == &FaceDoor);

                const QRect rect(0, 0, 100, 30);
                const QSize contents(60, 16);
                int differentFromIdle = 0;
                QSize idleButton;
                for (const QStyle::State state : states)
                {
                    const QStyleOptionButton legacyOption = legacyButton->option(state, rect);
                    const QStyleOptionButton flatOption = flatButton->option(state, rect);
                    const QSize button = legacyButton->style()->sizeFromContents(QStyle::CT_PushButton, &legacyOption, contents, legacyButton);
                    EXPECT_EQ(flatButton->style()->sizeFromContents(QStyle::CT_PushButton, &flatOption, contents, flatButton), button) << "button state " << int(state);
                    idleButton = state == e ? button : idleButton;
                    differentFromIdle += (button != idleButton) ? 1 : 0;

                    const QStyleOptionToolButton legacyToolOption = legacyTool->option(state, rect);
                    const QStyleOptionToolButton flatToolOption = flatTool->option(state, rect);
                    EXPECT_EQ(flatTool->style()->sizeFromContents(QStyle::CT_ToolButton, &flatToolOption, contents, flatTool),
                        legacyTool->style()->sizeFromContents(QStyle::CT_ToolButton, &legacyToolOption, contents, legacyTool)) << "tool button state " << int(state);

                    const QStyleOptionFrame legacyField = fieldOption(legacyEdit, state, rect);
                    const QStyleOptionFrame flatField = fieldOption(flatEdit, state, rect);
                    EXPECT_EQ(flatEdit->style()->sizeFromContents(QStyle::CT_LineEdit, &flatField, contents, flatEdit),
                        legacyEdit->style()->sizeFromContents(QStyle::CT_LineEdit, &legacyField, contents, legacyEdit)) << "line edit state " << int(state);
                }
                EXPECT_GT(differentFromIdle, 0);        // the rules do change the size of the box with the state
            }
        }

        //! The drop-down of a combo box with a padding on every side: its box is the width and the padding wide, and the arrow is centered in
        //! what the padding leaves; the size of the combo box is that of the state it is asked in
        TEST_F(BoxElementFaceTests, ComboBox_TheArrowIsCenteredInWhatThePaddingOfTheDropDownLeaves)
        {
            for (const Qt::LayoutDirection direction : Directions)
            {
                SCOPED_TRACE(direction == Qt::RightToLeft ? "right to left" : "left to right");
                m_direction = direction;

                const QStyle::State e = QStyle::State_Enabled;
                const QStyle::State h = QStyle::State_MouseOver;
                const States states = { e, e | h, e | QStyle::State_HasFocus, QStyle::State(QStyle::State_None) };
                const QString rules = QString::fromUtf8(PaddedDropDownRules).arg(QString::fromUtf8(images().m_arrow));
                Twins twins(*this, [](QWidget& root)
                {
                    auto* box = add<OptionComboBox>(root, "paddedCombo", QRect(5, 5, 160, 30));
                    box->addItem(QStringLiteral("First"));
                    box->addItem(swatchIcon(0x3366CC), QStringLiteral("Second"));
                }, rules, QSize(170, 40));
                twins.shareFont("paddedCombo");
                const auto* legacyCombo = static_cast<const OptionComboBox*>(twins.legacyChild<QComboBox>("paddedCombo"));
                const auto* flatCombo = static_cast<const OptionComboBox*>(twins.flatChild<QComboBox>("paddedCombo"));
                ASSERT_TRUE(BoxElement::forWidget(flatCombo) == &PaddedDropDown);
                EXPECT_EQ(flatCombo->sizeHint(), legacyCombo->sizeHint());
                EXPECT_EQ(flatCombo->minimumSizeHint(), legacyCombo->minimumSizeHint());
                EXPECT_EQ(flatCombo->minimumSize(), legacyCombo->minimumSize());
                EXPECT_EQ(flatCombo->maximumSize(), legacyCombo->maximumSize());

                int differentFromIdle = 0;
                QSize idleSize;
                for (const QStyle::State state : states)
                {
                    const QStyleOptionComboBox legacyOption = legacyCombo->option(state, QRect(0, 0, 160, 30));
                    const QStyleOptionComboBox flatOption = flatCombo->option(state, QRect(0, 0, 160, 30));
                    const QSize size = legacyCombo->style()->sizeFromContents(QStyle::CT_ComboBox, &legacyOption, QSize(70, 14), legacyCombo);
                    EXPECT_EQ(flatCombo->style()->sizeFromContents(QStyle::CT_ComboBox, &flatOption, QSize(70, 14), flatCombo), size) << "state " << int(state);
                    idleSize = state == e ? size : idleSize;
                    differentFromIdle += (size != idleSize) ? 1 : 0;

                    for (const QRect& rect : { QRect(0, 0, 160, 30), QRect(3, 4, 121, 27) })
                    {
                        const QStyleOptionComboBox legacyRectOption = legacyCombo->option(state, rect);
                        const QStyleOptionComboBox flatRectOption = flatCombo->option(state, rect);
                        for (const QStyle::SubControl control : { QStyle::SC_ComboBoxFrame, QStyle::SC_ComboBoxEditField, QStyle::SC_ComboBoxArrow })
                        {
                            EXPECT_EQ(flatCombo->style()->subControlRect(QStyle::CC_ComboBox, &flatRectOption, control, flatCombo),
                                legacyCombo->style()->subControlRect(QStyle::CC_ComboBox, &legacyRectOption, control, legacyCombo))
                                << int(control) << " in " << rectText(rect) << " state " << int(state);
                        }
                    }

                    // the arrow of the sheet is the marker image, where the arrow rect of the state says
                    const QImage legacyImage = drawn(QSize(160, 30), 1.0, [&](QPainter* painter)
                    {
                        legacyCombo->style()->drawComplexControl(QStyle::CC_ComboBox, &legacyOption, painter, legacyCombo);
                    });
                    const QRect legacyArrow = boundsOf(legacyImage, rgb(0xFF00FF));
                    ASSERT_FALSE(legacyArrow.isNull()) << "state " << int(state);
                    EXPECT_EQ(PaddedDropDown.arrowRect(&flatOption, flatCombo).intersected(QRect(0, 0, 160, 30)), legacyArrow) << "state " << int(state);
                    for (const qreal ratio : Ratios)
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
                        EXPECT_EQ(differing(paint(flatCombo), paint(legacyCombo), legacyArrow), 0) << "state " << int(state) << " at ratio " << ratio;
                    }
                }
                EXPECT_GT(differentFromIdle, 0);        // the margin and the border of :hover do change the size of the box
            }
        }

        // ----------------------------------------------------------------------------------
        // T3: the label spacing of a radio button and a check box (qstylesheetstyle.cpp:5125-5139): the size reads it with the state of
        // the option, the label is laid out and drawn with the one Qt reads with no option
        // ----------------------------------------------------------------------------------

        TEST_F(BoxElementFaceTests, RadioButton_TheSpacingOfTheStateSizesTheButtonAndTheLabelIsLaidOutByTheIdleOne)
        {
            for (const Qt::LayoutDirection direction : Directions)
            {
                SCOPED_TRACE(direction == Qt::RightToLeft ? "right to left" : "left to right");
                m_direction = direction;

                const QStyle::State e = QStyle::State_Enabled;
                const QStyle::State h = QStyle::State_MouseOver;
                const QStyle::State f = QStyle::State_HasFocus;
                const QStyle::State on = QStyle::State_On;
                const QStyle::State off = QStyle::State_Off;
                const States states = { e | off, e | off | h, e | on, e | on | h, e | on | f, e | off | f, off, on };
                const QString rules = appSheet() + QString::fromUtf8(PmLhsRadioRules);
                Twins twins(*this, [](QWidget& root)
                {
                    auto* steps = add<QWidget>(root, "createAGemLHS", QRect(0, 0, 330, 160));
                    auto* layout = new QVBoxLayout(steps);
                    layout->setContentsMargins(0, 0, 0, 0);
                    layout->setSpacing(0);
                    for (const char* name : { "stepChecked", "stepOne", "stepTwo" })
                    {
                        auto* step = add<OptionRadioButton>(*steps, name);
                        step->setText(QStringLiteral("Step"));
                        layout->addWidget(step);
                    }
                    steps->findChild<QRadioButton*>(QStringLiteral("stepChecked"))->setChecked(true);
                }, rules, QSize(330, 160));
                for (const char* name : { "stepChecked", "stepOne" })
                {
                    twins.shareFont(name);
                }
                for (const char* name : { "stepChecked", "stepOne" })
                {
                    const auto* legacyRadio = static_cast<const OptionRadioButton*>(twins.legacyChild<QRadioButton>(name));
                    const auto* flatRadio = static_cast<const OptionRadioButton*>(twins.flatChild<QRadioButton>(name));
                    ASSERT_TRUE(BoxElement::forWidget(flatRadio) == &PmLhsRadio) << name;
                    const bool isChecked = qstrcmp(name, "stepChecked") == 0;

                    // The Native numbers: the label (180px at least) and the indicator's 20, the spacing of the state the hint was asked in
                    // (ProjectManager.qss:83, :93)
                    EXPECT_EQ(legacyRadio->sizeHint().width(), isChecked ? 180 + 20 + 30 : 180 + 20 + 10) << name;
                    EXPECT_EQ(flatRadio->sizeHint(), legacyRadio->sizeHint()) << name;
                    EXPECT_EQ(flatRadio->minimumSizeHint(), legacyRadio->minimumSizeHint()) << name;
                    EXPECT_EQ(flatRadio->minimumSize(), legacyRadio->minimumSize()) << name;
                    EXPECT_EQ(flatRadio->maximumSize(), legacyRadio->maximumSize()) << name;

                    // where the indicator and the label are in each state: the label never starts after the spacing of :checked, it starts
                    // after the 10px of the rule with no state (Qt asks for it with no option, qstylesheetstyle.cpp:6200)
                    const QRect rect(0, 0, 230, 40);
                    for (const QStyle::State state : states)
                    {
                        const QStyleOptionButton legacyOption = legacyRadio->option(state, rect);
                        const QStyleOptionButton flatOption = flatRadio->option(state, rect);
                        for (const QStyle::SubElement element : { QStyle::SE_RadioButtonIndicator, QStyle::SE_RadioButtonContents,
                                QStyle::SE_RadioButtonClickRect })
                        {
                            EXPECT_EQ(flatRadio->style()->subElementRect(element, &flatOption, flatRadio),
                                legacyRadio->style()->subElementRect(element, &legacyOption, legacyRadio)) << name << " " << int(element) << " state " << int(state);
                        }
                        const QRect contents = legacyRadio->style()->subElementRect(QStyle::SE_RadioButtonContents, &legacyOption, legacyRadio);
                        const int offset = direction == Qt::LeftToRight ? contents.left() - rect.left() : rect.right() - contents.right();
                        EXPECT_EQ(offset, 20 + 10) << name << " state " << int(state);
                    }

                    // the pixels of the whole button, the indicator's rect apart (the sheet drew nothing there, the indicator element draws)
                    const QStyleOptionButton whole = legacyRadio->option(e | off, rect);
                    const QRect indicator = legacyRadio->style()->subElementRect(QStyle::SE_RadioButtonIndicator, &whole, legacyRadio);
                    for (const qreal ratio : Ratios)
                    {
                        for (const QStyle::State state : states)
                        {
                            const auto button = [&](const OptionRadioButton* radio)
                            {
                                return drawn(rect.size(), ratio, [&](QPainter* painter)
                                {
                                    const QStyleOptionButton option = radio->option(state, rect);
                                    radio->style()->drawControl(QStyle::CE_RadioButton, &option, painter, radio);
                                });
                            };
                            const QImage flatImage = button(flatRadio);
                            EXPECT_EQ(differing(flatImage, button(legacyRadio), indicator), 0) << name << " in state " << int(state) << " at ratio " << ratio;
                            EXPECT_GT(touched(flatImage), 0) << name << " in state " << int(state);
                        }
                    }
                }

                // the list of steps is as wide as its widest step, the checked one (Native: the page's list was 24px wider than the flat one)
                QWidget* legacyList = twins.legacyChild<QWidget>("createAGemLHS");
                QWidget* flatList = twins.flatChild<QWidget>("createAGemLHS");
                EXPECT_EQ(legacyList->sizeHint().width(), 180 + 20 + 30);
                EXPECT_EQ(flatList->sizeHint(), legacyList->sizeHint());
                EXPECT_EQ(flatList->minimumSizeHint(), legacyList->minimumSizeHint());
            }
        }

        struct CheckCase
        {
            const char* m_dialog;
            QString m_rules;
            const BoxElement* m_element;
        };

        TEST_F(BoxElementFaceTests, CheckBox_TheSpacingOfTheRuleSizesTheButtonAndMovesItsLabelAndItsClickRect)
        {
            for (const Qt::LayoutDirection direction : Directions)
            {
                SCOPED_TRACE(direction == Qt::RightToLeft ? "right to left" : "left to right");
                m_direction = direction;

                const QStyle::State e = QStyle::State_Enabled;
                const QStyle::State h = QStyle::State_MouseOver;
                const QStyle::State on = QStyle::State_On;
                const QStyle::State off = QStyle::State_Off;
                const States states = { e | off, e | off | h, e | on, e | on | h, off, on };
                const CheckCase cases[] = {
                    { "GemDependenciesDialog", appSheet() + QString::fromUtf8(PmDependencyCheckRules), &PmDependencyCheck },
                    { "SpacedDialog", appSheet() + QString::fromUtf8(SpacedCheckRules), &SpacedCheck },
                    { "SizedDialog", appSheet() + QString::fromUtf8(SizedCheckRules), &SizedCheck },
                };
                for (const CheckCase& test : cases)
                {
                    int uncheckedWidth = 0;
                    for (const bool checked : { false, true })
                    {
                        for (const QString& text : { QStringLiteral("Dependency"), QString() })
                        {
                            const QString what = QStringLiteral("%1 checked %2 text [%3]").arg(QLatin1String(test.m_dialog)).arg(checked).arg(text);
                            Twins twins(*this, [&](QWidget& root)
                            {
                                auto* dialog = add<QWidget>(root, test.m_dialog, QRect(0, 0, 300, 60));
                                auto* box = add<OptionCheckBox>(*dialog, "dependency", QRect(5, 5, 200, 40));
                                box->setText(text);
                                box->setChecked(checked);
                            }, test.m_rules, QSize(300, 60));
                            twins.shareFont("dependency");
                            const auto* legacyBox = static_cast<const OptionCheckBox*>(twins.legacyChild<QCheckBox>("dependency"));
                            const auto* flatBox = static_cast<const OptionCheckBox*>(twins.flatChild<QCheckBox>("dependency"));
                            ASSERT_TRUE(BoxElement::forWidget(flatBox) == test.m_element) << what.toStdString();

                            // the size is asked in the state the box has: 3px of spacing, 11px while it is checked in the second rule
                            EXPECT_EQ(flatBox->sizeHint(), legacyBox->sizeHint()) << what.toStdString();
                            EXPECT_EQ(flatBox->minimumSizeHint(), legacyBox->minimumSizeHint()) << what.toStdString();
                            EXPECT_EQ(flatBox->minimumSize(), legacyBox->minimumSize()) << what.toStdString();
                            EXPECT_EQ(flatBox->maximumSize(), legacyBox->maximumSize()) << what.toStdString();
                            if (!text.isEmpty() && test.m_element == &SpacedCheck)
                            {
                                // the hint of the same box differs by the two spacings
                                if (!checked)
                                {
                                    uncheckedWidth = legacyBox->sizeHint().width();
                                }
                                else
                                {
                                    EXPECT_EQ(legacyBox->sizeHint().width() - uncheckedWidth, 11 - 3) << what.toStdString();
                                }
                            }

                            const QRect rect(0, 0, 200, 40);
                            for (const QStyle::State state : states)
                            {
                                const QStyleOptionButton legacyOption = legacyBox->option(state, rect);
                                const QStyleOptionButton flatOption = flatBox->option(state, rect);
                                for (const QStyle::SubElement element : { QStyle::SE_CheckBoxIndicator, QStyle::SE_CheckBoxContents,
                                        QStyle::SE_CheckBoxClickRect })
                                {
                                    EXPECT_EQ(flatBox->style()->subElementRect(element, &flatOption, flatBox),
                                        legacyBox->style()->subElementRect(element, &legacyOption, legacyBox)) << what.toStdString() << " " << int(element) << " state " << int(state);
                                }
                            }
                            for (const qreal ratio : Ratios)
                            {
                                for (const QStyle::State state : states)
                                {
                                    // the indicator is where the state puts it: the margin of :checked moves it
                                    const QStyleOptionButton whole = legacyBox->option(state, rect);
                                    const QRect indicator = legacyBox->style()->subElementRect(QStyle::SE_CheckBoxIndicator, &whole, legacyBox);
                                    const auto button = [&](const OptionCheckBox* check)
                                    {
                                        return drawn(rect.size(), ratio, [&](QPainter* painter)
                                        {
                                            const QStyleOptionButton option = check->option(state, rect);
                                            check->style()->drawControl(QStyle::CE_CheckBox, &option, painter, check);
                                        });
                                    };
                                    const QImage flatImage = button(flatBox);
                                    EXPECT_EQ(differing(flatImage, button(legacyBox), indicator), 0) << what.toStdString() << " state " << int(state) << " at ratio " << ratio;
                                    EXPECT_GT(touched(flatImage), 0) << what.toStdString();
                                }
                            }
                        }
                    }
                }
            }
        }

        // ----------------------------------------------------------------------------------
        // T4: the border image of a tool button (AssetProcessor.qss:199): stretched over the border box as QRenderRule::drawBorderImage does
        // ----------------------------------------------------------------------------------

        struct ToolCase
        {
            const char* m_name;
            const BoxElement* m_element;
            QSize m_iconSize;
        };

        TEST_F(BoxElementFaceTests, ToolButton_ABorderImageIsStretchedOverTheBorderBoxAsTheSheetDoesIt)
        {
            for (const Qt::LayoutDirection direction : Directions)
            {
                SCOPED_TRACE(direction == Qt::RightToLeft ? "right to left" : "left to right");
                m_direction = direction;

                const QStyle::State e = QStyle::State_Enabled | QStyle::State_Raised;
                const QStyle::State h = QStyle::State_MouseOver;
                const QStyle::State s = QStyle::State_Sunken;
                const States states = { e, e | h, e | h | s, e | s, QStyle::State(QStyle::State_Raised) };
                const QString extra = QString::fromUtf8(ApServerToolExtra).arg(QString::fromUtf8(images().m_border), QString::fromUtf8(images().m_borderHover));
                const ToolCase cases[] = {
                    { "serverAddressToolButton", &ApServerTool, QSize(16, 16) },
                    { "serverAddressToolButton3", &ApServerToolBordered, QSize(16, 16) },
                    { "serverAddressToolButtonRound", &ApServerToolRound, QSize(16, 16) },
                    { "serverAddressToolButtonImageOnly", &ApServerToolImageOnly, QSize(16, 16) },
                    { "serverAddressToolButtonHover", &ApServerToolHover, QSize(16, 16) },
                };
                for (const ToolCase& test : cases)
                {
                    Twins twins(*this, [&](QWidget& root)
                    {
                        auto* button = add<OptionToolButton>(root, test.m_name, QRect(5, 5, 60, 30));
                        button->setIcon(swatchIcon(0xFF8000));
                        button->setIconSize(test.m_iconSize);
                        button->setText(QStringLiteral("Choose a remote folder."));
                        button->setToolButtonStyle(Qt::ToolButtonIconOnly);
                    }, apServerToolRules(images().m_border, extra), QSize(100, 60));
                    const auto* legacyButton = static_cast<const OptionToolButton*>(twins.legacyChild<QToolButton>(test.m_name));
                    const auto* flatButton = static_cast<const OptionToolButton*>(twins.flatChild<QToolButton>(test.m_name));
                    ASSERT_TRUE(BoxElement::forWidget(flatButton) == test.m_element) << test.m_name;

                    EXPECT_EQ(flatButton->sizeHint(), legacyButton->sizeHint()) << test.m_name;
                    EXPECT_EQ(flatButton->minimumSizeHint(), legacyButton->minimumSizeHint()) << test.m_name;
                    EXPECT_EQ(flatButton->minimumSize(), legacyButton->minimumSize()) << test.m_name;
                    EXPECT_EQ(flatButton->maximumSize(), legacyButton->maximumSize()) << test.m_name;
                    if (test.m_element == &ApServerTool)
                    {
                        // the Native number: the n2 capture of the shared cache page has the button at 19 x 19, its icon of 16 and the 3 x 3
                        // Qt adds to a tool button whose rule has a box (qstylesheetstyle.cpp:5332-5334)
                        EXPECT_EQ(legacyButton->sizeHint(), QSize(19, 19));
                    }

                    // the rects and the pixels, in the shape of the button and in others (the image follows the rect it is stretched over)
                    for (const QRect& rect : { QRect(0, 0, 60, 30), QRect(0, 0, 28, 28), QRect(0, 0, 45, 17), QRect(0, 0, 17, 45), QRect(3, 2, 31, 19) })
                    {
                        const QStyleOptionToolButton legacyOption = legacyButton->option(e, rect);
                        const QStyleOptionToolButton flatOption = flatButton->option(e, rect);
                        EXPECT_EQ(flatButton->style()->subControlRect(QStyle::CC_ToolButton, &flatOption, QStyle::SC_ToolButton, flatButton),
                            legacyButton->style()->subControlRect(QStyle::CC_ToolButton, &legacyOption, QStyle::SC_ToolButton, legacyButton))
                            << test.m_name << " in " << rectText(rect);
                        for (const qreal ratio : Ratios)
                        {
                            for (const QStyle::State state : states)
                            {
                                const auto paint = [&](const OptionToolButton* tool)
                                {
                                    return drawn(rect.size() + QSize(2 * rect.x(), 2 * rect.y()), ratio, [&](QPainter* painter)
                                    {
                                        const QStyleOptionToolButton option = tool->option(state, rect);
                                        tool->style()->drawComplexControl(QStyle::CC_ToolButton, &option, painter, tool);
                                    });
                                };
                                const QImage flatImage = paint(flatButton);
                                EXPECT_EQ(differing(flatImage, paint(legacyButton)), 0)
                                    << test.m_name << " in " << rectText(rect) << " state " << int(state) << " at ratio " << ratio;
                                EXPECT_GT(touched(flatImage), 0) << test.m_name;
                            }
                        }
                    }
                }
            }
        }

        //! A border image that does not load is no border image: Qt drops it (fixupBorder, qstylesheetstyle.cpp:1129-1130), and the face
        //! draws its edges
        TEST_F(BoxElementFaceTests, ToolButton_ABorderImageThatDoesNotLoadDrawsTheEdgesOfItsFace)
        {
            static const BoxElement missing{
                .m_name = "Test.Face.Missing",
                .m_hasBox = true,
                .m_nativeBorder = false,
                .m_idle = { .m_fill = QBrush(Qt::transparent), .m_edges = BoxElement::border(2, rgb(0x00FF00)), .m_borderImage = "no/such/image.png" },
            };
            static const BoxElement edges{
                .m_name = "Test.Face.Edges",
                .m_hasBox = true,
                .m_nativeBorder = false,
                .m_idle = { .m_fill = QBrush(Qt::transparent), .m_edges = BoxElement::border(2, rgb(0x00FF00)) },
            };
            const QStyleOption option = optionOf(QStyle::State_Enabled, QRect(0, 0, 40, 24));
            for (const qreal ratio : Ratios)
            {
                const auto paint = [&](const BoxElement& element)
                {
                    return drawn(QSize(40, 24), ratio, [&](QPainter* painter) { element.draw(painter, &option, nullptr); });
                };
                EXPECT_EQ(differing(paint(missing), paint(edges)), 0);
                EXPECT_GT(touched(paint(missing)), 0);
            }
        }
    } // namespace BoxElementFaceTestsInternal
} // namespace AzQtComponents
