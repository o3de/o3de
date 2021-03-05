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

#include <AzCore/Component/ComponentBus.h>
#include <AzFramework/Physics/Ragdoll.h>

namespace AzFramework
{
    class CharacterPhysicsDataRequests
        : public AZ::ComponentBus
    {
    public:
        virtual ~CharacterPhysicsDataRequests() = default;

        virtual bool GetRagdollConfiguration(Physics::RagdollConfiguration& config) const = 0;
        virtual Physics::RagdollState GetBindPose(const Physics::RagdollConfiguration& config) const = 0;
        virtual AZStd::string GetParentNodeName(const AZStd::string& childName) const = 0;
    };

    using CharacterPhysicsDataRequestBus = AZ::EBus<CharacterPhysicsDataRequests>;

    class CharacterPhysicsDataNotifications
        : public AZ::ComponentBus
    {
    public:
        virtual ~CharacterPhysicsDataNotifications() = default;

        virtual void OnRagdollConfigurationReady() = 0;
    };

    using CharacterPhysicsDataNotificationBus = AZ::EBus<CharacterPhysicsDataNotifications>;
} // namespace AzFramework
