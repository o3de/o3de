/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#pragma once

#include <AzCore/RTTI/RTTI.h>
#include <AzCore/Math/Vector2.h>
#include <AzCore/Math/Color.h>
#include <AzCore/std/string/string_view.h>
#include <AzFramework/Viewport/ViewportId.h>

namespace AzFramework
{
    using FontId = uint32_t;
    static constexpr FontId InvalidFontId = 0xffffffffu;

    enum class TextHorizontalAlignment : uint16_t
    {
        Left,
        Right,
        Center
    };

    enum class TextVerticalAlignment : uint16_t
    {
        Top,
        Bottom,
        Center,
    };

    //! Standard parameters for drawing text on screen
    struct TextDrawParameters
    {
        ViewportId m_drawViewportId = InvalidViewportId; //! Viewport to draw into
        AZ::Vector3 m_position; //! world space position for 3d draws, screen space x,y,depth for 2d.
        AZ::Color   m_color = AZ::Colors::White; //! Color to draw the text
        AZ::Vector2 m_scale = AZ::Vector2(1.0f); //! font scale
        TextHorizontalAlignment m_hAlign = TextHorizontalAlignment::Left; //! Horizontal text alignment
        TextVerticalAlignment m_vAlign = TextVerticalAlignment::Top; //! Vertical text alignment
        bool m_monospace = false; //! disable character proportional spacing
        bool m_depthTest = false; //! Test character against the depth buffer
        bool m_virtual800x600ScreenSize = true; //! Text placement and size are scaled relative to a virtual 800x600 resolution
        bool m_scaleWithWindow = false; //! Font gets bigger as the window gets bigger
        bool m_multiline = true; //! text respects ascii newline characters
    };

    class FontDrawInterface
    {
    public:
        AZ_RTTI(FontDrawInterface, "{545A7C14-CB3E-4A5B-B435-13EA606708EE}");

        FontDrawInterface() = default;
        virtual ~FontDrawInterface() = default;

        virtual void DrawScreenAlignedText2d(
            const TextDrawParameters& params,
            const AZStd::string_view& string) = 0;
        virtual void DrawScreenAlignedText3d(
            const TextDrawParameters& params,
            const AZStd::string_view& string) = 0;
    };

    class FontQueryInterface
    {
    public:
        AZ_RTTI(FontQueryInterface, "{4BDD8520-EBC1-4680-B25E-421BDF31750F}");

        FontQueryInterface() = default;
        virtual ~FontQueryInterface() = default;

        FontId GetFontId(const AZStd::string_view& fontName) const {return FontId(AZ::Crc32(fontName));}
        virtual FontDrawInterface* GetFontDrawInterface(FontId) const = 0;
        virtual FontDrawInterface* GetDefaultFontDrawInterface() const = 0;

    };
} // namespace AzFramework
