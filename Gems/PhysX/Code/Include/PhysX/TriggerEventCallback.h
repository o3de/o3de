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

namespace physx
{
    struct PxTriggerPair;
}

namespace Physics
{
    /// Created to decouple AzFramework/Physics from PhysX internals.
    class ITriggerEventCallback
    {
    };
}

namespace PhysX
{
    /// A Low Level version of WorldEventHandler. Required for TouchBending
    /// to detect when actors with colliders touch the proximity trigger.
    class IPhysxTriggerEventCallback : public Physics::ITriggerEventCallback
    {
    public:
        /** @brief Notifies that something touched a trigger.
         *
         *  The TouchBending Gem will check if it owns the trigger object.
         *  If so, it will keep track of enter/exit count and kick off the
         *  creation of the Tree.
         *
         *  @param triggerPair As provided by PhysX
         *  @returns TRUE if the callback is related with touch bending, otherwise
         *           return FALSE for further processing by World.cpp
         */
        virtual bool OnTriggerCallback(physx::PxTriggerPair* triggerPair) = 0;
    };
}