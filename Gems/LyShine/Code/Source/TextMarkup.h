/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include "LyShine.h"
#include <AzCore/std/containers/list.h>

struct IConsoleCmdArgs;

namespace TextMarkup
{
    //! Default color value for font color attribute (represents unassigned state)
    static const AZ::Vector3 ColorInvalid(-1.0f, -1.0f, -1.0f);

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    //! Different tag types supported by TextMarkup
    enum class TagType
    {
        Root,
        Text,
        Bold,
        Italic,
        Anchor,
        Font,
        Image
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    //! Tag base class. All tags can have child tags.
    struct Tag
    {
        Tag() { }

        virtual ~Tag()
        {
            for (auto child : children)
            {
                delete child;
            }
        }

        // Copying and assignment disallowed
        Tag(const Tag&) = delete;
        Tag(const Tag&&) = delete;
        Tag& operator=(const Tag&) = delete;
        Tag& operator=(const Tag&&) = delete;

        //! There should only ever be one root tag in an TextMarkup tree (a root tag should never be a child of another tag).
        virtual TagType GetType() const { return TagType::Root; };

        AZStd::list<Tag*> children;     //!< List of child tags
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    //! Contains text data
    struct TextTag
        : public Tag
    {
        TagType GetType() const override { return TagType::Text; }
        AZStd::string text;
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    //! Indicates that child elements should be bolded
    struct BoldTag
        : public Tag
    {
        TagType GetType() const override { return TagType::Bold; }
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    //! Indicates that child elements should be italicized
    struct ItalicTag
        : public Tag
    {
        TagType GetType() const override { return TagType::Italic; }
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    //! Defines clickable regions of text (links)
    struct AnchorTag
        : public Tag
    {
        TagType GetType() const override { return TagType::Anchor; }
        AZStd::string action;
        AZStd::string data;
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    //! Allows modifying font display properties, such as face and color
    struct FontTag
        : public Tag
    {
        TagType GetType() const override { return TagType::Font; }
        AZStd::string face;
        AZ::Vector3 color;
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    //! Contains data to display an image
    struct ImageTag
        : public Tag
    {
        TagType GetType() const override { return TagType::Image; }
        AZStd::string m_imagePathname;
        AZStd::string m_height; // an absolute value or a string identifying how to calculate the height
        float m_scale;
        AZStd::string m_vAlign;
        float m_yOffset;
        float m_leftPadding;
        float m_rightPadding;
    };

    //! Takes markup text and populates an markup tag tree from it.
    bool ParseMarkupBuffer(const AZStd::string& sourceBuffer, TextMarkup::Tag& markupTag, bool suppressWarnings = false);

    //! Takes a source markup buffer and dumps only the character data to the target buffer.
    void CopyCharData(const AZStd::string& sourceBuffer, AZStd::string& targetBuffer);

#if defined(LYSHINE_INTERNAL_UNIT_TEST)
    void UnitTest(IConsoleCmdArgs* cmdArgs);
#endif
}
