/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <FlatElements.h>

#include <AzQtComponents/Components/Elements/BoxElement.h>
#include <AzQtComponents/Components/Elements/FrameElement.h>
#include <AzQtComponents/Components/Elements/IndicatorElement.h>
#include <AzQtComponents/Components/Elements/ItemViewElement.h>
#include <AzQtComponents/Components/Elements/TabElement.h>
#include <AzQtComponents/Components/Elements/TextElement.h>

#include <QBrush>
#include <QCheckBox>
#include <QColor>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFrame>
#include <QLabel>
#include <QLineEdit>
#include <QListView>
#include <QMainWindow>
#include <QMargins>
#include <QProgressBar>
#include <QPushButton>
#include <QRadioButton>
#include <QScrollArea>
#include <QSize>
#include <QStackedWidget>
#include <QString>
#include <QTabBar>
#include <QTableView>
#include <QVariant>
#include <QWidget>

// =====================================================================================================================
// ProjectManager.qss in the flattened GUI
//
// With style sheets disabled the Project Manager carries its sheet (Resources/ProjectManager.qss) as elements of
// AzQtComponents: frames, boxes, text and tabs. StyleManager runs RegisterFlatElements once, when the sheet is first set
// on the main window (Application::Run); the legacy GUI never calls it.
//
// Every value is the final one the legacy GUI rendered for the widgets its element covers (the winners of Qt's cascade,
// replayed over the captures of the legacy GUI), and the comment beside it names the declaration that grants it as file:line:
// ProjectManager.qss, or the AzQtComponents sheet that reached the widget too (BaseStyleSheet.qss, Text.qss, PushButton.qss,
// ...). Nothing is estimated.
//
// - An element is a complete definition; none is built from another. A matcher stands for the selector of the rules.
//   Of the rows that match a widget the first registered wins, so the cascade is written as an order: the rule that is more
//   specific, or comes later in the sheet, is registered first.
// - The same goes for the other registries a rule can reach: the indicator of a check box or radio button (IndicatorElement:
//   `::indicator`, ProjectManager.qss:97-99) and the item views (ItemViewElement: the header of the gem list, :872-874).
// - UNVERIFIED: no capture shows the widget (a dialog, a popup), or the widget in a state (hover, pressed, focus, disabled: the
//   comment above the state faces of the element names them). Its values are the cascade's; its look has not been compared yet.
// - OPEN: a declaration that no element can hold, with the reason.
// - m_nativeBorder is false where the winning declarations give the border a style of its own, a `border` or a `border-radius`
//   (which cancels Qt's native border); its comment names that declaration. m_hasBox is set where a margin or padding of 0 is written.
// - TOKENIZE: a colour that will become a token.
// - A declaration Qt ignores is not carried: ProjectManager.qss:393 `font-size: 12` (a font size needs px or pt, qcssparser.cpp:1188)
//   and :472 `font-weight: 600px` (a weight is a number or a keyword, qcssparser.cpp:1247). A length in % is read as its number
//   (`max-width: 20%` is 20px, qcssparser.cpp:439-467 and 2960-2963), and `font-weight: bold` is 700 (qcssparser.cpp:1242): an element
//   carries what Qt applied.
// - The inline sheets of the code (GemInspector.cpp:309, GemListHeaderWidget.cpp:28, GemFilterTagWidget.cpp:27 and :38,
//   GemRequirementListView.cpp:19, LinkWidget.cpp:56 and :76) still run in the flattened GUI (QStyleSheetStyle proxies them),
//   so they need nothing here: the widgets they style keep what Qt applies from them.
// - Declarations that change nothing a widget draws or is sized by are not carried: the text-align of a label (a label has its own
//   alignment property), line-height (Qt reads it for rich text only), subcontrol-origin and -position, and outline. The text-align of
//   a push button is carried (BoxElement::m_textAlign): a rule with a position has the sheet lay the icon and the text of its buttons
//   out itself, 4px apart where the base style's are 2px. The spacing of a radio button and a check box is carried (BoxElement::m_spacing
//   and the faces'), and the drop-down of a combo box (m_dropDownWidth, m_dropDownPadding). Sub-controls with no element family (the
//   popup rows of the form combo box, ProjectManager.qss:170-174 and :334-338, ::chunk, the ::indicator of the tag chips) are marked
//   OPEN where their widget's element is.
// - Selectors name classes and ancestors as Qt matches them: `#a B` is any ancestor named a, `#a > B` the parent only, a type
//   selector includes the subclasses (a QLabel is a QFrame), and a rule of a dead selector (a name no widget has: #createAGem
//   #formFrame, #createGemTab #formFrame, #gemCatalogUninstallGemButton, #gemRepoHeaderRefreshButton) is not registered.
// =====================================================================================================================

namespace O3DE::ProjectManager
{
    //! Everything private to this file. A named namespace, not an anonymous one: a unity build joins this file with others
    //! into one translation unit, where anonymous namespaces merge.
    namespace FlatElementsInternal
    {
        using AzQtComponents::BoxElement;
        using AzQtComponents::FrameElement;
        using AzQtComponents::IndicatorElement;
        using AzQtComponents::ItemViewElement;
        using AzQtComponents::TabElement;
        using AzQtComponents::TextElement;
        using AzQtComponents::Selectors::childOf;
        using AzQtComponents::Selectors::hasProperty;
        using AzQtComponents::Selectors::named;
        using AzQtComponents::Selectors::within;

        // ------------------------------------------------------------------------------
        // Selectors this file composes from AzQtComponents' (ElementCommon.h)
        // ------------------------------------------------------------------------------

        //! #name > QFrame > W: the parent of the widget is a QFrame (a label, a scroll area, a stack is one) that is a child of #name
        bool parentChildOf(const QWidget* w, const char* name)
        {
            const QWidget* parent = w->parentWidget();
            return parent && parent->metaObject()->inherits(&QFrame::staticMetaObject) && childOf(parent, name);
        }

        //! #outer Class[name="value"] W: some ancestor of the widget, not only its parent, is of the class and passes the test
        bool withinMatching(const QWidget* w, const QMetaObject* ancestorClass, bool (*test)(const QWidget*))
        {
            for (const QWidget* ancestor = w->parentWidget(); ancestor; ancestor = ancestor->parentWidget())
            {
                if (ancestor->metaObject()->inherits(ancestorClass) && test(ancestor))
                {
                    return true;
                }
            }
            return false;
        }

        // ------------------------------------------------------------------------------
        // Colours, named by purpose. A colour the legacy sheets use for two purposes is two constants.
        // ------------------------------------------------------------------------------

        const QColor Transparent(0, 0, 0, 0);                 // BaseStyleSheet.qss:17 `* { background-color: transparent }`
        // BaseStyleSheet.qss:18 `* { color: white }`; the white outlines of ProjectManager.qss:631, 687
        const QColor White(0xFF, 0xFF, 0xFF);
        const QColor HeaderBandFill(0x13, 0x13, 0x13);        // TOKENIZE: HeaderBandFill (ProjectManager.qss:3)
        const QColor HeaderFill(0x11, 0x11, 0x11);            // TOKENIZE: HeaderFill (ProjectManager.qss:403)
        const QColor TopBarFill(0x1E, 0x25, 0x2F);            // TOKENIZE: TopBarFill (ProjectManager.qss:607, 981, 1037)
        // TOKENIZE: PageFill (the grey body of a page: ProjectManager.qss:102, 227, 448, 803, 863, 891, 920, 999)
        const QColor PageFill(0x33, 0x33, 0x33);
        // TOKENIZE: PanelFill (a lighter panel: ProjectManager.qss:210, 452, 515, 869, 942, 977, 1084)
        const QColor PanelFill(0x44, 0x44, 0x44);
        const QColor WizardFooterFill(0x00, 0x00, 0x00);      // TOKENIZE: WizardFooterFill (ProjectManager.qss:463)
        const QColor CardFill(0x00, 0x00, 0x00, 0xB4);        // TOKENIZE: StartCardFill (ProjectManager.qss:649: rgba(0,0,0,180))
        const QColor ScrimFill(0x00, 0x00, 0x00, 0xB2);       // TOKENIZE: CardScrimFill (ProjectManager.qss:738: rgba(0,0,0,0.7) is 178)
        const QColor CountBadgeFill(0x42, 0x85, 0xF4);        // TOKENIZE: CountBadgeFill (ProjectManager.qss:816)
        const QColor CartFill(0x55, 0x55, 0x55);              // TOKENIZE: CartFill (ProjectManager.qss:811)
        const QColor ChipFill(0x55, 0x55, 0x55);              // TOKENIZE: ChipFill (ProjectManager.qss:233)
        const QColor SummaryLine(0x55, 0x55, 0x55);           // TOKENIZE: SummaryLine (ProjectManager.qss:538)
        const QColor FormBorder(0xDD, 0xDD, 0xDD);            // TOKENIZE: FormBorder (ProjectManager.qss:211)
        const QColor FormBorderDisabled(0x95, 0x95, 0x95);    // TOKENIZE: FormBorderDisabled (ProjectManager.qss:262)
        const QColor FormBorderInvalid(0xFF, 0x00, 0x00);     // TOKENIZE: FormBorderInvalid (ProjectManager.qss:258: red)
        const QColor ToastFill(0x00, 0x00, 0x00);             // TOKENIZE: ToastFill (ProjectManager.qss:182: black)
        const QColor ToastBorder(0xDD, 0xDD, 0xDD);           // TOKENIZE: ToastBorder (ProjectManager.qss:184)
        const QColor AccentBlue(0x1E, 0x70, 0xEB);            // TOKENIZE: AccentFill (ProjectManager.qss:635, 693, 1004...)
        const QColor AccentBluePressed(0x0E, 0x60, 0xEB);     // TOKENIZE: AccentFillPressed (ProjectManager.qss:644)
        const QColor LinkBlue(0x94, 0xD2, 0xFF);              // TOKENIZE: LinkText (ProjectManager.qss:239, 1024)

        // ProjectManager.qss:15-59: the face of a push button role is a vertical gradient per state
        // TOKENIZE: PrimaryFill (ProjectManager.qss:17-18)
        const QBrush PrimaryFill = BoxElement::vertical(QColor(0x00, 0x95, 0xF2), QColor(0x1E, 0x70, 0xEB));
        // TOKENIZE: PrimaryHoverFill (ProjectManager.qss:21-22)
        const QBrush PrimaryHoverFill = BoxElement::vertical(QColor(0x10, 0xA5, 0xF2), QColor(0x2E, 0x80, 0xEB));
        // TOKENIZE: PrimaryPressedFill (ProjectManager.qss:25-26)
        const QBrush PrimaryPressedFill = BoxElement::vertical(QColor(0x00, 0x85, 0xE2), QColor(0x0E, 0x60, 0xDB));
        // TOKENIZE: PrimaryDisabledFill (ProjectManager.qss:29-30)
        const QBrush PrimaryDisabledFill = BoxElement::vertical(QColor(0x77, 0x77, 0x77), QColor(0x44, 0x44, 0x44));
        // TOKENIZE: SecondaryFill (ProjectManager.qss:35-36)
        const QBrush SecondaryFill = BoxElement::vertical(QColor(0x88, 0x88, 0x88), QColor(0x55, 0x55, 0x55));
        // TOKENIZE: SecondaryHoverFill (ProjectManager.qss:39-40)
        const QBrush SecondaryHoverFill = BoxElement::vertical(QColor(0x99, 0x99, 0x99), QColor(0x66, 0x66, 0x66));
        // TOKENIZE: SecondaryPressedFill (ProjectManager.qss:43-44)
        const QBrush SecondaryPressedFill = BoxElement::vertical(QColor(0x55, 0x55, 0x55), QColor(0x77, 0x77, 0x77));
        // TOKENIZE: DangerFill (ProjectManager.qss:49-50)
        const QBrush DangerFill = BoxElement::vertical(QColor(0xE3, 0x2C, 0x27), QColor(0x95, 0x1D, 0x21));
        // TOKENIZE: DangerHoverFill (ProjectManager.qss:53-54)
        const QBrush DangerHoverFill = BoxElement::vertical(QColor(0xFD, 0x31, 0x29), QColor(0xAF, 0x22, 0x21));
        // TOKENIZE: DangerPressedFill (ProjectManager.qss:57-58)
        const QBrush DangerPressedFill = BoxElement::vertical(QColor(0x95, 0x1D, 0x1F), QColor(0xC9, 0x27, 0x24));

        // ------------------------------------------------------------------------------
        // Frames
        // ------------------------------------------------------------------------------

        // ProjectManager.qss:481-483 `#horizontalSeparatingLine { color: #666666 }` and :485-487 `#verticalSeparatingLine
        // { color: #888888 }`: the QFrame dividers of the dialogs and of the Gem catalog (HLine, VLine; Plain, line width 1).
        // The rules only set the color: the frames have no class tag, so `.QFrame[frameShape]` fills them as it fills
        // every divider (BaseStyleSheet.qss:41-48). TOKENIZE: HorizontalSeparatorColor, VerticalSeparatorColor
        const FrameElement HorizontalSeparator{
            .m_name = "ProjectManager.Frame.HorizontalSeparator",
            .m_fill = QColor(0x44, 0x44, 0x44),                                 // BaseStyleSheet.qss:47
            .m_fillShapes = FrameElement::PanelShapes,                          // BaseStyleSheet.qss:41-45
            .m_foreground = QColor(0x66, 0x66, 0x66),                           // ProjectManager.qss:482
        };
        const FrameElement VerticalSeparator{
            .m_name = "ProjectManager.Frame.VerticalSeparator",
            .m_fill = QColor(0x44, 0x44, 0x44),                                 // BaseStyleSheet.qss:47
            .m_fillShapes = FrameElement::PanelShapes,                          // BaseStyleSheet.qss:41-45
            .m_foreground = QColor(0x88, 0x88, 0x88),                           // ProjectManager.qss:486
        };

        void registerFrames()
        {
            FrameElement::registerElement(&HorizontalSeparator, &QFrame::staticMetaObject, [](const QWidget* w)
                {
                    return named(w, "horizontalSeparatingLine");
                });
            FrameElement::registerElement(&VerticalSeparator, &QFrame::staticMetaObject, [](const QWidget* w)
                {
                    return named(w, "verticalSeparatingLine");
                });
        }

        // ------------------------------------------------------------------------------
        // Boxes
        // ------------------------------------------------------------------------------

        // ---- The window and the widgets of no particular place

        //! A progress bar (QProgressBar): no border, a clear track. UNVERIFIED: no capture shows one.
        const BoxElement ProgressBar{
            .m_name = "ProjectManager.Box.ProgressBar",
            .m_hasBox = true,                                 // a margin or padding of 0: ProjectManager.qss:775 (QProgressBar)
            .m_nativeBorder = false,                          // border: ProjectManager.qss:773 (QProgressBar)
            .m_size = QSize(-1, 5),                           // ProgressBar.qss:15 (QProgressBar)
            .m_idle = { .m_fill = QBrush(Transparent) },      // ProjectManager.qss:774 (QProgressBar)
            // OPEN the track: this clear fill (ProjectManager.qss:774) wants a QProgressBar door; the style paints the groove with the
            // palette's Base, #9A9A9A (Style.cpp:2849-2857). The chunk (::chunk, ProjectManager.qss:778-780) is #1E70EB, the palette's
            // Highlight there: nothing to carry
            .m_properties = { { "textVisible", false } },     // ProgressBar.qss:18 (QProgressBar)
        };

        //! The stack of screens (#ScreensCtrl): at least 1200x700.
        const BoxElement ScreensCtrl{
            .m_name = "ProjectManager.Box.ScreensCtrl",
            .m_minSize = QSize(1200, 700),                    // ProjectManager.qss:11,12 (#ScreensCtrl)
            .m_idle = { .m_fill = QBrush(Transparent) },      // BaseStyleSheet.qss:17 (*)
        };

        //! The window (QMainWindow): the dark band behind the tabs, and the O3DE logo laid in its content box (padding 25px 16px,
        //! background-origin: content).
        const BoxElement MainWindow{
            .m_name = "ProjectManager.Box.MainWindow",
            .m_padding = QMargins(16, 25, 16, 25),            // ProjectManager.qss:6 (QMainWindow)
            // ProjectManager.qss:3 (QMainWindow)
            .m_idle = { .m_fill = QBrush(HeaderBandFill), .m_image = ":/o3de.svg" },
            .m_imageOrigin = BoxElement::Origin::Content,     // ProjectManager.qss:3,5 (QMainWindow)
        };

        // ---- Screen chrome: headers, footers and surfaces

        // The footer buttons carry no maximum height. The sheet gives them max-height: 28px (ProjectManager.qss:469), but
        // PushButton::polish gives every push button a maximum height of 24 (PushButton.cpp:107-110) after the sheet's geometry, and
        // DialogButtonBox::polish repolishes its buttons through the style below the sheet's (DialogButtonBox.cpp:23-34). Native's maximum
        // is 24 (the n2 dumps): an element must not fight the code.
        //! A primary button of a footer (#footer > QPushButton[primary]): the blue gradient on the footer's button box.
        const BoxElement FooterButtonPrimary{
            .m_name = "ProjectManager.Box.FooterButtonPrimary",
            .m_margin = QMargins(0, 0, 30, 0),                // ProjectManager.qss:471 (#footer > QPushButton)
            .m_nativeBorder = false,                          // border-radius: ProjectManager.qss:467 (#footer > QPushButton)
            .m_radius = 3,                                    // ProjectManager.qss:467 (#footer > QPushButton)
            .m_minSize = QSize(150, 28),                      // ProjectManager.qss:468,470 (#footer > QPushButton)
            // no maximum height: see the note above FooterButtonPrimary (ProjectManager.qss:469)
            .m_idle = { .m_fill = PrimaryFill },              // ProjectManager.qss:17 (QPushButton[primary="true"])
            // UNVERIFIED states: hover, pressed, disabled (no capture shows the widget in them)
            .m_stateFaces = {
                // ProjectManager.qss:21 (QPushButton[primary="true"]:hover)
                { BoxElement::Hover, { .m_fill = PrimaryHoverFill } },
                // ProjectManager.qss:25 (QPushButton[primary="true"]:pressed)
                { BoxElement::Pressed, { .m_fill = PrimaryPressedFill } },
                // ProjectManager.qss:29 (QPushButton[primary="true"]:disabled)
                { BoxElement::Disabled, { .m_fill = PrimaryDisabledFill } },
            },
            .m_properties = { { "flat", true } },             // ProjectManager.qss:16 (QPushButton[primary="true"])
        };

        //! A secondary button of a footer: the grey gradient on the same box.
        const BoxElement FooterButtonSecondary{
            .m_name = "ProjectManager.Box.FooterButtonSecondary",
            .m_margin = QMargins(0, 0, 30, 0),                // ProjectManager.qss:471 (#footer > QPushButton)
            .m_nativeBorder = false,                          // border-radius: ProjectManager.qss:467 (#footer > QPushButton)
            .m_radius = 3,                                    // ProjectManager.qss:467 (#footer > QPushButton)
            .m_minSize = QSize(150, 28),                      // ProjectManager.qss:468,470 (#footer > QPushButton)
            // no maximum height: see the note above FooterButtonPrimary (ProjectManager.qss:469)
            .m_idle = { .m_fill = SecondaryFill },            // ProjectManager.qss:35 (QPushButton[secondary="true"])
            // UNVERIFIED states: hover, pressed (no capture shows the widget in them)
            .m_stateFaces = {
                // ProjectManager.qss:39 (QPushButton[secondary="true"]:hover)
                { BoxElement::Hover, { .m_fill = SecondaryHoverFill } },
                // ProjectManager.qss:43 (QPushButton[secondary="true"]:pressed)
                { BoxElement::Pressed, { .m_fill = SecondaryPressedFill } },
            },
            .m_properties = { { "flat", true } },             // ProjectManager.qss:34 (QPushButton[secondary="true"])
        };

        //! A danger button of a footer (the uninstall dialog): the red gradient on the same box. UNVERIFIED: no capture shows one.
        const BoxElement FooterButtonDanger{
            .m_name = "ProjectManager.Box.FooterButtonDanger",
            .m_margin = QMargins(0, 0, 30, 0),                // ProjectManager.qss:471 (#footer > QPushButton)
            .m_nativeBorder = false,                          // border-radius: ProjectManager.qss:467 (#footer > QPushButton)
            .m_radius = 3,                                    // ProjectManager.qss:467 (#footer > QPushButton)
            .m_minSize = QSize(150, 28),                      // ProjectManager.qss:468,470 (#footer > QPushButton)
            // no maximum height: see the note above FooterButtonPrimary (ProjectManager.qss:469)
            .m_idle = { .m_fill = DangerFill },               // ProjectManager.qss:49 (QPushButton[danger="true"])
            .m_stateFaces = {
                // ProjectManager.qss:53 (QPushButton[danger="true"]:hover)
                { BoxElement::Hover, { .m_fill = DangerHoverFill } },
                // ProjectManager.qss:57 (QPushButton[danger="true"]:pressed)
                { BoxElement::Pressed, { .m_fill = DangerPressedFill } },
            },
            .m_properties = { { "flat", true } },             // ProjectManager.qss:48 (QPushButton[danger="true"])
        };

