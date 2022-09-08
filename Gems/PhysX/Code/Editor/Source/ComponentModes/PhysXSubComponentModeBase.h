/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

namespace AZ
{
    class EntityComponentIdPair;
} // namespace AZ

namespace AzToolsFramework::ViewportInteraction
{
    struct MouseInteractionEvent;
} // namespace AzToolsFramework::ViewportInteraction

namespace PhysX
{
    class PhysXSubComponentModeBase
    {
    public:
        virtual ~PhysXSubComponentModeBase() = default;

        //! Called when the mode is entered to initialize the mode.
        //! @param idPair The entity/component id pair.
        virtual void Setup(const AZ::EntityComponentIdPair& idPair) = 0;

        //! Called when the mode needs to refresh it's values.
        //! @param idPair The entity/component id pair.
        virtual void Refresh(const AZ::EntityComponentIdPair& idPair) = 0;

        //! Called when the mode exits to perform cleanup.
        //! @param idPair The entity/component id pair.
        virtual void Teardown(const AZ::EntityComponentIdPair& idPair) = 0;

        //! Called when reset hot key is pressed.
        //! Should reset values in the sub component mode to sensible defaults.
        //! @param idPair The entity/component id pair.
        virtual void ResetValues(const AZ::EntityComponentIdPair& idPair) = 0;

        //! Additional mouse handling by sub-component mode. Does not absorb mouse event.
        virtual void HandleMouseInteraction(
            [[maybe_unused]] const AzToolsFramework::ViewportInteraction::MouseInteractionEvent& mouseInteraction) {};
    };

    //! NullObject implementation.
    //! Use it as a placeholder for unimplemented component modes
    class NullColliderComponentMode : public PhysXSubComponentModeBase
    {
    public:
        void Setup([[maybe_unused]] const AZ::EntityComponentIdPair& idPair) override {}
        void Refresh([[maybe_unused]] const AZ::EntityComponentIdPair& idPair) override{}
        void Teardown([[maybe_unused]] const AZ::EntityComponentIdPair& idPair) override{}
        void ResetValues([[maybe_unused]] const AZ::EntityComponentIdPair& idPair) override{}
    };

} // namespace PhysX
