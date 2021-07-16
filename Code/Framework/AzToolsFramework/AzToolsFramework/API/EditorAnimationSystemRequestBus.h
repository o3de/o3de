/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

namespace AzToolsFramework
{
    /**
    * This bus allows you to make editor specific requests of the Animation System
    */
    class EditorAnimationSystemRequests : public AZ::EBusTraits
    {
    public:
        virtual ~EditorAnimationSystemRequests() = default;

        enum AnimationSystem
        {
            EMotionFX,
            CryAnimation
        };

        /**
        * Determines if the given animation system is active
        * @param systemType the type of system to query for
        */
        virtual bool IsSystemActive([[maybe_unused]] AnimationSystem systemType) { return false; }
    };

    using EditorAnimationSystemRequestsBus = AZ::EBus<EditorAnimationSystemRequests>;
}
