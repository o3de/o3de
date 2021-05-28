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
