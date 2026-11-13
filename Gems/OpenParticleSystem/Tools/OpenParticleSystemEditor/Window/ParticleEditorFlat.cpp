/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Window/ParticleEditorFlat.h>

#include <Window/EffectorInspector.h>
#include <Window/ParticleGraphicsView.h>
#include <Window/ParticleItemWidget.h>
#include <Window/ParticleLineWidget.h>
#include <Window/PropertyEditorWidget.h>

#include <AzQtComponents/Components/Elements/BoxElement.h>
#include <AzQtComponents/Components/Elements/ElementCommon.h>
#include <AzQtComponents/Components/Elements/Foundations.h>
#include <AzQtComponents/Components/Elements/IndicatorElement.h>
#include <AzQtComponents/Components/Elements/MenuElement.h>
#include <AzQtComponents/Components/Elements/TextElement.h>
#include <AzQtComponents/Components/Style.h>
#include <AzQtComponents/Components/StyleManager.h>

#include <QCheckBox>
#include <QFrame>
#include <QLineEdit>
#include <QMenu>
#include <QToolButton>
#include <QWidget>

namespace OpenParticleSystemEditor
{
    // ----------------------------------------------------------------------------------
    // The elements of the Particle Editor
    //
    // What the widgets' own sheets drew, as complete static elements on rows of the widgets' own classes where they have one (the
    // graph view, the emitter window, the rows; ParticleLineWidget has a meta object of its own since it got Q_OBJECT), and on the
    // Qt classes of the widgets the forms build (the tool buttons and check boxes of a row, the name edit, the icon, the selection
    // lines). Each row's matcher tests the widget's own name, class or tag first and its parent only where the sheet's selector
    // reached through one (a sheet without a selector reaches its widget and everything below it).
    // ----------------------------------------------------------------------------------

    //! Everything private to this file. A named namespace, not an anonymous one: a unity build joins files into one translation unit,
    //! where anonymous namespaces merge.
    namespace ParticleEditorFlatInternal
    {
        using AzQtComponents::BoxElement;
        using AzQtComponents::IndicatorElement;
        using AzQtComponents::MenuElement;
        using AzQtComponents::TextElement;
        using AzQtComponents::Selectors::childOf;
        using AzQtComponents::Selectors::childOfClass;
        using AzQtComponents::Selectors::named;
        using AzQtComponents::Selectors::tagged;
        using AzQtComponents::Selectors::withinClass;
        namespace Foundations = AzQtComponents::Foundations;

        // ---- The names and tags the matchers read
        constexpr const char* ItemContainerName = "verticalLayoutWidget";   // ParticleItemWidget.ui:28
        constexpr const char* TitleRowName = "titleWidget";             // ParticleItemWidget.ui:51
        constexpr const char* SoloCheckName = "checkSolo";              // ParticleItemWidget.ui:108
        constexpr const char* NameEditName = "particleName";            // EffectorInspector.ui:51
        constexpr const char* NameIconName = "particNameIco";           // EffectorInspector.ui:32
        constexpr const char* LineLeftName = "lineLeft";                // ParticleItemWidget.ui:771
        constexpr const char* LineRightName = "lineRight";              // ParticleItemWidget.ui:787
        constexpr const char* LineBottomName = "lineBotton";            // ParticleItemWidget.ui:803 (the form's spelling)
        constexpr const char* LineTopName = "lineTop";                  // ParticleItemWidget.ui:819

        //! The class tag of a row that is pressed: the flattened GUI's form of the pressed row's sheet (ParticleLineWidget.cpp:40)
        constexpr const char* LinePressedTag = "ParticleLinePressed";
        //! The class tags of the name edit, which has none before it has had the focus (ParticleEditorFlat::NameEditState)
        constexpr const char* NameEditFocusedTag = "ParticleNameEditFocused";
        constexpr const char* NameEditBlurredTag = "ParticleNameEditBlurred";

