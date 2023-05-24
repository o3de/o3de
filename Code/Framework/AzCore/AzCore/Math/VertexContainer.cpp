/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Math/VertexContainer.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/Vector2.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace AZ
{
    void VertexContainerReflect(ReflectContext* context)
    {
        VertexContainer<Vector2>::Reflect(context);
        VertexContainer<Vector3>::Reflect(context);
    }

    AZ_CLASS_ALLOCATOR_IMPL_TEMPLATE(VertexContainer<Vector2>, AZ::SystemAllocator);
    AZ_CLASS_ALLOCATOR_IMPL_TEMPLATE(VertexContainer<Vector3>, AZ::SystemAllocator);
}
