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

namespace Physics
{
    /// Events dispatched by a ColliderComponent.
    /// A ColliderComponent describes the shape of an entity to the physics system.
    class ColliderComponentEvents
        : public AZ::ComponentBus
    {
    public:
        virtual void OnColliderChanged() {}
    };
    using ColliderComponentEventBus = AZ::EBus<ColliderComponentEvents>;
} // namespace Physics
