/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/PlatformDef.h>

AZ_PUSH_DISABLE_WARNING(4251 4800 4244, "-Wunknown-warning-option")
#include <QColor>
#include <QPen>
#include <QWidget>
AZ_POP_DISABLE_WARNING

#include <AzQtComponents/Components/Elements/BoxElement.h>
#include <AzQtComponents/Components/Elements/Foundations.h>
#include <AzQtComponents/Components/Elements/TextElement.h>

//=========================================================================
// COMMENT EDITOR LOOKS (the editor a comment or a group title is renamed in)
//
// CommentTextGraphicsWidget::UpdateStyles builds a sheet for the editor from the style the node resolved, which the canvas tools give
// in their JSON, and the legacy GUI sets it. The flattened GUI sets none: it tags the editor (TagEditor), and the elements below,
// picked by those tags on the editor's own class (CommentTextGraphicsWidget::RegisterFlatElements), draw what the sheet drew. The
// editor is transparent over the note, with a ring, no margin, padding or radius, and black text in the style's font: the comment's is
// italic and 120% of the default size, a group title's is the default.
//
// This home depends on Qt and AzQtComponents only, so that the tests of GraphCanvas can compile it and twin every look.
//
// The rings are the three the shipped tool sheets give the editor while it is edited (the sheets are those of Material Canvas, Pass
// Canvas, Script Canvas and Landscape Canvas; every GraphCanvas tool in the tree loads one of them):
//   - 2px solid #f3811d: the comment and the group title (`nodeGroup`) of Material Canvas, Pass Canvas and Script Canvas;
//   - 2px solid #17A3CD: the comment of Landscape Canvas;
//   - 1px solid black: the group title of Landscape Canvas, whose sheet has no rule for `nodeGroup` (it has `blockComment`, which no
//     node is styled by), so that StyleHelper::GetPen gives the editing `.commentText` its fallbacks.
// No shipped sheet gives the ring a dashed or dotted line. A ring that a user's own sheet gives beyond these three is run-time data that
// no static look can hold: RingTagOf names none for it, the editor has no box here and is the field of any text edit, and
// only its text look is carried.
//=========================================================================

namespace GraphCanvas
{
    //! Everything of the comment editor's flattened looks. A named namespace: a unity build joins files into one translation unit.
    namespace CommentTextEditFlat
    {
        // ---- Values

        // The tool sheets are the JSON files materialcanvas_style.json, passcanvas_style.json, ScriptCanvas graphcanvas_style.json and
        // LandscapeCanvas graphcanvas_style.json. The first, the second and the fourth have the rules of a comment at the same lines;
        // Script Canvas's has them 30 lines later (+30).

        //! `border-color` of `comment:editing > .commentText` (materialcanvas_style.json:277, passcanvas_style.json:277, ScriptCanvas
        //! graphcanvas_style.json:307) and of `nodeGroup:editing > .commentText` (:333, :333, :363).
        //! TOKENIZE: CommentEditorRingOrange
        inline constexpr QColor RingOrange(0xF3, 0x81, 0x1D);

        //! `border-color` of `comment:editing > .commentText` in Landscape Canvas (LandscapeCanvas graphcanvas_style.json:277).
        //! TOKENIZE: CommentEditorRingBlue
        inline constexpr QColor RingBlue(0x17, 0xA3, 0xCD);

        //! The colour of the ring where the style gives the editing `.commentText` none: GetPen's fallback, QColor(Qt::black)
        //! (StyleHelper.cpp:226). TOKENIZE: CommentEditorRingFallback
        inline constexpr QColor RingFallback(0x00, 0x00, 0x00);

        //! `border-width` of the same rules (:275 and :331, +30 in Script Canvas's; Landscape Canvas's :275).
        //! TOKENIZE: CommentEditorRingWidth
        inline constexpr int RingWidth = 2;

        //! The width of the ring where the style gives none: GetPen's fallback (StyleHelper.cpp:227).
        //! TOKENIZE: CommentEditorRingFallbackWidth
        inline constexpr int RingFallbackWidth = 1;

        //! `color` of `comment > .commentText` (:255, +30 in Script Canvas's). A group title's has none: GetFontStyleSheet prints the
        //! unset colour GetColor gives as `rgba(0,0,0,255)` (StyleHelper.cpp:178-182), the same black. TOKENIZE: CommentEditorText
        inline constexpr QColor TextColor(0x00, 0x00, 0x00);

        //! The size of a group title's text: GetFont gives it the pixel size of the default font (StyleHelper.cpp:163-167), which is the
        //! application font's, Foundations::Surface::Size (StyleManager.cpp:352-354). TOKENIZE: CommentEditorGroupTitleSize
        inline constexpr int GroupTitleFontSize = AzQtComponents::Foundations::Surface::Size;

        //! `font-size: 120%` of `comment > .commentText` (:258, +30 in Script Canvas's): 120% of the default font's pixel size, which is
        //! GroupTitleFontSize, cut to a whole pixel as Parser.cpp:776-777 does (int(12 * 1.2) is 14). TOKENIZE: CommentEditorCommentSize
        inline constexpr int CommentFontSize = static_cast<int>(GroupTitleFontSize * 1.2);

        // ---- The class tags the editor takes in the flattened GUI, which the matchers of the rows read: the ring, and the kind of text

        inline constexpr const char* RingOrangeTag = "CommentEditorRingOrange";
        inline constexpr const char* RingBlueTag = "CommentEditorRingBlue";
        inline constexpr const char* RingFallbackTag = "CommentEditorRingFallback";
        inline constexpr const char* CommentTag = "CommentEditorComment";
        inline constexpr const char* GroupTitleTag = "CommentEditorGroupTitle";

        // ---- The elements (CommentTextEditFlat.cpp)

        extern const AzQtComponents::BoxElement OrangeRingBox;      //!< the 2px #f3811d ring
        extern const AzQtComponents::BoxElement BlueRingBox;        //!< the 2px #17A3CD ring
        extern const AzQtComponents::BoxElement FallbackRingBox;    //!< the 1px black ring
        extern const AzQtComponents::TextElement CommentText;       //!< the text of a comment
        extern const AzQtComponents::TextElement GroupTitleText;    //!< the text of a group title

        // ---- The choice of the looks

        //! What an editor holds text of
        enum class Kind
        {
            Comment,
            GroupTitle,
        };

        //! The tag of the ring look the pen the style resolved (StyleHelper::GetBorder) has: its width, colour and line must be those of
        //! one of the three rings above. Null: the pen is none of them.
        const char* RingTagOf(const QPen& ring);

        //! The tag of the text look of a kind
        const char* KindTagOf(Kind kind);

        //! Tags the editor with the look of its ring and of its text, and takes away the tags of the others: a tag the editor has already,
        //! or lacks already, polishes nothing. Flattened GUI only.
        void TagEditor(QWidget* editor, const QPen& ring, Kind kind);
    } // namespace CommentTextEditFlat
} // namespace GraphCanvas