        //! The back arrow of a screen header (#header QPushButton): its arrow, in a 24x24 box.
        const BoxElement HeaderBackButton{
            .m_name = "ProjectManager.Box.HeaderBackButton",
            .m_margin = QMargins(10, 20, 10, 0),              // ProjectManager.qss:414 (#header QPushButton)
            .m_minSize = QSize(24, 24),                       // ProjectManager.qss:410,412 (#header QPushButton)
            .m_maxSize = QSize(24, 24),                       // ProjectManager.qss:411,413 (#header QPushButton)
            // Hover and Pressed are one face and both stay entries: a key press holds Pressed with no pointer over the button.
            // ProjectManager.qss:415 (#header QPushButton)
            .m_idle = { .m_fill = QBrush(Transparent), .m_image = ":/ArrowBack.svg" },
            // UNVERIFIED states: hover, pressed (no capture shows the widget in them)
            .m_stateFaces = {
                // ProjectManager.qss:426 (#header QPushButton:hover)
                { BoxElement::Hover, { .m_fill = QBrush(Transparent), .m_image = ":/ArrowBack_Hover.svg" } },
                // ProjectManager.qss:426 (#header QPushButton:pressed)
                { BoxElement::Pressed, { .m_fill = QBrush(Transparent), .m_image = ":/ArrowBack_Hover.svg" } },
            },
            .m_imageAlign = Qt::AlignCenter,                  // ProjectManager.qss:415,416 (#header QPushButton)
            .m_imageOrigin = BoxElement::Origin::Content,     // ProjectManager.qss:415,416 (#header QPushButton)
            // ProjectManager.qss:417,418 (#header QPushButton)
            .m_properties = { { "flat", true }, { "iconSize", QSize(50, 50) } },
        };

        //! The small title of a screen header (#headerTitle): 20px tall.
        const BoxElement HeaderTitle{
            .m_name = "ProjectManager.Box.HeaderTitle",
            // a margin or padding of 0: ProjectManager.qss:432 (#headerTitle); ProjectManager.qss:433,434 (#headerTitle)
            .m_hasBox = true,
            .m_minSize = QSize(-1, 20),                       // ProjectManager.qss:435 (#headerTitle)
            .m_maxSize = QSize(-1, 20),                       // ProjectManager.qss:436 (#headerTitle)
            .m_idle = { .m_fill = QBrush(Transparent) },      // BaseStyleSheet.qss:17 (*)
        };

        //! The big title of a screen header (#headerSubTitle): 42px tall, 5px up.
        const BoxElement HeaderSubtitle{
            .m_name = "ProjectManager.Box.HeaderSubtitle",
            .m_padding = QMargins(0, -5, 0, 0),               // ProjectManager.qss:442 (#headerSubTitle)
            .m_minSize = QSize(-1, 42),                       // ProjectManager.qss:443 (#headerSubTitle)
            .m_maxSize = QSize(-1, 42),                       // ProjectManager.qss:444 (#headerSubTitle)
            .m_idle = { .m_fill = QBrush(Transparent) },      // BaseStyleSheet.qss:17 (*)
        };

        //! The header of a wizard or settings screen (#header): an 80px dark band.
        const BoxElement Header{
            .m_name = "ProjectManager.Box.Header",
            .m_minSize = QSize(-1, 80),                       // ProjectManager.qss:404 (#header)
            .m_maxSize = QSize(-1, 80),                       // ProjectManager.qss:405 (#header)
            .m_idle = { .m_fill = QBrush(HeaderFill) },       // ProjectManager.qss:403 (#header)
        };

        //! The footer of a screen or dialog (#footer): 54px tall.
        const BoxElement Footer{
            .m_name = "ProjectManager.Box.Footer",
            .m_minSize = QSize(50, 54),                       // ProjectManager.qss:457,458 (#footer)
            .m_maxSize = QSize(-1, 54),                       // ProjectManager.qss:459 (#footer)
            .m_idle = { .m_fill = QBrush(Transparent) },      // BaseStyleSheet.qss:17 (*)
        };

        //! The grey body of a screen (#body, #GemCatalogScreen, #createAGemRHS).
        const BoxElement Body{
            .m_name = "ProjectManager.Box.Body",
            // ProjectManager.qss:102 (QStackedWidget#createAGemRHS), :448 (#body), :803 (#GemCatalogScreen)
            .m_idle = { .m_fill = QBrush(PageFill) },
        };

        //! The dark blue bar over a tab page (#engineTopFrame, #projectSettingsTopFrame, #GemCatalogHeaderWidget).
        const BoxElement TopBar{
            .m_name = "ProjectManager.Box.TopBar",
            // ProjectManager.qss:607 (#projectSettingsTopFrame), :981 (#GemCatalogHeaderWidget), :1037 (#engineTopFrame)
            .m_idle = { .m_fill = QBrush(TopBarFill) },
        };

        // ---- The start page

        //! The 'Create a project' card of the start page: 210x276, a white outline that turns blue, and its plus.
        const BoxElement StartCardCreate{
            .m_name = "ProjectManager.Box.StartCardCreate",
            .m_nativeBorder = false,                          // border: ProjectManager.qss:631 (#firstTimeContent > QPushButton)
            .m_minSize = QSize(210, 276),                     // ProjectManager.qss:624,626 (#firstTimeContent > QPushButton)
            .m_maxSize = QSize(210, 276),                     // ProjectManager.qss:625,627 (#firstTimeContent > QPushButton)
            // ProjectManager.qss:649 (#createProjectButton); ProjectManager.qss:631 (#firstTimeContent > QPushButton)
            .m_idle = { .m_fill = QBrush(CardFill), .m_edges = BoxElement::border(1, White), .m_image = ":/AddOffset.svg" },
            // UNVERIFIED states: hover, pressed (no capture shows the widget in them)
            .m_stateFaces = {
                // ProjectManager.qss:649 (#createProjectButton); ProjectManager.qss:640 (#firstTimeContent > QPushButton:focus)
                { BoxElement::Focus, { .m_fill = QBrush(CardFill), .m_edges = BoxElement::border(1, AccentBlue),
                                       .m_image = ":/AddOffset.svg" } },
                // ProjectManager.qss:653 (#createProjectButton:hover); ProjectManager.qss:635 (#firstTimeContent > QPushButton:hover);
                // ProjectManager.qss:636 (#firstTimeContent > QPushButton:hover)
                { BoxElement::Hover, { .m_fill = QBrush(CardFill), .m_edges = BoxElement::border(1, AccentBlue), .m_text = AccentBlue,
                                       .m_image = ":/AddOffset_Hover.svg" } },
                // ProjectManager.qss:653 (#createProjectButton:pressed); ProjectManager.qss:644 (#firstTimeContent > QPushButton:pressed);
                // ProjectManager.qss:645 (#firstTimeContent > QPushButton:pressed)
                { BoxElement::Pressed, { .m_fill = QBrush(CardFill), .m_edges = BoxElement::border(1, AccentBluePressed),
                                         .m_text = AccentBluePressed, .m_image = ":/AddOffset_Hover.svg" } },
            },
            // ProjectManager.qss:629 (#firstTimeContent > QPushButton), :649 (#createProjectButton)
            .m_imageAlign = Qt::AlignCenter,
            // ProjectManager.qss:629 (#firstTimeContent > QPushButton), :649 (#createProjectButton)
            .m_imageOrigin = BoxElement::Origin::Content,
            .m_properties = { { "flat", true } },             // ProjectManager.qss:628 (#firstTimeContent > QPushButton)
        };

        //! The 'Open a project' card.
        const BoxElement StartCardOpen{
            .m_name = "ProjectManager.Box.StartCardOpen",
            .m_nativeBorder = false,                          // border: ProjectManager.qss:631 (#firstTimeContent > QPushButton)
            .m_minSize = QSize(210, 276),                     // ProjectManager.qss:624,626 (#firstTimeContent > QPushButton)
            .m_maxSize = QSize(210, 276),                     // ProjectManager.qss:625,627 (#firstTimeContent > QPushButton)
            // ProjectManager.qss:657 (#addProjectButton); ProjectManager.qss:631 (#firstTimeContent > QPushButton)
            .m_idle = { .m_fill = QBrush(CardFill), .m_edges = BoxElement::border(1, White), .m_image = ":/FolderOffset.svg" },
            // UNVERIFIED states: hover, pressed, focus (no capture shows the widget in them)
            .m_stateFaces = {
                // ProjectManager.qss:657 (#addProjectButton); ProjectManager.qss:640 (#firstTimeContent > QPushButton:focus)
                { BoxElement::Focus, { .m_fill = QBrush(CardFill), .m_edges = BoxElement::border(1, AccentBlue),
                                       .m_image = ":/FolderOffset.svg" } },
                // ProjectManager.qss:661 (#addProjectButton:hover); ProjectManager.qss:635 (#firstTimeContent > QPushButton:hover);
                // ProjectManager.qss:636 (#firstTimeContent > QPushButton:hover)
                { BoxElement::Hover, { .m_fill = QBrush(CardFill), .m_edges = BoxElement::border(1, AccentBlue), .m_text = AccentBlue,
                                       .m_image = ":/FolderOffset_Hover.svg" } },
                // ProjectManager.qss:661 (#addProjectButton:pressed); ProjectManager.qss:644 (#firstTimeContent > QPushButton:pressed);
                // ProjectManager.qss:645 (#firstTimeContent > QPushButton:pressed)
                { BoxElement::Pressed, { .m_fill = QBrush(CardFill), .m_edges = BoxElement::border(1, AccentBluePressed),
                                         .m_text = AccentBluePressed, .m_image = ":/FolderOffset_Hover.svg" } },
            },
            // ProjectManager.qss:629 (#firstTimeContent > QPushButton), :657 (#addProjectButton)
            .m_imageAlign = Qt::AlignCenter,
            // ProjectManager.qss:629 (#firstTimeContent > QPushButton), :657 (#addProjectButton)
            .m_imageOrigin = BoxElement::Origin::Content,
            .m_properties = { { "flat", true } },             // ProjectManager.qss:628 (#firstTimeContent > QPushButton)
        };

        //! The 'Add a remote project' card.
        const BoxElement StartCardRemote{
            .m_name = "ProjectManager.Box.StartCardRemote",
            .m_nativeBorder = false,                          // border: ProjectManager.qss:631 (#firstTimeContent > QPushButton)
            .m_minSize = QSize(210, 276),                     // ProjectManager.qss:624,626 (#firstTimeContent > QPushButton)
            .m_maxSize = QSize(210, 276),                     // ProjectManager.qss:625,627 (#firstTimeContent > QPushButton)
            // ProjectManager.qss:665 (#addRemoteProjectButton); ProjectManager.qss:631 (#firstTimeContent > QPushButton)
            .m_idle = { .m_fill = QBrush(CardFill), .m_edges = BoxElement::border(1, White), .m_image = ":/Cloud.svg" },
            // UNVERIFIED states: hover, pressed, focus (no capture shows the widget in them)
            .m_stateFaces = {
                // ProjectManager.qss:665 (#addRemoteProjectButton); ProjectManager.qss:640 (#firstTimeContent > QPushButton:focus)
                { BoxElement::Focus, { .m_fill = QBrush(CardFill), .m_edges = BoxElement::border(1, AccentBlue),
                                       .m_image = ":/Cloud.svg" } },
                // ProjectManager.qss:669 (#addRemoteProjectButton:hover); ProjectManager.qss:635 (#firstTimeContent > QPushButton:hover);
                // ProjectManager.qss:636 (#firstTimeContent > QPushButton:hover)
                { BoxElement::Hover, { .m_fill = QBrush(CardFill), .m_edges = BoxElement::border(1, AccentBlue), .m_text = AccentBlue,
                                       .m_image = ":/Cloud_Hover.svg" } },
                // ProjectManager.qss:669 (#addRemoteProjectButton:pressed); ProjectManager.qss:644 (#firstTimeContent >
                // QPushButton:pressed); ProjectManager.qss:645 (#firstTimeContent > QPushButton:pressed)
                { BoxElement::Pressed, { .m_fill = QBrush(CardFill), .m_edges = BoxElement::border(1, AccentBluePressed),
                                         .m_text = AccentBluePressed, .m_image = ":/Cloud_Hover.svg" } },
            },
            // ProjectManager.qss:629 (#firstTimeContent > QPushButton), :665 (#addRemoteProjectButton)
            .m_imageAlign = Qt::AlignCenter,
            // ProjectManager.qss:629 (#firstTimeContent > QPushButton), :665 (#addRemoteProjectButton)
            .m_imageOrigin = BoxElement::Origin::Content,
            .m_properties = { { "flat", true } },             // ProjectManager.qss:628 (#firstTimeContent > QPushButton)
        };

        //! The big title of the start page: 73px above it, no indent.
        const BoxElement StartTitle{
            .m_name = "ProjectManager.Box.StartTitle",
            .m_margin = QMargins(0, 73, 0, 0),                // ProjectManager.qss:613 (#firstTimeContent > #titleLabel)
            .m_idle = { .m_fill = QBrush(Transparent) },      // BaseStyleSheet.qss:17 (*)
            .m_properties = { { "indent", 0 } },              // ProjectManager.qss:614 (#firstTimeContent > #titleLabel)
        };

        //! The line under it: 10px above, 60px below.
        const BoxElement StartIntro{
            .m_name = "ProjectManager.Box.StartIntro",
            .m_margin = QMargins(0, 10, 0, 60),               // ProjectManager.qss:619 (#firstTimeContent > #introLabel)
            .m_idle = { .m_fill = QBrush(Transparent) },      // BaseStyleSheet.qss:17 (*)
            .m_properties = { { "indent", 0 } },              // ProjectManager.qss:620 (#firstTimeContent > #introLabel)
        };

        // ---- The projects page and its cards

        //! The red button that cancels a project card's action (#labelButton #projectActionCancelButton). UNVERIFIED: no capture shows one.
        const BoxElement ProjectCardCancel{
            .m_name = "ProjectManager.Box.ProjectCardCancel",
            .m_margin = QMargins(1, 0, 0, 0),                 // ProjectManager.qss:734 (#labelButton #projectActionCancelButton)
            // border-radius: ProjectManager.qss:733 (#labelButton #projectActionCancelButton)
            .m_nativeBorder = false,
            .m_radius = 3,                                    // ProjectManager.qss:733 (#labelButton #projectActionCancelButton)
            .m_minSize = QSize(140, 24),                      // ProjectManager.qss:726,728 (#labelButton QPushButton)
            .m_maxSize = QSize(140, 24),                      // ProjectManager.qss:727,729 (#labelButton QPushButton)
            .m_idle = { .m_fill = DangerFill },               // ProjectManager.qss:49 (QPushButton[danger="true"])
            .m_stateFaces = {
                // ProjectManager.qss:53 (QPushButton[danger="true"]:hover)
                { BoxElement::Hover, { .m_fill = DangerHoverFill } },
                // ProjectManager.qss:57 (QPushButton[danger="true"]:pressed)
                { BoxElement::Pressed, { .m_fill = DangerPressedFill } },
            },
            .m_properties = { { "flat", true } },             // ProjectManager.qss:48 (QPushButton[danger="true"])
        };

        //! The 'New Project' button of the projects page (#projectsContent > QFrame > #newProjectButton): 150x26, not flat.
        const BoxElement NewProjectButton{
            .m_name = "ProjectManager.Box.NewProjectButton",
            .m_minSize = QSize(150, 26),                      // ProjectManager.qss:766,768 (#projectsContent > QFrame > #newProjectButton)
            .m_maxSize = QSize(150, 26),                      // ProjectManager.qss:767,769 (#projectsContent > QFrame > #newProjectButton)
            .m_idle = { .m_fill = QBrush(Transparent) },      // BaseStyleSheet.qss:17 (*)
        };

        //! A title label with no indent: the projects page's, and the first header of the create-a-gem form.
        const BoxElement ProjectsTitle{
            .m_name = "ProjectManager.Box.ProjectsTitle",
            .m_margin = QMargins(0, 1, 0, 1),                 // Text.qss:93,94 (QLabel)
            .m_idle = { .m_fill = QBrush(Transparent) },      // BaseStyleSheet.qss:17 (*)
            // ProjectManager.qss:366 (#rightPaneHeader), :678 (#projectsContent > QFrame > #titleLabel)
            .m_properties = { { "indent", 0 } },
        };

        //! The scroll area of the project cards (#projectsContent > QScrollArea): 40px above, 5px below.
        const BoxElement ProjectsGrid{
            .m_name = "ProjectManager.Box.ProjectsGrid",
            // QDockWidget.qss:35 (QAbstractScrollArea); ProjectManager.qss:682,683 (#projectsContent > QScrollArea)
            .m_margin = QMargins(0, 40, 0, 5),
            .m_idle = { .m_fill = QBrush(Transparent) },      // BaseStyleSheet.qss:17 (*)
        };

        //! The frame with the title and the new project button (#projectsContent > QFrame): 60px above.
        const BoxElement ProjectsHeader{
            .m_name = "ProjectManager.Box.ProjectsHeader",
            .m_margin = QMargins(0, 60, 0, 0),                // ProjectManager.qss:673 (#projectsContent > QFrame)
            .m_idle = { .m_fill = QBrush(Transparent) },      // BaseStyleSheet.qss:17 (*)
        };

        //! The darkening veil over a project card's image (#labelButtonOverlay). UNVERIFIED: no capture shows one.
        const BoxElement ProjectCardOverlay{
            .m_name = "ProjectManager.Box.ProjectCardOverlay",
            .m_hasBox = true,                                 // a margin or padding of 0: ProjectManager.qss:711 (#projectButton QLabel)
            .m_idle = { .m_fill = QBrush(ScrimFill) },        // ProjectManager.qss:738 (#labelButtonOverlay)
            .m_properties = { { "indent", 0 } },              // ProjectManager.qss:710 (#projectButton QLabel)
        };

        //! The message over a project card (#projectMessageOverlay): 160x32. UNVERIFIED.
        const BoxElement ProjectCardMessage{
            .m_name = "ProjectManager.Box.ProjectCardMessage",
            .m_hasBox = true,                                 // a margin or padding of 0: ProjectManager.qss:711 (#projectButton QLabel)
            .m_minSize = QSize(160, 32),                      // ProjectManager.qss:742,744 (#projectMessageOverlay)
            .m_maxSize = QSize(160, 32),                      // ProjectManager.qss:743,745 (#projectMessageOverlay)
            .m_idle = { .m_fill = QBrush(Transparent) },      // BaseStyleSheet.qss:17 (*)
            .m_properties = { { "indent", 0 } },              // ProjectManager.qss:710 (#projectButton QLabel)
        };

        //! The second message over a project card (#projectSubMessageOverlay): 160x32. UNVERIFIED.
        const BoxElement ProjectCardSubMessage{
            .m_name = "ProjectManager.Box.ProjectCardSubMessage",
            .m_hasBox = true,                                 // a margin or padding of 0: ProjectManager.qss:711 (#projectButton QLabel)
            .m_minSize = QSize(160, 32),                      // ProjectManager.qss:742,744 (#projectSubMessageOverlay)
            .m_maxSize = QSize(160, 32),                      // ProjectManager.qss:743,745 (#projectSubMessageOverlay)
            .m_idle = { .m_fill = QBrush(Transparent) },      // BaseStyleSheet.qss:17 (*)
            .m_properties = { { "indent", 0 } },              // ProjectManager.qss:710 (#projectButton QLabel)
        };

        //! The face of a project card (#projectButton > #labelButton): a white outline, blue while hovered, pressed or focused.
        const BoxElement ProjectCardOutline{
            .m_name = "ProjectManager.Box.ProjectCardOutline",
            .m_hasBox = true,                                 // a margin or padding of 0: ProjectManager.qss:711 (#projectButton QLabel)
            .m_nativeBorder = false,                          // border: ProjectManager.qss:687 (#projectButton > #labelButton)
            // Hover, Pressed and Focus are one face and all stay entries: each can hold alone (a key press, a focus by Tab).
            // BaseStyleSheet.qss:17 (*); ProjectManager.qss:687 (#projectButton > #labelButton)
            .m_idle = { .m_fill = QBrush(Transparent), .m_edges = BoxElement::border(1, White) },
            // UNVERIFIED states: hover, pressed, focus (no capture shows the widget in them)
            .m_stateFaces = {
                // BaseStyleSheet.qss:17 (*); ProjectManager.qss:693 (#projectButton > #labelButton:hover)
                { BoxElement::Hover, { .m_fill = QBrush(Transparent), .m_edges = BoxElement::border(1, AccentBlue) } },
                // BaseStyleSheet.qss:17 (*); ProjectManager.qss:693 (#projectButton > #labelButton:pressed)
                { BoxElement::Pressed, { .m_fill = QBrush(Transparent), .m_edges = BoxElement::border(1, AccentBlue) } },
                // BaseStyleSheet.qss:17 (*); ProjectManager.qss:693 (#projectButton > #labelButton:focus)
                { BoxElement::Focus, { .m_fill = QBrush(Transparent), .m_edges = BoxElement::border(1, AccentBlue) } },
            },
            .m_properties = { { "indent", 0 } },              // ProjectManager.qss:710 (#projectButton QLabel)
        };

        //! The buttons over a project card (#labelButton QPushButton): 140x24.
        const BoxElement ProjectCardAction{
            .m_name = "ProjectManager.Box.ProjectCardAction",
            .m_minSize = QSize(140, 24),                      // ProjectManager.qss:726,728 (#labelButton QPushButton)
            .m_maxSize = QSize(140, 24),                      // ProjectManager.qss:727,729 (#labelButton QPushButton)
            .m_idle = { .m_fill = QBrush(Transparent) },      // BaseStyleSheet.qss:17 (*)
        };

