/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzQtComponents/Tests/SheetTwins.h>

#include <Source/FlatElements.h>

#include <AzQtComponents/Components/Elements/BoxElement.h>
#include <AzQtComponents/Components/StyleManager.h>

#include <QByteArray>
#include <QColor>
#include <QLabel>
#include <QPainter>
#include <QPalette>
#include <QPlainTextEdit>
#include <QResizeEvent>
#include <QStyle>
#include <QStyleOption>
#include <QWidget>

#include <string>
#include <utility>
#include <vector>

// ----------------------------------------------------------------------------------
// The flattened GUI of LuaIDE (Source/FlatElements.cpp) against the sheets its sites set in the legacy GUI. The legacy twin is Qt's own
// style sheet style with the application's real sheet; the flat twin is the flattened Style with the elements of the module and the
// calls the site makes in the flattened GUI (SheetTwins.h).
//
//   - C077: the Perforce status label of a document view (LUAEditorMainWindow.cpp:565), a QLabel, whose box and text elements the
//     module registers: the twin is on the real class, with the calls of the site on it.
//   - C078: the code view's colours (LUAEditorView.cpp, CreateStyleSheet), handed to the palette by CodeViewPalette, which the twin
//     runs as it is. The code view's own class (LUAEditorPlainTextEdit, a QPlainTextEdit with a signal and the context bus of the
//     application) cannot be built here: the twin has a QPlainTextEdit, which the sheet's selectors name as well (a type selector
//     reaches the subclasses), and the calls of CreateStyleSheet and SetReadonly on it.
//   - C079: the fold gutter (LUAEditor::FoldingWidget) fills itself in its paintEvent by its enabled state. The class needs the
//     application's text edit and settings: the twin is a stand-in widget that makes the same calls in its paintEvent (SheetTwins.h,
//     the stand-in way), and the sheet's class selector is swapped for the stand-in's object name.
// ----------------------------------------------------------------------------------

namespace LUAEditor
{
    namespace FlatElementsTestsInternal
    {
        namespace Twins = AzQtComponents::SheetTwins;
        using AzQtComponents::BoxElement;
        using AzQtComponents::StyleManager;

        class LuaIDEFlatElementsTests
            : public ::UnitTest::LeakDetectionFixture
        {
        protected:
            Twins::Context m_context;
        };

        // ------------------------------------------------------------------------------
        // C077: the Perforce status label
        // ------------------------------------------------------------------------------

        //! The sheet of the site as the file has it (LUAEditorMainWindow.cpp:565 before the flattening)
        constexpr const char* PerforceStatusSheet =
            "background: rgba(192,192,192,255); color: black;  border-style: inset;\n"
            "border-width: 1px;\nborder-color: rgba(100,100,100,255);\nborder-radius: 8px;";

        Twins::Row perforceStatusRow(const char* site, const char* text)
        {
            return Twins::Row{
                .m_site = site,
                .m_make = [text](QWidget* parent) -> QWidget*
                {
                    // The calls of LUAEditorMainWindow::OnOpenLUAView on the label
                    auto* label = new QLabel(QString::fromLatin1(text), parent);
                    label->setObjectName(LUAEditorMainWindowFlat::PerforceStatusName);
                    label->setContentsMargins(2, 2, 2, 2);
                    label->setAutoFillBackground(true);
                    label->setTextInteractionFlags(Qt::NoTextInteraction);
                    label->setAttribute(Qt::WA_TransparentForMouseEvents);
                    return label;
                },
                .m_sheets = { { "LUAEditorMainWindow.cpp:565", Twins::SetOn::Widget, PerforceStatusSheet } },
                .m_realApplicationSheet = true,
                .m_register = &RegisterFlatElements,
                .m_states = { Twins::States::disabled() },
                .m_compare = Twins::Compare::Geometry | Twins::Compare::Fonts | Twins::Compare::Palette | Twins::Compare::Pixels,
                .m_roles = { QPalette::WindowText, QPalette::Text, QPalette::ButtonText },
                .m_size = QSize(110, 24),
            };
        }

        TEST_F(LuaIDEFlatElementsTests, C077_ThePerforceStatusLabelIsTheLabelOfTheSheet)
        {
            ASSERT_TRUE(StyleManager::stylesheetsDisabled());
            EXPECT_TRUE(check(perforceStatusRow("C077 the Perforce status label, as it first reads", "Pending Status"), m_context));
        }

        TEST_F(LuaIDEFlatElementsTests, C077_AnotherStatusTextIsTheLabelOfTheSheetToo)
        {
            ASSERT_TRUE(StyleManager::stylesheetsDisabled());
            EXPECT_TRUE(check(perforceStatusRow("C077 the Perforce status label, with the text of a checked out file",
                                   "Checked out by you, revision 12 of 12"),
                m_context));
        }