        // ---- The values the elements add to the sites' (ParticleEditorFlat.h)
        //! The box of a check box's indicator: the image's size is the style's (IndicatorElement::ImageSize, CheckBox.qss:25-26) and
        //! its margin (CheckBox.qss:27) is restated, as an element takes the place of the Global one.
        //! TOKENIZE: ParticleCheckIndicatorMargin
        constexpr int CheckIndicatorMargin = 2;
        //! The line of the name edit's field: `border-width: 2px` (LineEdit.qss:16, $line.field.idle.width.Alt2).
        //! TOKENIZE: ParticleNameEditBorderWidth
        constexpr int NameEditBorderWidth = 2;

        // ----------------------------------------------------------------------------------
        // Box: the view, the emitter window, the icon, the rows and what sits in them, the name edit
        // ----------------------------------------------------------------------------------

        //! The graph view (`background-color`, ParticleGraphicsView.cpp:52): the fill of the whole view, under the grid the scene paints.
        //! Its viewport draws the view's element (BoxElement::containerOf). A margin of 0 as every scroll area has (QDockWidget.qss:35).
        //! TOKENIZE: ParticleGraphViewFill
        const BoxElement GraphView{
            .m_name = "OpenParticle.Box.GraphView",
            .m_hasBox = true,                                   // margin: 0, QDockWidget.qss:35 (QAbstractScrollArea)
            .m_idle = { .m_fill = QBrush(ParticleGraphicsViewFlat::ViewFill) },  // ParticleGraphicsView.cpp:52
        };

        //! The emitter window's fill (`background-color: #333333; border-radius: 8px`, ParticleItemWidget.cpp:54). The window is a
        //! translucent top-level window of a class with Q_OBJECT whose paint hook is misspelled (painEvent), and a translucent window's
        //! own background is never painted (QWidget::setAttribute sets WA_NoSystemBackground), so the fill the user sees is the form's
        //! plain container below it (ParticleItemWidget.ui:28, `verticalLayoutWidget`), which the sheet reached as a descendant: a
        //! rounded rect, whose corners show the translucent window through. TOKENIZE: ParticleItemWindowFill
        const BoxElement ItemContainer{
            .m_name = "OpenParticle.Box.ItemContainer",
            .m_nativeBorder = false,                                                // border-radius: ParticleItemWidget.cpp:54
            .m_radius = ParticleItemWidgetFlat::ItemWindowRadius,                   // ParticleItemWidget.cpp:54
            .m_idle = { .m_fill = QBrush(ParticleItemWidgetFlat::ItemWindowFill) }, // ParticleItemWidget.cpp:54
        };

        //! The icon beside the name (`background-color: qlineargradient`, EffectorInspector.ui:46), a plain widget 23 x 22 px:
        //! a vertical gradient. TOKENIZE: ParticleNameIconFill
        const BoxElement NameIcon{
            .m_name = "OpenParticle.Box.NameIcon",
            .m_idle = {
                .m_fill = BoxElement::vertical(EffectorInspectorFlat::NameIconTop, EffectorInspectorFlat::NameIconBottom),
            },                                                  // EffectorInspector.ui:46
        };

        //! The title row (ParticleItemWidget.ui:59-63): a vertical gradient with round top corners. The sheet has no selector, so the
        //! gradient is laid again over each tool button and check box of the row, a tile of its own on each (the same element, below).
        //! TOKENIZE: ParticleItemTitleFill
        const BoxElement Title{
            .m_name = "OpenParticle.Box.Title",
            .m_nativeBorder = false,                            // border-radius: ParticleItemWidget.ui:60-63
            .m_radii = {
                .m_topLeft = ParticleItemWidgetFlat::TitleRadius,           // ParticleItemWidget.ui:60
                .m_topRight = ParticleItemWidgetFlat::TitleRadius,          // ParticleItemWidget.ui:61
                .m_bottomRight = ParticleItemWidgetFlat::TitleRowRadius,    // ParticleItemWidget.ui:63
                .m_bottomLeft = ParticleItemWidgetFlat::TitleRowRadius,     // ParticleItemWidget.ui:62
            },
            .m_idle = {
                .m_fill = BoxElement::vertical(ParticleItemWidgetFlat::TitleTop, ParticleItemWidgetFlat::TitleBottom),    // .ui:59
            },
        };

