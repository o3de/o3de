/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzTest/AzTest.h>
#include <AzCore/UnitTest/TestTypes.h>

#include <AzQtComponents/Components/Elements/BoxElement.h>
#include <AzQtComponents/Components/Elements/ElementCommon.h>
#include <AzQtComponents/Components/Elements/TextElement.h>
#include <AzQtComponents/Components/Style.h>
#include <AzQtComponents/Components/StyleManager.h>
#include <AzQtComponents/Tests/SheetTwins.h>

#include <Components/Nodes/Comment/CommentTextEditFlat.h>

#include <QApplication>
#include <QColor>
#include <QGraphicsProxyWidget>
#include <QGraphicsScene>
#include <QPen>
#include <QTextEdit>
#include <QWidget>

#include <string>

namespace UnitTest
{
    // ----------------------------------------------------------------------------------
    // C156, CommentTextGraphicsWidget.cpp: the editor a comment or a group title is renamed in. UpdateStyles builds a sheet for it from the
    // style the node resolved; the legacy GUI sets that sheet, and the flattened GUI tags the editor (CommentTextEditFlat::TagEditor), so
    // that the elements of CommentTextEditFlat draw it: a clear field with a ring (2px #f3811d, 2px #17A3CD in Landscape Canvas, and the
    // 1px black of GetPen's fallbacks for the group title there) and black text, the comment's italic and 120% of the default size (14px
    // at the application font's 12px), a group title's the default.
    //
    // The home of the looks compiles into this test, which cannot link the module that holds the editor (FocusableTextEdit, and the rows
    // CommentTextGraphicsWidget::RegisterFlatElements registers on it). The twins are a plain QTextEdit that this test registers the same
    // elements on, with matchers of its own that read the same tags: the legacy twin is Qt's own style sheet style with the sheet quoted
    // below on the editor, the flat twin the flattened Style with the tags the site sets and no sheet.
    //
    // The roles compared are the text's: the legacy sheet's fill made the Base, Button and Window of the editor clear, which nothing
    // paints through here (the box of the element draws the clear field itself), and the Pixels of every row say so. The rows have the
    // application's real sheet, as the editor has it in every tool: it gives the scroll bars their extent, which the flat Style has too
    // (without it the legacy scroll bars are 14px and the minimum size hint of the editor 4px larger). The minimum size hint is left out
    // of the comparison (checkEditor): a legacy cascade side effect that reaches nothing drawn.
    // ----------------------------------------------------------------------------------

    namespace CommentTextEditFlatSheetTestsInternal
    {
        namespace Twins = AzQtComponents::SheetTwins;
        namespace Looks = GraphCanvas::CommentTextEditFlat;

        class CommentTextEditFlatSheetTests
            : public LeakDetectionFixture
        {
        protected:
            Twins::Context m_context;
        };

        // The sheets UpdateStyles sets for the pens the shipped sheets give, as it writes them (CommentTextGraphicsWidget.cpp:149-174
        // before the flattening, with the fields of StyleHelper::GetFontStyleSheet): the ring, and the font of the kind of text
        constexpr const char* CommentOrangeSheet =
            "background-color: rgba(0,0,0,0); border-width: 2; border-style: solid; border-color: rgba(243,129,29,255); border-radius: 0; "
            "margin: 0; padding: 0; color: rgba(0,0,0,255); font-family: Open Sans; font-size: 14px; font-style: italic";
        constexpr const char* CommentBlueSheet =
            "background-color: rgba(0,0,0,0); border-width: 2; border-style: solid; border-color: rgba(23,163,205,255); border-radius: 0; "
            "margin: 0; padding: 0; color: rgba(0,0,0,255); font-family: Open Sans; font-size: 14px; font-style: italic";
        constexpr const char* GroupTitleOrangeSheet =
            "background-color: rgba(0,0,0,0); border-width: 2; border-style: solid; border-color: rgba(243,129,29,255); border-radius: 0; "
            "margin: 0; padding: 0; color: rgba(0,0,0,255); font-family: Open Sans; font-size: 12px";
        constexpr const char* GroupTitleFallbackSheet =
            "background-color: rgba(0,0,0,0); border-width: 1; border-style: solid; border-color: rgba(0,0,0,255); border-radius: 0; "
            "margin: 0; padding: 0; color: rgba(0,0,0,255); font-family: Open Sans; font-size: 12px";