        TEST_F(LuaIDEFlatElementsTests, C077_OnlyTheLabelTheSiteNamesHasTheElements)
        {
            Twins::Context::registerOnce(&RegisterFlatElements);

            QWidget root;
            QLabel named(&root);
            named.setObjectName(LUAEditorMainWindowFlat::PerforceStatusName);
            QLabel other(&root);
            other.setObjectName(QStringLiteral("anotherLabel"));
            BoxElement::resolve(&named);
            BoxElement::resolve(&other);

            ASSERT_NE(BoxElement::forWidget(&named), nullptr);
            EXPECT_STREQ(BoxElement::forWidget(&named)->m_name, "LuaIDE.Box.PerforceStatus");
            const BoxElement* otherBox = BoxElement::forWidget(&other);
            EXPECT_TRUE(!otherBox || std::string(otherBox->m_name) != "LuaIDE.Box.PerforceStatus");
        }

        // ------------------------------------------------------------------------------
        // C078: the code view's colours
        // ------------------------------------------------------------------------------

        // Four backgrounds and two selection colours that no two states share, as the sheet is built from the user's settings
        const CodeViewColors TestColors{
            .m_focusedBackground = QColor(0x1A, 0x2B, 0x3C),
            .m_unfocusedBackground = QColor(0x3C, 0x1A, 0x2B),
            .m_readOnlyFocusedBackground = QColor(0x2B, 0x3C, 0x1A),
            .m_readOnlyUnfocusedBackground = QColor(0x4D, 0x4D, 0x1A),
            .m_selectedText = QColor(0xF0, 0xE0, 0xA0),
            .m_selectedBackground = QColor(0x80, 0x20, 0x20),
        };

        //! The sheet of LUAViewWidget::CreateStyleSheet as the site builds it, with the colours of TestColors (LUAEditorView.cpp:196-231
        //! before the flattening). It lives for the process: a Row holds the text by pointer.
        const char* codeViewSheet()
        {
            static const QByteArray sheet = QString(R"(QPlainTextEdit:focus
                                    {
                                        background-color: %1;
                                        selection-background-color:  %6;
                                        selection-color: %5;
                                    }

                                    QPlainTextEdit:!focus
                                    {
                                        background-color: %2;
                                        selection-color: %5;
                                        selection-background-color:  %6;
                                    }

                                    QPlainTextEdit[readOnly="true"]:focus
                                    {
                                        background-color: %3;
                                        selection-color: %5;
                                        selection-background-color:  %6;
                                    }

                                    QPlainTextEdit[readOnly="true"]:!focus
                                    {
                                        background-color: %4;
                                        selection-color: %5;
                                        selection-background-color:  %6;
                                    }
                                    )")
                                                .arg(TestColors.m_focusedBackground.name())
                                                .arg(TestColors.m_unfocusedBackground.name())
                                                .arg(TestColors.m_readOnlyFocusedBackground.name())
                                                .arg(TestColors.m_readOnlyUnfocusedBackground.name())
                                                .arg(TestColors.m_selectedText.name())
                                                .arg(TestColors.m_selectedBackground.name())
                                                .toLatin1();
            return sheet.constData();
        }

        //! LUAViewWidget::SetReadonly on the view: the read-only state, the polish again, and the palette made from the new state
        void setReadOnly(QWidget& widget, bool readOnly)
        {
            auto& view = static_cast<QPlainTextEdit&>(widget);
            view.setReadOnly(readOnly);
            view.style()->unpolish(&view);
            view.style()->polish(&view);
            if (StyleManager::flatStylingActive())
            {
                CodeViewPalette::Refresh(&view);
            }
        }

        Twins::State readOnlyState()
        {
            return { "read only", [](QWidget& widget) { setReadOnly(widget, true); }, [](QWidget& widget) { setReadOnly(widget, false); } };
        }

        Twins::State readOnlyFocusedState()
        {
            const Twins::State focus = Twins::States::focus();
            return { "read only and focus",
                [focus](QWidget& widget)
                {
                    setReadOnly(widget, true);
                    focus.m_enter(widget);
                },
                [focus](QWidget& widget)
                {
                    focus.m_leave(widget);
                    setReadOnly(widget, false);
                } };
        }

        //! A square of one colour that paints itself (no style), for the corners of the code view
        class CornerCover
            : public QWidget
        {
        public:
            using QWidget::QWidget;

        protected:
            void paintEvent(QPaintEvent*) override
            {
                QPainter(this).fillRect(rect(), QColor(0x80, 0x80, 0x80));
            }
        };