        //! A row, idle (`background-color: #474747; border-radius: 1px`, ParticleLineWidget.cpp:18, :55 and the form's twelve sheets of
        //! the same text), and the tile the same sheet lays over each tool button of the row (a label and a check box of the row
        //! have the row's colour already, which is no change). TOKENIZE: ParticleLineIdleFill
        const BoxElement Line{
            .m_name = "OpenParticle.Box.Line",
            .m_nativeBorder = false,                            // border-radius: ParticleLineWidget.cpp:18
            .m_radius = ParticleLineWidgetFlat::LineRadius,     // ParticleLineWidget.cpp:18
            .m_idle = { .m_fill = QBrush(ParticleLineWidgetFlat::LineIdleFill) },   // ParticleLineWidget.cpp:18
        };

        //! A row while it is pressed (ParticleLineWidget.cpp:40), and the tile on its tool buttons. TOKENIZE: ParticleLinePressedFill
        const BoxElement LinePressed{
            .m_name = "OpenParticle.Box.LinePressed",
            .m_nativeBorder = false,                            // border-radius: ParticleLineWidget.cpp:40
            .m_radius = ParticleLineWidgetFlat::LineRadius,     // ParticleLineWidget.cpp:40
            .m_idle = { .m_fill = QBrush(ParticleLineWidgetFlat::LinePressedFill) },    // ParticleLineWidget.cpp:40
        };

        //! The name edit before it has had the focus (`background:transparent; color:white`, EffectorInspector.cpp:31): the field of
        //! every line edit (LineEdit.qss:11-20 over BaseStyleSheet.qss:68-84, drawn as LineEditFlat::Frame) with no fill. An element is
        //! the whole definition of its frame, so it restates the field's box: the 2px line, the padding, the height and the radius.
        //! The widget's sheet outranks the application's :disabled rule (LineEdit.qss:31-35), so no state changes the fill or the text.
        //! TOKENIZE: ParticleNameEditInitialFill
        const BoxElement NameEditInitial{
            .m_name = "OpenParticle.Box.NameEditInitial",
            .m_padding = QMargins(Foundations::Field::PaddingLeft, Foundations::Field::PaddingTop, 0, Foundations::Field::PaddingBottom),
            .m_nativeBorder = false,                            // border-style: solid, LineEdit.qss:17
            .m_radius = Foundations::Field::Radius,             // BaseStyleSheet.qss:79
            .m_size = QSize(-1, Foundations::Field::Height),    // height: 16px, LineEdit.qss:12
            .m_idle = {
                .m_fill = QBrush(EffectorInspectorFlat::NameEditIdleFill),                          // EffectorInspector.cpp:31
                .m_edges = BoxElement::border(NameEditBorderWidth, Foundations::Field::Line),   // LineEdit.qss:16, BaseStyleSheet.qss:81
            },
        };

        //! The name edit after it lost the focus (`background:#444444; border-width:0; border-style:outset`, EffectorInspector.cpp:58):
        //! a flat fill and no line; the field's padding, height and radius stay. TOKENIZE: ParticleNameEditBlurredFill
        const BoxElement NameEditBlurred{
            .m_name = "OpenParticle.Box.NameEditBlurred",
            .m_padding = QMargins(Foundations::Field::PaddingLeft, Foundations::Field::PaddingTop, 0, Foundations::Field::PaddingBottom),
            .m_nativeBorder = false,                            // border-style: outset, border-width: 0, EffectorInspector.cpp:58
            .m_radius = Foundations::Field::Radius,             // BaseStyleSheet.qss:79
            .m_size = QSize(-1, Foundations::Field::Height),    // height: 16px, LineEdit.qss:12
            .m_idle = { .m_fill = QBrush(EffectorInspectorFlat::NameEditBlurredFill) },             // EffectorInspector.cpp:58
        };

