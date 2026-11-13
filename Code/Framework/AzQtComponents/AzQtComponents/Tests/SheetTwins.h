/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

// ----------------------------------------------------------------------------------
// SheetTwins: a table of sites, each judged by what its style sheet drew in the legacy GUI. One row per site; one function runs a row.
//
// A row says what the site is (the widget, the parents its selectors need), what the legacy GUI had on it (the sheets, each quoted
// with its file:line and the place it is set), which states to drive, and what to compare. The harness builds the site twice and
// reports every difference by row:
//   - the legacy twin: the widget built at the Qss rung, under Qt's own style sheet style over Fusion (or over
//     AzQtComponents::Style, where the base style matters: Base::AzQtComponentsStyle), with the row's sheets set where the real GUI
//     sets them;
//   - the flat twin: the same widget built at the Flat rung, under AzQtComponents::Style over Fusion, with the Global elements, the
//     module's registrar (run once for the process) and the flat palette and class fonts on the application. No sheet is set in it,
//     and a widget of it that still has one is a difference: that is how the census would list the site.
// Each twin is built, driven through the states, read and gone before the next is built, so each lives at the rung of its GUI.
//
// Header only, with no fixture of its own: the modules' tests that twin a sheet include it (AzQtComponents, AzToolsFramework, the
// Editor, the gems' editor tests) and link nothing beyond AzQtComponents and Qt. Everything lives in the namespace SheetTwins, so
// that a unity build joining this file with other test files finds no second definition. It builds on FieldTwins.h (the real
// application sheet, the image comparisons) and ToolTipTwins.h (the tip of a widget).
//
// A test holds one Context (it makes the QApplication when the test has none, sets the application font, palette and style the
// engine has, makes the flattened Style and registers the Global elements: it changes the application as FieldTwins::Fixture does,
// so a test process that also tests the legacy GUI makes it in the fixtures of the twin tests only), and runs rows through check():
//
//     class MyTests : public ::UnitTest::LeakDetectionFixture
//     {
//     protected:
//         SheetTwins::Context m_context;
//     };
//
//     TEST_F(MyTests, Chip_IsTheChipOfTheSheet)
//     {
//         using namespace SheetTwins;
//         const Row row{
//             .m_site = "C000 the chip of the search box",
//             .m_make = [](QWidget* parent) { return new SelectionCountButton(parent); },
//             .m_chain = { { .m_name = "page" } },                              // the parents the selectors need, outermost first
//             .m_sheets = {                                                     // what the legacy GUI had, quoted verbatim
//                 { "BaseStyleSheet.qss:15-21", SetOn::Application, "* { color: white; }" },
//                 { "Card.cpp:167", SetOn::Widget, "QToolTip { padding: 5px; }" },
//             },
//             .m_register = &MyModule::registerFlatElements,                    // the module's registrar, run once
//             .m_states = { States::hover(), States::disabled() },              // each state is read after the idle one
//             .m_compare = Compare::Geometry | Compare::Fonts | Compare::Palette | Compare::Pixels,
//             .m_size = QSize(120, 24),
//         };
//         EXPECT_TRUE(check(row, m_context));
//     }
//
// What a row compares (m_compare), for the idle widget and for each state:
//   Geometry    size, size hints, minimum and maximum size, contents margins and rect, the frame width, the layout's margins and
//               spacing
//   Fonts       the font the widget draws with
//   Palette     the colour of each of m_roles (a default set) in the active, inactive and disabled groups
//   Properties  the value of each of m_properties (an icon or pixmap by its pixels)
//   Pixels      the widget rendered onto an image of the device ratio 1, 1.5 and 2 (QWidget::render, a widget that is not a window
//               without its window background); a legacy twin that drew nothing is a difference, as a comparison of nothing proves
//               nothing (m_mayDrawNothing says otherwise). QWidget::render hands the style the widget as the paint device
//               (QWidgetPrivate::drawWidget), so style code that reads painter->device()->devicePixelRatio() (an icon's pixmap, an
//               image cache key) sees the ratio of the screen the test runs on at all three images: the rows at 1.5 and 2 do not
//               prove such code. A site with such code gives the row a paint function (m_paint), which the harness calls with a
//               painter on each image, so that the device is the image: the pictures "painted at ratio N".
//   ToolTip     the tip the widget shows: its box, hint, colours and pixels (ToolTipTwins); the widget has its tooltip text already
//               (a legacy twin that shows no tip is a difference, as a comparison of nothing proves nothing). The flat tip is handed
//               the font of the legacy tip (ToolTipTwins::showAndReadTip), so the font of a tip is not compared
//
// The application a twin lives in (ApplicationScope): the legacy twin has the class fonts the legacy GUI has (none: StyleManager sets
// the application font, which clears them); the flat twin has the class fonts of TextElement. Both have the base palette of the
// flattened Style as the palette of the application, as Style::polish(QApplication*) sets it in the flattened GUI (the legacy GUI's
// is the machine's theme, which no sheet decides): a window below the root of a twin takes the palette of the application, not that
// of the root (Qt::WA_WindowPropagation is off), and what Qt derives from it, such as the tint of a disabled icon, is the same in both.
//
// Each twin is made as its GUI makes it (ApplicationStyle, Row::m_asMade): the application's style is AzQtComponents::Style at the
// twin's rung for the twin's life, as StyleManager makes it in both GUIs. The flat twin's widgets have it as their style, so a widget
// the factory polishes while it builds (a page a stacked layout shows, a size hint a constructor reads) is polished with only what the
// factory had set by then, a widget made later (a popup, a line edit's icon button) has it too, and nothing polishes the tree again
// after it is made. The legacy twin's widgets keep the row's base style and the quoted sheets (the sheet style over them), and its
// application style makes what the legacy GUI's makes: QIcon's disabled, active and selected pictures (QStyleSheetStyle hands
// generatedIconPixmap to AzQtComponents::Style), and the widgets made later. A row whose factory cannot be made so sets m_asMade
// false: the application style is then Fusion, and the flattened Style is set on the flat twin's widgets once they are built, which
// polishes them again.
//
// What the twins do not reproduce, and a row must carry itself:
//   - the legacy twin has no sheet on the application: its widgets have their own style, so the factory's constructor-time style()
//     reads see the base style, not the sheet style; ToolTipTwins gives the tip its style and margin for that reason;
//   - the application's sheet is on the root of the legacy twin: a window the factory makes without a parent gets neither the sheets
//     nor the style, and the harness reports the widget that is not below the root as a flaw.
//
// A row is checked as a row: a flaw is reported when a sheet is set on a place the chain does not have, when the widget is not below
// the root, when the legacy twin is not under a sheet style, when the flat twin still has a sheet, when the sheets of the row decide
// nothing that the row compares (the legacy twin looks the same without them: a selector that reaches nothing; m_mayDecideNothing
// says otherwise), and when the row compares nothing.
//
// A row that is meant to fail (the harness is proven by one) names the fact or picture of the difference it expects in
// m_expectedDifference, exactly ("pixels at ratio 1" is not "pixels at ratio 1.5"): check() then passes when the row has that
// difference, and fails when the row passes, when it has none of that name, or when it is flawed as a twin (no expected
// difference excuses a flaw). Other differences that the same break causes are not judged. Failures name the sheets of the row.
//
// A stand-in (Link::m_className) answers qt_metacast with a class name, for a class the test cannot link: the flat matchers ask
// for a class by QObject::inherits. Qt's style sheet style matches a selector by the class of the meta object, which a stand-in
// does not have, so the legacy twin of a stand-in needs a selector the stand-in has: m_replacements swaps the class selector of the
// quoted sheets for one (an object name, a property).
// ----------------------------------------------------------------------------------

