/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/ComponentBus.h>

namespace AzToolsFramework
{
    //! Bus used to communicate with shape component mode.
    class ShapeComponentModeRequests
        : public AZ::EntityComponentBus
    {
    public:
        enum class SubMode : AZ::u32
        {
            Dimensions,
            TranslationOffset,
            NumModes
        };

        //! Gets the current shape component mode sub mode.
        virtual SubMode GetShapeSubMode() const = 0;

        //! Sets the current shape component mode sub mode.
        //! @param mode The new sub mode to set.
        virtual void SetShapeSubMode(SubMode mode) = 0;

        //! Resets the UI for the current shape component mode sub mode.
        virtual void ResetShapeSubMode() = 0;
    };

    using ShapeComponentModeRequestBus = AZ::EBus<ShapeComponentModeRequests>;
} // namespace AzToolsFramework