        // ----------------------------------------------------------------------------------
        // Text: the name edit's text, in each state
        // ----------------------------------------------------------------------------------

        //! `color:white` of the name edit before it has had the focus and after it lost it (EffectorInspector.cpp:31, :58), over the
        //! entry field's own text: the family and the size (BaseStyleSheet.qss:78, :82), which an element restates. The widget's sheet
        //! outranks the application's disabled colour, so the colour is the same disabled. TOKENIZE: ParticleNameEditText
        const TextElement NameEditWhite{
            .m_name = "OpenParticle.Text.NameEditWhite",
            .m_family = Foundations::Field::Family,             // BaseStyleSheet.qss:78
            .m_pixelSize = Foundations::Field::Size,            // BaseStyleSheet.qss:82
            .m_color = EffectorInspectorFlat::NameEditText,     // EffectorInspector.cpp:31, :58
        };

        //! `color:rgb(68,68,68)` of the name edit while it has the focus (EffectorInspector.cpp:54); its field is the entry field's, with
        //! the fill of every line edit. TOKENIZE: ParticleNameEditFocusedText
        const TextElement NameEditFocused{
            .m_name = "OpenParticle.Text.NameEditFocused",
            .m_family = Foundations::Field::Family,             // BaseStyleSheet.qss:78
            .m_pixelSize = Foundations::Field::Size,            // BaseStyleSheet.qss:82
            .m_color = EffectorInspectorFlat::NameEditFocusedText,      // EffectorInspector.cpp:54
        };

        // ----------------------------------------------------------------------------------
        // Indicator: the check boxes, one image for each state of the box
        // ----------------------------------------------------------------------------------

        //! The check boxes of a row and of a property editor (`QCheckBox::indicator:unchecked { image: off.svg }` and `:checked
        //! { image: on.svg }`, ParticleItemWidget.ui:141-142, PropertyEditorWidget.cpp:55-56): the images of CheckBox.qss:50-73 for
        //! every state of the two (the widget's sheet outranks the application's :focus and :disabled rules), in the box of
        //! CheckBox.qss:23-28. TOKENIZE: ParticleItemFixedCheckImages
        const IndicatorElement FixedCheck{
            .m_name = "OpenParticle.Indicator.FixedCheck",
            .m_imageSize = IndicatorElement::ImageSize,
            .m_margin = CheckIndicatorMargin,                   // CheckBox.qss:27
            .m_focusImageSize = IndicatorElement::ImageSize,
            .m_focusMargin = CheckIndicatorMargin,              // CheckBox.qss:27
            .m_stateImages = {
                { IndicatorElement::Unchecked, ParticleItemWidgetFlat::FixedUncheckedImage },   // ParticleItemWidget.ui:141
                { IndicatorElement::Checked, ParticleItemWidgetFlat::FixedCheckedImage },       // ParticleItemWidget.ui:142
            },
        };

        //! The solo check box of the title row (ParticleItemWidget.ui:120-121): the same, with the images of the solo mark.
        //! TOKENIZE: ParticleItemSoloCheckImages
        const IndicatorElement SoloCheck{
            .m_name = "OpenParticle.Indicator.SoloCheck",
            .m_imageSize = IndicatorElement::ImageSize,
            .m_margin = CheckIndicatorMargin,                   // CheckBox.qss:27
            .m_focusImageSize = IndicatorElement::ImageSize,
            .m_focusMargin = CheckIndicatorMargin,              // CheckBox.qss:27
            .m_stateImages = {
                { IndicatorElement::Unchecked, ParticleItemWidgetFlat::SoloUncheckedImage },    // ParticleItemWidget.ui:120
                { IndicatorElement::Checked, ParticleItemWidgetFlat::SoloCheckedImage },        // ParticleItemWidget.ui:121
            },
        };

        // ----------------------------------------------------------------------------------
        // Frame: the lines that frame a selected emitter window
        // ----------------------------------------------------------------------------------