        //! Every other label of a project card (#projectButton QLabel): no margin, no indent.
        const BoxElement ProjectCardLabel{
            .m_name = "ProjectManager.Box.ProjectCardLabel",
            .m_hasBox = true,                                 // a margin or padding of 0: ProjectManager.qss:711 (#projectButton QLabel)
            .m_idle = { .m_fill = QBrush(Transparent) },      // BaseStyleSheet.qss:17 (*)
            .m_properties = { { "indent", 0 } },              // ProjectManager.qss:710 (#projectButton QLabel)
        };

        //! The frame under a project card's face (#projectButton > QFrame): 8px above.
        const BoxElement ProjectCardFooter{
            .m_name = "ProjectManager.Box.ProjectCardFooter",
            .m_margin = QMargins(0, 8, 0, 0),                 // ProjectManager.qss:704 (#projectButton > QFrame)
            .m_idle = { .m_fill = QBrush(Transparent) },      // BaseStyleSheet.qss:17 (*)
        };

        //! A project card (#projectButton): 210x324.
        const BoxElement ProjectCard{
            .m_name = "ProjectManager.Box.ProjectCard",
            .m_minSize = QSize(210, 324),                     // ProjectManager.qss:697,699 (#projectButton)
            .m_maxSize = QSize(210, 324),                     // ProjectManager.qss:698,700 (#projectButton)
            .m_idle = { .m_fill = QBrush(Transparent) },      // BaseStyleSheet.qss:17 (*)
        };

        //! The menu button of a project card (#projectMenuButton): 30x14, its three dots.
        const BoxElement ProjectMenuButton{
            .m_name = "ProjectManager.Box.ProjectMenuButton",
            .m_minSize = QSize(30, 14),                       // ProjectManager.qss:760,762 (#projectMenuButton)
            .m_maxSize = QSize(30, 14),                       // ProjectManager.qss:759,761 (#projectMenuButton)
            // ProjectManager.qss:758 (#projectMenuButton)
            .m_idle = { .m_fill = QBrush(Transparent), .m_image = ":/menu.svg" },
            .m_imageAlign = Qt::AlignCenter,                  // ProjectManager.qss:758 (#projectMenuButton)
            .m_properties = { { "flat", true } },             // ProjectManager.qss:757 (#projectMenuButton)
        };

        // ---- Form fields

        //! The drop down button of a form field (#formFrame QPushButton#dropDownButton): 35px wide, its arrow.
        const BoxElement FormDropDownButton{
            .m_name = "ProjectManager.Box.FormDropDownButton",
            .m_margin = QMargins(-10, 0, 0, 0),               // ProjectManager.qss:287 (#formFrame QPushButton#dropDownButton)
            .m_padding = QMargins(0, 0, 0, 2),                // ProjectManager.qss:286 (#formFrame QPushButton#dropDownButton)
            .m_size = QSize(35, -1),                          // ProjectManager.qss:288 (#formFrame QPushButton#dropDownButton)
            // ProjectManager.qss:283 (#formFrame QPushButton#dropDownButton)
            .m_idle = { .m_image = ":/CarrotArrowDown.svg" },
            .m_imageAlign = Qt::AlignCenter,                  // ProjectManager.qss:283 (#formFrame QPushButton#dropDownButton)
            // ProjectManager.qss:284,285 (#formFrame QPushButton#dropDownButton)
            .m_properties = { { "flat", true }, { "iconSize", QSize(40, 40) } },
        };

        //! The browse button of a form field (#formFrame QPushButton): its folder icon.
        const BoxElement FormBrowseButton{
            .m_name = "ProjectManager.Box.FormBrowseButton",
            // ProjectManager.qss:277 (#formFrame QPushButton)
            .m_idle = { .m_fill = QBrush(Transparent), .m_image = ":/Select_Folder.svg" },
            .m_stateFaces = {
                // ProjectManager.qss:296 (#formFrame QPushButton:disabled)
                { BoxElement::Disabled, { .m_fill = QBrush(Transparent), .m_image = ":/Select_Folder_Disabled.svg" } },
            },
            .m_imageAlign = Qt::AlignCenter,                  // ProjectManager.qss:277 (#formFrame QPushButton)
            .m_properties = { { "flat", true } },             // ProjectManager.qss:278 (#formFrame QPushButton)
        };

        //! The entry of a form field (#formFrame QLineEdit): no border, clear, 24px of contents.
        //! `height: 1em` (ProjectManager.qss:305) is the height of the entry's font: Qt reads an em as QFontMetrics::height() of the font of the
        //! rule (lengthValueFromData, qcssparser.cpp:462-467), which is the 18px Open Sans of :301 (FormLineEditText), 24px high. n2 holds the
        //! entry's hint: 157x21 in every row of every form, the 24px of the height less the 4px of padding above and plus the 1px below (:306
        //! and LineEdit.qss:18); the entry without a height of its own hints 23 (f5). Each row of a form is 78 high in n2 (formFrame 68, entry 21)
        //! and was 80 without it: the entry, its frame, its row and the page below were 2px too tall (the rows of the Engine and the project
        //! settings pages, the create and edit gem forms; the template list of the new project page lost the 6px of its three rows). The 24
        //! is the number n2 shows for this font: a font of another height gives another em.
        //! TOKENIZE: FormLineEditHeight
        const BoxElement FormLineEdit{
            .m_name = "ProjectManager.Box.FormLineEdit",
            // LineEdit.qss:15,18 (QLineEdit); ProjectManager.qss:306 (#formFrame QLineEdit)
            .m_padding = QMargins(0, -4, 0, 1),
            .m_nativeBorder = false,                          // border: ProjectManager.qss:303 (#formFrame QLineEdit)
            .m_radius = 2,                                    // BaseStyleSheet.qss:79 (QLineEdit)
            .m_size = QSize(-1, 24),                          // height: 1em, ProjectManager.qss:305 (#formFrame QLineEdit): see above
            .m_idle = { .m_fill = QBrush(Transparent) },      // ProjectManager.qss:300 (#formFrame QLineEdit)
        };

        //! The combo box of a form field (#formFrame QComboBox, ProjectManager.qss:149-159 and its twin :313-323, with the drop-down of :161-168
        //! and :325-332), over ComboBox.qss:13-57 and BaseStyleSheet.qss:68-84: 810px wide (min and max, :152-153), 26px high (:156) with the
        //! 1px margin of every combo box (ComboBox.qss:17), a drop-down 24px wide with 20px of padding to its right, the arrow in its left 24.
        //! n2 (the project settings page): the combo box is at [68,485,812,28], hints 858x28 and has the limits 812 wide, its row
        //! (FormComboBoxWidget) is 890x85, and the ink of its arrow lies at x 840.7 to 850.0, y 497.3 to 501.3; f5 had it 802x18 at [68,491]
        //! (hint 85x18) and the row 75 high. ComboBox.qss:22-42 changes the box with the state: :hover and :focus (and the open popup,
        //! [HasPopupOpen=true]) take the margin away, give it a border of 1px and round it by 3px instead of 2, so each carries its own margin
        //! and radius (Face::m_margin, m_radius). The idle box and its sizes are Native's; the faces of the other states are the cascade's and
        //! are proven against the legacy rules by the twin tests of AzQtComponents.Tests (BoxElementFaceTests.ComboBox_*), UNVERIFIED by
        //! capture: no capture holds this combo box hovered, focused, open or disabled. An open popup is the state the combo box reports as
        //! Checked (QComboBox::initStyleOption, qcombobox.cpp:1499-1500): the declarations of :focus.
        //! OPEN: the rows of its popup (`#formFrame QComboBox QListView::item { height: 40px; font-size: 18px }`, :170-174 and :334-338), which
        //! ComboPopupElement owns. TOKENIZE: FormComboBoxFill, FormComboBoxHoverEdge, FormComboBoxFocusEdge, FormComboBoxFocusFill,
        //! FormComboBoxDisabledFill, FormComboBoxDisabledText
        const BoxElement FormComboBox{
            .m_name = "ProjectManager.Box.FormComboBox",
            .m_margin = QMargins(1, 1, 1, 1),                 // ComboBox.qss:17 (QComboBox)
            .m_padding = QMargins(0, 0, 0, 0),                // ProjectManager.qss:157,158 (#formFrame QComboBox padding-left, padding-right)
            .m_hasBox = true,                                 // a margin and a padding are declared (ComboBox.qss:17, ProjectManager.qss:157)
            .m_nativeBorder = false,                          // border-width: BaseStyleSheet.qss:80 (QComboBox), border-radius :79
            .m_radius = 2,                                    // BaseStyleSheet.qss:79 (QComboBox)
            .m_size = QSize(-1, 26),                          // height: ProjectManager.qss:156 (#formFrame QComboBox)
            .m_minSize = QSize(810, -1),                      // min-width: ProjectManager.qss:152 (#formFrame QComboBox)
            .m_maxSize = QSize(810, -1),                      // max-width: ProjectManager.qss:153 (#formFrame QComboBox)
            .m_dropDownWidth = 24,                            // width: ProjectManager.qss:167 (#formFrame QComboBox::drop-down)
            .m_dropDownPadding = QMargins(0, 0, 20, 0),       // padding-right: ProjectManager.qss:164 (#formFrame QComboBox::drop-down)
            // ComboBox.qss:19 (QComboBox background-color), BaseStyleSheet.qss:77 (QComboBox color: black)
            .m_idle = { .m_fill = QBrush(QColor(0xCC, 0xCC, 0xCC)), .m_text = QColor(0x00, 0x00, 0x00) },
            .m_stateFaces = {
                // ComboBox.qss:22-29 (QComboBox:hover): no margin, a 1px border of rgba(0, 0, 0, 50), round by 3px
                { BoxElement::Hover, {
                    .m_fill = QBrush(QColor(0xCC, 0xCC, 0xCC)), .m_edges = BoxElement::border(1, QColor(0, 0, 0, 50)),
                    .m_text = QColor(0x00, 0x00, 0x00), .m_margin = QMargins(0, 0, 0, 0), .m_radius = 3 } },
                // ComboBox.qss:31-42 (QComboBox:focus): no margin, a 1px border of #94D2FF, round by 3px, white
                { BoxElement::Focus, {
                    .m_fill = QBrush(QColor(0xFF, 0xFF, 0xFF)), .m_edges = BoxElement::border(1, QColor(0x94, 0xD2, 0xFF)),
                    .m_text = QColor(0x00, 0x00, 0x00), .m_margin = QMargins(0, 0, 0, 0), .m_radius = 3 } },
                // ComboBox.qss:32-33 (QComboBox[HasPopupOpen=true]): the declarations of :focus. UNVERIFIED (no capture holds an open popup)
                { BoxElement::Checked, {
                    .m_fill = QBrush(QColor(0xFF, 0xFF, 0xFF)), .m_edges = BoxElement::border(1, QColor(0x94, 0xD2, 0xFF)),
                    .m_text = QColor(0x00, 0x00, 0x00), .m_margin = QMargins(0, 0, 0, 0), .m_radius = 3 } },
                // ComboBox.qss:53-57 (QComboBox:disabled): the fill and the colour only. UNVERIFIED
                { BoxElement::Disabled, { .m_fill = QBrush(QColor(0x66, 0x66, 0x66)), .m_text = QColor(0x99, 0x99, 0x99) } },
                // :disabled is declared after :focus (ComboBox.qss:53 after :31): a disabled combo box that has the focus has the fill and the
                // colour of the first and the box of the second. UNVERIFIED
                { BoxElement::Disabled | BoxElement::Focus, {
                    .m_fill = QBrush(QColor(0x66, 0x66, 0x66)), .m_edges = BoxElement::border(1, QColor(0x94, 0xD2, 0xFF)),
                    .m_text = QColor(0x99, 0x99, 0x99), .m_margin = QMargins(0, 0, 0, 0), .m_radius = 3 } },
            },
        };

        //! The nested form frame of the create-a-gem wizard when its value is invalid (#formFrame[Valid="false"]). UNVERIFIED.
        const BoxElement FormFrameNestedInvalid{
            .m_name = "ProjectManager.Box.FormFrameNestedInvalid",
            // ProjectManager.qss:214 (#formFrame), :397 (#createAGemRadioButtonSubFormField #formFrame)
            .m_margin = QMargins(23, 10, 0, 0),
            .m_padding = QMargins(6, 0, 10, 2),               // ProjectManager.qss:213 (#formFrame)
            .m_nativeBorder = false,                          // border: ProjectManager.qss:258 (#formFrame[Valid="false"])
            .m_radius = 4,                                    // ProjectManager.qss:212 (#formFrame)
            .m_maxSize = QSize(840, -1),                      // ProjectManager.qss:209 (#formFrame)
            // ProjectManager.qss:210 (#formFrame); ProjectManager.qss:258 (#formFrame[Valid="false"])
            .m_idle = { .m_fill = QBrush(PanelFill), .m_edges = BoxElement::border(1, FormBorderInvalid) },
            .m_stateFaces = {
                // ProjectManager.qss:210 (#formFrame); ProjectManager.qss:262 (#formFrame:disabled)
                { BoxElement::Disabled, { .m_fill = QBrush(PanelFill), .m_edges = BoxElement::border(1, FormBorderDisabled) } },
            },
        };

        //! The same while its entry has the focus (#formFrame[Focus="true"]). UNVERIFIED.
        const BoxElement FormFrameNestedFocused{
            .m_name = "ProjectManager.Box.FormFrameNestedFocused",
            // ProjectManager.qss:214 (#formFrame), :397 (#createAGemRadioButtonSubFormField #formFrame)
            .m_margin = QMargins(23, 10, 0, 0),
            .m_padding = QMargins(6, 0, 10, 2),               // ProjectManager.qss:213 (#formFrame)
            .m_nativeBorder = false,                          // border: ProjectManager.qss:254 (#formFrame[Focus="true"])
            .m_radius = 4,                                    // ProjectManager.qss:212 (#formFrame)
            .m_maxSize = QSize(840, -1),                      // ProjectManager.qss:209 (#formFrame)
            // ProjectManager.qss:210 (#formFrame); ProjectManager.qss:254 (#formFrame[Focus="true"])
            .m_idle = { .m_fill = QBrush(PanelFill), .m_edges = BoxElement::border(1, AccentBlue) },
            .m_stateFaces = {
                // ProjectManager.qss:210 (#formFrame); ProjectManager.qss:262 (#formFrame:disabled)
                { BoxElement::Disabled, { .m_fill = QBrush(PanelFill), .m_edges = BoxElement::border(1, FormBorderDisabled) } },
            },
        };

        //! A form frame nested in the wizard's template option (#createAGemRadioButtonSubFormField #formFrame): 23px in.
        const BoxElement FormFrameNested{
            .m_name = "ProjectManager.Box.FormFrameNested",
            // ProjectManager.qss:214 (#formFrame), :397 (#createAGemRadioButtonSubFormField #formFrame)
            .m_margin = QMargins(23, 10, 0, 0),
            .m_padding = QMargins(6, 0, 10, 2),               // ProjectManager.qss:213 (#formFrame)
            .m_nativeBorder = false,                          // border: ProjectManager.qss:211 (#formFrame)
            .m_radius = 4,                                    // ProjectManager.qss:212 (#formFrame)
            .m_maxSize = QSize(840, -1),                      // ProjectManager.qss:209 (#formFrame)
            // ProjectManager.qss:210 (#formFrame); ProjectManager.qss:211 (#formFrame)
            .m_idle = { .m_fill = QBrush(PanelFill), .m_edges = BoxElement::border(1, FormBorder) },
            .m_stateFaces = {
                // ProjectManager.qss:210 (#formFrame); ProjectManager.qss:262 (#formFrame:disabled)
                { BoxElement::Disabled, { .m_fill = QBrush(PanelFill), .m_edges = BoxElement::border(1, FormBorderDisabled) } },
            },
        };

        //! A form frame of the wizard or of a dialog when its value is invalid. UNVERIFIED.
        const BoxElement FormFrameWizardInvalid{
            .m_name = "ProjectManager.Box.FormFrameWizardInvalid",
            // ProjectManager.qss:107 (#createAGemRHS #formFrame), :214 (#formFrame), :1136 (#addRemoteProjectDialog #formFrame)
            .m_margin = QMargins(0, 10, 0, 0),
            .m_padding = QMargins(6, 0, 10, 2),               // ProjectManager.qss:213 (#formFrame)
            .m_nativeBorder = false,                          // border: ProjectManager.qss:258 (#formFrame[Valid="false"])
            .m_radius = 4,                                    // ProjectManager.qss:212 (#formFrame)
            .m_maxSize = QSize(840, -1),                      // ProjectManager.qss:209 (#formFrame)
            // ProjectManager.qss:210 (#formFrame); ProjectManager.qss:258 (#formFrame[Valid="false"])
            .m_idle = { .m_fill = QBrush(PanelFill), .m_edges = BoxElement::border(1, FormBorderInvalid) },
            .m_stateFaces = {
                // ProjectManager.qss:210 (#formFrame); ProjectManager.qss:262 (#formFrame:disabled)
                { BoxElement::Disabled, { .m_fill = QBrush(PanelFill), .m_edges = BoxElement::border(1, FormBorderDisabled) } },
            },
        };

        //! The same while its entry has the focus. UNVERIFIED.
        const BoxElement FormFrameWizardFocused{
            .m_name = "ProjectManager.Box.FormFrameWizardFocused",
            // ProjectManager.qss:107 (#createAGemRHS #formFrame), :214 (#formFrame), :1136 (#addRemoteProjectDialog #formFrame)
            .m_margin = QMargins(0, 10, 0, 0),
            .m_padding = QMargins(6, 0, 10, 2),               // ProjectManager.qss:213 (#formFrame)
            .m_nativeBorder = false,                          // border: ProjectManager.qss:254 (#formFrame[Focus="true"])
            .m_radius = 4,                                    // ProjectManager.qss:212 (#formFrame)
            .m_maxSize = QSize(840, -1),                      // ProjectManager.qss:209 (#formFrame)
            // ProjectManager.qss:210 (#formFrame); ProjectManager.qss:254 (#formFrame[Focus="true"])
            .m_idle = { .m_fill = QBrush(PanelFill), .m_edges = BoxElement::border(1, AccentBlue) },
            .m_stateFaces = {
                // ProjectManager.qss:210 (#formFrame); ProjectManager.qss:262 (#formFrame:disabled)
                { BoxElement::Disabled, { .m_fill = QBrush(PanelFill), .m_edges = BoxElement::border(1, FormBorderDisabled) } },
            },
        };

        //! A form frame of the create-a-gem wizard and of the dialogs that put it flush left (#createAGemRHS #formFrame and its twins).
        const BoxElement FormFrameWizard{
            .m_name = "ProjectManager.Box.FormFrameWizard",
            .m_margin = QMargins(0, 10, 0, 0),                // ProjectManager.qss:107 (#createAGemRHS #formFrame), :214 (#formFrame)
            .m_padding = QMargins(6, 0, 10, 2),               // ProjectManager.qss:213 (#formFrame)
            .m_nativeBorder = false,                          // border: ProjectManager.qss:211 (#formFrame)
            .m_radius = 4,                                    // ProjectManager.qss:212 (#formFrame)
            .m_maxSize = QSize(840, -1),                      // ProjectManager.qss:209 (#formFrame)
            // ProjectManager.qss:210 (#formFrame); ProjectManager.qss:211 (#formFrame)
            .m_idle = { .m_fill = QBrush(PanelFill), .m_edges = BoxElement::border(1, FormBorder) },
            .m_stateFaces = {
                // ProjectManager.qss:210 (#formFrame); ProjectManager.qss:262 (#formFrame:disabled)
                { BoxElement::Disabled, { .m_fill = QBrush(PanelFill), .m_edges = BoxElement::border(1, FormBorderDisabled) } },
            },
        };

        //! A form frame whose value is invalid (#formFrame[Valid="false"]): a red border. UNVERIFIED.
        const BoxElement FormFrameInvalid{
            .m_name = "ProjectManager.Box.FormFrameInvalid",
            .m_margin = QMargins(50, 10, 0, 0),               // ProjectManager.qss:214,215 (#formFrame)
            .m_padding = QMargins(6, 0, 10, 2),               // ProjectManager.qss:213 (#formFrame)
            .m_nativeBorder = false,                          // border: ProjectManager.qss:258 (#formFrame[Valid="false"])
            .m_radius = 4,                                    // ProjectManager.qss:212 (#formFrame)
            .m_maxSize = QSize(840, -1),                      // ProjectManager.qss:209 (#formFrame)
            // ProjectManager.qss:210 (#formFrame); ProjectManager.qss:258 (#formFrame[Valid="false"])
            .m_idle = { .m_fill = QBrush(PanelFill), .m_edges = BoxElement::border(1, FormBorderInvalid) },
            .m_stateFaces = {
                // ProjectManager.qss:210 (#formFrame); ProjectManager.qss:262 (#formFrame:disabled)
                { BoxElement::Disabled, { .m_fill = QBrush(PanelFill), .m_edges = BoxElement::border(1, FormBorderDisabled) } },
            },
        };