        //! The code view with its four corner squares painted over in both twins. The legacy field is rounded by 2px
        //! (BaseStyleSheet.qss:79) and the flattened idle field has always been square (judged in earlier A/Bs, Style.cpp
        //! g_textEditIdleRadius; ItemViewElementTests T12 pins the same deviation), so the corners of a field without a ring are left
        //! out of the comparison; everything else of the field is compared.
        class CornerCoveredTextEdit
            : public QPlainTextEdit
        {
        public:
            static constexpr int CoverSide = 3;     // the corner squares of ItemViewElementTests T12

            explicit CornerCoveredTextEdit(QWidget* parent)
                : QPlainTextEdit(parent)
            {
                for (CornerCover*& cover : m_covers)
                {
                    cover = new CornerCover(this);
                    cover->setAttribute(Qt::WA_TransparentForMouseEvents);
                    cover->resize(CoverSide, CoverSide);
                }
            }

        protected:
            void resizeEvent(QResizeEvent* event) override
            {
                QPlainTextEdit::resizeEvent(event);
                const int right = width() - CoverSide;
                const int bottom = height() - CoverSide;
                m_covers[0]->move(0, 0);
                m_covers[1]->move(right, 0);
                m_covers[2]->move(0, bottom);
                m_covers[3]->move(right, bottom);
                for (CornerCover* cover : m_covers)
                {
                    cover->raise();
                }
            }

        private:
            CornerCover* m_covers[4] = {};
        };

        Twins::Row codeViewRow(const char* site, std::vector<Twins::State> states, unsigned compare)
        {
            return Twins::Row{
                .m_site = site,
                .m_make = [](QWidget* parent) -> QWidget*
                {
                    // LUAViewWidget::CreateStyleSheet on the view, with some text selected so that the selection colours show
                    auto* view = new CornerCoveredTextEdit(parent);
                    view->setPlainText(QStringLiteral("local answer = 42\nprint(answer)"));
                    view->selectAll();
                    if (StyleManager::flatStylingActive())
                    {
                        CodeViewPalette::Apply(view, TestColors);
                    }
                    return view;
                },
                .m_sheets = { { "LUAEditorView.cpp:196-231", Twins::SetOn::Widget, codeViewSheet() } },
                .m_realApplicationSheet = true,
                .m_states = std::move(states),
                .m_compare = compare,
                .m_roles = { QPalette::Highlight, QPalette::HighlightedText },
                .m_size = QSize(240, 80),
                .m_base = Twins::Base::AzQtComponentsStyle,
            };
        }

        TEST_F(LuaIDEFlatElementsTests, C078_TheCodeViewHasTheBackgroundOfEachStateOfTheSheet)
        {
            ASSERT_TRUE(StyleManager::stylesheetsDisabled());
            EXPECT_TRUE(check(codeViewRow("C078 the code view, in each state of its focus and its read-only flag",
                                  { Twins::States::focus(), readOnlyState(), readOnlyFocusedState() },
                                  Twins::Compare::Geometry | Twins::Compare::Pixels),
                m_context));
        }

        TEST_F(LuaIDEFlatElementsTests, C078_TheCodeViewHasTheSelectionColoursOfTheSheet)
        {
            ASSERT_TRUE(StyleManager::stylesheetsDisabled());
            EXPECT_TRUE(check(codeViewRow("C078 the selection colours of the code view",
                                  { Twins::States::focus(), readOnlyState() },
                                  Twins::Compare::Palette),
                m_context));
        }

        TEST_F(LuaIDEFlatElementsTests, CodeViewPalette_ApplyAgainKeepsOneObjectAndTheNewColours)
        {
            const CodeViewColors other{
                .m_focusedBackground = QColor(1, 2, 3),
                .m_unfocusedBackground = QColor(4, 5, 6, 128),    // the sheet carried #rrggbb: no alpha
                .m_readOnlyFocusedBackground = QColor(7, 8, 9),
                .m_readOnlyUnfocusedBackground = QColor(10, 11, 12),
                .m_selectedText = QColor(13, 14, 15),
                .m_selectedBackground = QColor(16, 17, 18),
            };

            QPlainTextEdit view;
            CodeViewPalette::Apply(&view, TestColors);
            CodeViewPalette::Apply(&view, other);

            int objects = 0;
            for (const QObject* child : view.children())
            {
                objects += dynamic_cast<const CodeViewPalette*>(child) ? 1 : 0;
            }
            EXPECT_EQ(objects, 1);

            // Not read only and not focused: the `:!focus` rule
            for (const QPalette::ColorGroup group : { QPalette::Active, QPalette::Inactive, QPalette::Disabled })
            {
                EXPECT_EQ(view.palette().color(group, QPalette::Base), QColor(4, 5, 6));
                EXPECT_EQ(view.palette().color(group, QPalette::Highlight), QColor(16, 17, 18));
                EXPECT_EQ(view.palette().color(group, QPalette::HighlightedText), QColor(13, 14, 15));
            }

            // Read only: the `[readOnly="true"]:!focus` rule, once the view's polish has been followed by a refresh
            view.setReadOnly(true);
            CodeViewPalette::Refresh(&view);
            EXPECT_EQ(view.palette().color(QPalette::Active, QPalette::Base), QColor(10, 11, 12));
        }

