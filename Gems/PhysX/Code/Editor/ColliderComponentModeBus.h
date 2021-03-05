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

#include <AzCore/Component/ComponentBus.h>

namespace PhysX
{
    //! Bus used to communicate with component mode
    class ColliderComponentModeRequests
        : public AZ::EntityComponentBus
    {
    public:
        enum class SubMode : AZ::u32
        {
            Dimensions,
            Offset,
            Rotation,
            NumModes
        };

        /// Gets the current sub collider component mode.
        /// @return The mode index.
        virtual SubMode GetCurrentMode() = 0;

        /// Sets the current sub collider component mode.
        /// @param mode The new mode to set.
        virtual void SetCurrentMode(SubMode mode) = 0;
    };

    //! Provides access to Component Mode specific UI options
    class ColliderComponentModeUiRequests : public AZ::EntityComponentBus
    {
    public:
        virtual AzToolsFramework::ViewportUi::ButtonId GetOffsetButtonId() const = 0;
        virtual AzToolsFramework::ViewportUi::ButtonId GetRotationButtonId() const = 0;
        virtual AzToolsFramework::ViewportUi::ClusterId GetClusterId() const = 0;
        virtual AzToolsFramework::ViewportUi::ButtonId GetDimensionsButtonId() const = 0;
    };

    using ColliderComponentModeRequestBus = AZ::EBus<ColliderComponentModeRequests>;
    using ColliderComponentModeUiRequestBus = AZ::EBus<ColliderComponentModeUiRequests>;
}

