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

#include <AzCore/RTTI/TypeInfo.h>
#include <AzCore/Memory/Memory.h>
#include <AzCore/std/containers/array.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>

#include <AzFramework/Physics/Collision/CollisionGroups.h>
#include <AzFramework/Physics/Collision/CollisionLayers.h>

namespace AZ
{
    class ReflectContext;
}

namespace AzPhysics
{
    //! Collision configuration is a convenience storage class for /ref CollisionLayers and /ref CollisionGroups,
    //! as they are frequently used together. It can be retrieved/mutated through
    //! the SystemConfiguration (for global settings) and the SceneConfiguration (for scene modifications).
    class CollisionConfiguration
    {
    public:
        AZ_CLASS_ALLOCATOR_DECL;
        AZ_TYPE_INFO(CollisionConfiguration, "{84059477-BF6E-4421-9AC7-A0A3B27DEA40}");
        static void Reflect(AZ::ReflectContext* context);

        CollisionConfiguration() = default;

        CollisionLayers m_collisionLayers;
        CollisionGroups m_collisionGroups;

        bool operator==(const CollisionConfiguration& other) const;
        bool operator!=(const CollisionConfiguration& other) const;
    };
}