        //! A form frame while its entry has the focus (#formFrame[Focus="true"]): a blue border. UNVERIFIED.
        const BoxElement FormFrameFocused{
            .m_name = "ProjectManager.Box.FormFrameFocused",
            .m_margin = QMargins(50, 10, 0, 0),               // ProjectManager.qss:214,215 (#formFrame)
            .m_padding = QMargins(6, 0, 10, 2),               // ProjectManager.qss:213 (#formFrame)
            .m_nativeBorder = false,                          // border: ProjectManager.qss:254 (#formFrame[Focus="true"])
            .m_radius = 4,                                    // ProjectManager.qss:212 (#formFrame)
            .m_maxSize = QSize(840, -1),                      // ProjectManager.qss:209 (#formFrame)
            // ProjectManager.qss:210 (#formFrame); ProjectManager.qss:254 (#formFrame[Focus="true"])
            .m_idle = { .m_fill = QBrush(PanelFill), .m_edges = BoxElement::border(1, AccentBlue) },
            .m_stateFaces = {
                // ProjectManager.qss:210 (#formFrame); ProjectManager.qss:262 (#formFrame:disabled)
                { BoxElement::Disabled, { .m_fill = QBrush(PanelFill), .m_edges = BoxElement::border(1, FormBorderDisabled) } },
            },
        };

        //! The card around a form field (#formFrame): a rounded, bordered panel, 50px in; a grey border when disabled.
        const BoxElement FormFrame{
            .m_name = "ProjectManager.Box.FormFrame",
            .m_margin = QMargins(50, 10, 0, 0),               // ProjectManager.qss:214,215 (#formFrame)
            .m_padding = QMargins(6, 0, 10, 2),               // ProjectManager.qss:213 (#formFrame)
            .m_nativeBorder = false,                          // border: ProjectManager.qss:211 (#formFrame)
            .m_radius = 4,                                    // ProjectManager.qss:212 (#formFrame)
            .m_maxSize = QSize(840, -1),                      // ProjectManager.qss:209 (#formFrame)
            // ProjectManager.qss:210 (#formFrame); ProjectManager.qss:211 (#formFrame)
            .m_idle = { .m_fill = QBrush(PanelFill), .m_edges = BoxElement::border(1, FormBorder) },
            // UNVERIFIED states: disabled (no capture shows the widget in them)
            .m_stateFaces = {
                // ProjectManager.qss:210 (#formFrame); ProjectManager.qss:262 (#formFrame:disabled)
                { BoxElement::Disabled, { .m_fill = QBrush(PanelFill), .m_edges = BoxElement::border(1, FormBorderDisabled) } },
            },
        };

        //! The frame of the wizard's options (#createAGemRHS #formOptionsFrame): 2px in.
        const BoxElement FormOptionsFrameWizard{
            .m_name = "ProjectManager.Box.FormOptionsFrameWizard",
            // ProjectManager.qss:111 (#createAGemRHS #formOptionsFrame), :221 (#formOptionsFrame)
            .m_margin = QMargins(2, 10, 0, 0),
            .m_padding = QMargins(6, 0, 10, 2),               // ProjectManager.qss:220 (#formOptionsFrame)
            .m_maxSize = QSize(840, -1),                      // ProjectManager.qss:219 (#formOptionsFrame)
            .m_idle = { .m_fill = QBrush(Transparent) },      // BaseStyleSheet.qss:17 (*)
        };

        //! A check box of the wizard's options (#formOptionsWidget QCheckBox): 40px tall.
        const BoxElement FormOptionsCheckBox{
            .m_name = "ProjectManager.Box.FormOptionsCheckBox",
            .m_size = QSize(-1, 40),                          // ProjectManager.qss:178 (#formOptionsWidget QCheckBox)
            .m_idle = { .m_fill = QBrush(Transparent) },      // CheckBox.qss:12 (QCheckBox)
        };

        //! The error under a form field in the wizard and the remote dialogs: flush left. UNVERIFIED: no capture shows one.
        const BoxElement FormErrorLabelFlush{
            .m_name = "ProjectManager.Box.FormErrorLabelFlush",
            // ProjectManager.qss:347 (#createAGemBody #formErrorLabel), :351 (#addRemoteProjectDialog #formErrorLabel), :355
            // (#addRemoteTemplateDialog #formErrorLabel); Text.qss:93,94 (QLabel)
            .m_margin = QMargins(0, 1, 0, 1),
            .m_idle = { .m_fill = QBrush(Transparent) },      // BaseStyleSheet.qss:17 (*)
        };

        //! The error under a form field (#formErrorLabel): 50px in. UNVERIFIED.
        const BoxElement FormErrorLabel{
            .m_name = "ProjectManager.Box.FormErrorLabel",
            .m_margin = QMargins(50, 1, 0, 1),                // ProjectManager.qss:343 (#formErrorLabel); Text.qss:93,94 (QLabel)
            .m_idle = { .m_fill = QBrush(Transparent) },      // BaseStyleSheet.qss:17 (*)
        };

        //! A form field's row (#formLineEditWidget and its three twins): at most 890px wide.
        const BoxElement FormRow{
            .m_name = "ProjectManager.Box.FormRow",
            // ProjectManager.qss:205 (#formBrowseEditWidget), :205 (#formComboBoxWidget), :205 (#formLineEditWidget), :205
            // (#formOptionsWidget)
            .m_maxSize = QSize(890, -1),
            .m_idle = { .m_fill = QBrush(Transparent) },      // BaseStyleSheet.qss:17 (*)
        };

        //! The tag list under the wizard's tags field (#createAGemRHS #formTagField). UNVERIFIED: no capture shows one.
        const BoxElement FormTagFieldWizard{
            .m_name = "ProjectManager.Box.FormTagFieldWizard",
            .m_padding = QMargins(6, 0, 10, 2),               // ProjectManager.qss:228 (#formTagField)
            .m_maxSize = QSize(840, -1),                      // ProjectManager.qss:226 (#formTagField)
            .m_idle = { .m_fill = QBrush(PageFill) },         // ProjectManager.qss:227 (#formTagField)
        };

        //! The tag list under a tags field (#formTagField): 50px in. UNVERIFIED.
        const BoxElement FormTagField{
            .m_name = "ProjectManager.Box.FormTagField",
            .m_margin = QMargins(50, 0, 0, 0),                // ProjectManager.qss:229 (#formTagField)
            .m_padding = QMargins(6, 0, 10, 2),               // ProjectManager.qss:228 (#formTagField)
            .m_maxSize = QSize(840, -1),                      // ProjectManager.qss:226 (#formTagField)
            .m_idle = { .m_fill = QBrush(PageFill) },         // ProjectManager.qss:227 (#formTagField)
        };

        //! A tag of that list (#formTagField QCheckBox): a chip. UNVERIFIED.
        const BoxElement FormTagChip{
            .m_name = "ProjectManager.Box.FormTagChip",
            .m_padding = QMargins(7, 2, 5, 2),                // ProjectManager.qss:234,235,236,237 (#formTagField QCheckBox)
            .m_idle = { .m_fill = QBrush(ChipFill) },         // ProjectManager.qss:233 (#formTagField QCheckBox)
            // OPEN ::indicator { image: url(:/X.svg); width: 10%; padding-left: 3px } (ProjectManager.qss:242-247): a sub-control
        };

        //! The title of a settings form (#formTitleLabel): 50px in, 24px above, 10px below.
        const BoxElement FormTitle{
            .m_name = "ProjectManager.Box.FormTitle",
            .m_margin = QMargins(50, 24, 0, 10),              // ProjectManager.qss:361 (#formTitleLabel)
            .m_idle = { .m_fill = QBrush(Transparent) },      // BaseStyleSheet.qss:17 (*)
        };

        // ---- The create-a-gem wizard

        //! A step of the wizard's list (#createAGemLHS QRadioButton): 40px tall, a lighter panel while checked. The label starts 10px after
        //! the box of the indicator (`spacing`, ProjectManager.qss:83), and the button is as wide as its label and the 30px of its :checked
        //! rule (:93): Qt reads the spacing of the state of the option for the size of the button and the one of the rule with no pseudo-class
        //! for where the label sits (SE_RadioButtonContents asks with no option, qstylesheetstyle.cpp:6200), so a checked step is 20px wider than
        //! the others and its label never moves. n2: the steps hint 230 (checked) and 210, the label's ink starts at device column 47 (46 for
        //! one glyph) in every step, and the list of steps is 230 wide, where f5 had 206 (180 + the 20px of the indicator + 6px) and the label
        //! at column 41. The indicator of a step is the box with nothing in it (WizardStepIndicator).
        const BoxElement WizardStepRadio{
            .m_name = "ProjectManager.Box.WizardStepRadio",
            .m_spacing = 10,                                  // spacing: ProjectManager.qss:83 (#createAGemLHS QRadioButton)
            .m_size = QSize(-1, 40),                          // ProjectManager.qss:81 (#createAGemLHS QRadioButton)
            .m_minSize = QSize(180, -1),                      // ProjectManager.qss:82 (#createAGemLHS QRadioButton)
            .m_idle = { .m_fill = QBrush(Transparent) },      // BaseStyleSheet.qss:17 (*)
            .m_stateFaces = {
                // ProjectManager.qss:92 (#createAGemLHS QRadioButton:checked); ProjectManager.qss:93 (spacing) and :94 (color)
                { BoxElement::Checked, { .m_fill = QBrush(PageFill), .m_text = White, .m_spacing = 30 } },
            },
        };

        // ---- Indicators: the box of a check box or radio button, with its image

        //! The box of the radio buttons of the wizard's steps (#createAGemLHS QRadioButton::indicator { image: url(:/nothing.png) },
        //! ProjectManager.qss:97-99) with nothing in it: the sheet's image replaces the images of RadioButton.qss:36-64 in every state
        //! (its selector, an id, outranks theirs), :/nothing.png is no resource of the Project Manager (ProjectManager.qrc does not list the
        //! file), and an image that does not load paints nothing. The size is the sheet's: RadioButton.qss:24-26 (16px, 2px of margin: 20px of
        //! box) and :31-33 with the focus (18px, 1px of margin). n2: the steps have no mark in any state, and the label starts at 30px.
        const IndicatorElement WizardStepIndicator{
            .m_name = "ProjectManager.Indicator.WizardStep",
            .m_imageSize = QSize(16, 16),                     // RadioButton.qss:24-25 (QRadioButton::indicator:!focus)
            .m_margin = 2,                                    // RadioButton.qss:26
            .m_focusImageSize = QSize(18, 18),                // RadioButton.qss:31-32 (QRadioButton::indicator:focus)
            .m_focusMargin = 1,                               // RadioButton.qss:33
            .m_hidden = true,                                 // image: url(:/nothing.png), ProjectManager.qss:98
        };

        // ---- Item views: the header of the gem list

        //! The header that heads the gem list and the table of the repositories (`#adjustableHeaderWidget`, a QTableWidget that is only its
        //! header), `#adjustableHeaderWidget QHeaderView::section { background-color: transparent }` (ProjectManager.qss:872-874): the sections
        //! are clear and the page shows through them, where the global section is #2d2d2d (TableView.qss:69); the height, font and padding
        //! of the section rule are TableView.qss's (:62-76) and the item view element's. n2: the header's viewport is #333333 (the page)
        //! on the Gems and the Remote Sources pages; f5 #2d2d2d. TOKENIZE: AdjustableHeaderFill
        const ItemViewElement AdjustableHeader{
            .m_name = "ProjectManager.ItemView.AdjustableHeader",
            .m_headerFill = QColor(0, 0, 0, 0),               // background-color: ProjectManager.qss:873 (#adjustableHeaderWidget QHeaderView::section)
        };

        //! The scroll area of the wizard's right pane (#createAGemRightPane): 60px and 25px of padding, at least 660px wide.
        const BoxElement WizardRightPane{
            .m_name = "ProjectManager.Box.WizardRightPane",
            .m_padding = QMargins(60, 25, 0, 0),              // ProjectManager.qss:380,381 (#createAGemRightPane)
            .m_minSize = QSize(660, -1),                      // ProjectManager.qss:382 (#createAGemRightPane)
            .m_idle = { .m_fill = QBrush(Transparent) },      // BaseStyleSheet.qss:17 (*)
        };

        //! The line under the pane's header (#rightPaneSubheader): 20px of padding below, no indent.
        const BoxElement WizardSubheader{
            .m_name = "ProjectManager.Box.WizardSubheader",
            .m_margin = QMargins(0, 1, 0, 1),                 // Text.qss:93,94 (QLabel)
            .m_padding = QMargins(0, 0, 0, 20),               // ProjectManager.qss:376 (#rightPaneSubheader)
            .m_idle = { .m_fill = QBrush(Transparent) },      // BaseStyleSheet.qss:17 (*)
            .m_properties = { { "indent", 0 } },              // ProjectManager.qss:375 (#rightPaneSubheader)
        };

        //! The text under a template option (#createAGemRadioButtonSubtext).
        const BoxElement WizardRadioSubtext{
            .m_name = "ProjectManager.Box.WizardRadioSubtext",
            .m_margin = QMargins(0, 1, 0, 1),                 // Text.qss:93,94 (QLabel)
            .m_padding = QMargins(23, 0, 0, 20),              // ProjectManager.qss:391,392 (#createAGemRadioButtonSubtext)
            .m_idle = { .m_fill = QBrush(Transparent) },      // BaseStyleSheet.qss:17 (*)
        };

        //! The body of a create or edit gem screen (#createAGemBody).
        const BoxElement WizardBody{
            .m_name = "ProjectManager.Box.WizardBody",
            .m_idle = { .m_fill = QBrush(PanelFill) },        // ProjectManager.qss:452 (#createAGemBody)
        };

        //! The black strip under it (#createAGemFooter).
        const BoxElement WizardFooter{
            .m_name = "ProjectManager.Box.WizardFooter",
            // ProjectManager.qss:463 (#createAGemFooter)
            .m_idle = { .m_fill = QBrush(WizardFooterFill) },
        };

        // ---- Project settings and templates

        //! The button beside the tabs of the project settings (#projectSettingsTab::tab-bar > QPushButton): a 170x26 secondary button.
        const BoxElement SettingsTabButton{
            .m_name = "ProjectManager.Box.SettingsTabButton",
            .m_margin = QMargins(0, 0, 30, 12),               // ProjectManager.qss:594,595,596 (#projectSettingsTab::tab-bar > QPushButton)
            // border-radius: ProjectManager.qss:601 (#projectSettingsTab::tab-bar > QPushButton)
            .m_nativeBorder = false,
            .m_radius = 3,                                    // ProjectManager.qss:601 (#projectSettingsTab::tab-bar > QPushButton)
            .m_minSize = QSize(170, 26),                      // ProjectManager.qss:597,599 (#projectSettingsTab::tab-bar > QPushButton)
            .m_maxSize = QSize(170, 26),                      // ProjectManager.qss:598,600 (#projectSettingsTab::tab-bar > QPushButton)
            .m_idle = { .m_fill = SecondaryFill },            // ProjectManager.qss:35 (QPushButton[secondary="true"])
            // UNVERIFIED states: hover, pressed (no capture shows the widget in them)
            .m_stateFaces = {
                // ProjectManager.qss:39 (QPushButton[secondary="true"]:hover)
                { BoxElement::Hover, { .m_fill = SecondaryHoverFill } },
                // ProjectManager.qss:43 (QPushButton[secondary="true"]:pressed)
                { BoxElement::Pressed, { .m_fill = SecondaryPressedFill } },
            },
            .m_properties = { { "flat", true } },             // ProjectManager.qss:34 (QPushButton[secondary="true"])
            .m_textAlign = Qt::AlignCenter,                   // text-align: ProjectManager.qss:602 (#projectSettingsTab::tab-bar > QPushButton)
        };

        //! A project template's button (#projectTemplate QPushButton): 96x160, flat, a lighter panel under the pointer. UNVERIFIED: no
        //! capture shows one.
        const BoxElement TemplateButton{
            .m_name = "ProjectManager.Box.TemplateButton",
            .m_minSize = QSize(96, 160),                      // ProjectManager.qss:549,551 (#projectTemplate QPushButton)
            .m_maxSize = QSize(96, 160),                      // ProjectManager.qss:550,552 (#projectTemplate QPushButton)
            .m_idle = { .m_fill = QBrush(Transparent) },      // PushButton.qss:51 (QPushButton:flat)
            .m_stateFaces = {
                // ProjectManager.qss:568 (#projectTemplate QPushButton:hover)
                { BoxElement::Hover, { .m_fill = QBrush(PanelFill) } },
            },
            .m_properties = { { "flat", true } },             // ProjectManager.qss:548 (#projectTemplate QPushButton)
        };

        //! The picture of the checked template (#projectTemplate QPushButton[Checked="true"] #templateImage): a blue frame. UNVERIFIED.
        const BoxElement TemplateImageChecked{
            .m_name = "ProjectManager.Box.TemplateImageChecked",
            .m_margin = QMargins(0, 1, 0, 1),                 // Text.qss:93,94 (QLabel)
            // border: ProjectManager.qss:562 (#projectTemplate QPushButton[Checked="true"] #templateImage)
            .m_nativeBorder = false,
            .m_radius = 4,                                    // ProjectManager.qss:559 (#projectTemplate QPushButton #templateImage)
            // BaseStyleSheet.qss:17 (*); ProjectManager.qss:562 (#projectTemplate QPushButton[Checked="true"] #templateImage)
            .m_idle = { .m_fill = QBrush(Transparent), .m_edges = BoxElement::border(3, AccentBlue) },
        };

        //! The picture of a template (#projectTemplate QPushButton #templateImage): a clear frame at radius 4px. UNVERIFIED.
        const BoxElement TemplateImage{
            .m_name = "ProjectManager.Box.TemplateImage",
            .m_margin = QMargins(0, 1, 0, 1),                 // Text.qss:93,94 (QLabel)
            // border: ProjectManager.qss:558 (#projectTemplate QPushButton #templateImage)
            .m_nativeBorder = false,
            .m_radius = 4,                                    // ProjectManager.qss:559 (#projectTemplate QPushButton #templateImage)
            // BaseStyleSheet.qss:17 (*); ProjectManager.qss:558 (#projectTemplate QPushButton #templateImage)
            .m_idle = { .m_fill = QBrush(Transparent), .m_edges = BoxElement::border(3, Transparent) },
        };

        //! The name of a template (#projectTemplate #templateLabel): centered. UNVERIFIED.
        const BoxElement TemplateLabel{
            .m_name = "ProjectManager.Box.TemplateLabel",
            .m_margin = QMargins(0, 1, 0, 1),                 // Text.qss:93,94 (QLabel)
            .m_idle = { .m_fill = QBrush(Transparent) },      // BaseStyleSheet.qss:17 (*)
            // ProjectManager.qss:555 (#projectTemplate #templateLabel)
            .m_properties = { { "alignment", QString("AlignCenter") } },
        };

        //! The darkening veil over a template's picture (#labelButtonOverlay). UNVERIFIED.
        const BoxElement TemplateOverlay{
            .m_name = "ProjectManager.Box.TemplateOverlay",
            .m_margin = QMargins(0, 1, 0, 1),                 // Text.qss:93,94 (QLabel)
            .m_idle = { .m_fill = QBrush(ScrimFill) },        // ProjectManager.qss:738 (#labelButtonOverlay)
        };

        //! The frame of a project's settings (#projectSettings): 30px above.
        const BoxElement ProjectSettings{
            .m_name = "ProjectManager.Box.ProjectSettings",
            .m_margin = QMargins(0, 30, 0, 0),                // ProjectManager.qss:497 (#projectSettings)
            .m_idle = { .m_fill = QBrush(Transparent) },      // BaseStyleSheet.qss:17 (*)
        };

        //! The template chooser (#projectTemplate): 50px in, 25px above.
        const BoxElement ProjectTemplate{
            .m_name = "ProjectManager.Box.ProjectTemplate",
            .m_margin = QMargins(50, 25, 0, 0),               // ProjectManager.qss:501 (#projectTemplate)
            .m_idle = { .m_fill = QBrush(Transparent) },      // BaseStyleSheet.qss:17 (*)
        };

        //! The line over the template chooser (#projectTemplateDetailsLabel).
        const BoxElement TemplateDetailsLabel{
            .m_name = "ProjectManager.Box.TemplateDetailsLabel",
            .m_margin = QMargins(0, 1, 0, 20),                // Text.qss:93 (QLabel); ProjectManager.qss:511 (#projectTemplateDetailsLabel)
            .m_minSize = QSize(-1, 40),                       // ProjectManager.qss:510 (#projectTemplateDetailsLabel)
            .m_idle = { .m_fill = QBrush(Transparent) },      // BaseStyleSheet.qss:17 (*)
        };

        //! The template details panel (#projectTemplateDetails): a lighter panel, 30px in.
        const BoxElement TemplateDetails{
            .m_name = "ProjectManager.Box.TemplateDetails",
            .m_margin = QMargins(30, 0, 0, 0),                // ProjectManager.qss:518 (#projectTemplateDetails)
            .m_minSize = QSize(240, -1),                      // ProjectManager.qss:517 (#projectTemplateDetails)
            .m_maxSize = QSize(20, -1),                       // ProjectManager.qss:516 (#projectTemplateDetails)
            // `max-width: 20%` (ProjectManager.qss:516) is 20px to Qt: a % length is read as its number (qcssparser.cpp:2960-2963,
            // 439-467). The legacy widget is 50 wide at most and 270 at least with its 30px margin (the n2 dump), and the minimum wins.
            .m_idle = { .m_fill = QBrush(PanelFill) },        // ProjectManager.qss:515 (#projectTemplateDetails)
        };

