/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/RTTI/RTTI.h>
#include <AzCore/Math/Vector2.h>
#include <AzCore/Math/Color.h>
#include <AzCore/Math/Matrix3x4.h>
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
        ViewportId m_drawViewportId = InvalidViewportId; //!< Viewport to draw into
        AZ::Vector3 m_position; //!< world space position for 3d draws, screen space x,y,depth for 2d.
        AZ::Color m_color = AZ::Colors::White; //!< Color to draw the text
        unsigned int m_effectIndex = 0; //!< effect index to apply
        AZ::Vector2 m_scale = AZ::Vector2(1.0f); //!< font scale
        float m_textSizeFactor = 12.0f; //!< font size in pixels
        float m_lineSpacing = 1.0f; //!< Spacing between new lines, as a percentage of m_scale.
        TextHorizontalAlignment m_hAlign = TextHorizontalAlignment::Left; //!< Horizontal text alignment
        TextVerticalAlignment m_vAlign = TextVerticalAlignment::Top; //!< Vertical text alignment
        bool m_useTransform = false; //!< Use specified transform
        AZ::Matrix3x4 m_transform = AZ::Matrix3x4::Identity(); //!< Transform to apply to text quads
        bool m_monospace = false; //!< disable character proportional spacing
        bool m_depthTest = false; //!< Test character against the depth buffer
        bool m_virtual800x600ScreenSize = false; //!< Text placement and size are scaled relative to a virtual 800x600 resolution
        bool m_scaleWithWindow = false; //!< Font gets bigger as the window gets bigger
        bool m_multiline = true; //!< text respects ascii newline characters
    };

    class FontDrawInterface
    {
    public:
        AZ_RTTI(FontDrawInterface, "{545A7C14-CB3E-4A5B-B435-13EA606708EE}");

        FontDrawInterface() = default;
        virtual ~FontDrawInterface() = default;

        virtual void DrawScreenAlignedText2d(
            const TextDrawParameters& params,
            AZStd::string_view text) = 0;
        virtual void DrawScreenAlignedText3d(
            const TextDrawParameters& params,
            AZStd::string_view text) = 0;
        virtual AZ::Vector2 GetTextSize(
            const TextDrawParameters& params,
            AZStd::string_view text) = 0;
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