#include <AzQtComponents/Tests/FieldTwins.h>
#include <AzQtComponents/Tests/ToolTipTwins.h>

#include <AzQtComponents/Components/Elements/ColorPickerElements.h>
#include <AzQtComponents/Components/GuiPerfFloor.h>

#include <QAbstractButton>
#include <QDebug>
#include <QFrame>
#include <QHoverEvent>
#include <QIcon>
#include <QLayout>
#include <QMetaEnum>
#include <QMetaType>
#include <QMouseEvent>
#include <QPainter>
#include <QPixmap>
#include <QStyleFactory>
#include <QToolTip>
#include <QVariant>

#include <algorithm>
#include <functional>
#include <memory>
#include <set>
#include <string>
#include <utility>
#include <vector>

namespace AzQtComponents
{
    namespace SheetTwins
    {
        // ----------------------------------------------------------------------------------
        // The row
        // ----------------------------------------------------------------------------------

        using Rung = GuiPerfFloor::Rung;

        //! Builds the site's widget below `parent`, as the site's code does (it runs at the rung of each twin). Returns the widget the
        //! row compares: the one the rules decide, which may be a part of what the factory built.
        using WidgetFactory = std::function<QWidget*(QWidget* parent)>;

        //! What a row compares (m_compare)
        enum Compare : unsigned
        {
            Geometry = 1 << 0,
            Fonts = 1 << 1,
            Palette = 1 << 2,
            Properties = 1 << 3,
            Pixels = 1 << 4,
            ToolTip = 1 << 5,
        };

        //! The style the legacy twin's sheet style draws over
        enum class Base
        {
            Fusion,
            AzQtComponentsStyle,
        };

        //! Where the real GUI sets a sheet
        enum class SetOn
        {
            Application,        //!< the application's sheet: on the root of the twin, above every parent
            Ancestor,           //!< a module's or a window's sheet: on the link of the chain at the index Sheet::m_ancestor
            Widget,             //!< the widget's own sheet (an inline setStyleSheet); it replaces the one the site's code set
        };

        //! A sheet of the legacy twin, quoted as the file has it
        struct Sheet
        {
            const char* m_cite;         //!< "File.qss:10-20" or "File.cpp:101", where the rules are (a failure names it)
            SetOn m_on;
            const char* m_text;         //!< the rules verbatim
            int m_ancestor = 0;         //!< SetOn::Ancestor: the index in the chain
        };

        //! A parent in the chain between the root and the widget, for the selectors that look at ancestors
        struct Link
        {
            const char* m_name = "";                //!< its object name
            const char* m_className = nullptr;      //!< a stand-in that answers qt_metacast with this class name (null: a QWidget)
            WidgetFactory m_make;                   //!< the real class, where it links (m_className is then ignored)
        };

        //! A state to drive: entered, read, left
        struct State
        {
            std::string m_name;
            std::function<void(QWidget&)> m_enter;
            std::function<void(QWidget&)> m_leave;
        };

        //! What a site draws through its style, for the sites whose look reads the device the painter paints on: it is called with a
        //! painter on an image of each device ratio (FieldTwins::paintThroughStyle), so that painter->device() is the image
        using PaintFunction = std::function<void(QPainter& painter, QWidget& widget)>;

        struct Row
        {
            const char* m_site = "";                        //!< the census id and place, for the report
            WidgetFactory m_make;
            std::vector<Link> m_chain;                      //!< outermost first
            std::vector<Sheet> m_sheets;
            bool m_realApplicationSheet = false;            //!< the application's real sheet (BaseStyleSheet.qss and its imports) first
            void (*m_register)() = nullptr;                 //!< the module's registrar of flat elements, run once per process
            std::vector<State> m_states;
            unsigned m_compare = Geometry | Fonts | Palette | Pixels;
            std::vector<QPalette::ColorRole> m_roles;       //!< Palette: empty is the default set (defaultRoles)
            std::vector<const char*> m_properties;          //!< Properties: the names to read
            PaintFunction m_paint;                          //!< Pixels: also what the site draws through its style, at each ratio
            QSize m_size;                                   //!< the widget's size; invalid: its size hint
            QSize m_rootSize = QSize(300, 200);
            Base m_base = Base::Fusion;
            Rung m_legacyRung = Rung::Qss;                  //!< the rung the legacy twin's code runs at
            std::vector<std::pair<const char*, const char*>> m_replacements;    //!< selector swaps for the quoted sheets
            bool m_mayDrawNothing = false;                  //!< the legacy twin may draw nothing (a picture of one colour) or show no tip
            bool m_mayDecideNothing = false;                //!< the sheets may decide nothing the row compares (the row proves equality)
            const char* m_expectedDifference = nullptr;     //!< a row that must fail: the name of the fact or picture that differs
            bool m_asMade = true;                           //!< each twin is made under its GUI's application style (ApplicationStyle)
        };

        //! The roles a rule on text and fills decides
        inline std::vector<QPalette::ColorRole> defaultRoles()
        {
            return { QPalette::Base, QPalette::Button, QPalette::ButtonText, QPalette::Text, QPalette::Window, QPalette::WindowText,
                QPalette::PlaceholderText };
        }