        //! The summary of the chosen template (#projectTemplateDetails #summary): a line under it.
        const BoxElement TemplateSummary{
            .m_name = "ProjectManager.Box.TemplateSummary",
            .m_margin = QMargins(0, 1, 0, 1),                 // Text.qss:93,94 (QLabel)
            .m_nativeBorder = false,                          // border: ProjectManager.qss:538 (#projectTemplateDetails #summary)
            .m_minSize = QSize(-1, 80),                       // ProjectManager.qss:539 (#projectTemplateDetails #summary)
            // BaseStyleSheet.qss:17 (*); ProjectManager.qss:538 (#projectTemplateDetails #summary)
            .m_idle = { .m_fill = QBrush(Transparent), .m_edges = { {}, {}, { 2, SummaryLine }, {} } },
            // ProjectManager.qss:540 (#projectTemplateDetails #summary)
            .m_properties = { { "alignment", QString("AlignTop") } },
        };

        //! The title of its gem list (#includedGemsTitle).
        const BoxElement TemplateGemsTitle{
            .m_name = "ProjectManager.Box.TemplateGemsTitle",
            .m_margin = QMargins(0, 5, 0, 5),                 // ProjectManager.qss:532,533 (#projectTemplateDetails #includedGemsTitle)
            .m_idle = { .m_fill = QBrush(Transparent) },      // BaseStyleSheet.qss:17 (*)
        };

        //! The 'more gems' line (#moreGems): 20px above.
        const BoxElement TemplateMoreGems{
            .m_name = "ProjectManager.Box.TemplateMoreGems",
            // ProjectManager.qss:528 (#projectTemplateDetails #moreGems); Text.qss:94 (QLabel)
            .m_margin = QMargins(0, 20, 0, 1),
            .m_idle = { .m_fill = QBrush(Transparent) },      // BaseStyleSheet.qss:17 (*)
        };

        //! The 'browse catalog' link line (#browseCatalog).
        const BoxElement TemplateBrowseCatalog{
            .m_name = "ProjectManager.Box.TemplateBrowseCatalog",
            .m_margin = QMargins(0, 5, 0, 15),                // ProjectManager.qss:544 (#projectTemplateDetails #browseCatalog)
            .m_idle = { .m_fill = QBrush(Transparent) },      // BaseStyleSheet.qss:17 (*)
        };

        // ---- The gem catalog and the gem repositories

        //! The 'Use Version' button of the gem inspector: its box with a margin above and below (#GemCatalogVersion QPushButton).
        //! UNVERIFIED.
        const BoxElement InspectorVersionButton{
            .m_name = "ProjectManager.Box.InspectorVersionButton",
            .m_margin = QMargins(0, 5, 0, 5),                 // ProjectManager.qss:970 (#GemCatalogVersion QPushButton)
            .m_nativeBorder = false,                          // border-radius: ProjectManager.qss:905 (#GemCatalogInspector QPushButton)
            .m_radius = 3,                                    // ProjectManager.qss:905 (#GemCatalogInspector QPushButton)
            .m_minSize = QSize(-1, 24),                       // ProjectManager.qss:903 (#GemCatalogInspector QPushButton)
            .m_maxSize = QSize(-1, 24),                       // ProjectManager.qss:904 (#GemCatalogInspector QPushButton)
            .m_idle = { .m_fill = SecondaryFill },            // ProjectManager.qss:35 (QPushButton[secondary="true"])
            .m_stateFaces = {
                // ProjectManager.qss:39 (QPushButton[secondary="true"]:hover)
                { BoxElement::Hover, { .m_fill = SecondaryHoverFill } },
                // ProjectManager.qss:43 (QPushButton[secondary="true"]:pressed)
                { BoxElement::Pressed, { .m_fill = SecondaryPressedFill } },
            },
            .m_properties = { { "flat", true } },             // ProjectManager.qss:902 (#GemCatalogInspector QPushButton)
            .m_textAlign = Qt::AlignCenter,                   // text-align: ProjectManager.qss:906 (#GemCatalogInspector QPushButton)
        };

        //! A secondary button of the gem inspector (#GemCatalogInspector QPushButton).
        const BoxElement InspectorButtonSecondary{
            .m_name = "ProjectManager.Box.InspectorButtonSecondary",
            .m_nativeBorder = false,                          // border-radius: ProjectManager.qss:905 (#GemCatalogInspector QPushButton)
            .m_radius = 3,                                    // ProjectManager.qss:905 (#GemCatalogInspector QPushButton)
            .m_minSize = QSize(-1, 24),                       // ProjectManager.qss:903 (#GemCatalogInspector QPushButton)
            .m_maxSize = QSize(-1, 24),                       // ProjectManager.qss:904 (#GemCatalogInspector QPushButton)
            .m_idle = { .m_fill = SecondaryFill },            // ProjectManager.qss:35 (QPushButton[secondary="true"])
            // UNVERIFIED states: hover, pressed (no capture shows the widget in them)
            .m_stateFaces = {
                // ProjectManager.qss:39 (QPushButton[secondary="true"]:hover)
                { BoxElement::Hover, { .m_fill = SecondaryHoverFill } },
                // ProjectManager.qss:43 (QPushButton[secondary="true"]:pressed)
                { BoxElement::Pressed, { .m_fill = SecondaryPressedFill } },
            },
            .m_properties = { { "flat", true } },             // ProjectManager.qss:902 (#GemCatalogInspector QPushButton)
            .m_textAlign = Qt::AlignCenter,                   // text-align: ProjectManager.qss:906 (#GemCatalogInspector QPushButton)
        };

        //! The 'Download Gem' button of the gem inspector. UNVERIFIED: no capture shows one.
        const BoxElement InspectorButtonPrimary{
            .m_name = "ProjectManager.Box.InspectorButtonPrimary",
            .m_nativeBorder = false,                          // border-radius: ProjectManager.qss:905 (#GemCatalogInspector QPushButton)
            .m_radius = 3,                                    // ProjectManager.qss:905 (#GemCatalogInspector QPushButton)
            .m_minSize = QSize(-1, 24),                       // ProjectManager.qss:903 (#GemCatalogInspector QPushButton)
            .m_maxSize = QSize(-1, 24),                       // ProjectManager.qss:904 (#GemCatalogInspector QPushButton)
            .m_idle = { .m_fill = PrimaryFill },              // ProjectManager.qss:17 (QPushButton[primary="true"])
            .m_stateFaces = {
                // ProjectManager.qss:21 (QPushButton[primary="true"]:hover)
                { BoxElement::Hover, { .m_fill = PrimaryHoverFill } },
                // ProjectManager.qss:25 (QPushButton[primary="true"]:pressed)
                { BoxElement::Pressed, { .m_fill = PrimaryPressedFill } },
                // ProjectManager.qss:29 (QPushButton[primary="true"]:disabled)
                { BoxElement::Disabled, { .m_fill = PrimaryDisabledFill } },
            },
            .m_properties = { { "flat", true } },             // ProjectManager.qss:902 (#GemCatalogInspector QPushButton)
            .m_textAlign = Qt::AlignCenter,                   // text-align: ProjectManager.qss:906 (#GemCatalogInspector QPushButton)
        };

        //! The 'Uninstall Gem' button of the gem inspector. UNVERIFIED: no capture shows one.
        const BoxElement InspectorButtonDanger{
            .m_name = "ProjectManager.Box.InspectorButtonDanger",
            .m_nativeBorder = false,                          // border-radius: ProjectManager.qss:905 (#GemCatalogInspector QPushButton)
            .m_radius = 3,                                    // ProjectManager.qss:905 (#GemCatalogInspector QPushButton)
            .m_minSize = QSize(-1, 24),                       // ProjectManager.qss:903 (#GemCatalogInspector QPushButton)
            .m_maxSize = QSize(-1, 24),                       // ProjectManager.qss:904 (#GemCatalogInspector QPushButton)
            .m_idle = { .m_fill = DangerFill },               // ProjectManager.qss:49 (QPushButton[danger="true"])
            .m_stateFaces = {
                // ProjectManager.qss:53 (QPushButton[danger="true"]:hover)
                { BoxElement::Hover, { .m_fill = DangerHoverFill } },
                // ProjectManager.qss:57 (QPushButton[danger="true"]:pressed)
                { BoxElement::Pressed, { .m_fill = DangerPressedFill } },
            },
            .m_properties = { { "flat", true } },             // ProjectManager.qss:902 (#GemCatalogInspector QPushButton)
            .m_textAlign = Qt::AlignCenter,                   // text-align: ProjectManager.qss:906 (#GemCatalogInspector QPushButton)
        };

        //! The 'Remove' button of the gem repository inspector (#gemRepoInspector QPushButton): red, 10px below the details.
        const BoxElement RepoInspectorButtonDanger{
            .m_name = "ProjectManager.Box.RepoInspectorButtonDanger",
            .m_margin = QMargins(0, 10, 0, 0),                // ProjectManager.qss:1126 (#gemRepoInspector QPushButton)
            .m_nativeBorder = false,                          // border-radius: ProjectManager.qss:1122 (#gemRepoInspector QPushButton)
            .m_radius = 3,                                    // ProjectManager.qss:1122 (#gemRepoInspector QPushButton)
            .m_minSize = QSize(-1, 24),                       // ProjectManager.qss:1120 (#gemRepoInspector QPushButton)
            .m_maxSize = QSize(-1, 24),                       // ProjectManager.qss:1121 (#gemRepoInspector QPushButton)
            .m_idle = { .m_fill = DangerFill },               // ProjectManager.qss:49 (QPushButton[danger="true"])
            // UNVERIFIED states: hover, pressed (no capture shows the widget in them)
            .m_stateFaces = {
                // ProjectManager.qss:53 (QPushButton[danger="true"]:hover)
                { BoxElement::Hover, { .m_fill = DangerHoverFill } },
                // ProjectManager.qss:57 (QPushButton[danger="true"]:pressed)
                { BoxElement::Pressed, { .m_fill = DangerPressedFill } },
            },
            .m_properties = { { "flat", true } },             // ProjectManager.qss:1119 (#gemRepoInspector QPushButton)
            .m_textAlign = Qt::AlignCenter,                   // text-align: ProjectManager.qss:1123 (#gemRepoInspector QPushButton)
        };

        //! The add and update buttons of the gem repositories page (#gemRepoAddButton): 120x24, secondary. The rule has a position
        //! (`text-align: center`, :1062), so the sheet lays the label of 'Update All' out itself: its icon and its text 4px apart, where the
        //! base style puts the text 2px after the icon. n2: the text of the button starts 3 device px later than in f5, the icon is where
        //! it was (the Remote Sources page, the button at [775,200,120,24]: ink of the icon at device columns 25 to 55 in both, of the text
        //! 67 to 153 in n2 and 64 to 150 in f5).
        const BoxElement GemRepoAddButton{
            .m_name = "ProjectManager.Box.GemRepoAddButton",
            .m_nativeBorder = false,                          // border-radius: ProjectManager.qss:1061 (#gemRepoAddButton)
            .m_radius = 3,                                    // ProjectManager.qss:1061 (#gemRepoAddButton)
            .m_minSize = QSize(120, 24),                      // ProjectManager.qss:1057,1059 (#gemRepoAddButton)
            .m_maxSize = QSize(120, 24),                      // ProjectManager.qss:1058,1060 (#gemRepoAddButton)
            .m_idle = { .m_fill = SecondaryFill },            // ProjectManager.qss:35 (QPushButton[secondary="true"])
            // UNVERIFIED states: hover, pressed (no capture shows the widget in them)
            .m_stateFaces = {
                // ProjectManager.qss:39 (QPushButton[secondary="true"]:hover)
                { BoxElement::Hover, { .m_fill = SecondaryHoverFill } },
                // ProjectManager.qss:43 (QPushButton[secondary="true"]:pressed)
                { BoxElement::Pressed, { .m_fill = SecondaryPressedFill } },
            },
            .m_properties = { { "flat", true } },             // ProjectManager.qss:34 (QPushButton[secondary="true"])
            .m_textAlign = Qt::AlignCenter,                   // text-align: ProjectManager.qss:1062 (#gemRepoAddButton)
        };

        //! The requirements notice of the gem inspector (#GemCatalogInspector #GemCatalogRequirements): its info icon. UNVERIFIED.
        const BoxElement InspectorNoticeInfo{
            .m_name = "ProjectManager.Box.InspectorNoticeInfo",
            .m_margin = QMargins(0, 0, 0, 10),                // ProjectManager.qss:848 (#GemCatalogInspector #GemCatalogRequirements)
            .m_padding = QMargins(34, 0, 0, 0),               // ProjectManager.qss:849 (#GemCatalogInspector #GemCatalogRequirements)
            .m_minSize = QSize(-1, 34),                       // ProjectManager.qss:850 (#GemCatalogInspector #GemCatalogRequirements)
            // ProjectManager.qss:853 (#GemCatalogInspector #GemCatalogRequirements)
            .m_idle = { .m_fill = QBrush(Transparent), .m_image = ":/Info.svg" },
            // ProjectManager.qss:853 (#GemCatalogInspector #GemCatalogRequirements)
            .m_imageAlign = Qt::AlignLeft | Qt::AlignVCenter,
        };

        //! The compatibility warning of the gem inspector: its warning icon. UNVERIFIED.
        const BoxElement InspectorNoticeWarning{
            .m_name = "ProjectManager.Box.InspectorNoticeWarning",
            // ProjectManager.qss:848 (#GemCatalogInspector #GemCatalogCompatibilityWarning)
            .m_margin = QMargins(0, 0, 0, 10),
            // ProjectManager.qss:849 (#GemCatalogInspector #GemCatalogCompatibilityWarning)
            .m_padding = QMargins(34, 0, 0, 0),
            // ProjectManager.qss:850 (#GemCatalogInspector #GemCatalogCompatibilityWarning)
            .m_minSize = QSize(-1, 34),
            // ProjectManager.qss:857 (#GemCatalogInspector #GemCatalogCompatibilityWarning)
            .m_idle = { .m_fill = QBrush(Transparent), .m_image = ":/Warning.svg" },
            // ProjectManager.qss:857 (#GemCatalogInspector #GemCatalogCompatibilityWarning)
            .m_imageAlign = Qt::AlignLeft | Qt::AlignVCenter,
        };

        //! A field name in the gem inspector (QLabel[class="label"]): 50px wide.
        const BoxElement InspectorFieldName{
            .m_name = "ProjectManager.Box.InspectorFieldName",
            // a margin or padding of 0: ProjectManager.qss:957 (#GemCatalogInspector QLabel)
            .m_hasBox = true,
            .m_minSize = QSize(50, -1),                       // ProjectManager.qss:965 (#GemCatalogInspector QLabel[class="label"])
            .m_maxSize = QSize(50, -1),                       // ProjectManager.qss:966 (#GemCatalogInspector QLabel[class="label"])
            .m_idle = { .m_fill = QBrush(Transparent) },      // BaseStyleSheet.qss:17 (*)
        };

        //! A tag in an inspector (#TagWidget): a rounded chip with no margin.
        const BoxElement InspectorTag{
            .m_name = "ProjectManager.Box.InspectorTag",
            .m_padding = QMargins(3, 3, 3, 3),                // ProjectManager.qss:921 (#TagWidget)
            .m_nativeBorder = false,                          // border-radius: ProjectManager.qss:924 (#TagWidget)
            .m_radius = 3,                                    // ProjectManager.qss:924 (#TagWidget)
            .m_idle = { .m_fill = QBrush(PageFill) },         // ProjectManager.qss:920 (#TagWidget)
        };

        //! A tag anywhere else (the cart, the template details): the chip with the label margins. UNVERIFIED.
        const BoxElement Tag{
            .m_name = "ProjectManager.Box.Tag",
            .m_margin = QMargins(0, 1, 0, 1),                 // Text.qss:93,94 (QLabel)
            .m_padding = QMargins(3, 3, 3, 3),                // ProjectManager.qss:921 (#TagWidget)
            .m_nativeBorder = false,                          // border-radius: ProjectManager.qss:924 (#TagWidget)
            .m_radius = 3,                                    // ProjectManager.qss:924 (#TagWidget)
            .m_idle = { .m_fill = QBrush(PageFill) },         // ProjectManager.qss:920 (#TagWidget)
        };

        //! Every other label of an inspector (#GemCatalogInspector QLabel, #gemRepoInspector QLabel): no margin.
        const BoxElement InspectorLabel{
            .m_name = "ProjectManager.Box.InspectorLabel",
            // a margin or padding of 0: ProjectManager.qss:957 (#GemCatalogInspector QLabel), :1088 (#gemRepoInspector QLabel)
            .m_hasBox = true,
            .m_idle = { .m_fill = QBrush(Transparent) },      // BaseStyleSheet.qss:17 (*)
        };

        //! The side panes of the gem pages (#GemCatalogInspector, #GemCatalogFilterWidget, #gemRepoInspector): a lighter panel.
        const BoxElement Inspector{
            .m_name = "ProjectManager.Box.Inspector",
            .m_hasBox = true,                                 // a margin or padding of 0: QDockWidget.qss:35 (QAbstractScrollArea)
            // ProjectManager.qss:942 (#GemCatalogInspector), :977 (#GemCatalogFilterWidget), :1084 (#gemRepoInspector)
            .m_idle = { .m_fill = QBrush(PanelFill) },
        };

        //! A tag container of the gem inspector (#GemCatalogInspector #TagWidgetContainer): 210px wide. UNVERIFIED.
        const BoxElement InspectorTagContainer{
            .m_name = "ProjectManager.Box.InspectorTagContainer",
            .m_minSize = QSize(210, -1),                      // ProjectManager.qss:952 (#GemCatalogInspector #TagWidgetContainer)
            .m_maxSize = QSize(210, -1),                      // ProjectManager.qss:953 (#GemCatalogInspector #TagWidgetContainer)
            .m_idle = { .m_fill = QBrush(Transparent) },      // BaseStyleSheet.qss:17 (*)
        };

        //! A tag container of the repository inspector (#gemRepoInspector #TagWidgetContainer): 210px wide, no margin.
        const BoxElement RepoInspectorTagContainer{
            .m_name = "ProjectManager.Box.RepoInspectorTagContainer",
            // a margin or padding of 0: ProjectManager.qss:1100 (#gemRepoInspector #TagWidgetContainer)
            .m_hasBox = true,
            .m_minSize = QSize(210, -1),                      // ProjectManager.qss:1098 (#gemRepoInspector #TagWidgetContainer)
            .m_maxSize = QSize(210, -1),                      // ProjectManager.qss:1099 (#gemRepoInspector #TagWidgetContainer)
            .m_idle = { .m_fill = QBrush(Transparent) },      // BaseStyleSheet.qss:17 (*)
        };

        //! The count beside a filter (#GemCatalogFilterWidget #CountLabel): a rounded badge.
        const BoxElement FilterCount{
            .m_name = "ProjectManager.Box.FilterCount",
            .m_margin = QMargins(0, 1, 0, 1),                 // Text.qss:93,94 (QLabel)
            // border-radius: ProjectManager.qss:1000 (#GemCatalogFilterWidget #CountLabel)
            .m_nativeBorder = false,
            .m_radius = 3,                                    // ProjectManager.qss:1000 (#GemCatalogFilterWidget #CountLabel)
            .m_idle = { .m_fill = QBrush(PageFill) },         // ProjectManager.qss:999 (#GemCatalogFilterWidget #CountLabel)
        };

        //! The number on the cart button (#GemCatalogCartCountLabel): a blue badge.
        const BoxElement CartCount{
            .m_name = "ProjectManager.Box.CartCount",
            .m_margin = QMargins(0, 1, 0, 1),                 // Text.qss:93,94 (QLabel)
            .m_nativeBorder = false,                          // border-radius: ProjectManager.qss:817 (#GemCatalogCartCountLabel)
            .m_radius = 3,                                    // ProjectManager.qss:817 (#GemCatalogCartCountLabel)
            .m_idle = { .m_fill = QBrush(CountBadgeFill) },   // ProjectManager.qss:816 (#GemCatalogCartCountLabel)
        };

        //! The cart popup (#GemCatalogCart): a lighter panel. UNVERIFIED: no capture shows it.
        const BoxElement GemCart{
            .m_name = "ProjectManager.Box.GemCart",
            .m_hasBox = true,                                 // a margin or padding of 0: QDockWidget.qss:35 (QAbstractScrollArea)
            .m_idle = { .m_fill = QBrush(CartFill) },         // ProjectManager.qss:811 (#GemCatalogCart)
        };

        //! The list of downloads in the cart (#GemCatalogCartOverlayGemDownloadBG). UNVERIFIED.
        const BoxElement CartDownloadList{
            .m_name = "ProjectManager.Box.CartDownloadList",
            // a margin or padding of 0: ProjectManager.qss:867 (#GemCatalogCartOverlayGemDownloadBG); ProjectManager.qss:868
            // (#GemCatalogCartOverlayGemDownloadBG)
            .m_hasBox = true,
            .m_idle = { .m_fill = QBrush(PanelFill) },        // ProjectManager.qss:869 (#GemCatalogCartOverlayGemDownloadBG)
        };

        //! The header of that list (#GemCatalogCartOverlayGemDownloadHeader). UNVERIFIED.
        const BoxElement CartDownloadHeader{
            .m_name = "ProjectManager.Box.CartDownloadHeader",
            // a margin or padding of 0: ProjectManager.qss:861 (#GemCatalogCartOverlayGemDownloadHeader); ProjectManager.qss:862
            // (#GemCatalogCartOverlayGemDownloadHeader)
            .m_hasBox = true,
            .m_idle = { .m_fill = QBrush(PageFill) },         // ProjectManager.qss:863 (#GemCatalogCartOverlayGemDownloadHeader)
        };

