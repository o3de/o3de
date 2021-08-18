/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

namespace Multiplayer
{
    //! @class IMultiplayerDebug
    //! @brief IMultiplayerDebug provides access to multiplayer debug overlays
    class IMultiplayerDebug
    {
    public:
        AZ_RTTI(IMultiplayerDebug, "{C5EB7F3A-E19F-4921-A604-C9BDC910123C}");

        virtual ~IMultiplayerDebug() = default;

        //! Enables printing of debug text over entities that have significant amount of traffic.
        virtual void ShowEntityBandwidthDebugOverlay() = 0;

        //! Disables printing of debug text over entities that have significant amount of traffic.
        virtual void HideEntityBandwidthDebugOverlay() = 0;
    };
}
