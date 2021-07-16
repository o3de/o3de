/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