        //! The gem list (#GemCatalogListView): the grey page fill, no frame.
        const BoxElement GemList{
            .m_name = "ProjectManager.Box.GemList",
            .m_hasBox = true,                                 // a margin or padding of 0: QDockWidget.qss:35 (QAbstractScrollArea)
            .m_nativeBorder = false,                          // border: TableView.qss:28 (QListView)
            .m_idle = { .m_fill = QBrush(PageFill) },         // ProjectManager.qss:883 (#GemCatalogListView)
            // OPEN the sections of its header (#adjustableHeaderWidget QHeaderView::section { background-color: transparent },
            // ProjectManager.qss:872): no element family holds a header section
        };

        //! The count over the gem list (#GemCatalogHeaderShowCountLabel); its fill is the list header's inline sheet.
        const BoxElement GemListCount{
            .m_name = "ProjectManager.Box.GemListCount",
            .m_margin = QMargins(0, 1, 0, 1),                 // Text.qss:93,94 (QLabel)
            .m_idle = { .m_fill = QBrush(PageFill) },         // GemListHeaderWidget.cpp:28 (*)
        };

        //! The refresh button over the gem list (#RefreshButton): 24x24, its icon.
        const BoxElement GemListRefreshButton{
            .m_name = "ProjectManager.Box.GemListRefreshButton",
            .m_minSize = QSize(24, 24),                       // ProjectManager.qss:828,830 (#RefreshButton)
            .m_maxSize = QSize(24, 24),                       // ProjectManager.qss:827,829 (#RefreshButton)
            // GemListHeaderWidget.cpp:28 (*); ProjectManager.qss:825 (#RefreshButton)
            .m_idle = { .m_fill = QBrush(PageFill), .m_image = ":/Refresh.svg" },
            // OPEN the faces: under GemListHeaderWidget's inline sheet (`background-color: #333333`, GemListHeaderWidget.cpp:28, for its
            // whole subtree): QStyleSheetStyle still paints this button, so the style may never be asked for its icon; limits and
            // properties apply
            // UNVERIFIED states: hover (no capture shows the widget in them)
            .m_stateFaces = {
                // GemListHeaderWidget.cpp:28 (*); ProjectManager.qss:834 (#RefreshButton:hover)
                { BoxElement::Hover, { .m_fill = QBrush(PageFill), .m_image = ":/Refresh_Hover.svg" } },
            },
            .m_imageAlign = Qt::AlignCenter,                  // ProjectManager.qss:825 (#RefreshButton)
            .m_properties = { { "flat", true } },             // ProjectManager.qss:826 (#RefreshButton)
        };

        //! The menu button of the catalog header (#gemCatalogMenuButton): 36x24.
        const BoxElement GemCatalogMenuButton{
            .m_name = "ProjectManager.Box.GemCatalogMenuButton",
            .m_minSize = QSize(36, 24),                       // ProjectManager.qss:840,842 (#gemCatalogMenuButton)
            .m_maxSize = QSize(36, 24),                       // ProjectManager.qss:839,841 (#gemCatalogMenuButton)
            .m_idle = { .m_fill = QBrush(Transparent) },      // PushButton.qss:51 (QPushButton:flat)
            .m_properties = { { "flat", true } },             // ProjectManager.qss:838 (#gemCatalogMenuButton)
        };

        // ---- Dialogs and the toast

        //! The address in the external link dialog (#ExternalLinkDialog #externalLink). UNVERIFIED: no capture shows it.
        const BoxElement ExternalLink{
            .m_name = "ProjectManager.Box.ExternalLink",
            .m_margin = QMargins(0, 5, 0, 30),                // ProjectManager.qss:491 (#ExternalLinkDialog #externalLink)
            .m_idle = { .m_fill = QBrush(Transparent) },      // BaseStyleSheet.qss:17 (*)
        };

        //! A label of the gem dependencies dialog (#GemDependenciesDialog QLabel). UNVERIFIED.
        const BoxElement DependencyLabel{
            .m_name = "ProjectManager.Box.DependencyLabel",
            // Text.qss:93 (QLabel); ProjectManager.qss:887 (#GemDependenciesDialog QLabel)
            .m_margin = QMargins(0, 1, 0, 10),
            .m_idle = { .m_fill = QBrush(Transparent) },      // BaseStyleSheet.qss:17 (*)
        };

        //! A check box of that dialog (#GemDependenciesDialog QCheckBox): a chip. UNVERIFIED.
        const BoxElement DependencyCheckBox{
            .m_name = "ProjectManager.Box.DependencyCheckBox",
            .m_margin = QMargins(0, 5, 5, 0),                 // ProjectManager.qss:894,896 (#GemDependenciesDialog QCheckBox)
            .m_padding = QMargins(4, 4, 4, 4),                // ProjectManager.qss:895 (#GemDependenciesDialog QCheckBox)
            .m_nativeBorder = false,                          // border-radius: ProjectManager.qss:892 (#GemDependenciesDialog QCheckBox)
            .m_radius = 3,                                    // ProjectManager.qss:892 (#GemDependenciesDialog QCheckBox)
            .m_idle = { .m_fill = QBrush(PageFill) },         // ProjectManager.qss:891 (#GemDependenciesDialog QCheckBox)
        };

        //! A toast notification (#ToastNotification): black, a light rounded outline. UNVERIFIED: no capture shows one.
        const BoxElement Toast{
            .m_name = "ProjectManager.Box.Toast",
            .m_nativeBorder = false,                          // border: ProjectManager.qss:184 (#ToastNotification)
            .m_radius = 20,                                   // ProjectManager.qss:183 (#ToastNotification)
            // ProjectManager.qss:182 (#ToastNotification); ProjectManager.qss:184 (#ToastNotification)
            .m_idle = { .m_fill = QBrush(ToastFill), .m_edges = BoxElement::border(1, ToastBorder) },
            // ProjectManager.qss:185 (#ToastNotification)
            .m_properties = { { "minimumSize", QSize(100, 50) } },
        };

        //! The frame of a toast's icon (#ToastNotification #icon_frame). UNVERIFIED.
        const BoxElement ToastIconFrame{
            .m_name = "ProjectManager.Box.ToastIconFrame",
            .m_nativeBorder = false,                          // border-radius: ProjectManager.qss:189 (#ToastNotification #icon_frame)
            .m_radius = 4,                                    // ProjectManager.qss:189 (#ToastNotification #icon_frame)
            .m_idle = { .m_fill = QBrush(Transparent) },      // BaseStyleSheet.qss:17 (*)
            // ProjectManager.qss:190 (#ToastNotification #icon_frame)
            .m_properties = { { "minimumSize", QSize(44, 20) } },
        };

        //! A toast's icon (#ToastNotification #iconLabel): 30x20. UNVERIFIED.
        const BoxElement ToastIcon{
            .m_name = "ProjectManager.Box.ToastIcon",
            // ProjectManager.qss:196 (#ToastNotification #iconLabel); Text.qss:93,94 (QLabel)
            .m_margin = QMargins(6, 1, 0, 1),
            .m_idle = { .m_fill = QBrush(Transparent) },      // BaseStyleSheet.qss:17 (*)
            // ProjectManager.qss:194,195 (#ToastNotification #iconLabel)
            .m_properties = { { "minimumSize", QSize(30, 20) }, { "maximumSize", QSize(30, 20) } },
        };

        // ---- The indicators and the item views of the sheet

        void registerIndicators()
        {
            // `#createAGemLHS QRadioButton::indicator` (ProjectManager.qss:97-99): any radio button below the list of steps
            IndicatorElement::registerElement(&WizardStepIndicator, &QRadioButton::staticMetaObject,
                [](const QWidget* w) { return within(w, "createAGemLHS"); });
        }

        void registerItemViews()
        {
            // `#adjustableHeaderWidget QHeaderView::section` (ProjectManager.qss:872-874): the header is a QTableWidget named so
            ItemViewElement::registerElement(&AdjustableHeader, &QTableView::staticMetaObject,
                [](const QWidget* w) { return named(w, "adjustableHeaderWidget"); });
        }

        void registerBoxes()
        {
            BoxElement::registerElement(&ProgressBar, &QProgressBar::staticMetaObject,
                [](const QWidget*) { return true; });
            BoxElement::registerElement(&ScreensCtrl, &QWidget::staticMetaObject,
                [](const QWidget* w) { return named(w, "ScreensCtrl"); });
            BoxElement::registerElement(&MainWindow, &QMainWindow::staticMetaObject,
                [](const QWidget*) { return true; });
            BoxElement::registerElement(&FooterButtonPrimary, &QPushButton::staticMetaObject,
                [](const QWidget* w) { return childOf(w, "footer") && hasProperty(w, "primary", "true"); });
            BoxElement::registerElement(&FooterButtonSecondary, &QPushButton::staticMetaObject,
                [](const QWidget* w) { return childOf(w, "footer") && hasProperty(w, "secondary", "true"); });
            BoxElement::registerElement(&FooterButtonDanger, &QPushButton::staticMetaObject,
                [](const QWidget* w) { return childOf(w, "footer") && hasProperty(w, "danger", "true"); });
            BoxElement::registerElement(&HeaderBackButton, &QPushButton::staticMetaObject,
                [](const QWidget* w) { return within(w, "header"); });
            BoxElement::registerElement(&HeaderTitle, &QLabel::staticMetaObject,
                [](const QWidget* w) { return named(w, "headerTitle"); });
            BoxElement::registerElement(&HeaderSubtitle, &QLabel::staticMetaObject,
                [](const QWidget* w) { return named(w, "headerSubTitle"); });
            BoxElement::registerElement(&Header, &QFrame::staticMetaObject,
                [](const QWidget* w) { return named(w, "header"); });
            BoxElement::registerElement(&Footer, &QDialogButtonBox::staticMetaObject,
                [](const QWidget* w) { return named(w, "footer"); });
            BoxElement::registerElement(&Body, &QFrame::staticMetaObject,
                [](const QWidget* w)
                {
                    // the third selector names its class: QStackedWidget#createAGemRHS (ProjectManager.qss:101)
                    return named(w, "body") || named(w, "GemCatalogScreen") ||
                        (named(w, "createAGemRHS") && qobject_cast<const QStackedWidget*>(w));
                });
            BoxElement::registerElement(&TopBar, &QFrame::staticMetaObject,
                [](const QWidget* w)
                {
                    return named(w, "engineTopFrame") || named(w, "projectSettingsTopFrame") || named(w, "GemCatalogHeaderWidget");
                });
            BoxElement::registerElement(&StartCardCreate, &QPushButton::staticMetaObject,
                [](const QWidget* w) { return childOf(w, "firstTimeContent") && named(w, "createProjectButton"); });
            BoxElement::registerElement(&StartCardOpen, &QPushButton::staticMetaObject,
                [](const QWidget* w) { return childOf(w, "firstTimeContent") && named(w, "addProjectButton"); });
            BoxElement::registerElement(&StartCardRemote, &QPushButton::staticMetaObject,
                [](const QWidget* w) { return childOf(w, "firstTimeContent") && named(w, "addRemoteProjectButton"); });
            BoxElement::registerElement(&StartTitle, &QLabel::staticMetaObject,
                [](const QWidget* w) { return childOf(w, "firstTimeContent") && named(w, "titleLabel"); });
            BoxElement::registerElement(&StartIntro, &QLabel::staticMetaObject,
                [](const QWidget* w) { return childOf(w, "firstTimeContent") && named(w, "introLabel"); });
            BoxElement::registerElement(&ProjectCardCancel, &QPushButton::staticMetaObject,
                [](const QWidget* w) { return named(w, "projectActionCancelButton") && within(w, "labelButton"); });
            BoxElement::registerElement(&NewProjectButton, &QPushButton::staticMetaObject,
                [](const QWidget* w) { return named(w, "newProjectButton") && parentChildOf(w, "projectsContent"); });
            BoxElement::registerElement(&ProjectsTitle, &QLabel::staticMetaObject,
                [](const QWidget* w)
                {
                    return named(w, "rightPaneHeader") || (named(w, "titleLabel") && parentChildOf(w, "projectsContent"));
                });
            BoxElement::registerElement(&ProjectsGrid, &QScrollArea::staticMetaObject,
                [](const QWidget* w) { return childOf(w, "projectsContent"); });
            BoxElement::registerElement(&ProjectsHeader, &QFrame::staticMetaObject,
                [](const QWidget* w) { return childOf(w, "projectsContent"); });
            BoxElement::registerElement(&ProjectCardOverlay, &QLabel::staticMetaObject,
                [](const QWidget* w) { return named(w, "labelButtonOverlay") && within(w, "projectButton"); });
            BoxElement::registerElement(&ProjectCardMessage, &QLabel::staticMetaObject,
                [](const QWidget* w) { return named(w, "projectMessageOverlay"); });
            BoxElement::registerElement(&ProjectCardSubMessage, &QLabel::staticMetaObject,
                [](const QWidget* w) { return named(w, "projectSubMessageOverlay"); });
            BoxElement::registerElement(&ProjectCardOutline, &QLabel::staticMetaObject,
                [](const QWidget* w) { return named(w, "labelButton") && childOf(w, "projectButton"); });
            BoxElement::registerElement(&ProjectCardAction, &QPushButton::staticMetaObject,
                [](const QWidget* w) { return within(w, "labelButton"); });
            BoxElement::registerElement(&ProjectCardLabel, &QLabel::staticMetaObject,
                [](const QWidget* w) { return within(w, "projectButton"); });
            BoxElement::registerElement(&ProjectCardFooter, &QFrame::staticMetaObject,
                [](const QWidget* w) { return childOf(w, "projectButton"); });
            BoxElement::registerElement(&ProjectCard, &QFrame::staticMetaObject,
                [](const QWidget* w) { return named(w, "projectButton"); });
            BoxElement::registerElement(&ProjectMenuButton, &QPushButton::staticMetaObject,
                [](const QWidget* w) { return named(w, "projectMenuButton"); });
            BoxElement::registerElement(&FormDropDownButton, &QPushButton::staticMetaObject,
                [](const QWidget* w) { return named(w, "dropDownButton") && within(w, "formFrame"); });
            BoxElement::registerElement(&FormBrowseButton, &QPushButton::staticMetaObject,
                [](const QWidget* w) { return within(w, "formFrame"); });
            BoxElement::registerElement(&FormLineEdit, &QLineEdit::staticMetaObject,
                [](const QWidget* w) { return within(w, "formFrame"); });
            BoxElement::registerElement(&FormComboBox, &QComboBox::staticMetaObject,
                [](const QWidget* w) { return within(w, "formFrame"); });
            BoxElement::registerElement(&FormFrameNestedInvalid, &QFrame::staticMetaObject,
                [](const QWidget* w)
                {
                    return named(w, "formFrame") && within(w, "createAGemRadioButtonSubFormField") && hasProperty(w, "Valid", "false");
                });
            BoxElement::registerElement(&FormFrameNestedFocused, &QFrame::staticMetaObject,
                [](const QWidget* w)
                {
                    return named(w, "formFrame") && within(w, "createAGemRadioButtonSubFormField") && hasProperty(w, "Focus", "true");
                });
            BoxElement::registerElement(&FormFrameNested, &QFrame::staticMetaObject,
                [](const QWidget* w) { return named(w, "formFrame") && within(w, "createAGemRadioButtonSubFormField"); });
            BoxElement::registerElement(&FormFrameWizardInvalid, &QFrame::staticMetaObject,
                [](const QWidget* w)
                {
                    return named(w, "formFrame") &&
                            (within(w, "createAGemRHS") || within(w, "addGemRepoDialog") || within(w, "addRemoteTemplateDialog") ||
                            within(w, "addRemoteProjectDialog")) &&
                        hasProperty(w, "Valid", "false");
                });
            BoxElement::registerElement(&FormFrameWizardFocused, &QFrame::staticMetaObject,
                [](const QWidget* w)
                {
                    return named(w, "formFrame") &&
                            (within(w, "createAGemRHS") || within(w, "addGemRepoDialog") || within(w, "addRemoteTemplateDialog") ||
                            within(w, "addRemoteProjectDialog")) &&
                        hasProperty(w, "Focus", "true");
                });
            BoxElement::registerElement(&FormFrameWizard, &QFrame::staticMetaObject,
                [](const QWidget* w)
                {
                    return named(w, "formFrame") &&
                            (within(w, "createAGemRHS") || within(w, "addGemRepoDialog") || within(w, "addRemoteTemplateDialog") ||
                            within(w, "addRemoteProjectDialog"));
                });
            BoxElement::registerElement(&FormFrameInvalid, &QFrame::staticMetaObject,
                [](const QWidget* w) { return named(w, "formFrame") && hasProperty(w, "Valid", "false"); });
            BoxElement::registerElement(&FormFrameFocused, &QFrame::staticMetaObject,
                [](const QWidget* w) { return named(w, "formFrame") && hasProperty(w, "Focus", "true"); });
            BoxElement::registerElement(&FormFrame, &QFrame::staticMetaObject,
                [](const QWidget* w) { return named(w, "formFrame"); });
            BoxElement::registerElement(&FormOptionsFrameWizard, &QFrame::staticMetaObject,
                [](const QWidget* w) { return named(w, "formOptionsFrame") && within(w, "createAGemRHS"); });
            BoxElement::registerElement(&FormOptionsCheckBox, &QCheckBox::staticMetaObject,
                [](const QWidget* w) { return within(w, "formOptionsWidget"); });
            BoxElement::registerElement(&FormErrorLabelFlush, &QLabel::staticMetaObject,
                [](const QWidget* w)
                {
                    return named(w, "formErrorLabel") &&
                        (within(w, "createAGemBody") || within(w, "addRemoteProjectDialog") || within(w, "addRemoteTemplateDialog"));
                });
            BoxElement::registerElement(&FormErrorLabel, &QLabel::staticMetaObject,
                [](const QWidget* w) { return named(w, "formErrorLabel"); });
            BoxElement::registerElement(&FormRow, &QWidget::staticMetaObject,
                [](const QWidget* w)
                {
                    return named(w, "formBrowseEditWidget") || named(w, "formComboBoxWidget") || named(w, "formLineEditWidget") ||
                        named(w, "formOptionsWidget");
                });
            BoxElement::registerElement(&FormTagFieldWizard, &QFrame::staticMetaObject,
                [](const QWidget* w) { return named(w, "formTagField") && within(w, "createAGemRHS"); });
            BoxElement::registerElement(&FormTagField, &QFrame::staticMetaObject,
                [](const QWidget* w) { return named(w, "formTagField"); });
            BoxElement::registerElement(&FormTagChip, &QCheckBox::staticMetaObject,
                [](const QWidget* w) { return within(w, "formTagField"); });
            BoxElement::registerElement(&FormTitle, &QLabel::staticMetaObject,
                [](const QWidget* w) { return named(w, "formTitleLabel"); });
            BoxElement::registerElement(&WizardStepRadio, &QRadioButton::staticMetaObject,
                [](const QWidget* w) { return within(w, "createAGemLHS"); });
            BoxElement::registerElement(&WizardRightPane, &QScrollArea::staticMetaObject,
                [](const QWidget* w) { return named(w, "createAGemRightPane"); });
            BoxElement::registerElement(&WizardSubheader, &QLabel::staticMetaObject,
                [](const QWidget* w) { return named(w, "rightPaneSubheader"); });
            BoxElement::registerElement(&WizardRadioSubtext, &QLabel::staticMetaObject,
                [](const QWidget* w) { return named(w, "createAGemRadioButtonSubtext"); });
            BoxElement::registerElement(&WizardBody, &QFrame::staticMetaObject,
                [](const QWidget* w) { return named(w, "createAGemBody"); });
            BoxElement::registerElement(&WizardFooter, &QFrame::staticMetaObject,
                [](const QWidget* w) { return named(w, "createAGemFooter"); });
            BoxElement::registerElement(&SettingsTabButton, &QPushButton::staticMetaObject,
                [](const QWidget* w) { return childOf(w, "projectSettingsTab") && hasProperty(w, "secondary", "true"); });
            BoxElement::registerElement(&TemplateButton, &QPushButton::staticMetaObject,
                [](const QWidget* w) { return named(w, "templateButton") && within(w, "projectTemplate"); });
            BoxElement::registerElement(&TemplateImageChecked, &QLabel::staticMetaObject,
                [](const QWidget* w)
                {
                    return named(w, "templateImage") && withinMatching(w, &QPushButton::staticMetaObject, [](const QWidget* button)
                        {
                            return hasProperty(button, "Checked", "true") && within(button, "projectTemplate");
                        });
                });
            BoxElement::registerElement(&TemplateImage, &QLabel::staticMetaObject,
                [](const QWidget* w)
                {
                    return named(w, "templateImage") && withinMatching(w, &QPushButton::staticMetaObject, [](const QWidget* button)
                        {
                            return within(button, "projectTemplate");
                        });
                });
            BoxElement::registerElement(&TemplateLabel, &QLabel::staticMetaObject,
                [](const QWidget* w) { return named(w, "templateLabel") && within(w, "projectTemplate"); });
            BoxElement::registerElement(&TemplateOverlay, &QLabel::staticMetaObject,
                [](const QWidget* w) { return named(w, "labelButtonOverlay"); });
            BoxElement::registerElement(&ProjectSettings, &QFrame::staticMetaObject,
                [](const QWidget* w) { return named(w, "projectSettings"); });
            BoxElement::registerElement(&ProjectTemplate, &QFrame::staticMetaObject,
                [](const QWidget* w) { return named(w, "projectTemplate"); });
            BoxElement::registerElement(&TemplateDetailsLabel, &QLabel::staticMetaObject,
                [](const QWidget* w) { return named(w, "projectTemplateDetailsLabel"); });
            BoxElement::registerElement(&TemplateDetails, &QFrame::staticMetaObject,
                [](const QWidget* w) { return named(w, "projectTemplateDetails"); });
            BoxElement::registerElement(&TemplateSummary, &QLabel::staticMetaObject,
                [](const QWidget* w) { return named(w, "summary") && within(w, "projectTemplateDetails"); });
            BoxElement::registerElement(&TemplateGemsTitle, &QLabel::staticMetaObject,
                [](const QWidget* w) { return named(w, "includedGemsTitle") && within(w, "projectTemplateDetails"); });
            BoxElement::registerElement(&TemplateMoreGems, &QLabel::staticMetaObject,
                [](const QWidget* w) { return named(w, "moreGems") && within(w, "projectTemplateDetails"); });
            BoxElement::registerElement(&TemplateBrowseCatalog, &QLabel::staticMetaObject,
                [](const QWidget* w) { return named(w, "browseCatalog") && within(w, "projectTemplateDetails"); });
            BoxElement::registerElement(&InspectorVersionButton, &QPushButton::staticMetaObject,
                [](const QWidget* w) { return within(w, "GemCatalogVersion") && hasProperty(w, "secondary", "true"); });
            BoxElement::registerElement(&InspectorButtonSecondary, &QPushButton::staticMetaObject,
                [](const QWidget* w) { return within(w, "GemCatalogInspector") && hasProperty(w, "secondary", "true"); });
            BoxElement::registerElement(&InspectorButtonPrimary, &QPushButton::staticMetaObject,
                [](const QWidget* w) { return within(w, "GemCatalogInspector") && hasProperty(w, "primary", "true"); });
            BoxElement::registerElement(&InspectorButtonDanger, &QPushButton::staticMetaObject,
                [](const QWidget* w) { return within(w, "GemCatalogInspector") && hasProperty(w, "danger", "true"); });
            BoxElement::registerElement(&RepoInspectorButtonDanger, &QPushButton::staticMetaObject,
                [](const QWidget* w) { return within(w, "gemRepoInspector") && hasProperty(w, "danger", "true"); });
            BoxElement::registerElement(&GemRepoAddButton, &QPushButton::staticMetaObject,
                [](const QWidget* w) { return named(w, "gemRepoAddButton") && hasProperty(w, "secondary", "true"); });
            BoxElement::registerElement(&InspectorNoticeInfo, &QLabel::staticMetaObject,
                [](const QWidget* w) { return named(w, "GemCatalogRequirements") && within(w, "GemCatalogInspector"); });
            BoxElement::registerElement(&InspectorNoticeWarning, &QLabel::staticMetaObject,
                [](const QWidget* w) { return named(w, "GemCatalogCompatibilityWarning") && within(w, "GemCatalogInspector"); });
            BoxElement::registerElement(&InspectorFieldName, &QLabel::staticMetaObject,
                [](const QWidget* w) { return within(w, "GemCatalogInspector") && hasProperty(w, "class", "label"); });
            BoxElement::registerElement(&InspectorTag, &QLabel::staticMetaObject,
                [](const QWidget* w)
                {
                    return named(w, "TagWidget") && (within(w, "gemRepoInspector") || within(w, "GemCatalogInspector"));
                });
            BoxElement::registerElement(&Tag, &QLabel::staticMetaObject,
                [](const QWidget* w) { return named(w, "TagWidget"); });
            BoxElement::registerElement(&InspectorLabel, &QLabel::staticMetaObject,
                [](const QWidget* w) { return within(w, "GemCatalogInspector") || within(w, "gemRepoInspector"); });
            BoxElement::registerElement(&Inspector, &QScrollArea::staticMetaObject,
                [](const QWidget* w)
                {
                    return named(w, "GemCatalogInspector") || named(w, "GemCatalogFilterWidget") || named(w, "gemRepoInspector");
                });
            BoxElement::registerElement(&InspectorTagContainer, &QWidget::staticMetaObject,
                [](const QWidget* w) { return named(w, "TagWidgetContainer") && within(w, "GemCatalogInspector"); });
            BoxElement::registerElement(&RepoInspectorTagContainer, &QWidget::staticMetaObject,
                [](const QWidget* w) { return named(w, "TagWidgetContainer") && within(w, "gemRepoInspector"); });
            BoxElement::registerElement(&FilterCount, &QLabel::staticMetaObject,
                [](const QWidget* w) { return named(w, "CountLabel") && within(w, "GemCatalogFilterWidget"); });
            BoxElement::registerElement(&CartCount, &QLabel::staticMetaObject,
                [](const QWidget* w) { return named(w, "GemCatalogCartCountLabel"); });
            BoxElement::registerElement(&GemCart, &QScrollArea::staticMetaObject,
                [](const QWidget* w) { return named(w, "GemCatalogCart"); });
            BoxElement::registerElement(&CartDownloadList, &QWidget::staticMetaObject,
                [](const QWidget* w) { return named(w, "GemCatalogCartOverlayGemDownloadBG"); });
            BoxElement::registerElement(&CartDownloadHeader, &QWidget::staticMetaObject,
                [](const QWidget* w) { return named(w, "GemCatalogCartOverlayGemDownloadHeader"); });
            BoxElement::registerElement(&GemList, &QListView::staticMetaObject,
                [](const QWidget* w) { return named(w, "GemCatalogListView"); });
            BoxElement::registerElement(&GemListCount, &QLabel::staticMetaObject,
                [](const QWidget* w) { return named(w, "GemCatalogHeaderShowCountLabel"); });
            BoxElement::registerElement(&GemListRefreshButton, &QPushButton::staticMetaObject,
                [](const QWidget* w) { return named(w, "RefreshButton"); });
            BoxElement::registerElement(&GemCatalogMenuButton, &QPushButton::staticMetaObject,
                [](const QWidget* w) { return named(w, "gemCatalogMenuButton"); });
            BoxElement::registerElement(&ExternalLink, &QLabel::staticMetaObject,
                [](const QWidget* w) { return named(w, "externalLink") && within(w, "ExternalLinkDialog"); });
            BoxElement::registerElement(&DependencyLabel, &QLabel::staticMetaObject,
                [](const QWidget* w) { return within(w, "GemDependenciesDialog"); });
            BoxElement::registerElement(&DependencyCheckBox, &QCheckBox::staticMetaObject,
                [](const QWidget* w) { return within(w, "GemDependenciesDialog"); });
            BoxElement::registerElement(&Toast, &QDialog::staticMetaObject,
                [](const QWidget* w) { return named(w, "ToastNotification"); });
            BoxElement::registerElement(&ToastIconFrame, &QFrame::staticMetaObject,
                [](const QWidget* w) { return named(w, "icon_frame") && within(w, "ToastNotification"); });
            BoxElement::registerElement(&ToastIcon, &QLabel::staticMetaObject,
                [](const QWidget* w) { return named(w, "iconLabel") && within(w, "ToastNotification"); });
        }