        //! The four lines (`order-top: 1px solid yellow; background-color: yellow;`, ParticleItemWidget.ui:781, :797, :813, :829):
        //! `order-top` is no property, so only the fill applies. The window's sheet has no selector and reached the lines with its
        //! `border-radius: 8px` (ParticleItemWidget.cpp:54), which gives a line a border of its own (no line is drawn, and the contents
        //! are the whole rect), not the style's frame; the radius is wider than the 2px of a line, so its corners are square. A box
        //! element, as the frame element is for a frame with a border-less rule. TOKENIZE: ParticleItemSelectionLine
        const BoxElement SelectionLine{
            .m_name = "OpenParticle.Box.SelectionLine",
            .m_nativeBorder = false,                                // border-radius: ParticleItemWidget.cpp:54
            .m_radius = ParticleItemWidgetFlat::ItemWindowRadius,   // ParticleItemWidget.cpp:54
            .m_idle = { .m_fill = QBrush(ParticleItemWidgetFlat::SelectionLine) },  // ParticleItemWidget.ui:781
        };

        // ----------------------------------------------------------------------------------
        // Menu: the menu the graph view pops up
        // ----------------------------------------------------------------------------------

        //! The view's sheet (`background-color: #2D2D2D`, ParticleGraphicsView.cpp:52) has no selector, so it reaches the menu the view
        //! parents (ParticleGraphicsView::ShowMenu) as a descendant, and a sheet of an ancestor outranks the application's: the legacy
        //! menu's surface is the view's fill, not `QMenu { background-color: #222222 }` (Menu.qss:12). Everything else of the menu is the
        //! application sheet's, which the Global popup element carries, so this element is a copy of it with the surface (a copy made
        //! when first needed, as the Global element is private to MenuElement.cpp). TOKENIZE: ParticleGraphMenuSurface
        const MenuElement& graphMenu()
        {
            static const MenuElement menu = []
            {
                MenuElement element = MenuElement::forWidget(nullptr);
                element.m_name = "OpenParticle.Menu.GraphView";
                element.m_surface = ParticleGraphicsViewFlat::ViewFill;     // ParticleGraphicsView.cpp:52
                return element;
            }();
            return menu;
        }

        // ----------------------------------------------------------------------------------
        // Matchers (stateless)
        // ----------------------------------------------------------------------------------

        const char* rowClass()
        {
            return ParticleLineWidget::staticMetaObject.className();
        }

        const char* itemClass()
        {
            return ParticleItemWidget::staticMetaObject.className();
        }

        const char* viewClass()
        {
            return ParticleGraphicsView::staticMetaObject.className();
        }

        const char* inspectorClass()
        {
            return EffectorInspector::staticMetaObject.className();
        }

        const char* propertyEditorClass()
        {
            return PropertyEditorWidget::staticMetaObject.className();
        }

        //! A row on the widget's own class: every widget of the class is the one the sheet was set on
        bool always(const QWidget*)
        {
            return true;
        }

        //! The title row, a ParticleLineWidget of the form
        bool isTitleRow(const QWidget* widget)
        {
            return named(widget, TitleRowName);
        }

        //! A row that is pressed: tagged by SetLineRowPressed
        bool isPressedRow(const QWidget* widget)
        {
            return tagged(widget, LinePressedTag);
        }

        //! A tool button or check box of the title row: the row's sheet had no selector, so it reached the widgets in the row
        bool isTitleRowChild(const QWidget* widget)
        {
            return childOf(widget, TitleRowName) && childOfClass(widget, rowClass());
        }

        //! A tool button of a row that is pressed
        bool isPressedRowChild(const QWidget* widget)
        {
            return childOfClass(widget, rowClass()) && tagged(widget->parentWidget(), LinePressedTag);
        }

        //! A tool button of a row
        bool isRowChild(const QWidget* widget)
        {
            return childOfClass(widget, rowClass());
        }

        //! The solo check box of the title row
        bool isSoloCheck(const QWidget* widget)
        {
            return named(widget, SoloCheckName) && childOfClass(widget, rowClass());
        }