        //! The pen StyleHelper::GetBorder gives: the width, the colour and the line the style resolved
        QPen ringPen(int width, const QColor& color, Qt::PenStyle line = Qt::SolidLine)
        {
            QPen pen;
            pen.setWidth(width);
            pen.setColor(color);
            pen.setStyle(line);
            return pen;
        }

        QPen orangePen()
        {
            return ringPen(Looks::RingWidth, Looks::RingOrange);
        }

        QPen bluePen()
        {
            return ringPen(Looks::RingWidth, Looks::RingBlue);
        }

        QPen fallbackPen()
        {
            return ringPen(Looks::RingFallbackWidth, Looks::RingFallback);
        }

        // ------------------------------------------------------------------------------
        // The rows of the editor, as CommentTextGraphicsWidget::RegisterFlatElements registers them, on a plain text edit
        // ------------------------------------------------------------------------------

        bool hasTag(const QWidget* widget, const char* tag)
        {
            return AzQtComponents::Selectors::tagged(widget, tag);
        }

        void registerEditorRows()
        {
            const QMetaObject* editor = &QTextEdit::staticMetaObject;
            AzQtComponents::BoxElement::registerElement(&Looks::OrangeRingBox, editor,
                [](const QWidget* widget) { return hasTag(widget, Looks::RingOrangeTag); });
            AzQtComponents::BoxElement::registerElement(&Looks::BlueRingBox, editor,
                [](const QWidget* widget) { return hasTag(widget, Looks::RingBlueTag); });
            AzQtComponents::BoxElement::registerElement(&Looks::FallbackRingBox, editor,
                [](const QWidget* widget) { return hasTag(widget, Looks::RingFallbackTag); });
            AzQtComponents::TextElement::registerElement(&Looks::CommentText, editor,
                [](const QWidget* widget) { return hasTag(widget, Looks::CommentTag); });
            AzQtComponents::TextElement::registerElement(&Looks::GroupTitleText, editor,
                [](const QWidget* widget) { return hasTag(widget, Looks::GroupTitleTag); });
        }

        //! The editor as SetupProxyWidget makes it, and tagged as UpdateStyles tags it in the flattened GUI
        QTextEdit* makeEditor(QWidget* parent, const QPen& ring, Looks::Kind kind)
        {
            auto* editor = new QTextEdit(parent);
            editor->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
            editor->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
            editor->setPlainText(QStringLiteral("A note on the graph"));
            if (AzQtComponents::StyleManager::flatStylingActive())
            {
                Looks::TagEditor(editor, ring, kind);
            }
            return editor;
        }

        Twins::Row editorRow(const char* site, const char* sheet, const QPen& ring, Looks::Kind kind)
        {
            Twins::Row row;
            row.m_site = site;
            row.m_make = [ring, kind](QWidget* parent)
            {
                return makeEditor(parent, ring, kind);
            };
            row.m_sheets = { { "CommentTextGraphicsWidget.cpp:149-174 before the flattening", Twins::SetOn::Widget, sheet } };
            row.m_realApplicationSheet = true;      // the editor is under the application's sheet in every tool, its scroll bars included
            row.m_register = &registerEditorRows;
            row.m_compare = Twins::Compare::Geometry | Twins::Compare::Fonts | Twins::Compare::Palette | Twins::Compare::Pixels;
            row.m_roles = { QPalette::Text, QPalette::WindowText, QPalette::ButtonText };
            row.m_size = QSize(160, 60);
            row.m_base = Twins::Base::AzQtComponentsStyle;
            return row;
        }