        // ------------------------------------------------------------------------------
        // Text: the fonts and colours the rules gave the widgets that draw text
        // ------------------------------------------------------------------------------

        // ---- Screen chrome: headers, footers and surfaces

        //! The title of a screen (#headerSubTitle, and the projects page's #titleLabel).
        const TextElement PageTitleText{
            .m_name = "ProjectManager.Text.PageTitle",
            // ProjectManager.qss:439 (#headerSubTitle), :677 (#projectsContent > QFrame > #titleLabel)
            .m_pixelSize = 24,
        };

        //! The lines under a title.
        const TextElement SubtitleText{
            .m_name = "ProjectManager.Text.Subtitle",
            // ProjectManager.qss:374 (#rightPaneSubheader), :430 (#headerTitle), :509 (#projectTemplateDetailsLabel), :527
            // (#projectTemplateDetails #moreGems), :618 (#firstTimeContent > #introLabel)
            .m_pixelSize = 14,
        };

        // ---- The start page

        //! The big title of the start page.
        const TextElement HeroTitleText{
            .m_name = "ProjectManager.Text.HeroTitle",
            .m_pixelSize = 60,                                // ProjectManager.qss:612 (#firstTimeContent > #titleLabel)
        };

        //! The label of a start card.
        const TextElement StartCardLabelText{
            .m_name = "ProjectManager.Text.StartCardLabel",
            .m_pixelSize = 14,                                // ProjectManager.qss:630 (#firstTimeContent > QPushButton)
        };

        // ---- The projects page and its cards

        //! The name of this engine on a project card (#projectButton QLabel#thisEngineLabel).
        const TextElement ProjectCardThisEngineText{
            .m_name = "ProjectManager.Text.ProjectCardThisEngine",
            .m_pixelSize = 12,                                // ProjectManager.qss:717 (#projectButton QLabel#thisEngineLabel)
            // TOKENIZE: ProjectCardThisEngineText; ProjectManager.qss:718 (#projectButton QLabel#thisEngineLabel)
            .m_color = QColor(0xEE, 0xEE, 0xEE),
        };

        //! The name of another engine on a project card, and the repository inspector's body text.
        const TextElement ProjectCardOtherEngineText{
            .m_name = "ProjectManager.Text.ProjectCardOtherEngine",
            // ProjectManager.qss:717 (#projectButton QLabel#otherEngineLabel), :1109 (#gemRepoInspectorBodyLabel)
            .m_pixelSize = 12,
            // TOKENIZE: ProjectCardOtherEngineText; ProjectManager.qss:722 (#projectButton QLabel#otherEngineLabel), :1110
            // (#gemRepoInspectorBodyLabel)
            .m_color = QColor(0xDD, 0xDD, 0xDD),
        };

        //! The message over a project card (#labelButton #projectMessageOverlay). UNVERIFIED.
        const TextElement ProjectCardMessageText{
            .m_name = "ProjectManager.Text.ProjectCardMessage",
            .m_pixelSize = 16,                                // ProjectManager.qss:749 (#labelButton #projectMessageOverlay)
            .m_weight = 700,                                  // ProjectManager.qss:708 (#projectButton QLabel)
        };

        //! The second message over a project card. UNVERIFIED.
        const TextElement ProjectCardSubMessageText{
            .m_name = "ProjectManager.Text.ProjectCardSubMessage",
            .m_pixelSize = 12,                                // ProjectManager.qss:753 (#labelButton #projectSubMessageOverlay)
            .m_weight = 700,                                  // ProjectManager.qss:708 (#projectButton QLabel)
        };

        //! A label of a project card (#projectButton QLabel): bold.
        const TextElement ProjectCardLabelText{
            .m_name = "ProjectManager.Text.ProjectCardLabel",
            .m_pixelSize = 14,                                // ProjectManager.qss:709 (#projectButton QLabel)
            .m_weight = 700,                                  // ProjectManager.qss:708 (#projectButton QLabel)
        };

        // ---- Form fields

        //! The label in a form field (#formFrame QLabel).
        const TextElement FormLabelText{
            .m_name = "ProjectManager.Text.FormLabel",
            .m_pixelSize = 13,                                // ProjectManager.qss:267 (#formFrame QLabel)
            .m_color = QColor(0xCC, 0xCC, 0xCC),              // TOKENIZE: FormLabelText; ProjectManager.qss:268 (#formFrame QLabel)
            // TOKENIZE: FormLabelDisabledText; ProjectManager.qss:272 (#formFrame QLabel:disabled)
            .m_disabledColor = QColor(0x88, 0x88, 0x88),
        };

        //! The label over the wizard's options (#formOptionsFrame QLabel).
        const TextElement FormOptionsLabelText{
            .m_name = "ProjectManager.Text.FormOptionsLabel",
            .m_pixelSize = 13,                                // ProjectManager.qss:267 (#formOptionsFrame QLabel)
            // TOKENIZE: FormOptionsLabelText; ProjectManager.qss:268 (#formOptionsFrame QLabel)
            .m_color = QColor(0xCC, 0xCC, 0xCC),
        };

        //! The error under a form field (#formErrorLabel). UNVERIFIED.
        const TextElement FormErrorLabelText{
            .m_name = "ProjectManager.Text.FormErrorLabel",
            .m_pixelSize = 14,                                // ProjectManager.qss:342 (#formErrorLabel)
            .m_color = QColor(0xEC, 0x30, 0x30),              // TOKENIZE: FormErrorLabelText; ProjectManager.qss:341 (#formErrorLabel)
        };

        //! The header of a pane or a form.
        const TextElement PaneHeaderText{
            .m_name = "ProjectManager.Text.PaneHeader",
            // ProjectManager.qss:359 (#formTitleLabel), :365 (#rightPaneHeader), :370 (#rightPaneDetailsHeader)
            .m_pixelSize = 21,
        };

        //! The text of a form entry (#formFrame QLineEdit).
        const TextElement FormLineEditText{
            .m_name = "ProjectManager.Text.FormLineEdit",
            .m_pixelSize = 18,                                // ProjectManager.qss:301 (#formFrame QLineEdit)
            .m_color = QColor(0xFF, 0xFF, 0xFF),              // TOKENIZE: FormLineEditText; ProjectManager.qss:302 (#formFrame QLineEdit)
            // TOKENIZE: FormLineEditDisabledText; ProjectManager.qss:310 (#formFrame QLineEdit:disabled)
            .m_disabledColor = QColor(0xAA, 0xAA, 0xAA),
        };

        //! The text of a form combo box (#formFrame QComboBox).
        const TextElement FormComboBoxText{
            .m_name = "ProjectManager.Text.FormComboBox",
            .m_pixelSize = 18,                                // ProjectManager.qss:319 (#formFrame QComboBox)
        };

        //! A check box of the wizard's options.
        const TextElement FormOptionsCheckBoxText{
            .m_name = "ProjectManager.Text.FormOptionsCheckBox",
            .m_pixelSize = 16,                                // ProjectManager.qss:177 (#formOptionsWidget QCheckBox)
        };

        //! A tag chip of the tags field (#formTagField QCheckBox): light blue, disabled too: the window's sheet outranks the
        //! application's whatever the specificity (qstylesheetstyle.cpp:1689, qcssparser.cpp:2220-2222), so the grey of CheckBox.qss:19
        //! (QCheckBox:disabled) does not reach it. UNVERIFIED: no capture shows one.
        const TextElement FormTagChipText{
            .m_name = "ProjectManager.Text.FormTagChip",
            .m_color = QColor(0x94, 0xD2, 0xFF),              // TOKENIZE: FormTagChipText; ProjectManager.qss:239 (#formTagField QCheckBox)
        };

        // ---- The create-a-gem wizard

        //! A step of the wizard's list: grey, white while checked.
        const TextElement WizardStepRadioText{
            .m_name = "ProjectManager.Text.WizardStepRadio",
            .m_pixelSize = 16,                                // ProjectManager.qss:80 (#createAGemLHS QRadioButton)
            // TOKENIZE: WizardStepRadioText; ProjectManager.qss:88 (#createAGemLHS QRadioButton:enabled)
            .m_color = QColor(0xAA, 0xAA, 0xAA),
            // TOKENIZE: WizardStepRadioDisabledText; ProjectManager.qss:84 (#createAGemLHS QRadioButton)
            .m_disabledColor = QColor(0x6B, 0x6B, 0x6B),
        };

        // ---- Project settings and templates

        //! The small note under a project's preview (#projectSmallInfoLabel).
        const TextElement SmallInfoText{
            .m_name = "ProjectManager.Text.SmallInfo",
            .m_pixelSize = 10,                                // ProjectManager.qss:582 (#projectSmallInfoLabel)
        };

        //! The label over the template chooser (#projectTemplateLabel): thin.
        const TextElement ProjectTemplateLabelText{
            .m_name = "ProjectManager.Text.ProjectTemplateLabel",
            .m_pixelSize = 16,                                // ProjectManager.qss:504 (#projectTemplateLabel)
            .m_weight = 100,                                  // ProjectManager.qss:505 (#projectTemplateLabel)
        };

        //! The name of the checked template: bold. UNVERIFIED.
        const TextElement TemplateLabelCheckedText{
            .m_name = "ProjectManager.Text.TemplateLabelChecked",
            .m_pixelSize = 12,                                // Text.qss:92 (QLabel)
            // ProjectManager.qss:565 (#projectTemplate QPushButton[Checked="true"] #templateLabel)
            .m_weight = 700,
        };

        //! The label of the settings tab button.
        const TextElement SettingsTabButtonText{
            .m_name = "ProjectManager.Text.SettingsTabButton",
            .m_pixelSize = 13,                                // ProjectManager.qss:603 (#projectSettingsTab::tab-bar > QPushButton)
        };

        // ---- The gem catalog and the gem repositories

        //! The count beside a filter.
        const TextElement FilterCountText{
            .m_name = "ProjectManager.Text.FilterCount",
            .m_pixelSize = 12,                                // ProjectManager.qss:998 (#GemCatalogFilterWidget #CountLabel)
            // TOKENIZE: FilterCountText; ProjectManager.qss:1001 (#GemCatalogFilterWidget #CountLabel)
            .m_color = QColor(0x94, 0xD2, 0xFF),
        };

        //! The title of a filter category (#GemCatalogFilterCategoryTitle).
        const TextElement FilterCategoryTitleText{
            .m_name = "ProjectManager.Text.FilterCategoryTitle",
            .m_pixelSize = 12,                                // ProjectManager.qss:985 (#GemCatalogFilterCategoryTitle)
            .m_weight = 600,                                  // ProjectManager.qss:986 (#GemCatalogFilterCategoryTitle)
        };

        //! The text of a filter tag (#FilterTagWidgetTextLabel). UNVERIFIED.
        const TextElement FilterTagTextText{
            .m_name = "ProjectManager.Text.FilterTagText",
            .m_pixelSize = 10,                                // ProjectManager.qss:916 (#FilterTagWidgetTextLabel)
            // TOKENIZE: FilterTagTextText; ProjectManager.qss:915 (#FilterTagWidgetTextLabel)
            .m_color = QColor(0x94, 0xD2, 0xFF),
        };

        //! The count over the gem list: italic.
        const TextElement GemListCountText{
            .m_name = "ProjectManager.Text.GemListCount",
            .m_pixelSize = 12,                                // ProjectManager.qss:877 (#GemCatalogHeaderShowCountLabel)
            .m_italic = 1,                                    // ProjectManager.qss:878 (#GemCatalogHeaderShowCountLabel)
        };

        //! The notices of the gem inspector. UNVERIFIED.
        const TextElement InspectorNoticeText{
            .m_name = "ProjectManager.Text.InspectorNotice",
            // ProjectManager.qss:851 (#GemCatalogInspector #GemCatalogCompatibilityWarning), :851 (#GemCatalogInspector
            // #GemCatalogRequirements)
            .m_pixelSize = 10,
            // TOKENIZE: InspectorNoticeText; ProjectManager.qss:852 (#GemCatalogInspector #GemCatalogCompatibilityWarning), :852
            // (#GemCatalogInspector #GemCatalogRequirements)
            .m_color = QColor(0xDD, 0xDD, 0xDD),
        };

        //! The small text of a gem sub-widget (#gemSubWidgetTextLabel). UNVERIFIED.
        const TextElement InspectorSubTextText{
            .m_name = "ProjectManager.Text.InspectorSubText",
            .m_pixelSize = 10,                                // ProjectManager.qss:936 (#gemSubWidgetTextLabel)
            // TOKENIZE: InspectorSubTextText; ProjectManager.qss:935 (#gemSubWidgetTextLabel)
            .m_color = QColor(0xDD, 0xDD, 0xDD),
        };

        //! A section title of the cart (#GemCatalogCartOverlaySectionLabel). UNVERIFIED.
        const TextElement CartSectionLabelText{
            .m_name = "ProjectManager.Text.CartSectionLabel",
            .m_pixelSize = 12,                                // Text.qss:92 (QLabel)
            .m_weight = 600,                                  // ProjectManager.qss:821 (#GemCatalogCartOverlaySectionLabel)
        };

        //! The title of an item or a section.
        const TextElement ItemTitleText{
            .m_name = "ProjectManager.Text.ItemTitle",
            // ProjectManager.qss:523 (#projectTemplateDetails #displayName), :523 (#projectTemplateDetails #includedGemsTitle), :577
            // (#projectSettingsSectionTitle), :807 (#GemCatalogTitle), :1104 (#gemRepoInspectorNameLabel)
            .m_pixelSize = 18,
        };

        //! The title of a group in an inspector.
        const TextElement SectionTitleText{
            .m_name = "ProjectManager.Text.SectionTitle",
            // ProjectManager.qss:931 (#gemSubWidgetTitleLabel), :961 (#GemCatalogInspector QLabel[class="title"]), :1047
            // (#gemRepoNoReposLabel), :1114 (#gemRepoInspectorAddInfoTitleLabel)
            .m_pixelSize = 16,
        };

        //! The buttons of the gem pages: semi-bold.
        const TextElement GemRepoAddButtonText{
            .m_name = "ProjectManager.Text.GemRepoAddButton",
            // ProjectManager.qss:907 (#GemCatalogInspector QPushButton), :1063 (#gemRepoAddButton), :1124 (#gemRepoInspector QPushButton)
            .m_pixelSize = 12,
            // ProjectManager.qss:908 (#GemCatalogInspector QPushButton), :1064 (#gemRepoAddButton), :1125 (#gemRepoInspector QPushButton)
            .m_weight = 600,
        };

