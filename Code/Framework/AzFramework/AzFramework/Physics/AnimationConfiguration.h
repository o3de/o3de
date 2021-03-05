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

#include <AzFramework/Physics/Character.h>
#include <AzFramework/Physics/Ragdoll.h>

namespace Physics
{
    /// Configuration for animated physics structures which are more detailed than the character controller.
    /// For example, ragdoll or hit detection configurations.
    class AnimationConfiguration
    {
    public:
        AZ_RTTI(AnimationConfiguration, "{6D53168F-470E-4B41-986A-612506F09B40}");
        AZ_CLASS_ALLOCATOR_DECL

        virtual ~AnimationConfiguration() = default;

        static void Reflect(AZ::ReflectContext* context);

        CharacterColliderConfiguration      m_hitDetectionConfig;
        RagdollConfiguration                m_ragdollConfig;
        CharacterColliderConfiguration      m_clothConfig;
        CharacterColliderConfiguration      m_simulatedObjectColliderConfig;
    };
} // namespace Physics