        //! check() for a row of the editor, but for the minimum size hint of the editor, which is the one geometry fact left out. A text
        //! edit's minimum size hint is the hints of its scroll bars plus its frame. The legacy sheet has no selector, so its border also
        //! reaches the scroll bars, which are always off and so never drawn, and inflates their hints: the legacy hint is 56 + 6 * w px
        //! for a ring of w px (62, 68 and 74 for 1, 2 and 3 px), the flat Style's 64 + 2 * w (the scroll bars have their own extent),
        //! and the two meet only at 2px. No drawn size reads it: the editor sits in a proxy that takes the widget's own minimum size
        //! (Proxy_TakesTheMinimumSizeOfTheEditorAndNeverItsHint), which UpdateSizing sets. A cascade side effect that changes nothing
        //! the user sees is not carried (D.17). Every other fact and picture of the row stays compared.
        ::testing::AssertionResult checkEditor(const Twins::Row& row, Twins::Context& context)
        {
            const Twins::Result result = Twins::run(row, context);
            std::string unexplained;
            for (const std::string& flaw : result.m_flaws)
            {
                unexplained += flaw + "\n";
            }
            for (const std::string& difference : result.m_differences)
            {
                if (!Twins::namesDifference(difference, "geometry.minimumSizeHint"))
                {
                    unexplained += difference + "\n";
                }
            }
            if (unexplained.empty())
            {
                return ::testing::AssertionSuccess();
            }
            return ::testing::AssertionFailure() << "the twins of " << row.m_site << " differ:\n" << unexplained;
        }
    } // namespace CommentTextEditFlatSheetTestsInternal

    using CommentTextEditFlatSheetTestsInternal::CommentTextEditFlatSheetTests;
    namespace CommentEditor = CommentTextEditFlatSheetTestsInternal;

    // ----------------------------------------------------------------------------------
    // Every look against the sheet it replaces, at every device ratio
    // ----------------------------------------------------------------------------------

    TEST_F(CommentTextEditFlatSheetTests, Comment_WithTheOrangeRingIsTheClearItalicFieldOfTheSheetAtEveryRatio)
    {
        EXPECT_TRUE(CommentEditor::checkEditor(
            CommentEditor::editorRow("C156 a comment, orange ring", CommentEditor::CommentOrangeSheet, CommentEditor::orangePen(),
                CommentEditor::Looks::Kind::Comment),
            m_context));
    }

    TEST_F(CommentTextEditFlatSheetTests, Comment_WithTheBlueRingOfLandscapeCanvasIsTheClearItalicFieldOfTheSheetAtEveryRatio)
    {
        EXPECT_TRUE(CommentEditor::checkEditor(
            CommentEditor::editorRow("C156 a comment, blue ring", CommentEditor::CommentBlueSheet, CommentEditor::bluePen(),
                CommentEditor::Looks::Kind::Comment),
            m_context));
    }

    TEST_F(CommentTextEditFlatSheetTests, GroupTitle_WithTheOrangeRingIsTheClearUprightFieldOfTheSheetAtEveryRatio)
    {
        EXPECT_TRUE(CommentEditor::checkEditor(
            CommentEditor::editorRow("C156 a group title, orange ring", CommentEditor::GroupTitleOrangeSheet, CommentEditor::orangePen(),
                CommentEditor::Looks::Kind::GroupTitle),
            m_context));
    }

    TEST_F(CommentTextEditFlatSheetTests, GroupTitle_WithTheFallbackRingOfLandscapeCanvasIsTheClearUprightFieldOfTheSheetAtEveryRatio)
    {
        EXPECT_TRUE(CommentEditor::checkEditor(
            CommentEditor::editorRow("C156 a group title, the ring of the fallbacks", CommentEditor::GroupTitleFallbackSheet,
                CommentEditor::fallbackPen(), CommentEditor::Looks::Kind::GroupTitle),
            m_context));
    }

    // ----------------------------------------------------------------------------------
    // What the editor's minimum size hint reaches: the proxy of the real editor takes the widget's own minimum size
    // ----------------------------------------------------------------------------------