        //! A check box of a row or of a property editor widget (the other check boxes of the editor have no sheet of their own)
        bool isFixedCheck(const QWidget* widget)
        {
            return childOfClass(widget, rowClass()) || childOfClass(widget, propertyEditorClass());
        }

        //! The name edit of an effector inspector
        bool isNameEdit(const QWidget* widget)
        {
            return named(widget, NameEditName) && childOfClass(widget, inspectorClass());
        }

        bool isInitialNameEdit(const QWidget* widget)
        {
            return isNameEdit(widget) && !tagged(widget, NameEditFocusedTag) && !tagged(widget, NameEditBlurredTag);
        }

        bool isBlurredNameEdit(const QWidget* widget)
        {
            return isNameEdit(widget) && tagged(widget, NameEditBlurredTag);
        }

        bool isFocusedNameEdit(const QWidget* widget)
        {
            return isNameEdit(widget) && tagged(widget, NameEditFocusedTag);
        }

        //! The name edit before it has had the focus and after it lost it: the text is white
        bool isUnfocusedNameEdit(const QWidget* widget)
        {
            return isNameEdit(widget) && !tagged(widget, NameEditFocusedTag);
        }

        //! The icon beside the name edit
        bool isNameIcon(const QWidget* widget)
        {
            return named(widget, NameIconName) && childOfClass(widget, inspectorClass());
        }

        //! The plain container of the emitter window's form
        bool isItemContainer(const QWidget* widget)
        {
            return named(widget, ItemContainerName) && childOfClass(widget, itemClass());
        }

        //! One of the four lines of an emitter window
        bool isSelectionLine(const QWidget* widget)
        {
            const bool line = named(widget, LineLeftName) || named(widget, LineRightName) || named(widget, LineBottomName)
                || named(widget, LineTopName);
            return line && childOfClass(widget, itemClass());
        }

        //! The menus below the graph view, as the view's sheet reached its descendants (`Class W`)
        bool isGraphMenu(const QWidget* widget)
        {
            return withinClass(widget, viewClass());
        }

        // ----------------------------------------------------------------------------------
        // The sheets' text
        // ----------------------------------------------------------------------------------

        QString rgbaOf(const QColor& color)
        {
            return QStringLiteral("rgba(%1, %2, %3, %4)").arg(color.red()).arg(color.green()).arg(color.blue()).arg(color.alpha());
        }

        //! `qlineargradient(spread:pad, x1:0, x2:0, y1:0, y2:1, stop:0 top, stop:1 bottom)` (BoxElement::vertical)
        QString verticalGradientOf(const QColor& top, const QColor& bottom)
        {
            return QStringLiteral("qlineargradient(spread:pad, x1:0, x2:0, y1:0, y2:1, stop:0 %1, stop:1 %2)")
                .arg(rgbaOf(top), rgbaOf(bottom));
        }

        //! The sheet of a check box's indicator for the two images (the form's and PropertyEditorWidget's text)
        QString indicatorImagesSheet(const char* unchecked, const char* checked)
        {
            return QStringLiteral("QCheckBox::indicator:unchecked{image: url(%1);}\nQCheckBox::indicator:checked{image: url(%2);}")
                .arg(QString::fromLatin1(unchecked), QString::fromLatin1(checked));
        }

        //! The sheet of a name edit in a state (EffectorInspector.cpp:31, :54, :58)
        QString nameEditSheetOf(ParticleEditorFlat::NameEditState state)
        {
            using ParticleEditorFlat::NameEditState;
            switch (state)
            {
            case NameEditState::Focused:
                return QString("QLineEdit{color:%1;}").arg(EffectorInspectorFlat::NameEditFocusedText.name());
            case NameEditState::Blurred:
                return QString("QLineEdit{background:%1;border-width:0;border-style:outset;color:%2;}")
                    .arg(EffectorInspectorFlat::NameEditBlurredFill.name())
                    .arg(EffectorInspectorFlat::NameEditText.name());
            case NameEditState::Initial:
                break;
            }
            return QString("background:%1;color:%2")
                .arg(EffectorInspectorFlat::NameEditIdleFill.name(QColor::HexArgb))
                .arg(EffectorInspectorFlat::NameEditText.name());
        }
    } // namespace ParticleEditorFlatInternal

