/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>
#include <AzCore/std/containers/vector.h>

namespace AZ
{
    namespace Render
    {
        class EntityReferenceComponentConfig final
            : public ComponentConfig
        {
        public:
            AZ_CLASS_ALLOCATOR(EntityReferenceComponentConfig, SystemAllocator)
            AZ_RTTI(AZ::Render::EntityReferenceComponentConfig, "{12D214C7-878A-48D2-AFDD-4FCFF0BBC876}", AZ::ComponentConfig);

            static void Reflect(ReflectContext* context);

            AZStd::vector<AZ::EntityId> m_entityIdReferences;
        };
    }
}
