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

        //! Select the pre-baked boolean variant (or the base). Call BakeWhiteBox to apply it.
        virtual void SetLiveBoolean([[maybe_unused]] bool enabled) {}

        //! Whether the runtime White Box is currently set to use its baked boolean geometry.
        virtual bool GetLiveBoolean() const
        {
            return false;
        }

        //! Apply the current live-boolean selection: rebuild the render mesh and re-create
        //! the physics collider from the matching pre-baked geometry. This is the "bake"
        //! command callable from Lua at runtime.
        virtual void BakeWhiteBox() {}

    protected:
        ~WhiteBoxComponentRequests() = default;
    };

    using WhiteBoxComponentRequestBus = AZ::EBus<WhiteBoxComponentRequests>;
} // namespace WhiteBox
