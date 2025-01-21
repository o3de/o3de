/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/RTTI/RTTI.h>
#include <AzCore/Interface/Interface.h>
#include <AzCore/Math/Color.h>

#include <AzFramework/Viewport/ViewportId.h>

namespace AzToolsFramework
{
    //! An interface for loading simple icon assets and rendering them to screen on a per-viewport basis.
    class EditorViewportIconDisplayInterface
    {
    public:
        AZ_RTTI(EditorViewportIconDisplayInterface, "{D5190B58-2561-4F3F-B793-F1E7D454CDF2}");

        using IconId = AZ::s32;
        static constexpr IconId InvalidIconId = -1;

        enum class CoordinateSpace : AZ::u8
        {
            ScreenSpace,
            WorldSpace
        };

        //! These draw parameters control rendering for a single icon to a single viewport.
        struct DrawParameters
        {
            //! The ViewportId to render to.
            AzFramework::ViewportId m_viewport = AzFramework::InvalidViewportId;
            //! The icon ID, retrieved from GetOrLoadIconForPath, to render to screen.
            IconId m_icon = InvalidIconId;
            //! The color, including opacity, to render the icon with. White will render the icon as opaque in its original color.
            AZ::Color m_color = AZ::Colors::White;
            //! The position to render the icon to, in world or screen space depending on m_positionSpace.
            AZ::Vector3 m_position;
            //! The coordinate system to use for m_position.
            //! ScreenSpace will accept m_position in the form of [X, Y, Depth], where X & Y are screen coordinates in
            //! pixels and Depth is a z-ordering depth value from 0.0f to 1.0f.
            //! WorldSpace will accept a 3D vector in world space coordinates that will be translated back into screen
            //! space when the icon is rendered.
            CoordinateSpace m_positionSpace = CoordinateSpace::ScreenSpace;
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

        //! Draws an icon (immediately) to a viewport given a set of draw parameters.
        //! Requires an IconId retrieved from GetOrLoadIconForPath.
        //! Note that this is an immediate draw request for backward compatability.
        //! For more efficient rendering, use the AddIcon method.
        virtual void DrawIcon(const DrawParameters& drawParameters) = 0;

        //! Adds an icon to the upcoming draw batch.
        //! Use DrawIcons() after adding them all to actually commit them to the renderer.
        //! DrawParameters.m_viewport must be set to a valid viewport and must be the same viewport
        //! as all other invocations of AddIcon since the last call to DrawIcons (do one viewport at a time!)
        virtual void AddIcon(const DrawParameters& drawParameters) = 0;

        //! Call this after adding all of the icons via AddIcon.
        //! This commits all of them to the renderer, in batches organized per icon texture.
        virtual void DrawIcons() = 0;

        //! Retrieves a reusable IconId for an icon at a given path.
        //! This will load the icon, if it has not already been loaded.
        //! @param path should be a relative asset path to an icon image asset.
        //! png and svg icons are currently supported.
        virtual IconId GetOrLoadIconForPath(AZStd::string_view path) = 0;
        //! Gets the current load status of an icon retrieved via GetOrLoadIconForPath.
        virtual IconLoadStatus GetIconLoadStatus(IconId icon) = 0;
    };

    using EditorViewportIconDisplay = AZ::Interface<EditorViewportIconDisplayInterface>;
} //namespace AzToolsFramework
