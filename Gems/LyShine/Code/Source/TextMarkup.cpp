/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "TextMarkup.h"
#include <AzCore/std/containers/stack.h>
#include <AzCore/std/string/conversions.h>
#include <AzCore/std/string/regex.h>

#include <CryPath.h>

namespace
{
    //! Takes an input source string and wraps it for XML parsing
    void InsertMarkup(const AZStd::string& sourceBuffer, AZStd::string& targetBuffer)
    {
        targetBuffer = "<root>" + sourceBuffer + "</root>";

        AZStd::string::size_type pos = targetBuffer.find(">");
        while (pos != AZStd::string::npos)
        {
            ++pos;
            if (pos < targetBuffer.length())
            {
                AZStd::string::value_type nextChar = targetBuffer.at(pos);

                if (nextChar != '<')
                {
                    static const AZStd::string CharStartTag("<ch value=\"");
                    targetBuffer.insert(pos, CharStartTag);
                    pos += CharStartTag.length();

                    pos = targetBuffer.find("<", pos);

                    if (AZStd::string::npos != pos)
                    {
                        static const AZStd::string CharEndTag("\" />");
                        targetBuffer.insert(pos, CharEndTag);
                        pos += CharEndTag.length();
                    }
                }
            }

            pos = targetBuffer.find(">", pos);
        }

        // Newlines need to be escaped or the XML parser could toss them out
        targetBuffer = AZStd::regex_replace(targetBuffer, AZStd::regex("\n"), "\\n");
    }

    //! Takes an TextMarkup::Tag and dumps all character data to the given output string buffer.
    void DumpCharData(const TextMarkup::Tag& markupRootTag, AZStd::string& ouputText)
    {
        AZStd::stack<const TextMarkup::Tag*> tagStack;
        tagStack.push(&markupRootTag);

        while (!tagStack.empty())
        {
            const TextMarkup::Tag* curTag = tagStack.top();
            tagStack.pop();

            auto riter = curTag->children.rbegin();
            for (; riter != curTag->children.rend(); ++riter)
            {
                tagStack.push(*riter);
            }

            TextMarkup::TagType type = curTag->GetType();

            if (type == TextMarkup::TagType::Text)
            {
                const TextMarkup::TextTag* pText = static_cast<const TextMarkup::TextTag*>(curTag);
                ouputText += pText->text;
            }
        }
    }

