/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/EBus/EBus.h>
#include <EMotionFX/Source/MotionSet.h>


namespace EMotionFX
{
    /**
     * EMotion FX Anim Graph Editor Request Bus
     * Used for making requests to anim graph editor.
     */
    class AnimGraphEditorRequests
        : public AZ::EBusTraits
    {
    public:
        /**
         * Called whenever something inside an object changes that influences the visual graph or any other UI element.
         * @param[in] object The object that changed and requests the UI sync.
         */
        virtual MotionSet* GetSelectedMotionSet() { return nullptr; }

        virtual void UpdateMotionSetComboBox() {}
    };

    using AnimGraphEditorRequestBus = AZ::EBus<AnimGraphEditorRequests>;

    /**
     * EMotion FX Anim Graph Notification Bus
     * Used for monitoring events from the anim graph editor.
     */
    class AnimGraphEditorNotifications
        : public AZ::EBusTraits
    {
    public:
        virtual void OnFocusIn() {}
        virtual void OnShow() {}
    };

    using AnimGraphEditorNotificationBus = AZ::EBus<AnimGraphEditorNotifications>;
} // namespace EMotionFX