    TEST_F(CommentTextEditFlatSheetTests, Proxy_TakesTheMinimumSizeOfTheEditorAndNeverItsHint)
    {
        QGraphicsScene scene;

        // A comment: UpdateSizing gives the editor a minimum and a maximum size of the label's preferred size (a 100 x 40 label at least)
        auto* comment = new QTextEdit();
        comment->setMinimumSize(QSize(100, 40));
        comment->setMaximumSize(QSize(100, 40));
        auto* commentProxy = new QGraphicsProxyWidget();
        commentProxy->setWidget(comment);
        scene.addItem(commentProxy);
        ASSERT_NE(comment->minimumSizeHint(), QSize(100, 40));
        EXPECT_EQ(commentProxy->effectiveSizeHint(Qt::MinimumSize), QSizeF(100, 40));

        // A group title: a minimum and a maximum height only, the width is the group's
        auto* group = new QTextEdit();
        group->setMinimumHeight(30);
        group->setMaximumHeight(30);
        auto* groupProxy = new QGraphicsProxyWidget();
        groupProxy->setWidget(group);
        scene.addItem(groupProxy);
        ASSERT_GT(group->minimumSizeHint().width(), 0);
        EXPECT_EQ(groupProxy->effectiveSizeHint(Qt::MinimumSize).width(), 0);
        EXPECT_EQ(groupProxy->effectiveSizeHint(Qt::MinimumSize).height(), 30);
    }

    // ----------------------------------------------------------------------------------
    // The choice of the looks: the tag of the pen the style resolved
    // ----------------------------------------------------------------------------------

    TEST_F(CommentTextEditFlatSheetTests, RingTagOf_NamesEachShippedRingByItsWidthColourAndLine)
    {
        EXPECT_STREQ(CommentEditor::Looks::RingTagOf(CommentEditor::orangePen()), CommentEditor::Looks::RingOrangeTag);
        EXPECT_STREQ(CommentEditor::Looks::RingTagOf(CommentEditor::bluePen()), CommentEditor::Looks::RingBlueTag);
        EXPECT_STREQ(CommentEditor::Looks::RingTagOf(CommentEditor::fallbackPen()), CommentEditor::Looks::RingFallbackTag);
    }

    TEST_F(CommentTextEditFlatSheetTests, RingTagOf_NamesNoneForARingNoShippedSheetGives)
    {
        namespace Looks = CommentEditor::Looks;

        // A width that is not the colour's own, a colour that is none of the three, a translucent shipped colour, and the other lines
        EXPECT_EQ(Looks::RingTagOf(CommentEditor::ringPen(1, Looks::RingOrange)), nullptr);
        EXPECT_EQ(Looks::RingTagOf(CommentEditor::ringPen(3, Looks::RingBlue)), nullptr);
        EXPECT_EQ(Looks::RingTagOf(CommentEditor::ringPen(Looks::RingWidth, Looks::RingFallback)), nullptr);
        EXPECT_EQ(Looks::RingTagOf(CommentEditor::ringPen(Looks::RingFallbackWidth, Looks::RingBlue)), nullptr);
        EXPECT_EQ(Looks::RingTagOf(CommentEditor::ringPen(Looks::RingWidth, QColor(0x12, 0x34, 0x56))), nullptr);
        EXPECT_EQ(Looks::RingTagOf(CommentEditor::ringPen(Looks::RingWidth, QColor(0xF3, 0x81, 0x1D, 0x80))), nullptr);
        EXPECT_EQ(Looks::RingTagOf(CommentEditor::ringPen(Looks::RingWidth, Looks::RingOrange, Qt::DashLine)), nullptr);
        EXPECT_EQ(Looks::RingTagOf(CommentEditor::ringPen(Looks::RingWidth, Looks::RingBlue, Qt::DotLine)), nullptr);
        EXPECT_EQ(Looks::RingTagOf(CommentEditor::ringPen(Looks::RingFallbackWidth, Looks::RingFallback, Qt::NoPen)), nullptr);
    }