    //! Serializes a given XML root object to an TextMarkup tag tree.
    bool PopulateTagTreeFromXml(const XmlNodeRef& node, TextMarkup::Tag& markupTag)
    {
        if (!node)
        {
            return false;
        }

        TextMarkup::Tag* newTag = &markupTag;

        if (AZStd::string(node->getTag()) == "b")
        {
            newTag = new TextMarkup::BoldTag();
        }
        else if (AZStd::string(node->getTag()) == "i")
        {
            newTag = new TextMarkup::ItalicTag();
        }
        else if (AZStd::string(node->getTag()) == "a")
        {
            const int numAttributes = node->getNumAttributes();

            if (numAttributes < 1)
            {
                // Expecting at least one attribute
                return false;
            }

            AZStd::string action;
            AZStd::string data;

            for (int i = 0, count = numAttributes; i < count; ++i)
            {
                const char* key = "";
                const char* value = "";
                if (node->getAttributeByIndex(i, &key, &value))
                {
                    if (AZStd::string(key) == "action")
                    {
                        action = value;
                    }
                    else if (AZStd::string(key) == "data")
                    {
                        data = value;
                    }
                    else
                    {
                        // Unexpected font tag attribute
                        return false;
                    }
                }
            }

            TextMarkup::AnchorTag* anchorTag = new TextMarkup::AnchorTag();
            newTag = anchorTag;

            anchorTag->action = action;
            anchorTag->data = data;
        }
        else if (AZStd::string(node->getTag()) == "font")
        {
            const int numAttributes = node->getNumAttributes();

            if (numAttributes <= 0)
            {
                // Expecting at least one attribute
                return false;
            }

            AZStd::string face;
            AZ::Vector3 color(TextMarkup::ColorInvalid);

            for (int i = 0, count = node->getNumAttributes(); i < count; ++i)
            {
                const char* key = "";
                const char* value = "";
                if (node->getAttributeByIndex(i, &key, &value))
                {
                    if (AZStd::string(key) == "face")
                    {
                        face = value;
                    }
                    else if (AZStd::string(key) == "color")
                    {
                        AZStd::string colorValue(value);
                        AZ::StringFunc::TrimWhiteSpace(colorValue, true, true);
                        AZStd::string::size_type ExpectedNumChars = 7;
                        if (ExpectedNumChars == colorValue.size() && '#' == colorValue.at(0))
                        {
                            static const int base16 = 16;
                            static const float normalizeRgbMultiplier = 1.0f / 255.0f;
                            color.SetX(static_cast<float>(AZStd::stoi(colorValue.substr(1, 2), 0, base16)) * normalizeRgbMultiplier);
                            color.SetY(static_cast<float>(AZStd::stoi(colorValue.substr(3, 2), 0, base16)) * normalizeRgbMultiplier);
                            color.SetZ(static_cast<float>(AZStd::stoi(colorValue.substr(5, 2), 0, base16)) * normalizeRgbMultiplier);
                        }
                    }
                    else
                    {
                        // Unexpected font tag attribute
                        return false;
                    }
                }
            }

            TextMarkup::FontTag* fontTag = new TextMarkup::FontTag();
            newTag = fontTag;

            fontTag->face = face;
            fontTag->color = color;
        }
        else if (AZStd::string(node->getTag()) == "img")
        {
            const int numAttributes = node->getNumAttributes();

            if (numAttributes <= 0)
            {
                // Expecting at least one attribute
                return false;
            }

            AZStd::string imagePathname;
            AZStd::string height;
            float scale = 1.0f;
            AZStd::string vAlign;
            float yOffset = 0.0f;
            float leftPadding = 0.0f;
            float rightPadding = 0.0f;

            for (int i = 0, count = node->getNumAttributes(); i < count; ++i)
            {
                const char* key = "";
                const char* value = "";
                if (node->getAttributeByIndex(i, &key, &value))
                {
                    if (AZStd::string(key) == "src")
                    {
                        imagePathname = value;
                    }
                    else if (AZStd::string(key) == "height")
                    {
                        height = value;
                    }
                    else if (AZStd::string(key) == "scale")
                    {
                        scale = AZStd::stof(AZStd::string(value));
                    }
                    else if (AZStd::string(key) == "vAlign")
                    {
                        vAlign = value;
                    }
                    else if (AZStd::string(key) == "yOffset")
                    {
                        yOffset = AZStd::stof(AZStd::string(value));
                    }
                    else if (AZStd::string(key) == "xPadding")
                    {
                        leftPadding = AZStd::stof(AZStd::string(value));
                        rightPadding = leftPadding;
                    }
                    else if (AZStd::string(key) == "lPadding")
                    {
                        leftPadding = AZStd::stof(AZStd::string(value));
                    }
                    else if (AZStd::string(key) == "rPadding")
                    {
                        rightPadding = AZStd::stof(AZStd::string(value));
                    }
                    else
                    {
                        // Unexpected font tag attribute
                        return false;
                    }
                }
            }

            if (imagePathname.empty())
            {
                // Need at least a path to a texture
                return false;
            }

            TextMarkup::ImageTag* imageTag = new TextMarkup::ImageTag();
            newTag = imageTag;

            // Add an extension if it's not there
            const char* extension = PathUtil::GetExt(imagePathname.c_str());
            if (!extension || extension[0] == '\0')
            {
                // Empty path - add the texture extension
                const AZStd::string textureExtension(".dds");
                imagePathname += textureExtension;
            }

            imageTag->m_imagePathname = imagePathname;
            imageTag->m_height = height;
            imageTag->m_scale = scale;
            imageTag->m_vAlign = vAlign;
            imageTag->m_yOffset = yOffset;
            imageTag->m_leftPadding = leftPadding;
            imageTag->m_rightPadding = rightPadding;
        }
        else if (AZStd::string(node->getTag()) == "ch")
        {
            AZStd::string nodeContent;

            const char* key = "";
            const char* value = "";
            if (!node->getAttributeByIndex(0, &key, &value))
            {
                return false;
            }

            if (AZStd::string(key) == "value")
            {
                nodeContent = value;
            }
            else
            {
                // Unexpected attribute
                return false;
            }

            TextMarkup::TextTag* textTag = new TextMarkup::TextTag();
            newTag = textTag;

            textTag->text = nodeContent;
        }
        else if (AZStd::string(node->getTag()) == "root")
        {
            // Ignore
        }
        else
        {
            return false;
        }

        // Guard against a tag adding itself as its own child
        if (newTag != &markupTag)
        {
            markupTag.children.push_back(newTag);
        }

        for (int i = 0, count = node->getChildCount(); i < count; ++i)
        {
            XmlNodeRef child = node->getChild(i);
            if (!PopulateTagTreeFromXml(child, *newTag))
            {
                return false;
            }
        }

        return true;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool TextMarkup::ParseMarkupBuffer(const AZStd::string& sourceBuffer, TextMarkup::Tag& markupTag, bool suppressWarnings)
{
    // First, wrap up the source text to make it parseable XML
    AZStd::string wrappedSourceText(sourceBuffer);
    InsertMarkup(sourceBuffer, wrappedSourceText);

    // Parse the wrapped text as XML
    XmlNodeRef xmlRoot = GetISystem()->LoadXmlFromBuffer(wrappedSourceText.c_str(), wrappedSourceText.length(), false, suppressWarnings);

    if (xmlRoot)
    {
        return PopulateTagTreeFromXml(xmlRoot, markupTag);
    }

    return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void TextMarkup::CopyCharData(const AZStd::string& sourceBuffer, AZStd::string& targetBuffer)
{
    TextMarkup::Tag markupRootTag;
    if (ParseMarkupBuffer(sourceBuffer, markupRootTag))
    {
        DumpCharData(markupRootTag, targetBuffer);
    }

    if (targetBuffer.empty())
    {
        // If, for some reason we couldn't parse the text as XML, we simply
        // assign the source buffer
        targetBuffer = sourceBuffer;
    }
}

#include "Tests/internal/test_TextMarkup.cpp"
