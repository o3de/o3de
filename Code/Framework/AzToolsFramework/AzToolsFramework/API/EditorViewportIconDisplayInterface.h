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
#include <AzCore/Interface/Interface.h>
#include <AzCore/Math/Color.h>

#include <AzFramework/Viewport/ViewportId.h>

namespace AzToolsFramework
{
    //! An interface for loading simple icon assets and rendering them to screen to viewports.
    class EditorViewportIconDisplayInterface
    {
    public:
        AZ_RTTI(EditorViewportIconDisplayInterface, "{D5190B58-2561-4F3F-B793-F1E7D454CDF2}");

        using IconId = AZ::s32;
        static constexpr IconId InvalidIconId = -1;

        //! These draw parameters control rendering for a single icon to a single viewport.
        struct DrawParameters
        {
            //! The viewport ID to render to.
            AzFramework::ViewportId m_viewport = AzFramework::InvalidViewportId;
            //! The icon ID, retrieved from GetOrLoadIconForPath, to render to screen.
            IconId m_icon = InvalidIconId;
            //! The color, including opacity, to render the icon with. White will render the icon as opaque in its original color.
            AZ::Color m_color = AZ::Colors::White;
            //! The position, in world-space, to render the icon to.
            AZ::Vector3 m_position;
            //! The size to render the icon as, in pixels.
            AZ::Vector2 m_size;
        };

        //! The current load status of an icon retrieved by GetOrLoadIconForPath.
        enum class IconLoadStatus : AZ::u8
        {
            Unloaded,
            Loading,
            Loaded,
            Error
        };

        //! Draws an icon to a viewport given a set of draw parameters.
        //! Requires an icon ID retrieved from GetOrLoadIconForPath.
        virtual void DrawIcon(const DrawParameters& drawParameters) = 0;
        //! Retrieves a reusable render ID for an icon at a given path.
        //! This will load the icon, if it has not already been loaded.
        //! path should be a relative asset path to an icon image asset.
        //! png and svg icons are currently supported.
        virtual IconId GetOrLoadIconForPath(AZStd::string_view path) = 0;
        //! Gets the current load status of an icon retrieved via GetOrLoadIconForPath.
        virtual IconLoadStatus GetIconLoadStatus(IconId icon) = 0;
    };

    using EditorViewportIconDisplay = AZ::Interface<EditorViewportIconDisplayInterface>;
} //namespace AzToolsFramework
