/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/ComponentBus.h>

namespace WhiteBox
{
    //! WhiteBoxComponent requests, addressed by EntityId so runtime Lua can call them.
    class WhiteBoxComponentRequests : public AZ::ComponentBus
    {
    public:
        // EBusTraits overrides ...
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;

        virtual bool WhiteBoxIsVisible() const = 0;

        //! Show or hide the runtime render mesh (the collider is unaffected), e.g. to fade out a wall the camera has moved behind.
        virtual void SetWhiteBoxVisible([[maybe_unused]] bool visible) {}

        //! Select the pre-baked boolean variant (or the base). Call BakeWhiteBox to apply it.
        virtual void SetLiveBoolean([[maybe_unused]] bool enabled) {}

        //! Whether the runtime White Box is currently set to use its baked boolean geometry.
        virtual bool GetLiveBoolean() const
        {
            return false;
        }

        //! Apply the live-boolean selection to the render mesh and collider.
        //! @param rebakeCollider also rebuild the collider at the entity's current scale.
        virtual void BakeWhiteBox([[maybe_unused]] bool rebakeCollider = false) {}

    protected:
        ~WhiteBoxComponentRequests() = default;
    };

    using WhiteBoxComponentRequestBus = AZ::EBus<WhiteBoxComponentRequests>;
} // namespace WhiteBox
