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

#include <AzCore/EBus/EBus.h>

namespace TouchBending
{
    namespace Simulation
    {
        class AsyncSkeletonBuilderRequest
        : public AZ::EBusTraits
        {
        public:
            //////////////////////////////////////////////////////////////////////////
            // EBusTraits overrides
            static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
            static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
            using MutexType = AZStd::recursive_mutex;
            static const bool EnableEventQueue = true;
            //////////////////////////////////////////////////////////////////////////

            virtual ~AsyncSkeletonBuilderRequest() {}

            /** @brief Creates a TouchBending::PhysicalizedSkeleton from a Physics::SpineTree archetype.
             *
             *  Dispatched when a dynamic PhysX actor starts touching a proximity trigger and
             *  The proximity trigger is within e_CullVegDistance from the Camera. Then, asynchronously we build the
             *  Tree.
             *
             *  @param triggerHandle Handle to the proximity trigger that was touched by a collider.
             *  @param worldTransform The original worldTransform of \p triggerHandle. It is passed by Value,
             *         otherwise we would have to acquire a mutex to get the transform when building the tree.             
             *  @param spineTreeArchetype Pointer to the archetype that we should use to build the PhysicalizedSkeleton.
             *
             *  @returns void
             */
            virtual void AsyncBuildSkeleton(Physics::TouchBendingTriggerHandle* triggerHandle, AZ::Transform worldTransform,
                                        AZStd::shared_ptr<Physics::SpineTree> spineTreeArchetype) = 0;
        };

        /// Bus to service the PhysX Trigger Area Component event group.
        using AsyncSkeletonBuilderBus = AZ::EBus<AsyncSkeletonBuilderRequest>;
    } // namespace Physics
}//namespace TouchBending