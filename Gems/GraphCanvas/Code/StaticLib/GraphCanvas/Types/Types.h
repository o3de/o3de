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
#include <qcolor.h>
#include <qfont.h>
#include <qfontinfo.h>
AZ_POP_DISABLE_WARNING

#include <AzCore/Math/Color.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/containers/vector.h>

#include <GraphCanvas/Editor/EditorTypes.h>

namespace GraphCanvas
{
    enum class ConstructType
    {
        Unknown,
        CommentNode,
        BookmarkAnchor,
        NodeGroup
    };    

    enum RootGraphicsItemDisplayState
    {
        // Order of this enum, also determines the priority, and which states
        // are stacked over each other.
        Neutral     = 0,
        Preview,
        PartialDisabled,
        Disabled,
        GroupHighlight,
        Inspection,
        InspectionTransparent,
        Deletion
    };

    enum RootGraphicsItemEnabledState
    {
        ES_Unknown = -1,
        ES_Enabled = 0,

        // Partial Disabled implies that the node will not be acted upon in the current 
        // chain because of a previously disabled node in the chain. But the node itself is still in the 'active' state
        ES_PartialDisabled,

        // A node that has been explicitly disabled and will not run in the specified graph.
        ES_Disabled
    };

    class EnumStringifier
    {
    public:
        static AZStd::string GetConstructTypeString(ConstructType constructType)
        {
            if (constructType == ConstructType::CommentNode)
            {
                return "Comment";
            }
            else if (constructType == ConstructType::BookmarkAnchor)
            {
                return "Bookmark";
            }
            else if (constructType == ConstructType::NodeGroup)
            {
                return "Node Group";
            }

            return "???";
        }
    };
    
    class CandyStripeConfiguration
    {
    public:
        int m_maximumSize = 5;
        int m_minStripes = 1;
        
        // How much to offset the stripe from vertical
        int m_stripeAngle = 10;

        QColor m_stripeColor;

        // Control field for improving visuals and just offsetting the initial drawing point
        int m_initialOffset;
    };

    class PatternFillConfiguration
    {
    public:
        // Controls the minimum number of tile repetitions to fit into the fill area
        // Horizontally
        int m_minimumTileRepetitions = 1;

        // Offset to even rows specified in percent of tile width
        float m_evenRowOffsetPercent = 0.0f;

        // Offset to odd rows specified in percent of tile width
        float m_oddRowOffsetPercent = 0.0f;
    };

    class PatternedFillGenerator
    {
    public:
        // Editor Target
        EditorId m_editorId;

        /// Icon Information
        AZStd::string m_id;
        AZStd::vector< AZStd::string > m_palettes;
        AZStd::vector< QColor > m_colors;

        // Pattern Information
        PatternFillConfiguration m_configuration;
    };

    class FontConfiguration
    {
    public:
        AZ_TYPE_INFO(FontConfiguration, "{6D1FBE30-5BD8-4E8D-9D57-7BE79DAA9CF4}");

        FontConfiguration()
            : m_fontColor(0.0f, 0.0f, 0.0f, 1.0f)
            , m_fontFamily("default")
            , m_pixelSize(-1)
            , m_weight(QFont::Normal)
            , m_style(QFont::Style::StyleNormal)
            , m_verticalAlignment(Qt::AlignmentFlag::AlignTop)
            , m_horizontalAlignment(Qt::AlignmentFlag::AlignLeft)
        {}

        void InitializePixelSize()
        {
            if (m_pixelSize < 0)
            {
                QFont defaultFont(m_fontFamily.c_str());
                QFontInfo defaultFontInfo(defaultFont);

                m_pixelSize = defaultFontInfo.pixelSize();
            }
        }

        AZ::Color               m_fontColor;
        AZStd::string           m_fontFamily;
        AZ::s32                 m_pixelSize; 
        AZ::s32                 m_weight;
        AZ::s32                 m_style;
        AZ::s32                 m_verticalAlignment;
        AZ::s32                 m_horizontalAlignment;
    };
}
