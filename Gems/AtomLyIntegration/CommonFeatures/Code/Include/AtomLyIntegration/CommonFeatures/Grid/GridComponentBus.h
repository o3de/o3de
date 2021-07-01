/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
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
        //! GridComponentRequestBus provides an interface to request operations on a GridComponent
        class GridComponentRequests
            : public ComponentBus
        {
        public:
            virtual void SetSize(float gridSize) = 0;
            virtual float GetSize() const = 0;
            virtual void SetPrimarySpacing(float gridPrimarySpacing) = 0;
            virtual float GetPrimarySpacing() const = 0;
            virtual void SetSecondarySpacing(float gridSecondarySpacing) = 0;
            virtual float GetSecondarySpacing() const = 0;

            virtual void SetAxisColor(const AZ::Color& gridAxisColor) = 0;
            virtual AZ::Color GetAxisColor() const = 0;
            virtual void SetPrimaryColor(const AZ::Color& gridPrimaryColor) = 0;
            virtual AZ::Color GetPrimaryColor() const = 0;
            virtual void SetSecondaryColor(const AZ::Color& gridSecondaryColor) = 0;
            virtual AZ::Color GetSecondaryColor() const = 0;
        };

        using GridComponentRequestBus = EBus<GridComponentRequests>;

        //! GridComponent provides an interface for receiving notifications about a GridComponent
        class GridComponentNotifications
            : public ComponentBus
        {
        public:
            virtual void OnGridChanged() {}
        };

        using GridComponentNotificationBus = EBus<GridComponentNotifications>;

    } // namespace Render
} // namespace AZ