    TEST_F(CommentTextEditFlatSheetTests, KindTagOf_NamesTheTextOfEachKind)
    {
        EXPECT_STREQ(CommentEditor::Looks::KindTagOf(CommentEditor::Looks::Kind::Comment), CommentEditor::Looks::CommentTag);
        EXPECT_STREQ(CommentEditor::Looks::KindTagOf(CommentEditor::Looks::Kind::GroupTitle), CommentEditor::Looks::GroupTitleTag);
    }

    TEST_F(CommentTextEditFlatSheetTests, TagEditor_GivesTheEditorTheTagsOfItsLooksAndTakesAwayTheOthers)
    {
        namespace Looks = CommentEditor::Looks;
        CommentEditor::Twins::RungScope flat(CommentEditor::Twins::Rung::Flat);

        QWidget root;
        auto* editor = new QTextEdit(&root);

        const char* const rings[] = { Looks::RingOrangeTag, Looks::RingBlueTag, Looks::RingFallbackTag };
        const auto onlyRing = [&](const char* expected)
        {
            for (const char* ring : rings)
            {
                EXPECT_EQ(CommentEditor::hasTag(editor, ring), expected && qstrcmp(ring, expected) == 0) << ring;
            }
        };

        Looks::TagEditor(editor, CommentEditor::orangePen(), Looks::Kind::Comment);
        onlyRing(Looks::RingOrangeTag);
        EXPECT_TRUE(CommentEditor::hasTag(editor, Looks::CommentTag));
        EXPECT_FALSE(CommentEditor::hasTag(editor, Looks::GroupTitleTag));

        // The style changes (a reload of the sheet): the ring tag follows it
        Looks::TagEditor(editor, CommentEditor::bluePen(), Looks::Kind::Comment);
        onlyRing(Looks::RingBlueTag);

        // A pen that is none of the three takes the ring tags away and leaves the text's
        Looks::TagEditor(editor, CommentEditor::ringPen(5, QColor(0x12, 0x34, 0x56)), Looks::Kind::Comment);
        onlyRing(nullptr);
        EXPECT_TRUE(CommentEditor::hasTag(editor, Looks::CommentTag));

        // The group title of Landscape Canvas
        Looks::TagEditor(editor, CommentEditor::fallbackPen(), Looks::Kind::GroupTitle);
        onlyRing(Looks::RingFallbackTag);
        EXPECT_TRUE(CommentEditor::hasTag(editor, Looks::GroupTitleTag));
        EXPECT_FALSE(CommentEditor::hasTag(editor, Looks::CommentTag));

        // Tagging again with the same pen changes nothing (the class text keeps one copy of each tag)
        const QVariant before = editor->property("class");
        Looks::TagEditor(editor, CommentEditor::fallbackPen(), Looks::Kind::GroupTitle);
        EXPECT_EQ(editor->property("class"), before);
    }

    // ----------------------------------------------------------------------------------
    // The elements: what each look holds, and that the tags reach them and nothing else
    // ----------------------------------------------------------------------------------