        // ----------------------------------------------------------------------------------
        // The process state a row needs
        // ----------------------------------------------------------------------------------

        //! The rung for a scope, and the rung it was after it
        class RungScope
        {
        public:
            explicit RungScope(Rung rung)
                : m_before(GuiPerfFloor::rung())
            {
                GuiPerfFloor::setRungForTesting(rung);
            }

            ~RungScope()
            {
                GuiPerfFloor::setRungForTesting(m_before);
            }

            RungScope(const RungScope&) = delete;
            RungScope& operator=(const RungScope&) = delete;

        private:
            Rung m_before;
        };

        //! A widget that Qt takes for an instance of a class it is not (the matchers ask for a class by QObject::inherits)
        class StandIn
            : public QWidget
        {
        public:
            StandIn(const char* className, QWidget* parent)
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

        //! The application of a test: the QApplication (made when the test has none), the font, palette and style the engine has when
        //! StyleManager sets its style, the flattened Style (an application filter, as Style::polish(QApplication*) installs it) and
        //! the Global elements, and the class fonts of the flattened GUI. Make it in the fixture, before any widget. A twin changes the
        //! class fonts and the palette for its life (ApplicationScope) and puts them back.
        class Context
        {
        public:
            Context()
            {
                if (!QApplication::instance())
                {
                    m_application = std::make_unique<QApplication>(m_argc, m_argv);
                }

                // The fonts and the text colour the application has when StyleManager sets the style (StyleManager.cpp:362-364,
                // Style.cpp buildO3DEBasePalette): the legacy sheet gives labels and buttons the same
                QFont applicationFont(QStringLiteral("Open Sans"));
                applicationFont.setPixelSize(12);
                QApplication::setFont(applicationFont);
                QPalette palette;
                for (const QPalette::ColorRole role : { QPalette::WindowText, QPalette::Text, QPalette::ButtonText })
                {
                    palette.setColor(role, Qt::white);
                }
                QApplication::setPalette(palette);

                QApplication::setStyle(QStyleFactory::create(QStringLiteral("Fusion")));
                m_fusion.reset(QStyleFactory::create(QStringLiteral("Fusion")));
                m_flat = std::make_unique<Style>(QStyleFactory::create(QStringLiteral("Fusion")));
                m_legacyStyle = std::make_unique<Style>(QStyleFactory::create(QStringLiteral("Fusion")));

                // What Style::polish(QApplication*) registers in the flattened GUI, once for the process
                RungScope flat(Rung::Flat);
                TextElement::installClassFonts();
                ControlElements::registerGlobal();
                ColorPickerElements::registerGlobal();
                PropertyEditorElements::registerGlobal();
                ComboBox::registerFlatElements();

                QApplication::instance()->installEventFilter(m_flat.get());
            }

            ~Context()
            {
                QApplication::instance()->removeEventFilter(m_flat.get());
            }

            Context(const Context&) = delete;
            Context& operator=(const Context&) = delete;

            QStyle* flatStyle() const
            {
                return m_flat.get();
            }

            QStyle* baseOf(Base base) const
            {
                return base == Base::Fusion ? m_fusion.get() : m_legacyStyle.get();
            }

            //! The registrar of a module's flat elements, at the Flat rung, once for the process
            static void registerOnce(void (*registrar)())
            {
                static std::set<void (*)()> done;
                if (registrar && done.insert(registrar).second)
                {
                    RungScope flat(Rung::Flat);
                    registrar();
                }
            }

        private:
            int m_argc = 1;                                         // QApplication keeps a reference to it
            char m_programName[5] = "test";
            char* m_argv[2] = { m_programName, nullptr };
            std::unique_ptr<QApplication> m_application;
            std::unique_ptr<QStyle> m_fusion;
            std::unique_ptr<Style> m_flat;
            std::unique_ptr<Style> m_legacyStyle;
        };

        //! The application as the GUI of a twin has it, for the life of the twin; it is made before the twin's widgets, which take the
        //! class fonts and the palette when they are made, and it puts back what it changed.
        //!   - the palette: the base palette of the flattened Style, in both twins. The flattened GUI has it as the palette of the
        //!     application (Style::polish(QApplication*)), so that a window below the root of a twin, which does not take the palette
        //!     of its parent (Qt::WA_WindowPropagation is off), has it as in the GUI. The legacy GUI has the platform's palette, which
        //!     is the theme of the machine and no decision of a sheet or of the style; what Qt derives from the application palette (the
        //!     disabled and the active tint of an icon: QIcon::pixmap reads QGuiApplication::palette()) would differ with the
        //!     machine, so the legacy twin is held to the palette of the flat one: a row compares what the sheets and the style decide.
        //!   - the class fonts: the legacy GUI has none of the flattened GUI's. StyleManager sets the application font, which clears
        //!     every class font (QApplication::setFont), and Style::polish(QApplication*) installs none at the Qss rung; this process
        //!     has the offscreen platform, which supplies none. A font that the quoted sheets leave unset is then the application's, and
        //!     the Fonts of a row compare the sheets with the flat code, not the flat code with itself. The flattened GUI has the class
        //!     fonts of TextElement.
        class ApplicationScope
        {
        public:
            ApplicationScope(bool legacy, const Context& context)
                : m_legacy(legacy)
                , m_font(QApplication::font())
                , m_palette(QApplication::palette())
            {
                RungScope flat(Rung::Flat);
                QPalette palette = m_palette;
                context.flatStyle()->polish(palette);   // Style::polish(QPalette&): the base palette, whatever the palette it is given
                QApplication::setPalette(palette);

                if (legacy)
                {
                    QApplication::setFont(m_font);      // no class name: the class fonts of the process are cleared
                }
                else
                {
                    TextElement::installClassFonts();
                }
            }

            ~ApplicationScope()
            {
                QApplication::setPalette(m_palette);
                if (m_legacy)
                {
                    // The process has the class fonts of the flattened GUI between twins (Context)
                    RungScope flat(Rung::Flat);
                    TextElement::installClassFonts();
                }
            }

            ApplicationScope(const ApplicationScope&) = delete;
            ApplicationScope& operator=(const ApplicationScope&) = delete;

        private:
            bool m_legacy;
            QFont m_font;
            QPalette m_palette;
        };

        //! AzQtComponents::Style at a twin's rung as the style of the application, for the life of a twin made as its GUI makes it
        //! (Row::m_asMade): StyleManager makes it the application's style in both GUIs. A widget of the flat twin polished while the
        //! factory builds it is polished by it with only what the factory had set by then, and a widget made later (a popup, a tip, a
        //! line edit's icon button) has it too; in both twins QIcon's pictures of a mode come from it. The context's flattened Style
        //! leaves the application's event filters for that life, so that one Style filters the events, as in the GUI. The style the
        //! application had is kept (QApplication::setStyle deletes an old style the application parents, and a test's StyleManager
        //! holds its style) and put back after, with what QApplication::setStyle changes in the process (the palette and the class
        //! fonts of the application, the palette of the tips: Style::polish(QApplication*)). Inactive: nothing.
        class ApplicationStyle
        {
        public:
            ApplicationStyle(bool active, Rung rung, const Context& context)
                : m_active(active)
                , m_context(context)
                , m_palette(QApplication::palette())
                , m_font(QApplication::font())
                , m_tipPalette(QToolTip::palette())
            {
                if (m_active)
                {
                    m_rung = std::make_unique<RungScope>(rung);
                    m_previous = QApplication::style();
                    m_previous->setParent(nullptr);     // kept: QApplication::setStyle deletes an old style the application parents
                    QApplication::instance()->removeEventFilter(m_previous);
                    QApplication::instance()->removeEventFilter(m_context.flatStyle());
                    QApplication::setStyle(new Style(QStyleFactory::create(QStringLiteral("Fusion"))));
                }
            }

            ~ApplicationStyle()
            {
                if (m_active)
                {
                    QApplication::setStyle(m_previous);     // the application parents it again, and deletes the flattened Style
                    QToolTip::setPalette(m_tipPalette);
                    QApplication::setPalette(m_palette);
                    QApplication::setFont(m_font);      // no class name: the class fonts of the process are cleared, and the process
                    TextElement::installClassFonts();   // has those of the flattened GUI between twins (Context)
                    QApplication::instance()->installEventFilter(m_context.flatStyle());
                }
            }

            ApplicationStyle(const ApplicationStyle&) = delete;
            ApplicationStyle& operator=(const ApplicationStyle&) = delete;

        private:
            bool m_active;
            const Context& m_context;
            QPalette m_palette;
            QFont m_font;
            QPalette m_tipPalette;
            QStyle* m_previous = nullptr;       // the application's style before
            std::unique_ptr<RungScope> m_rung;  // the twin's rung while the Style is the application's
        };

        // ----------------------------------------------------------------------------------
        // The states
        // ----------------------------------------------------------------------------------

        namespace States
        {
            //! The active window, set at once (QApplication::setActiveWindow is deprecated for QWidget::activateWindow, which asks the
            //! platform and so takes effect later; a focus state needs the window active on this turn)
            inline void setActiveWindowNow(QWidget* window)
            {
                QT_WARNING_PUSH
                QT_WARNING_DISABLE_DEPRECATED
                QApplication::setActiveWindow(window);
                QT_WARNING_POP
            }

            //! unpolish and polish of the widget and everything under it: the sheet style reads a property selector at polish, and
            //! the flattened Style resolves its element then
            inline void repolish(QWidget& widget)
            {
                QList<QWidget*> widgets = widget.findChildren<QWidget*>();
                widgets.prepend(&widget);
                for (QWidget* each : widgets)
                {
                    each->style()->unpolish(each);
                    each->style()->polish(each);
                    each->update();
                }
            }

            //! The pointer enters (or leaves) the widget as Qt delivers it: the widget is under the mouse, gets the hover event if it
            //! tracks hover (Qt::WA_Hover) and the move to its centre if it tracks the mouse. The sheet style turns mouse tracking on
            //! for a widget with a :hover rule (QStyleSheetStyle::polish), and a push button then takes its hover from the move
            //! (QPushButtonPrivate::styleButtonState and QPushButton::mouseMoveEvent): without it no legacy button is hovered
            inline void movePointer(QWidget& widget, bool inside)
            {
                widget.setAttribute(Qt::WA_UnderMouse, inside);
                const QPointF centre = widget.rect().center();
                const QPointF outside(-1, -1);
                if (widget.testAttribute(Qt::WA_Hover))
                {
                    QHoverEvent hover(inside ? QEvent::HoverEnter : QEvent::HoverLeave, inside ? centre : outside,
                        widget.mapToGlobal(inside ? centre : outside), inside ? outside : centre);
                    QApplication::sendEvent(&widget, &hover);
                }
                if (inside && widget.hasMouseTracking())
                {
                    QMouseEvent move(QEvent::MouseMove, centre, widget.mapToGlobal(centre), Qt::NoButton, Qt::NoButton, Qt::NoModifier);
                    QApplication::sendEvent(&widget, &move);
                }
                widget.update();
            }

            inline State hover()
            {
                return { "hover", [](QWidget& widget) { movePointer(widget, true); }, [](QWidget& widget) { movePointer(widget, false); } };
            }

            //! A button held down
            inline State pressed()
            {
                const auto set = [](QWidget& widget, bool down)
                {
                    if (auto* button = qobject_cast<QAbstractButton*>(&widget))
                    {
                        button->setDown(down);
                    }
                };
                return { "pressed", [set](QWidget& widget) { set(widget, true); }, [set](QWidget& widget) { set(widget, false); } };
            }

            //! A button checked (made checkable if it is not)
            inline State checked()
            {
                const auto set = [](QWidget& widget, bool on)
                {
                    if (auto* button = qobject_cast<QAbstractButton*>(&widget))
                    {
                        button->setCheckable(true);
                        button->setChecked(on);
                    }
                };
                return { "checked", [set](QWidget& widget) { set(widget, true); }, [set](QWidget& widget) { set(widget, false); } };
            }

            inline State disabled()
            {
                return { "disabled", [](QWidget& widget) { widget.setEnabled(false); }, [](QWidget& widget) { widget.setEnabled(true); } };
            }

            //! The widget has the keyboard focus, in a window that is the active one
            inline State focus()
            {
                return { "focus",
                    [](QWidget& widget)
                    {
                        setActiveWindowNow(widget.window());
                        widget.setFocus(Qt::OtherFocusReason);
                    },
                    [](QWidget& widget)
                    {
                        widget.clearFocus();
                        setActiveWindowNow(nullptr);
                    } };
            }

            //! A property set (a `[name=value]` selector, a class tag), and the widget polished again; left, it is put back
            inline State property(const char* name, const QVariant& value)
            {
                auto before = std::make_shared<QVariant>();
                return { std::string("property ") + name,
                    [name, value, before](QWidget& widget)
                    {
                        *before = widget.property(name);
                        widget.setProperty(name, value);
                        repolish(widget);
                    },
                    [name, before](QWidget& widget)
                    {
                        widget.setProperty(name, *before);
                        repolish(widget);
                    } };
            }
        } // namespace States

        // ----------------------------------------------------------------------------------
        // Phase 1: a twin, built
        // ----------------------------------------------------------------------------------

        //! The application's root, the chain of parents and the widget under it, shown. The root is the place of the application's
        //! sheet; it is a plain widget (not shown on screen). The application the twin lives in (ApplicationScope) is made before the
        //! root, and so before every widget, and is put back after them.
        class Twin
        {
        public:
            Twin(const Row& row, const Context& context, bool legacy)
                : m_application(legacy, context)
                , m_applicationStyle(row.m_asMade, legacy ? row.m_legacyRung : Rung::Flat, context)
            {
                for (const Sheet& sheet : row.m_sheets)
                {
                    if (sheet.m_on == SetOn::Ancestor && (sheet.m_ancestor < 0 || sheet.m_ancestor >= static_cast<int>(row.m_chain.size())))
                    {
                        m_flaws.push_back(std::string("the sheet ") + sheet.m_cite + " is set on the link "
                            + std::to_string(sheet.m_ancestor) + " of a chain of " + std::to_string(row.m_chain.size())
                            + ", so no twin has it");
                    }
                }

                QWidget* parent = &m_root;
                for (const Link& link : row.m_chain)
                {
                    QWidget* made = link.m_make ? link.m_make(parent)
                                                : (link.m_className ? new StandIn(link.m_className, parent) : new QWidget(parent));
                    made->setObjectName(QString::fromLatin1(link.m_name));
                    made->setGeometry(QRect(QPoint(0, 0), row.m_rootSize));
                    m_chain.push_back(made);
                    parent = made;
                }
                m_widget = row.m_make(parent);
                if (!isBelowRoot(m_widget))
                {
                    m_flaws.push_back("the widget is not below the root of the twin (a window made without a parent gets neither the "
                                      "sheets nor the style of the twin)");
                }

                if (legacy)
                {
                    setLegacySheets(row, context);
                }
                else if (!row.m_asMade)
                {
                    setFlatStyle(context);
                }

                m_root.setAttribute(Qt::WA_DontShowOnScreen);
                m_root.resize(row.m_rootSize);
                m_root.show();
                QApplication::processEvents();

                // Polished as the application does it, before the widget is sized: the polish carries the minimum and maximum sizes
                for (QWidget* each : widgetsOfTree())
                {
                    each->ensurePolished();
                }
                if (row.m_size.isValid())
                {
                    m_widget->resize(row.m_size);
                }
                else
                {
                    m_widget->adjustSize();
                }
                QApplication::processEvents();
            }

            QWidget* widget() const
            {
                return m_widget;
            }

            //! Every widget of the twin, the root first
            QList<QWidget*> widgetsOfTree() const
            {
                QList<QWidget*> widgets = m_root.findChildren<QWidget*>();
                widgets.prepend(const_cast<QWidget*>(&m_root));
                return widgets;
            }

            //! What is wrong with the twin as a twin: the legacy one has to be under a sheet style, the flat one under none. What is
            //! wrong with the row itself (a sheet on a link the chain does not have, a widget outside the tree) is the legacy twin's,
            //! so that a row's flaw is reported once.
            std::vector<std::string> flaws(bool legacy) const
            {
                std::vector<std::string> flaws;
                if (legacy)
                {
                    flaws = m_flaws;
                    if (!m_widget->style()->inherits("QStyleSheetStyle"))
                    {
                        flaws.push_back("the legacy twin is not under a style sheet style (neither the row nor the site sets a sheet)");
                    }
                    return flaws;
                }
                for (const QWidget* each : widgetsOfTree())
                {
                    if (!each->styleSheet().isEmpty() || each->style()->inherits("QStyleSheetStyle"))
                    {
                        flaws.push_back(std::string("the flat twin still has a style sheet on ") + each->metaObject()->className() + "#"
                            + each->objectName().toStdString());
                    }
                }
                return flaws;
            }

        private:
            //! Whether the widget is the root or a descendant of it, through windows (QWidget::isAncestorOf stops at a window)
            bool isBelowRoot(const QWidget* widget) const
            {
                for (const QWidget* each = widget; each; each = each->parentWidget())
                {
                    if (each == &m_root)
                    {
                        return true;
                    }
                }
                return false;
            }

            //! The quoted sheets of one place, joined, with the row's selector swaps
            static QString sheetsOn(const Row& row, SetOn on, int ancestor)
            {
                QString joined;
                if (on == SetOn::Application && row.m_realApplicationSheet)
                {
                    joined += FieldTwins::appSheet();
                }
                for (const Sheet& sheet : row.m_sheets)
                {
                    if (sheet.m_on == on && (on != SetOn::Ancestor || sheet.m_ancestor == ancestor))
                    {
                        joined += QString::fromLatin1(sheet.m_text) + QLatin1Char('\n');
                    }
                }
                for (const auto& replacement : row.m_replacements)
                {
                    joined.replace(QString::fromLatin1(replacement.first), QString::fromLatin1(replacement.second));
                }
                return joined;
            }

            void setStyleOnTree(QStyle* style)
            {
                for (QWidget* each : widgetsOfTree())
                {
                    each->setStyle(style);
                }
            }

            //! The legacy twin: the base style on every widget (a style set on a widget reaches no child), then the sheets where the
            //! real GUI sets them, the application's above all
            void setLegacySheets(const Row& row, const Context& context)
            {
                setStyleOnTree(context.baseOf(row.m_base));

                const auto set = [](QWidget* widget, const QString& sheet)
                {
                    if (!sheet.isEmpty())
                    {
                        widget->setStyleSheet(sheet);
                    }
                };
                set(&m_root, sheetsOn(row, SetOn::Application, 0));
                for (size_t i = 0; i < m_chain.size(); ++i)
                {
                    set(m_chain[i], sheetsOn(row, SetOn::Ancestor, static_cast<int>(i)));
                }
                set(m_widget, sheetsOn(row, SetOn::Widget, 0));
            }

            //! The flat twin of a row with m_asMade false: the flattened Style on every widget once they are built (the palette is the
            //! application's, ApplicationScope)
            void setFlatStyle(const Context& context)
            {
                setStyleOnTree(context.flatStyle());
            }

            ApplicationScope m_application;         // before the widgets: made first, put back last
            ApplicationStyle m_applicationStyle;    // after the application's palette and fonts, before the widgets: put back first
            QWidget m_root;
            std::vector<QWidget*> m_chain;
            QWidget* m_widget = nullptr;
            std::vector<std::string> m_flaws;
        };

        // ----------------------------------------------------------------------------------
        // Phase 2: a twin, read in each state
        // ----------------------------------------------------------------------------------

        struct Fact
        {
            std::string m_name;
            std::string m_text;
        };

        struct Picture
        {
            std::string m_name;
            QImage m_image;
        };

        //! What a twin showed in one state
        struct Reading
        {
            std::string m_state;
            std::vector<Fact> m_facts;
            std::vector<Picture> m_pictures;
            QFont m_tipFont;            //!< the legacy tip's font, which the flat tip is given (a font is not what a row proves)
        };

        inline void addFact(Reading& reading, const std::string& name, const std::string& text)
        {
            reading.m_facts.push_back({ name, text });
        }

        inline std::string colorText(const QColor& color)
        {
            return color.name(QColor::HexArgb).toStdString();
        }

        inline std::string digestOf(const QImage& image)
        {
            return std::to_string(image.width()) + "x" + std::to_string(image.height()) + " #"
                + std::to_string(static_cast<unsigned long long>(qHashBits(image.constBits(), static_cast<size_t>(image.sizeInBytes()))));
        }

        //! A property value as text: an icon or a pixmap by its pixels, anything else as QDebug writes it
        inline std::string variantText(const QVariant& value)
        {
            if (!value.isValid())
            {
                return "(invalid)";
            }
            if (value.metaType() == QMetaType::fromType<QIcon>())
            {
                const QIcon icon = value.value<QIcon>();
                return icon.isNull() ? "null icon" : "icon, " + digestOf(icon.pixmap(QSize(32, 32)).toImage());
            }
            if (value.metaType() == QMetaType::fromType<QPixmap>())
            {
                return "pixmap, " + digestOf(value.value<QPixmap>().toImage());
            }
            QString text;
            QDebug(&text).noquote() << value;
            return text.toStdString();
        }

        //! The font a widget draws with: the font the sheet or the element asked for (QFont, which does not depend on the fonts the
        //! machine has) and the one the font database resolved it to (QFontInfo, which does)
        inline void readFont(Reading& reading, const QFont& font)
        {
            addFact(reading, "font.request",
                "family " + font.family().toStdString() + ", " + std::to_string(font.pixelSize()) + "px, "
                    + std::to_string(font.pointSizeF()) + "pt, weight " + std::to_string(static_cast<int>(font.weight()))
                    + (font.italic() ? ", italic" : "") + (font.underline() ? ", underline" : ""));
            addFact(reading, "font.resolved", ToolTipTwins::fontText(font));
        }

        inline void readGeometry(Reading& reading, QWidget& widget)
        {
            using FieldTwins::sizeText;
            addFact(reading, "geometry.size", sizeText(widget.size()));
            addFact(reading, "geometry.sizeHint", sizeText(widget.sizeHint()));
            addFact(reading, "geometry.minimumSizeHint", sizeText(widget.minimumSizeHint()));
            addFact(reading, "geometry.minimumSize", sizeText(widget.minimumSize()));
            addFact(reading, "geometry.maximumSize", sizeText(widget.maximumSize()));
            addFact(reading, "geometry.contentsMargins", ToolTipTwins::marginsText(widget.contentsMargins()));
            addFact(reading, "geometry.contentsRect", FieldTwins::rectText(widget.contentsRect()));
            if (const auto* frame = qobject_cast<const QFrame*>(&widget))
            {
                addFact(reading, "geometry.frameWidth", std::to_string(frame->frameWidth()));
            }
            if (const QLayout* layout = widget.layout())
            {
                addFact(reading, "geometry.layoutMargins", ToolTipTwins::marginsText(layout->contentsMargins()));
                addFact(reading, "geometry.layoutSpacing", std::to_string(layout->spacing()));
            }
        }

        inline void readPalette(Reading& reading, const QWidget& widget, const Row& row)
        {
            const QMetaEnum groups = QMetaEnum::fromType<QPalette::ColorGroup>();
            const QMetaEnum roleNames = QMetaEnum::fromType<QPalette::ColorRole>();
            const std::vector<QPalette::ColorRole> roles = row.m_roles.empty() ? defaultRoles() : row.m_roles;
            for (const QPalette::ColorGroup group : { QPalette::Active, QPalette::Inactive, QPalette::Disabled })
            {
                for (const QPalette::ColorRole role : roles)
                {
                    addFact(reading, std::string("palette.") + groups.valueToKey(group) + "." + roleNames.valueToKey(role),
                        colorText(widget.palette().color(group, role)));
                }
            }
        }

        //! The widget rendered onto an image of each device ratio, on grey: what stays grey is what neither the sheet nor the element
        //! draws. A widget that is not a window is rendered without the window's background (QWidget::DrawWindowBackground). The
        //! style is handed the widget as the paint device there (QWidgetPrivate::drawWidget), and so the ratio of the screen: a row
        //! whose site reads the ratio of the painter's device gives a paint function, which gets a painter on each image.
        inline void readPixels(Reading& reading, QWidget& widget, const Row& row)
        {
            const QWidget::RenderFlags flags =
                widget.isWindow() ? QWidget::RenderFlags(QWidget::DrawWindowBackground | QWidget::DrawChildren) : QWidget::DrawChildren;
            for (const qreal ratio : ToolTipTwins::Ratios)
            {
                QImage image((QSizeF(widget.size()) * ratio).toSize(), QImage::Format_ARGB32_Premultiplied);
                image.setDevicePixelRatio(ratio);
                image.fill(QColor(0x80, 0x80, 0x80));
                widget.render(&image, QPoint(), QRegion(), flags);
                reading.m_pictures.push_back({ "pixels at ratio " + QString::number(ratio).toStdString(), image });
            }

            if (row.m_paint)
            {
                for (const qreal ratio : ToolTipTwins::Ratios)
                {
                    const QImage image = FieldTwins::paintThroughStyle(&widget, ratio,
                        [&row, &widget](QPainter& painter) { row.m_paint(painter, widget); });
                    reading.m_pictures.push_back({ "painted at ratio " + QString::number(ratio).toStdString(), image });
                }
            }
        }

        //! The tip of the widget. The legacy tip is made below the widget, so below its sheet; the flat tip is made under the
        //! application's style and handed the flattened one then (ToolTipTwins::showAndReadTip), with the font of the legacy tip.
        inline void readTip(Reading& reading, QWidget& widget, QStyle* flatStyle, const Reading* legacy)
        {
            const ToolTipTwins::TipFacts tip =
                ToolTipTwins::showAndReadTip(&widget, flatStyle, legacy ? &legacy->m_tipFont : nullptr);
            addFact(reading, "tip.shown", tip.m_found ? "yes" : "no");
            if (!tip.m_found)
            {
                return;
            }
            reading.m_tipFont = tip.m_font;
            addFact(reading, "tip.contentsMargins", ToolTipTwins::marginsText(tip.m_contents));
            addFact(reading, "tip.margin", std::to_string(tip.m_margin));
            addFact(reading, "tip.sizeHint", FieldTwins::sizeText(tip.m_hint));
            addFact(reading, "tip.contentsRect", FieldTwins::rectText(tip.m_contentsRect));
            addFact(reading, "tip.base", colorText(tip.m_base));
            addFact(reading, "tip.text", colorText(tip.m_text));
            for (int i = 0; i < 3; ++i)
            {
                reading.m_pictures.push_back({ "tip at ratio " + QString::number(ToolTipTwins::Ratios[i]).toStdString(), tip.m_images[i] });
            }
        }

        inline Reading readOnce(const Row& row, QWidget& widget, const Context& context, bool legacy, const Reading* legacyReading)
        {
            Reading reading;
            if (row.m_compare & Geometry)
            {
                readGeometry(reading, widget);
            }
            if (row.m_compare & Fonts)
            {
                readFont(reading, widget.font());
            }
            if (row.m_compare & Palette)
            {
                readPalette(reading, widget, row);
            }
            if (row.m_compare & Properties)
            {
                for (const char* name : row.m_properties)
                {
                    addFact(reading, std::string("property.") + name, variantText(widget.property(name)));
                }
            }
            if (row.m_compare & Pixels)
            {
                readPixels(reading, widget, row);
            }
            if (row.m_compare & ToolTip)
            {
                readTip(reading, widget, legacy ? nullptr : context.flatStyle(), legacyReading);
            }
            return reading;
        }

        //! The widget idle, then in each state of the row (entered, read, left)
        inline std::vector<Reading> readStates(
            const Row& row, QWidget& widget, const Context& context, bool legacy, const std::vector<Reading>* legacyReadings)
        {
            std::vector<Reading> readings;
            const auto legacyAt = [legacyReadings](size_t index) { return legacyReadings ? &(*legacyReadings)[index] : nullptr; };

            readings.push_back(readOnce(row, widget, context, legacy, legacyAt(0)));
            readings.back().m_state = "idle";

            for (const State& state : row.m_states)
            {
                state.m_enter(widget);
                QApplication::processEvents();
                readings.push_back(readOnce(row, widget, context, legacy, legacyAt(readings.size())));
                readings.back().m_state = state.m_name;
                state.m_leave(widget);
                QApplication::processEvents();
            }
            return readings;
        }

        // ----------------------------------------------------------------------------------
        // Phase 3: the twins, compared
        // ----------------------------------------------------------------------------------

        //! What a row found: the flaws of the row as a twin (the twins are not twins, or the row proves nothing), every difference
        //! between the twins, by state, and how much was compared
        struct Result
        {
            std::vector<std::string> m_flaws;
            std::vector<std::string> m_differences;
            int m_compared = 0;         //!< facts and pictures compared, so that a row that compared nothing shows

            bool ok() const
            {
                return m_flaws.empty() && m_differences.empty();
            }

            std::string text() const
            {
                std::string text;
                for (const std::string& flaw : m_flaws)
                {
                    text += flaw + "\n";
                }
                for (const std::string& difference : m_differences)
                {
                    text += difference + "\n";
                }
                return text;
            }
        };

        inline void compareReadings(const Row& row, const Reading& legacy, const Reading& flat, Result& result)
        {
            const std::string where = std::string(row.m_site) + " [" + legacy.m_state + "] ";

            for (const Fact& fact : legacy.m_facts)
            {
                ++result.m_compared;
                const auto found = std::find_if(flat.m_facts.begin(), flat.m_facts.end(),
                    [&fact](const Fact& other) { return other.m_name == fact.m_name; });
                if (found == flat.m_facts.end())
                {
                    result.m_differences.push_back(where + fact.m_name + ": the flat twin has no such fact");
                }
                else if (found->m_text != fact.m_text)
                {
                    result.m_differences.push_back(where + fact.m_name + ": legacy " + fact.m_text + ", flat " + found->m_text);
                }
                else if (fact.m_name == "tip.shown" && fact.m_text == "no" && !row.m_mayDrawNothing)
                {
                    // Neither twin showed a tip: the facts agree, and nothing was compared
                    result.m_differences.push_back(where + fact.m_name + ": the legacy twin showed no tip, which proves nothing");
                }
            }

            for (size_t i = 0; i < legacy.m_pictures.size() && i < flat.m_pictures.size(); ++i)
            {
                const Picture& legacyPicture = legacy.m_pictures[i];
                const QImage& flatImage = flat.m_pictures[i].m_image;
                ++result.m_compared;
                if (!row.m_mayDrawNothing && !FieldTwins::isDrawnOn(legacyPicture.m_image))
                {
                    result.m_differences.push_back(where + legacyPicture.m_name + ": the legacy twin drew nothing, which proves nothing");
                }
                else if (FieldTwins::differing(legacyPicture.m_image, flatImage) != 0)
                {
                    result.m_differences.push_back(where + legacyPicture.m_name + ": "
                        + std::to_string(FieldTwins::differing(legacyPicture.m_image, flatImage)) + " pixels differ, "
                        + FieldTwins::firstDifference(legacyPicture.m_image, flatImage));
                }
            }
            if (legacy.m_pictures.size() != flat.m_pictures.size())
            {
                result.m_differences.push_back(where + "the twins drew a different number of pictures");
            }
        }

        // ----------------------------------------------------------------------------------
        // Running a row
        // ----------------------------------------------------------------------------------

        //! Whether two readings of the same widget differ in a fact or a picture
        inline bool readingsDiffer(const Reading& a, const Reading& b)
        {
            if (a.m_facts.size() != b.m_facts.size() || a.m_pictures.size() != b.m_pictures.size())
            {
                return true;
            }
            for (size_t i = 0; i < a.m_facts.size(); ++i)
            {
                if (a.m_facts[i].m_name != b.m_facts[i].m_name || a.m_facts[i].m_text != b.m_facts[i].m_text)
                {
                    return true;
                }
            }
            for (size_t i = 0; i < a.m_pictures.size(); ++i)
            {
                if (FieldTwins::differing(a.m_pictures[i].m_image, b.m_pictures[i].m_image) != 0)
                {
                    return true;
                }
            }
            return false;
        }

        //! The legacy twin of a row without the sheets the row quotes (what the site and the base style give the widget by
        //! themselves), read idle and in each state
        inline std::vector<Reading> readLegacyWithoutSheets(const Row& row, Context& context)
        {
            Row bare = row;
            bare.m_sheets.clear();
            bare.m_realApplicationSheet = false;

            RungScope rung(row.m_legacyRung);
            const Twin twin(bare, context, true);
            return readStates(bare, *twin.widget(), context, true, nullptr);
        }

        //! Builds the legacy twin, reads it, lets it go; builds the flat twin, reads it, lets it go; compares the readings. A row whose
        //! quoted sheets change nothing the row compares is flawed: its legacy twin looks the same without them (a selector that
        //! reaches nothing), so that the row would pass for any flat code that is the base style.
        inline Result run(const Row& row, Context& context)
        {
            Result result;
            Context::registerOnce(row.m_register);

            std::unique_ptr<ToolTipTwins::Guard> tipGuard;
            if (row.m_compare & ToolTip)
            {
                tipGuard = std::make_unique<ToolTipTwins::Guard>();
            }

            std::vector<Reading> legacy;
            {
                RungScope rung(row.m_legacyRung);
                const Twin twin(row, context, true);
                for (const std::string& flaw : twin.flaws(true))
                {
                    result.m_flaws.push_back(std::string(row.m_site) + ": " + flaw);
                }
                legacy = readStates(row, *twin.widget(), context, true, nullptr);
            }

            const bool quotesSheets = row.m_realApplicationSheet || !row.m_sheets.empty();
            if (quotesSheets && !row.m_mayDecideNothing && result.m_flaws.empty())
            {
                const std::vector<Reading> bare = readLegacyWithoutSheets(row, context);
                bool decides = bare.size() != legacy.size();
                for (size_t i = 0; i < legacy.size() && i < bare.size() && !decides; ++i)
                {
                    decides = readingsDiffer(legacy[i], bare[i]);
                }
                if (!decides)
                {
                    result.m_flaws.push_back(std::string(row.m_site)
                        + ": the sheets of the row decide nothing that the row compares (the legacy twin looks the same without them)");
                }
            }

            std::vector<Reading> flat;
            {
                RungScope rung(Rung::Flat);
                const Twin twin(row, context, false);
                for (const std::string& flaw : twin.flaws(false))
                {
                    result.m_flaws.push_back(std::string(row.m_site) + ": " + flaw);
                }
                flat = readStates(row, *twin.widget(), context, false, &legacy);
            }

            for (size_t i = 0; i < legacy.size(); ++i)
            {
                compareReadings(row, legacy[i], flat[i], result);
            }
            if (result.m_compared == 0)
            {
                result.m_flaws.push_back(std::string(row.m_site) + ": the row compares nothing");
            }
            return result;
        }

        //! Whether a difference is of the fact or picture the name gives, exactly: a difference reads "<site> [<state>] <name>: ...",
        //! so "pixels at ratio 1" names the picture at ratio 1 and not the one at ratio 1.5
        inline bool namesDifference(const std::string& difference, const char* name)
        {
            return difference.find(std::string("] ") + name + ":") != std::string::npos;
        }

        //! Where a sheet is set, for the report of a failure
        inline std::string placeOf(const Sheet& sheet)
        {
            switch (sheet.m_on)
            {
            case SetOn::Application:
                return "the application";
            case SetOn::Widget:
                return "the widget";
            case SetOn::Ancestor:
                break;
            }
            return "link " + std::to_string(sheet.m_ancestor) + " of the chain";
        }

        //! The sheets of a row and where each is set, for the report of a failure
        inline std::string sheetsOf(const Row& row)
        {
            std::string text = "the sheets of the row:";
            if (row.m_realApplicationSheet)
            {
                text += " the application's real sheet (BaseStyleSheet.qss and the files it imports) on the application;";
            }
            for (const Sheet& sheet : row.m_sheets)
            {
                text += std::string(" ") + sheet.m_cite + " on " + placeOf(sheet) + ";";
            }
            return text + "\n";
        }

        //! The row as a test asserts it: it passes when the twins agree. A row that names an expected difference passes when it has a
        //! difference of that name, and fails when it passes (the break was fixed or the row proves nothing), has none of that name,
        //! or is flawed as a twin (a flaw is no difference to expect). The other differences of the same break are not judged.
        inline ::testing::AssertionResult check(const Row& row, Context& context)
        {
            const Result result = run(row, context);
            if (!row.m_expectedDifference)
            {
                if (result.ok())
                {
                    return ::testing::AssertionSuccess();
                }
                return ::testing::AssertionFailure() << "the twins of " << row.m_site << " differ:\n" << result.text() << sheetsOf(row);
            }

            if (!result.m_flaws.empty())
            {
                return ::testing::AssertionFailure() << row.m_site << " was to fail with '" << row.m_expectedDifference
                                                     << "', but the row is flawed as a twin, which no difference excuses:\n"
                                                     << result.text() << sheetsOf(row);
            }
            for (const std::string& difference : result.m_differences)
            {
                if (namesDifference(difference, row.m_expectedDifference))
                {
                    return ::testing::AssertionSuccess();
                }
            }
            const std::string found = result.ok() ? std::string("the twins agree") : "it failed otherwise:\n" + result.text();
            return ::testing::AssertionFailure() << row.m_site << " was to fail with '" << row.m_expectedDifference << "', but " << found
                                                 << sheetsOf(row);
        }
    } // namespace SheetTwins
} // namespace AzQtComponents
