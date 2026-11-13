/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Components/Nodes/Comment/CommentTextEditFlat.h>

#include <AzQtComponents/Components/Style.h>

namespace GraphCanvas
{
    namespace CommentTextEditFlat
    {
        // ---- The boxes of the editor: the field, which is clear, and the ring around it

        // The sheet UpdateStyles sets for the legacy GUI (CommentTextGraphicsWidget.cpp:149-172 before the flattening) gives every ring
        // the same field and box: a clear fill (:149) in place of the field of a text edit, a border of its own (:151-168) of the pen's
        // width, line and colour, a radius of 0 (:169; the style's `border-radius`, :278 and :334, +30 in Script Canvas's, and the
        // fallback's own 0), and the margin and padding of 0 (:171-172) which the sheet declares.

        //! The box with the orange ring of Material Canvas, Pass Canvas and Script Canvas
        const AzQtComponents::BoxElement OrangeRingBox{
            .m_name = "GraphCanvas.Box.CommentEditorOrange",
            .m_hasBox = true,                                                           // margin: 0, padding: 0: :171-172
            .m_nativeBorder = false,                                                    // border-width, -style, -color: :151-168
            .m_radius = 0,                                                              // border-radius: 0: :169
            .m_idle = {
                .m_fill = QBrush(AzQtComponents::Foundations::Clear),                   // background-color: rgba(0,0,0,0): :149
                .m_edges = AzQtComponents::BoxElement::border(RingWidth, RingOrange),   // border: :151-168
            },
        };

        //! The box with the blue ring of the comment of Landscape Canvas
        const AzQtComponents::BoxElement BlueRingBox{
            .m_name = "GraphCanvas.Box.CommentEditorBlue",
            .m_hasBox = true,                                                           // margin: 0, padding: 0: :171-172
            .m_nativeBorder = false,                                                    // border-width, -style, -color: :151-168
            .m_radius = 0,                                                              // border-radius: 0: :169
            .m_idle = {
                .m_fill = QBrush(AzQtComponents::Foundations::Clear),                   // background-color: rgba(0,0,0,0): :149
                .m_edges = AzQtComponents::BoxElement::border(RingWidth, RingBlue),     // border: :151-168
            },
        };

        //! The box with the ring of the fallbacks, the group title of Landscape Canvas
        const AzQtComponents::BoxElement FallbackRingBox{
            .m_name = "GraphCanvas.Box.CommentEditorFallback",
            .m_hasBox = true,                                                           // margin: 0, padding: 0: :171-172
            .m_nativeBorder = false,                                                    // border-width, -style, -color: :151-168
            .m_radius = 0,                                                              // border-radius: the default 0: :169
            .m_idle = {
                .m_fill = QBrush(AzQtComponents::Foundations::Clear),                   // background-color: rgba(0,0,0,0): :149
                .m_edges = AzQtComponents::BoxElement::border(RingFallbackWidth, RingFallback),     // GetPen: StyleHelper.cpp:226-228
            },
        };

        // ---- The text of the editor

        //! The text of a comment, at the lines of `comment > .commentText` (+30 in Script Canvas's sheet): black (`color`, :255), Open
        //! Sans (`font-family`, :257), 120% of the default size (`font-size`, :258), italic (`font-style`, :259) and the default weight
        //! (`font-weight`, :261), which is the application font's, Foundations::Surface::Weight.
        const AzQtComponents::TextElement CommentText{
            .m_name = "GraphCanvas.Text.CommentEditorComment",
            .m_family = AzQtComponents::Foundations::Surface::Family,                   // font-family: :257
            .m_pixelSize = CommentFontSize,                                             // font-size: :258
            .m_weight = AzQtComponents::Foundations::Surface::Weight,                   // font-weight: :261
            .m_italic = 1,                                                              // font-style: :259
            .m_color = TextColor,                                                       // color: :255
        };

        //! The text of a group title: black, in the default font, with no italic (the rules of `nodeGroup > .commentText` have no font
        //! or colour, so GetFont and GetFontStyleSheet give the defaults)
        const AzQtComponents::TextElement GroupTitleText{
            .m_name = "GraphCanvas.Text.CommentEditorGroupTitle",
            .m_family = AzQtComponents::Foundations::Surface::Family,                   // the default family: StyleHelper.cpp:166
            .m_pixelSize = GroupTitleFontSize,                                          // the default size: StyleHelper.cpp:167
            .m_weight = AzQtComponents::Foundations::Surface::Weight,                   // the default weight: StyleHelper.cpp:168
            .m_color = TextColor,                                                       // the unset colour: StyleHelper.cpp:178-182
        };

        // ---- The choice of the looks

        const char* RingTagOf(const QPen& ring)
        {
            // The sheets give a solid line (border-style, :276 and :332, +30 in Script Canvas's), as the pen's fallback is
            if (ring.style() != Qt::SolidLine)
            {
                return nullptr;
            }

            if (ring.width() == RingWidth && ring.color() == RingOrange)
            {
                return RingOrangeTag;
            }
            if (ring.width() == RingWidth && ring.color() == RingBlue)
            {
                return RingBlueTag;
            }
            if (ring.width() == RingFallbackWidth && ring.color() == RingFallback)
            {
                return RingFallbackTag;
            }
            return nullptr;
        }

        const char* KindTagOf(Kind kind)
        {
            return kind == Kind::Comment ? CommentTag : GroupTitleTag;
        }

        //! A class tag on or off; a tag the editor has already, or lacks already, changes nothing and polishes nothing
        static void SetTag(QWidget* editor, const char* tag, bool on)
        {
            const QString name = QString::fromLatin1(tag);
            if (on)
            {
                AzQtComponents::Style::addClass(editor, name);
            }
            else
            {
                AzQtComponents::Style::removeClass(editor, name);
            }
        }

        void TagEditor(QWidget* editor, const QPen& ring, Kind kind)
        {
            const char* ringTag = RingTagOf(ring);
            SetTag(editor, RingOrangeTag, ringTag == RingOrangeTag);
            SetTag(editor, RingBlueTag, ringTag == RingBlueTag);
            SetTag(editor, RingFallbackTag, ringTag == RingFallbackTag);
            SetTag(editor, CommentTag, kind == Kind::Comment);
            SetTag(editor, GroupTitleTag, kind == Kind::GroupTitle);
        }
    } // namespace CommentTextEditFlat
} // namespace GraphCanvas