    TEST_F(CommentTextEditFlatSheetTests, Elements_HoldTheRingsAndTheTextOfTheShippedSheets)
    {
        namespace Looks = CommentEditor::Looks;
        const struct
        {
            const AzQtComponents::BoxElement* m_box;
            int m_width;
            QColor m_color;
        } rings[] = {
            { &Looks::OrangeRingBox, 2, QColor(0xF3, 0x81, 0x1D) },
            { &Looks::BlueRingBox, 2, QColor(0x17, 0xA3, 0xCD) },
            { &Looks::FallbackRingBox, 1, QColor(0x00, 0x00, 0x00) },
        };
        for (const auto& ring : rings)
        {
            const AzQtComponents::BoxElement::Edges& edges = ring.m_box->m_idle.m_edges;
            for (const AzQtComponents::BoxElement::Edge* edge : { &edges.m_top, &edges.m_right, &edges.m_bottom, &edges.m_left })
            {
                EXPECT_EQ(edge->m_width, ring.m_width) << ring.m_box->m_name;
                EXPECT_EQ(edge->m_color, ring.m_color) << ring.m_box->m_name;
                EXPECT_EQ(edge->m_style, AzQtComponents::BoxElement::LineStyle::Solid) << ring.m_box->m_name;
            }
            EXPECT_FALSE(ring.m_box->m_nativeBorder) << ring.m_box->m_name;
            EXPECT_TRUE(ring.m_box->m_hasBox) << ring.m_box->m_name;
            EXPECT_EQ(ring.m_box->m_radius, 0) << ring.m_box->m_name;
            EXPECT_EQ(ring.m_box->m_idle.m_fill.color().alpha(), 0) << ring.m_box->m_name;
        }

        EXPECT_EQ(Looks::CommentText.m_pixelSize, 14);
        EXPECT_EQ(Looks::CommentText.m_italic, 1);
        EXPECT_EQ(Looks::GroupTitleText.m_pixelSize, 12);
        EXPECT_EQ(Looks::GroupTitleText.m_italic, -1);
        EXPECT_EQ(Looks::CommentText.m_color, QColor(0, 0, 0));
        EXPECT_EQ(Looks::GroupTitleText.m_color, QColor(0, 0, 0));
    }

    TEST_F(CommentTextEditFlatSheetTests, Elements_AreThoseOfTheEditorsTagsAndOfNoOtherTextEdit)
    {
        namespace Looks = CommentEditor::Looks;
        CommentEditor::Twins::RungScope flat(CommentEditor::Twins::Rung::Flat);
        CommentEditor::Twins::Context::registerOnce(&CommentEditor::registerEditorRows);

        QWidget root;
        QTextEdit* group = CommentEditor::makeEditor(&root, CommentEditor::fallbackPen(), Looks::Kind::GroupTitle);
        QTextEdit* comment = CommentEditor::makeEditor(&root, CommentEditor::orangePen(), Looks::Kind::Comment);
        QTextEdit* blue = CommentEditor::makeEditor(&root, CommentEditor::bluePen(), Looks::Kind::Comment);
        auto* plain = new QTextEdit(&root);

        for (QTextEdit* each : { group, comment, blue, plain })
        {
            each->setStyle(m_context.flatStyle());
            each->ensurePolished();
        }

        // The ring of the pen picks the box
        const AzQtComponents::BoxElement* groupBox = AzQtComponents::BoxElement::forWidget(group);
        const AzQtComponents::BoxElement* commentBox = AzQtComponents::BoxElement::forWidget(comment);
        const AzQtComponents::BoxElement* blueBox = AzQtComponents::BoxElement::forWidget(blue);
        ASSERT_NE(groupBox, nullptr);
        ASSERT_NE(commentBox, nullptr);
        ASSERT_NE(blueBox, nullptr);
        EXPECT_EQ(groupBox, &Looks::FallbackRingBox);
        EXPECT_EQ(commentBox, &Looks::OrangeRingBox);
        EXPECT_EQ(blueBox, &Looks::BlueRingBox);
        EXPECT_EQ(AzQtComponents::BoxElement::forWidget(plain), nullptr);

        // The kind picks the text: a comment's is italic at 14px, a group title's upright at 12px
        EXPECT_TRUE(comment->font().italic());
        EXPECT_EQ(comment->font().pixelSize(), 14);
        EXPECT_FALSE(group->font().italic());
        EXPECT_EQ(group->font().pixelSize(), 12);
        EXPECT_FALSE(plain->font().italic());

        // A ring no shipped sheet gives: no box, whatever text
        Looks::TagEditor(group, CommentEditor::ringPen(5, QColor(0x12, 0x34, 0x56)), Looks::Kind::GroupTitle);
        EXPECT_EQ(AzQtComponents::BoxElement::forWidget(group), nullptr);
    }
} // namespace UnitTest
