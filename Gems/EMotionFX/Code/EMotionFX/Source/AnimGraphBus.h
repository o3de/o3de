/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/EBus/EBus.h>

namespace EMotionFX
{
    class AnimGraphNode;
    class AnimGraphObject;
    class AnimGraphReferenceNode;
    class BlendTree;
    class ValueParameter;

    /**
     * EMotion FX Anim Graph Request Bus
     * Used for making requests to anim graphs.
     */
    class AnimGraphRequests
        : public AZ::EBusTraits
    {
    public:
    };

    using AnimGraphRequestBus = AZ::EBus<AnimGraphRequests>;

    /**
     * EMotion FX Anim Graph Notification Bus
     * Used for monitoring events from anim graphs.
     */
    class AnimGraphNotifications
        : public AZ::EBusTraits
    {
    public:
        /**
         * OnSyncVisualObject can be called from multiple threads when loading happens in a non-GUI thread.
         */
        using MutexType = AZStd::mutex;

        /**
         * Called whenever something inside an object changes that influences the visual graph or any other UI element.
         * @param[in] object The object that changed and requests the UI sync.
         */
        virtual void OnSyncVisualObject(AnimGraphObject* object) { AZ_UNUSED(object); }

        /**
        * Called whenever an attribute changes that influences the visual graph or any other UI element.
        */
        virtual void OnParameterActionTriggered(const ValueParameter* valueParameter) { AZ_UNUSED(valueParameter); }

        /**
        * Called whenever the virtual final node is set on a blend tree.
        */
        virtual void OnVirtualFinalNodeSet(BlendTree* blendTree) { AZ_UNUSED(blendTree); }

        virtual void OnReferenceAnimGraphAboutToBeChanged([[maybe_unused]] AnimGraphReferenceNode* referenceNode) {}
        /**
        * Called whenever the anim graph in a reference node changed.
        */
        virtual void OnReferenceAnimGraphChanged(AnimGraphReferenceNode* referenceNode) { AZ_UNUSED(referenceNode); }
 
    };

    using AnimGraphNotificationBus = AZ::EBus<AnimGraphNotifications>;
} // namespace EMotionFX