    // ----------------------------------------------------------------------------------
    // Registration
    // ----------------------------------------------------------------------------------

    namespace ParticleEditorFlat
    {
        void RegisterFlatElements()
        {
            static bool registered = false;
            if (registered)
            {
                return;
            }
            registered = true;

            namespace Flat = ParticleEditorFlatInternal;
            using AzQtComponents::BoxElement;
            using AzQtComponents::IndicatorElement;
            using AzQtComponents::MenuElement;
            using AzQtComponents::TextElement;

            // The rows of a Qt class come in the order the cascade decides them: the first that matches wins. The graph view first (its
            // own class, so every other widget is spared its matcher)
            BoxElement::registerElement(&Flat::GraphView, &ParticleGraphicsView::staticMetaObject, &Flat::always);

            // The rows: the title row, then a row pressed, then a row
            BoxElement::registerElement(&Flat::Title, &ParticleLineWidget::staticMetaObject, &Flat::isTitleRow);
            BoxElement::registerElement(&Flat::LinePressed, &ParticleLineWidget::staticMetaObject, &Flat::isPressedRow);
            BoxElement::registerElement(&Flat::Line, &ParticleLineWidget::staticMetaObject, &Flat::always);

            // What sits in the rows: the tile of the row's sheet on each tool button (the title row's first, then a pressed row's), and on
            // the check boxes of the title row
            BoxElement::registerElement(&Flat::Title, &QToolButton::staticMetaObject, &Flat::isTitleRowChild);
            BoxElement::registerElement(&Flat::LinePressed, &QToolButton::staticMetaObject, &Flat::isPressedRowChild);
            BoxElement::registerElement(&Flat::Line, &QToolButton::staticMetaObject, &Flat::isRowChild);
            BoxElement::registerElement(&Flat::Title, &QCheckBox::staticMetaObject, &Flat::isTitleRowChild);

            // The container of the emitter window's form, and the form of the effector inspector: the icon and the name edit in each state
            BoxElement::registerElement(&Flat::ItemContainer, &QWidget::staticMetaObject, &Flat::isItemContainer);
            BoxElement::registerElement(&Flat::NameIcon, &QWidget::staticMetaObject, &Flat::isNameIcon);
            BoxElement::registerElement(&Flat::NameEditBlurred, &QLineEdit::staticMetaObject, &Flat::isBlurredNameEdit);
            BoxElement::registerElement(&Flat::NameEditInitial, &QLineEdit::staticMetaObject, &Flat::isInitialNameEdit);
            TextElement::registerElement(&Flat::NameEditFocused, &QLineEdit::staticMetaObject, &Flat::isFocusedNameEdit);
            TextElement::registerElement(&Flat::NameEditWhite, &QLineEdit::staticMetaObject, &Flat::isUnfocusedNameEdit);

            // The check boxes: the solo box before the others
            IndicatorElement::registerElement(&Flat::SoloCheck, &QCheckBox::staticMetaObject, &Flat::isSoloCheck);
            IndicatorElement::registerElement(&Flat::FixedCheck, &QCheckBox::staticMetaObject, &Flat::isFixedCheck);

            // The lines of a selected emitter window, and the menu of the graph view
            BoxElement::registerElement(&Flat::SelectionLine, &QFrame::staticMetaObject, &Flat::isSelectionLine);
            MenuElement::registerElement(&Flat::graphMenu(), &QMenu::staticMetaObject, &Flat::isGraphMenu);
        }

        // ----------------------------------------------------------------------------------
        // The sheets of the legacy GUI
        // ----------------------------------------------------------------------------------

        QString NameIconSheet()
        {
            return QStringLiteral("background-color: %1;")
                .arg(ParticleEditorFlatInternal::verticalGradientOf(
                    EffectorInspectorFlat::NameIconTop, EffectorInspectorFlat::NameIconBottom));
        }

