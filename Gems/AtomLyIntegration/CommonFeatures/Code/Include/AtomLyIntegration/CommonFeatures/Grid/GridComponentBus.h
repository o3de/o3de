/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/ComponentBus.h>
#include <AzCore/Math/Color.h>

namespace AZ
{
    namespace Render
    {
        //! GridComponentRequestBus declares an interface for configuring and interacting with the grid component
        class GridComponentRequests : public ComponentBus
        {
        public:
            //! Sets the dimensions of the grid in meters
            virtual void SetSize(float size) = 0;

            //! Returns the dimensions of the grid in meters
            virtual float GetSize() const = 0;

            //! Sets the distance between grid lines drawn in the primary grid color
            virtual void SetPrimarySpacing(float spacing) = 0;

            //! Returns the distance between grid lines drawn and the primary grid color
            virtual float GetPrimarySpacing() const = 0;

            //! Sets the distance between grid lines drawn in the secondary grid color
            virtual void SetSecondarySpacing(float spacing) = 0;

            //! Returns the distance between grid lines drawn in the secondary grid color
            virtual float GetSecondarySpacing() const = 0;

            //! Sets the color of the axis lines spanning the grid dimensions
            virtual void SetAxisColor(const AZ::Color& color) = 0;

            //! Returns the color of the axis lines spanning the grid dimensions
            virtual AZ::Color GetAxisColor() const = 0;

            //! Sets the color of primary grid lines
            virtual void SetPrimaryColor(const AZ::Color& color) = 0;

            //! Returns the color of primary grid lines
            virtual AZ::Color GetPrimaryColor() const = 0;

            //! Sets the color of secondary grid lines
            virtual void SetSecondaryColor(const AZ::Color& color) = 0;

            //! Returns the color of secondary grid lines
            virtual AZ::Color GetSecondaryColor() const = 0;
        };

        using GridComponentRequestBus = EBus<GridComponentRequests>;

        //! GridComponentNotificationBus notifications are triggered whenever the grid changes
        class GridComponentNotifications : public ComponentBus
        {
        public:
            //! Notify any handlers that the grid has been modified
            virtual void OnGridChanged(){};
        };

        using GridComponentNotificationBus = EBus<GridComponentNotifications>;

    } // namespace Render
} // namespace AZ