        TEST_F(LuaIDEFlatElementsTests, CodeViewPalette_APaletteTheStyleWritesIsPutRightAgain)
        {
            QPlainTextEdit view;
            CodeViewPalette::Apply(&view, TestColors);

            // What Style::polish does to a text edit: its own Base and selection roles over the palette
            QPalette written = view.palette();
            for (const QPalette::ColorGroup group : { QPalette::Active, QPalette::Inactive, QPalette::Disabled })
            {
                written.setColor(group, QPalette::Base, QColor(0xE9, 0xE9, 0xE9));
                written.setColor(group, QPalette::Highlight, QColor(0x00, 0x00, 0xFF));
            }
            view.setPalette(written);

            EXPECT_EQ(view.palette().color(QPalette::Active, QPalette::Base), TestColors.m_unfocusedBackground);
            EXPECT_EQ(view.palette().color(QPalette::Active, QPalette::Highlight), TestColors.m_selectedBackground);
        }

        TEST_F(LuaIDEFlatElementsTests, CodeViewPalette_RefreshOfAViewWithNoColoursDoesNothing)
        {
            QPlainTextEdit view;
            const QPalette before = view.palette();
            CodeViewPalette::Refresh(&view);
            EXPECT_TRUE(view.palette() == before);
        }

        // ------------------------------------------------------------------------------
        // C079: the fold gutter
        // ------------------------------------------------------------------------------

        // What SyntaxStyleSettings gives the gutter, enabled and not
        const QColor GutterEnabled(0x4A, 0x6A, 0x8A);
        const QColor GutterDisabled(0x8A, 0x6A, 0x4A);

        //! The sheet of LUAViewWidget::CreateStyleSheet for the gutter, with the colours above (LUAEditorView.cpp:246 before the
        //! flattening)
        constexpr const char* FoldingSheet =
            "LUAEditor--FoldingWidget:enabled { background-color: #4a6a8a; }\n"
            "LUAEditor--FoldingWidget:!enabled { background-color: #8a6a4a; }\n";

        //! The calls of LUAEditor::FoldingWidget::paintEvent that make its background: the fill of the flattened GUI by the enabled
        //! state, and the style's PE_Widget, under which the legacy GUI's sheet fills it
        class FoldingGutterStandIn
            : public QWidget
        {
        public:
            using QWidget::QWidget;

        protected:
            void paintEvent(QPaintEvent*) override
            {
                QStyleOption option;
                option.initFrom(this);
                QPainter painter(this);
                if (StyleManager::flatStylingActive())
                {
                    painter.fillRect(rect(), QColor((isEnabled() ? GutterEnabled : GutterDisabled).rgb()));
                }
                style()->drawPrimitive(QStyle::PE_Widget, &option, &painter, this);

                // The fold marks the class draws over its background (a square and a line): the picture is more than one colour, which
                // the harness takes for a twin that drew nothing
                painter.setPen(Qt::white);
                painter.drawRect(QRect(3, 10, 8, 8));
                painter.drawLine(7, 18, 7, rect().bottom());
            }
        };

        Twins::State enabledState()
        {
            return { "enabled", [](QWidget& widget) { widget.setEnabled(true); }, [](QWidget& widget) { widget.setEnabled(false); } };
        }

        TEST_F(LuaIDEFlatElementsTests, C079_TheFoldGutterIsFilledByItsEnabledStateAsTheSheetFilledIt)
        {
            ASSERT_TRUE(StyleManager::stylesheetsDisabled());
            const Twins::Row row{
                .m_site = "C079 the fold gutter, disabled as the class makes it and enabled",
                .m_make = [](QWidget* parent) -> QWidget*
                {
                    auto* gutter = new FoldingGutterStandIn(parent);
                    gutter->setObjectName(QStringLiteral("luaFoldingGutter"));
                    gutter->setEnabled(false);      // LUAEditorFoldingWidget.cpp:26
                    return gutter;
                },
                .m_sheets = { { "LUAEditorView.cpp:246", Twins::SetOn::Widget, FoldingSheet } },
                .m_realApplicationSheet = true,
                .m_states = { enabledState() },
                .m_compare = Twins::Compare::Geometry | Twins::Compare::Pixels,
                .m_size = QSize(16, 80),
                .m_base = Twins::Base::AzQtComponentsStyle,
                .m_replacements = { { "LUAEditor--FoldingWidget", "#luaFoldingGutter" } },
            };
            EXPECT_TRUE(check(row, m_context));
        }
    } // namespace FlatElementsTestsInternal
} // namespace LUAEditor