        QString TitleRowSheet()
        {
            using namespace ParticleItemWidgetFlat;
            return QStringLiteral(
                       "background-color: %1;\nborder-top-left-radius: %2px;\nborder-top-right-radius: %2px;\n"
                       "border-bottom-left-radius: %3px;\nborder-bottom-right-radius: %3px;\n")
                .arg(ParticleEditorFlatInternal::verticalGradientOf(TitleTop, TitleBottom))
                .arg(TitleRadius)
                .arg(TitleRowRadius);
        }

        QString LineRowSheet(bool pressed)
        {
            using namespace ParticleLineWidgetFlat;
            return QStringLiteral("background-color: %1; border-radius: %2px;")
                .arg((pressed ? LinePressedFill : LineIdleFill).name())
                .arg(LineRadius);
        }

        QString FixedCheckSheet()
        {
            return ParticleEditorFlatInternal::indicatorImagesSheet(
                ParticleItemWidgetFlat::FixedUncheckedImage, ParticleItemWidgetFlat::FixedCheckedImage);
        }

        QString SoloCheckSheet()
        {
            return ParticleEditorFlatInternal::indicatorImagesSheet(
                ParticleItemWidgetFlat::SoloUncheckedImage, ParticleItemWidgetFlat::SoloCheckedImage);
        }

        QString SelectionLineSheet()
        {
            // `order-top` is the form's spelling: no property, ignored by the sheet, kept as it was
            return QStringLiteral("order-top: 1px solid %1; background-color: %1; ").arg(ParticleItemWidgetFlat::SelectionLine.name());
        }

        // ----------------------------------------------------------------------------------
        // The states
        // ----------------------------------------------------------------------------------

        void SetLineRowPressed(QWidget* row, bool pressed)
        {
            using AzQtComponents::Style;
            using AzQtComponents::StyleManager;

            StyleManager::setLegacyStyleSheet(row, LineRowSheet(pressed));
            if (!StyleManager::flatStylingActive())
            {
                return;
            }

            // The row's own look is the tag's (addClass and removeClass polish the row). The tool buttons in it have resolved their
            // elements already and read the row's tag when they are polished (isPressedRowChild), so they are polished again
            const QString tag = QString::fromLatin1(ParticleEditorFlatInternal::LinePressedTag);
            if (pressed)
            {
                Style::addClass(row, tag);
            }
            else
            {
                Style::removeClass(row, tag);
            }
            for (QToolButton* button : row->findChildren<QToolButton*>(Qt::FindDirectChildrenOnly))
            {
                if (button->testAttribute(Qt::WA_WState_Polished))
                {
                    button->style()->unpolish(button);
                    button->style()->polish(button);
                }
            }
        }

        void SetNameEditState(QLineEdit* edit, NameEditState state)
        {
            using AzQtComponents::Style;
            using AzQtComponents::StyleManager;

            StyleManager::setLegacyStyleSheet(edit, ParticleEditorFlatInternal::nameEditSheetOf(state));
            if (!StyleManager::flatStylingActive())
            {
                return;
            }

            // The edit has no tag before it has had the focus: that is the initial look. A change of tag polishes the edit again
            const QString focused = QString::fromLatin1(ParticleEditorFlatInternal::NameEditFocusedTag);
            const QString blurred = QString::fromLatin1(ParticleEditorFlatInternal::NameEditBlurredTag);
            switch (state)
            {
            case NameEditState::Initial:
                Style::removeClass(edit, focused);
                Style::removeClass(edit, blurred);
                break;
            case NameEditState::Focused:
                Style::removeClass(edit, blurred);
                Style::addClass(edit, focused);
                break;
            case NameEditState::Blurred:
                Style::removeClass(edit, focused);
                Style::addClass(edit, blurred);
                break;
            }
        }
    } // namespace ParticleEditorFlat
} // namespace OpenParticleSystemEditor