        // ---- Dialogs and the toast

        //! The sub-title of a dialog (#dialogSubTitle). UNVERIFIED.
        const TextElement DialogSubtitleText{
            .m_name = "ProjectManager.Text.DialogSubtitle",
            .m_pixelSize = 14,                                // ProjectManager.qss:477 (#dialogSubTitle)
            .m_weight = 600,                                  // ProjectManager.qss:478 (#dialogSubTitle)
        };

        //! The address in the external link dialog. UNVERIFIED.
        const TextElement ExternalLinkText{
            .m_name = "ProjectManager.Text.ExternalLink",
            .m_pixelSize = 12,                                // ProjectManager.qss:492 (#ExternalLinkDialog #externalLink)
            // TOKENIZE: ExternalLinkText; ProjectManager.qss:490 (#ExternalLinkDialog #externalLink)
            .m_color = QColor(0x94, 0xD2, 0xFF),
        };

        //! The instruction titles of the remote project dialog. UNVERIFIED.
        const TextElement RemoteInstructionText{
            .m_name = "ProjectManager.Text.RemoteInstruction",
            // ProjectManager.qss:786 (#remoteProjectDialogDownloadProjectLabel), :786 (#remoteProjectDialogInstructionTitleLabel)
            .m_pixelSize = 14,
            // ProjectManager.qss:787 (#remoteProjectDialogDownloadProjectLabel), :787 (#remoteProjectDialogInstructionTitleLabel)
            .m_weight = 600,
            // TOKENIZE: RemoteInstructionDisabledText; ProjectManager.qss:797 (#addRemoteProjectDialog > QLabel:disabled)
            .m_disabledColor = QColor(0xAA, 0xAA, 0xAA),
        };

        //! The section titles of the remote project dialog: light. UNVERIFIED.
        const TextElement RemoteSectionText{
            .m_name = "ProjectManager.Text.RemoteSection",
            // ProjectManager.qss:792 (#remoteProjectDialogLicensesTitleLabel), :792 (#remoteProjectDialogRequirementsTitleLabel)
            .m_pixelSize = 16,
            // ProjectManager.qss:793 (#remoteProjectDialogLicensesTitleLabel), :793 (#remoteProjectDialogRequirementsTitleLabel)
            .m_weight = 300,
            // TOKENIZE: RemoteSectionDisabledText; ProjectManager.qss:797 (#addRemoteProjectDialog > QLabel:disabled)
            .m_disabledColor = QColor(0xAA, 0xAA, 0xAA),
        };

        //! Any other label of the remote project dialog: grey while disabled (#addRemoteProjectDialog > QLabel:disabled). UNVERIFIED.
        const TextElement RemoteLabelText{
            .m_name = "ProjectManager.Text.RemoteLabel",
            .m_pixelSize = 12,                                // Text.qss:92 (QLabel)
            // TOKENIZE: RemoteLabelDisabledText; ProjectManager.qss:797 (#addRemoteProjectDialog > QLabel:disabled)
            .m_disabledColor = QColor(0xAA, 0xAA, 0xAA),
        };

        //! The instruction of the add repository dialog. UNVERIFIED.
        const TextElement RepoInstructionText{
            .m_name = "ProjectManager.Text.RepoInstruction",
            .m_pixelSize = 14,                                // ProjectManager.qss:1068 (#gemRepoAddDialogInstructionTitleLabel)
            .m_weight = 700,                                  // ProjectManager.qss:1069 (#gemRepoAddDialogInstructionTitleLabel)
        };

        //! The warning of the add repository dialog: italic. UNVERIFIED.
        const TextElement RepoWarningText{
            .m_name = "ProjectManager.Text.RepoWarning",
            .m_pixelSize = 12,                                // ProjectManager.qss:1073 (#gemRepoAddDialogWarningLabel)
            .m_italic = 1,                                    // ProjectManager.qss:1074 (#gemRepoAddDialogWarningLabel)
        };

        // ---- The tabs

        //! The tabs of the engine page (#engineTabBar::tab).
        const TextElement TabBarEngineText{
            .m_name = "ProjectManager.Text.TabBarEngine",
            .m_pixelSize = 18,                                // ProjectManager.qss:1019 (#engineTabBar::tab)
        };

        //! The big tabs (QTabBar::tab, #projectSettingsTabBar::tab).
        const TextElement TabBarText{
            .m_name = "ProjectManager.Text.TabBar",
            .m_pixelSize = 24,                                // ProjectManager.qss:70 (QTabBar::tab)
        };

        void registerTexts()
        {
            TextElement::registerElement(&PageTitleText, &QLabel::staticMetaObject,
                [](const QWidget* w)
                {
                    return named(w, "headerSubTitle") || (named(w, "titleLabel") && parentChildOf(w, "projectsContent"));
                });
            TextElement::registerElement(&SubtitleText, &QLabel::staticMetaObject,
                [](const QWidget* w)
                {
                    return named(w, "headerTitle") || named(w, "projectTemplateDetailsLabel") || named(w, "rightPaneSubheader") ||
                        (named(w, "introLabel") && childOf(w, "firstTimeContent")) ||
                        (named(w, "moreGems") && within(w, "projectTemplateDetails"));
                });
            TextElement::registerElement(&HeroTitleText, &QLabel::staticMetaObject,
                [](const QWidget* w) { return childOf(w, "firstTimeContent") && named(w, "titleLabel"); });
            TextElement::registerElement(&StartCardLabelText, &QPushButton::staticMetaObject,
                [](const QWidget* w) { return childOf(w, "firstTimeContent"); });
            TextElement::registerElement(&ProjectCardThisEngineText, &QLabel::staticMetaObject,
                [](const QWidget* w) { return named(w, "thisEngineLabel") && within(w, "projectButton"); });
            TextElement::registerElement(&ProjectCardOtherEngineText, &QLabel::staticMetaObject,
                [](const QWidget* w)
                {
                    return (named(w, "otherEngineLabel") && within(w, "projectButton")) || named(w, "gemRepoInspectorBodyLabel");
                });
            TextElement::registerElement(&ProjectCardMessageText, &QLabel::staticMetaObject,
                [](const QWidget* w) { return named(w, "projectMessageOverlay") && within(w, "labelButton"); });
            TextElement::registerElement(&ProjectCardSubMessageText, &QLabel::staticMetaObject,
                [](const QWidget* w) { return named(w, "projectSubMessageOverlay") && within(w, "labelButton"); });
            TextElement::registerElement(&ProjectCardLabelText, &QLabel::staticMetaObject,
                [](const QWidget* w) { return within(w, "projectButton"); });
            TextElement::registerElement(&FormLabelText, &QLabel::staticMetaObject,
                [](const QWidget* w) { return within(w, "formFrame"); });
            TextElement::registerElement(&FormOptionsLabelText, &QLabel::staticMetaObject,
                [](const QWidget* w) { return within(w, "formOptionsFrame"); });
            TextElement::registerElement(&FormErrorLabelText, &QLabel::staticMetaObject,
                [](const QWidget* w) { return named(w, "formErrorLabel"); });
            TextElement::registerElement(&PaneHeaderText, &QLabel::staticMetaObject,
                [](const QWidget* w)
                {
                    return named(w, "formTitleLabel") || named(w, "rightPaneDetailsHeader") || named(w, "rightPaneHeader");
                });
            TextElement::registerElement(&FormLineEditText, &QLineEdit::staticMetaObject,
                [](const QWidget* w) { return within(w, "formFrame"); });
            TextElement::registerElement(&FormComboBoxText, &QComboBox::staticMetaObject,
                [](const QWidget* w) { return within(w, "formFrame"); });
            TextElement::registerElement(&FormOptionsCheckBoxText, &QCheckBox::staticMetaObject,
                [](const QWidget* w) { return within(w, "formOptionsWidget"); });
            TextElement::registerElement(&FormTagChipText, &QCheckBox::staticMetaObject,
                [](const QWidget* w) { return within(w, "formTagField"); });
            TextElement::registerElement(&WizardStepRadioText, &QRadioButton::staticMetaObject,
                [](const QWidget* w) { return within(w, "createAGemLHS"); });
            TextElement::registerElement(&SmallInfoText, &QLabel::staticMetaObject,
                [](const QWidget* w) { return named(w, "projectSmallInfoLabel"); });
            TextElement::registerElement(&ProjectTemplateLabelText, &QLabel::staticMetaObject,
                [](const QWidget* w) { return named(w, "projectTemplateLabel"); });
            TextElement::registerElement(&TemplateLabelCheckedText, &QLabel::staticMetaObject,
                [](const QWidget* w)
                {
                    return named(w, "templateLabel") && withinMatching(w, &QPushButton::staticMetaObject, [](const QWidget* button)
                        {
                            return hasProperty(button, "Checked", "true") && within(button, "projectTemplate");
                        });
                });
            TextElement::registerElement(&SettingsTabButtonText, &QPushButton::staticMetaObject,
                [](const QWidget* w) { return childOf(w, "projectSettingsTab"); });
            TextElement::registerElement(&FilterCountText, &QLabel::staticMetaObject,
                [](const QWidget* w) { return named(w, "CountLabel") && within(w, "GemCatalogFilterWidget"); });
            TextElement::registerElement(&FilterCategoryTitleText, &QLabel::staticMetaObject,
                [](const QWidget* w) { return named(w, "GemCatalogFilterCategoryTitle"); });
            TextElement::registerElement(&FilterTagTextText, &QLabel::staticMetaObject,
                [](const QWidget* w) { return named(w, "FilterTagWidgetTextLabel"); });
            TextElement::registerElement(&GemListCountText, &QLabel::staticMetaObject,
                [](const QWidget* w) { return named(w, "GemCatalogHeaderShowCountLabel"); });
            TextElement::registerElement(&InspectorNoticeText, &QLabel::staticMetaObject,
                [](const QWidget* w)
                {
                    return (named(w, "GemCatalogRequirements") || named(w, "GemCatalogCompatibilityWarning")) &&
                        within(w, "GemCatalogInspector");
                });
            TextElement::registerElement(&InspectorSubTextText, &QLabel::staticMetaObject,
                [](const QWidget* w) { return named(w, "gemSubWidgetTextLabel"); });
            TextElement::registerElement(&CartSectionLabelText, &QLabel::staticMetaObject,
                [](const QWidget* w) { return named(w, "GemCatalogCartOverlaySectionLabel"); });
            TextElement::registerElement(&ItemTitleText, &QLabel::staticMetaObject,
                [](const QWidget* w)
                {
                    return named(w, "GemCatalogTitle") || named(w, "gemRepoInspectorNameLabel") ||
                        named(w, "projectSettingsSectionTitle") ||
                        ((named(w, "displayName") || named(w, "includedGemsTitle")) && within(w, "projectTemplateDetails"));
                });
            TextElement::registerElement(&SectionTitleText, &QLabel::staticMetaObject,
                [](const QWidget* w)
                {
                    return named(w, "gemRepoInspectorAddInfoTitleLabel") || named(w, "gemRepoNoReposLabel") ||
                        named(w, "gemSubWidgetTitleLabel") || (hasProperty(w, "class", "title") && within(w, "GemCatalogInspector"));
                });
            TextElement::registerElement(&GemRepoAddButtonText, &QPushButton::staticMetaObject,
                [](const QWidget* w)
                {
                    return named(w, "gemRepoAddButton") || within(w, "gemRepoInspector") || within(w, "GemCatalogInspector");
                });
            TextElement::registerElement(&DialogSubtitleText, &QLabel::staticMetaObject,
                [](const QWidget* w) { return named(w, "dialogSubTitle"); });
            TextElement::registerElement(&ExternalLinkText, &QLabel::staticMetaObject,
                [](const QWidget* w) { return named(w, "externalLink") && within(w, "ExternalLinkDialog"); });
            TextElement::registerElement(&RemoteInstructionText, &QLabel::staticMetaObject,
                [](const QWidget* w)
                {
                    return named(w, "remoteProjectDialogInstructionTitleLabel") || named(w, "remoteProjectDialogDownloadProjectLabel");
                });
            TextElement::registerElement(&RemoteSectionText, &QLabel::staticMetaObject,
                [](const QWidget* w)
                {
                    return named(w, "remoteProjectDialogRequirementsTitleLabel") || named(w, "remoteProjectDialogLicensesTitleLabel");
                });
            TextElement::registerElement(&RemoteLabelText, &QLabel::staticMetaObject,
                [](const QWidget* w) { return childOf(w, "addRemoteProjectDialog"); });
            TextElement::registerElement(&RepoInstructionText, &QLabel::staticMetaObject,
                [](const QWidget* w) { return named(w, "gemRepoAddDialogInstructionTitleLabel"); });
            TextElement::registerElement(&RepoWarningText, &QLabel::staticMetaObject,
                [](const QWidget* w) { return named(w, "gemRepoAddDialogWarningLabel"); });
            TextElement::registerElement(&TabBarEngineText, &QTabBar::staticMetaObject,
                [](const QWidget* w) { return named(w, "engineTabBar"); });
            TextElement::registerElement(&TabBarText, &QTabBar::staticMetaObject,
                [](const QWidget*) { return true; });
        }

        // ------------------------------------------------------------------------------
        // Tabs: the tabs of the window (QTabBar::tab, ProjectManager.qss:61-143), of the project settings (:585-591) and of the
        // engine page (:1011-1034). The bars are QTabBar, not AzQtComponents' TabBar. The size of a tab's font is its bar's: a row
        // of the Text section (TabBar, TabBarEngine). The bar paints nothing of its own (QTabBar { background-color: transparent },
        // :61-63), so no element carries a bar fill, and the pane has no border (:120), so none carries a pane edge.
        // ------------------------------------------------------------------------------

        //! `border-bottom: 3px solid <color>`: the underline of a tab
        TabElement::Edges underline(const QColor& color)
        {
            return { {}, {}, { 3, color }, {} };
        }

        const QColor TabFocusFill(0x52, 0x52, 0x52);                // TOKENIZE: TabFocusFill; ProjectManager.qss:141
        const QColor TabFocusText(0x40, 0x82, 0xEB);                // TOKENIZE: TabFocusText; ProjectManager.qss:142
        const QColor EngineTabPressedText(0x66, 0xBC, 0xFA);        // TOKENIZE: EngineTabPressedText; ProjectManager.qss:1033

        //! QTabBar::tab: the tabs of the window (Projects, Gems, Engine)
        //! UNVERIFIED states: hover, pressed, focus (no capture shows a tab in them)
        const TabElement HeaderTab{
            .m_name = "ProjectManager.Tab.Header",
            // ProjectManager.qss:69,73 (QTabBar::tab); the text colour is BaseStyleSheet.qss:18
            .m_idle = { QColor(), White, underline(Transparent) },
            // ProjectManager.qss:129,130 (QTabBar::tab:hover); the edge is :73
            .m_hover = { QColor(), AccentBlue, underline(Transparent), 500 },
            // ProjectManager.qss:123-126 (QTabBar::tab:selected)
            .m_selected = { QColor(), AccentBlue, underline(AccentBlue), 500 },
            // selected :123-126, over hover :129,130
            .m_selectedHover = { QColor(), AccentBlue, underline(AccentBlue), 500 },
            // ProjectManager.qss:133 (QTabBar::tab:pressed), over selected :124-126
            .m_pressed = { QColor(), AccentBluePressed, underline(AccentBlue), 500 },
            // ProjectManager.qss:141,142 (QTabBar::tab:focus), over selected :124-126
            .m_focus = { TabFocusFill, TabFocusText, underline(AccentBlue), 500 },
            // ProjectManager.qss:72 (QTabBar::tab margin-right)
            .m_margin = QMargins(0, 0, 40, 0),
            // ProjectManager.qss:71 (QTabBar::tab min-width)
            .m_minContentWidth = 100,
            // TabWidget.qss:18 (QTabBar::tab max-width)
            .m_maxContentWidth = 200,
            // ProjectManager.qss:68 (QTabBar::tab height)
            .m_contentHeight = 82,
            // ProjectManager.qss:136-138 (QTabBar::focus outline)
            .m_hideFocusRect = true,
            // ProjectManager.qss:119 (QTabWidget::pane)
            .m_paneFill = PageFill,
            // ProjectManager.qss:65 (QTabWidget::tab-bar left)
            .m_barLeft = 78,
        };

        //! #projectSettingsTabBar::tab: the same tabs, 50px tall, the bar 60px in
        //! UNVERIFIED states: hover, pressed, focus (no capture shows a tab in them)
        const TabElement SettingsTab{
            .m_name = "ProjectManager.Tab.Settings",
            // ProjectManager.qss:69,73 (QTabBar::tab); the text colour is BaseStyleSheet.qss:18
            .m_idle = { QColor(), White, underline(Transparent) },
            // ProjectManager.qss:129,130 (QTabBar::tab:hover); the edge is :73
            .m_hover = { QColor(), AccentBlue, underline(Transparent), 500 },
            // ProjectManager.qss:123-126 (QTabBar::tab:selected)
            .m_selected = { QColor(), AccentBlue, underline(AccentBlue), 500 },
            // selected :123-126, over hover :129,130
            .m_selectedHover = { QColor(), AccentBlue, underline(AccentBlue), 500 },
            // ProjectManager.qss:133 (QTabBar::tab:pressed), over selected :124-126
            .m_pressed = { QColor(), AccentBluePressed, underline(AccentBlue), 500 },
            // ProjectManager.qss:141,142 (QTabBar::tab:focus), over selected :124-126
            .m_focus = { TabFocusFill, TabFocusText, underline(AccentBlue), 500 },
            // ProjectManager.qss:72 (QTabBar::tab margin-right)
            .m_margin = QMargins(0, 0, 40, 0),
            // ProjectManager.qss:71 (QTabBar::tab min-width)
            .m_minContentWidth = 100,
            // TabWidget.qss:18 (QTabBar::tab max-width)
            .m_maxContentWidth = 200,
            // ProjectManager.qss:590 (#projectSettingsTabBar::tab height)
            .m_contentHeight = 50,
            // ProjectManager.qss:136-138 (QTabBar::focus outline)
            .m_hideFocusRect = true,
            // ProjectManager.qss:119 (QTabWidget::pane)
            .m_paneFill = PageFill,
            // ProjectManager.qss:586 (#projectSettingsTab::tab-bar left)
            .m_barLeft = 60,
        };

        //! #engineTabBar::tab: the sub-tabs of the engine page, in light blue, with no focus face
        //! UNVERIFIED states: hover, pressed (no capture shows a tab in them)
        const TabElement EngineTab{
            .m_name = "ProjectManager.Tab.Engine",
            // ProjectManager.qss:73 (QTabBar::tab), :1018 (#engineTabBar::tab font-weight)
            .m_idle = { QColor(), White, underline(Transparent), 400 },
            // ProjectManager.qss:1029,1030 (#engineTabBar::tab:hover); the edge is :73
            .m_hover = { QColor(), LinkBlue, underline(Transparent), 600 },
            // ProjectManager.qss:1024-1026 (#engineTabBar::tab:selected)
            .m_selected = { QColor(), LinkBlue, underline(LinkBlue), 600 },
            // selected :1024-1026, over hover :1029,1030
            .m_selectedHover = { QColor(), LinkBlue, underline(LinkBlue), 600 },
            // ProjectManager.qss:1033 (#engineTabBar::tab:pressed), over selected :1024-1026
            .m_pressed = { QColor(), EngineTabPressedText, underline(LinkBlue), 600 },
            // ProjectManager.qss:72 (QTabBar::tab margin-right)
            .m_margin = QMargins(0, 0, 40, 0),
            // ProjectManager.qss:1020 (#engineTabBar::tab min-width)
            .m_minContentWidth = 160,
            // TabWidget.qss:18 (QTabBar::tab max-width)
            .m_maxContentWidth = 200,
            // ProjectManager.qss:1016 (#engineTabBar::tab height)
            .m_contentHeight = 50,
            // ProjectManager.qss:136-138 (QTabBar::focus outline)
            .m_hideFocusRect = true,
            // ProjectManager.qss:119 (QTabWidget::pane)
            .m_paneFill = PageFill,
            // ProjectManager.qss:1012 (#engineTab::tab-bar left)
            .m_barLeft = 60,
        };

        void registerTabs()
        {
            // The engine and settings bars are named by the code (EngineScreenCtrl.cpp:34, UpdateProjectCtrl.cpp:65); every other
            // tab bar is the sheet's QTabBar (the window's own)
            const QMetaObject* tabBar = &QTabBar::staticMetaObject;
            TabElement::registerElement(&EngineTab, tabBar, [](const QWidget* bar) { return named(bar, "engineTabBar"); });
            TabElement::registerElement(&SettingsTab, tabBar, [](const QWidget* bar) { return named(bar, "projectSettingsTabBar"); });
            TabElement::registerElement(&HeaderTab, tabBar, [](const QWidget*) { return true; });
        }

    } // namespace FlatElementsInternal

    // ----------------------------------------------------------------------------------
    // Registration
    // ----------------------------------------------------------------------------------

    void RegisterFlatElements()
    {
        // A registration lives for the whole run, and the registries do not look for repeats
        static bool registered = false;
        if (registered)
        {
            return;
        }
        registered = true;

        FlatElementsInternal::registerFrames();
        FlatElementsInternal::registerBoxes();
        FlatElementsInternal::registerIndicators();
        FlatElementsInternal::registerItemViews();
        FlatElementsInternal::registerTexts();
        FlatElementsInternal::registerTabs();
    }
} // namespace O3DE::ProjectManager
